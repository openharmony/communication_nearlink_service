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
#include "nearlink_sle_advertiser.h"

#include "nearlink_sa_manager.h"
#include "nearlink_sle_advertise_callback_stub.h"
#include "nearlink_def.h"
#include "nearlink_errorcode.h"
#include "nearlink_host.h"
#include "log_util.h"
#include "i_nearlink_sle_advertiser.h"
#include "iservice_registry.h"
#include "nearlink_safe_map.h"
#include "system_ability_definition.h"

#include <memory>

namespace OHOS {
namespace Nearlink {
const uint32_t SLE_ADV_PER_FIELD_OVERHEAD_LENGTH = 2;
const uint32_t SLE_ADV_MANUFACTURER_ID_LENGTH = 2;
const uint32_t SLE_ADV_FLAGS_FIELD_LENGTH = 3;

struct SleAdvertiser::impl {
    impl();
    ~impl();
    void Init(std::weak_ptr<SleAdvertiser> advertiser);
    void ConvertSleAdvertiserData(const SleAdvertiserData &data, NearlinkSleAdvertiserData &outData);
    uint32_t GetAdvertiserTotalBytes(const NearlinkSleAdvertiserData &data, bool isFlagsIncluded);
    NlErrCode CheckAdvertiserData(const NearlinkSleAdvertiserSettings &setting,
    const NearlinkSleAdvertiserData &advData, const NearlinkSleAdvertiserData &scanResponse);
    int32_t GetAdvHandleImp(std::shared_ptr<SleAdvertiseCallback> callback);
    std::shared_ptr<SleAdvertiseCallback> GetAdvObserverImp(int32_t handle);

    class NearlinkSleAdvertiserCallbackImp;
    sptr<NearlinkSleAdvertiserCallbackImp> callbackImp_ = nullptr;
    NearlinkSafeMap<int32_t, std::weak_ptr<SleAdvertiseCallback>> callbacks_;
    int32_t profileRegisterId_{0};
};

class SleAdvertiser::impl::NearlinkSleAdvertiserCallbackImp : public NearlinkSleAdvertiseCallbackStub {
public:
    explicit NearlinkSleAdvertiserCallbackImp(std::weak_ptr<SleAdvertiser> advertiser): sleAdvertiser_(advertiser)
    {}

    ~NearlinkSleAdvertiserCallbackImp()
    {}

    void OnStartResultEvent(int32_t result, int32_t advHandle, int32_t opcode) override
    {
        HILOGD("result(%{public}d), advHandle(%{public}d), opcode(%{public}d)", result, advHandle, opcode);
        std::shared_ptr<SleAdvertiser> sleAdvertiserSptr = sleAdvertiser_.lock();
        NL_CHECK_RETURN(sleAdvertiserSptr, "sleAdvertiserSptr is nullptr.");

        std::shared_ptr<SleAdvertiseCallback> callbackSptr = GetAdvCallback(advHandle, sleAdvertiserSptr);
        if (callbackSptr) {
            callbackSptr->OnStartResultEvent(result, advHandle);
        }
        if (result == ADV_RESULT_FAILED_CHECK_PARA_FAIL) {
            HILOGE("start adv failed, result(%{public}d), advHandle(%{public}d)", result, advHandle);
            sleAdvertiserSptr->pimpl->callbacks_.Erase(advHandle);
        }
    }

    void OnStopResultEvent(int32_t result, int32_t advHandle) override
    {
        HILOGI("result(%{public}d), advHandle(%{public}d)", result, advHandle);
        std::shared_ptr<SleAdvertiser> sleAdvertiserSptr = sleAdvertiser_.lock();
        NL_CHECK_RETURN(sleAdvertiserSptr, "sleAdvertiserSptr is nullptr.");

        std::shared_ptr<SleAdvertiseCallback> callbackSptr = GetAdvCallback(advHandle, sleAdvertiserSptr);
        if (callbackSptr) {
            callbackSptr->OnStopResultEvent(result, advHandle);
        }
        sleAdvertiserSptr->pimpl->callbacks_.Erase(advHandle);
    }

