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

#ifndef OHOS_NEARLINK_STANDARD_SLE_ADVERTISER_PROXY_H
#define OHOS_NEARLINK_STANDARD_SLE_ADVERTISER_PROXY_H

#include "i_nearlink_sle_advertiser.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSleAdvertiserProxy : public IRemoteProxy<INearlinkSleAdvertiser> {
public:
    NearlinkSleAdvertiserProxy() = delete;
    explicit NearlinkSleAdvertiserProxy(const sptr<IRemoteObject> &impl);
    ~NearlinkSleAdvertiserProxy() override;
    DISALLOW_COPY_AND_MOVE(NearlinkSleAdvertiserProxy);

    NlErrCode RegisterSleAdvertiserCallback(const sptr<INearlinkSleAdvertiseCallback> &callback) override;
    NlErrCode DeregisterSleAdvertiserCallback(const sptr<INearlinkSleAdvertiseCallback> &callback) override;
    NlErrCode StartAdvertising(const NearlinkSleAdvertiserSettings &settings, const NearlinkSleAdvertiserData &advData,
        const NearlinkSleAdvertiserData &scanResponse, int32_t advHandle) override;
    NlErrCode StopAdvertising(int32_t advHandle) override;
    NlErrCode GetAdvertiserHandle(int32_t &advHandle) override;
    NlErrCode SetAdvertisingData(const NearlinkSleAdvertiserData &advData,
        const NearlinkSleAdvertiserData &scanResponse, int32_t advHandle) override;
    NlErrCode EnableAdvertising(int32_t advHandle) override;
    NlErrCode DisableAdvertising(int32_t advHandle) override;

private:
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
    static inline BrokerDelegator<NearlinkSleAdvertiserProxy> delegator_;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif