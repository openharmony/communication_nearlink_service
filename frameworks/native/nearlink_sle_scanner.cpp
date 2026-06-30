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

#include "nearlink_sle_scanner.h"

#include "nearlink_sa_manager.h"
#include "nearlink_sle_central_manager_callback_stub.h"
#include "i_nearlink_host.h"
#include "nearlink_def.h"
#include "nearlink_errorcode.h"
#include "nearlink_host.h"
#include "log.h"
#include "log_util.h"
#include "i_nearlink_sle_central_manager.h"

namespace OHOS {
namespace Nearlink {
struct SleCentralManager::impl {
    impl(std::shared_ptr<SleCentralManagerCallback> callback);
    ~impl();
    void Init(std::weak_ptr<SleCentralManager> sleCentralManager);
    void ConvertSleScanFilter(const std::vector<SleScanFilter> &filters,
        std::vector<NearlinkSleScanFilter> &nearlinkSleScanFilters);

    class NearlinkSleCentralManagerCallbackImp;
    sptr<NearlinkSleCentralManagerCallbackImp> callbackImp_ = nullptr;
    std::weak_ptr<SleCentralManagerCallback> callback_;

    uint32_t scannerId_ = SLE_SCAN_INVALID_ID;
    std::mutex scannerIdMutex_ {};
    bool enableRandomAddrMode_ = true;
    int32_t profileRegisterId_{0};
};

class SleCentralManager::impl::NearlinkSleCentralManagerCallbackImp : public NearlinkSleCentralManagerCallBackStub {
public:
    explicit NearlinkSleCentralManagerCallbackImp(std::weak_ptr<SleCentralManager> sleCentralManger)
        : sleCentralManger_(sleCentralManger)
    {}

    ~NearlinkSleCentralManagerCallbackImp()
    {}

