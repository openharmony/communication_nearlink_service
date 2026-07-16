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
#include <list>
#include <mutex>
#include "nearlink_errorcode.h"
#include "log.h"
#include "nearlink_utils_server.h"
#include "ssap_data.h"
#include "i_nearlink_ssap_server.h"
#include "SleInterfaceManager.h"
#include "interface_profile_ssap_server.h"
#include "SleInterfaceProfileManager.h"
#include "ipc_skeleton.h"
#include "nearlink_permission_manager.h"
#include "nearlink_ssap_server_server.h"
#include "nearlink_device_manager.h"
#ifdef NEARLINK_KIA_ENABLE
#include "SleKiaManager.h"
#endif
#ifdef NEARLINK_EDM_ENABLE
#include "SleEdmManager.h"
#endif
#include "nearlink_remote_container.h"

namespace OHOS {
namespace Nearlink {
struct NearlinkSsapServerServer::impl {
    class SsapServerCallbackImpl;
    class SystemStateObserver;

    std::unique_ptr<SystemStateObserver> systemStateObserver_;
    std::list<std::shared_ptr<SsapServerCallbackImpl>> callbacks_;

    impl();
    ~impl();

    InterfaceProfileSsapServer *GetServicePtr()
    {
        return static_cast<InterfaceProfileSsapServer *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_SSAP_SERVER));
    }

    struct SsapServerServerRemoteInfo;
    class SsapServerServerRemoteContainer;
    // weak for deathReipient of container
    std::shared_ptr<SsapServerServerRemoteContainer> remoteContainer_ =
        std::make_shared<SsapServerServerRemoteContainer>();
};

struct NearlinkSsapServerServer::impl::SsapServerServerRemoteInfo {
    SsapServerServerRemoteInfo(int32_t pid, int32_t uid, int32_t appId) : pid(pid), uid(uid), appId(appId) {}
    int32_t pid = 0;
    int32_t uid = 0;
    int32_t appId = -1;
};

class NearlinkSsapServerServer::impl::SsapServerServerRemoteContainer final
        : public NearlinkRemoteContainer<NearlinkSsapServerServer::impl::SsapServerServerRemoteInfo> {
public:
    ~SsapServerServerRemoteContainer() override = default;
    void OnRemoteDied(const wptr<IRemoteObject> &remote) override
    {
        HILOGI("ssap server app died");
        int appId = -1;
        {
            std::lock_guard<std::mutex> lk(vecMutex_);
            auto it = std::find_if(vec_.begin(), vec_.end(), [remote](const auto &obj) { return obj.first == remote; });
            NL_CHECK_RETURN(it != vec_.end(), "remote info unexpectedly not found");
            appId = it->second.appId;
        }
        DeleteRemoteInfo(remote);
        if (appId >= 0) {
            InterfaceProfileSsapServer *serverService = static_cast<InterfaceProfileSsapServer *>(
                SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_SSAP_SERVER));
            NL_CHECK_RETURN(serverService, "serverService invalid.");
            int ret = serverService->DeregisterApplication(appId);
            HILOGI("DeregisterApplication result:%{public}d, appId:%{public}d", ret, appId);
        }
    }

    /*
     * 容器里保存appid是否被使用.
     * 由于协议栈在星闪开关关闭时,协议栈会清除所有appid,上层无法感知此行为,
     * 需要增加替换逻辑清除上层残余过期appid.
     */
    wptr<IRemoteObject> FindRemoteSsapServerAppId(int32_t appId)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        auto it = std::find_if(vec_.begin(), vec_.end(), [appId](const auto &obj) {
            return obj.second.appId == appId;
        });
        if (it != vec_.end()) {
            HILOGI("appId: %{public}d, pid: %{public}d, uid: %{public}d", appId, it->second.pid, it->second.uid);
            return it->first;
        }
        return nullptr;
    }

    bool CheckSsapServerApp(int32_t appId)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        int32_t pid = IPCSkeleton::GetCallingPid();
        int32_t uid = IPCSkeleton::GetCallingUid();
        auto it = std::find_if(vec_.begin(), vec_.end(), [appId](const auto &obj) {
            return obj.second.appId == appId;
        });
        if (it == vec_.end()) {
            HILOGE("can't find appId: %{public}d", appId);
            return false;
        }
        bool ret = (it->second.pid == pid && it->second.uid == uid);
        if (ret) {
            HILOGD("pass appId: %{public}d, pid: %{public}d, uid: %{public}d", appId, pid, uid);
        } else {
            HILOGE("check failed appId: %{public}d, pid: %{public}d, uid: %{public}d", appId, pid, uid);
        }
        return ret;
    }
};