    void OnEnableResultEvent(int32_t result, int32_t advHandle) override
    {
        HILOGI("result(%{public}d), advHandle(%{public}d)", result, advHandle);
        std::shared_ptr<SleAdvertiser> sleAdvertiserSptr = sleAdvertiser_.lock();
        NL_CHECK_RETURN(sleAdvertiserSptr, "sleAdvertiserSptr is nullptr.");

        std::shared_ptr<SleAdvertiseCallback> callbackSptr = GetAdvCallback(advHandle, sleAdvertiserSptr);
        if (callbackSptr) {
            callbackSptr->OnEnableResultEvent(result, advHandle);
        }
    }

    void OnDisableResultEvent(int32_t result, int32_t advHandle) override
    {
        HILOGI("result(%{public}d), advHandle(%{public}d)", result, advHandle);
        std::shared_ptr<SleAdvertiser> sleAdvertiserSptr = sleAdvertiser_.lock();
        NL_CHECK_RETURN(sleAdvertiserSptr, "sleAdvertiserSptr is nullptr.");

        std::shared_ptr<SleAdvertiseCallback> callbackSptr = GetAdvCallback(advHandle, sleAdvertiserSptr);
        if (callbackSptr) {
            callbackSptr->OnDisableResultEvent(result, advHandle);
        }
    }

    void OnAutoStopAdvEvent(int32_t advHandle) override
    {
        HILOGI("advHandle(%{public}d)", advHandle);
        std::shared_ptr<SleAdvertiser> sleAdvertiserSptr = sleAdvertiser_.lock();
        NL_CHECK_RETURN(sleAdvertiserSptr, "sleAdvertiserSptr is nullptr.");

        sleAdvertiserSptr->pimpl->callbacks_.Erase(advHandle);
    }

    void OnSetAdvDataEvent(int32_t result, int32_t advHandle) override
    {
        HILOGI("result(%{public}d), advHandle(%{public}d)", result, advHandle);
        std::shared_ptr<SleAdvertiser> sleAdvertiserSptr = sleAdvertiser_.lock();
        NL_CHECK_RETURN(sleAdvertiserSptr, "sleAdvertiserSptr is nullptr.");

        std::shared_ptr<SleAdvertiseCallback> callbackSptr = GetAdvCallback(advHandle, sleAdvertiserSptr);
        if (callbackSptr) {
            callbackSptr->OnSetAdvDataEvent(result);
        }
    }

private:
    std::weak_ptr<SleAdvertiser> sleAdvertiser_;
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkSleAdvertiserCallbackImp);
    std::shared_ptr<SleAdvertiseCallback> GetAdvCallback(int32_t advHandle, std::shared_ptr<SleAdvertiser> advertiser)
    {
        std::weak_ptr<SleAdvertiseCallback> callbackWptr;
        bool ret = advertiser->pimpl->callbacks_.GetValue(advHandle, callbackWptr);
        NL_CHECK_RETURN_RET(ret, nullptr, "advHandle(%{public}d) not exist.", advHandle);
        return callbackWptr.lock();
    }
};

SleAdvertiser::impl::impl()
{}

SleAdvertiser::impl::~impl()
{
    HILOGD("start");
    NearlinkSaManager::GetInstance().DeregisterFunc(profileRegisterId_);
    callbacks_.Clear();
    sptr<INearlinkSleAdvertiser> proxy = GetProxy<INearlinkSleAdvertiser>(SLE_ADVERTISER_SERVER);
    NL_CHECK_RETURN(proxy != nullptr, "failed: no proxy");
    proxy->DeregisterSleAdvertiserCallback(callbackImp_);
}

void SleAdvertiser::impl::Init(std::weak_ptr<SleAdvertiser> advertiser)
{
    callbackImp_ = new (std::nothrow) NearlinkSleAdvertiserCallbackImp(advertiser);

    std::shared_ptr<NearlinkRegisterInfo> info = std::make_shared<NearlinkRegisterInfo>(SLE_ADVERTISER_SERVER);
    info->serviceStartedFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        sptr<INearlinkSleAdvertiser> proxy = iface_cast<INearlinkSleAdvertiser>(remote);
        NL_CHECK_RETURN(proxy, "proxy is nullptr.");
        NL_CHECK_RETURN(callbackImp_, "callbackImp_ is nullptr");
        proxy->RegisterSleAdvertiserCallback(callbackImp_);
    };
    info->serviceStoppedFunc_ = [this]() -> void {
        callbacks_.Clear();
    };
    profileRegisterId_ = NearlinkSaManager::GetInstance().RegisterFunc(info);
    if (profileRegisterId_ == INVALID_PROFILE_ID) {
        HILOGE("profileRegisterId_ is invalid");
    }
}

