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

#include "napi_nearlink_utils.h"
#include <algorithm>
#include <functional>
#include <optional>
#include "nearlink_errorcode.h"
#include "log_util.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_nearlink_error.h"
#include "napi_parser_utils.h"
#include "securec.h"

namespace OHOS {
namespace Nearlink {
constexpr size_t NAPI_ARRAY_MAX_LENGTH = 0xFFFF;

using namespace std;
bool ParseString(napi_env env, string &param, napi_value args)
{
    napi_valuetype valuetype;
    napi_typeof(env, args, &valuetype);

    if (valuetype != napi_string) {
        HILOGE("Wrong argument type(%{public}d). String expected.", valuetype);
        return false;
    }
    size_t size = 0;

    if (napi_get_value_string_utf8(env, args, nullptr, 0, &size) != napi_ok) {
        HILOGE("can not get string size");
        param = "";
        return false;
    }
    param.reserve(size + 1);
    param.resize(size);
    if (napi_get_value_string_utf8(env, args, param.data(), (size + 1), &size) != napi_ok) {
        HILOGE("can not get string value");
        param = "";
        return false;
    }
    return true;
}

bool ParseInt32(napi_env env, int32_t &param, napi_value args)
{
    napi_valuetype valuetype;
    napi_typeof(env, args, &valuetype);
    if (valuetype != napi_number) {
        HILOGE("Wrong argument type(%{public}d). Int32 expected.", valuetype);
        return false;
    }
    napi_status status = napi_get_value_int32(env, args, &param);
    NAPI_NL_RETURN_IF(status != napi_ok, "Failed to napi_get_value_int32", false);
    return true;
}

bool ParseBool(napi_env env, bool &param, napi_value args)
{
    napi_valuetype valuetype;
    napi_typeof(env, args, &valuetype);

    if (valuetype != napi_boolean) {
        HILOGE("Wrong argument type(%{public}d). bool expected.", valuetype);
        return false;
    }
    napi_status status = napi_get_value_bool(env, args, &param);
    NAPI_NL_RETURN_IF(status != napi_ok, "Failed to napi_get_value_bool", false);
    return true;
}


bool ParseArrayBuffer(napi_env env, uint8_t** data, size_t &size, napi_value args)
{
    napi_status status;
    napi_valuetype valuetype;
    napi_typeof(env, args, &valuetype);

    if (valuetype != napi_object) {
        HILOGE("Wrong argument type(%{public}d). object expected.", valuetype);
        return false;
    }

    status = napi_get_arraybuffer_info(env, args, reinterpret_cast<void**>(data), &size);
    if (status != napi_ok) {
        HILOGE("can not get arraybuffer, error is %{public}d", status);
        return false;
    }
    HILOGI("arraybuffer size is %{public}zu", size);
    return true;
}

void ConvertStrVectorToJS(napi_env env, napi_value result, const std::vector<std::string> &strVector)
{
    HILOGI("enter");
    size_t idx = 0;

    if (strVector.empty()) {
        return;
    }
    HILOGI("size: %{public}zu", strVector.size());
    for (auto& str : strVector) {
        napi_value jsStr = nullptr;
        napi_create_string_utf8(env, str.c_str(), NAPI_AUTO_LENGTH, &jsStr);
        napi_set_element(env, result, idx, jsStr);
        idx++;
    }
}

void SetNamedPropertyByInteger(napi_env env, napi_value dstObj, int32_t objName, const char *propName)
{
    napi_value prop = nullptr;
    if (napi_create_int32(env, objName, &prop) == napi_ok) {
        napi_set_named_property(env, dstObj, propName, prop);
    }
}

napi_value NapiGetNull(napi_env env)
{
    napi_value result = nullptr;
    napi_get_null(env, &result);
    return result;
}

napi_value NapiGetBooleanFalse(napi_env env)
{
    napi_value result = nullptr;
    napi_get_boolean(env, false, &result);
    return result;
}

napi_value NapiGetBooleanTrue(napi_env env)
{
    napi_value result = nullptr;
    napi_get_boolean(env, true, &result);
    return result;
}

napi_value NapiGetUndefinedRet(napi_env env)
{
    napi_value ret = nullptr;
    napi_get_undefined(env, &ret);
    return ret;
}

struct UvContext {
    std::function<void(void)> func;
};

int DoInJsMainThread(napi_env env, std::function<void(void)> func, std::string taskname)
{
    if (napi_send_event(env, func, napi_eprio_high, taskname.c_str()) != napi_ok) {
        HILOGE("Failed to SendEvent");
        return -1;
    }
    return 0;
}

napi_status NapiIsBoolean(napi_env env, napi_value value)
{
    napi_valuetype valuetype = napi_undefined;
    NAPI_NL_CALL_RETURN(napi_typeof(env, value, &valuetype));
    NAPI_NL_RETURN_IF(valuetype != napi_boolean, "Wrong argument type. Boolean expected.", napi_boolean_expected);
    return napi_ok;
}

napi_status NapiIsNumber(napi_env env, napi_value value)
{
    napi_valuetype valuetype = napi_undefined;
    NAPI_NL_CALL_RETURN(napi_typeof(env, value, &valuetype));
    NAPI_NL_RETURN_IF(valuetype != napi_number, "Wrong argument type. Number expected.", napi_number_expected);
    return napi_ok;
}

napi_status NapiIsString(napi_env env, napi_value value)
{
    napi_valuetype valuetype = napi_undefined;
    NAPI_NL_CALL_RETURN(napi_typeof(env, value, &valuetype));
    NAPI_NL_RETURN_IF(valuetype != napi_string, "Wrong argument type. String expected.", napi_string_expected);
    return napi_ok;
}

napi_status NapiIsFunction(napi_env env, napi_value value)
{
    napi_valuetype valuetype = napi_undefined;
    NAPI_NL_CALL_RETURN(napi_typeof(env, value, &valuetype));
    NAPI_NL_RETURN_IF(valuetype != napi_function, "Wrong argument type. Function expected.", napi_function_expected);
    return napi_ok;
}

napi_status NapiIsArrayBuffer(napi_env env, napi_value value)
{
    bool isArrayBuffer = false;
    NAPI_NL_CALL_RETURN(napi_is_arraybuffer(env, value, &isArrayBuffer));
    NAPI_NL_RETURN_IF(!isArrayBuffer, "Expected arraybuffer type", napi_arraybuffer_expected);
    return napi_ok;
}

napi_status NapiIsArray(napi_env env, napi_value value)
{
    bool isArray = false;
    NAPI_NL_CALL_RETURN(napi_is_array(env, value, &isArray));
    NAPI_NL_RETURN_IF(!isArray, "Expected array type", napi_array_expected);
    return napi_ok;
}

napi_status NapiIsObject(napi_env env, napi_value value)
{
    napi_valuetype valuetype = napi_undefined;
    NAPI_NL_CALL_RETURN(napi_typeof(env, value, &valuetype));
    NAPI_NL_RETURN_IF(valuetype != napi_object, "Wrong argument type. Object expected.", napi_object_expected);
    return napi_ok;
}

// shall check object type is napi_object before, if it's other type, return false
// If the 'name' field is not exist, or napi function call error, return false
bool NapiIsObjectPropertyExist(napi_env env, napi_value object, const char *name)
{
    auto status = NapiIsObject(env, object);
    if (status != napi_ok) {
        HILOGE("expect object");
        return false;
    }
    bool exist = false;
    status = napi_has_named_property(env, object, name, &exist);
    HILOGI("name = %{public}s, exist = %{public}d", name, exist);
    if (status != napi_ok) {
        HILOGE("Get object property failed, name: %{public}s", name);
        return false;
    }
    return exist;
}

napi_status CheckEmptyParam(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_SIZE_ZERO;
    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr));
    NAPI_NL_RETURN_IF(argc != ARGS_SIZE_ZERO, "Requires 0 argument.", napi_invalid_arg);
    return napi_ok;
}

