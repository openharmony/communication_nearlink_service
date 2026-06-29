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
#ifndef INTERFACE_SCAN_SERVICE_H
#define INTERFACE_SCAN_SERVICE_H

#include "SleMultiScanData.h"
#include "nearlink_sle_scan_settings.h"
#include "nearlink_sle_scan_filter.h"

namespace OHOS {
namespace Nearlink {
constexpr std::array<uint8_t, SLE_MANU_ABILITY_LEN> INVALID_MANUFACTURER_ABLITITY ={0};

class ISleCentralManagerCallback {
public:
    virtual ~ISleCentralManagerCallback() = default;
    virtual void OnScanCallback(const std::vector<uint32_t> &scannerIds, const SleScanResultImpl &result) = 0;
    virtual void OnSleBatchScanResultsEvent(std::vector<SleScanResultImpl> &results) = 0;
    virtual void OnStartOrStopScanEvent(int resultCode, bool isStartScan) = 0;
};

class InterfaceScanService {
public:
    virtual ~InterfaceScanService() = default;
    static InterfaceScanService &GetInstance();
    virtual void RegisterSleCentralManagerCallback(ISleCentralManagerCallback &callback) = 0;
    virtual void DeregisterSleCentralManagerCallback() const = 0;
    virtual void StartScan(uint32_t scannerId, const NearlinkSleScanSettings &settings,
        const std::vector<SleScanFilterImpl> &filters) = 0;
    virtual void StopScan(uint32_t scannerId) const = 0;
    virtual void StopAllScan() = 0;
    virtual void ClearScanResultInfo() const = 0;
    virtual uint32_t AllocScannerId() = 0;
    virtual void RemoveScannerId(uint32_t scannerId) = 0;
    virtual std::string GetDeviceName(const std::string &address) const = 0;
    virtual int GetDeviceAppearance(const std::string &address) const = 0;
    virtual std::array<uint8_t, SLE_MANU_ABILITY_LEN> GetDeviceManufacturerAbility(const std::string &address)
        const = 0;
    virtual uint8_t GetDeviceAddrType(const std::string &address) const = 0;
    virtual int GetManufacturerBusinessType(const std::string &address) const = 0;
    virtual bool IsAudioDevice(const std::string &address) const = 0;
    virtual RawAddress GetCurrentAddress(const RawAddress &reportAddr) const = 0;
    virtual RawAddress GetReportAddrByCurrentAddress(const RawAddress &currentAddr) const = 0;
    virtual std::string GetModelId(const std::string &address) const = 0;
    virtual std::string GetNewModelId(const std::string &address) const = 0;
    virtual std::string GetSubModelId(const std::string &address) const = 0;
    virtual std::string GetIconId(const std::string &address) const = 0;
    virtual std::string GetBtAddr(const std::string &address) const = 0;
    virtual RawAddress GetCollaborateAddress(const RawAddress &reportAddr) const = 0;
};
} // namespace Nearlink
} // namespace OHOS
#endif // INTERFACE_SCAN_SERVICE_H