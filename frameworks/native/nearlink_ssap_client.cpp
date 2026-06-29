/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <condition_variable>
#include <memory>
#include <set>

#include "nearlink_sa_manager.h"
#include "nearlink_errorcode.h"
#include "nearlink_def.h"
#include "nearlink_ssap_client_callback_stub.h"
#include "nearlink_host.h"
#include "nearlink_host_proxy.h"
#include "nearlink_safe_map.h"
#include "nearlink_ssap_service_parcel.h"
#include "log_util.h"
#include "i_nearlink_ssap_client.h"
#include "nearlink_ssap_client.h"

namespace OHOS {
namespace Nearlink {
#define WPTR_SSAP_CBACK(cbWptr, func, ...)          \
do {                                                \
    auto cbSptr = (cbWptr).lock();                  \
    if (cbSptr) {                                   \
        cbSptr->func(__VA_ARGS__);                  \
    } else {                                        \
        HILOGE(#cbWptr ": callback is nullptr");    \
    }                                               \
} while (0)

const int WAIT_TIMEOUT = 10; // 10s
enum class SsapConnectionPriority : int {
    BALANCED,
    HIGH,
    LOW_POWER
};
struct DiscoverInfomation {
    bool isFinding;
    bool needNotify;
    std::mutex clientMutex;
    std::condition_variable condition;
    DiscoverInfomation() : isFinding(false), needNotify(false)
    {}
};

struct SsapClient::impl {
    class NearlinkSsapClientCallbackStubImpl;

    bool isGetServiceYet_;
    std::atomic_bool isRegisterSucceeded_ = false;
    std::weak_ptr<SsapClientCallback> callback_;
    std::atomic<int> applicationId_ = 0;
    std::atomic<int> connectionState_ = 0;
    std::shared_ptr<NearlinkRemoteDevice> device_;
    sptr<NearlinkSsapClientCallbackStubImpl> clientCallback_;
    NearlinkSafeMap<uint16_t, std::shared_ptr<SsapService>> ssapServices_;
    DiscoverInfomation findInformation_;

    explicit impl(std::shared_ptr<NearlinkRemoteDevice> device);
    ~impl();

    void Init(std::weak_ptr<SsapClient> client);

    NlErrCode FindStructureStart();
    NlErrCode FindStructureByUuidStart(const UUID &uuid);
    void FindStructureComplete(int state);
    void FindStructureByUuidComplete(int state, const UUID &uuid);
    std::shared_ptr<SsapService> BuildService(const NearlinkSsapServiceParcel &src);
    void BuildServiceList(const std::vector<NearlinkSsapServiceParcel> &src);
    std::shared_ptr<SsapService> FindService(uint16_t handle);
    NlErrCode GetServices();
    void CleanConnectionInfo();
    int32_t profileRegisterId_{0};
};

class SsapClient::impl::NearlinkSsapClientCallbackStubImpl : public NearlinkSsapClientCallbackStub {
public:
    explicit NearlinkSsapClientCallbackStubImpl(std::weak_ptr<SsapClient> client) : client_(client)
    {}

    ~NearlinkSsapClientCallbackStubImpl() override
    {}

    void OnConnectionStateChanged(int32_t state, int32_t newState) override
    {
        HILOGD("ssapClient conn state, status: %{public}d, newState: %{public}s",
            state, GetConnStateString(newState).c_str());
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }
        if (newState == static_cast<int>(SleConnectState::DISCONNECTED)) {
            clientSptr->pimpl->CleanConnectionInfo();
        }

        clientSptr->pimpl->connectionState_.store(newState);

        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnConnectionStateChanged, newState, state);
    }

    void OnPropertyChanged(const NearlinkSsapPropertyParcel &property) override
    {
        HILOGD("recv notification, length:%{public}zu", property.value_.size());
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }
        UUID uuid = UUID::ConvertFrom128Bits(property.uuid_.ConvertTo128Bits());
        SsapProperty propertyCallback(property.handle_, uuid,
            property.opInd_, property.permission_);
        propertyCallback.SetValue(property.value_.data(), property.value_.size());

