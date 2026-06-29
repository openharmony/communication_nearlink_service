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
#include <uv.h>
#include "log.h"
#include "napi_async_work.h"
#include "napi_native_object.h"
#include "napi_nearlink_ssap_client.h"
#include "nearlink_errorcode.h"
#include "napi_nearlink_ssap_client.h"
#include "napi_nearlink_ssap_utils.h"
#include "napi_nearlink_ssap_client_callback.h"
namespace OHOS {
namespace Nearlink {
namespace {
static const int32_t DESCRIPTOR_TYPE_OFFSET = 1;
}

NapiNearlinkSsapClientCallback::NapiNearlinkSsapClientCallback()
    : eventSubscribe({SLE_SSAP_CLIENT_CALLBACK_CONNECTION_STATE_CHANGE,
        SLE_SSAP_CLIENT_CALLBACK_PROPERTY_CHANGE,
        SLE_SSAP_CLIENT_CALLBACK_MTU_CHANGE,
        SLE_SSAP_CLIENT_CALLBACK_EVENT_NOTIFY},
        NL_MODULE_NAME)
{}

void NapiNearlinkSsapClientCallback::OnConnectionStateChanged(int connectionState, int ret)
{
    HILOGI("connectionState:%{public}d, ret:%{public}d", connectionState, ret);
    NL_CHECK_RETURN(client_, "client is nullptr");
    NL_CHECK_RETURN(client_->GetDevice(), "device is nullptr");
    std::string deviceId = client_->GetDevice()->GetDeviceAddr();
    auto napiNative = std::make_shared<NapiNativeSsapConnectionState>(deviceId, connectionState);
    eventSubscribe.PublishEvent(SLE_SSAP_CLIENT_CALLBACK_CONNECTION_STATE_CHANGE, napiNative);
}

void NapiNearlinkSsapClientCallback::OnMtuUpdate(uint16_t mtu, int ret)
{
    HILOGI("mtu:%{public}d, ret:%{public}d", mtu, ret);
    auto napiNative = std::make_shared<NapiNativeInt>(mtu);
    eventSubscribe.PublishEvent(SLE_SSAP_CLIENT_CALLBACK_MTU_CHANGE, napiNative);
}

void NapiNearlinkSsapClientCallback::OnPropertyChanged(const SsapProperty &property)
{
    SsapProperty ssapProperty(property);
    auto napiNative = std::make_shared<NapiNativeSsapProperty>(ssapProperty);
    eventSubscribe.PublishEvent(SLE_SSAP_CLIENT_CALLBACK_PROPERTY_CHANGE, napiNative);
}

void NapiNearlinkSsapClientCallback::OnEventNotified(const SsapEvent &event)
{
    SsapEvent ssapEvent(event);
    auto napiNative = std::make_shared<NapiNativeSsapEvent>(ssapEvent);
    eventSubscribe.PublishEvent(SLE_SSAP_CLIENT_CALLBACK_EVENT_NOTIFY, napiNative);
}

void NapiNearlinkSsapClientCallback::OnPropertyReadResult(const SsapProperty &property, int ret)
{
    HILOGI("ret: %{public}d", ret);
    SsapProperty ssapProperty(property);
    auto napiProperty = std::make_shared<NapiNativeSsapProperty>(ssapProperty);
    AsyncWorkCallFunction(asyncPromiseMap_, NapiAsyncType::SSAP_CLIENT_READ_PROPERTY, napiProperty, ret);
}

void NapiNearlinkSsapClientCallback::OnMethodCallResult(const SsapMethod &method, int ret)
{
    HILOGI("ret: %{public}d", ret);
    SsapMethod ssapMethod(method);
    auto napiMethod = std::make_shared<NapiNativeSsapMethod>(ssapMethod);
    AsyncWorkCallFunction(asyncPromiseMap_, NapiAsyncType::SSAP_CLIENT_CALL_METHOD, napiMethod, ret);
}

void NapiNearlinkSsapClientCallback::OnPropertiesReadResult(const std::vector<SsapProperty> &properties, int ret)
{
    HILOGI("ret: %{public}d", ret);
    std::vector<SsapProperty> ssapProperties = properties;
    auto napiProperties = std::make_shared<NapiNativeSsapPropertyArray>(ssapProperties);
    AsyncWorkCallFunction(asyncPromiseMap_, NapiAsyncType::SSAP_CLIENT_READ_PROPERTY_BY_UUID, napiProperties, ret);
}

void NapiNearlinkSsapClientCallback::OnPropertyWriteResult(const SsapProperty &property, int ret)
{
    HILOGI("ret: %{public}d", ret);
    AsyncWorkCallFunction(asyncPromiseMap_, NapiAsyncType::SSAP_CLIENT_WRITE_PROPERTY, nullptr, ret);
}

void NapiNearlinkSsapClientCallback::OnDescriptorReadResult(const SsapDescriptor &descriptor, int ret)
{
    HILOGI("ret: %{public}d", ret);
    SsapDescriptor ssapDescriptor(descriptor);
    auto napiDesscriptor = std::make_shared<NapiNativeSsapDescriptor>(ssapDescriptor,
        descriptor.GetDescriptorType() - DESCRIPTOR_TYPE_OFFSET);
    AsyncWorkCallFunction(asyncPromiseMap_, NapiAsyncType::SSAP_CLIENT_READ_DESCRIPTOR, napiDesscriptor, ret);
}

void NapiNearlinkSsapClientCallback::OnDescriptorWriteResult(const SsapDescriptor &descriptor, int ret)
{
    HILOGI("ret: %{public}d", ret);
    AsyncWorkCallFunction(asyncPromiseMap_, NapiAsyncType::SSAP_CLIENT_WRITE_DESCRIPTOR, nullptr, ret);
}

void NapiNearlinkSsapClientCallback::OnSetPropertyNotifyResult(const SsapProperty &property, int enable, int ret)
{
    HILOGI("enable: %{public}d, ret: %{public}d", enable, ret);
    AsyncWorkCallFunction(asyncPromiseMap_, NapiAsyncType::SSAP_CLIENT_SET_PROPERTY_NOTIFY, nullptr, ret);
}

void NapiNearlinkSsapClientCallback::OnSetPropertyIndicateResult(const SsapProperty &property, int enable, int ret)
{
    HILOGI("enable: %{public}d, ret: %{public}d", enable, ret);
    AsyncWorkCallFunction(asyncPromiseMap_, NapiAsyncType::SSAP_CLIENT_SET_PROPERTY_INDICATE, nullptr, ret);
}
} // namespace Nearlink
} // namespace OHOS