SleAdvertiser::SleAdvertiser()
{
    if (pimpl == nullptr) {
        pimpl = std::make_unique<impl>();
        if (!pimpl) {
            HILOGE("failed, no pimpl");
        }
    }

    HILOGI("successful");
}

SleAdvertiser::~SleAdvertiser()
{}

std::shared_ptr<SleAdvertiser> SleAdvertiser::CreateSleAdvertiser(void)
{
    std::shared_ptr<SleAdvertiser> sleAdvertiser = std::make_shared<SleAdvertiser>(Pattern());
    NL_CHECK_RETURN_RET(sleAdvertiser, nullptr, "Create SleAdvertiser failed.");
    sleAdvertiser->pimpl->Init(sleAdvertiser);
    return sleAdvertiser;
}

void SleAdvertiser::impl::ConvertSleAdvertiserData(const SleAdvertiserData &data, NearlinkSleAdvertiserData &outData)
{
    std::map<uint16_t, std::string> manufacturerData = data.GetManufacturerData();
    for (auto iter = manufacturerData.begin(); iter != manufacturerData.end(); iter++) {
        outData.AddManufacturerData(iter->first, iter->second);
    }
    std::map<UUID, std::string> serviceData = data.GetServiceData();
    for (auto it = serviceData.begin(); it != serviceData.end(); it++) {
        HILOGI("serviceData UUID is %{public}s", GET_ENCRYPT_UUID(it->first));
        outData.AddServiceData(Uuid::ConvertFromString(it->first.ToString()), it->second);
    }
    std::vector<UUID> serviceUuids = data.GetServiceUuids();
    for (auto it = serviceUuids.begin(); it != serviceUuids.end(); it++) {
        outData.AddServiceUuid(Uuid::ConvertFromString(it->ToString()));
    }
    outData.SetIncludeDeviceName(data.GetIncludeDeviceName());
    outData.SetIncludeTxPower(data.GetIncludeTxPower());
}

uint32_t SleAdvertiser::impl::GetAdvertiserTotalBytes(const NearlinkSleAdvertiserData &data, bool isFlagsIncluded)
{
    // If the flag field is contained, the protocol stack adds the flag field, which consists of three bytes.
    uint32_t size = (isFlagsIncluded) ? SLE_ADV_FLAGS_FIELD_LENGTH : 0;
    HILOGD("GetAdvertiserTotalBytes Flag size=%{public}d", size);

    std::map<uint16_t, std::string> manufacturerData = data.GetManufacturerData();
    for (auto iter = manufacturerData.begin(); iter != manufacturerData.end(); ++iter) {
        size += SLE_ADV_PER_FIELD_OVERHEAD_LENGTH + SLE_ADV_MANUFACTURER_ID_LENGTH + iter->second.length();
    }
    HILOGD("GetAdvertiserTotalBytes manufacturerData size=%{public}d", size);

    std::map<Uuid, std::string> serviceData = data.GetServiceData();
    for (auto iter = serviceData.begin(); iter != serviceData.end(); ++iter) {
        HILOGI("serviceData Uuid is %{public}s", iter->first.GetEncryptUuid().c_str());
        size += SLE_ADV_PER_FIELD_OVERHEAD_LENGTH + static_cast<uint32_t>(iter->first.GetUuidType())
        + iter->second.length();
    }
    HILOGD("GetAdvertiserTotalBytes serviceData size=%{public}d", size);

    std::vector<Uuid> serviceUuids = data.GetServiceUuids();
    for (auto iter = serviceUuids.begin(); iter != serviceUuids.end(); ++iter) {
        size += SLE_ADV_PER_FIELD_OVERHEAD_LENGTH + static_cast<uint32_t>(iter->GetUuidType());
    }
    HILOGD("GetAdvertiserTotalBytes serviceUuids size=%{public}d", size);

    if (data.GetIncludeDeviceName()) {
        std::string localName = "";
        NearlinkHost::GetInstance().GetLocalName(localName);
        uint32_t deviceNameLen = localName.length();
        deviceNameLen = (deviceNameLen > DEVICE_NAME_MAX_LEN) ?  DEVICE_NAME_MAX_LEN : deviceNameLen;
        size += SLE_ADV_PER_FIELD_OVERHEAD_LENGTH + deviceNameLen;
    }
    HILOGD("GetAdvertiserTotalBytes DeviceName size=%{public}d", size);
    if (data.GetIncludeTxPower()) {
        size += SLE_ADV_FLAGS_FIELD_LENGTH;
    }
    HILOGD("GetAdvertiserTotalBytes TxPower size=%{public}d", size);
    return size;
}

