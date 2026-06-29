/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "nearlink_cdsm_client_server.h"

#include <optional>
#include "ipc_skeleton.h"
#include "remote_observer_list.h"
#include "SleInterfaceProfileCdsm.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceProfileManager.h"
#include "nearlink_utils_server.h"
#include "nearlink_permission_manager.h"
#include "nearlink_device_manager.h"
#include "nearlink_cdsm_info.h"
#include "nearlink_remote_container.h"
#include "nearlink_raw_address.h"
namespace OHOS {
namespace Nearlink {
struct NearlinkCdsmClientServer::impl {
    impl();
    ~impl();

    /* 开关的监听 */
    class SystemStateObserver;
    std::unique_ptr<SystemStateObserver> systemStateObserver_ = nullptr;

    /* 合作集Service的监听 */
    class NearlinkCdsmClientCallbackImpl;
    std::shared_ptr<InterfaceCdsmClientServiceCallback> cdsmClientCallback_ = nullptr;

    /* 外部注册的监听 */
    RemoteObserverList<INearlinkCdsmClientCallback> remoteObservers_;
    /* 外部的注册信息 */
    class CdsmClientRemoteInfo;
    class CdsmClientRemoteContainer;
    // shared_ptr for deathRecipient of container
    std::shared_ptr<CdsmClientRemoteContainer> remoteContainer_ = std::make_shared<CdsmClientRemoteContainer>();

    ProfileCdsm *GetServicePtr()
    {
        return static_cast<ProfileCdsm *>(
                SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    }
};

class NearlinkCdsmClientServer::impl::CdsmClientRemoteInfo {
public:
    CdsmClientRemoteInfo() : reportAddr_(RawAddress(INVALID_MAC_ADDRESS))  {}

    explicit CdsmClientRemoteInfo(const RawAddress &addr, const uint64_t tokenId, bool isUseRealAddrFlag)
        : reportAddr_(addr), tokenId_(tokenId), isUseRealAddrFlag_(isUseRealAddrFlag) {}

    RawAddress reportAddr_ = RawAddress(INVALID_MAC_ADDRESS);
    uint64_t tokenId_ = -1;
    bool isUseRealAddrFlag_ = false;
};

class NearlinkCdsmClientServer::impl::CdsmClientRemoteContainer final
    : public NearlinkRemoteContainer<CdsmClientRemoteInfo>
{
public:
    ~CdsmClientRemoteContainer() override = default;
    void OnRemoteDied(const wptr<IRemoteObject> &remote) override
    {
        HILOGI("[CdsmServer]enter");
        DeleteRemoteInfo(remote);
    }
};

class NearlinkCdsmClientServer::impl::SystemStateObserver final : public ISystemStateObserver {
public:
    explicit SystemStateObserver(impl *impl) : impl_(impl){}
    ~SystemStateObserver() override = default;

    void OnSystemStateChange(const SleSystemState state) override
    {
        HILOGI("[CdsmServer] system state = %{public}d", state);
        NL_CHECK_RETURN(impl_, "[CdsmServer]NearlinkCdsmClientServer::impl invalid.");
        switch (state) {
            case SleSystemState::ON: {
                ProfileCdsm *clientService = impl_->GetServicePtr();
                NL_CHECK_RETURN(clientService, "[CdsmServer]clientService invalid.");
                NL_CHECK_RETURN(impl_->cdsmClientCallback_, "[CdsmServer]clientService cdsmClientCallback invalid.");
                clientService->RegisterCdsmClientCallback(impl_->cdsmClientCallback_);
                break;
            }
            default:
                break;
        }
    }

private:
    impl *impl_ = nullptr;
};

class NearlinkCdsmClientServer::impl::NearlinkCdsmClientCallbackImpl : public InterfaceCdsmClientServiceCallback {
public:
    explicit NearlinkCdsmClientCallbackImpl(impl *impl) : impl_(impl)
    {
        HILOGI("[CdsmServer]Enter");
    }

    ~NearlinkCdsmClientCallbackImpl() override = default;

