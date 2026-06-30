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
#include "VcpService.h"

#include "audio_volume_client_manager.h"
#include "ClassCreator.h"
#include "SleInterfaceProfileManager.h"
#include "log_util.h"
#include "ThreadUtil.h"
#include "SleConfig.h"
#include "SleInterfaceProfileASC.h"
#include "CdsmService.h"
#include "SleServiceFfrtLog.h"
#include "nearlink_dft_ue.h"
#include "audio_errors.h"

namespace OHOS {
namespace Nearlink {
struct VcpService::impl {
    class AdapterCallback : public VcpStackCallback {
    public:
        AdapterCallback(impl &pimpl) : pimpl_(pimpl) {}
        void OnVolumeChangeEvent(const RawAddress &device, uint8_t volume) override;
        void OnMuteStatusChangeEvent(const RawAddress &device, uint8_t muteStatus) override;
        void OnConnectionStateChanged(const RawAddress &device, uint8_t state, uint8_t preState) override;
        void OnNotifyVolumeChange(const RawAddress &device, const std::list<StreamVolume> &streamVolumes) override;

    private:
        impl &pimpl_;
    };

    impl(VcpService &self) : service_(self), adapterCallback_(*this),
        stackAdapter_(adapterCallback_) {}

    VcpService &service_;
    AdapterCallback adapterCallback_;
    VcpStackAdapter stackAdapter_;
};

void VcpService::impl::AdapterCallback::OnVolumeChangeEvent(const RawAddress &device, uint8_t volume)
{
    pimpl_.service_.ProcessVolumeChangeEvent(device, volume);
}

void VcpService::impl::AdapterCallback::OnMuteStatusChangeEvent(const RawAddress &device, uint8_t muteStatus)
{
    pimpl_.service_.ProcessMuteStatusChangeEvent(device, muteStatus);
}

void VcpService::impl::AdapterCallback::OnConnectionStateChanged(const RawAddress &device, uint8_t state,
    uint8_t preState)
{
    pimpl_.service_.NotifyStateChanged(device, state, preState);
}

void VcpService::impl::AdapterCallback::OnNotifyVolumeChange(const RawAddress &device,
    const std::list<StreamVolume> &streamVolumes)
{
    pimpl_.service_.NotifyVolumeChanged(device, streamVolumes);
}

VcpService::VcpService()
    : utility::Context(PROFILE_NAME_VCP, "1.0.0"), pimpl(std::make_unique<VcpService::impl>(*this))
{
    HILOGD("[VCP]%{public}s:%{public}s Create", PROFILE_NAME_VCP.c_str(), Name().c_str());
    // 注册回调
    pimpl->stackAdapter_.RegisterStackCbk();
}

VcpService::~VcpService()
{
    HILOGD("[VCP]%{public}s:%{public}s Destroy", PROFILE_NAME_VCP.c_str(), Name().c_str());
    // 去注册回调
    pimpl->stackAdapter_.DeregisterStackCbk();
}

utility::Context *VcpService::GetContext()
{
    return this;
}

VcpService *VcpService::GetService()
{
    return static_cast<VcpService *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_VCP));
}

void VcpService::RegisterObserver(VcpClientObserver &serviceObserver)
{
    HILOGD("[VcpService]Enter");
    vcpClientObservers_.Register(serviceObserver);
}

void VcpService::DeregisterObserver(VcpClientObserver &serviceObserver)
{
    HILOGD("[VcpService]Enter");
    vcpClientObservers_.Deregister(serviceObserver);
}

int VcpService::GetConnectState()
{
    return 0;
}

std::list<RawAddress> VcpService::GetConnectDevices()
{
    return std::list<RawAddress>();
}

void VcpService::EnableTask()
{
    if (isStarted_.load()) {
        GetContext()->OnEnable(PROFILE_NAME_VCP, true);
        HILOGW("[VcpService]:VcpService has already been started before.");
        return;
    }

    //  获取AudioSystemManager的音量最大值
    constexpr AudioStandard::AudioVolumeType volumeTypeMusic = AudioStandard::AudioVolumeType::STREAM_MUSIC;
    const AudioStandard::DeviceType deviceType = AudioStandard::DeviceType::DEVICE_TYPE_NEARLINK;
    musicMaxVolumeLevel_ =
        AudioStandard::AudioVolumeClientManager::GetInstance().GetDeviceMaxVolume(volumeTypeMusic, deviceType);
    HILOGI("[VcpService]get audio system max volume is %{public}d", musicMaxVolumeLevel_);

    GetContext()->OnEnable(PROFILE_NAME_VCP, true);
    isStarted_.store(true);
}

