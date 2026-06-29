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

#include "ScanService.h"
#include "nearlink_permission_manager.h"
#include "nearlink_dft_exception.h"
#include "nearlink_dft_device_data.h"
#include "nlstk_api_type_ext.h"
#include "SleServiceFfrtLog.h"
#include "nearlink_safe_hashmap.h"
#include "nearlink_common_event_helper.h"
#include "SleUtils.h"
#include "queue"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr int STOP_ALL_SCAN_WAIT_TIMEOUT_MS = 2100;  // 2100ms
constexpr size_t MIN_SCNNER_IDS = 1;  //scan service自己会注册一个scnnerid
constexpr uint32_t SLE_DEVICE_APPEARANCE_PENCIL = 0x000504;
constexpr size_t MAX_SCAN_RESULT_QUEUE_SIZE = 200;
constexpr uint16_t SLE_COMPANY_ID_HUAWEI = 0x0009;
constexpr uint8_t SLE_ADV_MANUFACTURER_DATA_BUSINESS_LEN = 1;
constexpr uint8_t SLE_ADV_MANUFACTURER_DATA_EXTEND_TYPE_LEN = 1;
constexpr uint8_t SLE_ADDRESS_BYTE_LEN = 6;
constexpr uint8_t SLE_DISPLAY_BYTE_LEN = 1;
constexpr uint8_t SLE_NOT_DISPLAY = 1;
using SLE_ADV_MANUFACTURER_DATA_EXTEND_TYPE = enum {
    CDSM_REPORT_ADDR = 1,                        /*!< 合作设备集report地址 */
    CDSM_PAIR_ADDR = 2,                          /*!< 合作设备集对耳地址 */
    EARPHONE_DISPLAY_CONTROL = 3,         /*!< 是否为可发现广播 */
    MANUFACTURER_ABILITY = 4,             /*!< 能力位图 */
};
}
struct ScanService::impl {
    impl(ScanService &ScanService);
    ~impl() = default;

    ISleCentralManagerCallback *sleCentralManagerCallback_ = nullptr;
    uint32_t serviceScannerId_ = SLE_SCAN_INVALID_ID;
    ScanStackAdapter& scanStackAdapter_ = ScanStackAdapter::GetInstance();
    /// scan results
    NearlinkSafeHashMap<std::string, SleScanResultImpl> sleScanResults_{};
    /// scan results address queue
    std::deque<std::string> sleScanResultsQueue_{};
};

ScanService::impl::impl(ScanService &scanService)
{}

ScanService::ScanService(): pimpl(std::make_unique<ScanService::impl>(*this)) // 先构造impl
{
    pimpl->scanStackAdapter_.RegisterSleScanCallbackToStack(); // 同步
    pimpl->serviceScannerId_ = AllocScannerId(); // 同步

    NearlinkSleScanSettings temScanSettings;
    temScanSettings.SetScanMode(SCAN_MODE_MONITOR);
    std::vector<SleScanFilterImpl> temFilters = {};
    pimpl->scanStackAdapter_.StartScan(pimpl->serviceScannerId_, temScanSettings, temFilters);
    // 以上，确保每一条广播无视软过滤，都能上报到service
    HILOGI("Create ScanService");
}

ScanService::~ScanService()
{
    HILOGI("~ScanService");
}

InterfaceScanService &InterfaceScanService::GetInstance()
{
    return ScanService::GetInstance();
}

ScanService &ScanService::GetInstance()
{
    // C++11 static local variable initialization is thread-safe.
    static ScanService scanService;
    return scanService;
}

/******************扫描逻辑控制 开始*******************/
void ScanService::RegisterSleCentralManagerCallback(ISleCentralManagerCallback &callback)
{
    HILOGI("enter");
    DoInScanThread([this, &callback]() -> void {
        pimpl->sleCentralManagerCallback_ = &callback;
    });
}

