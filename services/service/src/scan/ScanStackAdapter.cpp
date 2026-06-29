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

#include "ScanStackAdapter.h"
#include <dlfcn.h>
#include "ScanService.h"
#include "SleUtils.h"
#include "ScanUtils.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr uint8_t INVALID_MODULE_ID = 0xFF;
}

struct ScanStackAdapter::impl {
    impl(ScanStackAdapter &ScanStackAdapter);
    ~impl() = default;
    uint8_t moduleId = INVALID_MODULE_ID;
    std::shared_ptr<std::promise<void>> stopAllScanPromise_ {nullptr};
};

ScanStackAdapter::impl::impl(ScanStackAdapter &ScanStackAdapter)
{}

ScanStackAdapter::ScanStackAdapter(): pimpl(std::make_unique<ScanStackAdapter::impl>(*this))
{
    HILOGI("[ScanStackAdapter]");
}

ScanStackAdapter::~ScanStackAdapter()
{
    NLSTK_DevdDeregScanModule(pimpl->moduleId);
    HILOGI("[ScanStackAdapter]");
}

ScanStackAdapter &ScanStackAdapter::GetInstance()
{
    // C++11 static local variable initialization is thread-safe.
    static ScanStackAdapter scanStackAdapter;
    return scanStackAdapter;
}

void ScanStackAdapter::RegisterSleScanCallbackToStack()
{
    HILOGI("[ScanStackAdapter]");
    NLSTK_DevdScanCbk_S callbacks = {
        .onStartOrStopEvent = &ScanStackAdapter::OnStartOrStopScanEventForStack,
        .onScanCallback = &ScanStackAdapter::OnScanCallbackForStack,
    };
    uint8_t temModuleId = INVALID_MODULE_ID;
    if (NLSTK_DevdRegScanModule(&temModuleId, &callbacks) == NLSTK_ERRCODE_SUCCESS) {
        HILOGI("moduleId is %{public}d", temModuleId);
        pimpl->moduleId = temModuleId;
    }
}

void ScanStackAdapter::OnStartOrStopScanEventForStack(NLSTK_Errcode_E resultCode, bool isStartScan)
{
    DoInScanThread([resultCode, isStartScan]() -> void {
        ScanStackAdapter &adapter = ScanStackAdapter::GetInstance();
        if (!isStartScan && adapter.pimpl->stopAllScanPromise_) {
            HILOGI("stop all scan finished");
            adapter.pimpl->stopAllScanPromise_->set_value();
            adapter.pimpl->stopAllScanPromise_ = nullptr;
        }
        ScanService::GetInstance().NotifyStartOrStopScanEvent(resultCode, isStartScan);
    });
}

void ScanStackAdapter::OnScanCallbackForStack(NLSTK_DevdAdvResult_S *result)
{
    NL_CHECK_RETURN(result, "result is null");
    ScanService::GetInstance().NotifyScanResults(result);
}

uint32_t ScanStackAdapter::AllocScannerId()
{
    NL_CHECK_RETURN_RET(pimpl->moduleId != INVALID_MODULE_ID, SLE_SCAN_INVALID_ID, "module id is invalid");
    uint32_t scannerId = SLE_SCAN_INVALID_ID;
    NLSTK_DevdAllocScannerId(pimpl->moduleId, &scannerId);
    return scannerId;
}

void ScanStackAdapter::RemoveScannerId(uint32_t scannerId)
{
    NLSTK_DevdRemoveScannerId(scannerId);
}

void ScanStackAdapter::StartScan(uint32_t scannerId, const NearlinkSleScanSettings &settings,
    const std::vector<SleScanFilterImpl> &filters)
{
    NLSTK_DevdScanSetting_S devdSettings = {};
    ScanUtils::ConvertScanSettings(settings, devdSettings);
    uint16_t filterNum = filters.size();
    NLSTK_DevdScanFilter_S *devdFilter = nullptr;
    if (filterNum > 0) {
        devdFilter = new NLSTK_DevdScanFilter_S[filterNum];
        NL_CHECK_RETURN(devdFilter, "devdFilter is null");
        (void)memset_s(devdFilter, sizeof(NLSTK_DevdScanFilter_S) * filterNum, 0x00,
            sizeof(NLSTK_DevdScanFilter_S) * filterNum);
        for (int i = 0; i < filterNum; ++i) {
            ScanUtils::CreatDevdFilter(filters[i], &devdFilter[i]);
        }
    }
    NLSTK_DevdStartScan(scannerId, &devdSettings, devdFilter, filterNum);
    for (int i = 0; i < filterNum; ++i) {
        ScanUtils::FreeDevdFilter(&devdFilter[i]);
    }
    if (devdFilter != nullptr) {
        delete[] devdFilter;
        devdFilter = nullptr;
    }
}

void ScanStackAdapter::StopScan(uint32_t scannerId)
{
    NLSTK_DevdStopScan(scannerId);
}

void ScanStackAdapter::StopAllScan(std::shared_ptr<std::promise<void>> &promise)
{
    pimpl->stopAllScanPromise_ = promise;
    if (NLSTK_DevdStopAllScan() != NLSTK_ERRCODE_SUCCESS) {
        pimpl->stopAllScanPromise_->set_value();
        pimpl->stopAllScanPromise_ = nullptr;
    }
}
}  // namespace Sle
}  // namespace OHOS
