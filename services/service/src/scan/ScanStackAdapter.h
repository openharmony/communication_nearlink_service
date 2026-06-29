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

#ifndef SCAN_STACK_ADAPTER_H
#define SCAN_STACK_ADAPTER_H

#include <vector>
#include <future>
#include "BaseDef.h"
#include "nlstk_scan_api.h"
#include "SleDefs.h"
#include "interface_scan_service.h"
#include "log.h"
#include "log_util.h"
#include "sle_uuid.h"
#include "ThreadUtil.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief SLE scan manager.
 */
class ScanStackAdapter{
public:
    ScanStackAdapter();
    ~ScanStackAdapter();

    static ScanStackAdapter &GetInstance();
    void RegisterSleScanCallbackToStack();
    uint32_t  AllocScannerId();
    void RemoveScannerId(uint32_t scannerId);
    void StartScan(uint32_t scannerId, const NearlinkSleScanSettings &settings,
        const std::vector<SleScanFilterImpl> &filters);
    void StopScan(uint32_t scannerId);
    void StopAllScan(std::shared_ptr<std::promise<void>> &promise);

private:
    static void OnStartOrStopScanEventForStack(NLSTK_Errcode_E resultCode, bool isStartScan);
    static void OnScanCallbackForStack(NLSTK_DevdAdvResult_S *result);
    SLE_DISALLOW_COPY_AND_ASSIGN(ScanStackAdapter);
    DECLARE_IMPL();
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // SCAN_STACK_ADAPTER_H
