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

#include "ani_nearlink_ssap_client_callback.h"
#include "log_util.h"
#include "ani_nearlink_error.h"
#include "ani_nearlink_ssap_utils.h"
#include "ani_nearlink_ssap_client.h"

namespace OHOS {
namespace Nearlink {

AniSsapClientCallback::AniSsapClientCallback()
    : eventSubscribe_({SLE_SSAP_CLIENT_CALLBACK_CONNECTION_STATE_CHANGE,
        SLE_SSAP_CLIENT_CALLBACK_PROPERTY_CHANGE,
        SLE_SSAP_CLIENT_CALLBACK_MTU_CHANGE,
        SLE_SSAP_CLIENT_CALLBACK_EVENT_NOTIFY},
        "nearlinkAdvertising_taihe")
{}

std::shared_ptr<AniSsapClientCallback> AniSsapClientCallback::GetInstance()
{
    static std::shared_ptr<AniSsapClientCallback> instance =
        std::make_shared<AniSsapClientCallback>();
    return instance;
}

void AniSsapClientCallback::OnConnectionStateChanged(int connectionState, int ret)
{
    HILOGI("connectionState:%{public}d, ret:%{public}d", connectionState, ret);
    std::string deviceId = "";
    {
        NL_CHECK_RETURN(client_, "client is nullptr");
        NL_CHECK_RETURN(client_->GetDevice(), "device is nullptr");
        deviceId = client_->GetDevice()->GetDeviceAddr();
    }

    ::ohos::nearlink::ssap::ConnectionChangeState result {
        .address = deviceId,
        .state = ohos::nearlink::constant::ConnectionState::from_value(connectionState)
    };
    auto &module = eventSubscribe_.GetModule<void(::ohos::nearlink::ssap::ConnectionChangeState const&)
            >(SLE_SSAP_CLIENT_CALLBACK_CONNECTION_STATE_CHANGE);
    module.PublishEvent(result);
}

void AniSsapClientCallback::OnMtuUpdate(uint16_t mtu, int ret)
{
    HILOGI("mtu:%{public}d, ret:%{public}d", mtu, ret);
    int32_t result = static_cast<int32_t>(mtu);
    auto &module = eventSubscribe_.GetModule<void(int32_t)>(SLE_SSAP_CLIENT_CALLBACK_MTU_CHANGE);
    module.PublishEvent(result);
}

void AniSsapClientCallback::OnPropertyChanged(const SsapProperty &property)
{
    auto result = ConvertPropertyToTaihe(property);
    auto &module = eventSubscribe_.GetModule<void(ohos::nearlink::ssap::Property const&)
            >(SLE_SSAP_CLIENT_CALLBACK_PROPERTY_CHANGE);
    module.PublishEvent(result);
}

void AniSsapClientCallback::OnEventNotified(const SsapEvent &event)
{
    auto result = ConvertEventToTaihe(event);
    auto &module = eventSubscribe_.GetModule<void(::ohos::nearlink::ssap::Event const&)
            >(SLE_SSAP_CLIENT_CALLBACK_EVENT_NOTIFY);
    module.PublishEvent(result);
}

void AniSsapClientCallback::OnPropertyReadResult(const SsapProperty &property, int ret)
{
    HILOGI("ret: %{public}d", ret);
    auto result = std::make_shared<TaiheNativeSsapProperty>(property);
    AsyncWorkCallFunction(asyncPromiseMap_, TaiheAsyncType::SSAP_CLIENT_READ_PROPERTY, result, ret);
}

void AniSsapClientCallback::OnMethodCallResult(const SsapMethod &method, int ret)
{
    HILOGI("ret: %{public}d", ret);
    auto result = std::make_shared<TaiheNativeSsapMethod>(method);
    AsyncWorkCallFunction(asyncPromiseMap_, TaiheAsyncType::SSAP_CLIENT_CALL_METHOD, result, ret);
}

void AniSsapClientCallback::OnPropertiesReadResult(const std::vector<SsapProperty> &properties, int ret)
{
    HILOGI("ret: %{public}d", ret);
}

void AniSsapClientCallback::OnPropertyWriteResult(const SsapProperty &property, int ret)
{
    HILOGI("ret: %{public}d", ret);
    AsyncWorkCallFunction(asyncPromiseMap_, TaiheAsyncType::SSAP_CLIENT_WRITE_PROPERTY, nullptr, ret);
}

void AniSsapClientCallback::OnDescriptorReadResult(const SsapDescriptor &descriptor, int ret)
{
    HILOGI("ret: %{public}d", ret);
    auto result = std::make_shared<TaiheNativeSsapDescriptor>(descriptor);
    AsyncWorkCallFunction(asyncPromiseMap_, TaiheAsyncType::SSAP_CLIENT_READ_DESCRIPTOR, result, ret);
}

void AniSsapClientCallback::OnDescriptorWriteResult(const SsapDescriptor &descriptor, int ret)
{
    HILOGI("ret: %{public}d", ret);
    AsyncWorkCallFunction(asyncPromiseMap_, TaiheAsyncType::SSAP_CLIENT_WRITE_DESCRIPTOR, nullptr, ret);
}

void AniSsapClientCallback::OnSetPropertyNotifyResult(const SsapProperty &property, int enable, int ret)
{
    HILOGI("enable: %{public}d, ret: %{public}d", enable, ret);
    AsyncWorkCallFunction(asyncPromiseMap_, TaiheAsyncType::SSAP_CLIENT_SET_PROPERTY_NOTIFY, nullptr, ret);
}

void AniSsapClientCallback::OnSetPropertyIndicateResult(const SsapProperty &property, int enable, int ret)
{
    HILOGI("enable: %{public}d, ret: %{public}d", enable, ret);
    AsyncWorkCallFunction(asyncPromiseMap_, TaiheAsyncType::SSAP_CLIENT_SET_PROPERTY_INDICATE, nullptr, ret);
}

}  // namespace Nearlink
}  // namespace OHOS
