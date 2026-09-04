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

#include "ohos.nearlink.scan.proj.hpp"
#include "ohos.nearlink.scan.impl.hpp"
#include "ani_nearlink_scan_callback.h"
#include "ani_nearlink_error.h"
#include "log.h"
#include "log_util.h"
#include "nearlink_errorcode.h"
#include "nearlink_host.h"
#include "nearlink_sle_scanner.h"
#include "nearlink_utils.h"
#include "ani_nearlink_utils.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

namespace OHOS {
namespace Nearlink {
const int SCAN_DURATION_MIN = 10;
const int SCAN_DURATION_MAX = 60;
constexpr int32_t SCAN_RSSI_MIN = -128;
constexpr int32_t SCAN_RSSI_MAX = 127;

enum class ScanDuty {
    SCAN_MODE_LOW_POWER = 0,   // low power mode */
    SCAN_MODE_BALANCED = 1,    // balanced power mode
    SCAN_MODE_LOW_LATENCY = 2  // Scan using highest duty cycle
};

std::shared_ptr<SleCentralManager> g_sleCentralManager = nullptr;
std::shared_ptr<AniNearlinkScanCallback> g_scanCallback = nullptr;
std::mutex g_scanManagerMutex;

std::shared_ptr<SleCentralManager> GetSleCentralManager()
{
    std::lock_guard<std::mutex> lock(g_scanManagerMutex);
    if (g_sleCentralManager == nullptr) {
        g_scanCallback = std::make_shared<AniNearlinkScanCallback>();
        g_sleCentralManager = SleCentralManager::CreateSleCentralManager(g_scanCallback);
    }
    return g_sleCentralManager;
}

static bool isFilterEmpty(const SleScanFilter &filter)
{
    if (!filter.GetDeviceId().empty()) {
        return false;
    } else if (!filter.GetName().empty()) {
        return false;
    } else if (filter.GetManufacturerId() > 0) {
        return false;
    } else if (!filter.GetManufactureData().empty()) {
        return false;
    } else if (!filter.GetManufactureDataMask().empty()) {
        return false;
    } else if (filter.HasRssiThreshold()) {
        return false;
    } else if (filter.HasServiceData()) {
        return false;
    } else if (filter.HasServiceDataMask()) {
        return false;
    } else {
        return true;
    }
}

static TaiheStatus ParseScanFilterDeviceIdParameters(::ohos::nearlink::scan::ScanFilters &filter,
    SleScanFilter &sleScanFilter)
{
    if (filter.address.has_value()) {
        std::string deviceId = std::string(filter.address.value());
        if (!IsValidAddress(deviceId)) {
            HILOGE("invalid deviceId");
            HandleSyncErr(NL_ERR_INVALID_ADDRESS);
            return TAIHE_INVALID_ARG;
        }
        sleScanFilter.SetDeviceId(deviceId);
    }
    return TAIHE_OK;
}

static TaiheStatus ParseScanFilterLocalNameParameters(::ohos::nearlink::scan::ScanFilters &filter,
    SleScanFilter &sleScanFilter)
{
    if (filter.deviceName.has_value()) {
        std::string name = std::string(filter.deviceName.value());
        if (name.empty()) {
            HILOGE("name is empty");
            return TAIHE_INVALID_ARG;
        }
        sleScanFilter.SetName(name);
    }
    return TAIHE_OK;
}

static TaiheStatus ParseScanFilterManufactureDataParameters(::ohos::nearlink::scan::ScanFilters &filter,
    SleScanFilter &sleScanFilter)
{
    bool hasManufacturerId = false;
    std::vector<uint8_t> data {};
    std::vector<uint8_t> mask {};
    if (filter.manufacturerId.has_value()) {
        hasManufacturerId = true;
        sleScanFilter.SetManufacturerId(static_cast<uint16_t>(filter.manufacturerId.value()));
    }
    if (filter.manufacturerData.has_value()) {
        data.assign(filter.manufacturerData.value().begin(), filter.manufacturerData.value().end());
    }
    if (filter.manufacturerDataMask.has_value()) {
        mask.assign(filter.manufacturerDataMask.value().begin(), filter.manufacturerDataMask.value().end());
    }
    if (!data.empty() && !hasManufacturerId) {
        HILOGE("invalid manufacturerId");
        return TAIHE_INVALID_ARG;
    }
    if (!mask.empty()) {
        if (data.empty()) {
            HILOGE("manufacturerData is empty while manufacturerDataMask is not empty");
            return TAIHE_INVALID_ARG;
        }
        if (data.size() != mask.size()) {
            HILOGE("size mismatch for manufacturerData and manufacturerDataMask");
            return TAIHE_INVALID_ARG;
        }
    }
    sleScanFilter.SetManufactureData(std::move(data));
    sleScanFilter.SetManufactureDataMask(std::move(mask));
    return TAIHE_OK;
}

static TaiheStatus ParseScanFilterRssiParameters(::ohos::nearlink::scan::ScanFilters &filter,
    SleScanFilter &sleScanFilter)
{
    if (filter.rssi.has_value()) {
        auto rssiValue = filter.rssi.value();
        if (rssiValue < SCAN_RSSI_MIN || rssiValue > SCAN_RSSI_MAX) {
            HILOGE("rssi should in range of -128 to 127, inclusive.");
            HandleSyncErr(NL_ERR_INVALID_INTERGER);
            return TAIHE_INVALID_ARG;
        }
        sleScanFilter.SetRssiThreshold(static_cast<int8_t>(rssiValue));
    }
    return TAIHE_OK;
}

static TaiheStatus ParseScanFilter(::ohos::nearlink::scan::ScanFilters &filter, SleScanFilter &sleScanFilter)
{
    TAIHE_NEARLINK_CALL_RETURN(ParseScanFilterDeviceIdParameters(filter, sleScanFilter));
    TAIHE_NEARLINK_CALL_RETURN(ParseScanFilterLocalNameParameters(filter, sleScanFilter));
    TAIHE_NEARLINK_CALL_RETURN(ParseScanFilterManufactureDataParameters(filter, sleScanFilter));
    TAIHE_NEARLINK_CALL_RETURN(ParseScanFilterRssiParameters(filter, sleScanFilter));
    return TAIHE_OK;
}

static TaiheStatus ParseScanFilterParameters(::taihe::array<::ohos::nearlink::scan::ScanFilters> filters,
    std::vector<SleScanFilter> &outScanFilters)
{
    if (filters.size() == 0) {
        HILOGE("Requires array length > 0");
        HandleSyncErr(NL_ERR_EMPTY_ARRAY);
        return TAIHE_INVALID_ARG;
    }
    for (auto &filter : filters) {
        SleScanFilter sleScanFilter;
        TAIHE_NEARLINK_CALL_RETURN(ParseScanFilter(filter, sleScanFilter));
        if (!isFilterEmpty(sleScanFilter)) {
            outScanFilters.push_back(sleScanFilter);
        }
    }
    if (outScanFilters.empty()) {
        HILOGE("Filters are all empty");
        HandleSyncErr(NL_ERR_EMPTY_ARRAY);
        return TAIHE_INVALID_ARG;
    }
    return TAIHE_OK;
}

static TaiheStatus CheckFilterScanParams(::ohos::nearlink::scan::Filters const& filters,
    ::taihe::optional<::ohos::nearlink::scan::ScanOptions> const &options, std::vector<SleScanFilter> &outScanFilters,
    SleScanSettings &outSettings)
{
    if (filters.get_tag() == ::ohos::nearlink::scan::Filters::tag_t::type_null) {
        SleScanFilter emptyFilter;
        outScanFilters.push_back(emptyFilter);
    } else {
        TAIHE_NEARLINK_CALL_RETURN(ParseScanFilterParameters(filters.get_type_Filters_ref(), outScanFilters));
    }

    if (options.has_value()) {
        auto scanOptions = options.value();
        if (scanOptions.scanMode.has_value()) {
            int32_t scanMode = static_cast<int32_t>(scanOptions.scanMode.value().get_value());
            if (scanMode < static_cast<int32_t>(ScanDuty::SCAN_MODE_LOW_POWER) || 
                scanMode > static_cast<int32_t>(ScanDuty::SCAN_MODE_LOW_LATENCY)) {
                HILOGE("Invalid scan dutyMode: %{public}d", scanMode);
                return TAIHE_INVALID_ARG;
            }
            outSettings.SetScanMode(scanMode);
        }
        if (scanOptions.duration.has_value()) {
            int32_t duration = scanOptions.duration.value();
            if (duration < SCAN_DURATION_MIN || duration > SCAN_DURATION_MAX) {
                HandleSyncErr(NL_ERR_INVALID_INTERGER);
                HILOGE("duration should in range of 10 to 60 (sec), inclusive.");
                return TAIHE_INVALID_ARG;
            }
            outSettings.SetDuration(duration);
        }
    }
    return TAIHE_OK;
}

void StartScan(
    ::ohos::nearlink::scan::Filters const& filters,
    ::taihe::optional<::ohos::nearlink::scan::ScanOptions> const &options)
{
    HILOGI("enter");
    std::vector<SleScanFilter> scanFilters;
    SleScanSettings nativeSettings;
    auto status = CheckFilterScanParams(filters, options, scanFilters, nativeSettings);
    ANI_NL_ASSERT_RETURN_VOID(status == TAIHE_OK, NL_ERR_INVALID_PARAM);

    // 提前校验ACCESS权限
    bool isGranted = false;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    ANI_NL_ASSERT_RETURN_VOID(checkResult == NL_NO_ERROR, checkResult);
    ANI_NL_ASSERT_RETURN_VOID(isGranted, NL_ERR_PERMISSION_FAILED);

    auto manager = GetSleCentralManager();
    NlErrCode err = manager->StartScanWithFilter(nativeSettings, scanFilters);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void StopScan()
{
    HILOGI("enter");
    // 提前校验ACCESS权限
    bool isGranted = false;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    ANI_NL_ASSERT_RETURN_VOID(checkResult == NL_NO_ERROR, checkResult);
    ANI_NL_ASSERT_RETURN_VOID(isGranted, NL_ERR_PERMISSION_FAILED);

    auto manager = GetSleCentralManager();
    NlErrCode err = manager->StopScan();
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void OnDeviceFound(::taihe::callback_view<void(::taihe::array_view<::ohos::nearlink::scan::ScanResults> data)> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto manager = GetSleCentralManager();
    g_scanCallback->eventSubscribe_.RegisterEvent(callback);
}

void OffDeviceFound(
    taihe::optional_view<taihe::callback<void(::taihe::array_view<::ohos::nearlink::scan::ScanResults> data)>> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto manager = GetSleCentralManager();
    g_scanCallback->eventSubscribe_.DeregisterEvent(callback);
}
} // namespace Nearlink
} // namespace OHOS

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_StartScan(OHOS::Nearlink::StartScan);
TH_EXPORT_CPP_API_StopScan(OHOS::Nearlink::StopScan);
TH_EXPORT_CPP_API_OnDeviceFound(OHOS::Nearlink::OnDeviceFound);
TH_EXPORT_CPP_API_OffDeviceFound(OHOS::Nearlink::OffDeviceFound);
// NOLINTEND