void ScanService::DeregisterSleCentralManagerCallback() const
{
    HILOGI("enter");
    DoInScanThread([this]() -> void {
        if (!pimpl->sleCentralManagerCallback_) {
            pimpl->sleCentralManagerCallback_ = nullptr;
        }
    });
}

void ScanService::NotifyStartOrStopScanEvent(int resultCode, bool isStartScanEvt)
{
    HILOGI("isStartScan=%{public}d, resultCode=%{public}d", isStartScanEvt, resultCode);
    if (isStartScanEvt && resultCode == SCAN_SUCCESS) {
        NearlinkHelper::NearlinkCommonEventHelper::PublishScanStartedEvent(SCAN_STARTED);
    }
    if (!isStartScanEvt && resultCode == SCAN_SUCCESS) {
        NearlinkHelper::NearlinkCommonEventHelper::PublishScanFinishedEvent(SCAN_STOPED);
    }
    NL_CHECK_RETURN(pimpl->sleCentralManagerCallback_, "sleCentralManagerCallback_ is null");
    pimpl->sleCentralManagerCallback_->OnStartOrStopScanEvent(resultCode, isStartScanEvt);
}

void ScanService::NotifyScanResults(NLSTK_DevdAdvResult_S *result)
{
    std::vector<uint32_t> scannerIds = {};
    SleScanResultImpl scanResult;
    ParseAdvResult(result, scannerIds, scanResult);
    DoInScanThread([this, scannerIds, scanResult]() -> void {
        AddPeripheralDevice(scanResult);
        auto it = std::find(scannerIds.begin(), scannerIds.end(), pimpl->serviceScannerId_);
        if (scannerIds.size() == MIN_SCNNER_IDS && it != scannerIds.end()) {
            return;
        }
        NL_CHECK_RETURN(pimpl->sleCentralManagerCallback_, "sleCentralManagerCallback_ is null");
        pimpl->sleCentralManagerCallback_->OnScanCallback(scannerIds, scanResult);
    });
}

uint32_t ScanService::AllocScannerId()
{
    std::promise<uint32_t> scanIdPromise;
    std::future<uint32_t> future = scanIdPromise.get_future();
    DoInScanThread([this, &scanIdPromise]() -> void {
        uint32_t scannerId = pimpl->scanStackAdapter_.AllocScannerId();
        scanIdPromise.set_value(scannerId);
    });
    uint32_t scannerId = future.get();
    HILOGI("scannerId(%{public}d)", scannerId);
    return scannerId;
}

void ScanService::RemoveScannerId(uint32_t scannerId)
{
    HILOGI("scannerId(%{public}d)", scannerId);
    DoInScanThread([this, scannerId]() -> void {
        pimpl->scanStackAdapter_.RemoveScannerId(scannerId);
    });
}

void ScanService::StartScan(uint32_t scannerId, const NearlinkSleScanSettings &settings,
    const std::vector<SleScanFilterImpl> &filters)
{
    HILOGI("scannerId(%{public}d)", scannerId);
    DftDealAccurateSearchScanInfo(NearLinkPermissionManager::GetCallingName(), DFT_SCAN);
    DoInScanThread([this, scannerId, settings, filters]() -> void {
        pimpl->scanStackAdapter_.StartScan(scannerId, settings, filters);
    });
}

void ScanService::StopScan(uint32_t scannerId) const
{
    HILOGI("scannerId(%{public}d)", scannerId);
    DoInScanThread([this, scannerId]() -> void {
        pimpl->scanStackAdapter_.StopScan(scannerId);
    });
}

void ScanService::StopAllScan()
{
    std::shared_ptr<std::promise<void>> stopAllScanPromise = std::make_shared<std::promise<void>>();
    std::future<void> future = stopAllScanPromise->get_future();
    DoInScanThread([this, &stopAllScanPromise]() -> void {
        pimpl->scanStackAdapter_.StopAllScan(stopAllScanPromise);
    });
    HILOGI("waiting for stop all scan....");
    auto status = future.wait_for(std::chrono::milliseconds(STOP_ALL_SCAN_WAIT_TIMEOUT_MS));
    if (status != std::future_status::ready) {
        HILOGE("stop all scan timeout");
        return;
    }
    HILOGI("all scan stopped");
    future.get();
}
/******************扫描逻辑控制 结束*******************/

