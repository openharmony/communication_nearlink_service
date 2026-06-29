/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef NEARLINK_SCAN_DEF_H
#define NEARLINK_SCAN_DEF_H

#include <map>
#include <vector>

#include "sle_service_data.h"
#include "nearlink_sle_scanner.h"
#include "sle_uuid.h"
#include "raw_address.h"
#include "nearlink_def_types.h"

namespace OHOS {
namespace Nearlink {
class ScanResult {
public:
    /**
     * @brief A constructor used to create a NearlinkScanResult instance.
     *
     * @since 6
     */
    ScanResult(){};

    explicit ScanResult(const SleScanResultImpl &other);

    /**
     * @brief A destructor used to delete the NearlinkScanResult instance.
     *
     * @since 6
     */
    virtual ~ScanResult(){};

    /**
     * @brief Get service uuids.
     *
     * @return Returns service uuids.
     * @since 6
     */
    std::vector<Uuid> GetServiceUuids() const
    {
        return serviceUuids_;
    }

    /**
     * @brief Get manufacture data.
     *
     * @return Returns manufacture data.
     * @since 6
     */
    std::map<uint16_t, std::string> GetManufacturerData() const
    {
        return manufacturerSpecificData_;
    }

    /**
     * @brief Get service data.
     *
     * @return Returns service data.
     * @since 6
     */
    std::map<Uuid, std::string> GetServiceData() const
    {
        return serviceData_;
    }

    /**
     * @brief Get peripheral device.
     *
     * @return Returns peripheral device pointer.
     * @since 6
     */
    const RawAddress &GetPeripheralDevice() const
    {
        return addr_;
    }

    /**
     * @brief Get peer device rssi.
     *
     * @return Returns peer device rssi.
     * @since 6
     */
    int8_t GetRssi() const
    {
        return rssi_;
    }

    /**
     * @brief Check if device is connectable.
     *
     * @return Returns true if device is connectable;
     *         returns false if device is not connectable.
     * @since 6
     */
    bool IsConnectable() const
    {
        return connectable_;
    }

    /**
     * @brief Get advertiser flag.
     *
     * @return Returns advertiser flag.
     * @since 6
     */
    uint8_t GetAdvertiseFlag() const
    {
        return advertiseFlag_;
    }

    /**
     * @brief Add manufacture data.
     *
     * @param manufacturerId Manufacture Id which addad data.
     * @since 6
     */
    void AddManufacturerData(uint16_t manufacturerId, std::string data)
    {
        manufacturerSpecificData_.insert(std::make_pair(manufacturerId, data));
    }

    /**
     * @brief Add service data.
     *
     * @param uuid Uuid of service data.
     * @param serviceData Service data.
     * @since 6
     */
    void AddServiceData(Uuid uuid, std::string serviceData)
    {
        serviceData_.insert(std::make_pair(uuid, serviceData));
    }

    /**
     * @brief Add service uuid.
     *
     * @param serviceUuid Service uuid.
     * @since 6
     */
    void AddServiceUuid(const Uuid &serviceUuid)
    {
        serviceUuids_.push_back(serviceUuid);
    }

    /**
     * @brief Set peripheral device.
     *
     * @param device Remote device.
     * @since 6
     */
    void SetPeripheralDevice(const RawAddress &device)
    {
        addr_ = device;
    }

    /**
     * @brief Set peer device rssi.
     *
     * @param rssi Peer device rssi.
     * @since 6
     */
    void SetRssi(int8_t rssi)
    {
        rssi_ = rssi;
    }

    /**
     * @brief Set connectable.
     *
     * @param connectable Whether it is connectable.
     * @since 6
     */
    void SetConnectable(bool connectable)
    {
        connectable_ = connectable;
    }

    /**
     * @brief Set advertiser flag.
     *
     * @param flag Advertiser flag.
     * @since 6
     */
    void SetAdvertiseFlag(uint8_t flag)
    {
        advertiseFlag_ = flag;
    }

    void SetPayload(const std::string &payload)
    {
        payload_ = payload;
    }

    std::string GetPayload() const
    {
        return payload_;
    }

    void SetName(const std::string &name)
    {
        name_ = name;
    }

    std::string GetName(void) const
    {
        return name_;
    }

    void SetDeviceClass(uint32_t deviceClass)
    {
        deviceClass_ = deviceClass;
    }

    uint32_t GetDeviceClass() const
    {
        return deviceClass_;
    }

    void SetPrimFrameType(uint8_t primFrameType)
    {
        primFrameType_ = primFrameType;
    }

