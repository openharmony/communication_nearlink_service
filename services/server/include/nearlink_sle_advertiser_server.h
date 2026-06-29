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

#ifndef OHOS_NEARLINK_STANDARD_SLE_ADVERTISER_SERVER_H
#define OHOS_NEARLINK_STANDARD_SLE_ADVERTISER_SERVER_H

#include "nearlink_sle_advertiser_stub.h"
#include "nearlink_types.h"
#include "i_nearlink_sle_advertiser.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability.h"
#include "nearlink_errorcode.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSleAdvertiserServer : public NearlinkSleAdvertiserStub {
public:
    NearlinkSleAdvertiserServer();
    ~NearlinkSleAdvertiserServer() override;

    virtual NlErrCode RegisterSleAdvertiserCallback(const sptr<INearlinkSleAdvertiseCallback> &callback) override;
    virtual NlErrCode DeregisterSleAdvertiserCallback(const sptr<INearlinkSleAdvertiseCallback> &callback) override;
    virtual NlErrCode StartAdvertising(const NearlinkSleAdvertiserSettings &settings,
        const NearlinkSleAdvertiserData &advData, const NearlinkSleAdvertiserData &scanResponse,
        int32_t advHandle) override;
    virtual NlErrCode StopAdvertising(int32_t advHandle) override;
    virtual NlErrCode GetAdvertiserHandle(int32_t &advHandle) override;
    NlErrCode SetAdvertisingData(const NearlinkSleAdvertiserData &advData,
        const NearlinkSleAdvertiserData &scanResponse, int32_t advHandle) override;
    virtual NlErrCode EnableAdvertising(int32_t advHandle) override;
    virtual NlErrCode DisableAdvertising(int32_t advHandle) override;

private:
    void SetDeviceName(SleAdvertiserDataImpl &sleAdvertiserData);
    struct impl;
    std::shared_ptr<impl> pimpl;
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkSleAdvertiserServer);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_SLE_ADVERTISER_SERVER_H