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

#ifndef OHOS_NEARLINK_STANDARD_HID_HOST_STUB_H
#define OHOS_NEARLINK_STANDARD_HID_HOST_STUB_H

#include <map>

#include "iremote_stub.h"
#include "i_nearlink_hid_host.h"
#include "nearlink_permission_item.h"

namespace OHOS {
namespace Nearlink {
class NearlinkHidHostStub : public IRemoteStub<INearlinkHidHost> {
public:
    NearlinkHidHostStub();
    virtual ~NearlinkHidHostStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    using NearlinkHidHostFunc = int32_t (*)(NearlinkHidHostStub *stub, MessageParcel &data, MessageParcel &reply);
    using NearlinkHidHostFuncPerm = std::pair<NearlinkHidHostFunc, std::shared_ptr<NearLinkPermissionItem>>;

private:
    static int32_t HidHostSetReportInner(NearlinkHidHostStub *stub, MessageParcel &data, MessageParcel &reply);

private:
    std::map<uint32_t, NearlinkHidHostFuncPerm> memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkHidHostStub);
};

}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_HID_HOST_STUB_H