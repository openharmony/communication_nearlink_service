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

#include "ipc_skeleton.h"
#include "remote_observer_list.h"
#include "log_util.h"
#include "interface_hadm_client_service.h"
#include "SleInterfaceManager.h"
#include "nearlink_utils_server.h"
#include "SleInterfaceAdapterSub.h"
#include "nearlink_remote_container.h"
#include "nearlink_hadm_client_server.h"
#include "nearlink_common_event_helper.h"
#include "nearlink_permission_manager.h"
#include "nearlink_device_manager.h"

namespace OHOS {
namespace Nearlink {

struct NearlinkHadmClientServer::impl {
    impl();
    ~impl();

    class SystemStateObserver;
    std::unique_ptr<SystemStateObserver> systemStateObserver_ = nullptr;

    class NearlinkHadmClientCallback;
    RemoteObserverList<INearlinkHadmClientCallback> remoteObservers_;
    std::shared_ptr<NearlinkHadmClientCallback> serviceObserverImp_ =
        std::make_shared<NearlinkHadmClientCallback>(this);

    struct HadmClientRemoteInfo;
    class HadmClientRemoteContainer;
    // weak for deathReipient of container
    std::shared_ptr<HadmClientRemoteContainer> remoteContainer_ =
        std::make_shared<HadmClientRemoteContainer>();
};

struct NearlinkHadmClientServer::impl::HadmClientRemoteInfo {
    HadmClientRemoteInfo () : pid(0), uid(0), tokenId_(0), hadmId_(SLE_HADM_INVALID_ID)  {}
    HadmClientRemoteInfo (int32_t pid, int32_t uid, uint64_t tokenId, uint32_t hadmId)
        : pid(pid), uid(uid), tokenId_(tokenId), hadmId_(hadmId) {}
    virtual ~HadmClientRemoteInfo() = default;

    int32_t pid = 0;
    int32_t uid = 0;
    uint64_t tokenId_ = 0;
    uint32_t hadmId_ = 0;
};

class NearlinkHadmClientServer::impl::HadmClientRemoteContainer final
    : public NearlinkRemoteContainer<NearlinkHadmClientServer::impl::HadmClientRemoteInfo> {
public:
    ~HadmClientRemoteContainer() override = default;
    void OnRemoteDied(const wptr<IRemoteObject> &remote) override
    {
        HILOGI("enter");
        uint32_t hadmId = GetHadmId(remote);
        NL_CHECK_RETURN(hadmId != SLE_HADM_INVALID_ID, "remote info unexpectedly not found");
        DeleteRemoteInfo(remote);
        InterfaceHadmClientService::GetInstance().StopSoundingById(hadmId);
        InterfaceHadmClientService::GetInstance().RemoveHadmId(hadmId);
    }

    uint32_t GetHadmId(const wptr<IRemoteObject> &remote) {
        uint32_t hadmId = SLE_HADM_INVALID_ID;
        std::lock_guard<std::mutex> lk(vecMutex_);
        auto it = std::find_if(vec_.begin(), vec_.end(), [remote](const auto &obj) { return obj.first == remote; });
        if (it != vec_.end()) {
            hadmId = it->second.hadmId_;
            HILOGI("sle hadm remote die id:%{public}u", hadmId);
        }
        return hadmId;
    }

    bool CheckHadmId(uint32_t hadmId)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        int32_t pid = IPCSkeleton::GetCallingPid();
        int32_t uid = IPCSkeleton::GetCallingUid();
        auto it = std::find_if(vec_.begin(), vec_.end(), [hadmId](const auto &obj) {
            return obj.second.hadmId_ == hadmId; });
        NL_CHECK_RETURN_RET(it != vec_.end(), false, "can't find hadmId: %{public}u", hadmId);
        bool ret = (it->second.pid == pid && it->second.uid == uid);
        NL_CHECK_RETURN_RET(ret, ret, "hadmId: %{public}u, pid: %{public}d, uid: %{public}d", hadmId, pid, uid);
        return ret;
    }
};

class NearlinkHadmClientServer::impl::NearlinkHadmClientCallback : public InterfaceHadmClientServiceCallback {
public:
    explicit NearlinkHadmClientCallback(NearlinkHadmClientServer::impl *pimpl) : pimpl_(pimpl) {};
    ~NearlinkHadmClientCallback() override = default;

