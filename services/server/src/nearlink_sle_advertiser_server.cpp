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

#include "nearlink_sle_advertiser_server.h"

#include <set>
#include "nearlink_errorcode.h"
#include "SleInterfaceAdapterSub.h"
#include "interface_advertiser_service.h"
#include "SleInterfaceManager.h"
#include "log_util.h"
#include "ipc_skeleton.h"
#include "remote_observer_list.h"
#include "nearlink_remote_container.h"

namespace OHOS {
namespace Nearlink {

enum class SleAdvOpcode : int {
    SLE_ADV_DEFAULT_OP_CODE = 0x00,
    SLE_ADV_STOP_COMPLETE_OP_CODE = 0x01,
    SLE_ADV_START_FAILED_OP_CODE = 0x02
};

struct NearlinkSleAdvertiserServer::impl : public std::enable_shared_from_this<impl> {
    impl();
    ~impl();

    void Init();
    SleAdvertiserDataImpl ConvertAdvertisingData(const NearlinkSleAdvertiserData &data) const;

    /// sys state observer
    class SystemStateObserver;
    std::unique_ptr<SystemStateObserver> systemStateObserver_ = nullptr;

    class SleAdvertiserCallback;
    std::shared_ptr<SleAdvertiserCallback> observerImp_;

    struct SleAdvertiserRemoteInfo;
    class SleAdvertiserRemoteContainer;
    // weak for deathReipient of container
    std::shared_ptr<SleAdvertiserRemoteContainer> remoteContainer_ =
        std::make_shared<SleAdvertiserRemoteContainer>();

    RemoteObserverList<INearlinkSleAdvertiseCallback> observers_;

private:
    std::weak_ptr<NearlinkSleAdvertiserServer::impl> slefAdvServerImplWeak_;
};

struct NearlinkSleAdvertiserServer::impl::SleAdvertiserRemoteInfo {
    SleAdvertiserRemoteInfo(int32_t pid, int32_t uid)
        : pid(pid), uid(uid) {}
    virtual ~SleAdvertiserRemoteInfo() = default;

    int32_t pid = 0;
    int32_t uid = 0;
    std::set<int32_t> advHandles {};
};

class NearlinkSleAdvertiserServer::impl::SleAdvertiserRemoteContainer final
    : public NearlinkRemoteContainer<NearlinkSleAdvertiserServer::impl::SleAdvertiserRemoteInfo> {
public:
    ~SleAdvertiserRemoteContainer() override = default;
    void OnRemoteDied(const wptr<IRemoteObject> &remote) override
    {
        HILOGI("enter");
        auto handles = GetAdvHandles(remote);
        for (int32_t handle : handles) {
            HILOGI("StopAdvertising handle: %{public}d", handle);
            InterfaceAdvertiserService::GetInstance().StopAdvertising(handle);
            RemoveAdvHandle(remote, handle);
        }
        {
            std::lock_guard<std::mutex> lk(vecMutex_);
            auto it = std::find_if(vec_.begin(), vec_.end(), [remote](const auto &obj) { return obj.first == remote; });
            NL_CHECK_RETURN(it != vec_.end(), "remote info unexpectedly not found");
            vec_.erase(it);
        }
    }

    void AddAdvHandle(int32_t pid, int32_t uid, int32_t handle)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        auto it = std::find_if(vec_.begin(), vec_.end(), [pid, uid](const auto &obj) {
            return obj.second.pid == pid && obj.second.uid == uid; });
        NL_CHECK_RETURN(it != vec_.end(), "remote info unexpectedly not found");
        it->second.advHandles.insert(handle);
        HILOGI("handle: %{public}d, pid: %{public}d, uid: %{public}d", handle, pid, uid);
    }

    void AddAdvHandle(const wptr<IRemoteObject> &remote, int32_t handle)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        auto it = std::find_if(vec_.begin(), vec_.end(), [remote](const auto &obj) { return obj.first == remote; });
        NL_CHECK_RETURN(it != vec_.end(), "remote info unexpectedly not found");
        it->second.advHandles.insert(handle);
    }

