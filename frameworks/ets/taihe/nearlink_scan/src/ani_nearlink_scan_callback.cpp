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

AniNearlinkScanCallback::AniNearlinkScanCallback()
{}

std::shared_ptr<AniNearlinkScanCallback> AniNearlinkScanCallback::GetInstance()
{
    static std::shared_ptr<AniNearlinkScanCallback> instance =
        std::make_shared<AniNearlinkScanCallback>();
    return instance;
}

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

    eventSubscribe_.PublishEvent(scanArray);
}

void AniNearlinkScanCallback::OnSleBatchScanResultsEvent(const std::vector<SleScanResult> &results)
{
    HILOGE("not implement");
}

void AniNearlinkScanCallback::OnStartOrStopScanEvent(int resultCode, bool isStartScan)
{
    HILOGE("not implement");
}

} // namespace Nearlink
} // namespace OHOS