    void OnCdsmClientStateChange(
        const RawAddress &remoteAddr, std::vector<NearlinkCdsmInfo> &cdsmInfo,
        std::optional<uint64_t> tokenId = std::nullopt) override
    {
        NL_CHECK_RETURN(impl_, "[CdsmServer]NearlinkCdsmClientServer::impl invalid.");
        HILOGI("[CdsmServer]observer size is %{public}d", impl_->remoteObservers_.Size());
        auto fun = [this, tokenId, &remoteAddr, &cdsmInfo](sptr<INearlinkCdsmClientCallback> observer) {
            CdsmClientRemoteInfo info = impl_->remoteContainer_->RetrieveRemoteInfo(observer->AsObject());
            if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, static_cast<uint32_t>(info.tokenId_))) {
                HILOGE("[CdsmServer]check permission failed, tokenId: %{public}lu", info.tokenId_);
                return;
            }
            if (info.reportAddr_ != remoteAddr) {
                return;
            }
            // 如果 tokenId 不是 nullopt，且当前 observer 的 tokenId 不匹配，则不触发回调
            if (tokenId.has_value() && info.tokenId_ != tokenId.value()) {
                uint64_t tid = tokenId.value();
                HILOGW("[CdsmServer]not current app register, do not callback, need tokenId is (%{public}lu)", tid);
                return;
            }