void VcpService::Enable()
{
    HILOGD("[VcpService]Enter");
    DoInVcpThread([this]() -> void {
        EnableTask();
    });
}

void VcpService::DisableTask()
{
    HILOGD("[VcpService]Enter");
    if (!isStarted_.load()) {
        GetContext()->OnDisable(PROFILE_NAME_VCP, true);
        HILOGW("[VcpService]:VcpService has already been shutdown before.");
        return;
    }

    isShuttingDown_.store(true);
    musicMaxVolumeLevel_ = 0;

    GetContext()->OnDisable(PROFILE_NAME_VCP, true);
    isStarted_.store(false);
    isShuttingDown_.store(false);
}

void VcpService::Disable()
{
    HILOGD("[VcpService]Enter");
    DoInVcpThread([this]() -> void {
        DisableTask();
    });
}

void VcpService::ConnectTask(const RawAddress &device)
{
    HILOGD("[VcpService]Enter");
    NL_CHECK_RETURN(!isShuttingDown_.load(), "[VcpService]:VcpService is shutting down.");
    pimpl->stackAdapter_.Connect(device);
}

int VcpService::Connect(const RawAddress &device)
{
    HILOGD("[VcpService]Enter");
    DoInVcpThread([this, addr = device]() -> void {
        ConnectTask(addr);
    });
    return VCP_SUCCESS;
}

void VcpService::DisconnectTask(const RawAddress &device)
{
    pimpl->stackAdapter_.Disconnect(device);
}

int VcpService::Disconnect(const RawAddress &device)
{
    HILOGI("[VcpService]Enter");
    DoInVcpThread([this, device]() -> void {
        DisconnectTask(device);
    });
    return VCP_SUCCESS;
}

void VcpService::SetAllStreamVolume(const RawAddress &device)
{
    std::list<StreamVolume> streamVolumes;
    StreamVolume callStreamVolume(SystemToVcpVolume(GetDeviceCallVolume(device)),
        static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_CALL));
    StreamVolume mediaStreamVolume(SystemToVcpVolume(GetDeviceMediaVolume(device)),
        static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_MEDIA));
    streamVolumes.emplace_back(callStreamVolume);
    streamVolumes.emplace_back(mediaStreamVolume);
    pimpl->stackAdapter_.SetStreamVolume(device, streamVolumes);
}

void VcpService::SetVolumeAdapter(const RawAddress &device, int32_t vcpVolume, uint8_t streamType, bool isNeedReport)
{
    bool streamVolumeAbility = false;
    if (!streamVolumeAbility_.GetValue(device.GetAddress(), streamVolumeAbility)) {
        HILOGW("[VcpService] not get stream volume ability");
    }

    if (streamVolumeAbility) {
        std::list<StreamVolume> streamVolumes;
        StreamVolume streamVolume(vcpVolume, streamType);
        streamVolumes.emplace_back(streamVolume);
        pimpl->stackAdapter_.SetStreamVolume(device, streamVolumes);
    } else if (isNeedReport) {
        pimpl->stackAdapter_.SetVolume(device, vcpVolume);
    }
}

bool VcpService::SetCdsmAbsoluteVolume(const RawAddress &device, int32_t vcpVolume, uint8_t streamType,
    bool isNeedReport)
{
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN_RET(cdsmService, false, "Cdsm service is null.");
    bool isCdsDevice = cdsmService->CdsmCheckIsCooperationDevice(device);
    if (isCdsDevice) {
        std::vector<NearlinkCdsmInfo> cdsmList;
        NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
        NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, false, "CdsmGetAllMemberInfo error.");
        for (const auto& info : cdsmList) {
            // 对于合作集设备，给组内所有成员（包括当前设备）发一下音量修改事件
            SetVolumeAdapter(info.addr_, vcpVolume, streamType, isNeedReport);
        }
    } else {
        SetVolumeAdapter(device, vcpVolume, streamType, isNeedReport);
    }
    return true;
}

