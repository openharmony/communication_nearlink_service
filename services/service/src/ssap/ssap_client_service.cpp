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
#include "ssap_client_service.h"
#include "ClassCreator.h"
#include "nearlink_safe_map.h"
#include "ssap_log.h"
#include "ssap_service_base.h"
#include "ssap_utils.h"
#include "ssap_client_stack_adapter.h"
#include "ssap_inner_def.h"
#include "ssap_type.h"
#include "nearlink_dft_exception.h"
#include "nearlink_permission_manager.h"
#include "ThreadUtil.h"
#include <future>

#include "SleRemoteDeviceAdapter.h"

#include "ipc_skeleton.h"
#ifdef RES_SCHED_SUPPORT
#include "nearlink_freeze_utils.h"
#endif

namespace OHOS {
namespace Nearlink {

struct SsapClientService::impl : public SsapServiceBase {
    struct ClientApplication {
        ClientApplication(const std::shared_ptr<InterfaceSsapClientCallback> &callback, const RawAddress &addr,
            uint32_t pid, uint32_t uid)
            : callback_(callback), addr_(addr), pid_(pid), uid_(uid)
        {}

        const std::shared_ptr<InterfaceSsapClientCallback> callback_;
        RawAddress addr_;
        uint32_t pid_;
        uint32_t uid_;
    };

    class AdapterCallback : public SsapClientStackCallback {
    public:
        AdapterCallback(impl &pimpl) : pimpl_(pimpl) {}
        void OnMtuChanged(int appId, uint16_t mtu, int ret) override;
        void OnDiscoverComplete(int appId, int ret) override;
        void OnDiscoverByUuidComplete(int appId, const Uuid &uuid, int ret) override;
        void OnReadProperty(int appId, Property &property, int ret) override;
        void OnCallMethod(int appId, Method &method, int ret) override;
        void OnReadDescriptor(int appId, Descriptor &descriptor, int ret) override;
        void OnReadPropertiesByUuid(int appId, std::list<Property> &properties, int ret) override;
        void OnWriteProperty(int appId, Property &property, int ret) override;
        void OnWriteDescriptor(int appId, Descriptor &descriptor, int ret) override;
        void OnGetPropertyNotification(int appId, const Property &property, bool enable, int ret) override;
        void OnGetPropertyIndication(int appId, const Property &property, bool enable, int ret) override;
        void OnSetPropertyNotification(int appId, const Property &property, bool enable, int ret) override;
        void OnSetPropertyIndication(int appId, const Property &property, bool enable, int ret) override;
        void OnPropertyChanged(int appId, const Property &property) override;
        void OnEvent(int appId, const Event &event) override;
        void OnServicesRediscovered(int appId, std::vector<Service> &services) override;
        void OnServiceChanged(int appId, uint16_t handle, const Uuid &uuid) override;
        void OnConnectionStateChanged(int appId, int state, int ret, int reason) override;
        void OnDisable(void) override;
    private:
        impl &pimpl_;
    };

    impl(SsapClientService &self) : client_(self), adapterCallback_(*this),
        stackAdapter_(adapterCallback_) {}

