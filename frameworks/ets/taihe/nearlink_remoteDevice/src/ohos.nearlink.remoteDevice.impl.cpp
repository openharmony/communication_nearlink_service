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

#include "ohos.nearlink.remoteDevice.proj.hpp"
#include "ohos.nearlink.remoteDevice.impl.hpp"
#include "ohos.nearlink.constant.proj.hpp"
#include "ohos.nearlink.constant.impl.hpp"
#include "ani_nearlink_utils.h"
#include "ani_nearlink_error.h"
#include "ani_nearlink_remote_device_callback.h"
#include "ani_nearlink_remote_device_rssi_observer.h"
#include "taihe/runtime.hpp"
#include "stdexcept"
#include "log.h"
#include "nearlink_errorcode.h"
#include "nearlink_host.h"
#include "nearlink_utils.h"

namespace OHOS {
namespace Nearlink {
using namespace Nearlink;
const size_t DEVICE_NAME_MAX_LENGTH = 64;
const size_t PASS_CODE_LEN = 6;

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

enum class AcbState {
    DISCONNECTED = 0,   // the current acb is disconnected
    CONNECTED = 1,      // the current acb is connected
    ENCRYPTED = 2,      // the current acb is encrypted
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

::ohos::nearlink::constant::PairingState ConvertToAniState(int32_t state)
{
    int pairState = static_cast<int>(PairingState::PAIRING_STATE_NONE);
    switch (state) {
        case static_cast<int>(SlePairState::SLE_PAIR_NONE):
            pairState = static_cast<int>(PairingState::PAIRING_STATE_NONE);
            break;
        case static_cast<int>(SlePairState::SLE_PAIR_PAIRING):
            pairState = static_cast<int>(PairingState::PAIRING_STATE_PAIRING);
            break;
        case static_cast<int>(SlePairState::SLE_PAIR_PAIRED):
        case static_cast<int>(SlePairState::SLE_PAIR_CANCELING):
            pairState = static_cast<int>(PairingState::PAIRING_STATE_PAIRED);
            break;
        default:
            HILOGE("Pair state is outside of expectations.");
            pairState = static_cast<int>(PairingState::PAIRING_STATE_NONE);
            break;
    }
    return ohos::nearlink::constant::PairingState::from_value(pairState);
}

::ohos::nearlink::constant::ConnectionState ConvertToAniConnectionState(int32_t state)
{
    int connState = static_cast<int>(ConnectionState::STATE_DISCONNECTED);
    switch (state) {
        case static_cast<int>(SleConnectState::CONNECTING):
            connState = static_cast<int>(ConnectionState::STATE_CONNECTING);
            break;
        case static_cast<int>(SleConnectState::CONNECTED):
            connState = static_cast<int>(ConnectionState::STATE_CONNECTED);
            break;
        case static_cast<int>(SleConnectState::DISCONNECTING):
            connState = static_cast<int>(ConnectionState::STATE_DISCONNECTING);
            break;
        case static_cast<int>(SleConnectState::DISCONNECTED):
            connState = static_cast<int>(ConnectionState::STATE_DISCONNECTED);
            break;
        default:
            HILOGE("Conn state is outside of expectations.");
            connState = static_cast<int>(ConnectionState::STATE_DISCONNECTED);
            break;
    }
    return ohos::nearlink::constant::ConnectionState::from_value(connState);
}

::ohos::nearlink::constant::AcbState ConvertToAniAcbState(int32_t state)
{
    int acbState = static_cast<int>(AcbState::DISCONNECTED);
    switch (state) {
        case static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED):
        case static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING):
            acbState = static_cast<int>(AcbState::DISCONNECTED);
            break;
        case static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTING):
        case static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED):
            acbState = static_cast<int>(AcbState::CONNECTED);
            break;
        case static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED):
            acbState = static_cast<int>(AcbState::ENCRYPTED);
            break;
        default:
            HILOGE("Acb State is outside of expectations.");
            acbState = static_cast<int>(AcbState::DISCONNECTED);
            break;
    }
    return ohos::nearlink::constant::AcbState::from_value(acbState);
}

bool IsValidPassCode(const std::string& passCode)
{
    if (passCode.length() != PASS_CODE_LEN) {
        return false;
    }
    for (char c : passCode) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}

int ConvertDeviceClass(int appearance)
{
    if (std::find(DEVICE_CLASS_VALUES.begin(), DEVICE_CLASS_VALUES.end(), appearance) !=
        DEVICE_CLASS_VALUES.end()) {
        return appearance;
    }
    HILOGE("Device class is outside of expectations.");
    return static_cast<int>(DeviceClass::DEVICE_INVALID_CLASS);
}