    uint8_t GetPrimFrameType() const
    {
        return primFrameType_;
    }

public:
    std::vector<Uuid> serviceUuids_ {};
    std::map<uint16_t, std::string> manufacturerSpecificData_ {};
    std::map<Uuid, std::string> serviceData_ {};
    RawAddress addr_ {};
    int8_t rssi_ {};
    bool connectable_ {};
    uint8_t advertiseFlag_ {};
    std::string payload_ {};
    std::string name_ {};
    uint32_t deviceClass_ {};
    uint8_t primFrameType_ = static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_1);
};

/**
 * @brief Represents Scan settings.
 *
 * @since 6
 */
class ScanSettings {
public:
    static constexpr int DEFAULT_PHY_VALUE = 255;

    /**
     * @brief A constructor used to create a NearlinkScanSettings instance.
     *
     * @since 6
     */
    ScanSettings(){};

    explicit ScanSettings(const SleScanSettings &other)
        : reportDelayMillis_(0), duration_(other.GetDuration()), scanMode_(other.GetScanMode()),
        legacy_(true), phy_(DEFAULT_PHY_VALUE), frameType_(other.GetFrameType())
    {}

    /**
     * @brief A destructor used to delete the NearlinkScanSettings instance.
     *
     * @since 6
     */
    ~ScanSettings(){};

    /**
     * @brief Set report delay time.
     *
     * @param reportDelayMillis Report delay time.
     * @since 6
     */
    void SetReportDelay(long reportDelayMillis)
    {
        reportDelayMillis_ = reportDelayMillis;
    }

    /**
     * @brief Get report delay time.
     *
     * @return Returns Report delay time.
     * @since 6
     */
    long GetReportDelayMillisValue() const
    {
        return reportDelayMillis_;
    }

    void SetDuration(int duration)
    {
        duration_ = duration;
    }

    int GetDuration() const
    {
        return duration_;
    }

    /**
     * @brief Set scan mode.
     *
     * @param scanMode Scan mode.
     * @return If the scanMode is invalid.
     * @since 6
     */
    void SetScanMode(int scanMode)
    {
        scanMode_ = scanMode;
    }

    /**
     * @brief Get scan mode.
     *
     * @return Scan mode.
     * @since 6
     */
    int GetScanMode() const
    {
        return scanMode_;
    }

    /**
     * @brief Set legacy flag.
     *
     * @param legacy Legacy value.
     * @since 6
     */
    void SetLegacy(bool legacy)
    {
        legacy_ = legacy;
    }

    /**
     * @brief Get legacy flag.
     *
     * @return Legacy flag.
     * @since 6
     */
    bool GetLegacy() const
    {
        return legacy_;
    }

    /**
     * @brief Set phy value.
     *
     * @param phy Phy value.
     * @since 6
     */
    void SetPhy(int phy)
    {
        phy_ = phy;
    }

    /**
     * @brief Get phy value.
     *
     * @return Phy value.
     * @since 6
     */
    int GetPhy() const
    {
        return phy_;
    }

    uint8_t GetFrameType() const
    {
        return frameType_;
    }

    void SetFrameType(uint8_t frameType)
    {
        frameType_ = frameType;
    }

public:
    long reportDelayMillis_ = 0;
    int duration_ = 0;
    int scanMode_ = 0;
    bool legacy_ = true;
    int phy_ = DEFAULT_PHY_VALUE;
    uint8_t frameType_ = static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_1);
};

/**
 * @brief Represents Scan filter.
 *
 */
class ScanFilter {
public:
    /**
     * @brief A constructor used to create a BleScanFilter instance.
     *
     */
    ScanFilter() {}

    /**
     * @brief A destructor used to delete the BleScanFilter instance.
     *
     */
    ~ScanFilter() {}

    /**
     * @brief Set device id.
     *
     * @param deviceId device id.
     */
    void SetDeviceId(std::string deviceId)
    {
        deviceId_ = deviceId;
    }

    /**
     * @brief Get device id.
     *
     * @return Returns device id.
     */
    std::string GetDeviceId() const
    {
        return deviceId_;
    }

    void SetName(std::string name)
    {
        name_ = name;
    }

    std::string GetName() const
    {
        return name_;
    }

    void SetServiceUuid(const Uuid uuid)
    {
        serviceUuid_ = uuid;
        hasServiceUuid_ = true;
    }

    bool HasServiceUuid()
    {
        return hasServiceUuid_;
    }