    SsapClientService &client_;
    NearlinkSafeMap<int, std::shared_ptr<ClientApplication>> appSafeMap_;
    AdapterCallback adapterCallback_;
    SsapClientStackAdapter stackAdapter_;
};

void SsapClientService::impl::AdapterCallback::OnConnectionStateChanged(int appId, int state, int ret, int reason)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGD("appId=%{public}d, addr=%{public}s, state=%{public}d", appId, ADDR_LOG(app->addr_), state);
        int reportReason = (ret != SsapStatus::SSAP_SUCCESS) ? ret : reason;

#ifdef RES_SCHED_SUPPORT
        std::string action;
        if (state == static_cast<uint8_t>(SleConnectState::CONNECTED)) {
            action = "Connect";
        } else if (state == static_cast<uint8_t>(SleConnectState::DISCONNECTED)) {
            action = "ACTIVE_DISCONNECT";
        }
        NearlinkFreezeUtil::GetInstance()->ReportNlConnectStateToRss(
            app->pid_, app->uid_, action, "SSAP", "SSAP " + action);
#endif
        if (state == static_cast<uint8_t>(SleConnectState::CONNECTED)) {
            DftCachePairConnTime(app->addr_.GetAddress(), PAIR_CONN_PATH_SSAP, SLE_SSAP_FINISH_TIME);
            DftCacheAccurateSearchConnInfo(app->addr_.GetAddress(), DFT_SSAP_CONN, DFT_CONN_SUCCESS);
        } else if (state == static_cast<uint8_t>(SleConnectState::DISCONNECTED)) {
            if (reportReason != 0) {
                DftReportPairInfo(app->addr_.GetAddress(), PAIR_CONN_PATH_SSAP, reportReason);
                DftReportAccurateSearchConnFailInfo(app->addr_.GetAddress(), DFT_SSAP_CONN, DFT_CONN_FAILED,
                    reportReason);
            }
        }

        app->callback_->OnConnectionStateChanged(state, reportReason);
        return;
    }
    HILOG_COMM_ERROR("[%{public}s:%{public}d]no app : appId=%{public}d", __FUNCTION__, __LINE__, appId);
}

void SsapClientService::impl::AdapterCallback::OnMtuChanged(int appId, uint16_t mtu, int ret)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, mtu=%{public}d ret=%{public}d", appId, mtu, ret);
        app->callback_->OnMtuChanged(mtu, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientMtuChanged");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnDiscoverComplete(int appId, int ret)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, ret=%{public}d", appId, ret);
        app->callback_->OnDiscoverComplete(ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientDiscoverComplete");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnDiscoverByUuidComplete(int appId, const Uuid &uuid, int ret)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, uuid=%{public}s ret=%{public}d", appId, GET_ENCRYPT_UUID(uuid), ret);
        app->callback_->OnDiscoverByUuidComplete(uuid, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientDiscoverByUuidComplete");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnReadProperty(int appId, Property &property, int ret)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, phdl=%{public}d ret=%{public}d", appId, property.handle_, ret);
        app->callback_->OnReadProperty(property, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientReadProperty");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnCallMethod(int appId, Method &method, int ret)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, mhdl=%{public}d ret=%{public}d", appId, method.handle_, ret);
        app->callback_->OnCallMethod(method, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientCallMethod");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnReadDescriptor(int appId, Descriptor &descriptor, int ret)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, dhdl=%{public}d type=%{public}d ret=%{public}d",
            appId, descriptor.handle_, descriptor.type_, ret);
        app->callback_->OnReadDescriptor(descriptor, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientReadDescriptor");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnReadPropertiesByUuid(
    int appId, std::list<Property> &properties, int ret)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, ret=%{public}d", appId, ret);
        app->callback_->OnReadPropertiesByUuid(properties, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientReadPropertiesByUuid");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnWriteProperty(int appId, Property &property, int ret)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, hdl=%{public}d ret=%{public}d", appId, property.handle_, ret);
        app->callback_->OnWriteProperty(property, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientWriteProperty");
#endif
        return;
    }
    SSAP_LOGD("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnWriteDescriptor(int appId, Descriptor &descriptor, int ret)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, hdl=%{public}d type=%{public}d ret=%{public}d",
            appId, descriptor.handle_, descriptor.type_, ret);
        app->callback_->OnWriteDescriptor(descriptor, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientWriteDescriptor");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnGetPropertyNotification(
    int appId, const Property &property, bool enable, int ret)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, hdl=%{public}d enable=%{public}d ret=%{public}d",
            appId, property.handle_, enable, ret);
        app->callback_->OnGetPropertyNotification(property, enable, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientGetPropertyNotification");
#endif 
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnGetPropertyIndication(
    int appId, const Property &property, bool enable, int ret)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, hdl=%{public}d enable=%{public}d ret=%{public}d",
            appId, property.handle_, enable, ret);
        app->callback_->OnGetPropertyIndication(property, enable, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientGetPropertyIndication");
#endif 
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnSetPropertyNotification(
    int appId, const Property &property, bool enable, int ret)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, hdl=%{public}d enable=%{public}d ret=%{public}d",
            appId, property.handle_, enable, ret);
        app->callback_->OnSetPropertyNotification(property, enable, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientSetPropertyNotification");
#endif 
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnSetPropertyIndication(
    int appId, const Property &property, bool enable, int ret)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, hdl=%{public}d enable=%{public}d ret=%{public}d",
            appId, property.handle_, enable, ret);
        app->callback_->OnSetPropertyIndication(property, enable, ret);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientSetPropertyIndication");
