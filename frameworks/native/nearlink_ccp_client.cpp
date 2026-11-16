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

#include <cstdint>
#include <memory>
#include <string>

#include "i_nearlink_ccp_client.h"
#include "log.h"
#include "nearlink_base_host.h"
#include "nearlink_base_sa_manager.h"

namespace OHOS {
namespace Nearlink {

struct NearlinkCcpClient::impl {};

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
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleEnabled(), NL_CCP_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkCcpClient> proxy = GetProxy<INearlinkCcpClient>(PROFILE_CCP_SERVER);
    if (proxy == nullptr) {
        /* 服务侧未提供 ccp profile 时的正常降级，不作为错误打印 */
        HILOGD("ccp proxy is unavailable.");
        return NL_CCP_ERR_UNAVAILABLE_PROXY;
    }
    int32_t ret = proxy->PhoneStateChanged(callId, numActive, numHeld, callState, number);
    if (ret != NL_CCP_ERR_IPC_TRANS_FAILED) {
        return ret;
    }
    HILOGW("ccp ipc failed, reacquire proxy and retry once.");
    proxy = GetProxy<INearlinkCcpClient>(PROFILE_CCP_SERVER);
    NL_CHECK_RETURN_RET(proxy != nullptr, ret, "ccp proxy is unavailable after reacquire.");
    return proxy->PhoneStateChanged(callId, numActive, numHeld, callState, number);
}

int32_t NearlinkCcpClient::CallDetailsChanged(int32_t callId, int32_t callState, int32_t callType)
{
    std::shared_ptr<impl> implPtr = std::atomic_load(&pimpl);
    NL_CHECK_RETURN_RET(implPtr != nullptr, NL_CCP_ERR_UNAVAILABLE_PROXY, "ccp impl is unavailable.");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleEnabled(), NL_CCP_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkCcpClient> proxy = GetProxy<INearlinkCcpClient>(PROFILE_CCP_SERVER);
    if (proxy == nullptr) {
        /* 服务侧未提供 ccp profile 时的正常降级，不作为错误打印 */
        HILOGD("ccp proxy is unavailable.");
        return NL_CCP_ERR_UNAVAILABLE_PROXY;
    }
    int32_t ret = proxy->CallDetailsChanged(callId, callState, callType);
    if (ret != NL_CCP_ERR_IPC_TRANS_FAILED) {
        return ret;
    }
    HILOGW("ccp ipc failed, reacquire proxy and retry once.");
    proxy = GetProxy<INearlinkCcpClient>(PROFILE_CCP_SERVER);
    NL_CHECK_RETURN_RET(proxy != nullptr, ret, "ccp proxy is unavailable after reacquire.");
    return proxy->CallDetailsChanged(callId, callState, callType);
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
