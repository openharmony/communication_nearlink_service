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

#include "SleMultiScanData.h"

#include <algorithm>

#include "array"
#include "map"
#include "vector"
#include "log.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief Represents scan settings.
 *
 * @since 6
 */

void SleScanSettingsImpl::SetReportDelay(long reportDelayMillis)
{
    reportDelayMillis_ = reportDelayMillis;
}

/**
 * @brief Get the report delay time.
 *
 * @return Returns Report delay time.
 * @since 6
 */
long SleScanSettingsImpl::GetReportDelayMillisValue() const
{
    return reportDelayMillis_;
}

void SleScanSettingsImpl::SetScanInterval(uint16_t scanInterval)
{
    scanInterval_ = scanInterval;
}

uint16_t SleScanSettingsImpl::GetScanInterval() const
{
    return scanInterval_;
}

void SleScanSettingsImpl::SetScanWindow(uint16_t scanWindow)
{
    scanWindow_ = scanWindow;
}

uint16_t SleScanSettingsImpl::GetScanWindow() const
{
    return scanWindow_;
}

void SleScanSettingsImpl::SetLegacy(bool legacy)
{
    legacy_ = legacy;
}

bool SleScanSettingsImpl::GetLegacy() const
{
    return legacy_;
}

void SleScanSettingsImpl::SetPhy(int phy)
{
    phy_ = phy;
}

int SleScanSettingsImpl::GetPhy() const
{
    return phy_;
}

void SleScanSettingsImpl::SetFrameType(uint8_t frameType)
{
    frameType_ = frameType;
}

uint8_t SleScanSettingsImpl::GetFrameType() const
{
    return frameType_;
}

void SleScanFilterImpl::SetDeviceId(std::string deviceId)
{
    deviceId_ = deviceId;
}

std::string SleScanFilterImpl::GetDeviceId() const
{
    return deviceId_;
}

void SleScanFilterImpl::SetName(std::string name)
{
    name_ = name;
}

std::string SleScanFilterImpl::GetName() const
{
    return name_;
}

void SleScanFilterImpl::SetServiceUuid(const Uuid &serviceUuid)
{
    serviceUuid_ = serviceUuid;
    hasServiceUuid_ = true;
}

bool SleScanFilterImpl::HasServiceUuid() const
{
    return hasServiceUuid_;
}

Uuid SleScanFilterImpl::GetServiceUuid() const
{
    return serviceUuid_;
}

void SleScanFilterImpl::SetServiceUuidMask(const Uuid &serviceUuidMask)
{
    serviceUuidMask_ = serviceUuidMask;
    hasServiceUuidMask_ = true;
}

bool SleScanFilterImpl::HasServiceUuidMask() const
{
    return hasServiceUuidMask_;
}

Uuid SleScanFilterImpl::GetServiceUuidMask() const
{
    return serviceUuidMask_;
}

void SleScanFilterImpl::SetServiceSolicitationUuid(const Uuid &serviceSolicitationUuid)
{
    serviceSolicitationUuid_ = serviceSolicitationUuid;
    hasSolicitationUuid_ = true;
}

bool SleScanFilterImpl::HasSolicitationUuid() const
{
    return hasSolicitationUuid_;
}

Uuid SleScanFilterImpl::GetServiceSolicitationUuid() const
{
    return serviceSolicitationUuid_;
}

void SleScanFilterImpl::SetServiceSolicitationUuidMask(const Uuid &serviceSolicitationUuidMask)
{
    serviceSolicitationUuidMask_ = serviceSolicitationUuidMask;
    hasSolicitationUuidMask_ = true;
}

bool SleScanFilterImpl::HasSolicitationUuidMask() const
{
    return hasSolicitationUuidMask_;
}

Uuid SleScanFilterImpl::GetServiceSolicitationUuidMask() const
{
    return serviceSolicitationUuidMask_;
}

void SleScanFilterImpl::SetServiceData(std::vector<uint8_t> serviceData)
{
    serviceData_ = serviceData;
    hasServiceData_ = true;
}

bool SleScanFilterImpl::HasServiceData() const
{
    return hasServiceData_;
}

std::vector<uint8_t> SleScanFilterImpl::GetServiceData() const
{
    return serviceData_;
}

void SleScanFilterImpl::SetServiceDataMask(std::vector<uint8_t> serviceDataMask)
{
    serviceDataMask_ = serviceDataMask;
    hasServiceDataMask_ = true;
}

bool SleScanFilterImpl::HasServiceDataMask() const
{
    return hasServiceDataMask_;
}

std::vector<uint8_t> SleScanFilterImpl::GetServiceDataMask() const
{
    return serviceDataMask_;
}

void SleScanFilterImpl::SetManufacturerId(uint16_t manufacturerId)
{
    manufacturerId_ = manufacturerId;
}

uint16_t SleScanFilterImpl::GetManufacturerId() const
{
    return manufacturerId_;
}

void SleScanFilterImpl::SetManufactureData(std::vector<uint8_t> manufactureData)
{
    manufactureData_ = manufactureData;
}

std::vector<uint8_t> SleScanFilterImpl::GetManufactureData() const
{
    return manufactureData_;
}

void SleScanFilterImpl::SetManufactureDataMask(std::vector<uint8_t> manufactureDataMask)
{
    manufactureDataMask_ = manufactureDataMask;
}

std::vector<uint8_t> SleScanFilterImpl::GetManufactureDataMask() const
{
    return manufactureDataMask_;
}

void SleScanFilterImpl::SetClientId(int clientId)
{
    clientId_ = clientId;
}

int SleScanFilterImpl::GetClientId() const
{
    return clientId_;
}

void SleScanFilterImpl::SetFiltIndex(uint8_t filtIndex)
{
    filtIndex_ = filtIndex;
}

uint8_t SleScanFilterImpl::GetFiltIndex() const
{
    return filtIndex_;
}

void SleScanFilterImpl::SetFilterAction(uint8_t action)
{
    action_ = action;
}

uint8_t SleScanFilterImpl::GetFilterAction() const
{
    return action_;
}

void SleScanFilterImpl::SetSensorHubChannel(bool isSensorHubChannel)
{
    isSensorHubChannel_ = isSensorHubChannel;
}

bool SleScanFilterImpl::IsSensorHubChannel() const
{
    return isSensorHubChannel_;
}

void SleScanFilterImpl::SetAdvIndReport(bool advIndReport)
{
    advIndReport_ = advIndReport;
}

bool SleScanFilterImpl::GetAdvIndReport() const
{
    return advIndReport_;
}

void SleScanFilterImpl::SetRssiThreshold(int8_t rssi)
{
    rssiThreshold_ = rssi;
    hasRssiThreshold_ = true;
}

int8_t SleScanFilterImpl::GetRssiThreshold() const
{
    return rssiThreshold_;
}

bool SleScanFilterImpl::HasRssiThreshold() const
{
    return hasRssiThreshold_;
}
}  // namespace Sle
}  // namespace OHOS