NlErrCode SleAdvertiser::impl::CheckAdvertiserData(const NearlinkSleAdvertiserSettings &setting,
    const NearlinkSleAdvertiserData &advData, const NearlinkSleAdvertiserData &scanResponse)
{
    uint32_t maxSize = SLE_ADV_MAX_LEGACY_ADVERTISING_DATA_BYTES;
    if (!setting.IsLegacyMode()) {
        NearlinkHost::GetInstance().GetSleMaxAdvertisingDataLength(maxSize);
    }
    uint32_t size = GetAdvertiserTotalBytes(advData, true);
    if (size > maxSize) {
        HILOGE("sleAdvertiserData size = %{public}d, maxSize = %{public}d", size, maxSize);
        return NL_ERR_INTERNAL_ERROR;
    }
    size = GetAdvertiserTotalBytes(scanResponse, false);
    if (size > maxSize) {
        HILOGE("sleScanResponse size = %{public}d, maxSize = %{public}d,", size, maxSize);
        return NL_ERR_INTERNAL_ERROR;
    }
    if (size > 0 && setting.GetPrimaryFrameType() ==
        static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_4)) {
        HILOGE("frame 4 not support config scanResponse, sleScanResponse size = %{public}d", size);
        return NL_ERR_INVALID_PARAM;
    }
    return NL_NO_ERROR;
}

int32_t SleAdvertiser::impl::GetAdvHandleImp(std::shared_ptr<SleAdvertiseCallback> callback)
{
    int32_t advHandle = static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE);
    callbacks_.Find([&advHandle, &callback](int32_t handle, std::weak_ptr<SleAdvertiseCallback> &cbWptr) -> bool {
        std::shared_ptr<SleAdvertiseCallback> cbSptr = cbWptr.lock();
        if (callback == cbSptr) {
            advHandle = handle;
            return true;
        }
        return false;
    });
    return advHandle;
}

NlErrCode SleAdvertiser::StartAdvertising(const SleAdvertiserSettings &settings, const SleAdvertiserData &advData,
    const SleAdvertiserData &scanResponse, uint16_t duration, std::shared_ptr<SleAdvertiseCallback> callback)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(callback, NL_ERR_INVALID_PARAM, "callback is nullptr");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    sptr<INearlinkSleAdvertiser> proxy = GetProxy<INearlinkSleAdvertiser>(SLE_ADVERTISER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    NearlinkSleAdvertiserSettings setting;
    setting.SetConnectable(settings.IsConnectable());
    setting.SetInterval(settings.GetInterval());
    setting.SetLegacyMode(settings.IsLegacyMode());
    setting.SetTxPower(settings.GetTxPower());
    setting.SetOwnAddr(settings.GetOwnAddr());
    setting.SetOwnAddrType(settings.GetOwnAddrType());
    setting.SetLinkRole(settings.GetLinkRole());
    setting.SetPrimaryFrameType(settings.GetPrimaryFrameType());

    NearlinkSleAdvertiserData sleAdvertiserData;
    NearlinkSleAdvertiserData sleScanResponse;
    sleAdvertiserData.SetAdvFlag(advData.GetAdvFlag());
    pimpl->ConvertSleAdvertiserData(advData, sleAdvertiserData);
    pimpl->ConvertSleAdvertiserData(scanResponse, sleScanResponse);

    NlErrCode ret = pimpl->CheckAdvertiserData(setting, sleAdvertiserData, sleScanResponse);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "CheckAdvertiserData failed.");

    int32_t advHandle = pimpl->GetAdvHandleImp(callback);
    if (advHandle != static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE)) {
        HILOGI("callback is exist.");
        callback->OnGetAdvHandleEvent(0, advHandle);
        return proxy->StartAdvertising(setting, sleAdvertiserData, sleScanResponse, advHandle);
    }

    std::map<UUID, std::string> serviceData1 = advData.GetServiceData();
    HILOGI("serviceData size of advData is %{public}lu", serviceData1.size());
    std::map<Uuid, std::string> serviceData2 = sleAdvertiserData.GetServiceData();
    HILOGI("serviceData size of sleAdvertiserData is %{public}lu", serviceData2.size());

    std::map<uint16_t, std::string> manufacturerData1 = scanResponse.GetManufacturerData();
    HILOGI("manufacturerData size of scanResponse is %{public}lu", manufacturerData1.size());
    std::map<uint16_t, std::string> manufacturerData2 = sleScanResponse.GetManufacturerData();
    HILOGI("manufacturerData size of sleScanResponse is %{public}lu", manufacturerData2.size());
    ret = proxy->GetAdvertiserHandle(advHandle);
    if (ret != NL_NO_ERROR || advHandle == static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE)) {
        HILOGE("Invalid advertising handle");
        callback->OnStartResultEvent(NL_ERR_INTERNAL_ERROR,
                                     static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE));
        return ret;
    }
    callback->OnGetAdvHandleEvent(0, advHandle);
    pimpl->callbacks_.EnsureInsert(advHandle, callback);
    return proxy->StartAdvertising(setting, sleAdvertiserData, sleScanResponse, advHandle);
}

