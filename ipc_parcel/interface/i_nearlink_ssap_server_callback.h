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

#ifndef OHOS_NEARLINK_STANDARD_SSAP_SERVER_CALLBACK_INTERFACE_H
#define OHOS_NEARLINK_STANDARD_SSAP_SERVER_CALLBACK_INTERFACE_H

#include "nearlink_service_ipc_interface_code.h"
#include "iremote_broker.h"
#include "nearlink_ssap_service_parcel.h"
#include "nearlink_ssap_descriptor_parcel.h"
#include "nearlink_ssap_device.h"
#include "nearlink_ssap_property_parcel.h"
#include "nearlink_uuid_parcel.h"

namespace OHOS {
namespace Nearlink {
class INearlinkSsapServerCallback : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkSsapServerCallback");

    virtual void OnMtuChanged(const NearlinkSsapDevice &device, uint16_t mtu) = 0;
    virtual void OnAddService(const NearlinkSsapServiceParcel &service, int ret) = 0;
    virtual void OnPropertyReadRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapPropertyParcel &property, int ret) = 0;
    virtual void OnDescriptorReadRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapDescriptorParcel &descriptor, int ret) = 0;
    virtual void OnPropertyWriteRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapPropertyParcel &property, int ret) = 0;
    virtual void OnDescriptorWriteRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapDescriptorParcel &descriptor, int ret) = 0;
    virtual void OnNotifyPropertyChanged(
        const NearlinkSsapDevice &device, const Uuid &uuid, uint16_t handle, int ret) = 0;
    virtual void OnNotifyEventChanged(
        const NearlinkSsapDevice &device, const Uuid &uuid, uint16_t handle, int ret) = 0;
    virtual void OnConnectionStateChanged(const NearlinkSsapDevice &device, uint8_t state, int reason) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // NEARLINK_PARCEL_SSAP_SERVER_CALLBACK_H