/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "nearlink_ccp_client.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "if_system_ability_manager.h"
#include "ipc_types.h"
#include "iremote_object.h"
#include "iservice_registry.h"
#include "message_parcel.h"
#include "system_ability_definition.h"

#include "i_nearlink_ccp_client.h"
#include "i_nearlink_host.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace {
/* proxy 失效后的最小重取间隔，避免星闪关闭时每个通话事件都探测一次 host */
constexpr int64_t PROXY_RETRY_INTERVAL_MS = 1000;

sptr<IRemoteObject> GetHostRemote()
{
    sptr<ISystemAbilityManager> samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    NL_CHECK_RETURN_RET(samgrProxy != nullptr, nullptr, "samgrProxy is nullptr.");
    sptr<IRemoteObject> hostRemote = samgrProxy->CheckSystemAbility(NEARLINK_HOST_SYS_ABILITY_ID);
    NL_CHECK_RETURN_RET(hostRemote != nullptr, nullptr, "nearlink host sa is not ready.");
    return hostRemote;
}

bool IsSleEnabled(const sptr<IRemoteObject> &hostRemote)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(INearlinkHost::GetDescriptor()), false, "Write token failed.");
    NL_CHECK_RETURN_RET(
        hostRemote->SendRequest(NearlinkHostInterfaceCode::NL_IS_SLE_ENABLED, data, reply, option) == ERR_NONE,
        false, "SendRequest IS_SLE_ENABLED failed.");
    NL_CHECK_RETURN_RET(reply.ReadInt32() == NL_CCP_SUCCESS, false, "query sle state failed.");
    return reply.ReadBool();
}

sptr<INearlinkCcpClient> GetCcpProxy(const sptr<IRemoteObject> &hostRemote)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(INearlinkHost::GetDescriptor()), nullptr, "Write token failed.");
    NL_CHECK_RETURN_RET(data.WriteString(PROFILE_CCP_SERVER), nullptr, "Write profile name failed.");
    NL_CHECK_RETURN_RET(
        hostRemote->SendRequest(NearlinkHostInterfaceCode::NL_GETPROFILE, data, reply, option) == ERR_NONE,
        nullptr, "SendRequest GET_PROFILE failed.");
    NL_CHECK_RETURN_RET(reply.ReadInt32() == NL_CCP_SUCCESS, nullptr, "GetProfile failed.");
    sptr<IRemoteObject> ccpRemote = reply.ReadRemoteObject();
    NL_CHECK_RETURN_RET(ccpRemote != nullptr, nullptr, "ccp remote is nullptr.");
    return iface_cast<INearlinkCcpClient>(ccpRemote);
}
}  // namespace

struct NearlinkCcpClient::impl : public std::enable_shared_from_this<NearlinkCcpClient::impl> {
public:
    impl() = default;

    ~impl()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        /* 静态单例在进程退出阶段析构，此时不应再访问可能已失效的远端代理；
           死亡监听持有 weak_ptr，对象失效后回调自然跳过 */
        ResetLocked();
    }

    template <typename Call>
    int32_t PushWithRetry(Call call)
    {
        sptr<INearlinkCcpClient> proxy = GetProxy();
        if (proxy == nullptr) {
            /* 星闪关闭或重取间隔内的正常降级，不作为错误打印 */
            HILOGD("ccp proxy is unavailable.");
            return NL_CCP_ERR_UNAVAILABLE_PROXY;
        }
        int32_t ret = call(proxy);
        if (ret == NL_CCP_ERR_SLE_OFF) {
            /* 服务侧判定星闪已关闭：丢弃缓存，下一次推送会重新探测 */
            Invalidate();
            return ret;
        }
        if (ret != NL_CCP_ERR_IPC_TRANS_FAILED) {
            return ret;
        }
        HILOGW("ccp ipc failed, reacquire proxy and retry once.");
        Invalidate();
        proxy = GetProxy();
        NL_CHECK_RETURN_RET(proxy != nullptr, ret, "ccp proxy is unavailable after invalidation.");
        return call(proxy);
    }