class NearlinkSsapServerServer::impl::SystemStateObserver : public ISystemStateObserver {
public:
    SystemStateObserver(NearlinkSsapServerServer::impl *impl) : impl_(impl){};
    ~SystemStateObserver() override = default;

    void OnSystemStateChange(const SleSystemState state) override
    {
        if (impl_) {
            HILOGI("OnSystemStateChange %{public}d.", state);
        }
    }

private:
    NearlinkSsapServerServer::impl *impl_;
};

class NearlinkSsapServerServer::impl::SsapServerCallbackImpl : public InterfaceSsapServerCallback {
public:
    void OnMtuChanged(const RawAddress &addr, uint8_t transport, uint16_t mtu) override
    {
        HILOGI("addr: %{public}s, transport: %{public}d, mtu: %{public}d",
            GetEncryptAddr(addr.GetAddress()).c_str(), transport, mtu);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId_: %{public}lu, sdkVersion: %{public}d", tokenId_, sdkVersion_);
            return;
        }
        if (callback_ == nullptr) {
            HILOGE("callback is nullptr.");
            return;
        }
        SsapDevice device(addr, transport);
        callback_->OnMtuChanged(static_cast<NearlinkSsapDevice>(device), mtu);
    }

    void OnAddService(Service &service, int ret) override
    {
        HILOGI("ret: %{public}d", ret);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        if (callback_ == nullptr) {
            HILOGE("callback is nullptr.");
            return;
        }
        callback_->OnAddService(static_cast<NearlinkSsapServiceParcel>(service), ret);
    }

    void OnSetPropertyValue(Property &property, int ret) override
    {
        HILOGI("ret: %{public}d", ret);
        if ((!NearLinkPermissionManager::CheckSystemPermission(tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK, tokenId_))) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
    }

    void OnSetDescriptorValue(Descriptor &descriptor, int ret) override
    {
        HILOGI("ret: %{public}d", ret);
        if ((!NearLinkPermissionManager::CheckSystemPermission(tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK, tokenId_))) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
    }

    void OnReadPropertyAuthorizeRequest(const RawAddress &addr, uint8_t transport, uint16_t requestId,
        Property &property) override
    {
        HILOGI("addr: %{public}s, transport: %{public}d, requestId: %{public}d",
            GetEncryptAddr(addr.GetAddress()).c_str(), transport, requestId);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId_: %{public}lu, sdkVersion: %{public}d", tokenId_, sdkVersion_);
            return;
        }
    }

    void OnReadDescriptorAuthorizeRequest(const RawAddress &addr, uint8_t transport, uint16_t requestId,
        Descriptor &descriptor) override
    {
        HILOGI("addr: %{public}s, transport: %{public}d, requestId: %{public}d",
            GetEncryptAddr(addr.GetAddress()).c_str(), transport, requestId);
        if ((!NearLinkPermissionManager::CheckSystemPermission(tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK, tokenId_))) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
    }

    void OnWritePropertyAuthorizeRequest(const RawAddress &addr, uint8_t transport, uint16_t requestId,
        Property &property) override
    {
        HILOGI("addr: %{public}s, transport: %{public}d, requestId: %{public}d",
            GetEncryptAddr(addr.GetAddress()).c_str(), transport, requestId);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId_: %{public}lu, sdkVersion: %{public}d", tokenId_, sdkVersion_);
            return;
        }
    }

    void OnWriteDescriptorAuthorizeRequest(const RawAddress &addr, uint8_t transport, uint16_t requestId,
        Descriptor &descriptor) override
    {
        HILOGI("addr: %{public}s, transport: %{public}d, requestId: %{public}d",
            GetEncryptAddr(addr.GetAddress()).c_str(), transport, requestId);
        if ((!NearLinkPermissionManager::CheckSystemPermission(tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK, tokenId_))) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
    }

    void OnReadProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) override
    {
        HILOGI("addr: %{public}s, transport: %{public}d, ret: %{public}d",
            GetEncryptAddr(addr.GetAddress()).c_str(), transport, ret);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId_: %{public}lu, sdkVersion: %{public}d", tokenId_, sdkVersion_);
            return;
        }
        if (callback_ == nullptr) {
            HILOGE("callback is nullptr.");
            return;
        }
        SsapDevice device(addr, transport);
        callback_->OnPropertyReadRequest(
            static_cast<NearlinkSsapDevice>(device), static_cast<NearlinkSsapPropertyParcel>(property), ret);
    }

    void OnReadDescriptor(const RawAddress &addr, uint8_t transport, Descriptor &descriptor, int ret) override
    {
        HILOGI("addr: %{public}s, transport: %{public}d, ret: %{public}d",
            GetEncryptAddr(addr.GetAddress()).c_str(), transport, ret);
        if ((!NearLinkPermissionManager::CheckSystemPermission(tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK, tokenId_))) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        if (callback_ == nullptr) {
            HILOGE("callback is nullptr.");
            return;
        }
        SsapDevice device(addr, transport);
        callback_->OnDescriptorReadRequest(
            static_cast<NearlinkSsapDevice>(device), static_cast<NearlinkSsapDescriptorParcel>(descriptor), ret);
    }

    void OnWriteProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) override
    {
        HILOGI("addr: %{public}s, transport: %{public}d, ret: %{public}d",
            GetEncryptAddr(addr.GetAddress()).c_str(), transport, ret);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId_: %{public}lu, sdkVersion: %{public}d", tokenId_, sdkVersion_);
            return;
        }
        if (callback_ == nullptr) {
            HILOGE("callback is nullptr.");
            return;
        }
        SsapDevice device(addr, transport);
        callback_->OnPropertyWriteRequest(
            static_cast<NearlinkSsapDevice>(device), static_cast<NearlinkSsapPropertyParcel>(property), ret);
    }

    void OnWriteDescriptor(const RawAddress &addr, uint8_t transport, Descriptor &descriptor, int ret) override
    {
        HILOGI("addr: %{public}s, transport: %{public}d, ret: %{public}d",
            GetEncryptAddr(addr.GetAddress()).c_str(), transport, ret);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        if (callback_ == nullptr) {
            HILOGE("callback is nullptr.");
            return;
        }
        SsapDevice device(addr, transport);
        callback_->OnDescriptorWriteRequest(
            static_cast<NearlinkSsapDevice>(device), static_cast<NearlinkSsapDescriptorParcel>(descriptor), ret);
    }

    void OnNotifyProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) override
    {
        HILOGI("addr: %{public}s, transport: %{public}d, ret: %{public}d",
            GetEncryptAddr(addr.GetAddress()).c_str(), transport, ret);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        if (callback_ == nullptr) {
            HILOGE("callback is nullptr.");
            return;
        }
        SsapDevice device(addr, transport);
        callback_->OnNotifyPropertyChanged(
            static_cast<NearlinkSsapDevice>(device), property.uuid_, property.handle_, ret);
    }

    void OnNotifyEvent(const RawAddress &addr, uint8_t transport, Event &event, int ret) override
    {
        HILOGI("addr: %{public}s, transport: %{public}d, ret: %{public}d",
            GetEncryptAddr(addr.GetAddress()).c_str(), transport, ret);
        if ((!NearLinkPermissionManager::CheckSystemPermission(tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK, tokenId_))) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        if (callback_ == nullptr) {
            HILOGE("callback is nullptr.");
            return;
        }
        SsapDevice device(addr, transport);
        callback_->OnNotifyEventChanged(static_cast<NearlinkSsapDevice>(device), event.uuid_, event.handle_, ret);
    }

    void OnConnectionStateChanged(
        const RawAddress &addr, uint8_t transport, uint8_t state, int ret, int reason) override
    {
        HILOGI("addr: %{public}s, transport: %{public}d, state: %{public}d reason: 0x%{public}x",
            GetEncryptAddr(addr.GetAddress()).c_str(), transport, state, reason);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId_: %{public}lu, sdkVersion: %{public}d", tokenId_, sdkVersion_);
            return;
        }
        if (callback_ == nullptr) {
            HILOGE("callback is nullptr.");
            return;
        }
        SsapDevice device(addr, transport);
        int reportReason = (ret != SsapStatus::SSAP_SUCCESS) ? ret : reason;
        callback_->OnConnectionStateChanged(static_cast<NearlinkSsapDevice>(device), state, reportReason);
    }

    sptr<INearlinkSsapServerCallback> GetCallback()
    {
        // called inner, already locked.
        return callback_;
    }

    SsapServerCallbackImpl(const sptr<INearlinkSsapServerCallback> &callback);
    ~SsapServerCallbackImpl()
    {
        callback_ = nullptr;
    };