    void RemoveAdvHandle(const wptr<IRemoteObject> &remote, int32_t handle)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        auto it = std::find_if(vec_.begin(), vec_.end(), [remote](const auto &obj) { return obj.first == remote; });
        NL_CHECK_RETURN(it != vec_.end(), "remote info unexpectedly not found");
        it->second.advHandles.erase(handle);
    }

    void RemoveAdvHandle(int32_t handle)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        auto it = std::find_if(vec_.begin(), vec_.end(), [handle](const auto &obj) {
            return obj.second.advHandles.find(handle) != obj.second.advHandles.end(); });
        if (it == vec_.end()) {
            HILOGE("can't find advHandle: %{public}d", handle);
            return;
        }
        it->second.advHandles.erase(handle);
    }

    std::set<int32_t> GetAdvHandles(const wptr<IRemoteObject> &remote)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        auto it = std::find_if(vec_.begin(), vec_.end(), [remote](const auto &obj) { return obj.first == remote; });
        NL_CHECK_RETURN_RET(it != vec_.end(), {}, "remote info unexpectedly not found");
        return it->second.advHandles;
    }

    std::set<int32_t> GetAdvHandles(int32_t pid, int32_t uid)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        auto it = std::find_if(vec_.begin(), vec_.end(), [pid, uid](const auto &obj) {
            return obj.second.pid == pid && obj.second.uid == uid; });
        NL_CHECK_RETURN_RET(it != vec_.end(), {}, "remote info unexpectedly not found");
        return it->second.advHandles;
    }

    bool IsRemoteAdv(int32_t handle)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        int32_t pid = IPCSkeleton::GetCallingPid();
        int32_t uid = IPCSkeleton::GetCallingUid();
        auto it = std::find_if(vec_.begin(), vec_.end(), [handle](const auto &obj) {
            return obj.second.advHandles.find(handle) != obj.second.advHandles.end(); });
        if (it == vec_.end()) {
            HILOGE("can't find advHandle: %{public}d", handle);
            return false;
        }
        bool ret = (it->second.pid == pid && it->second.uid == uid);
        if (!ret) {
            HILOGE("handle: %{public}d, pid: %{public}d, uid: %{public}d", handle, pid, uid);
        }
        return ret;
    }
};

class NearlinkSleAdvertiserServer::impl::SleAdvertiserCallback : public ISleAdvertiserCallback {
public:
    SleAdvertiserCallback(std::weak_ptr<NearlinkSleAdvertiserServer::impl> advServerImpl) : impl_(advServerImpl){};
    ~SleAdvertiserCallback() override = default;

    void OnStartResultEvent(int result, uint8_t advHandle) override
    {
        int opcode = static_cast<int>(SleAdvOpcode::SLE_ADV_DEFAULT_OP_CODE);
        HILOGI("result: %{public}d, advHandle: %{public}d, opcode: %{public}d", result, advHandle, opcode);

        observers_->ForEach([this, result, advHandle, opcode](INearlinkSleAdvertiseCallback *observer) {
            observer->OnStartResultEvent(result, static_cast<int32_t>(advHandle), opcode);
        });
    }

    void OnStopResultEvent(int result, uint8_t advHandle) override
    {
        HILOGI("result: %{public}d, advHandle: %{public}d", result, advHandle);
        observers_->ForEach([this, result, advHandle](INearlinkSleAdvertiseCallback *observer) {
            observer->OnStopResultEvent(result, advHandle);
        });
        auto impl = impl_.lock();
        NL_CHECK_RETURN(impl, "impl has been destroyed");
        impl->remoteContainer_->RemoveAdvHandle(static_cast<int32_t>(advHandle));
    }

    void OnEnableResultEvent(int result, uint8_t advHandle) override
    {
        HILOGI("result: %{public}d, advHandle: %{public}d", result, advHandle);
        observers_->ForEach([this, result, advHandle](INearlinkSleAdvertiseCallback *observer) {
            observer->OnEnableResultEvent(result, advHandle);
        });
    }

    void OnDisableResultEvent(int result, uint8_t advHandle) override
    {
        HILOGI("result: %{public}d, advHandle: %{public}d", result, advHandle);
        observers_->ForEach([this, result, advHandle](INearlinkSleAdvertiseCallback *observer) {
            observer->OnDisableResultEvent(result, advHandle);
        });
    }

    void OnAutoStopAdvEvent(uint8_t advHandle) override
    {
        HILOGI("advHandle: %{public}d", advHandle);

        observers_->ForEach(
            [advHandle](INearlinkSleAdvertiseCallback *observer) { observer->OnAutoStopAdvEvent(advHandle); });
        auto impl = impl_.lock();
        NL_CHECK_RETURN(impl, "impl has been destroyed");
        impl->remoteContainer_->RemoveAdvHandle(static_cast<int32_t>(advHandle));
    }

    void OnSetAdvDataEvent(int result, uint8_t advHandle) override
    {
        HILOGI("result: %{public}d, advHandle: %{public}d", result, advHandle);

        observers_->ForEach([this, result, advHandle](INearlinkSleAdvertiseCallback *observer) {
            observer->OnSetAdvDataEvent(result, advHandle);
        });
    }

    void SetObserver(RemoteObserverList<INearlinkSleAdvertiseCallback> *observers)
    {
        observers_ = observers;
    }

