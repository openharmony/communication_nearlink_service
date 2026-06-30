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

#ifndef OHOS_NEARLINK_STANDARD_SSAP_SERVER_STUB_H
#define OHOS_NEARLINK_STANDARD_SSAP_SERVER_STUB_H

#include <map>

#include "iremote_stub.h"
#include "i_nearlink_host.h"
#include "i_nearlink_ssap_server.h"
#include "nearlink_permission_item.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSsapServerStub : public IRemoteStub<INearlinkSsapServer> {
public:
    NearlinkSsapServerStub();
    virtual ~NearlinkSsapServerStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    using NearlinkSsapServerFunc = int32_t (*)(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply);
    using NearlinkSsapServerFuncPerm = std::pair<NearlinkSsapServerFunc, std::shared_ptr<NearLinkPermissionItem>>;

private:
    static int32_t AddServiceInner(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t ClearServicesInner(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t CancelConnectionInner(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t RegisterApplicationInner(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t DeregisterApplicationInner(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t NotifyClientInner(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t NotifyEventInner(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t SetPropertyValueInner(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t SetDescriptorValueInner(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t ConnectInner(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t RemoveServiceInner(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t AuthorizeResponseInner(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply);

private:
    std::map<uint32_t, NearlinkSsapServerFuncPerm> memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkSsapServerStub);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_SSAP_SERVER_STUB_H