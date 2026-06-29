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
#ifndef NAPI_NEARLINK_SSAP_SERVER_CALLBACK_H_
#define NAPI_NEARLINK_SSAP_SERVER_CALLBACK_H_
#include <shared_mutex>

#include "nearlink_ssap_server.h"
#include "napi_nearlink_utils.h"
#include "napi_nearlink_ssap_utils.h"
#include "napi_async_callback.h"
#include "nearlink_safe_map.h"
#include "napi_event_subscribe_module.h"

namespace OHOS {
namespace Nearlink {
const char *const SLE_SSAP_SERVER_CALLBACK_CONNECT_STATE_CHANGE = "connectionStateChange";
const char *const SLE_SSAP_SERVER_CALLBACK_PROPERTY_WRITE = "propertyWrite";
const char *const SLE_SSAP_SERVER_CALLBACK_PROPERTY_READ = "propertyRead";
const char *const SLE_SSAP_SERVER_CALLBACK_MTU_CHANGE = "mtuChange";

class NapiNearlinkSsapServerCallback : public SsapServerCallback {
public:
    void OnConnectionStateUpdate(const NearlinkRemoteDevice &device, int state, int reason) override;
    void OnPropertyReadRequest(
        const NearlinkRemoteDevice &device, const SsapProperty &property, int ret) override;
    void OnPropertyWriteRequest(
        const NearlinkRemoteDevice &device, const SsapProperty &property, int ret) override;
    void OnMtuUpdate(const NearlinkRemoteDevice &device, int mtu) override;

    NapiNearlinkSsapServerCallback();
    ~NapiNearlinkSsapServerCallback() override = default;

    NapiEventSubscribeModule eventSubscribe;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif /* NAPI_NEARLINK_SSAP_SERVER_CALLBACK_H_ */