class RemoteDeviceImpl {
public:
    explicit RemoteDeviceImpl(std::string& address)
    {
        HILOGI("enter");
        device_ = std::make_shared<NearlinkRemoteDevice>(address, ADAPTER_SLE);
    }

    ::ohos::nearlink::remoteDevice::DeviceInformation GetDeviceInformation()
    {
        HILOGI("enter");
        ::ohos::nearlink::remoteDevice::DeviceInformation info {};
        DeviceInformation deviceInfo;
        NlErrCode err = device_->GetDeviceInformation(deviceInfo);
        ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err, info);
        info.manufacturerData = static_cast<::taihe::string>(deviceInfo.GetManufacturerData());
        info.modelData = static_cast<::taihe::string>(deviceInfo.GetModelData());
        return info;
    }

    void StartPairing();
    ::ohos::nearlink::constant::PairingState GetPairingState();
    ::taihe::string GetDeviceName();
    ::ohos::nearlink::constant::DeviceClass GetDeviceClass();
    ::ohos::nearlink::constant::ConnectionState GetConnectionState();
    ::ohos::nearlink::constant::AcbState GetAcbState();
    void StartCrediblePairing();
    void RemovePairedDevice();
    void CancelDevicePairing();
    void SetPairingPasscode(::taihe::string_view passcode);
    void SetPairingConfirmation(bool accept);
    void Connect();
    void Disconnect();
    void SetDeviceAlias(::taihe::string_view alias);
    ::taihe::string GetDeviceAlias();
    ::ohos::nearlink::remoteDevice::DeviceModel GetDeviceModel();
    uintptr_t GetRssiValue();
    void SetConnectionInterval(::ohos::nearlink::constant::ConnectionInterval interval);

private:
    std::shared_ptr<NearlinkRemoteDevice> device_ = nullptr;
};

void RemoteDeviceImpl::StartPairing()
{
    HILOGI("enter");
    NlErrCode err = device_->StartPair();
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

::ohos::nearlink::constant::PairingState RemoteDeviceImpl::GetPairingState()
{
    HILOGI("enter");
    int pairState = 0;
    NlErrCode err = device_->GetPairState(pairState);
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err, ::ohos::nearlink::constant::PairingState::key_t::PAIRING_STATE_NONE);
    return ConvertToAniState(pairState);
}

::taihe::string RemoteDeviceImpl::GetDeviceName()
{
    HILOGI("enter");
    std::string deviceName;
    NlErrCode err = device_->GetDeviceName(deviceName);
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err, "");
    return static_cast<::taihe::string>(deviceName);
}

::ohos::nearlink::constant::DeviceClass RemoteDeviceImpl::GetDeviceClass()
{
    HILOGI("enter");
    int deviceAppearance = 0;
    NlErrCode err = device_->GetDeviceAppearance(deviceAppearance);
    HILOGI("deviceAppearance: %{public}d", deviceAppearance);
    ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err, ::ohos::nearlink::constant::DeviceClass::key_t::DEVICE_INVALID_CLASS);
    int outAppearance = ConvertDeviceClass(deviceAppearance);
    return ohos::nearlink::constant::DeviceClass::from_value(outAppearance);
}

::taihe::string RemoteDeviceImpl::GetDeviceAlias()
{
    HILOGI("enter");
    std::string alias;
    NlErrCode err = device_->GetDeviceAlias(alias);
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err, "");
    return static_cast<::taihe::string>(alias);
}

::ohos::nearlink::constant::ConnectionState RemoteDeviceImpl::GetConnectionState()
{
    HILOGI("enter");
    int connState = 0;
    NlErrCode err = NearlinkHost::GetInstance().GetProfileConnState(device_->GetDeviceAddr(), connState);
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err,
        ::ohos::nearlink::constant::ConnectionState::key_t::STATE_DISCONNECTED);
    return ConvertToAniConnectionState(connState);
}

::ohos::nearlink::constant::AcbState RemoteDeviceImpl::GetAcbState()
{
    HILOGI("enter");
    int acbState = 0;
    NlErrCode err = device_->GetAcbState(acbState);
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err,
        ::ohos::nearlink::constant::AcbState::key_t::DISCONNECTED);
    return ConvertToAniAcbState(acbState);
}