            NearlinkCdsInfoParcel result;
            for (const auto& devInfo : cdsmInfo) {
                NearlinkCdsMemberInfo memberInfo;
                NearlinkRawAddress randomAddr;
                NearlinkDeviceManager::GetInstance()->ConvertToRandomAddress(
                    info.isUseRealAddrFlag_, devInfo.addr_, randomAddr, false);
                memberInfo.SetDeviceAddr(randomAddr.GetAddress());
                memberInfo.SetState(devInfo.state_);
                result.AddCdsMemberInfo(memberInfo);
            }
            HILOGI("[CdsmServer]do OnCdsInfoChanged size=%{public}zu", result.GetCdsMemberList().size());
            observer->OnCdsInfoChanged(result);
        };
        impl_->remoteObservers_.ForEach(fun);
    }

private:
    impl *impl_ = nullptr;
};

NearlinkCdsmClientServer::impl::impl()
{
    HILOGI("[CdsmServer]Enter");

    cdsmClientCallback_ = std::make_shared<NearlinkCdsmClientCallbackImpl>(this);

    ProfileCdsm *clientService = GetServicePtr();
    NL_CHECK_RETURN(clientService, "[CdsmServer]clientService invalid.");
    clientService->RegisterCdsmClientCallback(cdsmClientCallback_);

    systemStateObserver_ = std::make_unique<SystemStateObserver>(this);
    NL_CHECK_RETURN(systemStateObserver_, "[CdsmServer]make systemStateObserver invalid.");
    SleInterfaceManager::GetInstance()->RegisterSystemStateObserver(*systemStateObserver_);

    remoteContainer_->Init();
}

NearlinkCdsmClientServer::impl::~impl()
{
    HILOGI("[CdsmServer]Enter");
}

NearlinkCdsmClientServer::NearlinkCdsmClientServer()
{
    HILOGI("[CdsmServer]Enter");
    pimpl = std::make_unique<impl>();
    NL_CHECK_RETURN(pimpl, "[CdsmServer]make impl invalid.");
}

NearlinkCdsmClientServer::~NearlinkCdsmClientServer()
{
    HILOGI("[CdsmServer]Enter");
}

NlErrCode NearlinkCdsmClientServer::RegisterCdsmClientCallback(const NearlinkRawAddress &addr,
    const sptr<INearlinkCdsmClientCallback> &callback)
{
    HILOGI("[CdsmServer]RegisterCdsmCallback enter: address: %{public}s", GetEncryptAddr(addr.GetAddress()).c_str());
    NL_CHECK_RETURN_RET(callback, NL_ERR_INVALID_PARAM, "[CdsmServer]callback is null.");
    NL_CHECK_RETURN_RET(pimpl->remoteObservers_.Size() < MAX_OBSERVER_SIZE,
        NL_ERR_INTERNAL_ERROR, "[CdsmServer]ranging observers exceeds the range");

    RawAddress realAddr = addr;
    NearlinkDeviceManager::GetInstance()->GetDeviceRealAddr(addr, realAddr);
    bool isUseRealAddrFlag = NearLinkPermissionManager::IsUseRealAddr();
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();

    pimpl->remoteObservers_.Register(callback);
    impl::CdsmClientRemoteInfo info(realAddr, tokenId, isUseRealAddrFlag);
    pimpl->remoteContainer_->AddRemoteInfo(callback->AsObject(), info);

    // 注册成功以后，触发一次回调的变化
    // 由于合作集的业务流程设计中，外部总是在一只耳机连接后，才可以识别到该设备为合作集，开始注册回调，因此这里需要主动触发一次
    ProfileCdsm *cdsmService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(cdsmService, NL_ERR_INTERNAL_ERROR, "[CdsmServer]cdsmService invalid.");
    cdsmService->HandleCdsmClientCallback(realAddr, tokenId);

    HILOGI("[CdsmServer]RegisterCdsmClientCallback success.");
    return NL_NO_ERROR;
}

NlErrCode NearlinkCdsmClientServer::DeregisterCdsmClientCallback(const NearlinkRawAddress &addr)
{
    HILOGI("[CdsmServer]Enter address: %{public}s", GetEncryptAddr(addr.GetAddress()).c_str());
    RawAddress realAddr = addr;
    NearlinkDeviceManager::GetInstance()->GetDeviceRealAddr(addr, realAddr);
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    sptr<INearlinkCdsmClientCallback> callBack = nullptr;
    pimpl->remoteObservers_.ForEach([this, &realAddr, tokenId, &callBack](sptr<INearlinkCdsmClientCallback> observer) {
        impl::CdsmClientRemoteInfo info = pimpl->remoteContainer_->RetrieveRemoteInfo(observer->AsObject());
        if (realAddr == info.reportAddr_ && tokenId == info.tokenId_) {
            HILOGI("[CdsmServer]find its callback");
            callBack = observer;
        }
    });
    NL_CHECK_RETURN_RET(callBack, NL_ERR_INVALID_PARAM, "[CdsmServer]callback is null.");
    pimpl->remoteObservers_.Deregister(callBack);
    pimpl->remoteContainer_->DeleteRemoteInfo(callBack->AsObject());
    HILOGI("[CdsmServer]DeregisterApplication success");
    return NL_NO_ERROR;
}

NlErrCode NearlinkCdsmClientServer::GetCdsmInfo(const NearlinkRawAddress &addr, NearlinkCdsInfoParcel &cdsInfo)
{
    HILOGI("[CdsmServer]address: %{public}s", GetEncryptAddr(addr.GetAddress()).c_str());
    RawAddress realAddr(addr);
    NearlinkDeviceManager::GetInstance()->GetDeviceRealAddr(addr, realAddr);

    ProfileCdsm *cdsmService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(cdsmService, NL_ERR_INTERNAL_ERROR, "[CdsmServer]cdsmService invalid.");

    bool isUseRealAddr = NearLinkPermissionManager::IsUseRealAddr();
    std::vector<NearlinkCdsmInfo> cdsmInfo = {};
    cdsmService->CdsmGetAllMemberInfo(realAddr, cdsmInfo);
    for (const auto &member : cdsmInfo) {
        NearlinkCdsMemberInfo memberInfo;
        NearlinkRawAddress randomAddr;
        NearlinkDeviceManager::GetInstance()->ConvertToRandomAddress(isUseRealAddr, member.addr_, randomAddr, false);
        memberInfo.SetDeviceAddr(randomAddr.GetAddress());
        memberInfo.SetState(member.state_);
        cdsInfo.AddCdsMemberInfo(memberInfo);
    }
    return NL_NO_ERROR;
}

}  // namespace Nearlink
}  // namespace OHOS