NlErrCode SleAdvertiser::SetAdvertisingData(const SleAdvertiserData &advData, const SleAdvertiserData &scanResponse,
    std::shared_ptr<SleAdvertiseCallback> callback)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(callback, NL_ERR_INTERNAL_ERROR, "callback is nullptr");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    sptr<INearlinkSleAdvertiser> proxy = GetProxy<INearlinkSleAdvertiser>(SLE_ADVERTISER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    int32_t advHandle = pimpl->GetAdvHandleImp(callback);
    NL_CHECK_RETURN_RET(advHandle != static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
        NL_ERR_INTERNAL_ERROR, "Advertising is not started.");

    NearlinkSleAdvertiserData sleAdvertiserData;
    sleAdvertiserData.SetAdvFlag(advData.GetAdvFlag());
    pimpl->ConvertSleAdvertiserData(advData, sleAdvertiserData);
    NearlinkSleAdvertiserData sleScanResponse;
    pimpl->ConvertSleAdvertiserData(scanResponse, sleScanResponse);

    return proxy->SetAdvertisingData(sleAdvertiserData, sleScanResponse, advHandle);
}

NlErrCode SleAdvertiser::StopAdvertising(std::shared_ptr<SleAdvertiseCallback> callback)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(callback, NL_ERR_INVALID_PARAM, "callback is nullptr");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    sptr<INearlinkSleAdvertiser> proxy = GetProxy<INearlinkSleAdvertiser>(SLE_ADVERTISER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    int32_t advHandle = pimpl->GetAdvHandleImp(callback);
    NL_CHECK_RETURN_RET(advHandle != static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
                        NL_ERR_INTERNAL_ERROR, "Invalid advertising handle");

    return proxy->StopAdvertising(advHandle);
}

NlErrCode SleAdvertiser::StopAdvertising(uint8_t advHandle)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    sptr<INearlinkSleAdvertiser> proxy = GetProxy<INearlinkSleAdvertiser>(SLE_ADVERTISER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    return proxy->StopAdvertising(advHandle);
}

NlErrCode SleAdvertiser::GetAdvHandle(std::shared_ptr<SleAdvertiseCallback> callback, uint8_t &advHandle)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    advHandle = pimpl->GetAdvHandleImp(callback);
    NL_CHECK_RETURN_RET(advHandle != static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
                        NL_ERR_INTERNAL_ERROR, "Invalid advertising handle");
    return NL_NO_ERROR;
}

NlErrCode SleAdvertiser::EnableAdvertising(uint8_t advHandle)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    sptr<INearlinkSleAdvertiser> proxy = GetProxy<INearlinkSleAdvertiser>(SLE_ADVERTISER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    return proxy->EnableAdvertising(advHandle);
}

NlErrCode SleAdvertiser::DisableAdvertising(uint8_t advHandle)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");

    sptr<INearlinkSleAdvertiser> proxy = GetProxy<INearlinkSleAdvertiser>(SLE_ADVERTISER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    return proxy->DisableAdvertising(advHandle);
}

SleAdvertiserData::SleAdvertiserData()
{}

SleAdvertiserData::~SleAdvertiserData()
{}

