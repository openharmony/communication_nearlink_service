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

/**
 * @addtogroup Nearlink
 * @{
 *
 * @brief Defines scanner, including scan filter and settings, and common functions.
 *
 * @since 6
 */

/**
 * @file nearlink_sle_scanner.h
 *
 * @brief Central manager common functions.
 *
 * @since 6
 */

#ifndef NEARLINK_SLE_SCANNER_H
#define NEARLINK_SLE_SCANNER_H

#include "nearlink_def.h"
#include "nearlink_types.h"
#include "nearlink_remote_device.h"
#include "nearlink_sle_advertiser.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief Represents scan result.
 *
 * @since 6
 */
class NEARLINK_API SleScanResult {
public:
    /**
     * @brief A constructor used to create a <b>SleScanResult</b> instance.
     *
     * @since 6
     */
    SleScanResult();

    /**
     * @brief A destructor used to delete the <b>SleScanResult</b> instance.
     *
     * @since 6
     */
    ~SleScanResult();

    /**
     * @brief Get service uuids.
     *
     * @return Returns service uuids, uuid is big endian order.
     * @since 6
     */
    std::vector<UUID> GetServiceUuids() const;

    /**
     * @brief Get manufacture data.
     *
     * @return Returns manufacture data.
     * @since 6
     */
    std::map<uint16_t, std::string> GetManufacturerData() const;

    /**
     * @brief Get service data.
     *
     * @return Returns service data, uuid is big endian order.
     * @since 6
     */
    std::map<UUID, std::string> GetServiceData() const;

    /**
     * @brief Get peripheral device.
     *
     * @return Returns peripheral device pointer.
     * @since 6
     */
    NearlinkRemoteDevice GetPeripheralDevice() const;

    /**
     * @brief Get peer device rssi.
     *
     * @return Returns peer device rssi.
     * @since 6
     */
    int8_t GetRssi() const;

    /**
     * @brief Check if device is connctable.
     *
     * @return Returns <b>true</b> if device is connctable;
     *         returns <b>false</b> if device is not connctable.
     * @since 6
     */
    bool IsConnectable() const;

    /**
     * @brief Get advertiser flag.
     *
     * @return Returns advertiser flag.
     * @since 6
     */
    uint8_t GetAdvertiseFlag() const;

    /**
     * @brief Get payload.
     *
     * @return Returns payload.
     * @since 6
     */
    std::vector<uint8_t> GetPayload() const;
    /**
     * @brief Add manufacture data.
     *
     * @param manufacturerId Manufacture Id which addad data.
     * @param data Manufacture data
     * @since 6
     */

    void AddManufacturerData(uint16_t manufacturerId, const std::string &data);

    /**
     * @brief Add service data.
     *
     * @param uuid Uuid of service data, uuid is big endian order.
     * @param serviceData Service data.
     * @since 6
     */
    void AddServiceData(const UUID &uuid, const std::string &serviceData);

    /**
     * @brief Add service uuid.
     *
     * @param serviceUuid Service uuid, uuid is big endian order.
     * @since 6
     */
    void AddServiceUuid(const UUID &serviceUuid);

    /**
     * @brief Add data to payload.
     *
     * @param payload Added payload.
     * @since 6
     */
    void SetPayload(std::string payload);

    /**
     * @brief Set peripheral device.
     *
     * @param device Remote device.
     * @since 6
     */
    void SetPeripheralDevice(const NearlinkRemoteDevice &device);

    /**
     * @brief Set peer device rssi.
     *
     * @param rssi Peer device rssi.
     * @since 6
     */
    void SetRssi(int8_t rssi);

    /**
     * @brief Set connectable.
     *
     * @param connectable Whether it is connectable.
     * @since 6
     */
    void SetConnectable(bool connectable);

    /**
     * @brief Set advertiser flag.
     *
     * @param flag Advertiser flag.
     * @since 6
     */
    void SetAdvertiseFlag(uint8_t flag);
    void SetDeviceClass(uint32_t deviceClass);
    void SetName(const std::string &name);
    std::string GetName(void);

private:
    std::vector<UUID> serviceUuids_ {}; // uuid is big endian order.
    std::map<uint16_t, std::string> manufacturerSpecificData_ {};
    std::map<UUID, std::string> serviceData_ {}; // uuid is big endian order.
    NearlinkRemoteDevice peripheralDevice_ {};
    int8_t rssi_ {};
    bool connectable_ {};
    uint8_t advertiseFlag_ {};
    std::vector<uint8_t> payload_ {};
    std::string name_ {};
    uint32_t deviceClass_ {};
};
/**
 * @brief Represents central manager callback.
 *
 * @since 6
 */
