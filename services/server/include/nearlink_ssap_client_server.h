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

#ifndef OHOS_NEARLINK_STANDARD_SSAP_CLIENT_SERVER_H
#define OHOS_NEARLINK_STANDARD_SSAP_CLIENT_SERVER_H

#include <map>

#include "nearlink_ssap_client_stub.h"
#include "nearlink_types.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSsapClientServer : public NearlinkSsapClientStub {
public:
    explicit NearlinkSsapClientServer();
    ~NearlinkSsapClientServer() override;

    NlErrCode RegisterApplication(const sptr<INearlinkSsapClientCallback> &callback, uint8_t secureReq,
        const NearlinkRawAddress &addr, int32_t transport, int &appId) override;

    NlErrCode RegisterApplication(const sptr<INearlinkSsapClientCallback> &callback, const NearlinkRawAddress &addr,
        int32_t transport, int &appId) override;
    NlErrCode DeregisterApplication(int32_t appId) override;
    NlErrCode Connect(int32_t appId, bool isAutoConnect) override;
    NlErrCode Disconnect(int32_t appId) override;
    NlErrCode DiscoveryServices(int32_t appId) override;
    NlErrCode DiscoverServiceByUuid(int32_t appId, const Uuid &uuid) override;
    NlErrCode ReadProperty(int32_t appId, const NearlinkSsapPropertyParcel &property) override;
    NlErrCode CallMethod(int32_t appId, NearlinkSsapMethodParcel *method, bool withoutRespond) override;
    NlErrCode WriteProperty(int32_t appId, NearlinkSsapPropertyParcel *property, bool withoutRespond) override;
    NlErrCode ReadDescriptor(int32_t appId, const NearlinkSsapDescriptorParcel &descriptor) override;
    NlErrCode WriteDescriptor(int32_t appId, NearlinkSsapDescriptorParcel *descriptor, bool withoutRespond) override;
    NlErrCode RequestExchangeMtu(int32_t appId, int32_t mtu) override;
    NlErrCode RequestConnectionPriority(int32_t appId, int32_t connPriority) override;
    NlErrCode GetServices(int32_t appId, std::vector<NearlinkSsapServiceParcel> &service) override;
    NlErrCode GetServicesByUuid(int32_t appId, const Uuid &uuid,
        std::vector<NearlinkSsapServiceParcel> &service) override;
    NlErrCode RequestPropertyNotification(int32_t appId, uint16_t propertyhandle,
        bool enable, uint8_t notifyOption) override;
private:
   NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkSsapClientServer);
   NEARLINK_DECLARE_IMPL();
};
}  // namespaceNearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_SSAP_CLIENT_SERVER_H