/******************扫描结果访问 开始*******************/
std::string ScanService::GetDeviceName(const std::string &address) const
{
    SleScanResultImpl value;
    if (pimpl->sleScanResults_.GetValue(address, value)) {
        return value.GetPeripheralDevice().GetName();
    }
    return "";
}

int ScanService::GetDeviceAppearance(const std::string &address) const
{
    SleScanResultImpl value;
    if (pimpl->sleScanResults_.GetValue(address, value)) {
        return static_cast<int>(value.GetPeripheralDevice().GetAppearance());
    }
    return INVALID_APPEARANCE;
}

std::array<uint8_t, SLE_MANU_ABILITY_LEN> ScanService::GetDeviceManufacturerAbility(const std::string &address) const
{
    SleScanResultImpl value;
    if (pimpl->sleScanResults_.GetValue(address, value)) {
        return value.GetPeripheralDevice().GetManufacturerAbility();
    }
    return {};
}

uint8_t ScanService::GetDeviceAddrType(const std::string &address) const
{
    SleScanResultImpl value;
    if (pimpl->sleScanResults_.GetValue(address, value)) {
        return value.GetPeripheralDevice().GetAddressType();
    }
    return SLE_ADDR_TYPE_END;
}

int ScanService::GetManufacturerBusinessType(const std::string &address) const
{
    SleScanResultImpl value;
    if (pimpl->sleScanResults_.GetValue(address, value)) {
        return value.GetPeripheralDevice().GetManufacturerBusiness();
    }
    return 0;
}

bool ScanService::IsAudioDevice(const std::string &address) const
{
    SleScanResultImpl value;
    if (pimpl->sleScanResults_.GetValue(address, value)) {
        return value.GetPeripheralDevice().GetIsAudioDeviceFlag();
    }
    return false;
}

std::string ScanService::GetModelId(const std::string &address) const
{
    SleScanResultImpl value;
    if (pimpl->sleScanResults_.GetValue(address, value)) {
        return value.GetPeripheralDevice().GetModelId();
    }
    return "";
}

std::string ScanService::GetNewModelId(const std::string &address) const
{
    SleScanResultImpl value;
    if (pimpl->sleScanResults_.GetValue(address, value)) {
        return value.GetPeripheralDevice().GetNewModelId();
    }
    return "";
}

std::string ScanService::GetSubModelId(const std::string &address) const
{
    SleScanResultImpl value;
    if (pimpl->sleScanResults_.GetValue(address, value)) {
        return value.GetPeripheralDevice().GetSubModelId();
    }
    return "";
}

std::string ScanService::GetIconId(const std::string &address) const
{
    SleScanResultImpl value;
    if (pimpl->sleScanResults_.GetValue(address, value)) {
        return value.GetPeripheralDevice().GetIconId();
    }
    return "";
}

std::string ScanService::GetBtAddr(const std::string &address) const
{
    SleScanResultImpl value;
    if (pimpl->sleScanResults_.GetValue(address, value)) {
        return value.GetPeripheralDevice().GetBtAddr();
    }
    return "";
}

RawAddress ScanService::GetCurrentAddress(const RawAddress &reportAddr) const
{
    SleScanResultImpl value;
    if (pimpl->sleScanResults_.GetValue(reportAddr.GetAddress(), value)) {
        return value.GetPeripheralDevice().GetCurrentRawAddress();
    }
    return reportAddr;
}

RawAddress ScanService::GetCollaborateAddress(const RawAddress &reportAddr) const
{
    SleScanResultImpl value;
    if (pimpl->sleScanResults_.GetValue(reportAddr.GetAddress(), value)) {
        return value.GetPeripheralDevice().GetCollaborateAddress();
    }
    return reportAddr;
}

