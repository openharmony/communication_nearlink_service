/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_NEARLINK_STANDARD_SLE_CENTRAL_MANAGER_PROXY_H
#define OHOS_NEARLINK_STANDARD_SLE_CENTRAL_MANAGER_PROXY_H

#include "i_nearlink_sle_central_manager.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSleCentralManagerProxy : public IRemoteProxy<INearlinkSleCentralManager> {
public:
    NearlinkSleCentralManagerProxy() = delete;
    explicit NearlinkSleCentralManagerProxy(const sptr<IRemoteObject> &impl);
    ~NearlinkSleCentralManagerProxy() override;
    DISALLOW_COPY_AND_MOVE(NearlinkSleCentralManagerProxy);

    NlErrCode RegisterSleCentralManagerCallback(uint32_t &scannerId, bool enableRandomAddrMode,
        const sptr<INearlinkSleCentralManagerCallback> &callback) override;
    NlErrCode DeregisterSleCentralManagerCallback(uint32_t scannerId,
        const sptr<INearlinkSleCentralManagerCallback> &callback) override;
    NlErrCode StartScanWithFilter(uint32_t scannerId, const NearlinkSleScanSettings &settings,
        const std::vector<NearlinkSleScanFilter> &filters) override;
    NlErrCode StartFullScan(uint32_t scannerId, const NearlinkSleScanSettings &settings) override;
    NlErrCode StopScan(uint32_t scannerId) override;

private:
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
    static inline BrokerDelegator<NearlinkSleCentralManagerProxy> delegator_;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // OHOS_NEARLINK_STANDARD_SLE_CENTRAL_MANAGER_PROXY_H