    void OnSoundingResult(const RawAddress &addr, const NearlinkHadmSoundingResult &result, uint32_t hadmId) override
    {
        HILOGI("sle hadm server addr=%{public}s, id=%{public}d ",
            GetEncryptAddr(addr.GetAddress()).c_str(), hadmId);
        remoteObservers_->ForEach([this, addr, result, hadmId](INearlinkHadmClientCallback *observer) {
            HadmClientRemoteInfo info = pimpl_->remoteContainer_->RetrieveRemoteInfo(observer->AsObject());
            NL_CHECK_RETURN(info.hadmId_ != SLE_HADM_INVALID_ID, "cannot retrieve remote info");
            if (info.hadmId_ == hadmId) {
                bool isUseRealAddrFlag =
                    NearLinkPermissionManager::VerifyPermission(GET_NEARLINK_PEER_MAC, info.tokenId_);
                NearlinkRawAddress address(addr);
                NearlinkRawAddress randomAddr;
                NearlinkDeviceManager::GetInstance()->ConvertToRandomAddress(isUseRealAddrFlag, address, randomAddr,
                                                                             false);
                NearlinkHadmClientSoundingResult soundingResult(result);
                observer->OnSoundingResult(randomAddr, soundingResult);
            }
        });
    }

    void OnSoundingStateChange(const RawAddress &addr, int newState, int errorCode, uint32_t hadmId) override
    {
        HILOGI("sle hadm server addr=%{public}s, id=%{public}d ",
            GetEncryptAddr(addr.GetAddress()).c_str(), hadmId);
        remoteObservers_->ForEach(
            [this, addr, newState, errorCode, hadmId](INearlinkHadmClientCallback *observer) {
            HadmClientRemoteInfo info = pimpl_->remoteContainer_->RetrieveRemoteInfo(observer->AsObject());
            NL_CHECK_RETURN(info.hadmId_ != SLE_HADM_INVALID_ID, "cannot retrieve remote info");
            if (info.hadmId_ == hadmId) {
                bool isUseRealAddrFlag =
                    NearLinkPermissionManager::VerifyPermission(GET_NEARLINK_PEER_MAC, info.tokenId_);
                NearlinkRawAddress address(addr);
                NearlinkRawAddress randomAddr;
                NearlinkDeviceManager::GetInstance()->ConvertToRandomAddress(isUseRealAddrFlag, address, randomAddr,
                                                                             false);
                observer->OnSoundingStateChange(randomAddr, newState, errorCode);
                NearlinkHelper::NearlinkCommonEventHelper::PublishSleRangingEvent(newState,
                    NearLinkPermissionManager::GetCallingName(info.tokenId_));
            }
        });
    }

    void SetObserver(RemoteObserverList<INearlinkHadmClientCallback> *observers)
    {
        remoteObservers_ = observers;
    }

private:
    RemoteObserverList<INearlinkHadmClientCallback> *remoteObservers_  = nullptr;;
    NearlinkHadmClientServer::impl *pimpl_ = nullptr;
};

class NearlinkHadmClientServer::impl::SystemStateObserver : public ISystemStateObserver {
public:
    explicit SystemStateObserver(NearlinkHadmClientServer::impl *impl) : pimpl_(impl){};
    ~SystemStateObserver() override = default;