void VcpService::DftVolumeControlInSource(const RawAddress &device, int32_t volume, int32_t storeVolume)
{
    NL_CHECK_RETURN(musicMaxVolumeLevel_ != 0, "musicMaxVolumeLevel_ == 0.");
    if (volume == 0) {
        NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(device, device, VOLUME_CONTROL_IN_SOURCE, VOLUME_IS_0);
    } else if (volume * MAX_VOLUME_VALUE / musicMaxVolumeLevel_ < VOLUME_VALUE_10) {
        NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(device, device, VOLUME_CONTROL_IN_SOURCE,
            VOLUME_BELOW_10);
    } else if (volume * MAX_VOLUME_VALUE / musicMaxVolumeLevel_ > VOLUME_VALUE_80) {
        NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(device, device, VOLUME_CONTROL_IN_SOURCE,
            VOLUME_ABOVE_80);
    }
}

void VcpService::SetDeviceAbsoluteVolumeTask(const RawAddress &device, int32_t volume, uint8_t streamType)
{
    NL_CHECK_RETURN(volume >= 0 && volume <= musicMaxVolumeLevel_, "[VcpService]Invalid volume %{public}d.", volume);
    NL_CHECK_RETURN(streamType < VolumeStreamType::SLE_STREAM_MAX, "[VcpService]Invalid streamType %{public}d.",
        streamType);
    HILOGI("[VcpService]streamType: %{public}u, volume: %{public}d", streamType, volume);
    bool isInCall = IsCalling();
    bool isCallParam = isInCall;
    int32_t storeVolume = VCP_DEFAULT_VOLUME_LEVEL;
    if (streamType == VolumeStreamType::SLE_STREAM_CALL) {
        isCallParam = true;
        storeVolume = GetDeviceCallVolume(device);
    } else if (streamType == VolumeStreamType::SLE_STREAM_MEDIA) {
        isCallParam = false;
        storeVolume = GetDeviceMediaVolume(device);
    }
    if (volume == storeVolume) {
        HILOGW("[VcpService]%{public}s Skipping update volume to same as current %{public}d.",
            GET_ENCRYPT_ADDR(device), volume);
        return;
    }
    SaveDeviceVolume(device, volume, isCallParam);

    bool isNeedReport = true;
    if (isInCall != isCallParam) {
        HILOGI("[VcpService]streamType is different from serviceType");
        // 下发的流类型与业务类型不一致, V1音频协议不通知耳机音量
        isNeedReport = false;
    }
    // 音量值转换
    int32_t vcpVolume = SystemToVcpVolume(volume);

    if (!SetCdsmAbsoluteVolume(device, vcpVolume, streamType, isNeedReport)) {
        SetVolumeAdapter(device, vcpVolume, streamType, isNeedReport);
    }

    DftVolumeControlInSource(device, volume, storeVolume);
}

/* 音频框架触发 */
/* 1. 调用stack接口发送音量修改值到对端耳机 */
/* 2. 保存当前设备音量 */
void VcpService::SetDeviceAbsoluteVolume(const RawAddress &device, int32_t volume, uint8_t streamType)
{
    HILOGI("[VcpService]SetDeviceAbsVolume device:%{public}s, volume:%{public}d", GET_ENCRYPT_ADDR(device), volume);
    DoInVcpThread([this, device, volume, streamType]() -> void {
        SetDeviceAbsoluteVolumeTask(device, volume, streamType);
    });
}

void VcpService::SwitchAbsVolumeDeviceTask(const RawAddress &device, bool isNeedSetVolume)
{
    // get store volume and put into effect
    int32_t storedVolume = GetDeviceVolume(device);
    HILOGI("[VcpService]device store volume = %{public}d.", storedVolume);
    int32_t vcpVolume = SystemToVcpVolume(storedVolume);

    bool streamVolumeAbility = false;
    if (!streamVolumeAbility_.GetValue(device.GetAddress(), streamVolumeAbility)) {
        HILOGW("[VcpService] not get stream volume ability");
    }

    // 不支持音频流音量的音频设备，只能设置当前通话或则媒体的音量给耳机
    if (!streamVolumeAbility) {
        CdsmService *cdsmService = CdsmService::GetService();
        if (cdsmService == nullptr) {
            HILOGI("Cdsm service is null.");
            pimpl->stackAdapter_.SetVolume(device, vcpVolume);
            return;
        }
        if (cdsmService->CdsmCheckIsCooperationDevice(device)) {
            std::vector<NearlinkCdsmInfo> cdsmList;
            NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
            if (ret != NL_NO_ERROR) {
                HILOGI("CdsmGetAllMemberInfo error.");
                pimpl->stackAdapter_.SetVolume(device, vcpVolume);
                return;
            }
            for (const auto& info : cdsmList) {
                // 对于合作集设备，给组内所有成员（包括当前设备）发一下音量修改事件
                pimpl->stackAdapter_.SetVolume(info.addr_, vcpVolume);
            }
        } else {
            pimpl->stackAdapter_.SetVolume(device, vcpVolume);
        }
    } else if (isNeedSetVolume) {
        SetAllStreamVolume(device);
    }
}

