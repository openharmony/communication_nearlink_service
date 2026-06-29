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

#ifndef SLE_MULTI_SCAN_DATA_H
#define SLE_MULTI_SCAN_DATA_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>
#include "nearlink_def.h"
#include "sle_uuid.h"
#include "cstdint"
#include "iosfwd"
#include "raw_address.h"
#include "string"
#include "utility"

namespace OHOS {
namespace Nearlink {
/**
 * @brief Represents scan settings.
 *
 * @since 6
 */
class SleScanSettingsImpl {
public:
    /**
     * @brief A constructor used to create a <b>BleScanSettingsInternal</b> instance.
     *
     * @since 6
     */
    SleScanSettingsImpl(){};

    /**
     * @brief A destructor used to delete the <b>BleScanSettingsInternal</b> instance.
     *
     * @since 6
     */
    ~SleScanSettingsImpl(){};

    /**
     * @brief Set report delay time.
     *
     * @param reportDelayMillis Report delay time.
     * @since 6
     */
    void SetReportDelay(long reportDelayMillis = 0);

    /**
     * @brief Get report delay time.
     *
     * @return Report delay time.
     * @since 6
     */
    long GetReportDelayMillisValue() const;

    /**
     * @brief Set scan interval.
     *
     * @param scanInterval Scan interval.
     * @since 6
     */
    void SetScanInterval(uint16_t scanInterval);

    /**
     * @brief Get scan interval.
     *
     * @return Scan interval.
     * @since 6
     */
    uint16_t GetScanInterval() const;

    /**
     * @brief Set scan window.
     *
     * @param scanWindow Scan window.
     * @since 6
     */
    void SetScanWindow(uint16_t scanWindow);

    /**
     * @brief Get scan window.
     *
     * @return Scan window.
     * @since 6
     */
    uint16_t GetScanWindow() const;

    /**
     * @brief Set legacy flag.
     *
     * @param legacy Legacy flag.
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
     * @brief Set phy value.
     *
     * @param phy Phy value.
     * @since 6
     */
    void SetPhy(int phy);

    /**
     * @brief Get phy value.
     *
     * @return Phy value.
     * @since 6
     */
    int GetPhy() const;

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
    /// delay millistime
    long reportDelayMillis_ = 0;
    uint16_t scanInterval_ = 0;
    uint16_t scanWindow_ = 0;
    bool legacy_ = true;
    int phy_ = static_cast<uint8_t>(SlePhyType::PHY_LE_ALL_SUPPORTED);
    uint8_t frameType_ = static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_1);
};

/**
 * @brief Represents scan filter.
 *
 */
class SleScanFilterImpl {
public:
    SleScanFilterImpl() {}
    ~SleScanFilterImpl() {}

    /**
     * @brief Set device id.
     *
     * @param deviceId filter device id.
     */
    void SetDeviceId(std::string deviceId);

    /**
     * @brief Get filter device id.
     *
     * @return Returns filter device id.
     */
    std::string GetDeviceId() const;

    void SetName(std::string name);

    std::string GetName() const;

    void SetServiceUuid(const Uuid &serviceUuid);

    bool HasServiceUuid() const;

    Uuid GetServiceUuid() const;

    void SetServiceUuidMask(const Uuid &serviceUuidMask);

    bool HasServiceUuidMask() const;

    Uuid GetServiceUuidMask() const;

    void SetServiceSolicitationUuid(const Uuid &serviceSolicitationUuid);

    bool HasSolicitationUuid() const;

    Uuid GetServiceSolicitationUuid() const;

    void SetServiceSolicitationUuidMask(const Uuid &serviceSolicitationUuidMask);

    bool HasSolicitationUuidMask() const;

    Uuid GetServiceSolicitationUuidMask() const;

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

    void SetClientId(int clientId);

    int GetClientId() const;

    void SetFiltIndex(uint8_t filtIndex);

    uint8_t GetFiltIndex() const;

    void SetFilterAction(uint8_t action);

    uint8_t GetFilterAction() const;

    void SetSensorHubChannel(bool isSensorHubChannel);

    bool IsSensorHubChannel() const;
    void SetAdvIndReport(bool advIndReport);

    bool GetAdvIndReport() const;
    
    void SetRssiThreshold(int8_t rssi);

    int8_t GetRssiThreshold() const;

    bool HasRssiThreshold() const;
private:
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
    bool hasRssiThreshold_ = false;

    std::vector<uint8_t> serviceData_;
    std::vector<uint8_t> serviceDataMask_;

    uint16_t manufacturerId_ = 0;
    std::vector<uint8_t> manufactureData_;
    std::vector<uint8_t> manufactureDataMask_;

    int clientId_ = 0;
    uint8_t filtIndex_ = 0;
    uint8_t action_ = -1;
    int8_t rssiThreshold_ = RSSI_DEFAULT_THRESHOLD;
    bool isSensorHubChannel_ = false;
    bool advIndReport_ = false;
};
}  // namespace Sle
}  // namespace OHOS
#endif  /// SLE_MULTI_SCAN_DATA_H
