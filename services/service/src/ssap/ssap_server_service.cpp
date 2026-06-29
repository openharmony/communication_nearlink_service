/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "ssap_server_service.h"

#include "ClassCreator.h"
#include "nearlink_safe_map.h"
#include "ssap_type.h"
#include "ssap_inner_def.h"
#include "ssap_log.h"
#include "ssap_service_base.h"
#include "ssap_utils.h"
#include "ssap_based_services_manager.h"
#include "ssap_server_stack_adapter.h"
#include "ThreadUtil.h"
#include <future>

#include "ipc_skeleton.h"
#ifdef RES_SCHED_SUPPORT
#include "nearlink_freeze_utils.h"
#endif

namespace OHOS {
namespace Nearlink {

struct SsapServerService::impl : public SsapServiceBase {
    struct ServerApplication {
        ServerApplication(const std::shared_ptr<InterfaceSsapServerCallback> &callback, int32_t pid, int32_t uid)
            : callback_(callback), pid_(pid), uid_(uid) {}
        std::shared_ptr<InterfaceSsapServerCallback> callback_;
        int32_t pid_;
        int32_t uid_;
    };

    class AdapterCallback : public SsapServerStackCallback {
    public:
        AdapterCallback(impl &pimpl) : pimpl_(pimpl) {}
        void OnMtuChanged(int appId, const RawAddress &addr, uint16_t mtu) override;
        void OnAddService(int appId, Service &service, int ret) override;
        void OnSetPropertyValue(int appId, Property &property, int ret) override;
        void OnSetDescriptorValue(int appId, Descriptor &descriptor, int ret) override;
        void OnReadPropertyAuthorizeRequest(
            int appId, const RawAddress &addr, uint16_t requestId, Property &property) override;
        void OnReadDescriptorAuthorizeRequest(
            int appId, const RawAddress &addr, uint16_t requestId, Descriptor &descriptor) override;
        void OnWritePropertyAuthorizeRequest(
            int appId, const RawAddress &addr, uint16_t requestId, Property &property) override;
        void OnWriteDescriptorAuthorizeRequest(
            int appId, const RawAddress &addr, uint16_t requestId, Descriptor &descriptor) override;
        void OnReadProperty(int appId, const RawAddress &addr, Property &property, int ret) override;
        void OnReadDescriptor(int appId, const RawAddress &addr, Descriptor &descriptor, int ret) override;
        void OnWriteProperty(int appId, const RawAddress &addr, Property &property, int ret) override;
        void OnWriteDescriptor(int appId, const RawAddress &addr, Descriptor &descriptor, int ret) override;
        void OnNotifyProperty(int appId, const RawAddress &addr, Property &property, int ret) override;
        void OnConnectionStateChanged(int appId, const RawAddress &addr, int state, int ret, int reason) override;
        void OnDisable(void) override;
    private:
        impl &pimpl_;
    };

    impl(SsapServerService &self) : server_(self), basedServicesManager_(std::make_shared<SsapBasedServicesManager>()),
        adapterCallback_(*this), stackAdapter_(adapterCallback_) {}