private:
    class SsapServerCallbackDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        SsapServerCallbackDeathRecipient(
            const sptr<INearlinkSsapServerCallback> &callback);

        sptr<INearlinkSsapServerCallback> GetCallback() const
        {
        // called inner, already locked.
            return callback_;
        };

        void OnRemoteDied(const wptr<IRemoteObject> &remote) override;

    private:
        sptr<INearlinkSsapServerCallback> callback_;
    };

    sptr<INearlinkSsapServerCallback> callback_;
    uint64_t tokenId_;
    int sdkVersion_;  // JS application api sdk version
    bool isUseRealAddrFlag;
};

NearlinkSsapServerServer::impl::SsapServerCallbackImpl::SsapServerCallbackImpl(
    const sptr<INearlinkSsapServerCallback> &callback)
    : callback_(callback)
{
    tokenId_ = IPCSkeleton::GetCallingFullTokenID();
    sdkVersion_ = NearLinkPermissionManager::GetNearlinkApiVersion();
    isUseRealAddrFlag = NearLinkPermissionManager::IsUseRealAddr();
}

using NlSsapServerServer = NearlinkSsapServerServer;

NlSsapServerServer::impl::SsapServerCallbackImpl::SsapServerCallbackDeathRecipient::SsapServerCallbackDeathRecipient(
    const sptr<INearlinkSsapServerCallback> &callback)
    : callback_(callback)
{}

