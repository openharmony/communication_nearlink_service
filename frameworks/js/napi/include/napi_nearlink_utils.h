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
#ifndef NAPI_NEARLINK_UTILS_H
#define NAPI_NEARLINK_UTILS_H

#include "log.h"
#include "nearlink_remote_device.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <cstdint>
#include <string>
#include <vector>

#include "uv.h"

namespace OHOS {
namespace Nearlink {
constexpr size_t ARGS_SIZE_ZERO = 0;
constexpr size_t ARGS_SIZE_ONE = 1;
constexpr size_t ARGS_SIZE_TWO = 2;
constexpr size_t ARGS_SIZE_THREE = 3;
constexpr size_t ARGS_SIZE_FOUR = 4;
constexpr size_t ARGS_SIZE_FIVE = 5;
constexpr int32_t PARAM0 = 0;
constexpr int32_t PARAM1 = 1;
constexpr int32_t PARAM2 = 2;
constexpr int32_t PARAM3 = 3;
constexpr int32_t PARAM4 = 4;
constexpr int HA_RESULT_SUCCESS = 0;
constexpr int HA_RESULT_FAIL = 1;
const std::string REGISTER_STATE_CHANGE_TYPE = "stateChange";

bool ParseString(napi_env env, std::string &param, napi_value args);
bool ParseArrayBuffer(napi_env env, uint8_t **data, size_t &size, napi_value args);
bool ParseInt32(napi_env env, int32_t &param, napi_value args);
bool ParseBool(napi_env env, bool &param, napi_value args);
void ConvertStrVectorToJS(napi_env env, napi_value result, const std::vector<std::string> &strVector);
void SetNamedPropertyByInteger(napi_env env, napi_value dstObj, int32_t objName, const char *propName);
napi_value NapiGetNull(napi_env env);
napi_value NapiGetBooleanFalse(napi_env env);
napi_value NapiGetBooleanTrue(napi_env env);
napi_value NapiGetUndefinedRet(napi_env env);

struct AsyncPromiseContext {
    napi_env env_;
    napi_async_work asyncWork_;
    napi_deferred deferred_;
    int errorCode_ = 0;
};

struct NearlinkCallbackContext {
    napi_env env_;
    napi_ref callback_ = nullptr;
    int state_;
    std::string deviceId_;
    int info_;
};

enum class AdevertiseMode {
    ADV_TX_POWER_LOW = 1,  //  low-power mode
    ADV_TX_POWER_MEDIUM = 2,       // medium-power mode
    ADV_TX_POWER_HIGH = 3       // high-power mode
};

enum class AdvertiseType {
    ADV_NOT_CONNECTABLE = 0, //  advertis is not connectable
    ADV_CONNECTABLE = 1,  //  advertis is connectable
};

enum class AdvertisingState {
    STARTED = 1, //  advertising state is started
    STOPPED = 2,  //  advertising state is stopped
};

enum class ScanDuty {
    SCAN_MODE_LOW_POWER = 0,   // low power mode */
    SCAN_MODE_BALANCED = 1,    // balanced power mode
    SCAN_MODE_LOW_LATENCY = 2  // Scan using highest duty cycle
};

enum class FrameType {
    FRAME_TYPE_1 = 1,  // 无线帧类型 1
    FRAME_TYPE_4 = 4   // 无线帧类型 4
};

struct ScanOptions {
    int32_t interval = 0;                                   // Time of delay for reporting the scan result
    ScanDuty dutyMode = ScanDuty::SCAN_MODE_LOW_POWER;      // Nearlink LE scan mode
    int32_t duration = 0;                                   // scan duration, 0 for never stop
    FrameType frameType = FrameType::FRAME_TYPE_1;          // Frame type, default frame 1
    bool hasFrameType = false;                              // Flag to indicate if frameType is explicitly set
};

struct NapiAdvManufactureData {
    uint16_t id = 0;
    std::string value {};
};

struct NapiAdvServiceData {
    std::string uuid {};
    std::vector<uint8_t> value {};
};

enum class ConnectionState {
    STATE_CONNECTING = 0,    // the current profile is being connected
    STATE_CONNECTED = 1,     // the current profile is connected
    STATE_DISCONNECTING = 2,  // the current profile is being disconnected
    STATE_DISCONNECTED = 3  // the current profile is disconnected
};

enum class PairingState {
    PAIRING_STATE_NONE = 1,        // Indicate the pair state is none.
    PAIRING_STATE_PAIRING = 2,     // Indicate the pair state is pairing.
    PAIRING_STATE_PAIRED = 3,       // Indicate the pair state is paired.
};

enum class DeviceClass {
    DEVICE_INVALID_CLASS = -1,                  // Invalid device class. Missing device class information.
    DEVICE_UNCATEGORIZED = 0x000100,            // Unclassified device.
    DEVICE_PHONE = 0x000200,                    // General phone.
    DEVICE_SMARTPHONE = 0x000201,               // Smartphone.
    DEVICE_COMPUTER = 0x000300,                 // General computer.
    DEVICE_LAPTOP = 0x000301,                   // Laptop.
    DEVICE_TABLET = 0x000302,                   // Tablet.
    DEVICE_ALL_IN_ONE_COMPUTER = 0x000303,      // All-in-one computer.
    DEVICE_MINI_PC = 0x000304,                  // Mini PC.
    DEVICE_WATCH = 0x000400,                    // General watch.
    DEVICE_SMART_WATCH = 0x000401,               // Smartwatch.
    DEVICE_HUMAN_INTERFACE = 0x000500,          // General human interface device.
    DEVICE_KEYBOARD = 0x000501,                 // Keyboard.
    DEVICE_MOUSE = 0x000502,                    // Mouse.
    DEVICE_HANDLE = 0x000503,                   // Handle.
    DEVICE_STYLUS = 0x000504,               // Stylus pen.
    DEVICE_TOUCHPAD = 0x000505,                 // Touchpad.
    DEVICE_AUDIO_PLAYBACK = 0x000600,           // General audio playback device.
    DEVICE_SMART_SPEAKER = 0x000601,            // Smart speaker.
    DEVICE_ECHO_WALL = 0x000602,                // Echo Wall.
    DEVICE_AUDIO_CAPTURE = 0x000700,            // General audio capture device.
    DEVICE_KARAOKE_MICROPHONE = 0x000701,       // Karaoke microphone.
    DEVICE_LAPEL_MICROPHONE = 0x000702,         // Lapel Microphone.
    DEVICE_WEARABLE_AUDIO = 0x000800,           // General wearable audio device.
    DEVICE_IN_EAR_EARPHONE = 0x000801,          // In-ear earphone.
    DEVICE_HEADSET = 0x000802,                  // Headset.
    DEVICE_OVER_EAR_HEADPHONE = 0x000803,       // Over-ear headphone.
    DEVICE_NECKBAND_EARPHONE = 0x000804,       // Neck-worn earphone.
    DEVICE_PERSONAL_CARE = 0x000900,            // General personal care.
    DEVICE_INTELLIGENT_TOOTHBRUSH = 0x000901,   // Intelligent toothbrush.
    DEVICE_SMART_CUP = 0x000902,                // Smart cup.
    DEVICE_INTELLIGENT_SHAVER = 0x000903,       // Intelligent shaver.
    DEVICE_HVAC = 0x000A00,                     // General HVAC.
    DEVICE_AIR_PURIFIER = 0x000A01,             // Air purifier.
    DEVICE_HUMIDIFIER = 0x000A02,               // Humidifier.
    DEVICE_AIR_CIRCULATION_FAN = 0x000A03,      // Air circulation fan.
    DEVICE_ELECTRIC_RIDE = 0x000B00,          // General electric riding.
    DEVICE_ELECTRIC_SCOOTER = 0x000B01,         // Electric scooter.
    DEVICE_ELECTRIC_BICYCLE = 0x000B02,         // Electric bicycle.
    DEVICE_LIGHT_FITTING = 0x000C00,            // General light fitting.
    DEVICE_SMART_TABLE_LAMP = 0x000C01,         // Smart table lamp.
    DEVICE_REMOTE_CONTROL = 0x000D00,           // General remote control.
    DEVICE_TV_REMOTE_CONTROL = 0x000D01,        // TV remote control.
    DEVICE_IMAGING = 0x000E00,                  // General imaging device.
    DEVICE_SMART_TV = 0x000E01,                 // Smart TV.
    DEVICE_IP_CAMERA = 0x000E02,                // IP camera.
    DEVICE_SCREEN_CASTER = 0x000E03,            // Screen caster.
    DEVICE_NETWORKING = 0x000F00,               // General network device.
    DEVICE_IOT_GATEWAY = 0x000F01,              // IoT gateway.
    DEVICE_ACCESS_CONTROL = 0x001000,           // General access control.
    DEVICE_INTELLIGENT_LOCK = 0x001001,         // Intelligent Lock.
    DEVICE_SMART_KEY = 0x001002,                // Smart key.
    DEVICE_VEHICLE_KEY  = 0x001003,             // Vehicle key.
    DEVICE_VEHICLE_LOCK  = 0x001004             // Vehicle lock.
};

enum class AcbState {
    DISCONNECTED = 0,   // the current acb is disconnected
    CONNECTED = 1,      // the current acb is connected
    ENCRYPTED = 2,      // the current acb is encrypted
};

enum class TransferMode {
    BASIC = 0,         // datatransfer basic mode
    RELIABLE = 1,      // datatransfer reliable mode
};

#define NAPI_NL_CALL_RETURN(func)                                          \
    do {                                                                   \
        napi_status ret = (func);                                          \
        if (ret != napi_ok) {                                              \
            HILOGE("napi call function failed. ret:%{public}d", ret);      \
            return ret;                                                    \
        }                                                                  \
    } while (0)

#define NAPI_NL_RETURN_IF(condition, msg, ret)              \
    do {                                                    \
        if ((condition)) {                                  \
            HILOGE(msg);                                    \
            return (ret);                                   \
        }                                                   \
    } while (0)

int DoInJsMainThread(napi_env env, std::function<void(void)> func, std::string taskname);

napi_status NapiIsBoolean(napi_env env, napi_value value);
napi_status NapiIsNumber(napi_env env, napi_value value);
napi_status NapiIsString(napi_env env, napi_value value);
napi_status NapiIsFunction(napi_env env, napi_value value);
napi_status NapiIsArray(napi_env env, napi_value value);
napi_status NapiIsArrayBuffer(napi_env env, napi_value value);
napi_status NapiIsObject(napi_env env, napi_value value);
bool NapiIsObjectPropertyExist(napi_env env, napi_value object, const char *name);
napi_status CheckEmptyParam(napi_env env, napi_callback_info info);
napi_status NapiCheckObjectPropertiesName(napi_env env, napi_value object, const std::vector<std::string> &names);
int NapiToJsPairState(int state);
int NapiToJsAcbState(int state);
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NAPI_NEARLINK_UTILS_H