/* active device changed, restore the volume saved last time. */
/* 1. 提供给音频流调用，在 AscService::SetActiveSinkDevice 时触发  */
/* 2. 同时在类内部当设备连接时，也需要调用，内部调用当设备连接时， isNeedSetVolume 设置为false */
void VcpService::SwitchAbsVolumeDevice(const RawAddress &device, bool isNeedSetVolume)
{
    HILOGI("[VcpService]SwitchAbsVolumeDevice device:%{public}s, isNeedSetVolume:%{public}d",
        GET_ENCRYPT_ADDR(device), isNeedSetVolume);
    DoInVcpThread([this, device, isNeedSetVolume]() -> void {
        SwitchAbsVolumeDeviceTask(device, isNeedSetVolume);
    });
}

bool VcpService::IsActiveDevice(const RawAddress &device, RawAddress &reportAddr){
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN_RET(cdsmService, false, "[VcpService] ProfileCdsm is null.");
    // 取了一下音频流当前的激活设备，这里需要和回调传回来的设备信息进行比对，如果不相同，直接返回
    RawAddress activeDevice = GetAscActiveDevice();
    if (cdsmService->CdsmCheckIsCooperationDevice(device)) {
        std::vector<RawAddress> devAddrList = {device, activeDevice};
        if (!(device == activeDevice) && !(cdsmService->CdsmCheckIsSameCooperation(devAddrList))) {
            HILOGD("[VcpService] device %{public}s is not the active device %{public}s",
                GetEncryptAddr(device.GetAddress()).c_str(), GetEncryptAddr(activeDevice.GetAddress()).c_str());
            return false;
        }
    } else {
        NL_CHECK_RETURN_RET(device == activeDevice, false, "device is not the current asc active device.");
    }

    if (!cdsmService->CdsmGetReportAddr(device, reportAddr)) {
        HILOGW("[VcpService] device %{public}s get report addr fail.", GetEncryptAddr(device.GetAddress()).c_str());
    }
    return true;
}

/* 处理协议栈回调 */
void VcpService::ProcessVolumeChangeEvent(const RawAddress &device, uint8_t volume)
{
    HILOGI("[VcpService][ProcessVolumeChangeEvent]");
    RawAddress reportAddr(device);
    bool isInCall = IsCalling();
    uint8_t streamType = isInCall ? static_cast<uint8_t>(SLE_STREAM_CALL) : static_cast<uint8_t>(SLE_STREAM_MEDIA);
    if (!IsActiveDevice(device, reportAddr)){
        SetCdsmAbsoluteVolume(device, volume, streamType, true);
        return;
    }
    int32_t deviceVolume = VcpToSystemVolume(volume);
    HILOGI("[VcpService]SetVolume VCPVolume: %{public}d. musicMaxVolume: %{public}d. deviceVolume: %{public}d.",
           volume, musicMaxVolumeLevel_, deviceVolume);
    // 获取设备缓存的音量
    int32_t storeVolume = GetDeviceVolume(device);
    // 保存当前设备的音量变化值
    SaveDeviceVolume(device, deviceVolume, isInCall);
    if (deviceVolume == storeVolume) {
        HILOGI("[VcpService]device %{public}s Skipping SetVolume to same as current %{public}d.",
               GET_ENCRYPT_ADDR(device), deviceVolume);
        SetCdsmAbsoluteVolume(device, volume, streamType, true);
        return;
    }
    bool updateUi = true;
    // If in call, not update ui for music volume
    int32_t result;
    if (isInCall) {
        result = AudioStandard::AudioVolumeClientManager::GetInstance().SetNearlinkDeviceVolume(
            reportAddr.GetAddress(), AudioStandard::AudioVolumeType::STREAM_VOICE_CALL, deviceVolume, updateUi);
        NearlinkDftUe::GetInstance().WriteAudioControlUeAndExcep(reportAddr, VOLUME_CONTROL_IN_SINK_SCENE,
            SINK_VOLUME_CONTROL_CALL, UPDATE_VOICE_STACK_REASON_INVALID, static_cast<NlErrCode>(result));
    } else {
        result = AudioStandard::AudioVolumeClientManager::GetInstance().SetNearlinkDeviceVolume(
            reportAddr.GetAddress(), AudioStandard::AudioVolumeType::STREAM_MUSIC, deviceVolume, updateUi);
        NearlinkDftUe::GetInstance().WriteAudioControlUeAndExcep(reportAddr, VOLUME_CONTROL_IN_SINK_SCENE,
            SINK_VOLUME_CONTROL_MEDIA, UPDATE_VOICE_STACK_REASON_INVALID, static_cast<NlErrCode>(result));
    }
    HILOGI("[VcpService]device=%{public}s, SetVolume To AudioFWK result=%{public}d, volume=%{public}d.",
           GET_ENCRYPT_ADDR(device), result, volume);
    if (result != AudioStandard::SUCCESS) {
        HILOGE("[VcpService]excute SetVolume error, and restore device volume to last save=%{public}d.", storeVolume);
        SetCdsmAbsoluteVolume(device, SystemToVcpVolume(storeVolume), streamType, true);
        // 如果设备音量设置失败，则恢复设备音量到缓存的音量
        SaveDeviceVolume(device, storeVolume, isInCall);
    } else {
        SetCdsmAbsoluteVolume(device, volume, streamType, true);
    }
}

