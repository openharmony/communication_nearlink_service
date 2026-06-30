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

#ifndef OHOS_NEARLINK_STANDARD_SSAP_SERVER_SERVER_H
#define OHOS_NEARLINK_STANDARD_SSAP_SERVER_SERVER_H

#include "i_nearlink_ssap_server.h"
#include "nearlink_ssap_server_stub.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability.h"
#include "nearlink_types.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSsapServerServer : public NearlinkSsapServerStub {
public:
    NearlinkSsapServerServer();
    ~NearlinkSsapServerServer() override;

    NlErrCode AddService(int32_t appId, NearlinkSsapServiceParcel *services) override;
    NlErrCode ClearServices(int appId) override;
    NlErrCode CancelConnection(int appId, const NearlinkSsapDevice &device) override;
    NlErrCode RegisterApplication(const sptr<INearlinkSsapServerCallback> &callback, int32_t &appId) override;
    NlErrCode DeregisterApplication(int32_t appId) override;
    NlErrCode NotifyClient(
        int appId, NearlinkSsapPropertyParcel *property, const NearlinkSsapDevice &device, bool needConfirm) override;
    NlErrCode NotifyEvent(int appId, NearlinkSsapEventParcel *event, std::vector<uint8_t> &value,
        const NearlinkSsapDevice &device, bool needConfirm) override;
    NlErrCode SetPropertyValue(int appId, NearlinkSsapPropertyParcel *property) override;
    NlErrCode SetDescriptorValue(int appId, NearlinkSsapDescriptorParcel *descriptor) override;
    NlErrCode Connect(int appId, const NearlinkSsapDevice &device, uint8_t secureReq, bool autoConnect) override;
    NlErrCode RemoveService(int32_t appId, const NearlinkSsapServiceParcel &services) override;
    NlErrCode AuthorizeResponse(int appId, uint16_t requestId, bool allow) override;

private:
    NEARLINK_DECLARE_IMPL();
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkSsapServerServer);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_SSAP_SERVER_SERVER_H