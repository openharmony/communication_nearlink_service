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

#ifndef OHOS_NEARLINK_STANDARD_SLE_CENTRAL_MANAGER_INTERFACE_H
#define OHOS_NEARLINK_STANDARD_SLE_CENTRAL_MANAGER_INTERFACE_H

#include "nearlink_sle_scanner.h"
#include "nearlink_sle_scan_settings.h"
#include "nearlink_sle_scan_filter.h"
#include "i_nearlink_sle_central_manager_callback.h"
#include "nearlink_service_ipc_interface_code.h"
#include "iremote_broker.h"

namespace OHOS {
namespace Nearlink {
namespace {
const std::string SLE_CENTRAL_MANAGER_SERVER = "SleCentralManagerServer";
}  // namespace

class INearlinkSleCentralManager : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkSleCentralManager");
    virtual NlErrCode RegisterSleCentralManagerCallback(uint32_t &scannerId, bool enableRandomAddrMode,
        const sptr<INearlinkSleCentralManagerCallback> &callback) = 0;
    virtual NlErrCode DeregisterSleCentralManagerCallback(uint32_t scannerId,
        const sptr<INearlinkSleCentralManagerCallback> &callback) = 0;
    virtual NlErrCode StartScanWithFilter(uint32_t scannerId, const NearlinkSleScanSettings &settings,
        const std::vector<NearlinkSleScanFilter> &filters) = 0;
    virtual NlErrCode StartFullScan(uint32_t scannerId, const NearlinkSleScanSettings &settings) = 0;
    virtual NlErrCode StopScan(uint32_t scannerId) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_SLE_CENTRAL_MANAGER_INTERFACE_H