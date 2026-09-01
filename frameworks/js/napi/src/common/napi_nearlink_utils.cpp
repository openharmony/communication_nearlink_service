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
#include <vector>
#include "nearlink_errorcode.h"
#include "log_util.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_nearlink_error.h"
#include "napi_parser_utils.h"
#include "securec.h"

namespace OHOS {
namespace Nearlink {

namespace {
constexpr size_t NAPI_ARRAY_MAX_LENGTH = 0xFFFF;

const std::vector<int> DEVICE_CLASS_VALUES = {
    static_cast<int>(DeviceClass::DEVICE_INVALID_CLASS),
    static_cast<int>(DeviceClass::DEVICE_UNCATEGORIZED),
    static_cast<int>(DeviceClass::DEVICE_PHONE),
    static_cast<int>(DeviceClass::DEVICE_SMARTPHONE),
    static_cast<int>(DeviceClass::DEVICE_COMPUTER),
    static_cast<int>(DeviceClass::DEVICE_LAPTOP),
    static_cast<int>(DeviceClass::DEVICE_TABLET),
    static_cast<int>(DeviceClass::DEVICE_ALL_IN_ONE_COMPUTER),
    static_cast<int>(DeviceClass::DEVICE_MINI_PC),
    static_cast<int>(DeviceClass::DEVICE_WATCH),
    static_cast<int>(DeviceClass::DEVICE_SMART_WATCH),
    static_cast<int>(DeviceClass::DEVICE_HUMAN_INTERFACE),
    static_cast<int>(DeviceClass::DEVICE_KEYBOARD),
    static_cast<int>(DeviceClass::DEVICE_MOUSE),
    static_cast<int>(DeviceClass::DEVICE_HANDLE),
    static_cast<int>(DeviceClass::DEVICE_STYLUS),
    static_cast<int>(DeviceClass::DEVICE_TOUCHPAD),
    static_cast<int>(DeviceClass::DEVICE_AUDIO_PLAYBACK),
    static_cast<int>(DeviceClass::DEVICE_SMART_SPEAKER),
    static_cast<int>(DeviceClass::DEVICE_ECHO_WALL),
    static_cast<int>(DeviceClass::DEVICE_AUDIO_CAPTURE),
    static_cast<int>(DeviceClass::DEVICE_KARAOKE_MICROPHONE),
    static_cast<int>(DeviceClass::DEVICE_LAPEL_MICROPHONE),
    static_cast<int>(DeviceClass::DEVICE_WEARABLE_AUDIO),
    static_cast<int>(DeviceClass::DEVICE_IN_EAR_EARPHONE),
    static_cast<int>(DeviceClass::DEVICE_HEADSET),
    static_cast<int>(DeviceClass::DEVICE_OVER_EAR_HEADPHONE),
    static_cast<int>(DeviceClass::DEVICE_NECKBAND_EARPHONE),
    static_cast<int>(DeviceClass::DEVICE_PERSONAL_CARE),
    static_cast<int>(DeviceClass::DEVICE_INTELLIGENT_TOOTHBRUSH),
    static_cast<int>(DeviceClass::DEVICE_SMART_CUP),
    static_cast<int>(DeviceClass::DEVICE_INTELLIGENT_SHAVER),
    static_cast<int>(DeviceClass::DEVICE_HVAC),
    static_cast<int>(DeviceClass::DEVICE_AIR_PURIFIER),
    static_cast<int>(DeviceClass::DEVICE_HUMIDIFIER),
    static_cast<int>(DeviceClass::DEVICE_AIR_CIRCULATION_FAN),
    static_cast<int>(DeviceClass::DEVICE_ELECTRIC_RIDE),
    static_cast<int>(DeviceClass::DEVICE_ELECTRIC_SCOOTER),
    static_cast<int>(DeviceClass::DEVICE_ELECTRIC_BICYCLE),
    static_cast<int>(DeviceClass::DEVICE_LIGHT_FITTING),
    static_cast<int>(DeviceClass::DEVICE_SMART_TABLE_LAMP),
    static_cast<int>(DeviceClass::DEVICE_REMOTE_CONTROL),
    static_cast<int>(DeviceClass::DEVICE_TV_REMOTE_CONTROL),
    static_cast<int>(DeviceClass::DEVICE_IMAGING),
    static_cast<int>(DeviceClass::DEVICE_SMART_TV),
    static_cast<int>(DeviceClass::DEVICE_IP_CAMERA),
    static_cast<int>(DeviceClass::DEVICE_SCREEN_CASTER),
    static_cast<int>(DeviceClass::DEVICE_NETWORKING),
    static_cast<int>(DeviceClass::DEVICE_IOT_GATEWAY),
    static_cast<int>(DeviceClass::DEVICE_ACCESS_CONTROL),
    static_cast<int>(DeviceClass::DEVICE_INTELLIGENT_LOCK),
    static_cast<int>(DeviceClass::DEVICE_SMART_KEY),
    static_cast<int>(DeviceClass::DEVICE_VEHICLE_KEY),
    static_cast<int>(DeviceClass::DEVICE_VEHICLE_LOCK),
};
}  // namespace

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
    int jsPairState = static_cast<int>(PairingState::PAIRING_STATE_NONE);
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
        default:
            HILOGE("Pair state is outside of expectations.");
            jsPairState = static_cast<int>(PairingState::PAIRING_STATE_NONE);
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

int NapiToJsConnState(int state)
{
    int jsConnState = static_cast<int>(ConnectionState::STATE_DISCONNECTED);
    switch (state) {
        case static_cast<int>(SleConnectState::CONNECTING):
            jsConnState = static_cast<int>(ConnectionState::STATE_CONNECTING);
            break;
        case static_cast<int>(SleConnectState::CONNECTED):
            jsConnState = static_cast<int>(ConnectionState::STATE_CONNECTED);
            break;
        case static_cast<int>(SleConnectState::DISCONNECTING):
            jsConnState = static_cast<int>(ConnectionState::STATE_DISCONNECTING);
            break;
        case static_cast<int>(SleConnectState::DISCONNECTED):
            jsConnState = static_cast<int>(ConnectionState::STATE_DISCONNECTED);
            break;
        default:
            HILOGE("Conn state is outside of expectations.");
            jsConnState = static_cast<int>(ConnectionState::STATE_DISCONNECTED);
            break;
    }
    return jsConnState;
}

int NapiToJsDeviceClass(int appearance)
{
    if (std::find(DEVICE_CLASS_VALUES.begin(), DEVICE_CLASS_VALUES.end(), appearance) !=
        DEVICE_CLASS_VALUES.end()) {
        return appearance;
    }
    HILOGE("Device class is outside of expectations.");
    return static_cast<int>(DeviceClass::DEVICE_INVALID_CLASS);
}

std::string NapiToJsPairReasonMsg(int reason)
{
    switch (reason) {
        case 0:  // PairingReason::PAIRING_SUCCESS
            return "Pairing successful";
        case 1:  // PairingReason::PAIRING_FAILURE
            return "Pairing failed";
        case 2:  // PairingReason::PAIRING_ACB_CONNECTION_FAILED
            return "ACB connection failed";
        case 3:  // PairingReason::PAIRING_EXCEED_ACB_MAX
            return "The number of ACB connections exceeded the maximum";
        case 4:  // PairingReason::PAIRING_REMOTE_CANCELED
            return "Pairing canceled by the remote device";
        case 5:  // PairingReason::PAIRING_LOCAL_CANCELED
            return "Pairing canceled locally";
        case 6:  // PairingReason::PAIRING_AUTH_FAILED
            return "Pairing authentication failed";
        default:
            return "";
    }
}

std::string NapiToJsConnReasonMsg(int reason)
{
    switch (reason) {
        case 0:  // ConnectionReason::CONNECTION_SUCCESS
            return "Connection successful";
        case 1:  // ConnectionReason::CONNECTION_FAIL
            return "Connection failed";
        case 2:  // ConnectionReason::CONNECTION_LOCAL_DISCONNECT
            return "Disconnected locally";
        case 3:  // ConnectionReason::CONNECTION_REMOTE_DISCONNECT
            return "Disconnected by the remote device";
        case 4:  // ConnectionReason::CONNECTION_FAIL_ACB_CONNECTION
            return "Connection failed due to ACB connection failure";
        case 5:  // ConnectionReason::CONNECTION_FAIL_SERVICE_DISCOVERY
            return "Connection failed due to service discovery failure";
        case 6:  // ConnectionReason::CONNECTION_FAIL_NO_AVAILABLE_SERVICE
            return "Connection failed because no available service";
        case 7:  // ConnectionReason::CONNECTION_FAIL_CONNECTION_NUM_LIMITED
            return "Connection failed due to the connection number limit";
        default:
            return "";
    }
}
}  // namespace Nearlink
}  // namespace OHOS
