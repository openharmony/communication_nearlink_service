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
#include "napi_nearlink_scan_callback.h"

#include "log_util.h"
#include "securec.h"

namespace OHOS {
namespace Nearlink {
void ConvertScanResult(const std::vector<SleScanResult> &results, const napi_env &env, napi_value &scanResultArray)
{
    HILOGI("enter");
    napi_create_array(env, &scanResultArray);
    size_t count = 0;
    for (auto sleScanResult : results) {
        napi_value result = nullptr;
        napi_value value = nullptr;
        napi_create_object(env, &result);
        napi_create_string_utf8(
            env, sleScanResult.GetPeripheralDevice().GetDeviceAddr().c_str(), NAPI_AUTO_LENGTH, &value);
        std::string propertyName = "address";
        napi_set_named_property(env, result, propertyName.c_str(), value);
        napi_create_int32(env, sleScanResult.GetRssi(), &value);
        napi_set_named_property(env, result, "rssi", value);
        uint8_t *native = nullptr;
        napi_value buffer = nullptr;
        napi_create_arraybuffer(env, sleScanResult.GetPayload().size(), reinterpret_cast<void **>(&native), &buffer);
        if (memcpy_s(native, sleScanResult.GetPayload().size(), sleScanResult.GetPayload().data(),
            sleScanResult.GetPayload().size()) != EOK) {
            HILOGE("ConvertScanResult memcpy_s fail");
            return;
        }
        napi_create_typedarray(env, napi_uint8_array, sleScanResult.GetPayload().size(), buffer, 0, &value);
        napi_set_named_property(env, result, "data", value);
        napi_create_string_utf8(env, sleScanResult.GetName().c_str(), NAPI_AUTO_LENGTH, &value);
        napi_set_named_property(env, result, "deviceName", value);
        napi_get_boolean(env, sleScanResult.IsConnectable(), &value);
        napi_set_named_property(env, result, "isConnectable", value);
        napi_set_element(env, scanResultArray, count, result);
        ++count;
    }
}

napi_value NapiNativeScanResult::ToNapiValue(napi_env env) const
{
    napi_value object = 0;
    ConvertScanResult(scanResult_, env, object);
    return object;
}

NapiNearlinkCentralManagerCallback::NapiNearlinkCentralManagerCallback()
    : eventSubscribe(REGISTER_SCAN_DEVICE_NAME, NL_MODULE_NAME)
{}

std::shared_ptr<NapiNearlinkCentralManagerCallback> NapiNearlinkCentralManagerCallback::GetInstance(void)
{
    static std::shared_ptr<NapiNearlinkCentralManagerCallback> callback =
        std::make_shared<NapiNearlinkCentralManagerCallback>();
    return callback;
}

void NapiNearlinkCentralManagerCallback::OnScanCallback(const SleScanResult &result)
{
    HILOGD("enter, remote device address: %{public}s", GET_ENCRYPT_DEVICE_ADDR(result.GetPeripheralDevice()));
    std::lock_guard<std::mutex> lock(callbackMutex_);
    std::vector<SleScanResult> ret {result};
    auto napiNative = std::make_shared<NapiNativeScanResult>(ret);

    eventSubscribe.PublishEvent(REGISTER_SCAN_DEVICE_NAME, napiNative);
}

void NapiNearlinkCentralManagerCallback::OnSleBatchScanResultsEvent(const std::vector<SleScanResult> &results)
{
    HILOGE("not implement");
}

void NapiNearlinkCentralManagerCallback::OnStartOrStopScanEvent(int resultCode, bool isStartScan)
{
    HILOGE("not implement");
}

}  // namespace Nearlink
}  // namespace OHOS
