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

#include "napi_nearlink_advertising_callback.h"

#include "log.h"
#include "napi_async_work.h"
#include "napi_native_object.h"

namespace OHOS {
namespace Nearlink {


napi_value AdvertisingStateChangeInfo::ToNapiValue(napi_env env) const
{
    napi_value object;
    napi_create_object(env, &object);

    napi_value value;
    napi_create_int32(env, advHandle_, &value);
    napi_set_named_property(env, object, "advertisingId", value);

    napi_create_int32(env, advState_, &value);
    napi_set_named_property(env, object, "state", value);
    return object;
}

NapiNearlinkAdvertiseCallback::NapiNearlinkAdvertiseCallback()
    : eventSubscribe(REGISTER_ADVERTISING_STATE_INFO_NAME, NL_MODULE_NAME)
{}

std::shared_ptr<NapiNearlinkAdvertiseCallback> NapiNearlinkAdvertiseCallback::GetInstance(void)
{
    static std::shared_ptr<NapiNearlinkAdvertiseCallback> instance =
        std::make_shared<NapiNearlinkAdvertiseCallback>();
    return instance;
}

void NapiNearlinkAdvertiseCallback::OnStartResultEvent(int result, int advHandle)
{
    HILOGI("enter, result: %{public}d advHandle: %{public}d", result, advHandle);
    std::lock_guard<std::mutex> lock(callbackMutex_);
    auto napiNative = std::make_shared<AdvertisingStateChangeInfo>(advHandle,
        static_cast<int>(AdvertisingState::STARTED));
    eventSubscribe.PublishEvent(REGISTER_ADVERTISING_STATE_INFO_NAME, napiNative);
}

void NapiNearlinkAdvertiseCallback::OnStopResultEvent(int result, int advHandle)
{
    HILOGI("enter, result: %{public}d advHandle: %{public}d", result, advHandle);
    std::lock_guard<std::mutex> lock(callbackMutex_);
    auto napiNative = std::make_shared<AdvertisingStateChangeInfo>(advHandle,
        static_cast<int>(AdvertisingState::STOPPED));
    eventSubscribe.PublishEvent(REGISTER_ADVERTISING_STATE_INFO_NAME, napiNative);
}

void NapiNearlinkAdvertiseCallback::OnSetAdvDataEvent(int result)
{}

void NapiNearlinkAdvertiseCallback::OnGetAdvHandleEvent(int result, int advHandle)
{
    auto napiAdvHandle = std::make_shared<NapiNativeInt>(advHandle);
    AsyncWorkCallFunction(asyncPromiseMap_, NapiAsyncType::GET_ADVERTISING_HANDLE, napiAdvHandle, result);
}
}  // namespace Nearlink
}  // namespace OHOS