    void OnSystemStateChange(const SleSystemState state) override
    {
        switch (state) {
            case SleSystemState::ON:
                InterfaceHadmClientService::GetInstance().
                    RegisterNearlinkHadmClientCallback(pimpl_->serviceObserverImp_);
                break;
            case SleSystemState::OFF:
                break;
            default:
                break;
        }
    }

private:
    NearlinkHadmClientServer::impl *pimpl_;
};

NearlinkHadmClientServer::impl::impl()
{
    InterfaceHadmClientService::GetInstance().
        RegisterNearlinkHadmClientCallback(serviceObserverImp_);
    remoteContainer_->Init();
}

NearlinkHadmClientServer::impl::~impl()
{}

NearlinkHadmClientServer::NearlinkHadmClientServer()
{
    pimpl = std::make_unique<impl>();
    pimpl->serviceObserverImp_->SetObserver(&(pimpl->remoteObservers_));
    pimpl->systemStateObserver_ = std::make_unique<impl::SystemStateObserver>(pimpl.get());
    SleInterfaceManager::GetInstance()->RegisterSystemStateObserver(*(pimpl->systemStateObserver_));
}

NearlinkHadmClientServer::~NearlinkHadmClientServer()
{}

NlErrCode NearlinkHadmClientServer::RegisterNearlinkHadmClientCallback(uint32_t &hadmId,
    const sptr<INearlinkHadmClientCallback> &callback)
{
    HILOGI("enter");
    int32_t pid = IPCSkeleton::GetCallingPid();
    int32_t uid = IPCSkeleton::GetCallingUid();
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    NL_CHECK_RETURN_RET(callback, NL_ERR_INVALID_PARAM, "callback is null");
    NL_CHECK_RETURN_RET(pimpl, NL_ERR_IMPL_ERROR, "pimpl is null");
    NL_CHECK_RETURN_RET(pimpl->remoteObservers_.Size() < MAX_OBSERVER_SIZE,
        NL_ERR_INTERNAL_ERROR, "ranging observers exceeds the range");
    hadmId = InterfaceHadmClientService::GetInstance().AllocHadmId();
    NL_CHECK_RETURN_RET(hadmId != SLE_HADM_INVALID_ID, NL_ERR_INTERNAL_ERROR, "alloc hadmId failed.");
    HILOGI("hadmId: %{public}u, pid: %{public}d, uid: %{public}d, tokenId: %{public}lu", hadmId, pid, uid, tokenId);
    
    pimpl->remoteObservers_.Register(callback);
    impl::HadmClientRemoteInfo info(pid, uid, tokenId, hadmId);
    pimpl->remoteContainer_->AddRemoteInfo(callback->AsObject(), info);
    return NL_NO_ERROR;
}

NlErrCode NearlinkHadmClientServer::DeregisterNearlinkHadmClientCallback(uint32_t hadmId,
    const sptr<INearlinkHadmClientCallback> &callback)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(callback, NL_ERR_INVALID_PARAM, "callback is null");
    NL_CHECK_RETURN_RET(pimpl, NL_ERR_IMPL_ERROR, "pimpl is null");
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->CheckHadmId(hadmId), NL_ERR_INVALID_PARAM,
        "hadmId is invalid.");
    pimpl->remoteObservers_.Deregister(callback);
    pimpl->remoteContainer_->DeleteRemoteInfo(callback->AsObject());
    InterfaceHadmClientService::GetInstance().RemoveHadmId(hadmId);
    return NL_NO_ERROR;
}

NlErrCode NearlinkHadmClientServer::StartSounding(uint32_t hadmId, const NearlinkRawAddress &addr)
{
    RawAddress realAddr;
    NearlinkDeviceManager::GetInstance()->GetDeviceRealAddr(addr, realAddr);
    HILOGI("sle hadm start address:%{public}s, hadmId:%{public}u",
        GetEncryptAddr(addr.GetAddress()).c_str(), hadmId);
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->CheckHadmId(hadmId), NL_ERR_INVALID_PARAM,
        "hadmId is invalid.");
    SleInterfaceAdapterSub *sleService = static_cast<SleInterfaceAdapterSub *>
        (SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN_RET(sleService, NL_ERR_INTERNAL_ERROR, "sleService invalid.");
    NL_CHECK_RETURN_RET(sleService->IsAcbConnected(realAddr), NL_ERR_DEVICE_DISCONNECTED, "device disconnected.");
    InterfaceHadmClientService::GetInstance().StartSounding(hadmId, realAddr);
    return NL_NO_ERROR;
}

NlErrCode NearlinkHadmClientServer::StopSounding(uint32_t hadmId, const NearlinkRawAddress &addr)
{
    RawAddress realAddr;
    NearlinkDeviceManager::GetInstance()->GetDeviceRealAddr(addr, realAddr);
    HILOGI("sle hadm stop address:%{public}s, hadmId:%{public}u",
        GetEncryptAddr(addr.GetAddress()).c_str(), hadmId);
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->CheckHadmId(hadmId), NL_ERR_INVALID_PARAM,
        "hadmId is invalid.");
    SleInterfaceAdapterSub *sleService = static_cast<SleInterfaceAdapterSub *>
        (SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN_RET(sleService, NL_ERR_INTERNAL_ERROR, "sleService invalid.");
    NL_CHECK_RETURN_RET(sleService->IsAcbConnected(realAddr), NL_ERR_DEVICE_DISCONNECTED, "device disconnected.");
    InterfaceHadmClientService::GetInstance().StopSounding(hadmId, realAddr);
    return NL_NO_ERROR;
}

NlErrCode NearlinkHadmClientServer::GetHadmFeature(uint8_t &capability)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(pimpl, NL_ERR_IMPL_ERROR, "pimpl is null");
    int status = SleInterfaceManager::GetInstance()->GetState(SleTransport::ADAPTER_SLE);
    if (status != SleStateID::STATE_TURN_ON && status != SleStateID::STATE_TURN_HALF) {
        HILOGE("nearlink is off, get capability failed");
        return NL_ERR_SLE_OFF;
    }
    capability = InterfaceHadmClientService::GetInstance().GetHadmFeature();
    HILOGI("sle hadm local capability=%{public}u", capability);
    return NL_NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS