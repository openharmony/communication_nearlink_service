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

#include "nearlink_tws_client_server.h"
#include "SleInterfaceProfileManager.h"
#include "SleInterfaceProfileTws.h"
#include "SleInterfaceManager.h"
#include "ipc_skeleton.h"
#include "log_util.h"
#include "nearlink_tws_client_observer_proxy.h"
#include "nearlink_safe_list.h"
#include "nearlink_safe_map.h"
#include "nearlink_permission_manager.h"
#include "nearlink_remote_container.h"

namespace OHOS {
namespace Nearlink {

struct NearlinkTwsClientServer::impl {
    class TwsClientObserverImpl;

    impl();
    ~impl();

    /* 外部的注册信息 */
    class TwsClientRemoteInfo;
    class TwsClientRemoteContainer;
    // shared_ptr for deathRecipient of container
    std::shared_ptr<TwsClientRemoteContainer> remoteContainer_;

    ProfileTws *GetServicePtr()
    {
        return static_cast<ProfileTws *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_TWS));
    }
};

class NearlinkTwsClientServer::impl::TwsClientRemoteInfo {
public:
    TwsClientRemoteInfo() : tokenId_(0) {}

    explicit TwsClientRemoteInfo(const uint64_t tokenId) : tokenId_(tokenId) {}

    uint64_t tokenId_ = 0;
};

class NearlinkTwsClientServer::impl::TwsClientRemoteContainer final
    : public NearlinkRemoteContainer<TwsClientRemoteInfo> {
public:
    TwsClientRemoteContainer() = default;
    ~TwsClientRemoteContainer() override = default;

    void OnRemoteDied(const wptr<IRemoteObject> &remote) override
    {
        HILOGI("enter TwsClientRemoteContainer OnRemoteDied");
        //找到remoteObject对应的observer
        twsObservers_.FindAndRmv([&remote](const sptr<INearlinkTwsClientObserver> &observer){
            if(observer->AsObject() == remote){
                HILOGI("TwsClientRemoteContainer OnRemoteDied deleted observer");
                return true;
            }
            return false;
        });
        DeleteRemoteInfo(remote);
    }

    NearlinkSafeList<sptr<INearlinkTwsClientObserver>> twsObservers_;
};

class NearlinkTwsClientServer::impl::TwsClientObserverImpl : public InterfaceTwsClientObserver {
public:
    TwsClientObserverImpl(NearlinkTwsClientServer::impl *impl) : impl_(impl){};
    ~TwsClientObserverImpl() override = default;

    void OnTwsRemoteInfo(const std::string &address, const std::vector<uint8_t> &value) override
    {
        HILOGI("NearlinkTwsClientServer OnTwsRemoteInfo address: %{public}s", GetEncryptAddr(address).c_str());
        if (impl_->remoteContainer_ == nullptr) {
            HILOGE("remoteContainer is nullptr!");
            return;
        }
        impl_->remoteContainer_->twsObservers_.Iterate(
            [this, &address, &value](sptr<INearlinkTwsClientObserver> observer) -> void {
            TwsClientRemoteInfo info = impl_->remoteContainer_->RetrieveRemoteInfo(observer->AsObject());
            if (info.tokenId_ == 0LL || !NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK, info.tokenId_)) {
                HILOGE("false, check permission failed");
                return;
            }
            observer->OnTwsRemoteInfo(address, value);
        });
    }

private:
    NearlinkTwsClientServer::impl *impl_ = nullptr;
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(TwsClientObserverImpl);
};


NearlinkTwsClientServer::impl::impl()
{
    HILOGI("starts");
    ProfileTws *twsService =
        static_cast<ProfileTws *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_TWS));
    NL_CHECK_RETURN(twsService, "twsService is null");

    int ret = twsService->RegisterApplication(std::make_unique<TwsClientObserverImpl>(this));
    if (ret != NL_NO_ERROR) {
        HILOGE("RegisterApplication failed");
    }
    remoteContainer_ = std::make_shared<TwsClientRemoteContainer>();
    remoteContainer_->Init();
}

NearlinkTwsClientServer::impl::~impl()
{}

NearlinkTwsClientServer::NearlinkTwsClientServer() : pimpl(std::make_unique<impl>())
{
    HILOGI("NearlinkTwsClientServer called.");
}

NearlinkTwsClientServer::~NearlinkTwsClientServer()
{
    HILOGI("~NearlinkTwsClientServer called.");
}

NlErrCode NearlinkTwsClientServer::RegisterApplication(const sptr<INearlinkTwsClientObserver> &observer)
{
    HILOGD("RegisterApplication");
    if (observer == nullptr || pimpl->remoteContainer_ == nullptr) {
        HILOGE("observer or remoteContainer is nullptr!");
        return NL_ERR_INVALID_PARAM;
    }
    pimpl->remoteContainer_->twsObservers_.Insert(observer);
    impl::TwsClientRemoteInfo info(IPCSkeleton::GetCallingFullTokenID());
    pimpl->remoteContainer_->AddRemoteInfo(observer->AsObject(), info);

    HILOGI("[NearlinkTwsClientServer]Register Application success");
    return NL_NO_ERROR;
}

NlErrCode NearlinkTwsClientServer::DeregisterApplication(const sptr<INearlinkTwsClientObserver> &observer)
{
    HILOGI("DeregisterApplication");
    if (observer == nullptr || pimpl->remoteContainer_ == nullptr) {
        HILOGE("observer or remoteContainer is nullptr!");
        return NL_ERR_INVALID_PARAM;
    }
    pimpl->remoteContainer_->twsObservers_.Erase(observer);
    pimpl->remoteContainer_->DeleteRemoteInfo(observer->AsObject());

    HILOGI("[NearlinkTwsClientServer]Deregister Application success");
    return NL_NO_ERROR;
}

