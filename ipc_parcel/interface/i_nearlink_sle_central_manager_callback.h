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

#ifndef OHOS_NEARLINK_STANDARD_SLE_CENTRAL_MANAGER_CALLBACK_INTERFACE_H
#define OHOS_NEARLINK_STANDARD_SLE_CENTRAL_MANAGER_CALLBACK_INTERFACE_H

#include "nearlink_sle_scan_result.h"
#include "sle_service_data.h"
#include "nearlink_service_ipc_interface_code.h"
#include "iremote_broker.h"

namespace OHOS {
namespace Nearlink {
class INearlinkSleCentralManagerCallback : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkSleCentralManagerCallback");

    virtual void OnScanCallback(const NearlinkSleScanResult &result) = 0;
    virtual void OnSleBatchScanResultsEvent(std::vector<NearlinkSleScanResult> &results) = 0;
    virtual void OnStartOrStopScanEvent(int resultCode, bool isStartScan) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_SLE_CENTRAL_MANAGER_CALLBACK_INTERFACE_H