#endif 
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnPropertyChanged(int appId, const Property &property)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, hdl=%{public}d", appId, property.handle_);
        app->callback_->OnPropertyChanged(property);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientNotifyIndicate");
#endif 
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnEvent(int appId, const Event &event)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, hdl=%{public}d", appId, event.handle_);
        app->callback_->OnEvent(event);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientEventNotify");
#endif 
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnServicesRediscovered(int appId, std::vector<Service> &services)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, serviceNum=%{public}zu", appId, services.size());
        app->callback_->OnServicesRediscovered(services);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientServicesRediscovered");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnServiceChanged(int appId, uint16_t handle, const Uuid &uuid)
{
    std::shared_ptr<ClientApplication> app = nullptr;
    if (pimpl_.appSafeMap_.GetValue(appId, app)) {
        SSAP_LOGI("appId=%{public}d, handle=%{public}d", appId, handle);
        app->callback_->OnServiceChanged(handle, uuid);
#ifdef RES_SCHED_SUPPORT
        NearlinkFreezeUtil::GetInstance()->RequestActive(app->pid_, app->uid_, "SSAP", "ClientServiceChanged");
#endif
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

void SsapClientService::impl::AdapterCallback::OnDisable(void)
{
    SSAP_LOGI("SsapClientService OnDisable");
    pimpl_.client_.ClearApplication();
    pimpl_.client_.OnDisable(pimpl_.client_.Name(), true);
}

SsapClientService::SsapClientService()
    : utility::Context(PROFILE_NAME_SSAP_CLIENT, "1.0.0"), pimpl(std::make_unique<SsapClientService::impl>(*this))
{
    SSAP_LOGI("start");
}

SsapClientService::~SsapClientService() = default;

utility::Context *SsapClientService::GetContext()
{
    return this;
}

void SsapClientService::ClearApplication()
{
    SSAP_LOGI("start");
    pimpl->appSafeMap_.Clear();
}

bool SsapClientService::IsValidAppId(int appId)
{
    return (appId < 0) ? false : true;
}

void SsapClientService::EnableTask()
{
    SSAP_LOGI("start");
    if (pimpl->InRunningState()) {
        SSAP_LOGW("impl already");
        pimpl->client_.OnEnable(pimpl->client_.Name(), true);
        return;
    }
    pimpl->Start();
    pimpl->client_.OnEnable(pimpl->client_.Name(), true);
}

void SsapClientService::Enable()
{
    SSAP_LOGI("start");
    DoInSsapThread([this]() -> void {
        EnableTask();
    });
}

void SsapClientService::DisableTask()
{
    SSAP_LOGI("start");
    if (!pimpl->InRunningState()) {
        SSAP_LOGW("impl already");
        pimpl->client_.OnDisable(pimpl->client_.Name(), true);
        return;
    }
    pimpl->Stop();
    pimpl->stackAdapter_.Disable();
}

void SsapClientService::Disable()
{
    SSAP_LOGI("start");
    DoInSsapThread([this]() -> void {
        DisableTask();
    });
}

int SsapClientService::RegisterApplicationTask(const std::shared_ptr<InterfaceSsapClientCallback> &callback,
    const RawAddress &addr, SsapSecureType secureReq, int32_t pid, int32_t uid)
{
    SSAP_LOGD("start");
    int appId = SSAP_INVALID_APPID;
    if (pimpl->stackAdapter_.RegisterApplication(appId, addr, secureReq) == SsapStatus::SSAP_SUCCESS) {
        std::shared_ptr<impl::ClientApplication> app =
            std::make_shared<impl::ClientApplication>(callback, addr, pid, uid);
        pimpl->appSafeMap_.EnsureInsert(appId, app);
    }
    return appId;
}

int SsapClientService::RegisterApplication(const std::shared_ptr<InterfaceSsapClientCallback> &callback,
    const RawAddress &addr, uint8_t transport, uint8_t secureReq)
{
    SSAP_LOGI("addr=%{public}s,tp=%{public}d,sec=%{public}d", ADDR_LOG(addr), transport, secureReq);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("not enable addr=%{public}s,tp=%{public}d,sec=%{public}d", ADDR_LOG(addr), transport, secureReq);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (transport >= SSAP_TRANSPORT_MAX ||
        secureReq >= SSAP_SEC_MAX || !callback) {
        SSAP_LOGE("invald addr=%{public}s,tp=%{public}d,sec=%{public}d", ADDR_LOG(addr), transport, secureReq);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    SsapSecureType sec = static_cast<SsapSecureType>(secureReq);
    
    int32_t pid = IPCSkeleton::GetCallingPid();
    int32_t uid = IPCSkeleton::GetCallingUid();
    
    std::promise<int> promise;
    DoInSsapThread([this, cbk = callback, address = addr, sec, pid, uid, &promise] {
        int appId = RegisterApplicationTask(cbk, address, sec, pid, uid);
        promise.set_value(appId);
    });
    return promise.get_future().get();
}

int SsapClientService::DeregisterApplicationTask(int appId)
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

int SsapClientService::DeregisterApplication(int appId)
{
    SSAP_LOGD("dereg cApp id=%{public}d", appId);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("dereg cApp not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("dereg cApp invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    std::promise<int> promise;
    DoInSsapThread([this, appId, &promise] {
        int ret = DeregisterApplicationTask(appId);
        promise.set_value(ret);
    });
    return promise.get_future().get();
}

void SsapClientService::ExchangeMtuTask(int appId, uint16_t mtu)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.ExchangeMtu(appId, mtu);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapClientService::ExchangeMtu(int appId, uint16_t mtu)
{
    SSAP_LOGI("reqMtu id=%{public}d mtu=%{public}d", appId, mtu);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("reqMtu not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId) || mtu < SSAP_MTU_DEFAULT || mtu > SSAP_MTU_MAX) {
        SSAP_LOGE("reqMtu invalid=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, mtu] {
        ExchangeMtuTask(appId, mtu);
    });
    return SsapStatus::SSAP_SUCCESS;
}

void SsapClientService::DiscoverServicesTask(int appId)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.DiscoverServices(appId);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapClientService::DiscoverServices(int appId)
{
    SSAP_LOGI("find all srv appId=%{public}d", appId);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("find all srv not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("find all srv invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId] {
        DiscoverServicesTask(appId);
    });
    return SsapStatus::SSAP_SUCCESS;
}

void SsapClientService::DiscoverServicesByUuidTask(int appId, const Uuid &uuid, uint16_t startHandle,
    uint16_t endHandle)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.DiscoverServicesByUuid(appId, uuid, startHandle, endHandle);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapClientService::DiscoverServicesByUuid(int appId, const Uuid &uuid, uint16_t startHandle, uint16_t endHandle)
{
    SSAP_LOGI("find uuid=%{public}s id=%{public}d shdl=%{public}d ehdl=%{public}d",
        GET_ENCRYPT_UUID(uuid), appId, startHandle, endHandle);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("find uuid not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId) || startHandle == INVALID_ENTRY_HANDLE ||
        endHandle == INVALID_ENTRY_HANDLE || startHandle > endHandle) {
        SSAP_LOGE("find uuid invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, uuid, startHandle, endHandle] {
        DiscoverServicesByUuidTask(appId, uuid, startHandle, endHandle);
    });
    return SsapStatus::SSAP_SUCCESS;
}

std::list<Service> SsapClientService::GetServicesTask(int appId)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        return pimpl->stackAdapter_.GetServices(appId);
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
    return std::list<Service>();
}

std::list<Service> SsapClientService::GetServices(int appId)
{
    SSAP_LOGD("client get srvs id=%{public}d", appId);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("client get srvs not enable id=%{public}d", appId);
        return std::list<Service>();
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("client get srvs invald=%{public}d", appId);
        return std::list<Service>();
    }

    std::list<Service> clientSrvs;
    std::promise<void> promise;
    DoInSsapThread([this, appId, &clientSrvs, &promise] {
        clientSrvs = GetServicesTask(appId);
        promise.set_value();
    });
    promise.get_future().get();
    return clientSrvs;
}

std::list<Service> SsapClientService::GetServicesByUuidTask(int appId, const Uuid &uuid)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        return pimpl->stackAdapter_.GetServicesByUuid(appId, uuid);
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
    return std::list<Service>();
}

