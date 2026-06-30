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
#include <thread>

#include "nearlink_errorcode.h"
#include "log_util.h"
#include "nearlink_safe_list.h"
#include "ssap_data.h"
#include "interface_profile_ssap_client.h"
#include "SleInterfaceProfileManager.h"
#include "nearlink_utils_server.h"
#include "SleInterfaceManager.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "ipc_skeleton.h"
#include "nearlink_permission_manager.h"
#include "nearlink_uuid.h"
#include "nearlink_device_manager.h"
#include "nearlink_ssap_client_server.h"
#ifdef NEARLINK_KIA_ENABLE
#include "SleKiaManager.h"
#endif
#ifdef NEARLINK_EDM_ENABLE
#include "SleEdmManager.h"
#endif

namespace OHOS {
namespace Nearlink {

struct NearlinkSsapClientServer::impl {
    class SsapClientCallbackImpl;
    class SystemStateObserver;

    std::unique_ptr<SystemStateObserver> systemStateObserver_;
    NearlinkSafeList<std::shared_ptr<SsapClientCallbackImpl>> callbacks_;
    std::mutex callbacksMutex_;

    impl();
    ~impl();

    InterfaceProfileSsapClient *GetServicePtr()
    {
        return static_cast<InterfaceProfileSsapClient *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_SSAP_CLIENT));
    }
};

class NearlinkSsapClientServer::impl::SystemStateObserver : public ISystemStateObserver {
public:
    explicit SystemStateObserver(NearlinkSsapClientServer::impl *impl) : impl_(impl){};
    ~SystemStateObserver() override = default;

    void OnSystemStateChange(const SleSystemState state) override
    {
        if (impl_) {
            HILOGI("OnSystemStateChange %{public}d.", state);
        }
    }

private:
    NearlinkSsapClientServer::impl *impl_;
};

class NearlinkSsapClientServer::impl::SsapClientCallbackImpl : public InterfaceSsapClientCallback {
public:
    void OnConnectionStateChanged(uint8_t newState, int state) override
    {
        HILOGD("curState(%{public}d) -> newState(%{public}d)", state, newState);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        callback_->OnConnectionStateChanged(state, static_cast<int32_t>(newState));
    }

    void OnPropertyChanged(const Property &property) override
    {
        HILOGI("enter");
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        callback_->OnPropertyChanged((NearlinkSsapPropertyParcel)property);
    }

    void OnEvent(const Event &event) override
    {
        HILOGI("enter");
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        callback_->OnEventNotified((NearlinkSsapEventParcel)event);
    }

    void OnReadProperty(Property &property, int ret) override
    {
        HILOGI("ret: %{public}d", ret);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        callback_->OnReadProperty(ret, (NearlinkSsapPropertyParcel)property);
    }

    void OnCallMethod(Method &method, int ret) override
    {
        HILOGI("ret: %{public}d", ret);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        callback_->OnCallMethod(ret, (NearlinkSsapMethodParcel)method);
    }

    void OnReadPropertiesByUuid(std::list<Property> &list, int ret) override
    {
        HILOGI("Enter");
        if ((!NearLinkPermissionManager::CheckSystemPermission(tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK, tokenId_))) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        std::vector<NearlinkSsapPropertyParcel> properties;
        for (auto &item : list) {
            properties.push_back(NearlinkSsapPropertyParcel(item));
        }
        callback_->OnReadPropertiesByUuid(ret, properties);
    }

    void OnWriteProperty(Property &property, int ret) override
    {
        HILOGI("ret: %{public}d", ret);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        callback_->OnWriteProperty(ret, (NearlinkSsapPropertyParcel)property);
    }

    void OnSetPropertyNotification(const Property &property, bool enable, int ret) override
    {
        HILOGI("ret: %{public}d", ret);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        callback_->OnSetPropertyNotification(ret, enable, (NearlinkSsapPropertyParcel)property);
    }