    void OnScanCallback(const NearlinkSleScanResult &result) override
    {
        HILOGI("addr:%{public}s", GET_ENCRYPT_ADDR(result.GetPeripheralDevice()));
        std::shared_ptr<SleCentralManager> sleCentralMangerSptr = sleCentralManger_.lock();
        NL_CHECK_RETURN(sleCentralMangerSptr, "sleCentralMangerSptr is nullptr.");

        std::shared_ptr<SleCentralManagerCallback> callbackSptr = sleCentralMangerSptr->pimpl->callback_.lock();
        NL_CHECK_RETURN(callbackSptr, "callbackSptr is nullptr.");

        NearlinkSleScanResult tempResult(result);
        SleScanResult scanResult;
        for (auto &manufacturerData : tempResult.GetManufacturerData()) {
            scanResult.AddManufacturerData(manufacturerData.first, manufacturerData.second);
        }

        for (auto &serviceUuidData : tempResult.GetServiceUuids()) {
            UUID uuid = UUID::ConvertFrom128Bits(serviceUuidData.ConvertTo128Bits());
            scanResult.AddServiceUuid(uuid);
        }

        for (auto &serviceData : tempResult.GetServiceData()) {
            UUID uuid = UUID::ConvertFrom128Bits(serviceData.first.ConvertTo128Bits());
            scanResult.AddServiceData(uuid, serviceData.second);
            HILOGI("%{public}s", uuid.GetEncryptUuid().c_str());
        }

        scanResult.SetAdvertiseFlag(tempResult.GetAdvertiseFlag());
        scanResult.SetRssi(tempResult.GetRssi());
        scanResult.SetConnectable(tempResult.IsConnectable());
        NearlinkRemoteDevice device(tempResult.GetPeripheralDevice().GetAddress(),
            static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        scanResult.SetPeripheralDevice(device);
        scanResult.SetPayload(tempResult.GetPayload());
        scanResult.SetName(tempResult.GetName());
        
        scanResult.SetDeviceClass(tempResult.GetDeviceClass());
        callbackSptr->OnScanCallback(scanResult);
    }

    void OnSleBatchScanResultsEvent(std::vector<NearlinkSleScanResult> &results) override
    {
        HILOGI("not implentment");
    }

    void OnStartOrStopScanEvent(int resultCode, bool isStartScan) override
    {
        HILOGI("resultCode: %{public}d, isStartScan: %{public}d", resultCode, isStartScan);
        std::shared_ptr<SleCentralManager> sleCentralMangerSptr = sleCentralManger_.lock();
        NL_CHECK_RETURN(sleCentralMangerSptr, "sleCentralMangerSptr is nullptr.");

        std::shared_ptr<SleCentralManagerCallback> callbackSptr = sleCentralMangerSptr->pimpl->callback_.lock();
        NL_CHECK_RETURN(callbackSptr, "callbackSptr is nullptr.");

        callbackSptr->OnStartOrStopScanEvent(resultCode, isStartScan);
    }

private:
    std::weak_ptr<SleCentralManager> sleCentralManger_;
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkSleCentralManagerCallbackImp);
};

SleCentralManager::impl::impl(std::shared_ptr<SleCentralManagerCallback> callback) : callback_(callback)
{}

SleCentralManager::impl::~impl()
{
    HILOGD("start");
    NearlinkSaManager::GetInstance().DeregisterFunc(profileRegisterId_);
    sptr<INearlinkSleCentralManager> proxy = GetProxy<INearlinkSleCentralManager>(SLE_CENTRAL_MANAGER_SERVER);
    NL_CHECK_RETURN(proxy, "proxy is nullptr.");
    proxy->DeregisterSleCentralManagerCallback(scannerId_, callbackImp_);
}

SleCentralManager::SleCentralManager(std::shared_ptr<SleCentralManagerCallback> callback) : pimpl(nullptr)
{
    if (pimpl == nullptr) {
        pimpl = std::make_unique<impl>(callback);
        NL_CHECK_RETURN(pimpl, "pimpl is nullptr");
    }
}

SleCentralManager::~SleCentralManager()
{}

void SleCentralManager::impl::Init(std::weak_ptr<SleCentralManager> sleCentralManager)
{
    callbackImp_ = new (std::nothrow) NearlinkSleCentralManagerCallbackImp(sleCentralManager);
    std::shared_ptr<NearlinkRegisterInfo> info = std::make_shared<NearlinkRegisterInfo>(SLE_CENTRAL_MANAGER_SERVER);
    info->serviceStartedFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        sptr<INearlinkSleCentralManager> proxy = iface_cast<INearlinkSleCentralManager>(remote);
        NL_CHECK_RETURN(proxy, "proxy is nullptr.");
        std::lock_guard<std::mutex> lock(scannerIdMutex_);
        NL_CHECK_RETURN(callbackImp_, "callbackImp_ is nullptr");
        proxy->RegisterSleCentralManagerCallback(scannerId_, enableRandomAddrMode_, callbackImp_);
    };
    info->serviceStoppedFunc_ = [this]() -> void {
        scannerId_ = SLE_SCAN_INVALID_ID;
    };
    profileRegisterId_ = NearlinkSaManager::GetInstance().RegisterFunc(info);
    if (profileRegisterId_ == INVALID_PROFILE_ID) {
        HILOGE("profileRegisterId_ is invalid");
    }
}

void SleCentralManager::impl::ConvertSleScanFilter(const std::vector<SleScanFilter> &filters,
    std::vector<NearlinkSleScanFilter> &nearlinkSleScanFilters)
{
    for (auto filter : filters) {
        NearlinkSleScanFilter scanFilter;
        scanFilter.SetDeviceId(filter.GetDeviceId());
        scanFilter.SetName(filter.GetName());
        if (filter.HasServiceUuid()) {
            scanFilter.SetServiceUuid(Uuid::ConvertFromString(
                filter.GetServiceUuid().ToString()));
        }
        if (filter.HasServiceUuidMask()) {
            scanFilter.SetServiceUuidMask(Uuid::ConvertFromString(
                filter.GetServiceUuidMask().ToString()));
        }
        if (filter.HasSolicitationUuid()) {
            scanFilter.SetServiceSolicitationUuid(Uuid::ConvertFromString(
                filter.GetServiceSolicitationUuid().ToString()));
        }
        if (filter.HasSolicitationUuidMask()) {
            scanFilter.SetServiceSolicitationUuidMask(Uuid::ConvertFromString(
                filter.GetServiceSolicitationUuidMask().ToString()));
        }
        if (filter.HasServiceData()) {
            scanFilter.SetServiceData(filter.GetServiceData());
        }
        if (filter.HasServiceDataMask()) {
            scanFilter.SetServiceDataMask(filter.GetServiceDataMask());
        }
        if (filter.HasRssiThreshold()) {
            scanFilter.SetRssiThreshold(filter.GetRssiThreshold());
        }
        scanFilter.SetManufacturerId(filter.GetManufacturerId());
        auto manufactureData = filter.GetManufactureData();
        if (!manufactureData.empty()) {
            scanFilter.SetManufactureData(manufactureData);
        }
        auto manufactureDataMask = filter.GetManufactureDataMask();
        if (!manufactureDataMask.empty()) {
            scanFilter.SetManufactureDataMask(manufactureDataMask);
        }
        scanFilter.SetSensorHubChannel(filter.IsSensorHubChannel());
        nearlinkSleScanFilters.push_back(scanFilter);
    }
}

std::shared_ptr<SleCentralManager> SleCentralManager::CreateSleCentralManager(
    std::shared_ptr<SleCentralManagerCallback> callback)
{
    HILOGI("start");
    std::shared_ptr<SleCentralManager> sleCentralManager = std::make_shared<SleCentralManager>(Pattern(), callback);
    NL_CHECK_RETURN_RET(sleCentralManager, nullptr, "Create sleCentralManager failed.");

    sleCentralManager->pimpl->Init(sleCentralManager);
    return sleCentralManager;
}

NlErrCode SleCentralManager::StartScanWithFilter(const SleScanSettings &settings,
    const std::vector<SleScanFilter> &filters)
{
    HILOGI("enter StartScanWithFilter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "Nearlink is off.");
    NL_CHECK_RETURN_RET(pimpl->scannerId_ != SLE_SCAN_INVALID_ID, NL_ERR_INTERNAL_ERROR, "invalid scannerId");
    NL_CHECK_RETURN_RET(settings.GetFrameType() >= static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_1) &&
                        settings.GetFrameType() <= static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_4),
                        NL_ERR_INTERNAL_ERROR, "invalid frameType");