RawAddress ScanService::GetReportAddrByCurrentAddress(const RawAddress &currentAddr) const
{
    RawAddress addr;
    pimpl->sleScanResults_.Find([&currentAddr, &addr](const std::string key, const SleScanResultImpl value) -> bool {
        if (value.GetPeripheralDevice().GetCurrentRawAddress() == currentAddr) {
            addr.SetAddress(key);
            return true;
        }
        return false;
    });
    return addr;
}
/******************扫描结果访问 结束*******************/

/******************扫描结果解析 开始*******************/
void ScanService::ParseAdvResult(NLSTK_DevdAdvResult_S *devdResult, std::vector<uint32_t> &scannerIds,
    SleScanResultImpl &scanResult)
{
    NL_CHECK_RETURN(devdResult, "devdResult is null");
    NL_CHECK_RETURN(devdResult->scannerIds, "scannerIds is null");
    NL_CHECK_RETURN(devdResult->serviceDataList, "serviceDataList is null");
    NL_CHECK_RETURN(devdResult->serviceUuids, "serviceUuids is null");
    NL_CHECK_RETURN(devdResult->manufacturerDataList, "manufacturerDataList is null");

    // 协议栈上报scannerIds最大为1024
    for (size_t scannerIdsIndex = 0; scannerIdsIndex < devdResult->scannerIds->size; ++scannerIdsIndex) {
        scannerIds.push_back(static_cast<uint32_t>(*(reinterpret_cast<uint32_t*>
            (SDF_VectorElementAt(devdResult->scannerIds, scannerIdsIndex)))));
    }
    SlePeripheralDevice device;
    RawAddress advertisedAddress(RawAddress::ConvertToString(devdResult->addr.addr));
    device.SetPayload(devdResult->advData.data, devdResult->advData.len);
    device.SetAddress(advertisedAddress);
    device.SetAddressType(devdResult->addr.type);
    device.SetCurrentRawAddress(advertisedAddress);
    device.SetRSSI(devdResult->rssi);
    device.SetTXPower(devdResult->txPower);
    device.SetAdFlag(devdResult->discoveryLevel);
    device.SetConnectable(devdResult->isConnectable);
    if (devdResult->localName.len != 0 && devdResult->localName.data != nullptr) {
        std::string str(reinterpret_cast<char*>(devdResult->localName.data), devdResult->localName.len);
        device.SetName(str);
    }
    if (devdResult->isPencil) {
        ParseAdvData(devdResult, device);
        device.SetAppearance(SLE_DEVICE_APPEARANCE_PENCIL);
        device.SetServiceUUID(Uuid::ConvertFrom16Bits(UUID_DEVICE_INFORMATION_SERVICE_PEN));
        device.SetServiceUUID(Uuid::ConvertFrom16Bits(SLE_STANDARD_SERVICE_HID_UUID_PEN));
        device.SetServiceUUID(Uuid::ConvertFrom16Bits(UUID_BATTERY_SERVICE_PEN));
    } else {
        ParseAdvData(devdResult, device);
        device.ParseSleServiceData(); // 如广播DIS服务填充名字信息则设备名字更新为此值。
        ParseManufacturerData(device);
    }
    /* audio私有特性，广播携带report地址，上报扫描时地址映射 */
    if (device.GetManufacturerBusiness() == SLE_PRIVATE_AUDIO_BUSINESS_TYPE &&
        !(device.GetRawAddress() == device.GetCurrentRawAddress())) {
        advertisedAddress = device.GetRawAddress();
    }
    scanResult.SetPeripheralDevice(device);
    scanResult.SetPrimFrameType(devdResult->frameType);
}