void NearlinkSsapServerServer::impl::SsapServerCallbackImpl::SsapServerCallbackDeathRecipient::OnRemoteDied(
    const wptr<IRemoteObject> &remote)
{}

NearlinkSsapServerServer::impl::impl() : systemStateObserver_(new SystemStateObserver(this))
{
    remoteContainer_->Init();
    SleInterfaceManager::GetInstance()->RegisterSystemStateObserver(*systemStateObserver_);
}

NearlinkSsapServerServer::impl::~impl()
{
    HILOGW("NearlinkSsapServerServer::~impl()");
    SleInterfaceManager::GetInstance()->DeregisterSystemStateObserver(*systemStateObserver_);
}

NlErrCode NearlinkSsapServerServer::AddService(int32_t appId, NearlinkSsapServiceParcel *services)
{
    HILOGI("enter, appId: %{public}d", appId);
#ifdef NEARLINK_KIA_ENABLE
    int32_t pid = IPCSkeleton::GetCallingPid();
    NL_CHECK_RETURN_RET(!SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_SSAP, pid),
        NL_ERR_PROFILE_PROHIBITED_BY_EDM, "kia refuse.");
#endif
#ifdef NEARLINK_EDM_ENABLE
    NL_CHECK_RETURN_RET(SleEdmManager::GetInstance()->IsAllowedConnect(ALLOW_PROTOCOL_TYPE_SSAP),
        NL_ERR_PROFILE_PROHIBITED_BY_EDM, "edm refuse.");