std::list<Service> SsapClientService::GetServicesByUuid(int appId, const Uuid &uuid)
{
    SSAP_LOGI("client get by uuid=%{public}s id=%{public}d", GET_ENCRYPT_UUID(uuid), appId);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("client get by uuid not enable id=%{public}d", appId);
        return std::list<Service>();
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("client get by uuid invald=%{public}d", appId);
        return std::list<Service>();
    }

    std::list<Service> clientSrvs;
    std::promise<void> promise;
    DoInSsapThread([this, appId, &uuid, &clientSrvs, &promise] {
        clientSrvs = GetServicesByUuidTask(appId, uuid);
        promise.set_value();
    });
    promise.get_future().get();
    return clientSrvs;
}

void SsapClientService::ReadPropertyTask(int appId, const Property &property)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.ReadProperty(appId, property);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapClientService::ReadProperty(int appId, const Property &property)
{
    SSAP_LOGI("read prop id=%{public}d hdl=%{public}d uuid=%{public}s",
        appId, property.handle_, GET_ENCRYPT_UUID(property.uuid_));
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("read prop not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("read prop invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, p = Property(property.handle_, property.uuid_)]() mutable {
        ReadPropertyTask(appId, p);
    });
    return SsapStatus::SSAP_SUCCESS;
}

