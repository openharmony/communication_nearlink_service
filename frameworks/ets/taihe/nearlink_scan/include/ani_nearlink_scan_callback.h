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

#ifndef ANI_NEARLINK_SCAN_CALLBACK_H
#define ANI_NEARLINK_SCAN_CALLBACK_H

#include <memory>
#include "ohos.nearlink.scan.proj.hpp"
#include "ohos.nearlink.scan.impl.hpp"
#include "taihe/runtime.hpp"
#include "ani_event_module.h"
#include "nearlink_sle_scanner.h"

namespace OHOS {
namespace Nearlink {

class AniNearlinkScanCallback : public SleCentralManagerCallback {
public:
    AniNearlinkScanCallback();
    ~AniNearlinkScanCallback() override = default;

    static std::shared_ptr<AniNearlinkScanCallback> GetInstance();

    void OnScanCallback(const SleScanResult &result) override;
    void OnSleBatchScanResultsEvent(const std::vector<SleScanResult> &results) override;
    void OnStartOrStopScanEvent(int resultCode, bool isStartScan) override;

    EventModule<void(::taihe::array_view<::ohos::nearlink::scan::ScanResults> data)> eventSubscribe_;
};

} // namespace Nearlink
} // namespace OHOS

#endif // ANI_NEARLINK_SCAN_CALLBACK_H