        auto func = [&property, &propertyCallback](const uint16_t handle,
            std::shared_ptr<SsapService> service) -> bool {
                for (auto &prop : service->GetProperty()) {
                    if (prop.GetHandle() == property.handle_) {
                        prop.SetValue(property.value_.data(), property.value_.size());
                        propertyCallback.SetServiceUuid(service->GetUuid());
                        return true;
                    }
                }
                return false;
        };
        if (!clientSptr->pimpl->ssapServices_.Find(func)) {
            HILOGE("recv notification failed, property is not exist.");
        }
        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnPropertyChanged, propertyCallback);
    }

    void OnEventNotified(const NearlinkSsapEventParcel &event) override
    {
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }
        UUID uuid = UUID::ConvertFrom128Bits(event.uuid_.ConvertTo128Bits());
        SsapEvent eventCallback(event.handle_, uuid);
        eventCallback.SetParameter(event.parameter_.data(), event.parameter_.size());

        auto func = [&event, &eventCallback](const uint16_t handle,
            std::shared_ptr<SsapService> service) -> bool {
                for (auto &evt : service->GetEvent()) {
                    if (evt.GetHandle() == event.handle_) {
                        evt.SetParameter(event.parameter_.data(), event.parameter_.size());
                        eventCallback.SetUuid(evt.GetUuid());
                        eventCallback.SetServiceUuid(service->GetUuid());
                        return true;
                    }
                }
                return false;
        };
        if (!clientSptr->pimpl->ssapServices_.Find(func)) {
            HILOGE("recv notification failed, event is not exist.");
        }
        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnEventNotified, eventCallback);
    }

    void OnReadProperty(int32_t ret, const NearlinkSsapPropertyParcel &property) override
    {
        HILOGI("ret:%{public}d, length:%{public}zu", ret, property.value_.size());
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }
        SsapProperty onReadProp(0, 0, UUID::FromString(property.uuid_.ToString()), property.opInd_, 0);
        if (ret == static_cast<int32_t>(SsapStatus::SSAP_SUCCESS)) {
            onReadProp.SetValue(property.value_.data(), property.value_.size());
        }
        bool isFindService = clientSptr->pimpl->ssapServices_.Find([&onReadProp](const uint16_t handle,
            std::shared_ptr<SsapService> &service) -> bool {
                for (auto &sp : service->GetProperty()) {
                    if (sp.GetUuid().Equals(onReadProp.GetUuid())) {
                        onReadProp.SetServiceUuid(service->GetUuid());
                        onReadProp.SetOperationIndication(sp.GetOperationIndication());
                        return true;
                    }
                }
                return false;
        });
        if (!isFindService) {
            HILOGE("not find service!");
            return;
        }
        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnPropertyReadResult, onReadProp, ret);
    }

    void OnCallMethod(int32_t ret, const NearlinkSsapMethodParcel &method) override
    {
        HILOGI("ret:%{public}d, result length:%{public}zu", ret, method.result_.size());
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }
        SsapMethod onCallMethd(method.handle_, 0, UUID::FromString(method.uuid_.ToString()), 0);
        if (ret == static_cast<int32_t>(SsapStatus::SSAP_SUCCESS)) {
            onCallMethd.SetResult(method.result_.data(), method.result_.size());
        }
        bool isFindService = clientSptr->pimpl->ssapServices_.Find([&onCallMethd](const uint16_t handle,
            std::shared_ptr<SsapService> &service) -> bool {
                for (auto &sp : service->GetMethod()) {
                    if (sp.GetHandle() == onCallMethd.GetHandle()) {
                        onCallMethd.SetServiceUuid(service->GetUuid());
                        return true;
                    }
                }
                return false;
        });
        if (!isFindService) {
            HILOGE("not find service!");
            return;
        }
        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnMethodCallResult, onCallMethd, ret);
    }

    void OnReadPropertiesByUuid(int32_t ret, const std::vector<NearlinkSsapPropertyParcel> &properties) override
    {
        HILOGI("ret:%{public}d, size:%{public}zu", ret, properties.size());
        NL_CHECK_RETURN(ret == static_cast<int32_t>(SsapStatus::SSAP_SUCCESS), "OnReadPropertiesByUuid error");
        NL_CHECK_RETURN(!properties.empty(), "OnReadPropertiesByUuid properties empty");
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        NL_CHECK_RETURN(clientSptr, "callback client is nullptr");
        std::shared_ptr<SsapService> setService;
        bool isFindService = clientSptr->pimpl->ssapServices_.Find([&properties, &setService](const uint16_t handle,
            std::shared_ptr<SsapService> &service) -> bool {
                for (auto &sp : service->GetProperty()) {
                    if (sp.GetUuid().Equals(UUID::FromString(properties[0].uuid_.ToString()))) {
                        setService = service;
                        return true;
                    }
                }
                return false;
        });
        NL_CHECK_RETURN(isFindService, "not find service!");
        std::vector<SsapProperty> onReadProperties;
        for (uint32_t i = 0; i < properties.size(); i++) {
            SsapProperty onReadProp(0, 0, UUID::FromString(properties[i].uuid_.ToString()), properties[i].opInd_, 0);
            onReadProp.SetValue(properties[i].value_.data(), properties[i].value_.size());
            onReadProp.SetServiceUuid(setService->GetUuid());
            onReadProperties.push_back(onReadProp);
        }
        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnPropertiesReadResult, onReadProperties, ret);
    }

    void OnWriteProperty(int32_t ret, const NearlinkSsapPropertyParcel &property) override
    {
        HILOGI("ret:%{public}d, length:%{public}zu", ret, property.value_.size());
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }
        SsapProperty onWriteProp(0, 0, UUID::FromString(property.uuid_.ToString()), property.opInd_, 0);
        onWriteProp.SetWriteType(static_cast<int>(SsapProperty::PropertyWriteType::PROPERTY_WRITE_REQ));
        if (ret == static_cast<int32_t>(SsapStatus::SSAP_SUCCESS)) {
            onWriteProp.SetValue(property.value_.data(), property.value_.size());
        }
        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnPropertyWriteResult, onWriteProp, ret);
    }

    void OnSetPropertyNotification(int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property) override
    {
        HILOGI("ret:%{public}d, length:%{public}zu", ret, property.value_.size());
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }

        SsapProperty onWriteProp(0, 0, UUID::FromString(property.uuid_.ToString()), property.opInd_, 0);
        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnSetPropertyNotifyResult, onWriteProp, enable, ret);
    }

    void OnSetPropertyIndication(int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property) override
    {
        HILOGI("ret:%{public}d, length:%{public}zu", ret, property.value_.size());
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }

        SsapProperty onWriteProp(0, 0, UUID::FromString(property.uuid_.ToString()), property.opInd_, 0);
        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnSetPropertyIndicateResult, onWriteProp, enable, ret);
    }

    void OnReadDescriptor(int32_t ret, const NearlinkSsapDescriptorParcel &descriptor) override
    {
        HILOGI("ret:%{public}d, length:%{public}zu", ret, descriptor.value_.size());
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }
        SsapDescriptor onReadDesc(descriptor.handle_, descriptor.type_, 0);
        if (ret == static_cast<int32_t>(SsapStatus::SSAP_SUCCESS)) {
            onReadDesc.SetValue(descriptor.value_.data(), descriptor.value_.size());
        }
        bool isFindService = clientSptr->pimpl->ssapServices_.Find([&onReadDesc](const uint16_t handle,
            std::shared_ptr <SsapService> &service) -> bool {
            for (auto &property: service->GetProperty()) {
                if (property.GetHandle() == onReadDesc.GetHandle()) {
                    onReadDesc.SetServiceUuid(service->GetUuid());
                    onReadDesc.SetPropertyUuid(property.GetUuid());
                    return true;
                }
            }
            return false;
        });
        if (!isFindService) {
            HILOGW("not find service!");
        }
        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnDescriptorReadResult, onReadDesc, ret);
    }

    void OnWriteDescriptor(int32_t ret, const NearlinkSsapDescriptorParcel &descriptor) override
    {
        HILOGI("ret:%{public}d, length:%{public}zu", ret, descriptor.value_.size());
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }
        SsapDescriptor onWriteDesc(descriptor.handle_, descriptor.type_, 0);
        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnDescriptorWriteResult, onWriteDesc, ret);
    }

    void OnMtuChanged(int state, uint16_t mtu) override
    {
        HILOGI("state: %{public}d, mtu: %{public}d", state, mtu);
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }
        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnMtuUpdate, mtu, state);
    }

    void OnServicesDiscovered(int32_t status) override
    {
        HILOGI("status: %{public}d", status);
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }
        clientSptr->pimpl->FindStructureComplete(status);
    }

    void OnServicesDiscoveredByUuid(int32_t status, const Uuid &uuid) override
    {
        HILOGI("status: %{public}d, uuid: %{public}s", status, uuid.GetEncryptUuid().c_str());
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }

        clientSptr->pimpl->FindStructureByUuidComplete(status, UUID::ConvertFrom128Bits(uuid.ConvertTo128Bits()));
    }

    void OnConnectionParameterChanged(int32_t interval, int32_t latency, int32_t timeout, int32_t status) override
    {
        HILOGI("interval: %{public}d, latency: %{public}d, timeout: %{public}d, status: %{public}d",
            interval, latency, timeout, status);
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }

        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnConnectionParameterChanged, interval, latency, timeout, status);
    }

    void OnServicesRediscovered(const std::vector<NearlinkSsapServiceParcel> &services) override
    {
        HILOGI("serviceNum: %{public}zu", services.size());
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }

        std::vector<SsapService> ssapServices;
        ssapServices.reserve(services.size());
        for (const auto &service : services) {
            auto servicePtr = clientSptr->pimpl->BuildService(service);
            if (servicePtr) {
                ssapServices.push_back(*servicePtr);
            }
        }
        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnServicesRediscovered, ssapServices);
    }

    void OnServiceChanged(uint16_t handle, const NearlinkUuidParcel &uuidParcel) override
    {
        HILOGI("handle: %{public}u", handle);
        std::shared_ptr<SsapClient> clientSptr = (client_).lock();
        if (!clientSptr) {
            HILOGE("callback client is nullptr");
            return;
        }
        UUID uuid = UUID::ConvertFrom128Bits(uuidParcel.ConvertTo128Bits());
        WPTR_SSAP_CBACK(clientSptr->pimpl->callback_, OnServiceChanged, handle, uuid);
    }
