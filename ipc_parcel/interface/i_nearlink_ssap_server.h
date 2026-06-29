
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
#ifndef OHOS_NEARLINK_STANDARD_SSAP_SERVER_INTERFACE_H
#define OHOS_NEARLINK_STANDARD_SSAP_SERVER_INTERFACE_H

#include "nearlink_service_ipc_interface_code.h"
#include "i_nearlink_ssap_server_callback.h"
#include "iremote_broker.h"
#include "nearlink_ssap_property_parcel.h"
#include "nearlink_ssap_descriptor_parcel.h"
#include "nearlink_ssap_device.h"
#include "nearlink_ssap_service_parcel.h"
#include "nearlink_ssap_event_parcel.h"
#include "ssap_data.h"
#include "nearlink_errorcode.h"

namespace OHOS {
namespace Nearlink {
namespace {
const std::string PROFILE_SSAP_SERVER = "SsapServerServer";
}  // namespace

class INearlinkSsapServer : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkSsapServer");

    virtual NlErrCode AddService(int32_t appId, NearlinkSsapServiceParcel *services) = 0;
    virtual NlErrCode ClearServices(int appId) = 0;
    virtual NlErrCode CancelConnection(int appId, const NearlinkSsapDevice &device) = 0;
    virtual NlErrCode RegisterApplication(const sptr<INearlinkSsapServerCallback> &callback, int32_t &appId) = 0;
    virtual NlErrCode DeregisterApplication(int32_t appId) = 0;
    virtual NlErrCode NotifyClient(
        int appId, NearlinkSsapPropertyParcel *property, const NearlinkSsapDevice &device, bool needConfirm) = 0;
    virtual NlErrCode NotifyEvent(int appId, NearlinkSsapEventParcel *event, std::vector<uint8_t> &value,
        const NearlinkSsapDevice &device, bool needConfirm) = 0;
    virtual NlErrCode SetPropertyValue(int32_t appId, NearlinkSsapPropertyParcel *property) = 0;
    virtual NlErrCode SetDescriptorValue(int32_t appId, NearlinkSsapDescriptorParcel *descriptor) = 0;
    virtual NlErrCode Connect(int32_t appId, const NearlinkSsapDevice &device, uint8_t secureReq, bool autoConnect) = 0;
    virtual NlErrCode RemoveService(int32_t appId, const NearlinkSsapServiceParcel &services) = 0;
    virtual NlErrCode AuthorizeResponse(int appId, uint16_t requestId, bool allow) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // OHOS_NEARLINK_STANDARD_SSAP_SERVER_INTERFACE_H
