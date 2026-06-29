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

#ifndef SCAN_UTILS_H
#define SCAN_UTILS_H

#include <string>
#include <vector>
#include "nlstk_scan_api.h"
#include "SleMultiScanData.h"

namespace OHOS {
namespace Nearlink {
class ScanUtils {
public:
    static void ConvertScanSettings(const NearlinkSleScanSettings& settings, NLSTK_DevdScanSetting_S &devdSettings)
    {
        devdSettings.reportDelayMillis = settings.GetReportDelayMillisValue();
        devdSettings.duration = settings.GetDuration();
        devdSettings.scanMode = settings.GetScanMode();
        devdSettings.legacy = settings.GetLegacy();
        devdSettings.phy = settings.GetPhy();
        devdSettings.frameType = settings.GetFrameType();
    }

    static void CopyVectorToAllocData(const std::vector<uint8_t> &src, NLSTK_VariableData_S &dst)
    {
        if (src.empty()) {
            return;
        }
        dst.data = new (std::nothrow) uint8_t[src.size()];
        if (dst.data != nullptr) {
            dst.len = src.size();
            (void)memcpy_s(dst.data, src.size(), src.data(), src.size());
        }
    }

    static void CopyUuidData(const Uuid &uuid, uint8_t *dst)
    {
        auto arr = uuid.ConvertTo128Bits();
        (void)memcpy_s(dst, SERVICE_UUID_LEN_128, arr.data(), SERVICE_UUID_LEN_128);
    }

    // 申请了内存, 需要FreeDevdFilter释放内存
    static void CreatDevdFilter(const SleScanFilterImpl &filter, NLSTK_DevdScanFilter_S *devdFilter)
    {
        std::string deviceId = filter.GetDeviceId();
        if (!deviceId.empty()) {
            devdFilter->hasAddr = true;
            RawAddress addr(deviceId);
            addr.ConvertToUint8(devdFilter->addr.addr);
        } else {
            devdFilter->hasAddr = false;
        }
        std::string filterName = filter.GetName();
        std::vector<uint8_t> nameVec(filterName.begin(), filterName.end());
        CopyVectorToAllocData(nameVec, devdFilter->name);
        if (filter.HasServiceUuid()) {
            devdFilter->hasServiceUuid = true;
            CopyUuidData(filter.GetServiceUuid(), devdFilter->serviceUuid);
        }
        if (filter.HasServiceUuidMask()) {
            devdFilter->hasServiceUuidMask = true;
            CopyUuidData(filter.GetServiceUuidMask(), devdFilter->serviceUuidMask);
        }
        if (filter.HasSolicitationUuid()) {
            devdFilter->hasSolicitationUuid = true;
            CopyUuidData(filter.GetServiceSolicitationUuid(), devdFilter->serviceSolicitationUuid);
        }
        if (filter.HasSolicitationUuidMask()) {
            devdFilter->hasSolicitationUuidMask = true;
            CopyUuidData(filter.GetServiceSolicitationUuidMask(), devdFilter->serviceSolicitationUuidMask);
        }
        if (filter.HasServiceData()) {
            CopyVectorToAllocData(filter.GetServiceData(), devdFilter->serviceData);
        }
        if (filter.HasServiceDataMask()) {
            CopyVectorToAllocData(filter.GetServiceDataMask(), devdFilter->serviceDataMask);
        }
        CopyVectorToAllocData(filter.GetManufactureData(), devdFilter->manufacturerData);
        CopyVectorToAllocData(filter.GetManufactureDataMask(), devdFilter->manufacturerDataMask);
        devdFilter->manufacturerId = filter.GetManufacturerId();
        devdFilter->hasRssiThreshold = filter.HasRssiThreshold();
        devdFilter->rssiThreshold = filter.GetRssiThreshold();
        devdFilter->isSensorHubChannel = filter.IsSensorHubChannel();
        devdFilter->advIndReport = filter.GetAdvIndReport();
    }

    static void FreeDevdFilter(NLSTK_DevdScanFilter_S *filter)
    {
        if (filter == NULL) {
            return;
        }
        if (filter->name.data != NULL) {
            delete[] filter->name.data;
            filter->name.data = nullptr;
        }
        if (filter->serviceData.data != NULL) {
            delete[] filter->serviceData.data;
            filter->serviceData.data = nullptr;
        }
        if (filter->serviceDataMask.data != NULL) {
            delete[] filter->serviceDataMask.data;
            filter->serviceDataMask.data = nullptr;
        }
        if (filter->manufacturerData.data != NULL) {
            delete[] filter->manufacturerData.data;
            filter->manufacturerData.data = nullptr;
        }
        if (filter->manufacturerDataMask.data != NULL) {
            delete[] filter->manufacturerDataMask.data;
            filter->manufacturerDataMask.data = nullptr;
        }
    }
};
}  // namespace Sle
}  // namespace OHOS
#endif // SCAN_UTILS_H