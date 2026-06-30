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

#ifndef OHOS_NEARLINK_STANDARD_SLE_ADVERTISER_INTERFACE_H
#define OHOS_NEARLINK_STANDARD_SLE_ADVERTISER_INTERFACE_H

#include "nearlink_sle_advertiser_data.h"
#include "nearlink_sle_advertiser_settings.h"
#include "nearlink_service_ipc_interface_code.h"
#include "i_nearlink_sle_advertise_callback.h"
#include "iremote_broker.h"
#include "nearlink_errorcode.h"

namespace OHOS {
namespace Nearlink {
namespace {
const std::string SLE_ADVERTISER_SERVER = "SleAdvertiserServer";
}  // namespace

class INearlinkSleAdvertiser : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkSleAdvertiser");

    virtual NlErrCode RegisterSleAdvertiserCallback(const sptr<INearlinkSleAdvertiseCallback> &callback) = 0;
    virtual NlErrCode DeregisterSleAdvertiserCallback(const sptr<INearlinkSleAdvertiseCallback> &callback) = 0;
    virtual NlErrCode StartAdvertising(const NearlinkSleAdvertiserSettings &settings,
        const NearlinkSleAdvertiserData &advData, const NearlinkSleAdvertiserData &scanResponse, int32_t advHandle) = 0;
    virtual NlErrCode StopAdvertising(int32_t advHandle) = 0;
    virtual NlErrCode GetAdvertiserHandle(int32_t &advHandle) = 0;
    virtual NlErrCode SetAdvertisingData(const NearlinkSleAdvertiserData &advData,
        const NearlinkSleAdvertiserData &scanResponse, int32_t advHandle) = 0;
    virtual NlErrCode EnableAdvertising(int32_t advHandle) = 0;
    virtual NlErrCode DisableAdvertising(int32_t advHandle) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_SLE_ADVERTISER_INTERFACE_H