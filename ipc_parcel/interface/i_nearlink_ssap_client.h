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

#ifndef I_NEARLINK_SSAP_CLIENT_H
#define I_NEARLINK_SSAP_CLIENT_H

#include "iremote_broker.h"
#include "nearlink_service_ipc_interface_code.h"
#include "i_nearlink_ssap_client_callback.h"
#include "../parcel/nearlink_ssap_property_parcel.h"
#include "../parcel/nearlink_ssap_method_parcel.h"
#include "../parcel/nearlink_ssap_descriptor_parcel.h"
#include "../parcel/nearlink_uuid_parcel.h"
#include "../parcel/nearlink_ssap_service_parcel.h"
#include "../parcel/nearlink_raw_address.h"
#include "nearlink_errorcode.h"

namespace OHOS {
namespace Nearlink {
namespace {
const std::string PROFILE_SSAP_CLIENT = "SsapClientServer";
}  // namespace

class INearlinkSsapClient : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkSsapClient");

    virtual NlErrCode RegisterApplication(const sptr<INearlinkSsapClientCallback> &callback,
        const NearlinkRawAddress &addr, int32_t transport, int &appId) = 0;
    virtual NlErrCode RegisterApplication(const sptr<INearlinkSsapClientCallback> &callback, uint8_t secureReq,
        const NearlinkRawAddress &addr, int32_t transport, int &appId) = 0;
    virtual NlErrCode DeregisterApplication(int32_t appId) = 0;
    virtual NlErrCode Connect(int32_t appId, bool isAutoConnect) = 0;
    virtual NlErrCode Disconnect(int32_t appId) = 0;
    virtual NlErrCode DiscoveryServices(int32_t appId) = 0;
    virtual NlErrCode DiscoverServiceByUuid(int32_t appId, const Uuid &uuid) = 0;
    virtual NlErrCode ReadProperty(int32_t appId, const NearlinkSsapPropertyParcel &property) = 0;
    virtual NlErrCode CallMethod(int32_t appId, NearlinkSsapMethodParcel *method, bool withoutRespond) = 0;
    virtual NlErrCode WriteProperty(
        int32_t appId, NearlinkSsapPropertyParcel *property, bool withoutRespond) = 0;
    virtual NlErrCode ReadDescriptor(int32_t appId, const NearlinkSsapDescriptorParcel &descriptor) = 0;
    virtual NlErrCode WriteDescriptor(int32_t appId, NearlinkSsapDescriptorParcel *descriptor, bool withoutRespond) = 0;
    virtual NlErrCode RequestExchangeMtu(int32_t appId, int32_t mtu) = 0;
    virtual NlErrCode RequestConnectionPriority(int32_t appId, int32_t connPriority) = 0;
    virtual NlErrCode GetServices(int32_t appId, std::vector<NearlinkSsapServiceParcel> &service) = 0;
    virtual NlErrCode GetServicesByUuid(int32_t appId, const Uuid &uuid,
        std::vector<NearlinkSsapServiceParcel> &service) = 0;
    virtual NlErrCode RequestPropertyNotification(int32_t appId, uint16_t propertyHandle, bool enable,
        uint8_t notifyOption) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_GATT_CLIENT_INTERFACE_H