class SleCentralManagerCallback {
public:
    /**
     * @brief A destructor used to delete the <b>SleCentralManagerCallback</b> instance.
     *
     * @since 6
     */
    virtual ~SleCentralManagerCallback() = default;

    /**
     * @brief Scan result callback.
     *
     * @param result Scan result.
     * @since 6
     */
    virtual void OnScanCallback(const SleScanResult &result) = 0;

    /**
     * @brief Batch scan results event callback.
     *
     * @param results Scan results.
     * @since 6
     */
    virtual void OnSleBatchScanResultsEvent(const std::vector<SleScanResult> &results) = 0;

    /**
     * @brief Start or Stop scan event callback.
     *
     * @param resultCode Scan result code.
     * @param isStartScan true->start scan, false->stop scan.
     * @since 6
     */
    virtual void OnStartOrStopScanEvent(int resultCode, bool isStartScan) = 0;
};

/**
 * @brief 单PHY扫描参数
 */
struct SleScanPhyParam {
    uint8_t scan_phy;
    uint8_t scan_type;
    uint16_t scan_interval;
    uint16_t scan_window;
};

/**
 * @brief Represents Scan settings.
 *
 * @since 6
 */
class NEARLINK_API SleScanSettings {
public:
    /**
     * @brief A constructor used to create a <b>SleScanSettings</b> instance.
     *
     * @since 6
     */
    SleScanSettings();

    /**
     * @brief A destructor used to delete the <b>SleScanSettings</b> instance.
     *
     * @since 6
     */
    ~SleScanSettings();

    /**
     * @brief Set repport delay time.
     *
     * @param reportDelayMillis Repport delay time.
     * @since 6
     */
    void SetReportDelay(long reportDelayMillis = 0);

    /**
     * @brief Get repport delay time.
     *
     * @return Returns Repport delay time.
     * @since 6
     */
    long GetReportDelayMillisValue() const;

    /**
     * @brief Set scan mode.
     *
     * @param scanMode Scan mode.
     * @return Returns <b>true</b> if set scanMode successfully;
     *         returns <b>false</b> if set scanMode failed.
     * @since 6
     */
    int SetScanMode(int scanMode);

    /**
     * @brief Get scan mode.
     *
     * @return Scan mode.
     * @since 6
     */
    int GetScanMode() const;

    /**
     * @brief Set legacy flag.
     *
     * @param legacy Legacy value.
     * @since 6
     */
    void SetLegacy(bool legacy);

    /**
     * @brief Get legacy flag.
     *
     * @return Legacy flag.
     * @since 6
     */
    bool GetLegacy() const;

    /**
     * @brief Set scan duration.
     *
     * @param scanDuration integer in range of 10 to 60 seconds, <b>inclusive</b>.
     * @since 6
     */
    void SetDuration(int scanDuration);

    /**
     * @brief Get scan duration.
     *
     * @return Scan duration
     * @since 6
     */
    int GetDuration() const;

    /**
     * @brief Set frame type.
     *
     * @param frameType frame type.
     * @since 6
     */
    void SetFrameType(uint8_t frameType);

    /**
     * @brief Get frame type.
     *
     * @return Returns frame type.
     * @since 6
     */
    uint8_t GetFrameType() const;

private:
    long reportDelayMillis_ = 0;
    int scanMode_ = SCAN_MODE_LOW_POWER;
    int duration_ = 0;
    bool legacy_ = true;
    uint8_t frameType_ = static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_1);
};

/**
 * @brief Represents Scan filter.
 *
 */
class NEARLINK_API SleScanFilter {
public:
    /**
     * @brief A constructor used to create a <b>SleScanFilter</b> instance.
     *
     */
    SleScanFilter();

    /**
     * @brief A destructor used to delete the <b>SleScanFilter</b> instance.
     *
     */
    ~SleScanFilter();

    void SetDeviceId(std::string deviceId); // addr is little endian order.

    std::string GetDeviceId() const;

    void SetName(std::string name);

    std::string GetName() const;

    /* uuid is big endian order. */
    void SetServiceUuid(const UUID &uuid);

    bool HasServiceUuid();

    /* uuid is big endian order. */
    UUID GetServiceUuid() const;

    void SetServiceUuidMask(const UUID &serviceUuidMask);

    bool HasServiceUuidMask();

    UUID GetServiceUuidMask() const;

    /* uuid is big endian order. */
    void SetServiceSolicitationUuid(const UUID &serviceSolicitationUuid);

    bool HasSolicitationUuid();

    /* uuid is big endian order. */
    UUID GetServiceSolicitationUuid() const;

    void SetServiceSolicitationUuidMask(const UUID &erviceSolicitationUuidMask);

    bool HasSolicitationUuidMask();

    UUID GetServiceSolicitationUuidMask() const;

    void SetServiceData(std::vector<uint8_t> serviceData);

    bool HasServiceData() const;

    std::vector<uint8_t> GetServiceData() const;

    void SetServiceDataMask(std::vector<uint8_t> serviceDataMask);

    bool HasServiceDataMask() const;

    std::vector<uint8_t> GetServiceDataMask() const;

    void SetManufacturerId(uint16_t manufacturerId);

    uint16_t GetManufacturerId() const;

    void SetManufactureData(std::vector<uint8_t> manufactureData);

    std::vector<uint8_t> GetManufactureData() const;

    void SetManufactureDataMask(std::vector<uint8_t> manufactureDataMask);

    std::vector<uint8_t> GetManufactureDataMask() const;

    void SetSensorHubChannel(bool isSensorHubChannel);

    bool IsSensorHubChannel();

    void SetRssiThreshold(int8_t rssi);

    int8_t GetRssiThreshold() const;

    bool HasRssiThreshold() const;

    private:
        std::string deviceId_; // address is little endian order.
        std::string name_;

        UUID serviceUuid_; // uuid is big endian order.
        UUID serviceUuidMask_;
        UUID serviceSolicitationUuid_; // uuid is big endian order.
        UUID serviceSolicitationUuidMask_;
        bool hasServiceUuid_ = false;
        bool hasServiceUuidMask_ = false;
        bool hasSolicitationUuid_ = false;
        bool hasSolicitationUuidMask_ = false;
        bool hasServiceData_ = false;
        bool hasServiceDataMask_ = false;
        bool hasRssiThreshold_ = false;

        std::vector<uint8_t> serviceData_;
        std::vector<uint8_t> serviceDataMask_;

        uint16_t manufacturerId_ = 0;
        std::vector<uint8_t> manufactureData_;
        std::vector<uint8_t> manufactureDataMask_;
        bool isSensorHubChannel_ = false;
        int8_t rssiThreshold_ = RSSI_DEFAULT_THRESHOLD;
};

struct SleActiveDeviceInfo {
    std::vector<int8_t> deviceId;
    int32_t status;
    int32_t timeOut;
};

/**
 * @brief Represents central manager.
 *
 * @since 6
 */
class NEARLINK_API SleCentralManager {
public:
    /**
     * @brief A constructor of SleCentralManager.
     */
    static std::shared_ptr<SleCentralManager> CreateSleCentralManager(
        std::shared_ptr<SleCentralManagerCallback> callback);

    /**
     * @brief A destructor used to delete the <b>SleCentralManager</b> instance.
     *
     * @since 6
     */
    ~SleCentralManager();

    /**
     * @brief Start scan with filter.
     *
     * @param settings Scan settings.
     * @return Returns the status code for this function called.
     */
    NlErrCode StartScanWithFilter(const SleScanSettings &settings, const std::vector<SleScanFilter> &filters);

    /**
     * @brief Start full scan.
     *
     * @param settings Scan settings.
     * @return Returns the status code for this function called.
     */
    NlErrCode StartFullScan(const SleScanSettings &settings);

    /**
     * @brief Start scan with default settings, invoke StartFullScan inside.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode StartScan();

    /**
     * @brief Stop scan.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode StopScan();

private:

    explicit SleCentralManager(std::shared_ptr<SleCentralManagerCallback> callback);

    NEARLINK_DISALLOW_COPY_AND_ASSIGN(SleCentralManager);

    NEARLINK_DECLARE_IMPL();

    // It has no specific meaning, only used to hide the constructor.
    struct Pattern {
        Pattern() {};
    };

public:
    // This constructor is not available, use CreateSleCentralManager interface to create objects.
    explicit SleCentralManager(Pattern, std::shared_ptr<SleCentralManagerCallback> callback) :
        SleCentralManager(callback) {};
};
} // namespace Nearlink
} // namespace OHOS
#endif