void ScanService::ParseAdvData(NLSTK_DevdAdvResult_S *result, SlePeripheralDevice &device)
{
    for (size_t serviceDataIndex = 0; serviceDataIndex < result->serviceDataList->size; ++serviceDataIndex) {
        NLSTK_DevdAdvServiceData_S *devdAdvServiceData =
            (NLSTK_DevdAdvServiceData_S *)SDF_VectorElementAt(result->serviceDataList, serviceDataIndex);
        std::array<uint8_t, Uuid::UUID128_BYTES_TYPE> serviceUuid;
        (void)memcpy_s(serviceUuid.data(), SERVICE_UUID_LEN_128, devdAdvServiceData->uuid, SERVICE_UUID_LEN_128);
        Uuid uuid = Uuid::ConvertFrom128Bits(serviceUuid);
        std::string serviceData(reinterpret_cast<char*>(devdAdvServiceData->data), devdAdvServiceData->len);
        device.SetServiceDataUUID(uuid, serviceData);
    }
    bool isAudioDevice = false;
    for (size_t serviceUuidsIndex = 0; serviceUuidsIndex < result->serviceUuids->size; ++serviceUuidsIndex) {
        NLSTK_DevdAdvServiceUuid_S *devdAdvServiceUuid =
            (NLSTK_DevdAdvServiceUuid_S *)SDF_VectorElementAt(result->serviceUuids, serviceUuidsIndex);
        std::array<uint8_t, Uuid::UUID128_BYTES_TYPE> serviceUuid;
        (void)memcpy_s(serviceUuid.data(), SERVICE_UUID_LEN_128, devdAdvServiceUuid->uuid, SERVICE_UUID_LEN_128);
        Uuid uuid = Uuid::ConvertFrom128Bits(serviceUuid);

        device.SetServiceUUID(uuid);
        if (uuid.ConvertTo16Bits() == static_cast<uint16_t>(SleUuid::SLE_STANDARD_ASC_MGMT_UUID) ||
            uuid.ConvertTo16Bits() == static_cast<uint16_t>(SleUuid::SLE_STANDARD_ASC_ABLTY_UUID)) {
            isAudioDevice |= true;
        }
    }
    if (isAudioDevice) {
        device.SetIsAudioDeviceFlag(true);
    }
    for (size_t manufacturerDataIndex = 0; manufacturerDataIndex < result->manufacturerDataList->size;
        ++manufacturerDataIndex) {
        NLSTK_DevdAdvManufacturerData_S *devdAdvManufacturerData =
            (NLSTK_DevdAdvManufacturerData_S *)SDF_VectorElementAt(result->manufacturerDataList, manufacturerDataIndex);

        uint16_t manufacturerId = devdAdvManufacturerData->manufacturerId;
        std::string manufacturerData(reinterpret_cast<char*>(devdAdvManufacturerData->data),
            devdAdvManufacturerData->len);
        device.SetManufacturerData(manufacturerId, manufacturerData);
    }
}

void ScanService::ParseManufacturerData(SlePeripheralDevice &device)
{
    std::map<uint16_t, std::string> manufacturerData = device.GetManufacturerData();
    auto it = manufacturerData.find(SLE_COMPANY_ID_HUAWEI);
    if (it == manufacturerData.end()) {
        HILOGD("parse adv manufactrue,not found company ID,dev:%{public}s.", GET_ENCRYPT_ADDR(device.GetRawAddress()));
        return;
    }
    std::string privateData = it->second;
    if (privateData.size() < SLE_ADV_MANUFACTURER_DATA_BUSINESS_LEN) {
        HILOGD("parse adv manufactrue,data size invalid:%{public}u", privateData.size());
        return;
    }
    uint8_t business = privateData[0];
    device.SetManufacturerBusiness(business);

    /* Audio 相关 */
    if (business == Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE) {
        ParseManufacturerDataAudio(device, privateData);
    }
}