private:
    class HostDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit HostDeathRecipient(std::weak_ptr<impl> owner) : owner_(std::move(owner)) {}

        void OnRemoteDied(const wptr<IRemoteObject> &object) override
        {
            (void)object;
            /* 锁住所有者，避免析构后回调访问已释放对象 */
            auto owner = owner_.lock();
            if (owner != nullptr) {
                owner->OnHostDied();
            }
        }

    private:
        std::weak_ptr<impl> owner_;
    };

    sptr<INearlinkCcpClient> GetProxy()
    {
        sptr<IRemoteObject::DeathRecipient> recipient = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (ccpProxy_ != nullptr) {
                return ccpProxy_;
            }
            /* 节流只针对失败：距上次探测失败不足 1s 才跳过，避免星闪关闭时每个事件都探测；
               首探未完成前不阻塞后来者，避免并发调用者被误判为不可用 */
            auto now = std::chrono::steady_clock::now();
            if (now - lastAcquireTime_ < std::chrono::milliseconds(PROXY_RETRY_INTERVAL_MS)) {
                return nullptr;
            }
            if (deathRecipient_ == nullptr) {
                deathRecipient_ = sptr<IRemoteObject::DeathRecipient>(
                    new (std::nothrow) HostDeathRecipient(weak_from_this()));
            }
            recipient = deathRecipient_;
        }

        /* 以下三步是同步跨进程调用，放在锁外：锁内做 IPC 会把探测耗时转嫁给所有
           通话事件线程，也会让 AddDeathRecipient 的同步死亡回调在持锁线程内重入 OnHostDied */
        sptr<IRemoteObject> hostRemote = GetHostRemote();
        if (hostRemote == nullptr || !IsSleEnabled(hostRemote)) {
            MarkProbeFailed();
            return nullptr;
        }
        sptr<INearlinkCcpClient> proxy = GetCcpProxy(hostRemote);
        if (proxy == nullptr) {
            MarkProbeFailed();
            return nullptr;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (ccpProxy_ != nullptr) {
                /* 探测期间其它调用者已完成装配，丢弃本次结果（此时尚未注册监听） */
                return ccpProxy_;
            }
            ccpProxy_ = proxy;
        }

        /* 赋值之后再注册：若宿主在探测期间已死亡，OnRemoteDied 会同步回调并清空刚装配的状态 */
        if (recipient == nullptr || !hostRemote->AddDeathRecipient(recipient)) {
            HILOGW("register host death recipient failed.");
        }
        return proxy;
    }

    /* 探测失败后记账：重取间隔只在这之后生效，首探进行中不拦后来者 */
    void MarkProbeFailed()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        lastAcquireTime_ = std::chrono::steady_clock::now();
    }

    void Invalidate()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        /* 只清本地缓存，不摘死亡监听：锁外 RemoveDeathRecipient 与并发 GetProxy 的
           AddDeathRecipient 交叉时会把刚注册的监听摘掉；保留监听后，旧 host 的迟到
           死亡回调最多多清一次缓存，由下一次事件重取自愈 */
        ccpProxy_ = nullptr;
        /* 置为 epoch 表示无失败记录，下一次调用可立即重取 */
        lastAcquireTime_ = std::chrono::steady_clock::time_point{};
    }

    void OnHostDied()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        /* host 已死亡，无需再摘除死亡监听 */
        ResetLocked();
        lastAcquireTime_ = std::chrono::steady_clock::time_point{};
    }

    void ResetLocked()
    {
        ccpProxy_ = nullptr;
    }

    std::mutex mutex_;
    sptr<INearlinkCcpClient> ccpProxy_;
    sptr<IRemoteObject::DeathRecipient> deathRecipient_;
    std::chrono::steady_clock::time_point lastAcquireTime_ = std::chrono::steady_clock::time_point{};
};

NearlinkCcpClient *NearlinkCcpClient::GetProfile()
{
    /* 通话事件热路径，每个事件至少触发两次，仅保留调试级别日志 */
    HILOGD("enter");
    static NearlinkCcpClient instance;
    return &instance;
}

int32_t NearlinkCcpClient::PhoneStateChanged(int32_t callId, int32_t numActive, int32_t numHeld,
    int32_t callState, const std::string &number)
{
    std::shared_ptr<impl> implPtr = std::atomic_load(&pimpl);
    NL_CHECK_RETURN_RET(implPtr != nullptr, NL_CCP_ERR_UNAVAILABLE_PROXY, "ccp impl is unavailable.");
    return implPtr->PushWithRetry([&](const sptr<INearlinkCcpClient> &proxy) {
        return proxy->PhoneStateChanged(callId, numActive, numHeld, callState, number);
    });
}

int32_t NearlinkCcpClient::CallDetailsChanged(int32_t callId, int32_t callState, int32_t callType)
{
    std::shared_ptr<impl> implPtr = std::atomic_load(&pimpl);
    NL_CHECK_RETURN_RET(implPtr != nullptr, NL_CCP_ERR_UNAVAILABLE_PROXY, "ccp impl is unavailable.");
    return implPtr->PushWithRetry([&](const sptr<INearlinkCcpClient> &proxy) {
        return proxy->CallDetailsChanged(callId, callState, callType);
    });
}

NearlinkCcpClient::NearlinkCcpClient()
{
    HILOGI("enter");
    pimpl = std::make_shared<impl>();
}

NearlinkCcpClient::~NearlinkCcpClient()
{
    HILOGI("enter");
    /* 本对象是进程级静态单例（见 GetProfile），正常只随进程退出析构；
       两个入口用 atomic_load 读取，避免与这里的写入构成数据竞争；
       析构后仍不得再调用公开接口 */
    std::atomic_store(&pimpl, std::shared_ptr<impl>());
}

}  // namespace Nearlink
}  // namespace OHOS
