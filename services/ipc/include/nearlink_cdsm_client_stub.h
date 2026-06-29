/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_NEARLINK_CDSM_CLIENT_STUB_H
#define OHOS_NEARLINK_CDSM_CLIENT_STUB_H

#include <map>
#include "iremote_stub.h"
#include "i_nearlink_cdsm_client.h"
#include "nearlink_permission_item.h"

namespace OHOS {
namespace Nearlink {
class NearlinkCdsmClientStub : public IRemoteStub<INearlinkCdsmClient> {
public:
    NearlinkCdsmClientStub();
    ~NearlinkCdsmClientStub() override;

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    using NearlinkCdsmClientFunc = int32_t (*)(NearlinkCdsmClientStub *stub, MessageParcel &data, MessageParcel &reply);
    using NearlinkCdsmClientFuncPerm = std::pair<NearlinkCdsmClientFunc, std::shared_ptr<NearLinkPermissionItem>>;

private:
    static int32_t RegisterCdsmClientCallbackInner(NearlinkCdsmClientStub *stub, MessageParcel &data,
                                                   MessageParcel &reply);
    static int32_t DeregisterCdsmClientCallbackInner(NearlinkCdsmClientStub *stub, MessageParcel &data,
                                                     MessageParcel &reply);
    static int32_t GetCdsInfoInner(NearlinkCdsmClientStub *stub, MessageParcel &data, MessageParcel &reply);

    std::map<uint32_t, NearlinkCdsmClientFuncPerm> memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkCdsmClientStub);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_CDSM_CLIENT_STUB_H