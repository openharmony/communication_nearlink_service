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

#include "napi_nearlink_constant.h"
#include "log_util.h"
#include "napi_nearlink_utils.h"
#include "napi_ha_manager.h"

namespace OHOS {
namespace Nearlink {
napi_value NapiNearlinkConstant::DefineJSConstant(napi_env env, napi_value exports)
{
    HILOGD("enter");
    ConstantPropertyValueInit(env, exports);
    napi_property_descriptor desc[] = {};
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    HILOGI("end");
    return exports;
}

napi_value NapiNearlinkConstant::ConstantPropertyValueInit(napi_env env, napi_value exports)
{
    HILOGD("enter");
    napi_value connectionStateObj = ConnectionStateInit(env);
    napi_value pairState = PairingStateInit(env);
    napi_value acbState = AcbStateInit(env);
    napi_value connectionInterval = ConnectionIntervalInit(env);
    napi_value deviceClass = DeviceClassInit(env);
    napi_property_descriptor exportFuncs[] = {
        DECLARE_NAPI_PROPERTY("ConnectionState", connectionStateObj),
        DECLARE_NAPI_PROPERTY("PairingState", pairState),
        DECLARE_NAPI_PROPERTY("AcbState", acbState),
        DECLARE_NAPI_PROPERTY("ConnectionInterval", connectionInterval),
        DECLARE_NAPI_PROPERTY("DeviceClass", deviceClass),
    };
    napi_define_properties(env, exports, sizeof(exportFuncs) / sizeof(*exportFuncs), exportFuncs);
    HILOGI("end");
    return exports;
}

napi_value NapiNearlinkConstant::ConnectionStateInit(napi_env env)
{
    HILOGD("enter");
    napi_value connectionState = nullptr;
    napi_create_object(env, &connectionState);
    SetNamedPropertyByInteger(env, connectionState,
        static_cast<int>(ConnectionState::STATE_CONNECTING), "STATE_CONNECTING");
    SetNamedPropertyByInteger(env, connectionState,
        static_cast<int>(ConnectionState::STATE_CONNECTED), "STATE_CONNECTED");
    SetNamedPropertyByInteger(env, connectionState,
        static_cast<int>(ConnectionState::STATE_DISCONNECTING), "STATE_DISCONNECTING");
    SetNamedPropertyByInteger(env, connectionState,
        static_cast<int>(ConnectionState::STATE_DISCONNECTED), "STATE_DISCONNECTED");
    return connectionState;
}

napi_value NapiNearlinkConstant::PairingStateInit(napi_env env)
{
    HILOGD("enter");
    napi_value pairState = nullptr;
    napi_create_object(env, &pairState);
    SetNamedPropertyByInteger(env, pairState,
        static_cast<int>(PairingState::PAIRING_STATE_NONE), "PAIRING_STATE_NONE");
    SetNamedPropertyByInteger(env, pairState,
        static_cast<int>(PairingState::PAIRING_STATE_PAIRING), "PAIRING_STATE_PAIRING");
    SetNamedPropertyByInteger(env, pairState,
        static_cast<int>(PairingState::PAIRING_STATE_PAIRED), "PAIRING_STATE_PAIRED");
    return pairState;
}

napi_value NapiNearlinkConstant::AcbStateInit(napi_env env)
{
    HILOGD("enter");
    napi_value acbState = nullptr;
    napi_create_object(env, &acbState);
    SetNamedPropertyByInteger(env, acbState,
        static_cast<int>(AcbState::DISCONNECTED), "DISCONNECTED");
    SetNamedPropertyByInteger(env, acbState,
        static_cast<int>(AcbState::CONNECTED), "CONNECTED");
    SetNamedPropertyByInteger(env, acbState,
        static_cast<int>(AcbState::ENCRYPTED), "ENCRYPTED");
    return acbState;
}

napi_value NapiNearlinkConstant::ConnectionIntervalInit(napi_env env)
{
    HILOGD("enter");
    napi_value connectionInterval = nullptr;
    napi_create_object(env, &connectionInterval);
    SetNamedPropertyByInteger(env,
        connectionInterval,
        static_cast<int>(ConnectionInterval::HIGH_SPEED_INTERVAL_4_5),
        "HIGH_SPEED_INTERVAL_4_5");
    SetNamedPropertyByInteger(env,
        connectionInterval,
        static_cast<int>(ConnectionInterval::HIGH_SPEED_INTERVAL_4_875),
        "HIGH_SPEED_INTERVAL_4_875");
    SetNamedPropertyByInteger(env,
        connectionInterval,
        static_cast<int>(ConnectionInterval::MID_SPEED_INTERVAL_11_25),
        "MID_SPEED_INTERVAL_11_25");
    SetNamedPropertyByInteger(
        env, connectionInterval, static_cast<int>(ConnectionInterval::MID_SPEED_INTERVAL_15), "MID_SPEED_INTERVAL_15");
    SetNamedPropertyByInteger(
        env, connectionInterval, static_cast<int>(ConnectionInterval::MID_SPEED_INTERVAL_50), "MID_SPEED_INTERVAL_50");
    SetNamedPropertyByInteger(env,
        connectionInterval,
        static_cast<int>(ConnectionInterval::LOW_SPEED_INTERVAL_100),
        "LOW_SPEED_INTERVAL_100");
    SetNamedPropertyByInteger(env,
        connectionInterval,
        static_cast<int>(ConnectionInterval::LOW_SPEED_INTERVAL_150),
        "LOW_SPEED_INTERVAL_150");
    SetNamedPropertyByInteger(env,
        connectionInterval,
        static_cast<int>(ConnectionInterval::LOW_SPEED_INTERVAL_200),
        "LOW_SPEED_INTERVAL_200");
    SetNamedPropertyByInteger(env,
        connectionInterval,
        static_cast<int>(ConnectionInterval::LOW_SPEED_INTERVAL_300),
        "LOW_SPEED_INTERVAL_300");
    SetNamedPropertyByInteger(env,
        connectionInterval,
        static_cast<int>(ConnectionInterval::LOW_SPEED_INTERVAL_500),
        "LOW_SPEED_INTERVAL_500");
    return connectionInterval;
}

napi_value NapiNearlinkConstant::DeviceClassInit(napi_env env)
{
    HILOGD("enter");
    napi_value deviceClass = nullptr;
    napi_create_object(env, &deviceClass);
    UsableDeviceClassInit(env, deviceClass);
    AudioDeviceClassInit(env, deviceClass);
    DailyUsedDeviceClassInit(env, deviceClass);
    IntelligentDeviceClassInit(env, deviceClass);
    return deviceClass;
}

void NapiNearlinkConstant::UsableDeviceClassInit(napi_env env, napi_value deviceClass)
{
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_INVALID_CLASS), "DEVICE_INVALID_CLASS");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_UNCATEGORIZED), "DEVICE_UNCATEGORIZED");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_PHONE), "DEVICE_PHONE");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_SMARTPHONE), "DEVICE_SMARTPHONE");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_COMPUTER), "DEVICE_COMPUTER");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_LAPTOP), "DEVICE_LAPTOP");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_TABLET), "DEVICE_TABLET");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_ALL_IN_ONE_COMPUTER), "DEVICE_ALL_IN_ONE_COMPUTER");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_MINI_PC), "DEVICE_MINI_PC");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_WATCH), "DEVICE_WATCH");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_SMART_WATCH), "DEVICE_SMART_WATCH");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_HUMAN_INTERFACE), "DEVICE_HUMAN_INTERFACE");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_KEYBOARD), "DEVICE_KEYBOARD");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_MOUSE), "DEVICE_MOUSE");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_HANDLE), "DEVICE_HANDLE");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_STYLUS), "DEVICE_STYLUS");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_TOUCHPAD), "DEVICE_TOUCHPAD");
}

