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

#ifndef OHOS_NEARLINK_STANDARD_SSAP_CLIENT_STUB_H
#define OHOS_NEARLINK_STANDARD_SSAP_CLIENT_STUB_H

#include <map>

#include "iremote_stub.h"
#include "i_nearlink_ssap_client.h"
#include "nearlink_permission_item.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSsapClientStub : public IRemoteStub<INearlinkSsapClient> {
public:
    NearlinkSsapClientStub();
    virtual ~NearlinkSsapClientStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    using NearlinkSsapClientFunc = int32_t (*)(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    using NearlinkSsapClientFuncPerm = std::pair<NearlinkSsapClientFunc, std::shared_ptr<NearLinkPermissionItem>>;

private:
    static int32_t RegisterApplicationInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t DeregisterApplicationInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t ConnectInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t DisconnectInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t DiscoveryServicesInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t DiscoverServiceByUuidInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t ReadPropertyInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t CallMethodInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t WritePropertyInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t ReadDescriptorInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t WriteDescriptorInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t RequestExchangeMtuInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t RequestConnectionPriorityInner(NearlinkSsapClientStub *stub, MessageParcel &data,
        MessageParcel &reply);
    static int32_t GetServicesInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetServicesByUuidInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t RequestNotificationInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t RequestIndicationInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply);

private:
    std::map<uint32_t, NearlinkSsapClientFuncPerm> memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkSsapClientStub);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_Nearlink_STANDARD_SSAP_CLIENT_STUB_H