void ScanService::ParseManufacturerDataAudio(SlePeripheralDevice &device, std::string &privateData)
{
    size_t msgIndex = 1;
    std::string reportAddr = "";
    std::string memberAddr = "";
    uint8_t extendType = 0;
    bool noError = true;
    while (msgIndex < privateData.size() && noError) {
        if (msgIndex + SLE_ADV_MANUFACTURER_DATA_EXTEND_TYPE_LEN > privateData.size()) {
            HILOGE("parse adv manufactrue,msg type length invalid:%{public}u", privateData.size());
            return;
        }
        extendType = privateData[msgIndex++];
        switch (extendType) {
            case CDSM_REPORT_ADDR: /* 上报地址 */
                if (msgIndex + SLE_ADDRESS_BYTE_LEN > privateData.size()) {
                    HILOGE("parse adv manufactrue,report addr len invalid:%{public}u", privateData.size());
                    return;
                }
                reportAddr = privateData.substr(msgIndex, SLE_ADDRESS_BYTE_LEN);
                msgIndex += SLE_ADDRESS_BYTE_LEN;
                break;
            case CDSM_PAIR_ADDR: /* 对耳地址 */
                if (msgIndex + SLE_ADDRESS_BYTE_LEN > privateData.size()) {
                    HILOGE("parse adv manufactrue,cdsm pair addr len invalid:%{public}u", privateData.size());
                    return;
                }
                memberAddr = privateData.substr(msgIndex, SLE_ADDRESS_BYTE_LEN);
                msgIndex += SLE_ADDRESS_BYTE_LEN;
                break;
            case EARPHONE_DISPLAY_CONTROL: /* 设置界面上是否显示星闪耳机的控制 */
                if (!ParseAdvEarphoneDisplayControl(device, msgIndex, privateData)) {
                    HILOGE("parse adv manufactrue,cdsm judge cover len invalid:%{public}u", privateData.size());
                    return;
                }
                break;
            case MANUFACTURER_ABILITY:
                if (!ParseAdvDeviceManufacturerAbility(device, msgIndex, privateData)) {
                    HILOGE("parse adv manufactrue,manufacturer ability len invalid:%{public}u", privateData.size());
                    return;
                }
                break;
            default:
                HILOGW("parse adv manufactrue,msg type not support:%{public}u", extendType);
                noError = false;
                break;
        }
    }
    ProcManufactureCollabrateAddr(device, reportAddr, memberAddr);
}

bool ScanService::ParseAdvEarphoneDisplayControl(SlePeripheralDevice &device, size_t &msgIndex,
    const std::string &privateData)
{
    if (msgIndex + SLE_DISPLAY_BYTE_LEN > privateData.size()) {
        return false;
    }
    device.SetIsDeviceDisplay(privateData[msgIndex] != SLE_NOT_DISPLAY);
    msgIndex += SLE_DISPLAY_BYTE_LEN;
    return true;
}

bool ScanService::ParseAdvDeviceManufacturerAbility(SlePeripheralDevice &device, size_t &msgIndex,
    const std::string &privateData)
{
    std::string manuAbilityStr = "";
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> manuAbility = {0};
    if (msgIndex + SLE_MANU_ABILITY_LEN > privateData.size()) {
        return false;
    }
    manuAbilityStr = privateData.substr(msgIndex, SLE_MANU_ABILITY_LEN);
    for (uint8_t i = 0; i < SLE_MANU_ABILITY_LEN; ++i) {
        manuAbility[i] = static_cast<uint8_t>(manuAbilityStr[i]);
    }
    device.SetManufacturerAbility(manuAbility);
    msgIndex += SLE_MANU_ABILITY_LEN;
    return true;
}

void ScanService::ProcManufactureCollabrateAddr(SlePeripheralDevice &device,
    std::string &reportAddr, std::string &collabAddr)
{
    if (reportAddr.empty() || collabAddr.empty()) {
        HILOGE("[scan adapter]:parse adv manufacture data fail, address invalid.");
        return;
    }

    RawAddress report = RawAddress::ConvertToString(reinterpret_cast<uint8_t*>(reportAddr.data()));
    RawAddress collab = RawAddress::ConvertToString(reinterpret_cast<uint8_t*>(collabAddr.data()));

    HILOGD("[scan adapter]:address mapping,%{public}s map to %{public}s.",
        GET_ENCRYPT_ADDR(device.GetRawAddress()), GET_ENCRYPT_ADDR(report));

    device.SetAddress(report);
    device.SetCollaborateAddress(collab);
}
/******************扫描结果解析 结束*******************/