void SsapClientService::ReadDescriptorTask(int appId, const Descriptor &descriptor)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.ReadDescriptor(appId, descriptor);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapClientService::ReadDescriptor(int appId, const Descriptor &descriptor)
{
    SSAP_LOGI("read desc id=%{public}d hdl=%{public}d type=%{public}d", appId, descriptor.handle_, descriptor.type_);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("read desc not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId) ||
        descriptor.type_ == DESC_TYPE_PROPERTY_RESERVE) {
        SSAP_LOGE("read desc invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, d = Descriptor(descriptor.handle_, descriptor.type_)]() mutable {
        ReadDescriptorTask(appId, d);
    });
    return SsapStatus::SSAP_SUCCESS;
}

void SsapClientService::ReadPropertiesByUuidTask(int appId, const Uuid &uuid, uint16_t startHandle, uint16_t endHandle)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.ReadPropertiesByUuid(appId, uuid, startHandle, endHandle);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapClientService::ReadPropertiesByUuid(int appId, const Uuid &uuid, uint16_t startHandle, uint16_t endHandle)
{
    SSAP_LOGI("read prop uuid=%{public}s id=%{public}d shdl=%{public}d ehdl=%{public}d",
        GET_ENCRYPT_UUID(uuid), appId, startHandle, endHandle);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("read prop uuid not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId) || startHandle == INVALID_ENTRY_HANDLE ||
        endHandle == INVALID_ENTRY_HANDLE || startHandle > endHandle) {
        SSAP_LOGE("read prop uuid invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, uuid, startHandle, endHandle] {
        ReadPropertiesByUuidTask(appId, uuid, startHandle, endHandle);
    });
    return SsapStatus::SSAP_SUCCESS;
}