void VcpService::ProcessMuteStatusChangeEvent(const RawAddress &device, uint8_t muteStatus)
{
    HILOGI("[VcpService] enter");
}

bool VcpService::SaveDeviceVolumeTask(const RawAddress &device, int32_t volume, bool isInCall)
{
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN_RET(cdsmService, false, "Cdsm service is null.");
    bool isCdsDevice = cdsmService->CdsmCheckIsCooperationDevice(device);
    if (isCdsDevice) {
        std::vector<NearlinkCdsmInfo> cdsmList;
        NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
        NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, false, "CdsmGetAllMemberInfo error.");
        for (const auto& info : cdsmList) {
            // 对于合作集设备，给组内所有成员保存一下音量
            if (isInCall) {
                SleConfig::GetInstance().SetDeviceCallVolume(info.addr_.GetAddress(), volume);
            } else {
                SleConfig::GetInstance().SetDeviceMediaVolume(info.addr_.GetAddress(), volume);
            }
        }
        return true;
    }
    return false;
}

void VcpService::SaveDeviceVolume(const RawAddress &device, int32_t volume, bool isInCall)
{
    HILOGI("[VcpService][SaveDeviceVolume]device=%{public}s, volume=%{public}d, isInCall=%{public}d.",
           GET_ENCRYPT_ADDR(device), volume, isInCall);
    if (isInCall) {
        if (!(SaveDeviceVolumeTask(device, volume, isInCall))) {
            SleConfig::GetInstance().SetDeviceCallVolume(device.GetAddress(), volume);
        }
    } else {
        if (!(SaveDeviceVolumeTask(device, volume, isInCall))) {
            SleConfig::GetInstance().SetDeviceMediaVolume(device.GetAddress(), volume);
        }
    }
    SleConfig::GetInstance().Save();
}

int VcpService::GetDeviceVolume(const RawAddress &device)
{
    bool isInCall = IsCalling();
    if (isInCall) {
        return GetDeviceCallVolume(device);
    } else {
        return GetDeviceMediaVolume(device);
    }
}

int VcpService::GetDeviceMediaVolume(const RawAddress &device)
{
    return SleConfig::GetInstance().GetDeviceMediaVolume(device.GetAddress(), GetDefaultVolume());
}

int VcpService::GetDeviceCallVolume(const RawAddress &device)
{
    return SleConfig::GetInstance().GetDeviceCallVolume(device.GetAddress(), GetDefaultVolume());
}

bool VcpService::IsCalling() const
{
    ProfileASC *ascService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN_RET(ascService, false, "ProfileASC is null.");
    return ascService->IsCalling();
}

RawAddress VcpService::GetAscActiveDevice()
{
    RawAddress device;
    ProfileASC *ascService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN_RET(ascService, device, "ProfileASC is null.");
    device = ascService->GetActiveSinkDevice();
    return device;
}

