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

#ifndef OHOS_NEARLINK_STANDARD_SLE_CENTRAL_MANAGER_SERVER_H
#define OHOS_NEARLINK_STANDARD_SLE_CENTRAL_MANAGER_SERVER_H

#include <cstdint>
#include <mutex>
#include <set>

#include "nearlink_sle_central_manager_stub.h"
#include "nearlink_types.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability.h"
#include "SleMultiScanData.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSleCentralManagerServer : public NearlinkSleCentralManagerStub {
public:
    NearlinkSleCentralManagerServer();
    ~NearlinkSleCentralManagerServer() override;

    NlErrCode RegisterSleCentralManagerCallback(uint32_t &scannerId, bool enableRandomAddrMode,
        const sptr<INearlinkSleCentralManagerCallback> &callback) override;
    NlErrCode DeregisterSleCentralManagerCallback(uint32_t scannerId,
        const sptr<INearlinkSleCentralManagerCallback> &callback) override;
    NlErrCode StartScanWithFilter(uint32_t scannerId, const NearlinkSleScanSettings &settings,
        const std::vector<NearlinkSleScanFilter> &filters) override;
    NlErrCode StartFullScan(uint32_t scannerId, const NearlinkSleScanSettings &settings) override;
    NlErrCode StopScan(uint32_t scannerId) override;

private:
    NlErrCode StartScan(uint32_t scannerId, const NearlinkSleScanSettings &settings,
        const std::vector<NearlinkSleScanFilter> &filters);
    void ConvertToScanFilterImpl(const std::vector<NearlinkSleScanFilter> &filters,
        std::vector<SleScanFilterImpl> &filterImpls);
    NEARLINK_DECLARE_IMPL();
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkSleCentralManagerServer);
    std::vector<NearlinkSleScanFilter> SetRssiFilter(const std::vector<NearlinkSleScanFilter>& filters);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_SLE_CENTRAL_MANAGER_SERVER_H