    std::mutex uidMutex_;

private:
    // it's thread safety in observers_.
    RemoteObserverList<INearlinkSleAdvertiseCallback> *observers_ = nullptr;
    std::weak_ptr<NearlinkSleAdvertiserServer::impl> impl_;
};

class NearlinkSleAdvertiserServer::impl::SystemStateObserver : public ISystemStateObserver {
public:
    explicit SystemStateObserver(NearlinkSleAdvertiserServer::impl *pimpl) : pimpl_(pimpl) {};
    void OnSystemStateChange(const SleSystemState state) override
    {
        HILOGI("OnSystemStateChange %{public}d.", state);
        if (state == SleSystemState::ON) {
            InterfaceAdvertiserService::GetInstance().RegisterSleAdvertiserCallback(pimpl_->observerImp_);
        }
    };

private:
    NearlinkSleAdvertiserServer::impl *pimpl_ = nullptr;
};

NearlinkSleAdvertiserServer::impl::impl()
{
    HILOGI("starts");
}

void NearlinkSleAdvertiserServer::impl::Init()
{
    HILOGI("starts");
    remoteContainer_->Init();
    slefAdvServerImplWeak_ = shared_from_this();
    observerImp_ = std::make_shared<impl::SleAdvertiserCallback>(slefAdvServerImplWeak_);
}

NearlinkSleAdvertiserServer::impl::~impl()
{
    HILOGW("NearlinkSleAdvertiserServer: ~impl()");
    InterfaceAdvertiserService::GetInstance().DeregisterSleAdvertiserCallback();
}

NearlinkSleAdvertiserServer::NearlinkSleAdvertiserServer()
{
    HILOGI("NearlinkSleAdvertiserServer construct");
    pimpl = std::make_shared<impl>();
    pimpl->Init();
    pimpl->observerImp_->SetObserver(&(pimpl->observers_));
    pimpl->systemStateObserver_ = std::make_unique<impl::SystemStateObserver>(pimpl.get());
    SleInterfaceManager::GetInstance()->RegisterSystemStateObserver(*(pimpl->systemStateObserver_));
    InterfaceAdvertiserService::GetInstance().RegisterSleAdvertiserCallback(pimpl->observerImp_);
}

NearlinkSleAdvertiserServer::~NearlinkSleAdvertiserServer()
{
    HILOGI("~NearlinkSleAdvertiserServer");
    SleInterfaceManager::GetInstance()->DeregisterSystemStateObserver(*(pimpl->systemStateObserver_));
}

SleAdvertiserDataImpl NearlinkSleAdvertiserServer::impl::ConvertAdvertisingData(
    const NearlinkSleAdvertiserData &data) const
{
    SleAdvertiserDataImpl outData;

    std::map<uint16_t, std::string> manufacturerData = data.GetManufacturerData();
    for (auto iter = manufacturerData.begin(); iter != manufacturerData.end(); iter++) {
        outData.AddManufacturerData(iter->first, iter->second);
    }
    std::map<Uuid, std::string> serviceData = data.GetServiceData();
    for (auto it = serviceData.begin(); it != serviceData.end(); it++) {
        HILOGI("serviceData UUID is %{public}s", GET_ENCRYPT_UUID(it->first));
        outData.AddServiceData(it->first, it->second);
    }
    std::vector<Uuid> serviceUuids = data.GetServiceUuids();
    for (auto it = serviceUuids.begin(); it != serviceUuids.end(); it++) {
        outData.AddServiceUuid(*it);
    }
    outData.AddData(data.GetPayload());

    return outData;
}

