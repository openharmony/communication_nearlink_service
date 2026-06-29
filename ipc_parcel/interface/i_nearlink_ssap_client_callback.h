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

#ifndef OHOS_NEARLINK_STANDARD_SSAP_CLIENT_CALLBACK_INTERFACE_H
#define OHOS_NEARLINK_STANDARD_SSAP_CLIENT_CALLBACK_INTERFACE_H

#include "iremote_broker.h"
#include "nearlink_ssap_property_parcel.h"
#include "nearlink_ssap_method_parcel.h"
#include "nearlink_ssap_event_parcel.h"
#include "nearlink_ssap_descriptor_parcel.h"
#include "nearlink_ssap_service_parcel.h"
#include "nearlink_uuid_parcel.h"
#include "nearlink_uuid.h"
#include "nearlink_service_ipc_interface_code.h"
#include "raw_address.h"

namespace OHOS {
namespace Nearlink {
class INearlinkSsapClientCallback : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkSsapClientCallback");

    virtual void OnConnectionStateChanged(int32_t state, int32_t newState) = 0;
    virtual void OnPropertyChanged(const NearlinkSsapPropertyParcel &property) = 0;
    virtual void OnEventNotified(const NearlinkSsapEventParcel &event) = 0;
    virtual void OnReadProperty(int32_t ret, const NearlinkSsapPropertyParcel &property) = 0;
    virtual void OnCallMethod(int32_t ret, const NearlinkSsapMethodParcel &method) = 0;
    virtual void OnReadPropertiesByUuid(int32_t ret, const std::vector<NearlinkSsapPropertyParcel> &properties) = 0;
    virtual void OnWriteProperty(int32_t ret, const NearlinkSsapPropertyParcel &property) = 0;
    virtual void OnSetPropertyNotification(int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property) = 0;
    virtual void OnSetPropertyIndication(int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property) = 0;
    virtual void OnReadDescriptor(int32_t ret, const NearlinkSsapDescriptorParcel &descriptor) = 0;
    virtual void OnWriteDescriptor(int32_t ret, const NearlinkSsapDescriptorParcel &descriptor) = 0;
    virtual void OnMtuChanged(int32_t state, uint16_t mtu) = 0;
    virtual void OnServicesDiscovered(int32_t status) = 0;
    virtual void OnServicesDiscoveredByUuid(int32_t status, const Uuid &uuid) = 0;
    virtual void OnServicesRediscovered(const std::vector<NearlinkSsapServiceParcel> &services) = 0;
    virtual void OnServiceChanged(uint16_t handle, const NearlinkUuidParcel &uuid) = 0;
    virtual void OnConnectionParameterChanged(int32_t interval, int32_t latency, int32_t timeout, int32_t status) = 0;
};
}  // namespace NEARLINK
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_GATT_CLIENT_CALLBACK_INTERFACE_H