void NapiNearlinkConstant::AudioDeviceClassInit(napi_env env, napi_value deviceClass)
{
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_AUDIO_PLAYBACK), "DEVICE_AUDIO_PLAYBACK");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_SMART_SPEAKER), "DEVICE_SMART_SPEAKER");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_ECHO_WALL), "DEVICE_ECHO_WALL");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_AUDIO_CAPTURE), "DEVICE_AUDIO_CAPTURE");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_KARAOKE_MICROPHONE), "DEVICE_KARAOKE_MICROPHONE");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_LAPEL_MICROPHONE), "DEVICE_LAPEL_MICROPHONE");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_WEARABLE_AUDIO), "DEVICE_WEARABLE_AUDIO");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_IN_EAR_EARPHONE), "DEVICE_IN_EAR_EARPHONE");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_HEADSET), "DEVICE_HEADSET");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_OVER_EAR_HEADPHONE), "DEVICE_OVER_EAR_HEADPHONE");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_NECKBAND_EARPHONE), "DEVICE_NECKBAND_EARPHONE");
}

void NapiNearlinkConstant::DailyUsedDeviceClassInit(napi_env env, napi_value deviceClass)
{
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_PERSONAL_CARE), "DEVICE_PERSONAL_CARE");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_INTELLIGENT_TOOTHBRUSH), "DEVICE_INTELLIGENT_TOOTHBRUSH");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_SMART_CUP), "DEVICE_SMART_CUP");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_INTELLIGENT_SHAVER), "DEVICE_INTELLIGENT_SHAVER");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_HVAC), "DEVICE_HVAC");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_AIR_PURIFIER), "DEVICE_AIR_PURIFIER");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_HUMIDIFIER), "DEVICE_HUMIDIFIER");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_AIR_CIRCULATION_FAN), "DEVICE_AIR_CIRCULATION_FAN");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_ELECTRIC_RIDE), "DEVICE_ELECTRIC_RIDE");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_ELECTRIC_SCOOTER), "DEVICE_ELECTRIC_SCOOTER");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_ELECTRIC_BICYCLE), "DEVICE_ELECTRIC_BICYCLE");
}