    void OnSetPropertyIndication(const Property &property, bool enable, int ret) override
    {
        HILOGI("ret: %{public}d", ret);
        if ((!NearLinkPermissionManager::CheckSystemPermission(tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK, tokenId_))) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        callback_->OnSetPropertyIndication(ret, enable, (NearlinkSsapPropertyParcel)property);
    }

    void OnReadDescriptor(Descriptor &descriptor, int ret) override
    {
        HILOGI("ret: %{public}d", ret);
        if ((!NearLinkPermissionManager::CheckSystemPermission(tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_))) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        callback_->OnReadDescriptor(ret, (NearlinkSsapDescriptorParcel)descriptor);
    }

    void OnReadDescriptorsByUuid(std::list<Descriptor> &list, int ret) override
    {
        HILOGI("Enter");
        if ((!NearLinkPermissionManager::CheckSystemPermission(tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK, tokenId_))) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
    }

    void OnWriteDescriptor(Descriptor &descriptor, int ret) override
    {
        HILOGI("ret: %{public}d", ret);
        if ((!NearLinkPermissionManager::CheckSystemPermission(tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_))) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        callback_->OnWriteDescriptor(ret, (NearlinkSsapDescriptorParcel)descriptor);
    }

    void OnMtuChanged(uint16_t mtu, int state) override
    {
        HILOGI("state: %{public}d, mtu: %{public}d", state, mtu);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        callback_->OnMtuChanged(state, mtu);
    }

    void OnDiscoverComplete(int status) override
    {
        HILOGI("status: %{public}d", status);
        if (!NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId_)) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        callback_->OnServicesDiscovered(status);
    }

    void OnDiscoverByUuidComplete(const Uuid &uuid, int status) override
    {
        HILOGI("status: %{public}d", status);
        if ((!NearLinkPermissionManager::CheckSystemPermission(tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK, tokenId_))) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        callback_->OnServicesDiscoveredByUuid(status, uuid);
    }

    void OnServicesRediscovered(std::vector<Service> &services) override
    {
        HILOGI("serviceNum: %{public}lu", services.size());
        if ((!NearLinkPermissionManager::CheckSystemPermission(tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK, tokenId_))) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        std::vector<NearlinkSsapServiceParcel> serviceParcels;
        for (auto &service : services) {
            serviceParcels.push_back(NearlinkSsapServiceParcel(service));
        }
        callback_->OnServicesRediscovered(serviceParcels);
    }

    void OnServiceChanged(uint16_t handle, const Uuid &uuid) override
    {
        HILOGI("handle: %{public}u", handle);
        if ((!NearLinkPermissionManager::CheckSystemPermission(tokenId_)) ||
            (!NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK, tokenId_))) {
            HILOGE("check permission failed, tokenId: %{public}lu", tokenId_);
            return;
        }
        NearlinkUuidParcel uuidParcel(uuid);
        callback_->OnServiceChanged(handle, uuidParcel);
    }

    sptr<INearlinkSsapClientCallback> GetCallback()
    {
        return callback_;
    }

    void SetAppId(int appId)
    {
        applicationId_ = appId;
    }

    int GetAppId()
    {
        return applicationId_;
    }

    SsapClientCallbackImpl(const sptr<INearlinkSsapClientCallback> &callback, NearlinkSsapClientServer &owner);
    ~SsapClientCallbackImpl() override
    {
        HILOGW("NearlinkSsapClientServer: ~SsapClientCallbackImpl()");
        // already locked to remove the instance inner.
        if (!callback_->AsObject()->RemoveDeathRecipient(deathRecipient_)) {
            HILOGE("Failed to unlink death recipient to callback");
        }
        callback_ = nullptr;
        deathRecipient_ = nullptr;
    };