    std::vector<NearlinkSleScanFilter> scanFilters;
    pimpl->ConvertSleScanFilter(filters, scanFilters);
    if (settings.GetFrameType() == static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_4)) {
        for (auto& filter : scanFilters) {
            filter.SetAdvIndReport(true);
        }
    }

    sptr<INearlinkSleCentralManager> proxy = GetProxy<INearlinkSleCentralManager>(SLE_CENTRAL_MANAGER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    return proxy->StartScanWithFilter(pimpl->scannerId_, NearlinkSleScanSettings(settings), scanFilters);
}

NlErrCode SleCentralManager::StartFullScan(const SleScanSettings &settings)
{
    HILOGI("enter StartFullScan");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "Nearlink is off.");
    NL_CHECK_RETURN_RET(pimpl->scannerId_ != SLE_SCAN_INVALID_ID, NL_ERR_INTERNAL_ERROR, "invalid scannerId");
    NL_CHECK_RETURN_RET(settings.GetFrameType() >= static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_1) &&
                        settings.GetFrameType() <= static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_4),
                        NL_ERR_INTERNAL_ERROR, "invalid frameType");

    sptr<INearlinkSleCentralManager> proxy = GetProxy<INearlinkSleCentralManager>(SLE_CENTRAL_MANAGER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    return proxy->StartFullScan(pimpl->scannerId_, NearlinkSleScanSettings(settings));
}

NlErrCode SleCentralManager::StartScan()
{
    HILOGI("enter StartScan");

    SleScanSettings defaultSettings;
    return StartFullScan(defaultSettings);
}

