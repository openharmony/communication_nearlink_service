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

#include "ani_nearlink_scan_callback.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

AniNearlinkScanCallback::AniNearlinkScanCallback() = default;

::ohos::nearlink::scan::ScanResults ConvertToScanResult(const SleScanResult &result)
{
    auto& r = const_cast<SleScanResult&>(result);
    const std::vector<uint8_t> payload = r.GetPayload();

    ::ohos::nearlink::scan::ScanResults scanResults = {};
    scanResults.address = static_cast<::taihe::string>(r.GetPeripheralDevice().GetDeviceAddr());
    scanResults.rssi = r.GetRssi();
    scanResults.deviceName = static_cast<::taihe::string>(r.GetName());
    scanResults.isConnectable = r.IsConnectable();
    scanResults.data = ::taihe::array<uint8_t>(::taihe::copy_data_t{}, payload.data(), payload.size());
    return scanResults;
}

void AniNearlinkScanCallback::OnScanCallback(const SleScanResult &result)
{
    HILOGI("enter");
    ::ohos::nearlink::scan::ScanResults scanResult = ConvertToScanResult(result);
    std::vector<::ohos::nearlink::scan::ScanResults> scanResultVec = {scanResult};
    ::taihe::array<::ohos::nearlink::scan::ScanResults> scanArray(
        ::taihe::copy_data_t{}, scanResultVec.data(), scanResultVec.size());

    auto scanResultEvent = eventSubscribe_.GetCallbacks();
    for (auto callback : scanResultEvent) {
        if (callback.has_value()) {
            (*callback)(scanArray);
        }
    }
}

void AniNearlinkScanCallback::OnSleBatchScanResultsEvent(const std::vector<SleScanResult> &results)
{
    HILOGI("enter");
    std::vector<::ohos::nearlink::scan::ScanResults> scanResultVec;
    scanResultVec.reserve(results.size());
    for (auto &result : results) {
        scanResultVec.emplace_back(ConvertToScanResult(result));
    }
    ::taihe::array<::ohos::nearlink::scan::ScanResults> scanArray(
        ::taihe::copy_data_t{}, scanResultVec.data(), scanResultVec.size());

    auto sleScanResult = eventSubscribe_.GetCallbacks();
    for (auto callback : sleScanResult) {
        if (callback.has_value()) {
            (*callback)(scanArray);
        }
    }
}

void AniNearlinkScanCallback::OnStartOrStopScanEvent(int resultCode, bool isStartScan)
{
    HILOGI("resultCode: %{public}d, isStartScan: %{public}d", resultCode, isStartScan);
}

} // namespace Nearlink
} // namespace OHOS
