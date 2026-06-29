/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_NEARLINK_STANDARD_SSAP_CLIENT_PROXY_H
#define OHOS_NEARLINK_STANDARD_SSAP_CLIENT_PROXY_H

#include "iremote_proxy.h"
#include "i_nearlink_ssap_client.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSsapClientProxy : public IRemoteProxy<INearlinkSsapClient> {
public:
    explicit NearlinkSsapClientProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<INearlinkSsapClient>(impl)
    {}
    ~NearlinkSsapClientProxy()
    {}

    NlErrCode RegisterApplication(const sptr<INearlinkSsapClientCallback> &callback, const NearlinkRawAddress &addr,
        int32_t transport, int &appId) override;
    NlErrCode RegisterApplication(const sptr<INearlinkSsapClientCallback> &callback, uint8_t secureReq,
            const NearlinkRawAddress &addr,
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
    NlErrCode RequestPropertyNotification(int32_t appId, uint16_t propertyHandle,
        bool enable, uint8_t notifyOption) override;
private:
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
    static inline BrokerDelegator<NearlinkSsapClientProxy> delegator_;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_SSAP_CLIENT_PROXY_H