void SleAdvertiserData::AddServiceData(const UUID &uuid, const std::string &serviceData)
{
    if (serviceData.empty()) {
        HILOGE("serviceData is empty");
        return;
    }

    serviceData_.insert(std::make_pair(uuid, serviceData));
}

void SleAdvertiserData::AddManufacturerData(uint16_t manufacturerId, const std::string &data)
{
    if (data.empty()) {
        HILOGE("serviceData is empty");
        return;
    }

    manufacturerSpecificData_.insert(std::make_pair(manufacturerId, data));
}

std::map<uint16_t, std::string> SleAdvertiserData::GetManufacturerData() const
{
    return manufacturerSpecificData_;
}

void SleAdvertiserData::AddServiceUuid(const UUID &serviceUuid)
{
    serviceUuids_.push_back(serviceUuid);
}

std::vector<UUID> SleAdvertiserData::GetServiceUuids() const
{
    return serviceUuids_;
}

void SleAdvertiserData::SetAdvFlag(uint8_t flag)
{
    advFlag_ = flag;
}

uint8_t SleAdvertiserData::GetAdvFlag() const
{
    return advFlag_;
}

std::map<UUID, std::string> SleAdvertiserData::GetServiceData() const
{
    return serviceData_;
}

bool SleAdvertiserData::GetIncludeDeviceName() const
{
    return includeDeviceName_;
}

void SleAdvertiserData::SetIncludeDeviceName(bool flag)
{
    includeDeviceName_ = flag;
}

bool SleAdvertiserData::GetIncludeTxPower() const
{
    return includeTxPower_;
}

void SleAdvertiserData::SetIncludeTxPower(bool flag)
{
    includeTxPower_ = flag;
}

SleAdvertiserSettings::SleAdvertiserSettings()
{}

SleAdvertiserSettings::~SleAdvertiserSettings()
{}

void SleAdvertiserSettings::SetConnectable(bool connectable)
{
    connectable_ = connectable;
}

bool SleAdvertiserSettings::IsConnectable() const
{
    return connectable_;
}

void SleAdvertiserSettings::SetLegacyMode(bool legacyMode)
{
    legacyMode_ = legacyMode;
}

bool SleAdvertiserSettings::IsLegacyMode() const
{
    return legacyMode_;
}

void SleAdvertiserSettings::SetInterval(uint32_t interval)
{
    interval_ = interval;
}

uint32_t SleAdvertiserSettings::GetInterval() const
{
    return interval_;
}

void SleAdvertiserSettings::SetTxPower(uint8_t txPower)
{
    txPower_ = txPower;
}

uint8_t SleAdvertiserSettings::GetTxPower() const
{
    return txPower_;
}

uint8_t SleAdvertiserSettings::GetPrimaryFrameType() const
{
    return primaryFrameType_;
}

void SleAdvertiserSettings::SetPrimaryFrameType(uint8_t primaryFrameType)
{
    primaryFrameType_ = primaryFrameType;
}

int SleAdvertiserSettings::GetPrimaryPhy() const
{
    return primaryPhy_;
}

void SleAdvertiserSettings::SetPrimaryPhy(int primaryPhy)
{
    primaryPhy_ = primaryPhy;
}

int SleAdvertiserSettings::GetSecondaryPhy() const
{
    return secondaryPhy_;
}

void SleAdvertiserSettings::SetSecondaryPhy(int secondaryPhy)
{
    secondaryPhy_ = secondaryPhy;
}

std::array<uint8_t, NEARLINK_SLE_ADDR_LEN> SleAdvertiserSettings::GetOwnAddr() const
{
    return ownAddr_;
}

void SleAdvertiserSettings::SetOwnAddr(const std::array<uint8_t, NEARLINK_SLE_ADDR_LEN>& addr)
{
    ownAddr_ = addr;
}

int8_t SleAdvertiserSettings::GetOwnAddrType() const
{
    return ownAddrType_;
}

void SleAdvertiserSettings::SetOwnAddrType(int8_t addrType)
{
    ownAddrType_ = addrType;
}

uint8_t SleAdvertiserSettings::GetLinkRole() const
{
    return linkRole_;
}

void SleAdvertiserSettings::SetLinkRole(uint8_t role)
{
    linkRole_ = role;
}

}  // namespace Nearlink
}  // namespace OHOS