private:
    class CallbackDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        CallbackDeathRecipient(const sptr<INearlinkSsapClientCallback> &callback, NearlinkSsapClientServer &owner);

        sptr<INearlinkSsapClientCallback> GetCallback() const
        {
            return callback_;
        };

        void OnRemoteDied(const wptr<IRemoteObject> &remote) override;

    private:
        sptr<INearlinkSsapClientCallback> callback_;
        NearlinkSsapClientServer &owner_;
    };

    sptr<INearlinkSsapClientCallback> callback_;
    sptr<CallbackDeathRecipient> deathRecipient_;
    int applicationId_;
    uint64_t tokenId_;
    int sdkVersion_;  // JS application api sdk version
    bool isUseRealAddrFlag;
};

NearlinkSsapClientServer::impl::SsapClientCallbackImpl::SsapClientCallbackImpl(
    const sptr<INearlinkSsapClientCallback> &callback, NearlinkSsapClientServer &owner)
    : callback_(callback), deathRecipient_(new CallbackDeathRecipient(callback, owner))
{
    // called inner, don't lock here.
    if (!callback_->AsObject()->AddDeathRecipient(deathRecipient_)) {
        HILOGE("Failed to link death recipient to callback");
    }
    tokenId_ = IPCSkeleton::GetCallingFullTokenID();
    sdkVersion_ = NearLinkPermissionManager::GetNearlinkApiVersion();
    isUseRealAddrFlag = NearLinkPermissionManager::IsUseRealAddr();
    applicationId_ = -1;
}

NearlinkSsapClientServer::impl::SsapClientCallbackImpl::CallbackDeathRecipient::CallbackDeathRecipient(
    const sptr<INearlinkSsapClientCallback> &callback, NearlinkSsapClientServer &owner)
    : callback_(callback), owner_(owner)
{}

void NearlinkSsapClientServer::impl::SsapClientCallbackImpl::CallbackDeathRecipient::OnRemoteDied(
    const wptr<IRemoteObject> &remote)
{
    HILOGW("enter OnRemoteDied");
    if (owner_.pimpl == nullptr) {
        HILOGE("ssapClientServerImpl pimpl is not support.");
        return;
    }
    InterfaceProfileSsapClient *clientService = owner_.pimpl->GetServicePtr();
    if (clientService == nullptr) {
        HILOGE("ssapClientServerImpl clientService_ is not support.");
        return;
    }
    int appId = -1;
    auto func = [&remote, &appId](std::shared_ptr<SsapClientCallbackImpl> callback) -> bool {
        if (callback->GetCallback()->AsObject() == remote) {
            HILOGI("callback is found from callbacks");
            sptr<CallbackDeathRecipient> dr = callback->deathRecipient_;
            if (!dr->GetCallback()->AsObject()->RemoveDeathRecipient(dr)) {
                HILOGE("Failed to unlink death recipient from callback");
            }
            HILOGI("App id is %{public}d", callback->GetAppId());
            appId = callback->GetAppId();
            return true;
        }
        return false;
    };
    owner_.pimpl->callbacks_.FindAndRmv(func);
    if (appId >= 0) {
        clientService->Disconnect(appId);
        clientService->DeregisterApplication(appId);
    }
}

NearlinkSsapClientServer::impl::impl() : systemStateObserver_(new SystemStateObserver(this))
{
    SleInterfaceManager::GetInstance()->RegisterSystemStateObserver(*systemStateObserver_);
}

NearlinkSsapClientServer::impl::~impl()
{
    SleInterfaceManager::GetInstance()->DeregisterSystemStateObserver(*systemStateObserver_);
}

NearlinkSsapClientServer::NearlinkSsapClientServer() : pimpl(new impl())
{
    HILOGI("enter");
}

NearlinkSsapClientServer::~NearlinkSsapClientServer()
{}

NlErrCode NearlinkSsapClientServer::RegisterApplication(const sptr<INearlinkSsapClientCallback> &callback,
    const NearlinkRawAddress &addr, int32_t transport, int &appId)
{
    uint8_t secureReq = 0;
    return RegisterApplication(callback, secureReq, addr, transport, appId);
}

