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

#ifndef OHOS_NEARLINK_TWS_CLIENT_STUB_H
#define OHOS_NEARLINK_TWS_CLIENT_STUB_H

#include <map>
#include "iremote_stub.h"
#include "i_nearlink_tws_client.h"
#include "nearlink_permission_item.h"

namespace OHOS {
namespace Nearlink {
class NearlinkTwsClientStub : public IRemoteStub<INearlinkTwsClient> {
public:
    NearlinkTwsClientStub();
    virtual ~NearlinkTwsClientStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    using NearlinkTwsClientFunc = int32_t (*)(NearlinkTwsClientStub *stub,
        MessageParcel &data, MessageParcel &reply);
    using NearlinkTwsClientFuncPerm = std::pair<NearlinkTwsClientFunc, std::shared_ptr<NearLinkPermissionItem>>;

private:
    static int32_t RegisterApplicationInner(NearlinkTwsClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t DeregisterApplicationInner(NearlinkTwsClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t EnableWearDetectionInner(NearlinkTwsClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t DisableWearDetectionInner(NearlinkTwsClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetWearDetectionStateInner(NearlinkTwsClientStub *stub,
        MessageParcel &data, MessageParcel &reply);
    static int32_t IsDeviceWearingInner(NearlinkTwsClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t IsWearDetectionSupportedInner(NearlinkTwsClientStub *stub,
        MessageParcel &data, MessageParcel &reply);
    static int32_t GetTwsRoleInfoInner(NearlinkTwsClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetTwsAudioDelayInner(NearlinkTwsClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t SendUserSelectionInner(NearlinkTwsClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t QueryStreamStateInner(NearlinkTwsClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t IsSupportVirtualAutoConnectInner(NearlinkTwsClientStub *stub, MessageParcel &data,
        MessageParcel &reply);
    static int32_t SetVirtualAutoConnectTypeInner(NearlinkTwsClientStub *stub, MessageParcel &data,
        MessageParcel &reply);
private:
    std::map<uint32_t, NearlinkTwsClientFuncPerm> memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkTwsClientStub);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_TWS_CLIENT_STUB_H