#endif
    InterfaceProfileSsapServer *serverService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(serverService, NL_ERR_INTERNAL_ERROR, "serverService invalid.");
    Service svc = static_cast<Service>(*services);
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->CheckSsapServerApp(appId), NL_ERR_INTERNAL_ERROR,
        "check failed.");
    int ret = serverService->AddService(appId, svc);
    return (ret == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR);
}

NlErrCode NearlinkSsapServerServer::ClearServices(int appId)
{
    HILOGI("enter, appId: %{public}d", appId);
    InterfaceProfileSsapServer *serverService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(serverService, NL_ERR_INTERNAL_ERROR, "serverService invalid.");
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->CheckSsapServerApp(appId), NL_ERR_INTERNAL_ERROR,
        "check failed.");
    int ret = serverService->ClearServices(appId);
    return (ret == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR);
}

NlErrCode NearlinkSsapServerServer::CancelConnection(int appId, const NearlinkSsapDevice &device)
{
    HILOGI("appId: %{public}d, addr: %{public}s", appId, GET_ENCRYPT_SSAP_ADDR(device));
    InterfaceProfileSsapServer *serverService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(serverService, NL_ERR_INTERNAL_ERROR, "serverService invalid.");
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->CheckSsapServerApp(appId), NL_ERR_INTERNAL_ERROR,
        "check failed.");
    int ret = serverService->Disconnect(appId, device.addr_, device.transport_);
    return (ret == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR);
}

NlErrCode NearlinkSsapServerServer::RegisterApplication(const sptr<INearlinkSsapServerCallback> &callback,
    int32_t &appId)
{
    InterfaceProfileSsapServer *serverService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(serverService, NL_ERR_INTERNAL_ERROR, "serverService invalid.");
    auto callbackImpl = std::make_shared<impl::SsapServerCallbackImpl>(callback);
    int ret = serverService->RegisterApplication(callbackImpl);
    if (ret >= 0) {
        appId = ret;
        auto remote = pimpl->remoteContainer_->FindRemoteSsapServerAppId(appId);
        if (remote != nullptr) {
            HILOGW("clear expired appId: %{public}d", appId);
            pimpl->remoteContainer_->DeleteRemoteInfo(remote);
        }
        int32_t pid = IPCSkeleton::GetCallingPid();
        int32_t uid = IPCSkeleton::GetCallingUid();
        impl::SsapServerServerRemoteInfo info(pid, uid, appId);
        pimpl->remoteContainer_->AddRemoteInfo(callback->AsObject(), info);
        HILOGI("appId: %{public}d, pid:%{public}d, uid:%{public}d", appId, pid, uid);
        return NL_NO_ERROR;
    }
    HILOGE("RegisterApplication %{public}d", ret);
    return NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSsapServerServer::DeregisterApplication(int32_t appId)
{
    HILOGI("appId: %{public}d", appId);
    InterfaceProfileSsapServer *serverService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(serverService, NL_ERR_INTERNAL_ERROR, "serverService invalid.");
    int ret = SsapStatus::SSAP_SUCCESS;
    if (pimpl->remoteContainer_->CheckSsapServerApp(appId)) {
        ret = serverService->DeregisterApplication(appId);
        auto remote = pimpl->remoteContainer_->FindRemoteSsapServerAppId(appId);
        if (remote != nullptr) {
            pimpl->remoteContainer_->DeleteRemoteInfo(remote);
        }
    }
    return (ret == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR);
}

NlErrCode NearlinkSsapServerServer::NotifyClient(int appId, NearlinkSsapPropertyParcel *property,
    const NearlinkSsapDevice &device, bool needConfirm)
{
    RawAddress realAddr("");
    NearlinkDeviceManager::GetInstance()->GetDeviceRealAddr(device.addr_, realAddr);
    HILOGI("appId: %{public}d, addr: %{public}s, needConfirm: %{public}d, transport: %{public}d",
        appId, GET_ENCRYPT_ADDR(realAddr), needConfirm, device.transport_);
    InterfaceProfileSsapServer *serverService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(serverService, NL_ERR_INTERNAL_ERROR, "serverService invalid.");
    Property proper(property->handle_);
    proper.value_ = std::move(property->value_);

    int ret = serverService->NotifyProperty(appId, proper, realAddr, device.transport_);
    return (ret == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR);
}

NlErrCode NearlinkSsapServerServer::NotifyEvent(int appId, NearlinkSsapEventParcel *event, std::vector<uint8_t> &value,
    const NearlinkSsapDevice &device, bool needConfirm)
{
    HILOGI("appId: %{public}d, addr: %{public}s, needConfirm: %{public}d, transport: %{public}d",
        appId, GET_ENCRYPT_SSAP_ADDR(device), needConfirm, device.transport_);
    InterfaceProfileSsapServer *serverService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(serverService, NL_ERR_INTERNAL_ERROR, "serverService invalid.");

    Event eve(event->handle_, event->uuid_);

    int ret = serverService->NotifyEvent(appId, eve, value, device.addr_, device.transport_);
    return (ret == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR);
}

NlErrCode NearlinkSsapServerServer::SetPropertyValue(int appId, NearlinkSsapPropertyParcel *property)
{
    HILOGI("appId: %{public}d", appId);
    InterfaceProfileSsapServer *serverService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(serverService, NL_ERR_INTERNAL_ERROR, "serverService invalid.");
    Property proper(property->handle_, property->uuid_);

    int ret = serverService->SetPropertyValue(appId, proper);
    return (ret == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR);
}

NlErrCode NearlinkSsapServerServer::SetDescriptorValue(int appId, NearlinkSsapDescriptorParcel *descriptor)
{
    HILOGI("appId: %{public}d", appId);
    InterfaceProfileSsapServer *serverService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(serverService, NL_ERR_INTERNAL_ERROR, "serverService invalid.");

    int ret = serverService->SetDescriptorValue(appId, *descriptor);
    return (ret == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR);
}

NlErrCode NearlinkSsapServerServer::Connect(int appId, const NearlinkSsapDevice &device,
    uint8_t secureReq, bool autoConnect)
{
    HILOGI("appId: %{public}d, addr: %{public}s, transport: %{public}d, secureReq: %{public}d,"
        "autoConnect: %{public}d", appId, GET_ENCRYPT_SSAP_ADDR(device), device.transport_, secureReq, autoConnect);
    InterfaceProfileSsapServer *serverService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(serverService, NL_ERR_INTERNAL_ERROR, "serverService invalid.");
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->CheckSsapServerApp(appId), NL_ERR_INTERNAL_ERROR,
        "check failed.");
    int ret = serverService->Connect(appId, device.addr_, device.transport_);
    return (ret == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR);
}