void RemoteDeviceImpl::StartCrediblePairing()
{
    HILOGI("enter");
    NlErrCode err = device_->StartCrediblePair();
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void RemoteDeviceImpl::RemovePairedDevice()
{
    HILOGI("enter");
    int32_t err = NearlinkHost::GetInstance().RemovePair(*device_);
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void RemoteDeviceImpl::CancelDevicePairing()
{
    HILOGI("enter");
    NlErrCode err = device_->CancelDevicePairing();
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void RemoteDeviceImpl::SetPairingPasscode(::taihe::string_view passcode)
{
    HILOGI("enter");
    std::string code = std::string(passcode);
    ANI_NL_ASSERT_RETURN_VOID(IsValidPassCode(code), NL_ERR_INVALID_PASSCODE);
    NlErrCode err = device_->SetPairingPassCode(code);
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void RemoteDeviceImpl::SetPairingConfirmation(bool accept)
{
    HILOGI("enter");
    NlErrCode err = device_->SetPairingConfirmation(accept);
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void RemoteDeviceImpl::Connect()
{
    HILOGI("enter");
    NlErrCode err = NearlinkHost::GetInstance().ConnectAllowedProfiles(device_->GetDeviceAddr());
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void RemoteDeviceImpl::Disconnect()
{
    HILOGI("enter");
    NlErrCode err = NearlinkHost::GetInstance().DisconnectAllowedProfiles(device_->GetDeviceAddr());
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void RemoteDeviceImpl::SetDeviceAlias(::taihe::string_view alias)
{
    HILOGI("enter");
    std::string deviceAlias = std::string(alias);
    ANI_NL_ASSERT_RETURN_VOID(!deviceAlias.empty(), NL_ERR_INVALID_PARAM);
    ANI_NL_ASSERT_RETURN_VOID(deviceAlias.length() <= DEVICE_NAME_MAX_LENGTH, NL_ERR_STRING_LENGTH_LIMITED);
    NlErrCode err = device_->SetDeviceAlias(deviceAlias);
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

::ohos::nearlink::remoteDevice::DeviceModel RemoteDeviceImpl::GetDeviceModel()
{
    HILOGI("enter");
    DeviceModel model;
    NlErrCode err = device_->GetDeviceModel(model);
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err, ::ohos::nearlink::remoteDevice::DeviceModel{});
    return {
        .modelId = static_cast<::taihe::string>(model.GetModelId()),
        .subModelId = static_cast<::taihe::string>(model.GetSubModelId()),
        .iconId = static_cast<::taihe::string>(model.GetIconId())
    };
}

uintptr_t RemoteDeviceImpl::GetRssiValue()
{
    HILOGI("enter");
    NearlinkHost::GetInstance().RegisterRssiObserver(AniRemoteDeviceRssiObserver::GetInstance());

    auto func = [device = device_]() {
        if (device == nullptr) {
            HILOGE("nearlinkRemoteDevice is nullptr");
            return TaiheAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }
        NlErrCode err = device->ReadRemoteRssiValue();
        return TaiheAsyncWorkRet(err);
    };
    ani_env *env = taihe::get_env();
    ANI_NL_ASSERT_RETURN(env, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    auto asyncWork = TaiheAsyncWorkFactory::CreateAsyncWork(env, nullptr, func);
    ANI_NL_ASSERT_RETURN(asyncWork, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    bool success = AniRemoteDeviceRssiObserver::GetInstance()
        ->asyncPromiseMap_.TryPush(TaiheAsyncType::GET_REMOTE_DEVICE_RSSI, asyncWork);
    ANI_NL_ASSERT_RETURN(success, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    asyncWork->Run();
    return reinterpret_cast<uintptr_t>(asyncWork->GetRet());
}

void RemoteDeviceImpl::SetConnectionInterval(::ohos::nearlink::constant::ConnectionInterval interval)
{
    HILOGI("enter");
    int32_t intervalValue = static_cast<int32_t>(interval);
    NlErrCode err = device_->UpdateConnectInterval(static_cast<ConnectionInterval>(intervalValue));
    HILOGI("ret: %{public}d", err);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

::ohos::nearlink::remoteDevice::RemoteDevice CreateRemoteDevice(taihe::string_view address)
{
    HILOGI("enter");
    std::string invalidAddr = "";
    ANI_NL_ASSERT_RETURN(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        (taihe::make_holder<RemoteDeviceImpl, ::ohos::nearlink::remoteDevice::RemoteDevice>(invalidAddr)));
    std::string remoteAddr = std::string(address);
    bool checkRet = IsValidAddress(remoteAddr);
    ANI_NL_ASSERT_RETURN(checkRet, NL_ERR_INVALID_ADDRESS,
        (taihe::make_holder<RemoteDeviceImpl, ::ohos::nearlink::remoteDevice::RemoteDevice>(invalidAddr)));
    return taihe::make_holder<RemoteDeviceImpl, ::ohos::nearlink::remoteDevice::RemoteDevice>(remoteAddr);
}

void OnPairingRequest(
    taihe::callback_view<void(::ohos::nearlink::remoteDevice::PairingRequestParam const& data)> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    std::unique_lock<std::shared_mutex> guard(g_pairingRequestMutex);
    ANI_NL_ASSERT_RETURN_VOID(g_pairingRequestObserverVec.size() <= MAX_CB_NUM, NL_ERR_INVALID_PARAM);

    ::taihe::optional<::taihe::callback<void(::ohos::nearlink::remoteDevice::PairingRequestParam const&)>>
        pairingRequestCb =
        ::taihe::optional<::taihe::callback<void(::ohos::nearlink::remoteDevice::PairingRequestParam const&)>>{
            std::in_place_t{}, callback};

    if (std::find(g_pairingRequestObserverVec.begin(), g_pairingRequestObserverVec.end(), pairingRequestCb) !=
        g_pairingRequestObserverVec.end()) {
        return;
    }
    g_pairingRequestObserverVec.emplace_back(pairingRequestCb);
}

void OffPairingRequest(
    taihe::optional_view<::taihe::callback<void(::ohos::nearlink::remoteDevice::PairingRequestParam const& data)>>
    callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    std::unique_lock<std::shared_mutex> guard(g_pairingRequestMutex);
    ANI_NL_ASSERT_RETURN_VOID(!g_pairingRequestObserverVec.empty(), NL_ERR_INVALID_PARAM);
    if (callback.has_value()) {
        for (size_t i = 0; i < g_pairingRequestObserverVec.size(); ++i) {
            if (g_pairingRequestObserverVec[i] == callback) {
                g_pairingRequestObserverVec.erase(g_pairingRequestObserverVec.begin() + i);
                return;
            }
        }
    } else {
        g_pairingRequestObserverVec.clear();
    }
}

void OnPairingStateChange(
    ::taihe::callback_view<void(::ohos::nearlink::remoteDevice::PairingStateParam const& data)> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    std::unique_lock<std::shared_mutex> guard(g_pairStatusChangedMutex);
    ANI_NL_ASSERT_RETURN_VOID(g_pairStatusChangedObserverVec.size() <= MAX_CB_NUM, NL_ERR_INVALID_PARAM);

    ::taihe::optional<::taihe::callback<void(::ohos::nearlink::remoteDevice::PairingStateParam const&)>>
        pairingStateChangeCb =
            ::taihe::optional<::taihe::callback<void(::ohos::nearlink::remoteDevice::PairingStateParam const&)>>{
                std::in_place_t{}, callback};

    if (std::find(g_pairStatusChangedObserverVec.begin(), g_pairStatusChangedObserverVec.end(), pairingStateChangeCb) !=
        g_pairStatusChangedObserverVec.end()) {
        return;
    }
    g_pairStatusChangedObserverVec.emplace_back(pairingStateChangeCb);
}

void OffPairingStateChange(
    taihe::optional_view<::taihe::callback<void(::ohos::nearlink::remoteDevice::PairingStateParam const& data)>>
    callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    std::unique_lock<std::shared_mutex> guard(g_pairStatusChangedMutex);
    ANI_NL_ASSERT_RETURN_VOID(!g_pairStatusChangedObserverVec.empty(), NL_ERR_INVALID_PARAM);
    if (callback.has_value()) {
        for (size_t i = 0; i < g_pairStatusChangedObserverVec.size(); ++i) {
            if (g_pairStatusChangedObserverVec[i] == callback) {
                g_pairStatusChangedObserverVec.erase(g_pairStatusChangedObserverVec.begin() + i);
                return;
            }
        }
    } else {
        g_pairStatusChangedObserverVec.clear();
    }
}

void OnConnectionStateChange(
    taihe::callback_view<void(::ohos::nearlink::remoteDevice::ConnectionStateParam const& data)> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    std::unique_lock<std::shared_mutex> guard(g_connectionStateChangedMutex);
    ANI_NL_ASSERT_RETURN_VOID(g_connectionStateChangedObserverVec.size() <= MAX_CB_NUM, NL_ERR_INVALID_PARAM);

    ::taihe::optional<::taihe::callback<void(::ohos::nearlink::remoteDevice::ConnectionStateParam const&)>>
        connectionStateChangeCb =
            ::taihe::optional<::taihe::callback<void(::ohos::nearlink::remoteDevice::ConnectionStateParam const&)>>{
                std::in_place_t{}, callback};

    if (std::find(g_connectionStateChangedObserverVec.begin(), g_connectionStateChangedObserverVec.end(),
        connectionStateChangeCb) != g_connectionStateChangedObserverVec.end()) {
        return;
    }
    g_connectionStateChangedObserverVec.emplace_back(connectionStateChangeCb);
}

void OffConnectionStateChange(
    taihe::optional_view<::taihe::callback<void(::ohos::nearlink::remoteDevice::ConnectionStateParam const& data)>>
    callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    std::unique_lock<std::shared_mutex> guard(g_connectionStateChangedMutex);
    ANI_NL_ASSERT_RETURN_VOID(!g_connectionStateChangedObserverVec.empty(), NL_ERR_INVALID_PARAM);
    if (callback.has_value()) {
        for (size_t i = 0; i < g_connectionStateChangedObserverVec.size(); ++i) {
            if (g_connectionStateChangedObserverVec[i] == callback) {
                g_connectionStateChangedObserverVec.erase(g_connectionStateChangedObserverVec.begin() + i);
                return;
            }
        }
    } else {
        g_connectionStateChangedObserverVec.clear();
    }
}

void OnAcbStateChange(::taihe::callback_view<void(::ohos::nearlink::remoteDevice::AcbStateParam const& data)> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    std::unique_lock<std::shared_mutex> guard(g_acbStateChangedMutex);
    ANI_NL_ASSERT_RETURN_VOID(g_acbStateChangedObserverVec.size() <= MAX_CB_NUM, NL_ERR_INVALID_PARAM);

    ::taihe::optional<::taihe::callback<void(::ohos::nearlink::remoteDevice::AcbStateParam const&)>> acbStateChangeCb =
        ::taihe::optional<::taihe::callback<void(::ohos::nearlink::remoteDevice::AcbStateParam const&)>>{
            std::in_place_t{}, callback};

    if (std::find(g_acbStateChangedObserverVec.begin(), g_acbStateChangedObserverVec.end(), acbStateChangeCb) !=
        g_acbStateChangedObserverVec.end()) {
        return;
    }
    g_acbStateChangedObserverVec.emplace_back(acbStateChangeCb);
}

void OffAcbStateChange(
    taihe::optional_view<::taihe::callback<void(::ohos::nearlink::remoteDevice::AcbStateParam const& data)>> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    std::unique_lock<std::shared_mutex> guard(g_acbStateChangedMutex);
    ANI_NL_ASSERT_RETURN_VOID(!g_acbStateChangedObserverVec.empty(), NL_ERR_INVALID_PARAM);
    if (callback.has_value()) {
        for (size_t i = 0; i < g_acbStateChangedObserverVec.size(); ++i) {
            if (g_acbStateChangedObserverVec[i] == callback) {
                g_acbStateChangedObserverVec.erase(g_acbStateChangedObserverVec.begin() + i);
                return;
            }
        }
    } else {
        g_acbStateChangedObserverVec.clear();
    }
}
}  // namespace Nearlink
}  // namespace OHOS

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_CreateRemoteDevice(OHOS::Nearlink::CreateRemoteDevice);
TH_EXPORT_CPP_API_OnPairingRequest(OHOS::Nearlink::OnPairingRequest);
TH_EXPORT_CPP_API_OffPairingRequest(OHOS::Nearlink::OffPairingRequest);
TH_EXPORT_CPP_API_OnPairingStateChange(OHOS::Nearlink::OnPairingStateChange);
TH_EXPORT_CPP_API_OffPairingStateChange(OHOS::Nearlink::OffPairingStateChange);
TH_EXPORT_CPP_API_OnConnectionStateChange(OHOS::Nearlink::OnConnectionStateChange);
TH_EXPORT_CPP_API_OffConnectionStateChange(OHOS::Nearlink::OffConnectionStateChange);
TH_EXPORT_CPP_API_OnAcbStateChange(OHOS::Nearlink::OnAcbStateChange);
TH_EXPORT_CPP_API_OffAcbStateChange(OHOS::Nearlink::OffAcbStateChange);
// NOLINTEND