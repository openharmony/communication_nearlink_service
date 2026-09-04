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

#ifndef ANI_NEARLINK_SSAP_SERVER_CALLBACK_H
#define ANI_NEARLINK_SSAP_SERVER_CALLBACK_H

#include "ani_event_module.h"
#include "nearlink_ssap_server.h"
#include "nearlink_ssap_property.h"
#include "nearlink_ssap_descriptor.h"
#include "nearlink_remote_device.h"
#include "ohos.nearlink.ssap.proj.hpp"
#include "taihe/runtime.hpp"

namespace OHOS {
namespace Nearlink {

class AniSsapServerCallback : public SsapServerCallback {
public:
    AniSsapServerCallback() = default;
    ~AniSsapServerCallback() override = default;

    void OnConnectionStateUpdate(const NearlinkRemoteDevice &device, int state, int reason) override;
    void OnPropertyReadRequest(const NearlinkRemoteDevice &device,
        const SsapProperty &property, int requestId) override;
    void OnPropertyWriteRequest(const NearlinkRemoteDevice &device,
        const SsapProperty &property, int requestId) override;
    void OnMtuUpdate(const NearlinkRemoteDevice &device, int mtu) override;

    EventModule<void(::ohos::nearlink::ssap::ConnectionChangeState const& data)> connectionStateEvent_;
    EventModule<void(::ohos::nearlink::ssap::PropertyReadRequest const& data)> propertyReadEvent_;
    EventModule<void(::ohos::nearlink::ssap::PropertyWriteRequest const& data)> propertyWriteEvent_;
    EventModule<void(int32_t data)> mtuChangeEvent_;
};

}  // namespace Nearlink
}  // namespace OHOS

#endif  // ANI_NEARLINK_SSAP_SERVER_CALLBACK_H