private:
    std::weak_ptr<SsapClient> client_;
};

SsapClient::SsapClient(std::shared_ptr<NearlinkRemoteDevice> device)
{
    HILOGI("create SsapClient start.");
    pimpl = std::make_unique<SsapClient::impl>(device);
    if (!pimpl) {
        HILOGE("create SsapClient failed.");
    }
}

SsapClient::~SsapClient()
{}

std::shared_ptr<SsapClient> SsapClient::CreateSsapClient(std::shared_ptr<NearlinkRemoteDevice> device)
{
    NL_CHECK_RETURN_RET(device, nullptr, "device is nullptr.");
    auto instance = std::make_shared<SsapClient>(Pattern(), device);
    NL_CHECK_RETURN_RET(instance->pimpl, nullptr, "pimpl is nullptr.");

    instance->pimpl->Init(instance);
    return instance;
}

void SsapClient::impl::Init(std::weak_ptr<SsapClient> client)
{
    clientCallback_ = new (std::nothrow) NearlinkSsapClientCallbackStubImpl(client);
    if (clientCallback_ == nullptr) {
        HILOGE("Failed to create NearlinkSsapClientCallbackStubImpl");
        return;
    }
    std::shared_ptr<NearlinkRegisterInfo> info = std::make_shared<NearlinkRegisterInfo>(PROFILE_SSAP_CLIENT);
    if (info == nullptr) {
        HILOGE("Failed to create NearlinkRegisterInfo");
        return;
    }
    info->stateOffFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        applicationId_.store(0);
        isRegisterSucceeded_.store(false);
        connectionState_.store(static_cast<int>(SleConnectState::DISCONNECTED));
    };
    info->serviceStoppedFunc_ = [this]() -> void {
        applicationId_.store(0);
        isRegisterSucceeded_.store(false);
        connectionState_.store(static_cast<int>(SleConnectState::DISCONNECTED));
    };
    profileRegisterId_ = NearlinkSaManager::GetInstance().RegisterFunc(info);
    if (profileRegisterId_ == INVALID_PROFILE_ID) {
        HILOGE("profileRegisterId_ is invalid");
    }
}