NlErrCode NearlinkTwsClientServer::EnableWearDetection(const std::string &address)
{
    HILOGI("address: %{public}s", GetEncryptAddr(address).c_str());
    ProfileTws *twsService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(twsService, NL_ERR_INTERNAL_ERROR, "twsService is nullptr.");
    RawAddress device(address);
    twsService->TwsEnableWearDetection(device);
    return NL_NO_ERROR;
}

NlErrCode NearlinkTwsClientServer::DisableWearDetection(const std::string &address)
{
    HILOGI("address: %{public}s", GetEncryptAddr(address).c_str());
    ProfileTws *twsService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(twsService, NL_ERR_INTERNAL_ERROR, "twsService is nullptr.");
    RawAddress device(address);
    twsService->TwsDisableWearDetection(device);
    return NL_NO_ERROR;
}

NlErrCode NearlinkTwsClientServer::GetWearDetectionState(const std::string &address, int32_t &state)
{
    ProfileTws *twsService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(twsService, NL_ERR_INTERNAL_ERROR, "twsService is nullptr.");
    RawAddress device(address);
    state = twsService->TwsGetWearDetectionState(device);
    HILOGD("address: %{public}s, state: %{public}d", GetEncryptAddr(address).c_str(), state);
    return NL_NO_ERROR;
}

NlErrCode NearlinkTwsClientServer::IsDeviceWearing(const std::string &address, bool &isWearing)
{
    HILOGD("address: %{public}s", GetEncryptAddr(address).c_str());
    ProfileTws *twsService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(twsService, NL_ERR_INTERNAL_ERROR, "twsService is nullptr.");
    RawAddress device(address);
    isWearing = twsService->TwsIsDeviceWearing(device);
    HILOGD("address: %{public}s, isWearing: %{public}d", GetEncryptAddr(address).c_str(), isWearing);
    return NL_NO_ERROR;
}

NlErrCode NearlinkTwsClientServer::IsWearDetectionSupported(const std::string &address, bool &isSupported)
{
    ProfileTws *twsService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(twsService, NL_ERR_INTERNAL_ERROR, "twsService is nullptr.");
    RawAddress device(address);
    isSupported = twsService->TwsIsSupportWearDetect(device);
    HILOGD("address: %{public}s, isSupported: %{public}d", GetEncryptAddr(address).c_str(), isSupported);
    return NL_NO_ERROR;
}

NlErrCode NearlinkTwsClientServer::GetTwsRoleInfo(const std::string &address, int32_t &roleInfo)
{
    HILOGD("address: %{public}s", GetEncryptAddr(address).c_str());
    ProfileTws *twsService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(twsService, NL_ERR_INTERNAL_ERROR, "twsService is nullptr.");
    RawAddress device(address);
    uint8_t roleType;
    twsService->TwsGetDeviceRole(device, roleType);
    roleInfo = static_cast<int32_t>(roleType);
    HILOGD("address: %{public}s, roleInfo: %{public}d", GetEncryptAddr(address).c_str(), roleInfo);
    return NL_NO_ERROR;
}

NlErrCode NearlinkTwsClientServer::GetTwsAudioDelay(const std::string &address, uint32_t &delayValue)
{
    HILOGD("address: %{public}s", GetEncryptAddr(address).c_str());
    ProfileTws *twsService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(twsService, NL_ERR_INTERNAL_ERROR, "twsService is nullptr.");
    RawAddress device(address);
    twsService->GetTwsAudioDelay(device, delayValue);
    return NL_NO_ERROR;
}

NlErrCode NearlinkTwsClientServer::SendUserSelection(const std::string &address,
    const std::vector<struct AudioStreamInfo> &streamInfo)
{
    HILOGI("address: %{public}s", GetEncryptAddr(address).c_str());
    ProfileTws *twsService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(twsService, NL_ERR_INTERNAL_ERROR, "twsService is nullptr.");
    RawAddress device(address);
    twsService->TwsSendUserSelection(device, streamInfo);
    return NL_NO_ERROR;
}

NlErrCode NearlinkTwsClientServer::QueryStreamState(const std::string &address,
    std::vector<struct AudioStreamInfo> &streamData)
{
    HILOGD("address: %{public}s", GetEncryptAddr(address).c_str());
    ProfileTws *twsService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(twsService, NL_ERR_INTERNAL_ERROR, "twsService is nullptr.");
    RawAddress device(address);
    twsService->QueryStreamState(device, streamData);
    if (streamData.empty()) {
        return NL_ERR_INVALID_PARAM;
    }
    return NL_NO_ERROR;
}

NlErrCode NearlinkTwsClientServer::IsSupportVirtualAutoConnect(const std::string &address, bool &isSupported)
{
    ProfileTws *twsService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(twsService, NL_ERR_INTERNAL_ERROR, "twsService is nullptr.");
    RawAddress device(address);
    isSupported = twsService->TwsIsSupportVirtualAutoConnect(device);
    HILOGI("address: %{public}s, isSupported: %{public}d", GetEncryptAddr(address).c_str(), isSupported);
    return NL_NO_ERROR;
}

NlErrCode NearlinkTwsClientServer::SetVirtualAutoConnectType(const std::string &address,
    int32_t connType, int32_t businessType)
{
    ProfileTws *twsService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(twsService, NL_ERR_INTERNAL_ERROR, "twsService is nullptr.");
    RawAddress device(address);
    twsService->SetVirtualAutoConnectType(device, connType, businessType);
    HILOGI("address:%{public}s,connType:%{public}d,BusinessType:%{public}d",
        GetEncryptAddr(address).c_str(), connType, businessType);
    return NL_NO_ERROR;
}

}  // namespace Nearlink
}  // namespace OHOS