    SsapServerService &server_;
    std::shared_ptr<SsapBasedServicesManager> basedServicesManager_;
    NearlinkSafeMap<int, std::shared_ptr<ServerApplication>> appSafeMap_;
    AdapterCallback adapterCallback_;
    SsapServerStackAdapter stackAdapter_;
};

void SsapServerService::impl::AdapterCallback::OnConnectionStateChanged(
    int appId, const RawAddress &addr, int state, int ret, int reason)
{
    std::shared_ptr<ServerApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        app->callback_->OnConnectionStateChanged(addr, SSAP_TRANSPORT_SLE, state, ret, reason);
#ifdef RES_SCHED_SUPPORT
    std::string action;
    if (state == static_cast<uint8_t>(SleConnectState::CONNECTED)) {
        action = "CONNECT";
    } else if (state == static_cast<uint8_t>(SleConnectState::DISCONNECTED)) {
        action = "ACTIVE_DISCONNECT";
    }
    NearlinkFreezeUtil::GetInstance()->ReportNlConnectStateToRss(
        app->pid_, app->uid_, action, "SSAP", std::to_string(reason));
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapServerService::impl::AdapterCallback::OnMtuChanged(int appId, const RawAddress &addr, uint16_t mtu)
{
    std::shared_ptr<ServerApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        app->callback_->OnMtuChanged(addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE), mtu);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ServerMtuChanged");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapServerService::impl::AdapterCallback::OnAddService(int appId, Service &service, int ret)
{
    std::shared_ptr<ServerApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        app->callback_->OnAddService(service, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ServerAddService");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapServerService::impl::AdapterCallback::OnSetPropertyValue(int appId, Property &property, int ret)
{
    std::shared_ptr<ServerApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        app->callback_->OnSetPropertyValue(property, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ServerSetPropertyValue");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapServerService::impl::AdapterCallback::OnSetDescriptorValue(int appId, Descriptor &descriptor, int ret)
{
    std::shared_ptr<ServerApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        app->callback_->OnSetDescriptorValue(descriptor, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ServerSetDescriptorValue");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapServerService::impl::AdapterCallback::OnReadPropertyAuthorizeRequest(
    int appId, const RawAddress &addr, uint16_t requestId, Property &property)
{
    std::shared_ptr<ServerApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        app->callback_->OnReadPropertyAuthorizeRequest(
            addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE), requestId, property);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(
            app->pid_, app->uid_, "SSAP", "ServerReadPropertyAuthorizeRequest");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapServerService::impl::AdapterCallback::OnReadDescriptorAuthorizeRequest(
    int appId, const RawAddress &addr, uint16_t requestId, Descriptor &descriptor)
{
    std::shared_ptr<ServerApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        app->callback_->OnReadDescriptorAuthorizeRequest(
            addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE), requestId, descriptor);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(
            app->pid_, app->uid_, "SSAP", "ServerReadDescriptorAuthorizeRequest");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapServerService::impl::AdapterCallback::OnWritePropertyAuthorizeRequest(
    int appId, const RawAddress &addr, uint16_t requestId, Property &property)
{
    std::shared_ptr<ServerApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        app->callback_->OnWritePropertyAuthorizeRequest(
            addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE), requestId, property);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(
            app->pid_, app->uid_, "SSAP", "ServerWritePropertyAuthorizeRequest");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapServerService::impl::AdapterCallback::OnWriteDescriptorAuthorizeRequest(
    int appId, const RawAddress &addr, uint16_t requestId, Descriptor &descriptor)
{
    std::shared_ptr<ServerApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        app->callback_->OnWriteDescriptorAuthorizeRequest(
            addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE), requestId, descriptor);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(
            app->pid_, app->uid_, "SSAP", "ServerWriteDescriptorAuthorizeRequest");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapServerService::impl::AdapterCallback::OnReadProperty(
    int appId, const RawAddress &addr, Property &property, int ret)
{
    std::shared_ptr<ServerApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        app->callback_->OnReadProperty(addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE), property, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ServerReadProperty");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapServerService::impl::AdapterCallback::OnReadDescriptor(
    int appId, const RawAddress &addr, Descriptor &descriptor, int ret)
{
    std::shared_ptr<ServerApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        app->callback_->OnReadDescriptor(addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE), descriptor, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ServerReadDescriptor");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapServerService::impl::AdapterCallback::OnWriteProperty(
    int appId, const RawAddress &addr, Property &property, int ret)
{
    std::shared_ptr<ServerApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        app->callback_->OnWriteProperty(addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE), property, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ServerWriteProperty");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapServerService::impl::AdapterCallback::OnWriteDescriptor(
    int appId, const RawAddress &addr, Descriptor &descriptor, int ret)
{
    std::shared_ptr<ServerApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        app->callback_->OnWriteDescriptor(addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE), descriptor, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ServerWriteDescriptor");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapServerService::impl::AdapterCallback::OnNotifyProperty(
    int appId, const RawAddress &addr, Property &property, int ret)
{
    std::shared_ptr<ServerApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        app->callback_->OnNotifyProperty(addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE), property, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ServerNotifyProperty");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapServerService::impl::AdapterCallback::OnDisable(void)
{
    pimpl_.server_.ClearApplication();
    pimpl_.server_.OnDisable(pimpl_.server_.Name(), true);
}

SsapServerService::SsapServerService()
    : utility::Context(PROFILE_NAME_SSAP_SERVER, "1.0.0"), pimpl(std::make_unique<SsapServerService::impl>(*this))
{
    SSAP_LOGI("start");
}

SsapServerService::~SsapServerService() = default;

utility::Context *SsapServerService::GetContext()
{
    return this;
}

void SsapServerService::ClearApplication()
{
    SSAP_LOGI("start");
    pimpl->appSafeMap_.Clear();
}

bool SsapServerService::IsValidAppId(int appId)
{
    return (appId < 0) ? false : true;
}

void SsapServerService::EnableTask()
{
    SSAP_LOGI("start");
    if (pimpl->InRunningState()) {
        SSAP_LOGW("impl already");
        pimpl->server_.OnEnable(pimpl->server_.Name(), true);
        return;
    }
    pimpl->Start();
    pimpl->basedServicesManager_->Enable();
    pimpl->server_.OnEnable(pimpl->server_.Name(), true);
}

void SsapServerService::Enable()
{
    SSAP_LOGI("start");
    DoInSsapThread([this]() -> void {
        EnableTask();
    });
}

void SsapServerService::DisableTask()
{
    SSAP_LOGI("start");
    if (!pimpl->InRunningState()) {
        SSAP_LOGW("impl already");
        pimpl->server_.OnDisable(pimpl->server_.Name(), true);
        return;
    }
    pimpl->basedServicesManager_->Disable();
    pimpl->Stop();
    pimpl->stackAdapter_.Disable();
}

void SsapServerService::Disable()
{
    SSAP_LOGI("start");
    DoInSsapThread([this]() -> void {
        DisableTask();
    });
}

int SsapServerService::RegisterApplicationTask(
    const std::shared_ptr<InterfaceSsapServerCallback> &callback, int32_t pid, int32_t uid)
{
    SSAP_LOGI("start");
    int appId = SSAP_INVALID_APPID;
    if (pimpl->stackAdapter_.RegisterApplication(appId) == SsapStatus::SSAP_SUCCESS) {
        std::shared_ptr<impl::ServerApplication> app = std::make_shared<impl::ServerApplication>(callback, pid, uid);
        pimpl->appSafeMap_.EnsureInsert(appId, app);
        pimpl->stackAdapter_.SetMtu(SSAP_MTU_MAX);
    }
    return appId;
}

int SsapServerService::RegisterApplication(const std::shared_ptr<InterfaceSsapServerCallback> &callback)
{
    SSAP_LOGI("start");
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("not enable");
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!callback) {
        SSAP_LOGE("invalid");
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    int32_t pid = IPCSkeleton::GetCallingPid();
    int32_t uid = IPCSkeleton::GetCallingUid();
    std::promise<int> promise;
    DoInSsapThread([this, cbk = callback, pid, uid, &promise] {
        int appId = RegisterApplicationTask(cbk, pid, uid);
        promise.set_value(appId);
    });
    return promise.get_future().get();
}

int SsapServerService::DeregisterApplicationTask(int appId)
{
    SSAP_LOGI("id=%{public}d", appId);
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.DeregisterApplication(appId);
        pimpl->appSafeMap_.Erase(appId);
        return SsapStatus::SSAP_SUCCESS;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
    return SsapStatus::SSAP_NO_APP;
}

int SsapServerService::DeregisterApplication(int appId)
{
    SSAP_LOGI("dereg sApp id=%{public}d", appId);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("dereg sApp not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("dereg sApp invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    std::promise<int> promise;
    DoInSsapThread([this, appId, &promise] {
        int ret = DeregisterApplicationTask(appId);
        promise.set_value(ret);
    });
    return promise.get_future().get();
}

void SsapServerService::SetMtuTask(int appId, uint16_t mtu)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.SetMtu(mtu);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapServerService::SetMtu(int appId, uint16_t mtu)
{
    SSAP_LOGI("setMtu id=%{public}d mtu=%{public}d", appId, mtu);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("setMtu not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId) || mtu < SSAP_MTU_MIN || mtu > SSAP_MTU_MAX) {
        SSAP_LOGE("setMtu invalid=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    uint16_t realMtu = mtu < SSAP_MTU_DEFAULT ? SSAP_MTU_DEFAULT : mtu;
    DoInSsapThread([this, appId, realMtu] {
        SetMtuTask(appId, realMtu);
    });
    return SsapStatus::SSAP_SUCCESS;
}

void SsapServerService::AddServiceTask(int appId, Service &service)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.AddService(appId, service);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapServerService::AddService(int appId, const Service &service)
{
    SSAP_LOGI("add srvs id=%{public}d uuid=%{public}s", appId, GET_ENCRYPT_UUID(service.uuid_));
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("add srvs not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("add srvs invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, s = service]() mutable {
        AddServiceTask(appId, s);
    });
    return SsapStatus::SSAP_SUCCESS;
}

void SsapServerService::RemoveServiceTask(int appId, uint16_t handle)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.RemoveService(appId, handle);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapServerService::RemoveService(int appId, const Service &service)
{
    SSAP_LOGI("del srv id=%{public}d hdl=%{public}d uuid=%{public}s",
        appId, service.handle_, GET_ENCRYPT_UUID(service.uuid_));
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("del srv not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("del srv invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, hdl = service.handle_] {
        RemoveServiceTask(appId, hdl);
    });
    return SsapStatus::SSAP_SUCCESS;
}

void SsapServerService::ClearServicesTask(int appId)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.ClearServices(appId);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapServerService::ClearServices(int appId)
{
    SSAP_LOGI("clear srv id=%{public}d", appId);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("clear srv not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("clear srv invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId] {
        ClearServicesTask(appId);
    });
    return SsapStatus::SSAP_SUCCESS;
}

bool SsapServerService::CheckServiceExistByUuidTask(int32_t appId, const Uuid &uuid)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        return pimpl->stackAdapter_.CheckServiceExistByUuid(appId, uuid);
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
    return false;
}

bool SsapServerService::CheckServiceExistByUuid(int appId, const Uuid &uuid)
{
    SSAP_LOGI("server get srv id=%{public}d", appId);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("server get srv not enable id=%{public}d", appId);
        return false;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("server get srv invald=%{public}d", appId);
        return false;
    }

    std::promise<bool> promise;
    DoInSsapThread([this, appId, &uuid, &promise] {
        bool ret = CheckServiceExistByUuidTask(appId, uuid);
        promise.set_value(ret);
    });
    return promise.get_future().get();
}

void SsapServerService::SetPropertyValueTask(int appId, Property &property)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.SetPropertyValue(appId, property);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapServerService::SetPropertyValue(int appId, Property &property)
{
    SSAP_LOGI("set prop id=%{public}d hdl=%{public}d", appId, property.handle_);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("set prop not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("set prop invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, p = Property(property.handle_, std::move(property.value_))]() mutable {
        SetPropertyValueTask(appId, p);
    });
    property.value_ = std::vector<uint8_t>();
    return SsapStatus::SSAP_SUCCESS;
}

void SsapServerService::SetDescriptorValueTask(int appId, Descriptor &descriptor)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.SetDescriptorValue(appId, descriptor);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapServerService::SetDescriptorValue(int appId, Descriptor &descriptor)
{
    SSAP_LOGI("set desc id=%{public}d hdl=%{public}d type=%{public}d", appId, descriptor.handle_, descriptor.type_);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("set desc not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId) || descriptor.type_ == DESC_TYPE_PROPERTY_RESERVE) {
        SSAP_LOGE("set desc invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread(
        [this, appId, d = Descriptor(descriptor.handle_, descriptor.type_, std::move(descriptor.value_))]() mutable {
            SetDescriptorValueTask(appId, d);
        });
    descriptor.value_ = std::vector<uint8_t>();
    return SsapStatus::SSAP_SUCCESS;
}

std::vector<uint8_t> SsapServerService::GetPropertyValue(int appId, const Property &property)
{
    return std::vector<uint8_t>();
}

std::vector<uint8_t> SsapServerService::GetDescriptorValue(int appId, const Descriptor &descriptor)
{
    return std::vector<uint8_t>();
}

void SsapServerService::AuthorizeResponseTask(int appId, uint16_t requestId, bool allow)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.AuthorizeResponse(appId, requestId, allow);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapServerService::AuthorizeResponse(int appId, uint16_t requestId, bool allow)
{
    SSAP_LOGI("auth rsp id=%{public}d reqId=%{public}d allow=%{public}d", appId, requestId, allow);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("auth rsp not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("auth rsp invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, requestId, allow] {
        AuthorizeResponseTask(appId, requestId, allow);
    });
    return SsapStatus::SSAP_SUCCESS;
}

void SsapServerService::NotifyPropertyTask(int appId, Property &property, const RawAddress &addr)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.Notify(appId, property, addr);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapServerService::NotifyProperty(int appId, Property &property, const RawAddress &addr, uint8_t transport)
{
    SSAP_LOGI("ntf prop id=%{public}d hdl=%{public}d addr=%{public}s tp=%{public}d", appId, property.handle_,
        ADDR_LOG(addr), transport);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("ntf prop not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId) || transport >= SSAP_TRANSPORT_MAX) {
        SSAP_LOGE("ntf prop invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread(
        [this, appId, ntfProp = Property(property.handle_, std::move(property.value_)), addr]() mutable {
            NotifyPropertyTask(appId, ntfProp, addr);
        });
    property.value_ = std::vector<uint8_t>();
    return SsapStatus::SSAP_SUCCESS;
}

int SsapServerService::IndicateProperty(int appId, Property &property, const RawAddress &addr, uint8_t transport)
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}

int SsapServerService::NotifyEvent(
    int appId, const Event &event, std::vector<uint8_t> &value, const RawAddress &addr, uint8_t transport)
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}

int SsapServerService::IndicateEvent(
    int appId, const Event &event, std::vector<uint8_t> &value, const RawAddress &addr, uint8_t transport)
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}

int SsapServerService::ReturnMethod(
    int appId, const Method &method, std::vector<uint8_t> &value, const RawAddress &addr, uint8_t transport)
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}

int SsapServerService::Connect(int appId, const RawAddress &addr, uint8_t transport)
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}

int SsapServerService::Disconnect(int appId, const RawAddress &addr, uint8_t transport)
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}

int SsapServerService::Connect(const RawAddress &device)
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}
int SsapServerService::Disconnect(const RawAddress &device)
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}
std::list<RawAddress> SsapServerService::GetConnectDevices()
{
    return std::list<RawAddress>();
}
int SsapServerService::GetConnectState()
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}
REGISTER_CLASS_CREATOR(SsapServerService);
} // namespace Nearlink
} // namespace OHOS