SsapClient::impl::impl(std::shared_ptr<NearlinkRemoteDevice> device)
    : isGetServiceYet_(false),
      isRegisterSucceeded_(false),
      applicationId_(0),
      connectionState_(static_cast<int>(SleConnectState::DISCONNECTED)),
      device_(device)
{}

SsapClient::impl::~impl()
{
    HILOGI("SsapClient ~impl");
    NearlinkSaManager::GetInstance().DeregisterFunc(profileRegisterId_);
    if (!isRegisterSucceeded_) {
        return;
    }
    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN(proxy, "proxy is nullptr.");

    proxy->DeregisterApplication(applicationId_);
}

NlErrCode SsapClient::impl::FindStructureStart()
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    {
        std::unique_lock<std::mutex> lock(findInformation_.clientMutex);
        while (findInformation_.isFinding) {
            auto ret = findInformation_.condition.wait_for(lock, std::chrono::seconds(WAIT_TIMEOUT));
            if (ret == std::cv_status::timeout) {
                HILOGE("timeout");
                return NL_ERR_INTERNAL_ERROR;
            }
        }
        findInformation_.isFinding = true;
    }

    NL_CHECK_RETURN_RET(isRegisterSucceeded_, NL_ERR_INTERNAL_ERROR, "isRegisterSucceeded_ is false.");

    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    NlErrCode result = proxy->DiscoveryServices(applicationId_);
    if (result != NL_NO_ERROR) {
        FindStructureComplete(NL_ERR_INTERNAL_ERROR);
    }
    return result;
}

NlErrCode SsapClient::impl::FindStructureByUuidStart(const UUID &uuid)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    {
        std::unique_lock<std::mutex> lock(findInformation_.clientMutex);
        while (findInformation_.isFinding) {
            auto ret = findInformation_.condition.wait_for(lock, std::chrono::seconds(WAIT_TIMEOUT));
            if (ret == std::cv_status::timeout) {
                HILOGE("timeout");
                return NL_ERR_INTERNAL_ERROR;
            }
        }
        findInformation_.isFinding = true;
    }

    NL_CHECK_RETURN_RET(isRegisterSucceeded_, NL_ERR_INTERNAL_ERROR, "isRegisterSucceeded_ is false.");

    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");
    Uuid pUuid = Uuid::ConvertFromString(uuid.ToString());
    NlErrCode result = proxy->DiscoverServiceByUuid(applicationId_, pUuid);
    if (result != NL_NO_ERROR) {
        FindStructureByUuidComplete(NL_ERR_INTERNAL_ERROR, uuid);
    }
    return result;
}

void SsapClient::impl::FindStructureComplete(int state)
{
    bool ret = false;
    {
        std::unique_lock<std::mutex> lock(findInformation_.clientMutex);
        if (findInformation_.isFinding) {
            findInformation_.isFinding = false;
            isGetServiceYet_ = false;
            findInformation_.condition.notify_all();
            ret = true;
        }
    }
    if (ret) {
        std::shared_ptr<SsapClientCallback> callbackSptr = (callback_).lock();
        if (!callbackSptr) {
            HILOGE("callback is nullptr");
            return;
        }
        callbackSptr->OnServicesDiscovered(state);
    }
}

void SsapClient::impl::FindStructureByUuidComplete(int state, const UUID &uuid)
{
    bool ret = false;
    {
        std::unique_lock<std::mutex> lock(findInformation_.clientMutex);
        if (findInformation_.isFinding) {
            findInformation_.isFinding = false;
            isGetServiceYet_ = false;
            findInformation_.condition.notify_all();
            ret = true;
        }
    }
    if (ret) {
        std::shared_ptr<SsapClientCallback> callbackSptr = (callback_).lock();
        if (!callbackSptr) {
            HILOGE("callback is nullptr");
            return;
        }
        callbackSptr->OnServicesDiscoveredByUuid(state, uuid);
    }
}

std::shared_ptr<SsapService> SsapClient::impl::BuildService(const NearlinkSsapServiceParcel &src)
{
    HILOGI("enter");
        auto svcTmp = std::make_shared<SsapService>(UUID::ConvertFrom128Bits(src.uuid_.ConvertTo128Bits()),
            src.handle_,
            src.endHandle_,
            src.isPrimary_ ? SsapServiceType::VENDOR_PROMARY : SsapServiceType::VENDOR_SECONDARY);
        for (auto &isvc : src.includeServices_) {
            auto isvcTmp = std::make_shared<SsapService>(UUID::ConvertFrom128Bits(isvc.uuid_.ConvertTo128Bits()),
            isvc.handle_,
            isvc.endHandle_,
            isvc.isPrimary_ ? SsapServiceType::VENDOR_PROMARY : SsapServiceType::VENDOR_SECONDARY);
            svcTmp->AddService(isvcTmp);
        }
        for (auto &pp : src.properties_) {
            SsapProperty ppTmp(pp.handle_,
            0,
            UUID::ConvertFrom128Bits(pp.uuid_.ConvertTo128Bits()),
            pp.opInd_,
            pp.permission_);
            for (auto &pdp : pp.descriptors_) {
                SsapDescriptor pdpTmp(pdp.handle_, pdp.type_, pdp.permission_);
                pdpTmp.SetServiceUuid(svcTmp->GetUuid());
                ppTmp.AddDescriptor(pdpTmp);
            }
            svcTmp->AddProperty(ppTmp);
        }
        for (auto &mm : src.methods_) {
            SsapMethod mmTmp(mm.handle_,
            0,
            UUID::ConvertFrom128Bits(mm.uuid_.ConvertTo128Bits()),
             mm.permission_);
            svcTmp->AddMethod(mmTmp);
        }
        for (auto &ee : src.events_) {
            SsapEvent eeTmp(ee.handle_,
            0,
            UUID::ConvertFrom128Bits(ee.uuid_.ConvertTo128Bits()));
            svcTmp->AddEvent(eeTmp);
        }
        for (auto &dp : src.descriptors_) {
            SsapDescriptor dpTmp (
                dp.handle_,
                dp.type_,
                dp.permission_);
            svcTmp->AddDescriptor(dpTmp);
        }
        return svcTmp;
}

void SsapClient::impl::BuildServiceList(const std::vector<NearlinkSsapServiceParcel> &src)
{
    HILOGI("service size:%{public}zu", src.size());
    for (auto &svc : src) {
        std::shared_ptr<SsapService> service = BuildService(svc);
        if (service) {
            ssapServices_.EnsureInsert(service->GetHandle(), service);
        }
    }

    for (auto &svc : src) {
        std::shared_ptr<SsapService> service = FindService(svc.handle_);
        if (!service) {
            HILOGE("FindService failed, service is nullptr.");
            continue;
        }

        for (auto &iSvc : svc.includeServices_) {
            std::shared_ptr<SsapService> includeSvc = FindService(iSvc.startHandle_);
            if (!includeSvc) {
                HILOGE("FindService failed, service is nullptr.");
                continue;
            }
            service->AddService(includeSvc);
        }
    }
}

std::shared_ptr<SsapService> SsapClient::impl::FindService(uint16_t handle)
{
    std::shared_ptr<SsapService> svc = nullptr;
    if (!ssapServices_.GetValue(handle, svc)) {
        HILOGE("Can not find the service handle(0x%{public}04X)", handle);
        return nullptr;
    }
    return svc;
}

void SsapClient::impl::CleanConnectionInfo()
{
    FindStructureComplete(static_cast<int>(SsapStatus::SSAP_FAILURE));
}

NlErrCode SsapClient::impl::GetServices()
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    std::unique_lock<std::mutex> lock(findInformation_.clientMutex);
    while (findInformation_.isFinding) {
        auto ret = findInformation_.condition.wait_for(lock, std::chrono::seconds(WAIT_TIMEOUT));
        if (ret == std::cv_status::timeout) {
            HILOGE("timeout");
            return NL_ERR_TIMEOUT;
        }
    }
    if (isGetServiceYet_) {
        HILOGI("isGetServiceYet_ is true.");
        return NL_NO_ERROR;
    }
    NL_CHECK_RETURN_RET(isRegisterSucceeded_, NL_ERR_INTERNAL_ERROR, "isRegisterSucceeded_ is false.");

    ssapServices_.Clear();
    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    std::vector<NearlinkSsapServiceParcel> result;
    proxy->GetServices(applicationId_, result);
    BuildServiceList(result);
    isGetServiceYet_ = true;
    return NL_NO_ERROR;
}

NlErrCode SsapClient::Connect(std::shared_ptr<SsapClientCallback> callback)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    if (pimpl->connectionState_ == static_cast<int>(SleConnectState::CONNECTED)) {
        HILOGE("Already connected");
        return NL_ERR_INVALID_STATE;
    }

    HILOGI("isRegisterSucceeded: %{public}d", pimpl->isRegisterSucceeded_.load());
    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");
    if (pimpl->isRegisterSucceeded_) {
        return proxy->Connect(pimpl->applicationId_, false);
    }
    NL_CHECK_RETURN_RET(pimpl->clientCallback_, NL_ERR_INTERNAL_ERROR, "clientCallback_ is nullptr");
    pimpl->callback_ = callback;
    int appId = 0;
    NlErrCode result = proxy->RegisterApplication(pimpl->clientCallback_, SSAP_SEC_NONE,
              static_cast<NearlinkRawAddress>(RawAddress(pimpl->device_->GetDeviceAddr())), ADAPTER_SLE, appId);
    if (result != NL_NO_ERROR) {
        return result;
    }
    if (appId >= 0) {
        pimpl->applicationId_.store(appId);
        pimpl->isRegisterSucceeded_.store(true);
        result = proxy->Connect(pimpl->applicationId_, false);
    }
    return result;
}

NlErrCode SsapClient::Disconnect()
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    if (!pimpl->isRegisterSucceeded_) {
        HILOGE("Request not supported");
        return NL_ERR_INVALID_STATE;
    }

    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");
    return proxy->Disconnect(pimpl->applicationId_);
}

NlErrCode SsapClient::Close()
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");
    if (pimpl->isRegisterSucceeded_) {
        NlErrCode result = proxy->DeregisterApplication(pimpl->applicationId_);
        HILOGI("result: %{public}d", result);
        if (result == NL_NO_ERROR) {
            pimpl->isRegisterSucceeded_.store(false);
        }
        return result;
    }
    HILOGI("isRegisterSucceeded_ is false");
    return NL_NO_ERROR;
}

NlErrCode SsapClient::FindStructure()
{
    HILOGI("FinStructure enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    if (pimpl->connectionState_ != static_cast<int>(SleConnectState::CONNECTED)) {
        HILOGE("Request not supported");
        return NL_ERR_INTERNAL_ERROR;
    }

    return pimpl->FindStructureStart();
}

NlErrCode SsapClient::FindStructureByUuid(const UUID &uuid)
{
    HILOGI("FinStructureByUuid enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    if (pimpl->connectionState_ != static_cast<int>(SleConnectState::CONNECTED)) {
        HILOGE("Request not supported");
        return NL_ERR_INTERNAL_ERROR;
    }

    return pimpl->FindStructureByUuidStart(uuid);
}

std::shared_ptr<SsapService> SsapClient::GetService(const UUID &uuid)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), nullptr, "nearlink is off.");
    NL_CHECK_RETURN_RET(pimpl->isRegisterSucceeded_, nullptr, "isRegisterSucceeded_ is false.");

    {
        std::unique_lock<std::mutex> lock(pimpl->findInformation_.clientMutex);
        while (pimpl->findInformation_.isFinding) {
            auto ret = pimpl->findInformation_.condition.wait_for(lock, std::chrono::seconds(WAIT_TIMEOUT));
            if (ret == std::cv_status::timeout) {
                HILOGE("timeout");
                return nullptr;
            }
        }
    }

    std::vector<NearlinkSsapServiceParcel> result;
    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, nullptr, "proxy is nullptr.");
    Uuid pUuid = Uuid::ConvertFromString(uuid.ToString());
    NlErrCode status = proxy->GetServicesByUuid(pimpl->applicationId_, pUuid, result);
    size_t len = result.size();
    NL_CHECK_RETURN_RET(status == NL_NO_ERROR && len > 0, nullptr, "get service failed, status(%{public}d).", status);

    std::shared_ptr<SsapService> service = pimpl->BuildService(result[0]);
    if (service) {
        pimpl->ssapServices_.EnsureInsert(service->GetHandle(), service);
    }
    return service;
}

NlErrCode SsapClient::GetService(std::vector<SsapService> &services)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    NlErrCode status = pimpl->GetServices();
    NL_CHECK_RETURN_RET(status == NL_NO_ERROR, status, "failed.");

    pimpl->ssapServices_.Iterate([&services](const uint16_t handle, std::shared_ptr<SsapService> &service) -> void {
        services.push_back(*service);
    });

    return NL_NO_ERROR;
}

bool SsapClient::GetHandle(const UUID &serviceUuid, const UUID &propertyUuid, uint16_t &handle)
{
    return pimpl->ssapServices_.Find([&handle, &serviceUuid, &propertyUuid](const uint16_t serviceHandle,
        std::shared_ptr<SsapService> &service) -> bool {
        if (!(service->GetUuid().Equals(serviceUuid))) {
            return false;
        }
        for (auto &sp : service->GetProperty()) {
            if (sp.GetUuid().Equals(propertyUuid)) {
                handle = sp.GetHandle();
                return true;
            }
        }
        return false;
    });
}

NlErrCode SsapClient::ReadProperty(SsapProperty &property)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    if (pimpl->connectionState_ != static_cast<int>(SleConnectState::CONNECTED) || !pimpl->isRegisterSucceeded_) {
        HILOGE("Request not supported");
        return NL_ERR_INTERNAL_ERROR;
    }

    uint16_t propertyHandle = property.GetHandle();
    if (!FindPropertyByUuid(property, propertyHandle)) {
        HILOGE("not find property handle which can be read handle%{public}d", propertyHandle);
        return NL_ERR_INVALID_PARAM;
    }
    NlErrCode result = NL_NO_ERROR;
    HILOGI("applicationId: %{public}d, handle: 0x%{public}04X", pimpl->applicationId_.load(), propertyHandle);
    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    result = proxy->ReadProperty(
        pimpl->applicationId_, (NearlinkSsapPropertyParcel)Nearlink::Property(
            propertyHandle,
            Uuid::ConvertFrom128Bits(property.GetUuid().ConvertTo128Bits()), property.GetOperationIndication()));
    HILOGI("result: %{public}d", result);
    return result;
}

