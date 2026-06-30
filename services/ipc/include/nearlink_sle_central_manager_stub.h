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

#ifndef OHOS_NEARLINK_STANDARD_SLE_CENTRAL_MANAGER_STUB_H
#define OHOS_NEARLINK_STANDARD_SLE_CENTRAL_MANAGER_STUB_H

#include <map>

#include "i_nearlink_sle_central_manager.h"
#include "iremote_stub.h"
#include "nearlink_permission_item.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSleCentralManagerStub : public IRemoteStub<INearlinkSleCentralManager> {
public:
    NearlinkSleCentralManagerStub();
    virtual ~NearlinkSleCentralManagerStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    using NearlinkCentralManagerFunc = int32_t (*)(NearlinkSleCentralManagerStub *stub, MessageParcel &data,
        MessageParcel &reply);
    using NearlinkCentralManagerFuncPerm = std::pair<NearlinkCentralManagerFunc,
        std::shared_ptr<NearLinkPermissionItem>>;

private:
    static int32_t RegisterSleCentralManagerCallbackInner(NearlinkSleCentralManagerStub *stub, MessageParcel &data,
        MessageParcel &reply);
    static int32_t DeregisterSleCentralManagerCallbackInner(NearlinkSleCentralManagerStub *stub, MessageParcel &data,
        MessageParcel &reply);
    static int32_t StartFullScanInner(NearlinkSleCentralManagerStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t StartScanWithFilterInner(NearlinkSleCentralManagerStub *stub, MessageParcel &data,
        MessageParcel &reply);
    static int32_t StopScanInner(NearlinkSleCentralManagerStub *stub, MessageParcel &data, MessageParcel &reply);

private:
    std::map<uint32_t, NearlinkCentralManagerFuncPerm> memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkSleCentralManagerStub);
};
}  // namespace Nearlink
}  // namespace OHOS

#endif