/* 收到状态变化后，触发 */
void VcpService::NotifyStateChanged(const RawAddress &device, int state, int preState)
{
    HILOGD("[VcpService]vcp client service state notify, newState:%{public}d, oldState:%{public}d", state, preState);
    streamVolumeAbility_.EnsureInsert(device.GetAddress(), pimpl->stackAdapter_.GetStreamVolumeAbility(device));

    vcpClientObservers_.ForEach([device, state, preState](VcpClientObserver &observer) {
        observer.OnConnectionStateChanged(device, state, preState);
    });
}

void VcpService::NotifyVolumeChanged(const RawAddress &device, const std::list<StreamVolume> &streamVolumes)
{
    RawAddress reportAddr(device);
    NL_CHECK_RETURN_LOGD(IsActiveDevice(device, reportAddr), "skipping if not active device");

    // 获取设备缓存的音量
    int32_t storeMediaVolume = GetDeviceMediaVolume(device);
    int32_t storeCallVolume = GetDeviceCallVolume(device);

    int32_t result;
    uint8_t streamType = static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_MEDIA);
    bool isInCall = IsCalling();
    for (auto &streamVolume : streamVolumes) {
        HILOGI("[VcpService]streamVolume additionalInfo:%{public}d, volumeStreamType_:%{public}d, volume_:%{public}d",
            streamVolume.additionalInfo_, streamVolume.volumeStreamType_, streamVolume.volume_);
        if (streamVolume.additionalInfo_ == static_cast<uint8_t>(VolumeAdditionalInfo::SLE_SERVER_CHANGE_VOLUME) &&
            streamVolume.volumeStreamType_ == static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_MEDIA) && !isInCall) {
            int32_t deviceMediaVolume = VcpToSystemVolume(streamVolume.volume_);
            if (deviceMediaVolume == storeMediaVolume) {
                continue;
            }
            result = AudioStandard::AudioVolumeClientManager::GetInstance().SetNearlinkDeviceVolume(
                reportAddr.GetAddress(), AudioStandard::AudioVolumeType::STREAM_MUSIC, deviceMediaVolume, true);
            NearlinkDftUe::GetInstance().WriteAudioControlUeAndExcep(reportAddr, VOLUME_CONTROL_IN_SINK_SCENE,
                SINK_VOLUME_CONTROL_MEDIA, UPDATE_VOICE_STACK_REASON_INVALID, static_cast<NlErrCode>(result));
            if (result != AudioStandard::SUCCESS) {
                HILOGE("[VcpService]excute SetVolume error, restore call volume to =%{public}d.", storeMediaVolume);
                SetCdsmAbsoluteVolume(device, SystemToVcpVolume(storeMediaVolume), streamType, true);
                // 如果设备音量设置失败，则恢复设备音量到缓存的音量
                SaveDeviceVolume(device, storeMediaVolume, isInCall);
                continue;
            }
            // 保存当前设备的音量变化值
            SaveDeviceVolume(device, deviceMediaVolume, isInCall);
        }
        if (streamVolume.additionalInfo_ == static_cast<uint8_t>(VolumeAdditionalInfo::SLE_SERVER_CHANGE_VOLUME) &&
            streamVolume.volumeStreamType_ == static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_CALL) && isInCall) {
            int32_t deviceCallVolume = VcpToSystemVolume(streamVolume.volume_);
            if (deviceCallVolume == storeCallVolume) {
                continue;
            }
            streamType = static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_CALL);
            result = AudioStandard::AudioVolumeClientManager::GetInstance().SetNearlinkDeviceVolume(
                reportAddr.GetAddress(), AudioStandard::AudioVolumeType::STREAM_VOICE_CALL, deviceCallVolume, true);
            NearlinkDftUe::GetInstance().WriteAudioControlUeAndExcep(reportAddr, VOLUME_CONTROL_IN_SINK_SCENE,
                SINK_VOLUME_CONTROL_CALL, UPDATE_VOICE_STACK_REASON_INVALID, static_cast<NlErrCode>(result));
            if (result != AudioStandard::SUCCESS) {
                HILOGE("[VcpService]excute SetVolume error, restore call volume to =%{public}d.", storeCallVolume);
                SetCdsmAbsoluteVolume(device, SystemToVcpVolume(storeCallVolume), streamType, true);
                // 如果设备音量设置失败，则恢复设备音量到缓存的音量
                SaveDeviceVolume(device, storeCallVolume, isInCall);
                continue;
            }
            // 保存当前设备的音量变化值
            SaveDeviceVolume(device, deviceCallVolume, isInCall);
        }
    }
}

REGISTER_CLASS_CREATOR(VcpService);
} // namespace Nearlink
} // namespace OHOS