void NearlinkSleAdvertiserServer::SetDeviceName(SleAdvertiserDataImpl &sleAdvertiserData)
{
    SleInterfaceAdapterSub *sleService = static_cast<SleInterfaceAdapterSub *>
        (SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    if (sleService == nullptr) {
        return;
    }
    std::string name = sleService->GetLocalName();
    if (name.empty()) {
        HILOGI("name empty");
        return;
    }
    uint32_t maxDataLen = sleService->GetSleMaxAdvertisingDataLength();
    uint32_t destLen = static_cast<uint32_t>(sleAdvertiserData.GetPayload().size() + name.size() +
        SLE_ADV_DATA_FIELD_TYPE_AND_LEN);
    if (destLen <= maxDataLen) {
        sleAdvertiserData.SetDeviceName(name);
    }
}

NlErrCode NearlinkSleAdvertiserServer::StartAdvertising(const NearlinkSleAdvertiserSettings &settings,
    const NearlinkSleAdvertiserData &advData, const NearlinkSleAdvertiserData &scanResponse, int32_t advHandle)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->IsRemoteAdv(advHandle), NL_ERR_INTERNAL_ERROR,
        "advHandle is invalid.");
    SleAdvertiserSettingsImpl settingsImpl;
    settingsImpl.SetConnectable(settings.IsConnectable());
    settingsImpl.SetInterval(settings.GetInterval());
    settingsImpl.SetLegacyMode(settings.IsLegacyMode());
    settingsImpl.SetTxPower(settings.GetTxPower());
    settingsImpl.SetOwnAddr(settings.GetOwnAddr());
    settingsImpl.SetOwnAddrType(settings.GetOwnAddrType());
    settingsImpl.SetLinkRole(settings.GetLinkRole());
    settingsImpl.SetPrimaryFrameType(settings.GetPrimaryFrameType());

    HILOGI("settingsImpl.GetPrimaryFrameType(): %{public}hhu", settingsImpl.GetPrimaryFrameType());
    
    SleAdvertiserDataImpl sleAdvertiserData = pimpl->ConvertAdvertisingData(advData);
    sleAdvertiserData.SetFlags(advData.GetAdvFlag());
    if (advData.GetIncludeDeviceName()) {
        SetDeviceName(sleAdvertiserData);
    }
    sleAdvertiserData.SetTxPowerFlag(advData.GetIncludeTxPower());
    SleAdvertiserDataImpl sleScanResponse = pimpl->ConvertAdvertisingData(scanResponse);
    InterfaceAdvertiserService::GetInstance().StartAdvertising(
        settingsImpl, sleAdvertiserData, sleScanResponse, advHandle);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleAdvertiserServer::StopAdvertising(int32_t advHandle)
{
    HILOGI("advHandle: %{public}d", advHandle);
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->IsRemoteAdv(advHandle), NL_ERR_INVALID_PARAM,
        "advHandle is invalid.");
    InterfaceAdvertiserService::GetInstance().StopAdvertising(advHandle);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleAdvertiserServer::RegisterSleAdvertiserCallback(
    const sptr<INearlinkSleAdvertiseCallback> &callback)
{
    HILOGI("enter");
    if (callback == nullptr) {
        HILOGE("callback is null");
        return NL_ERR_INVALID_PARAM;
    }
    if (pimpl->observers_.Size() >= MAX_OBSERVER_SIZE) {
        HILOGE("Register SleAdvertiser failed, the number of observers is the maximum");
        return NL_ERR_INTERNAL_ERROR;
    }
    pimpl->observers_.Register(callback);
    int32_t pid = IPCSkeleton::GetCallingPid();
    int32_t uid = IPCSkeleton::GetCallingUid();
    impl::SleAdvertiserRemoteInfo info(pid, uid);
    pimpl->remoteContainer_->AddRemoteInfo(callback->AsObject(), info);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleAdvertiserServer::DeregisterSleAdvertiserCallback(
    const sptr<INearlinkSleAdvertiseCallback> &callback)
{
    HILOGI("enter");
    if (callback == nullptr) {
        HILOGE("callback is null, or pimpl is null");
        return NL_ERR_IMPL_ERROR;
    }
    pimpl->observers_.Deregister(callback);

    pimpl->remoteContainer_->DeleteRemoteInfo(callback->AsObject());
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleAdvertiserServer::GetAdvertiserHandle(int32_t &advHandle)
{
    int32_t pid = IPCSkeleton::GetCallingPid();
    int32_t uid = IPCSkeleton::GetCallingUid();
    HILOGI("enter");
    advHandle = InterfaceAdvertiserService::GetInstance().GetAdvertiserHandle();
    pimpl->remoteContainer_->AddAdvHandle(pid, uid, advHandle);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleAdvertiserServer::SetAdvertisingData(const NearlinkSleAdvertiserData &advData,
    const NearlinkSleAdvertiserData &scanResponse, int32_t advHandle)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->IsRemoteAdv(advHandle), NL_ERR_INVALID_PARAM,
        "advHandle is invalid.");
    SleAdvertiserDataImpl sleAdvertiserData = pimpl->ConvertAdvertisingData(advData);
    SleAdvertiserDataImpl sleScanResponse = pimpl->ConvertAdvertisingData(scanResponse);
    InterfaceAdvertiserService::GetInstance().SetAdvertisingData(sleAdvertiserData, sleScanResponse, advHandle);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleAdvertiserServer::EnableAdvertising(int32_t advHandle)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->IsRemoteAdv(advHandle), NL_ERR_INVALID_PARAM,
        "advHandle is invalid.");
    InterfaceAdvertiserService::GetInstance().EnableAdvertising(advHandle);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleAdvertiserServer::DisableAdvertising(int32_t advHandle)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->IsRemoteAdv(advHandle), NL_ERR_INVALID_PARAM,
        "advHandle is invalid.");
    InterfaceAdvertiserService::GetInstance().DisableAdvertising(advHandle);
    return NL_NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS