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

#ifndef SCAN_ADAPTER_H
#define SCAN_ADAPTER_H

#include "ScanStackAdapter.h"

namespace OHOS {
namespace Nearlink {

class ScanService : public InterfaceScanService {
public:
    ScanService();
    ~ScanService();

    static ScanService &GetInstance();
    void RegisterSleCentralManagerCallback(ISleCentralManagerCallback &callback);
    void DeregisterSleCentralManagerCallback() const;
    uint32_t AllocScannerId();
    void RemoveScannerId(uint32_t scannerId);
    void StartScan(uint32_t scannerId, const NearlinkSleScanSettings &settings,
        const std::vector<SleScanFilterImpl> &filters);
    void StopScan(uint32_t scannerId) const;
    void StopAllScan();
    void NotifyStartOrStopScanEvent(int resultCode, bool isStartScanEvt);
    void NotifyScanResults(NLSTK_DevdAdvResult_S *result);

    void ClearScanResultInfo() const;

    std::string GetDeviceName(const std::string &address) const;
    int GetDeviceAppearance(const std::string &address) const;
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> GetDeviceManufacturerAbility(const std::string &address) const;
    uint8_t GetDeviceAddrType(const std::string &address) const;
    int GetManufacturerBusinessType(const std::string &address) const;
    bool IsAudioDevice(const std::string &address) const;
    std::string GetModelId(const std::string &address) const;
    std::string GetNewModelId(const std::string &address) const;
    std::string GetSubModelId(const std::string &address) const;
    std::string GetIconId(const std::string &address) const;
    std::string GetBtAddr(const std::string &address) const;
    RawAddress GetCurrentAddress(const RawAddress &reportAddr) const;
    RawAddress GetCollaborateAddress(const RawAddress &reportAddr) const;
    RawAddress GetReportAddrByCurrentAddress(const RawAddress &currentAddr) const;
private:
    void ParseAdvResult(NLSTK_DevdAdvResult_S *devdResult, std::vector<uint32_t> &scannerIds,
        SleScanResultImpl &scanResult);
    void ParseAdvData(NLSTK_DevdAdvResult_S *result, SlePeripheralDevice &device);
    void ParseManufacturerData(SlePeripheralDevice &device);
    void ParseManufacturerDataAudio(SlePeripheralDevice &device, std::string &privateData);
    bool ParseAdvEarphoneDisplayControl(SlePeripheralDevice &device, size_t &msgIndex, const std::string &privateData);
    bool ParseAdvDeviceManufacturerAbility(SlePeripheralDevice &device, size_t &msgIndex,
        const std::string &privateData);
    void ProcManufactureCollabrateAddr(SlePeripheralDevice &device, std::string &reportAddr, std::string &collabAddr);

    void AddPeripheralDevice(const SleScanResultImpl &scanResult);
    void UpdateScanResultQueue(const std::string &address);
    void ClearOldPeripheralDevice(const RawAddress &advertisedAddress, const SlePeripheralDevice &device);

    SLE_DISALLOW_COPY_AND_ASSIGN(ScanService);
    DECLARE_IMPL();
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // SCAN_ADAPTER_H