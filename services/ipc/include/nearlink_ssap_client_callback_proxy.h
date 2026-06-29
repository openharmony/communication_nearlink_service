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

#ifndef OHOS_NEARLINK_STANDARD_SSAP_CLIENT_CALLBACK_PROXY_H
#define OHOS_NEARLINK_STANDARD_SSAP_CLIENT_CALLBACK_PROXY_H

#include "iremote_proxy.h"
#include "i_nearlink_ssap_client_callback.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSsapClientCallbackProxy : public IRemoteProxy<INearlinkSsapClientCallback> {
public:
    explicit NearlinkSsapClientCallbackProxy(const sptr<IRemoteObject> &impl)
        : IRemoteProxy<INearlinkSsapClientCallback>(impl)
    {}
    ~NearlinkSsapClientCallbackProxy()
    {}

    void OnConnectionStateChanged(int32_t state, int32_t newState) override;
    void OnPropertyChanged(const NearlinkSsapPropertyParcel &property) override;
    void OnEventNotified(const NearlinkSsapEventParcel &event) override;
    void OnReadProperty(int32_t ret, const NearlinkSsapPropertyParcel &property) override;
    void OnCallMethod(int32_t ret, const NearlinkSsapMethodParcel &method) override;
    void OnReadPropertiesByUuid(int32_t ret, const std::vector<NearlinkSsapPropertyParcel> &properties) override;
    void OnWriteProperty(int32_t ret, const NearlinkSsapPropertyParcel &property) override;
    void OnSetPropertyNotification(int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property) override;
    void OnSetPropertyIndication(int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property) override;
    void OnReadDescriptor(int32_t ret, const NearlinkSsapDescriptorParcel &descriptor) override;
    void OnWriteDescriptor(int32_t ret, const NearlinkSsapDescriptorParcel &descriptor) override;
    void OnMtuChanged(int state, uint16_t mtu) override;
    void OnServicesDiscovered(int32_t status) override;
    void OnServicesDiscoveredByUuid(int32_t status, const Uuid &uuid) override;
    void OnServicesRediscovered(const std::vector<NearlinkSsapServiceParcel> &services) override;
    void OnServiceChanged(uint16_t handle, const NearlinkUuidParcel &uuid) override;
    void OnConnectionParameterChanged(int32_t interval, int32_t latency, int32_t timeout, int32_t status) override;
private:
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
    static inline BrokerDelegator<NearlinkSsapClientCallbackProxy> delegator_;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_SSAP_CLIENT_CALLBACK_PROXY_H