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

#ifndef OHOS_NEARLINK_HADM_CLIENT_STUB_H
#define OHOS_NEARLINK_HADM_CLIENT_STUB_H

#include <map>
#include "iremote_stub.h"
#include "i_nearlink_hadm_client.h"
#include "nearlink_permission_item.h"

namespace OHOS {
namespace Nearlink {
class NearlinkHadmClientStub : public IRemoteStub<INearlinkHadmClient> {
public:
    NearlinkHadmClientStub();
    virtual ~NearlinkHadmClientStub() override;

    virtual int OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    using NearlinkHadmClientFunc = int32_t (*)(NearlinkHadmClientStub *stub, MessageParcel &data, MessageParcel &reply);
    using NearlinkHadmClientFuncPerm = std::pair<NearlinkHadmClientFunc,
        std::shared_ptr<NearLinkPermissionItem>>;

private:
    std::map<uint32_t, NearlinkHadmClientFuncPerm> memberFuncMap_;

    static int32_t RegisterNearlinkHadmClientCallbackInner(NearlinkHadmClientStub *stub, MessageParcel &data,
        MessageParcel &reply);
    static int32_t DeregisterNearlinkHadmClientCallbackInner(NearlinkHadmClientStub *stub, MessageParcel &data,
        MessageParcel &reply);
    static int32_t StartSoundingInner(NearlinkHadmClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t StopSoundingInner(NearlinkHadmClientStub *stub, MessageParcel &data, MessageParcel &reply);
    DISALLOW_COPY_AND_MOVE(NearlinkHadmClientStub);
    static int32_t GetHadmFeatureInner(NearlinkHadmClientStub *stub, MessageParcel &data, MessageParcel &reply);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_HADM_CLIENT_STUB_H