NlErrCode SsapClient::CallMethod(SsapMethod &method)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    
    size_t length = 0;
    const uint8_t *pData = method.GetParameter(&length).get();

    HILOGI("%{public}s", method.GetUuid().GetEncryptUuid().c_str());

    if (pData == nullptr || length == 0) {
        HILOGE("Invalid parameters");
        return NL_ERR_INTERNAL_ERROR;
    }
    std::vector<uint8_t> value(pData, pData + length);
    return CallMethod(method, std::move(value));
}

NlErrCode SsapClient::CallMethod(SsapMethod &method, std::vector<uint8_t> value)
{
    if (pimpl->connectionState_ != static_cast<int>(SleConnectState::CONNECTED)) {
        HILOGE("Request not supported");
        return NL_ERR_INTERNAL_ERROR;
    }

    uint16_t methodHandle = method.GetHandle();
    if (!FindMethodByUuid(method, methodHandle)) {
        HILOGE("not find method handle which can be read handle%{public}d", methodHandle);
        return NL_ERR_INVALID_PARAM;
    }
    bool withoutRespond = false;

    NearlinkSsapMethodParcel methd(Nearlink::Method(methodHandle, value));

    NlErrCode result = NL_NO_ERROR;
    HILOGI("applicationId: %{public}d, handle: 0x%{public}04X", pimpl->applicationId_.load(), methodHandle);
    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    result = proxy->CallMethod(pimpl->applicationId_, &methd, withoutRespond);
    HILOGI("result: %{public}d", result);
    return result;
}

NlErrCode SsapClient::ReadPropertyByUuid(SsapProperty &property)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    if (pimpl->connectionState_ != static_cast<int>(SleConnectState::CONNECTED) || !pimpl->isRegisterSucceeded_) {
        HILOGE("Request not supported");
        return NL_ERR_INTERNAL_ERROR;
    }

    uint16_t propertyHandle = property.GetHandle();
    NlErrCode result = NL_NO_ERROR;
    HILOGI("applicationId: %{public}d, handle: 0x%{public}04X", pimpl->applicationId_.load(), propertyHandle);
    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    result = proxy->ReadProperty(
        pimpl->applicationId_, (NearlinkSsapPropertyParcel)Nearlink::Property(
            propertyHandle,
            Uuid::ConvertFrom128Bits(property.GetUuid().ConvertTo128Bits()), property.GetOperationIndication()));
    HILOGI("result: %{public}d", result);
    return result;
}

NlErrCode SsapClient::WriteProperty(SsapProperty &property)
{
    size_t length = 0;
    const uint8_t *pData = property.GetValue(&length).get();
    if (pData == nullptr || length == 0) {
        HILOGE("Invalid parameters");
        return NL_ERR_INTERNAL_ERROR;
    }
    std::vector<uint8_t> value(pData, pData + length);
    return WriteProperty(property, std::move(value));
}

bool SsapClient::FindPropertyByUuid(SsapProperty &property, uint16_t &propertyHandle)
{
    return pimpl->ssapServices_.Find([&propertyHandle, &property](const uint16_t handle,
        std::shared_ptr<SsapService> &service) -> bool {
        for (auto &sp : service->GetProperty()) {
            if (sp.GetUuid().Equals(property.GetUuid())) {
                propertyHandle = sp.GetHandle();
                return true;
            }
        }
        return false;
    });
}

bool SsapClient::FindMethodByUuid(SsapMethod &method, uint16_t &methodHandle)
{
    return pimpl->ssapServices_.Find([&methodHandle, &method](const uint16_t handle,
        std::shared_ptr<SsapService> &service) -> bool {
        for (auto &sm : service->GetMethod()) {
            if (sm.GetUuid().Equals(method.GetUuid())) {
                methodHandle = sm.GetHandle();
                HILOGI("methodHandle:%{public}hu", methodHandle);
                return true;
            }
        }
        return false;
    });
}

NlErrCode SsapClient::WriteProperty(SsapProperty &property, std::vector<uint8_t> value)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    if (pimpl->connectionState_ != static_cast<int>(SleConnectState::CONNECTED)) {
        HILOGE("Request not supported");
        return NL_ERR_INTERNAL_ERROR;
    }

    uint16_t propertyHandle = property.GetHandle();
    if (!FindPropertyByUuid(property, propertyHandle)) {
        HILOGE("not find property handle which can be write handle%{public}d", propertyHandle);
        return NL_ERR_INVALID_PARAM;
    }
    NearlinkSsapPropertyParcel proper(
        Nearlink::Property(propertyHandle, value));
    NlErrCode result = NL_NO_ERROR;
    bool withoutRespond = true;
    withoutRespond = ((property.GetWriteType() ==
        static_cast<int>(SsapProperty::PropertyWriteType::PROPERTY_WRITE_REQ)) ? false : true);
    HILOGD("Write without response");
    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");
    result = proxy->WriteProperty(pimpl->applicationId_, &proper, withoutRespond);
    HILOGD("result: %{public}d", result);
    return result;
}