NlErrCode SleCentralManager::StopScan()
{
    HILOGI("stop scan");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "Nearlink is off.");
    NL_CHECK_RETURN_RET(pimpl->scannerId_ != SLE_SCAN_INVALID_ID, NL_ERR_INTERNAL_ERROR, "invalid scannerId");

    sptr<INearlinkSleCentralManager> proxy = GetProxy<INearlinkSleCentralManager>(SLE_CENTRAL_MANAGER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    HILOGI("scannerId_: %{public}u", pimpl->scannerId_);

    return proxy->StopScan(pimpl->scannerId_);
}

SleScanResult::SleScanResult()
{}

SleScanResult::~SleScanResult()
{}

std::vector<UUID> SleScanResult::GetServiceUuids() const
{
    return serviceUuids_;
}

std::map<uint16_t, std::string> SleScanResult::GetManufacturerData() const
{
    return manufacturerSpecificData_;
}

std::map<UUID, std::string> SleScanResult::GetServiceData() const
{
    return serviceData_;
}

NearlinkRemoteDevice SleScanResult::GetPeripheralDevice() const
{
    return peripheralDevice_;
}

int8_t SleScanResult::GetRssi() const
{
    return rssi_;
}

bool SleScanResult::IsConnectable() const
{
    return connectable_;
}

uint8_t SleScanResult::GetAdvertiseFlag() const
{
    return advertiseFlag_;
}

std::vector<uint8_t> SleScanResult::GetPayload() const
{
    return payload_;
}

void SleScanResult::AddManufacturerData(uint16_t manufacturerId, const std::string &data)
{
    manufacturerSpecificData_.insert(std::make_pair(manufacturerId, data));
}

void SleScanResult::AddServiceData(const UUID &uuid, const std::string &data)
{
    serviceData_.insert(std::make_pair(uuid, data));
}

void SleScanResult::AddServiceUuid(const UUID &serviceUuid)
{
    serviceUuids_.push_back(serviceUuid);
}

void SleScanResult::SetPayload(std::string payload)
{
    payload_.assign(payload.begin(), payload.end());
}

void SleScanResult::SetPeripheralDevice(const NearlinkRemoteDevice &device)
{
    peripheralDevice_ = device;
}

void SleScanResult::SetRssi(int8_t rssi)
{
    rssi_ = rssi;
}

void SleScanResult::SetConnectable(bool connectable)
{
    connectable_ = connectable;
}

void SleScanResult::SetAdvertiseFlag(uint8_t flag)
{
    advertiseFlag_ = flag;
}

void SleScanResult::SetDeviceClass(uint32_t deviceClass)
{
    deviceClass_ = deviceClass;
}

void SleScanResult::SetName(const std::string &name)
{
    name_ = name;
}
std::string SleScanResult::GetName(void)
{
    return name_;
}

SleScanSettings::SleScanSettings()
{}

SleScanSettings::~SleScanSettings()
{}

void SleScanSettings::SetReportDelay(long reportDelayMillis)
{
    reportDelayMillis_ = reportDelayMillis;
}

long SleScanSettings::GetReportDelayMillisValue() const
{
    return reportDelayMillis_;
}

int SleScanSettings::SetScanMode(int scanMode)
{
    if (scanMode < SCAN_MODE_LOW_POWER || scanMode >= SCAN_MODE_INVALID) {
        return static_cast<int>(ReturnValue::RET_BAD_PARAM);
    }

    scanMode_ = scanMode;
    return static_cast<int>(ReturnValue::RET_NO_ERROR);
}

int SleScanSettings::GetScanMode() const
{
    return scanMode_;
}

void SleScanSettings::SetLegacy(bool legacy)
{
    legacy_ = legacy;
}

bool SleScanSettings::GetLegacy() const
{
    return legacy_;
}

void SleScanSettings::SetDuration(int duration)
{
    duration_ = duration;
}

int SleScanSettings::GetDuration() const
{
    return duration_;
}

void SleScanSettings::SetFrameType(uint8_t frameType)
{
    frameType_ = frameType;
}

uint8_t SleScanSettings::GetFrameType() const
{
    return frameType_;
}

SleScanFilter::SleScanFilter()
{}

SleScanFilter::~SleScanFilter()
{}

void SleScanFilter::SetDeviceId(std::string deviceId)
{
    deviceId_ = deviceId;
}