NlErrCode NearlinkSsapClientServer::RegisterApplication(const sptr<INearlinkSsapClientCallback> &callback,
    uint8_t secureReq, const NearlinkRawAddress &addr, int32_t transport, int &appId)
{
    HILOGI("address: %{public}s, transport: %{public}d", GetEncryptAddr(addr.GetAddress()).c_str(), transport);
    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");
    RawAddress realAddr = (RawAddress)addr;
    NearlinkDeviceManager::GetInstance()->GetDeviceRealAddr(addr, realAddr);
    auto serverCallback = std::make_shared<impl::SsapClientCallbackImpl>(callback, *this);
    appId = clientService->RegisterApplication(serverCallback, realAddr, transport, secureReq);
    if (appId >= 0) {
        HILOGI("appId: %{public}d", appId);
        serverCallback->SetAppId(appId);
        pimpl->callbacks_.EnsureInsert(serverCallback);
        return NL_NO_ERROR;
    }
    HILOGE("RegisterSharedApplication failed, appId: %{public}d", appId);
    return NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSsapClientServer::DeregisterApplication(int32_t appId)
{
    HILOGI("appId: %{public}d", appId);
    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");
    int ret = SsapStatus::SSAP_SUCCESS;
    auto func = [&appId](std::shared_ptr<impl::SsapClientCallbackImpl> callback) -> bool {
        return callback->GetAppId() == appId;
    };
    if(!pimpl->callbacks_.Find(func)){
        HILOGE("Unknown appId: %{public}d", appId);
        return NL_ERR_INTERNAL_ERROR;
    }
    pimpl->callbacks_.FindAndRmv(func);
    ret = clientService->DeregisterApplication(appId);
    HILOGI("DeregisterApplication ret: %{public}d", ret);
    return ret == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSsapClientServer::Connect(int32_t appId, bool isAutoConnect)
{
    HILOGI("appId: %{public}d, isautoConnect: %{public}d", appId, isAutoConnect);
#ifdef NEARLINK_KIA_ENABLE
    int32_t pid = IPCSkeleton::GetCallingPid();
    NL_CHECK_RETURN_RET(!SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_SSAP, pid),
        NL_ERR_PROFILE_PROHIBITED_BY_EDM, "kia refuse.");
#endif
#ifdef NEARLINK_EDM_ENABLE
    NL_CHECK_RETURN_RET(SleEdmManager::GetInstance()->IsAllowedConnect(ALLOW_PROTOCOL_TYPE_SSAP),
        NL_ERR_PROFILE_PROHIBITED_BY_EDM, "edm refuse.");
#endif
    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");
    int result = clientService->Connect(appId, isAutoConnect);
    HILOGI("Connect result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSsapClientServer::Disconnect(int32_t appId)
{
    HILOGI("appId: %{public}d", appId);
    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");
    int result = clientService->Disconnect(appId);
    HILOGI("Disconnect result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSsapClientServer::DiscoveryServices(int32_t appId)
{
    HILOGI("appId: %{public}d", appId);
    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");
    int result = clientService->DiscoverServices(appId);
    HILOGI("DiscoverServices result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSsapClientServer::DiscoverServiceByUuid(int32_t appId, const Uuid &uuid)
{
    HILOGI("appId: %{public}d", appId);
    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");
    int result = clientService->DiscoverServicesByUuid(appId, Uuid::ConvertFrom128Bits(uuid.ConvertTo128Bits()),
        0x0001, 0xFFFF);
    HILOGI("DiscoverServicesByUuid result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSsapClientServer::ReadProperty(int32_t appId, const NearlinkSsapPropertyParcel &property)
{
    HILOGI("appId: %{public}d", appId);
    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");
    int result = 0;
    if (property.handle_ != static_cast<uint16_t>(INVALID_ENTRY_HANDLE)) {
        result = clientService->ReadProperty(appId, (Property)property);
    } else {
        result = clientService->ReadPropertiesByUuid(appId, property.uuid_, 0x0001, 0xFFFF);
    }
    HILOGI("ReadProperty result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSsapClientServer::CallMethod(int32_t appId, NearlinkSsapMethodParcel *method, bool withoutRespond)
{
    HILOGI("appId: %{public}d, withoutRespond: %{public}d", appId, withoutRespond);
    Method methd(method->handle_);
    methd.parameter_.swap(method->parameter_);
    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");
    int result = clientService->CallMethod(appId, methd, withoutRespond);
    HILOGI("CallMethod result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSsapClientServer::WriteProperty(
    int32_t appId, NearlinkSsapPropertyParcel *property, bool withoutRespond)
{
    HILOGI("appId: %{public}d, withoutRespond: %{public}d", appId, withoutRespond);
    Property proper(property->handle_);
    proper.value_.swap(property->value_);
    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");
    int result = clientService->WriteProperty(appId, proper, withoutRespond);
    HILOGI("WriteProperty result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSsapClientServer::ReadDescriptor(int32_t appId, const NearlinkSsapDescriptorParcel &descriptor)
{
    HILOGI("appId: %{public}d", appId);
    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");
    int result = clientService->ReadDescriptor(appId, (Descriptor)descriptor);
    HILOGI("ReadDescriptor result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSsapClientServer::WriteDescriptor(int32_t appId, NearlinkSsapDescriptorParcel *descriptor,
    bool withoutRespond)
{
    HILOGI("appId: %{public}d", appId);
    Descriptor desc(descriptor->handle_, descriptor->type_);
    desc.value_.swap(descriptor->value_);

    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");
    int result = clientService->WriteDescriptor(appId, desc, withoutRespond);
    HILOGI("WriteDescriptor result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSsapClientServer::RequestExchangeMtu(int32_t appId, int32_t mtu)
{
    HILOGI("appId: %{public}d, mtu: %{public}d", appId, mtu);
    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");
    int result = clientService->ExchangeMtu(appId, static_cast<uint16_t>(mtu));
    HILOGI("ExchangeMtu result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSsapClientServer::RequestConnectionPriority(int32_t appId, int32_t connPriority)
{
    HILOGI("appId: %{public}d, connPriority: %{public}d", appId, connPriority);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSsapClientServer::GetServices(int32_t appId, ::std::vector<NearlinkSsapServiceParcel> &service)
{
    HILOGI("appId: %{public}d", appId);
    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");
    for (auto &svc : clientService->GetServices(appId)) {
        service.push_back(NearlinkSsapServiceParcel(svc));
    }
    return NL_NO_ERROR;
}

NlErrCode NearlinkSsapClientServer::GetServicesByUuid(int32_t appId, const Uuid &uuid,
    std::vector<NearlinkSsapServiceParcel> &service)
{
    HILOGI("appId: %{public}d uuid:%{public}s", appId, GET_ENCRYPT_UUID(uuid));
    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");
    for (auto &svc : clientService->GetServicesByUuid(appId,
        Uuid::ConvertFrom128Bits(uuid.ConvertTo128Bits()))) {
        service.push_back(NearlinkSsapServiceParcel(svc));
    }
    return NL_NO_ERROR;
}

NlErrCode NearlinkSsapClientServer::RequestPropertyNotification(int32_t appId, uint16_t propertyhandle, bool enable,
    uint8_t notifyOption)
{
    HILOGI("appId: %{public}d, property: %{public}u, enable: %{public}d", appId, propertyhandle, enable);
    InterfaceProfileSsapClient *clientService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(clientService, NL_ERR_INTERNAL_ERROR, "clientService invalid.");

    int result = 0;
    Property property(propertyhandle);
    if (notifyOption == static_cast<uint8_t>(NotifyOption::SSAP_SET_NOTIFY)) {
        result = clientService->SetPropertyNotification(appId, property, enable);
    } else {
        result = clientService->SetPropertyIndication(appId, property, enable);
    }
    if (result != SsapStatus::SSAP_SUCCESS) {
         return NL_ERR_INTERNAL_ERROR;
    }
    return NL_NO_ERROR;
}

}  // namespace Nearlink
}  // namespace OHOS
