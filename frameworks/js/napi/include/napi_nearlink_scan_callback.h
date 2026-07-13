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

#ifndef NAPI_NEARLINK_CENTRAL_MANAGER_CALLBACK_H
#define NAPI_NEARLINK_CENTRAL_MANAGER_CALLBACK_H

#include "nearlink_sle_scanner.h"
#include "napi_async_callback.h"
#include "napi_nearlink_utils.h"
#include "napi_event_subscribe_module.h"

namespace OHOS {
namespace Nearlink {
const char *const  REGISTER_SCAN_DEVICE_NAME = "deviceFound";
class NapiNearlinkCentralManagerCallback : public SleCentralManagerCallback {
public:
    NapiNearlinkCentralManagerCallback();
    ~NapiNearlinkCentralManagerCallback() override = default;

    static std::shared_ptr<NapiNearlinkCentralManagerCallback> GetInstance(void);

    void OnScanCallback(const SleScanResult &result) override;
    void OnSleBatchScanResultsEvent(const std::vector<SleScanResult> &results) override;
    void OnStartOrStopScanEvent(int resultCode, bool isStartScan) override;

    NapiEventSubscribeModule eventSubscribe;
private:
    std::mutex callbackMutex_ {};
};

void ConvertScanResult(const std::vector<SleScanResult> &results, const napi_env &env, napi_value &scanResultArray);

class NapiNativeScanResult : public NapiNativeObject {
public:
    NapiNativeScanResult(const std::vector<SleScanResult> &results) : scanResult_(results) {}
    ~NapiNativeScanResult() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    std::vector<SleScanResult> scanResult_ {};
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NAPI_NEARLINK_CENTRAL_MANAGER_CALLBACK_H