NlErrCode NearlinkSsapServerServer::RemoveService(int32_t appId, const NearlinkSsapServiceParcel &services)
{
    HILOGI("appId: %{public}d", appId);
    InterfaceProfileSsapServer *serverService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(serverService, NL_ERR_INTERNAL_ERROR, "serverService invalid.");

    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->CheckSsapServerApp(appId), NL_ERR_INTERNAL_ERROR,
        "check failed.");
    int ret = serverService->RemoveService(appId, static_cast<Service>(services));
    return (ret == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR);
}

NlErrCode NearlinkSsapServerServer::AuthorizeResponse(int appId, uint16_t requestId, bool allow)
{
    HILOGI("appId: %{public}d, requestId: %{public}d, allow: %{public}d", appId, requestId, allow);
    InterfaceProfileSsapServer *serverService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(serverService, NL_ERR_INTERNAL_ERROR, "serverService invalid.");
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->CheckSsapServerApp(appId), NL_ERR_INTERNAL_ERROR,
        "check failed.");
    int ret = serverService->AuthorizeResponse(appId, requestId, allow);
    return (ret == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR);
}

NearlinkSsapServerServer::NearlinkSsapServerServer() : pimpl(new impl())
{
    HILOGI("Nearlink Ssap Server Server Created!");
}

NearlinkSsapServerServer::~NearlinkSsapServerServer()
{}
}  // namespace Nearlink
}  // namespace OHOS