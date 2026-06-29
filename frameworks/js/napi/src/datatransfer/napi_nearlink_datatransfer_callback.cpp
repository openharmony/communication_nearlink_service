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

#include "napi_nearlink_datatransfer_callback.h"

#include "log.h"
#include "napi_async_work.h"
#include "napi_native_object.h"

namespace OHOS {
namespace Nearlink {
napi_value ConnectionResult::ToNapiValue(napi_env env) const
{
    napi_value object;
    napi_create_object(env, &object);

    napi_value value;
    napi_create_string_utf8(env, address_.c_str(), NAPI_AUTO_LENGTH, &value);
    napi_set_named_property(env, object, "address", value);

    napi_create_string_utf8(env, uuid_.c_str(), NAPI_AUTO_LENGTH, &value);
    napi_set_named_property(env, object, "uuid", value);

    napi_create_int32(env, mtu_, &value);
    napi_set_named_property(env, object, "mtu", value);

    napi_create_int32(env, state_, &value);
    napi_set_named_property(env, object, "state", value);
    return object;
}

napi_value DataResult::ToNapiValue(napi_env env) const
{
    napi_value object;
    napi_create_object(env, &object);

    napi_value addressVal;
    napi_create_string_utf8(env, dataParams_.GetAddress().c_str(), NAPI_AUTO_LENGTH, &addressVal);
    napi_set_named_property(env, object, "address", addressVal);

    napi_value uuidVal;
    napi_create_string_utf8(env, dataParams_.GetUuid().c_str(), NAPI_AUTO_LENGTH, &uuidVal);
    napi_set_named_property(env, object, "uuid", uuidVal);

    size_t valueSize = 0;
    uint8_t *valueData = dataParams_.GetData(&valueSize).get();
    napi_value value;
    uint8_t *bufferData = nullptr;
    napi_create_arraybuffer(env, valueSize, reinterpret_cast<void **>(&bufferData), &value);
    if (valueSize > 0 && valueData != nullptr && memcpy_s(bufferData, valueSize, valueData, valueSize) != EOK) {
        HILOGE("dataResult memcpy_s failed");
    } else {
        napi_set_named_property(env, object, "data", value);
    }

    return object;
}

NapiNearlinkDataTransferCallback::NapiNearlinkDataTransferCallback()
    : eventSubscribe({ SLE_DATATRANSFER_CALLBACK_CONNECTION_STATE_CHANGE, SLE_DATATRANSFER_CALLBACK_READ_DATA },
    NL_MODULE_NAME)
{}

std::shared_ptr<NapiNearlinkDataTransferCallback> NapiNearlinkDataTransferCallback::GetInstance(void)
{
    static std::shared_ptr<NapiNearlinkDataTransferCallback> instance =
        std::make_shared<NapiNearlinkDataTransferCallback>();
    return instance;
}

void NapiNearlinkDataTransferCallback::OnConnectionStateChanged(const ConnectionParams &result)
{
    HILOGI("enter, state: %{public}d", result.GetState());
    std::lock_guard<std::mutex> lock(callbackMutex_);
    auto napiNative = std::make_shared<ConnectionResult>(result.GetAddress(), result.GetUuid(), 0, result.GetState());
    eventSubscribe.PublishEvent(SLE_DATATRANSFER_CALLBACK_CONNECTION_STATE_CHANGE, napiNative);
}

void NapiNearlinkDataTransferCallback::OnReceiveData(const DataParams &result)
{
    HILOGI("enter");
    std::lock_guard<std::mutex> lock(callbackMutex_);
    auto napiNative = std::make_shared<DataResult>(result);
    eventSubscribe.PublishEvent(SLE_DATATRANSFER_CALLBACK_READ_DATA, napiNative);
}
} // namespace Nearlink
} // namespace OHOS