NlErrCode SsapClient::ReadDescriptor(SsapDescriptor &descriptor)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    if (pimpl->connectionState_ != static_cast<int>(SleConnectState::CONNECTED) || !pimpl->isRegisterSucceeded_) {
        HILOGE("Request not supported");
        return NL_ERR_INTERNAL_ERROR;
    }

    NlErrCode result = NL_NO_ERROR;
    HILOGI("applicationId: %{public}d, handle: 0x%{public}04X", pimpl->applicationId_.load(), descriptor.GetHandle());
    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");
    NearlinkSsapDescriptorParcel desc(
        Nearlink::Descriptor(descriptor.GetHandle(), descriptor.GetDescriptorType()));

    result = proxy->ReadDescriptor(pimpl->applicationId_, desc);
    HILOGI("result: %{public}d", result);

    return result;
}

NlErrCode SsapClient::WriteDescriptor(SsapDescriptor &descriptor)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    if (pimpl->connectionState_ != static_cast<int>(SleConnectState::CONNECTED) || !pimpl->isRegisterSucceeded_) {
        HILOGE("Request not supported");
        return NL_ERR_INTERNAL_ERROR;
    }

    size_t length = 0;
    const uint8_t *pData = descriptor.GetValue(&length).get();
    if (pData == nullptr || length == 0) {
        HILOGE("Invalid parameters");
        return NL_ERR_INTERNAL_ERROR;
    }
    std::vector<uint8_t> descriptorValue(pData, pData + length);
    NlErrCode result = NL_NO_ERROR;
    NearlinkSsapDescriptorParcel desc(
        Nearlink::Descriptor(descriptor.GetHandle(), descriptor.GetDescriptorType(), std::move(descriptorValue)));
    desc.permission_= descriptor.GetDescriptorPermission();
    bool withoutRespond = false;
    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    result = proxy->WriteDescriptor(pimpl->applicationId_, &desc, withoutRespond);
    HILOGI("result: %{public}d", result);
    return result;
}

NlErrCode SsapClient::RequestSleMtuSize(int mtu)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    if (pimpl->connectionState_ != static_cast<int>(SleConnectState::CONNECTED) || !pimpl->isRegisterSucceeded_) {
        HILOGE("Request not supported");
        return NL_ERR_INTERNAL_ERROR;
    }

    HILOGI("applicationId: %{public}d, mtu: %{public}d", pimpl->applicationId_.load(), mtu);
    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    return proxy->RequestExchangeMtu(pimpl->applicationId_, mtu);
}

NlErrCode SsapClient::RequestConnectionPriority(int connPriority)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    if (pimpl->connectionState_ != static_cast<int>(SleConnectState::CONNECTED)) {
        HILOGE("Not connected");
        return NL_ERR_INTERNAL_ERROR;
    }

    if (connPriority != static_cast<int>(SsapConnectionPriority::BALANCED) &&
        connPriority != static_cast<int>(SsapConnectionPriority::HIGH) &&
        connPriority != static_cast<int>(SsapConnectionPriority::LOW_POWER)) {
        HILOGE("Invalid parameters");
        return NL_ERR_INTERNAL_ERROR;
    }

    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    return proxy->RequestConnectionPriority(pimpl->applicationId_, connPriority);
}

std::string SsapClient::GetDeviceAddr()
{
    return pimpl->device_->GetDeviceAddr();
}

NlErrCode SsapClient::SetNotifyPropertyInner(SsapProperty &property, bool enable, uint8_t notifyOption)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    if (pimpl->connectionState_ != static_cast<int>(SleConnectState::CONNECTED)) {
        HILOGE("Request not supported");
        return NL_ERR_INTERNAL_ERROR;
    }

    uint16_t propertyHandle = property.GetHandle();
    if (!FindPropertyByUuid(property, propertyHandle)) {
        HILOGE("not find property handle which can be set notify property handle%{public}d", propertyHandle);
        return NL_ERR_INVALID_PARAM;
    }
    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");
    NlErrCode result = proxy->RequestPropertyNotification(
        pimpl->applicationId_, propertyHandle, enable, notifyOption);
    HILOGI("result: %{public}d", result);
    return result;
}

NlErrCode SsapClient::SetNotifyProperty(SsapProperty &property, bool enable)
{
    HILOGI("handle: 0x%{public}04X, enable: %{public}d", property.GetHandle(), enable);
    return SetNotifyPropertyInner(property, enable, static_cast<uint8_t>(NotifyOption::SSAP_SET_NOTIFY));
}

NlErrCode SsapClient::SetIndicateProperty(SsapProperty &property, bool enable)
{
    HILOGI("handle: 0x%{public}04X, enable: %{public}d", property.GetHandle(), enable);
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    if (pimpl->connectionState_ != static_cast<int>(SleConnectState::CONNECTED)) {
        HILOGE("Request not supported");
        return NL_ERR_CONNECTION_NOT_ESTABLISHED;
    }
    uint16_t propertyHandle = property.GetHandle();
    if (!FindPropertyByUuid(property, propertyHandle)) {
        HILOGE("not find property handle which can be set notify property handle%{public}d", propertyHandle);
        return NL_ERR_INVALID_PARAM;
    }
    sptr<INearlinkSsapClient> proxy = GetProxy<INearlinkSsapClient>(PROFILE_SSAP_CLIENT);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");
    NlErrCode result = proxy->RequestPropertyNotification(
        pimpl->applicationId_, propertyHandle, enable, static_cast<uint8_t>(NotifyOption::SSAP_SET_INDICATE));
    HILOGI("result: %{public}d", result);
    return result;
}


} // namespace Nearlink
} // namespace OHOS