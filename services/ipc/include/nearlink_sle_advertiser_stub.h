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

#ifndef OHOS_NEARLINK_STANDARD_SLE_ADVERTISER_STUB_H
#define OHOS_NEARLINK_STANDARD_SLE_ADVERTISER_STUB_H

#include <map>

#include "i_nearlink_sle_advertiser.h"
#include "iremote_stub.h"
#include "nearlink_permission_item.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSleAdvertiserStub : public IRemoteStub<INearlinkSleAdvertiser> {
public:
    NearlinkSleAdvertiserStub();
    virtual ~NearlinkSleAdvertiserStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    using NearlinkAdvertiserFunc = int32_t (*)(NearlinkSleAdvertiserStub *stub, MessageParcel &data,
        MessageParcel &reply);
    using NearlinkAdvertiserFuncPerm = std::pair<NearlinkAdvertiserFunc, std::shared_ptr<NearLinkPermissionItem>>;

private:
    static int32_t RegisterSleAdvertiserCallbackInner(NearlinkSleAdvertiserStub *stub, MessageParcel &data,
        MessageParcel &reply);
    static int32_t DeregisterSleAdvertiserCallbackInner(NearlinkSleAdvertiserStub *stub, MessageParcel &data,
        MessageParcel &reply);
    static int32_t StartAdvertisingInner(NearlinkSleAdvertiserStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t StopAdvertisingInner(NearlinkSleAdvertiserStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetAdvertiserHandleInner(NearlinkSleAdvertiserStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t SetAdvertisingDataInner(NearlinkSleAdvertiserStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t EnableAdvertisingInner(NearlinkSleAdvertiserStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t DisableAdvertisingInner(NearlinkSleAdvertiserStub *stub, MessageParcel &data, MessageParcel &reply);

private:
    std::map<uint32_t, NearlinkAdvertiserFuncPerm> memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkSleAdvertiserStub);
};
}  // namespace Nearlink
}  // namespace OHOS

#endif