    Uuid GetServiceUuid() const
    {
        return serviceUuid_;
    }

    void SetServiceUuidMask(const Uuid serviceUuidMask)
    {
        serviceUuidMask_ = serviceUuidMask;
        hasServiceUuidMask_ = true;
    }

    bool HasServiceUuidMask()
    {
        return hasServiceUuidMask_;
    }

    Uuid GetServiceUuidMask() const
    {
        return serviceUuidMask_;
    }

    void SetServiceSolicitationUuid(const Uuid serviceSolicitationUuid)
    {
        serviceSolicitationUuid_ = serviceSolicitationUuid;
        hasSolicitationUuid_ = true;
    }

    bool HasSolicitationUuid()
    {
        return hasSolicitationUuid_;
    }

    Uuid GetServiceSolicitationUuid() const
    {
        return serviceSolicitationUuid_;
    }

    void SetServiceSolicitationUuidMask(const Uuid serviceSolicitationUuidMask)
    {
        serviceSolicitationUuidMask_ = serviceSolicitationUuidMask;
        hasSolicitationUuidMask_ = true;
    }

    bool HasSolicitationUuidMask()
    {
        return hasSolicitationUuidMask_;
    }

    Uuid GetServiceSolicitationUuidMask() const
    {
        return serviceSolicitationUuidMask_;
    }

    void SetServiceData(std::vector<uint8_t> serviceData)
    {
        serviceData_ = serviceData;
        hasServiceData_ = true;
    }

    bool HasServiceData() const
    {
        return hasServiceData_;
    }

    std::vector<uint8_t> GetServiceData() const
    {
        return serviceData_;
    }

    void SetServiceDataMask(std::vector<uint8_t> serviceDataMask)
    {
        serviceDataMask_ = serviceDataMask;
        hasServiceDataMask_ = true;
    }

    bool HasServiceDataMask() const
    {
        return hasServiceDataMask_;
    }

    std::vector<uint8_t> GetServiceDataMask() const
    {
        return serviceDataMask_;
    }

    void SetManufacturerId(uint16_t manufacturerId)
    {
        manufacturerId_ = manufacturerId;
    }

    uint16_t GetManufacturerId() const
    {
        return manufacturerId_;
    }

    void SetManufactureData(std::vector<uint8_t> manufactureData)
    {
        manufactureData_ = manufactureData;
    }

    std::vector<uint8_t> GetManufactureData() const
    {
        return manufactureData_;
    }

    void SetManufactureDataMask(std::vector<uint8_t> manufactureDataMask)
    {
        manufactureDataMask_ = manufactureDataMask;
    }

    std::vector<uint8_t> GetManufactureDataMask() const
    {
        return manufactureDataMask_;
    }

    void SetSensorHubChannel(bool isSensorHubChannel)
    {
        isSensorHubChannel_ = isSensorHubChannel;
    }

    bool IsSensorHubChannel() const
    {
        return isSensorHubChannel_;
    }

    void SetAdvIndReport(bool advIndReport)
    {
        advIndReport_ = advIndReport;
    }

    bool GetAdvIndReport() const
    {
        return advIndReport_;
    }

    void SetRssiThreshold(int8_t rssi)
    {
        rssiThreshold_ = rssi;
        hasRssiThreshold_ = true;
    }

    int8_t GetRssiThreshold() const
    {
        return rssiThreshold_;
    }

    bool HasRssiThreshold() const
    {
        return hasRssiThreshold_;
    }

public:
    std::string deviceId_;
    std::string name_;

    Uuid serviceUuid_;
    Uuid serviceUuidMask_;
    Uuid serviceSolicitationUuid_;
    Uuid serviceSolicitationUuidMask_;
    bool hasServiceUuid_ = false;
    bool hasServiceUuidMask_ = false;
    bool hasSolicitationUuid_ = false;
    bool hasSolicitationUuidMask_ = false;
    bool hasServiceData_ = false;
    bool hasServiceDataMask_ = false;

    std::vector<uint8_t> serviceData_;
    std::vector<uint8_t> serviceDataMask_;

    uint16_t manufacturerId_ = 0;
    std::vector<uint8_t> manufactureData_;
    std::vector<uint8_t> manufactureDataMask_;
    int8_t rssiThreshold_ = RSSI_DEFAULT_THRESHOLD;
    bool isSensorHubChannel_ = false;
    bool advIndReport_ = false;
    bool hasRssiThreshold_ = false;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  /// NEARLINK_SCAN_DEF_H