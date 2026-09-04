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

#ifndef ANI_NEARLINK_SSAP_CLIENT_CALLBACK_H
#define ANI_NEARLINK_SSAP_CLIENT_CALLBACK_H

#include "ani_event_subscribe_module.h"
#include "nearlink_ssap_client.h"
#include "nearlink_ssap_property.h"
#include "nearlink_ssap_event.h"
#include "nearlink_ssap_method.h"
#include "nearlink_ssap_descriptor.h"
#include "ohos.nearlink.ssap.proj.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"
#include "taihe_async_work.h"

namespace OHOS {
namespace Nearlink {

class SsapClientImpl;
const char *const SLE_SSAP_CLIENT_CALLBACK_CONNECTION_STATE_CHANGE = "connectionStateChange";
const char *const SLE_SSAP_CLIENT_CALLBACK_PROPERTY_CHANGE = "propertyChange";
const char *const SLE_SSAP_CLIENT_CALLBACK_EVENT_NOTIFY = "eventNotify";
const char *const SLE_SSAP_CLIENT_CALLBACK_MTU_CHANGE = "mtuChange";

class AniSsapClientCallback : public SsapClientCallback {
public:
    AniSsapClientCallback();
    ~AniSsapClientCallback() override = default;

    std::shared_ptr<AniSsapClientCallback> GetInstance();
    void OnConnectionStateChanged(int connectionState, int ret) override;
    void OnMtuUpdate(uint16_t mtu, int ret) override;
    void OnPropertyChanged(const SsapProperty &property) override;
    void OnEventNotified(const SsapEvent &event) override;
    void OnPropertyReadResult(const SsapProperty &property, int ret) override;
    void OnMethodCallResult(const SsapMethod &method, int ret) override;
    void OnPropertiesReadResult(const std::vector<SsapProperty> &properties, int ret) override;
    void OnPropertyWriteResult(const SsapProperty &property, int ret) override;
    void OnDescriptorReadResult(const SsapDescriptor &descriptor, int ret) override;
    void OnDescriptorWriteResult(const SsapDescriptor &descriptor, int ret) override;
    void OnSetPropertyNotifyResult(const SsapProperty &property, int enable, int ret) override;
    void OnSetPropertyIndicateResult(const SsapProperty &property, int enable, int ret) override;

    void SetClient(SsapClientImpl *client)
    {
        client_ = client;
    }
    TaiheAsyncWorkMap asyncPromiseMap_ {};
    AniEventSubscribeModule eventSubscribe_;
private:
    SsapClientImpl *client_ = nullptr;
};

}  // namespace Nearlink
}  // namespace OHOS

#endif  // ANI_NEARLINK_SSAP_CLIENT_CALLBACK_H