/******************扫描结果增删 开始*******************/
void ScanService::ClearScanResultInfo() const
{
    HILOGI("enter");
    DoInScanThread([this]() -> void {
        pimpl->sleScanResults_.Clear();
        while (!pimpl->sleScanResultsQueue_.empty()) {
            pimpl->sleScanResultsQueue_.pop_front();
        }
    });
}

void ScanService::AddPeripheralDevice(const SleScanResultImpl &scanResult)
{
    SlePeripheralDevice device = scanResult.GetPeripheralDevice();
    RawAddress addr = device.GetRawAddress();
    ClearOldPeripheralDevice(addr, device);
    if (pimpl->sleScanResults_.FindIf(addr.GetAddress())) {
        pimpl->sleScanResults_.EnsureInsert(addr.GetAddress(), scanResult);
        return;
    }
    pimpl->sleScanResults_.EnsureInsert(addr.GetAddress(), scanResult);
    HILOGI("addr:%{public}s, currentAddr:%{public}s, audioFlag:%{public}d, bussinessType:%{public}d",
        GET_ENCRYPT_ADDR(addr), GET_ENCRYPT_ADDR(device.GetCurrentRawAddress()),
        device.GetIsAudioDeviceFlag(), device.GetManufacturerBusiness());
    UpdateScanResultQueue(addr.GetAddress());
    DftCachePeerInfo(addr.GetAddress(), device.GetName(), device.GetAppearance());
    DftDeviceManager::GetInstance().AddDevice(addr, device.GetAppearance(), device.GetName());
}

void ScanService::UpdateScanResultQueue(const std::string &address)
{
    pimpl->sleScanResultsQueue_.push_back(address);
    while (pimpl->sleScanResultsQueue_.size() > MAX_SCAN_RESULT_QUEUE_SIZE) {
        std::string deleteAddress = pimpl->sleScanResultsQueue_.front();
        pimpl->sleScanResultsQueue_.pop_front();
        RawAddress addr(deleteAddress);
        pimpl->sleScanResults_.Erase(deleteAddress);
    }
}

void ScanService::ClearOldPeripheralDevice(const RawAddress &advertisedAddress,
    const SlePeripheralDevice &device)
{
    if (device.GetManufacturerBusiness() != Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE) {
        return;
    }
    RawAddress currentRawAddr = device.GetCurrentRawAddress();  // 主耳
    std::string clearedAdvAddress = "";
    auto clearFunc = [&currentRawAddr, &advertisedAddress, &clearedAdvAddress]
        (const std::string key, const SleScanResultImpl value) -> bool {
        if (key != advertisedAddress.GetAddress() &&
            value.GetPeripheralDevice().GetCurrentRawAddress() == currentRawAddr) {
            clearedAdvAddress = key;
            return true;
        }
        return false;
    };
    pimpl->sleScanResults_.FindAndRmv(clearFunc);
    if (!clearedAdvAddress.empty()) {
        auto it = std::find(pimpl->sleScanResultsQueue_.begin(), pimpl->sleScanResultsQueue_.end(), clearedAdvAddress);
        if (it != pimpl->sleScanResultsQueue_.end()) {
            pimpl->sleScanResultsQueue_.erase(it);
        }
        HILOGI("Erase old adv addres %{public}s from cache, current addr %{public}s match new adv address %{public}s",
            GetEncryptAddr(clearedAdvAddress).c_str(), GET_ENCRYPT_ADDR(currentRawAddr),
            GET_ENCRYPT_ADDR(advertisedAddress));
    }
}
/******************扫描结果增删 结束*******************/
}  // namespace Nearlink
}  // namespace OHOS