int SsapClientService::ReadDescriptorsByUuid(
    int appId, const Uuid &uuid, uint8_t type, uint16_t startHandle, uint16_t endHandle)
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}

void SsapClientService::WritePropertyTask(int appId, Property &property, bool withoutRsp)
{
    std::shared_ptr<impl::ClientApplication> app = nullptr;
    if (pimpl->appSafeMap_.GetValue(appId, app)) {
        pimpl->stackAdapter_.WriteProperty(appId, property, withoutRsp);
        DftCacheMultideviceInfo(app->addr_.GetAddress(), NearLinkPermissionManager::GetCallingName(), DATA_TRANSPORT);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapClientService::WriteProperty(int appId, Property &property, bool withoutRsp)
{
    SSAP_LOGD("read prop id=%{public}d hdl=%{public}d withoutRsp=%{public}d", appId, property.handle_, withoutRsp);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("read prop not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("read prop invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, p = Property(property.handle_, std::move(property.value_)),
        withoutRsp]() mutable {
        WritePropertyTask(appId, p, withoutRsp);
    });
    property.value_ = std::vector<uint8_t>();
    return SsapStatus::SSAP_SUCCESS;
}

void SsapClientService::WriteDescriptorTask(int appId, Descriptor &descriptor, bool withoutRsp)
{
    std::shared_ptr<impl::ClientApplication> app = nullptr;
    if (pimpl->appSafeMap_.GetValue(appId, app)) {
        pimpl->stackAdapter_.WriteDescriptor(appId, descriptor, withoutRsp);
        DftCacheMultideviceInfo(app->addr_.GetAddress(), NearLinkPermissionManager::GetCallingName(), DATA_TRANSPORT);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapClientService::WriteDescriptor(int appId, Descriptor &descriptor, bool withoutRsp)
{
    SSAP_LOGI("write desc id=%{public}d hdl=%{public}d type=%{public}d withoutRsp=%{public}d", appId,
        descriptor.handle_, descriptor.type_, withoutRsp);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId) || descriptor.type_ == DESC_TYPE_CLIENT_CONFIG ||
        descriptor.type_ == DESC_TYPE_PROPERTY_RESERVE) {
        SSAP_LOGE("write desc invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId,
        d = Descriptor(descriptor.handle_, descriptor.type_, std::move(descriptor.value_)), withoutRsp]() mutable {
        WriteDescriptorTask(appId, d, withoutRsp);
    });
    descriptor.value_ = std::vector<uint8_t>();
    return SsapStatus::SSAP_SUCCESS;
}

void SsapClientService::GetPropertyNotificationTask(int appId, const Property &property)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.GetPropertyNotification(appId, property);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapClientService::GetPropertyNotification(int appId, const Property &property)
{
    SSAP_LOGI("get prop ntf id=%{public}d hdl=%{public}d", appId, property.handle_);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("get prop ntf not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("get prop ntf invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, p = Property(property.handle_, property.uuid_)]() mutable {
        GetPropertyNotificationTask(appId, p);
    });
    return SsapStatus::SSAP_SUCCESS;
}

void SsapClientService::SetPropertyNotificationTask(int appId, const Property &property, bool enable)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.SetPropertyNotification(appId, property, enable);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapClientService::SetPropertyNotification(int appId, const Property &property, bool enable)
{
    SSAP_LOGI("set prop ntf id=%{public}d hdl=%{public}d enable=%{public}d", appId, property.handle_, enable);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("set prop ntf not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("set prop ntf invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, p = Property(property.handle_, property.uuid_), enable]() mutable {
        SetPropertyNotificationTask(appId, p, enable);
    });
    return SsapStatus::SSAP_SUCCESS;
}

void SsapClientService::SetPropertyIndicationTask(int appId, const Property &property, bool enable)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.SetPropertyIndication(appId, property, enable);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapClientService::SetPropertyIndication(int appId, const Property &property, bool enable)
{
    SSAP_LOGI("set prop ind id=%{public}d hdl=%{public}d enable=%{public}d", appId, property.handle_, enable);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("set prop ind not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("set prop ind invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, p = Property(property.handle_, property.uuid_), enable]() mutable {
        SetPropertyIndicationTask(appId, p, enable);
    });
    return SsapStatus::SSAP_SUCCESS;
}

