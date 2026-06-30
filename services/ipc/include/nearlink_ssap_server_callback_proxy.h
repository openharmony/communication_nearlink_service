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

#ifndef OHOS_NEARLINK_STANDARD_SSAP_SERVER_CALLBACK_PROXY_H
#define OHOS_NEARLINK_STANDARD_SSAP_SERVER_CALLBACK_PROXY_H

#include "iremote_proxy.h"
#include "i_nearlink_host.h"
#include "i_nearlink_ssap_server_callback.h"
namespace OHOS {
namespace Nearlink {
class NearlinkSsapServerCallbackProxy : public IRemoteProxy<INearlinkSsapServerCallback> {
public:
    explicit NearlinkSsapServerCallbackProxy(const sptr<IRemoteObject> &impl)
        : IRemoteProxy<INearlinkSsapServerCallback>(impl) {};
    ~NearlinkSsapServerCallbackProxy() {};

    void OnMtuChanged(const NearlinkSsapDevice &device, uint16_t mtu) override;
    void OnAddService(const NearlinkSsapServiceParcel &service, int ret) override;
    void OnPropertyReadRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapPropertyParcel &property, int ret) override;
    void OnDescriptorReadRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapDescriptorParcel &descriptor, int ret) override;
    void OnPropertyWriteRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapPropertyParcel &property, int ret) override;
    void OnDescriptorWriteRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapDescriptorParcel &descriptor, int ret) override;
    void OnNotifyPropertyChanged(
        const NearlinkSsapDevice &device, const Uuid &uuid, uint16_t handle, int ret) override;
    void OnNotifyEventChanged(
        const NearlinkSsapDevice &device, const Uuid &uuid, uint16_t handle, int ret) override;
    void OnConnectionStateChanged(const NearlinkSsapDevice &device, uint8_t state, int reason) override;

private:
    static inline BrokerDelegator<NearlinkSsapServerCallbackProxy> delegator_;
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_SSAP_SERVER_CALLBACK_PROXY_H