std::string SleScanFilter::GetDeviceId() const
{
    return deviceId_;
}

void SleScanFilter::SetName(std::string name)
{
    name_ = name;
}

std::string SleScanFilter::GetName() const
{
    return name_;
}

void SleScanFilter::SetServiceUuid(const UUID &uuid)
{
    serviceUuid_ = uuid;
    hasServiceUuid_ = true;
}

bool SleScanFilter::HasServiceUuid()
{
    return hasServiceUuid_;
}

UUID SleScanFilter::GetServiceUuid() const
{
    return serviceUuid_;
}

void SleScanFilter::SetServiceUuidMask(const UUID &serviceUuidMask)
{
    serviceUuidMask_ = serviceUuidMask;
    hasServiceUuidMask_ = true;
}

bool SleScanFilter::HasServiceUuidMask()
{
    return hasServiceUuidMask_;
}

UUID SleScanFilter::GetServiceUuidMask() const
{
    return serviceUuidMask_;
}

void SleScanFilter::SetServiceSolicitationUuid(const UUID &serviceSolicitationUuid)
{
    serviceSolicitationUuid_ = serviceSolicitationUuid;
    hasSolicitationUuid_ = true;
}

bool SleScanFilter::HasSolicitationUuid()
{
    return hasSolicitationUuid_;
}

UUID SleScanFilter::GetServiceSolicitationUuid() const
{
    return serviceSolicitationUuid_;
}

void SleScanFilter::SetServiceSolicitationUuidMask(const UUID &serviceSolicitationUuidMask)
{
    serviceSolicitationUuidMask_ = serviceSolicitationUuidMask;
    hasSolicitationUuidMask_ = true;
}

bool SleScanFilter::HasSolicitationUuidMask()
{
    return hasSolicitationUuidMask_;
}

UUID SleScanFilter::GetServiceSolicitationUuidMask() const
{
    return serviceSolicitationUuidMask_;
}

void SleScanFilter::SetServiceData(std::vector<uint8_t> serviceData)

{
    serviceData_ = serviceData;
    hasServiceData_ = true;
}

bool SleScanFilter::HasServiceData() const
{
    return hasServiceData_;
}

std::vector<uint8_t> SleScanFilter::GetServiceData() const
{
    return serviceData_;
}

void SleScanFilter::SetServiceDataMask(std::vector<uint8_t> serviceDataMask)
{
    serviceDataMask_ = serviceDataMask;
    hasServiceDataMask_ = true;
}

bool SleScanFilter::HasServiceDataMask() const
{
    return hasServiceDataMask_;
}

std::vector<uint8_t> SleScanFilter::GetServiceDataMask() const
{
    return serviceDataMask_;
}

void SleScanFilter::SetManufacturerId(uint16_t manufacturerId)
{
    manufacturerId_ = manufacturerId;
}

uint16_t SleScanFilter::GetManufacturerId() const
{
    return manufacturerId_;
}

void SleScanFilter::SetManufactureData(std::vector<uint8_t> manufactureData)
{
    manufactureData_ = manufactureData;
}

std::vector<uint8_t> SleScanFilter::GetManufactureData() const
{
    return manufactureData_;
}

void SleScanFilter::SetManufactureDataMask(std::vector<uint8_t> manufactureDataMask)
{
    manufactureDataMask_ = manufactureDataMask;
}

std::vector<uint8_t> SleScanFilter::GetManufactureDataMask() const
{
    return manufactureDataMask_;
}

void SleScanFilter::SetSensorHubChannel(bool isSensorHubChannel)
{
    isSensorHubChannel_ = isSensorHubChannel;
}

bool SleScanFilter::IsSensorHubChannel()
{
    return isSensorHubChannel_;
}

void SleScanFilter::SetRssiThreshold(int8_t rssi)
{
    rssiThreshold_ = rssi;
    hasRssiThreshold_ = true;
}

int8_t SleScanFilter::GetRssiThreshold() const
{
    return rssiThreshold_;
}

bool SleScanFilter::HasRssiThreshold() const
{
    return hasRssiThreshold_;
}
}  // namespace Nearlink
}  // namespace OHOS