void SsapClientService::ConnectTask(int appId, bool autoConnect)
{
    std::shared_ptr<impl::ClientApplication> app = nullptr;
    if (pimpl->appSafeMap_.GetValue(appId, app)) {
        pimpl->stackAdapter_.Connect(appId, app->addr_, autoConnect);
        DftCacheSsapStart(app->addr_.GetAddress());
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapClientService::Connect(int appId, bool autoConnect)
{
    pimpl->appSafeMap_.GetValueAndOpt(appId, [](int key, std::shared_ptr<impl::ClientApplication> value){
        SleRemoteDeviceAdapter::GetInstance()->SetConnDirectActive(value->addr_);
    });
    SSAP_LOGD("client conn id=%{public}d auto=%{public}d", appId, autoConnect);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("client conn not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("client conn invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, autoConnect] {
        ConnectTask(appId, autoConnect);
    });
    return SsapStatus::SSAP_SUCCESS;
}

void SsapClientService::DisconnectTask(int appId)
{
    std::shared_ptr<impl::ClientApplication> app = nullptr;
    if (pimpl->appSafeMap_.GetValue(appId, app)) {
        pimpl->stackAdapter_.Disconnect(appId, app->addr_);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapClientService::Disconnect(int appId)
{
    SSAP_LOGI("client disconn id=%{public}d", appId);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("client disconn not enable id=%{public}d", appId);
        SSAP_LOGI("Disconnect ret=%{public}d", SsapStatus::SSAP_NOT_ENABLE);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("client disconn invald=%{public}d", appId);
        SSAP_LOGI("Disconnect ret=%{public}d", SsapStatus::SSAP_INVALID_PARAM);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId] {
        DisconnectTask(appId);
    });
    SSAP_LOGI("Disconnect SUCCESS");
    return SsapStatus::SSAP_SUCCESS;
}

int SsapClientService::SetEventNotification(int appId, const Event &event, bool enable)
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}

int SsapClientService::SetEventIndication(int appId, const Event &event, bool enable)
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}

void SsapClientService::CallMethodTask(int appId, Method &method, bool withoutRsp)
{
    if (pimpl->appSafeMap_.FindIf(appId)) {
        pimpl->stackAdapter_.CallMethod(appId, method, withoutRsp);
        return;
    }
    SSAP_LOGE("no app : appId=%{public}d", appId);
}

int SsapClientService::CallMethod(int appId, Method &method, bool withoutRsp)
{
    SSAP_LOGI("call method id=%{public}d hdl=%{public}d psize=%{public}lu withoutRsp=%{public}d",
        appId, method.handle_, method.parameter_.size(), withoutRsp);
    if (!pimpl->InRunningState()) {
        SSAP_LOGE("call method not enable id=%{public}d", appId);
        return SsapStatus::SSAP_NOT_ENABLE;
    }

    if (!IsValidAppId(appId)) {
        SSAP_LOGE("call method invald=%{public}d", appId);
        return SsapStatus::SSAP_INVALID_PARAM;
    }

    DoInSsapThread([this, appId, m = Method(method.handle_, method.parameter_), withoutRsp]() mutable {
        CallMethodTask(appId, m, withoutRsp);
    });
    return SsapStatus::SSAP_SUCCESS;
}

int SsapClientService::Connect(const RawAddress &device)
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}

int SsapClientService::Disconnect(const RawAddress &device)
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}

std::list<RawAddress> SsapClientService::GetConnectDevices()
{
    return std::list<RawAddress>();
}

int SsapClientService::GetConnectState()
{
    return SsapStatus::SSAP_NOT_SUPPORT;
}

REGISTER_CLASS_CREATOR(SsapClientService);
} // namespace Nearlink
} // namespace OHOS