napi_status NapiCheckObjectPropertiesName(napi_env env, napi_value object, const std::vector<std::string> &names)
{
    uint32_t len = 0;
    napi_value properties;
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(napi_get_property_names(env, object, &properties));
    NAPI_NL_CALL_RETURN(napi_get_array_length(env, properties, &len));
    NAPI_NL_RETURN_IF(len > NAPI_ARRAY_MAX_LENGTH, "Array is too long", napi_invalid_arg);
    for (uint32_t i = 0; i < len; ++i) {
        std::string name {};
        napi_value actualName;
        NAPI_NL_CALL_RETURN(napi_get_element(env, properties, i, &actualName));
        NAPI_NL_CALL_RETURN(NapiParseString(env, actualName, name));
        if (std::find(names.begin(), names.end(), name) == names.end()) {
            HILOGE("Unexpect object property name: \"%{public}s\"", name.c_str());
            return napi_invalid_arg;
        }
    }
    return napi_ok;
}

int NapiToJsPairState(int state)
{
    int jsPairState;
    switch (state) {
        case static_cast<int>(SlePairState::SLE_PAIR_NONE):
            jsPairState = static_cast<int>(PairingState::PAIRING_STATE_NONE);
            break;
        case static_cast<int>(SlePairState::SLE_PAIR_PAIRING):
            jsPairState = static_cast<int>(PairingState::PAIRING_STATE_PAIRING);
            break;
        case static_cast<int>(SlePairState::SLE_PAIR_PAIRED):
        case static_cast<int>(SlePairState::SLE_PAIR_CANCELING):
            jsPairState = static_cast<int>(PairingState::PAIRING_STATE_PAIRED);
            break;
    }
    return jsPairState;
}

int NapiToJsAcbState(int state)
{
    int jsAcbState = static_cast<int>(AcbState::DISCONNECTED);
    switch (state) {
        // 链路状态为已断连或连接中，视为已断连
        case static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED):
        case static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING):
            jsAcbState = static_cast<int>(AcbState::DISCONNECTED);
            break;
        // 链路状态为断连中或已连接，视为已连接
        case static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTING):
        case static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED):
            jsAcbState = static_cast<int>(AcbState::CONNECTED);
            break;
        case static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED):
            jsAcbState = static_cast<int>(AcbState::ENCRYPTED);
            break;
        default:
            HILOGE("Acb State is outside of expectations.");
            jsAcbState = static_cast<int>(AcbState::DISCONNECTED);
            break;
    }
    return jsAcbState;
}
}  // namespace Nearlink
}  // namespace OHOS