void NapiNearlinkConstant::IntelligentDeviceClassInit(napi_env env, napi_value deviceClass)
{
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_LIGHT_FITTING), "DEVICE_LIGHT_FITTING");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_SMART_TABLE_LAMP), "DEVICE_SMART_TABLE_LAMP");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_REMOTE_CONTROL), "DEVICE_REMOTE_CONTROL");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_TV_REMOTE_CONTROL), "DEVICE_TV_REMOTE_CONTROL");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_IMAGING), "DEVICE_IMAGING");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_SMART_TV), "DEVICE_SMART_TV");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_IP_CAMERA), "DEVICE_IP_CAMERA");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_SCREEN_CASTER), "DEVICE_SCREEN_CASTER");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_NETWORKING), "DEVICE_NETWORKING");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_IOT_GATEWAY), "DEVICE_IOT_GATEWAY");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_ACCESS_CONTROL), "DEVICE_ACCESS_CONTROL");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_INTELLIGENT_LOCK), "DEVICE_INTELLIGENT_LOCK");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_SMART_KEY), "DEVICE_SMART_KEY");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_VEHICLE_KEY), "DEVICE_VEHICLE_KEY");
    SetNamedPropertyByInteger(env, deviceClass,
        static_cast<int>(DeviceClass::DEVICE_VEHICLE_LOCK), "DEVICE_VEHICLE_LOCK");
}

EXTERN_C_START
/*
 * Module initialization function
 */
static napi_value Init(napi_env env, napi_value exports)
{
    HILOGI("-----Constant Init start------");
    napi_property_descriptor desc[] = {};
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    NapiNearlinkConstant::DefineJSConstant(env, exports);
    HILOGI("-----Constant Init end------");
    return exports;
}
EXTERN_C_END

/*
 * Module define
 */
static napi_module nearlinkConstantModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = NULL,
    .nm_register_func = Init,
    .nm_modname = "nearlink.constant",
    .nm_priv = ((void *)0),
    .reserved = {0}};

/*
 * Module register function
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    HILOGI("Register nearlinkConstantModule nm_modname:%{public}s", nearlinkConstantModule.nm_modname);
    napi_module_register(&nearlinkConstantModule);
}
}  // namespace Nearlink
}  // namespace OHOS
