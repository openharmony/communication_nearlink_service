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

#ifndef OHOS_NEARLINK_SLE_CONTROLLER_STUB_H
#define OHOS_NEARLINK_SLE_CONTROLLER_STUB_H

#include <map>
#include "i_nearlink_sle_controller.h"
#include "iremote_stub.h"
#include "nearlink_permission_item.h"

namespace OHOS::Nearlink {
class NearlinkSleControllerStub : public IRemoteStub<INearlinkSleController> {
public:
    NearlinkSleControllerStub();
    virtual ~NearlinkSleControllerStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    using NearlinkSleControllerFunc = int32_t (*)(
        NearlinkSleControllerStub *stub, MessageParcel &data, MessageParcel &reply);
    using NearlinkSleControllerFuncPerm = std::pair<NearlinkSleControllerFunc,
                                                    std::shared_ptr<NearLinkPermissionItem>>;

private:
    static int32_t SetSleCoexParamInner(NearlinkSleControllerStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t UpdateConnectIntervalInner(
        NearlinkSleControllerStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t SetSleCoexModeInner(NearlinkSleControllerStub *stub, MessageParcel &data, MessageParcel &reply);

private:
    std::map<uint32_t, NearlinkSleControllerFuncPerm> memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkSleControllerStub);
};
}  // namespace OHOS::Nearlink
#endif  // OHOS_NEARLINK_SLE_CONTROLLER_STUB_H
