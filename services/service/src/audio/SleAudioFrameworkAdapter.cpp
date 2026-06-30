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
#include "SleAudioFrameworkAdapter.h"

#include "McpServerServiceManager.h"
#include "McpServerService.h"
#include "McpDefines.h"
#include "ThreadUtil.h"
#include "nearlink_dft_ue.h"
#include "nearlink_utils.h"
#include "parameter_manager.h"
#include "SleRemoteDeviceAdapter.h"
#include "SleInterfaceProfileManager.h"
#include "ASCDefines.h"
#include "ASCMessage.h"
#include "SleUtils.h"

namespace OHOS {
namespace Nearlink {

namespace {
    constexpr int32_t SUCCESS = 0;
    constexpr int32_t MAX_STRING_LEN = 255;
}

void SleAudioFrameworkAdapter::RegisterAudioFrameworkAdapterListener()
{
    uint8_t supportParam = ParameterManager::GetMuteControlEnable();
    if (supportParam != SLE_SUPPORT_AND_OPEN) {
        return;
    }
    DoInAudioFwAdapterThread([this]() {
        this->RegisterOutputPipeChangeListener();
        this->RegisterRendererDataTransferListener();
    });
}

void SleAudioFrameworkAdapter::UnregisterAudioFrameworkAdapterListener()
{
    uint8_t supportParam = ParameterManager::GetMuteControlEnable();
    if (supportParam != SLE_SUPPORT_AND_OPEN) {
        return;
    }
    DoInAudioFwAdapterThread([this]() {
        this->UnregisterOutputPipeChangeListener();
        this->UnregisterRendererDataTransferListener();
        this->StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM);
        this->StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_DATA_ZERO);
        this->StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_VOL_ZERO);
    });
}

const std::vector<RouteFlagInfo> &SleAudioFrameworkAdapter::GetRouteFlagList() const
{
    return routeFlagList_;
}

void SleAudioFrameworkAdapter::AddMemberRouteFlagList(const RouteFlagInfo &newInfo)
{
    routeFlagList_.push_back(newInfo);
}

void SleAudioFrameworkAdapter::RemoveMemberRouteFlagList(const uint32_t &flag)
{
    routeFlagList_.erase(std::remove_if(routeFlagList_.begin(), routeFlagList_.end(),
                                        [flag](const RouteFlagInfo &item) { return item.routeFlag == flag; }),
                         routeFlagList_.end());
}

std::string SleAudioFrameworkAdapter::GetBundleName(uint32_t streamId)
{
    for (const auto &it : GetRouteFlagList()) {
        if (it.streamId == streamId) {
            return it.bundleName;
        }
    }
    return "";
}

void SleAudioFrameworkAdapter::ClearRouteFlagList()
{
    HILOGD("[SleAudioFrameworkAdapter] enter");
    routeFlagList_.clear();
    routeFlagList_.shrink_to_fit();
}

/* 音频框架在应用release场景会回调通知,rendererStreams里面的数量会减一 */
void SleAudioFrameworkAdapter::UpdateRouteFlagList(
    uint32_t flag, const std::map<uint32_t, AudioStandard::RendererStreamInfo> &rendererStreams)
{
    HILOGD("[SleAudioFrameworkAdapter] enter");
    std::string bundleName = "";
    // 刷新匹配flag先删除清理
    RemoveMemberRouteFlagList(flag);
    // 遍历新增插入,所有pipe中的流加起来最多256条
    for (const auto &rendererStream : rendererStreams) {
        auto info = rendererStream.second;
        HILOGD("[SleAudioFrameworkAdapter] state=%{public}d, usage=%{public}d, streamId=%{public}d", info.state_,
               info.usage_, info.streamId_);
        if (GetRouteFlagList().size() > MAX_ROUTE_FLAG_LIST_SIZE) {
            HILOGE("[SleAudioFrameworkAdapter] err size up to limit");
            return;
        }
        bundleName = info.bundleName_;
        if (bundleName.length() > MAX_STRING_LEN) {
            bundleName = "";
        }
        RouteFlagInfo newInfo = {
            .routeFlag = flag,
            .streamId = info.streamId_,
            .streamUsage = info.usage_,
            .status = info.state_,
            .bundleName = bundleName,
        };
        AddMemberRouteFlagList(newInfo);
    }
}

bool SleAudioFrameworkAdapter::isCallStreamType(uint32_t streamUsage)
{
    switch (streamUsage) {
        case OHOS::AudioStandard::STREAM_USAGE_VOICE_RINGTONE:
        case OHOS::AudioStandard::STREAM_USAGE_VOICE_ASSISTANT:
        case OHOS::AudioStandard::STREAM_USAGE_RINGTONE:
        case OHOS::AudioStandard::STREAM_USAGE_VOICE_MODEM_COMMUNICATION:
        case OHOS::AudioStandard::STREAM_USAGE_VOICE_CALL_ASSISTANT:
        case OHOS::AudioStandard::STREAM_USAGE_VOICE_COMMUNICATION:
        case OHOS::AudioStandard::STREAM_USAGE_VIDEO_COMMUNICATION:
            return true;
        default:
            break;
    }
    return false;
}

bool SleAudioFrameworkAdapter::IsBtOut()
{
    AudioStandard::DeviceType deviceType =
        AudioStandard::AudioDevicesClientManager::GetInstance().GetActiveOutputDevice();
    switch (deviceType) {
        case OHOS::AudioStandard::DeviceType::DEVICE_TYPE_BLUETOOTH_SCO:
        case OHOS::AudioStandard::DeviceType::DEVICE_TYPE_BLUETOOTH_A2DP:
        case OHOS::AudioStandard::DeviceType::DEVICE_TYPE_BLUETOOTH_A2DP_IN:
            return true;
        default:
            break;
    }
    return false;
}

bool SleAudioFrameworkAdapter::IsMusicActive()
{
    return OHOS::AudioStandard::AudioVolumeClientManager::GetInstance().IsStreamActive(
        OHOS::AudioStandard::AudioStreamType::STREAM_MUSIC);
}

bool SleAudioFrameworkAdapter::IsVoiceCallActive()
{
    return OHOS::AudioStandard::AudioVolumeClientManager::GetInstance().IsStreamActive(
        OHOS::AudioStandard::AudioStreamType::STREAM_VOICE_CALL) ||
        OHOS::AudioStandard::AudioVolumeClientManager::GetInstance().IsStreamActive(
            OHOS::AudioStandard::AudioStreamType::STREAM_VOICE_ASSISTANT);
}

bool SleAudioFrameworkAdapter::IsNearlinkOut()
{
    OHOS::AudioStandard::DeviceType deviceType =
        OHOS::AudioStandard::AudioDevicesClientManager::GetInstance().GetActiveOutputDevice();
    return deviceType == OHOS::AudioStandard::DeviceType::DEVICE_TYPE_NEARLINK;
}

void SleAudioFrameworkAdapter::JudgeEmptyStreamByRendererState(const RawAddress &device)
{
    HILOGD("[SleAudioFrameworkAdapter] enter");
    // 启动定时器要求 1.没有通话流, 2.没有流是running, 3.存在流stop或者pause的状态
    bool isStart = false;
    uint32_t streamId = 0;
    for (const auto &it : GetRouteFlagList()) {
        HILOGD("[SleAudioFrameworkAdapter] rendererState=%{public}d, streamUsage=%{public}d, streamId=%{public}d",
               it.status, it.streamUsage, it.streamId);
        if (isCallStreamType(it.streamUsage)) {
            StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM);
            return;
        }
        if (it.status == AudioStandard::RendererState::RENDERER_RUNNING) {
            StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM);
            return;
        }
        if (it.status == AudioStandard::RendererState::RENDERER_STOPPED ||
            it.status == AudioStandard::RendererState::RENDERER_PAUSED) {
            isStart = true;
            streamId = it.streamId;
        }
    }
    StartEmptyMuteScMcpPauseTimer(device, isStart, streamId, MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM);
}

bool SleAudioFrameworkAdapter::IsSlePipe(AudioStandard::HdiAdapterType adapterType)
{
    if (adapterType == AudioStandard::HDI_ADAPTER_TYPE_SLE || adapterType == AudioStandard::HDI_ADAPTER_TYPE_PRIMARY) {
        return true;
    }
    return false;
}

/* changeType注释: stream-pipe里流状态变化, device-pipe的设备变化，status-pipe自己的状态变化 */
void SleAudioFrameworkAdapter::SleAudioOutputPipeListener::OnOutputPipeChange(
    const AudioStandard::AudioPipeChangeType changeType,
    const std::shared_ptr<AudioStandard::AudioOutputPipeInfo> &changedPipeInfo)
{
    HILOGD("[SleAudioFrameworkAdapter] enter");
    DoInAudioFwAdapterThread([changeType, changedPipeInfo]() {
        SleAudioFrameworkAdapter::GetInstance().HandleAudioPipeStatusChangeInner(changeType, changedPipeInfo);
    });
}

void SleAudioFrameworkAdapter::HandleAudioPipeStatusChangeInner(const AudioStandard::AudioPipeChangeType changeType,
    const std::shared_ptr<AudioStandard::AudioOutputPipeInfo> &changedPipeInfo)
{
    NL_CHECK_RETURN(changedPipeInfo != nullptr, "[SleAudioFrameworkAdapter] changedPipeInfo is null!");
    ASCService *ascService = ASCService::GetService();
    NL_CHECK_RETURN(ascService != nullptr, "[SleAudioFrameworkAdapter] Get ascService error!");
    RawAddress device = ascService->GetActiveSinkDevice();
    NL_CHECK_RETURN(IsValidAddress(device.GetAddress()), "[SleAudioFrameworkAdapter] Invalid Address!");
    if (!SleRemoteDeviceAdapter::GetInstance()->IsVendorDevice(device)) {
        HILOGD("[SleAudioFrameworkAdapter] not VendorDevice device");
        return;
    }

    AudioStandard::HdiAdapterType adapterType = changedPipeInfo->GetAdapter();
    AudioStandard::AudioPipeStatus pipeStatus = changedPipeInfo->GetStatus();
    uint32_t routeFlag = changedPipeInfo->GetRouteFlag();
    auto rendererStreams = changedPipeInfo->GetStreams();

    if (!IsSlePipe(adapterType)) {
        HILOGD("[SleAudioFrameworkAdapter] not sle device");
        return;
    }

    if (changeType == AudioStandard::PIPE_CHANGE_TYPE_PIPE_STATUS &&
        pipeStatus == AudioStandard::PIPE_STATUS_CLOSE) {
        ClearRouteFlagList();
        return;
    }
    UpdateRouteFlagList(routeFlag, rendererStreams);
    JudgeEmptyStreamByRendererState(device);
}

SleAudioFrameworkAdapter &SleAudioFrameworkAdapter::GetInstance()
{
    static SleAudioFrameworkAdapter instance;
    return instance;
}

void SleAudioFrameworkAdapter::GetCurrentOutputPipeInfos()
{
    HILOGI("[SleAudioFrameworkAdapter] enter");
    DoInAudioFwAdapterThread([this]() {
        GetCurrentOutputPipeInfosInner();
    });
}

void SleAudioFrameworkAdapter::GetCurrentOutputPipeInfosInner()
{
    std::vector<std::shared_ptr<AudioOutputPipeInfo>> outputPipeInfos;
    auto ptr = DelayedSingleton<AudioStandard::AudioEngineClientManager>::GetInstance();
    NL_CHECK_RETURN(ptr != nullptr, "[SleAudioFrameworkAdapter] Get ptr nullptr!");
    ptr->GetCurrentOutputPipeChangeInfos(outputPipeInfos);
    for (const auto& pipeInfo : outputPipeInfos) {
        NL_CHECK_RETURN(pipeInfo != nullptr, "[SleAudioFrameworkAdapter] Get pipeInfo nullptr!");
        AudioStandard::HdiAdapterType adapterType = pipeInfo->GetAdapter();

        if (!IsSlePipe(adapterType)) {
            HILOGD("[SleAudioFrameworkAdapter] not sle device");
            continue;
        }
        uint32_t routeFlag = pipeInfo->GetRouteFlag();
        auto rendererStreams = pipeInfo->GetStreams();
        UpdateRouteFlagList(routeFlag, rendererStreams);
    }
}

void SleAudioFrameworkAdapter::RegisterOutputPipeChangeListener()
{
    audioOutputPipeCallback_ = std::make_shared<SleAudioOutputPipeListener>();
    auto ptr = DelayedSingleton<AudioStandard::AudioEngineClientManager>::GetInstance();
    NL_CHECK_RETURN(ptr != nullptr, "[SleAudioFrameworkAdapter] Get ptr nullptr!");
    auto ret = ptr->RegisterOutputPipeChangeCallback(audioOutputPipeCallback_);
    if (ret == SUCCESS) {
        GetCurrentOutputPipeInfosInner();
    }
    HILOGI("[SleAudioFrameworkAdapter] RegisterOutputPipeChangeListener ret=%{public}d", ret);
}

void SleAudioFrameworkAdapter::UnregisterOutputPipeChangeListener()
{
    if (!audioOutputPipeCallback_) {
        HILOGW("[SleAudioFrameworkAdapter] audioOutputPipeCallback_ is null, nothing to unregister");
        return;
    }
    auto ptr = DelayedSingleton<AudioStandard::AudioEngineClientManager>::GetInstance();
    NL_CHECK_RETURN(ptr != nullptr, "[SleAudioFrameworkAdapter] Get ptr nullptr!");
    auto ret = ptr->UnregisterOutputPipeChangeCallback(audioOutputPipeCallback_);
    audioOutputPipeCallback_ = nullptr;
    HILOGI("[SleAudioFrameworkAdapter] UnregisterOutputPipeChangeListener ret=%{public}d", ret);
}

void SleAudioFrameworkAdapter::JudgeDataMuteStreamByRendererState(
    const RawAddress &device, const uint32_t streamId, bool isStart)
{
    HILOGD("[SleAudioFrameworkAdapter] enter");
    if (!isStart) {
        StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_DATA_ZERO);
        McpServerService *mcpService = McpServerService::GetService();
        NL_CHECK_RETURN(mcpService != nullptr, "[SleAudioFrameworkAdapter] mcpService is null!");
        std::string appName = GetBundleName(streamId);
        NearlinkDftUe::GetInstance().WriteAudioMuteStreamUe(device, MUTE_STREAM, DATA_MUTE_RESUME,
            MCP_PLAYBACK_PAUSE_DEBOUNCE_TIMEOUT, appName);
        HILOGI("[SleAudioFrameworkAdapter] MCP_PLAYBACK_STATE_PLAY");
        mcpService->GetMediaLoader()->RendererStreamStateChange(MCP_PLAYBACK_STATE_PLAY);
        return;
    }
    for (const auto &it : GetRouteFlagList()) {
        if (isCallStreamType(it.streamUsage)) {
            StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_DATA_ZERO);
            return;
        }
        /* 匹配静音流的streamid */
        if (it.streamId == streamId) {
            continue;
        }
        if (it.status == AudioStandard::RendererState::RENDERER_RUNNING) {
            isStart = false;
            return;
        }
    }
    StartEmptyMuteScMcpPauseTimer(device, isStart, streamId, MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_DATA_ZERO);
}

void SleAudioFrameworkAdapter::HandleDataTransferStateInner(
    const AudioStandard::AudioRendererDataTransferStateChangeInfo &info)
{
    ASCService *ascService = ASCService::GetService();
    NL_CHECK_RETURN(ascService != nullptr, "[SleAudioFrameworkAdapter] Get ascService error!");
    RawAddress device = ascService->GetActiveSinkDevice();
    NL_CHECK_RETURN(IsValidAddress(device.GetAddress()), "[SleAudioFrameworkAdapter] Invalid Address!");
    if (!SleRemoteDeviceAdapter::GetInstance()->IsVendorDevice(device)) {
        HILOGD("[SleAudioFrameworkAdapter] not VendorDevice device");
        return;
    }
    /* audioMode: 0播放，1录音 */
    NL_CHECK_RETURN((info.audioMode != AudioStandard::AUDIO_MODE_RECORD), "[SleAudioFrameworkAdapter] record!");
    AudioStandard::DataTransferStateChangeType rendererState = info.stateChangeType;
    switch (rendererState) {
        case AudioStandard::DataTransferStateChangeType::AUDIO_STREAM_START:
        case AudioStandard::DataTransferStateChangeType::AUDIO_STREAM_STOP:
        case AudioStandard::DataTransferStateChangeType::AUDIO_STREAM_PAUSE:
            break;
        // 静音流恢复正常
        case AudioStandard::DataTransferStateChangeType::DATA_TRANS_RESUME:
            HILOGI("[SleAudioFrameworkAdapter] DATA_TRANS_RESUME");
            JudgeDataMuteStreamByRendererState(device, info.sessionId, false);
            break;
        // 静音流触发
        case AudioStandard::DataTransferStateChangeType::DATA_TRANS_STOP:
            HILOGI("[SleAudioFrameworkAdapter] DATA_TRANS_STOP");
            JudgeDataMuteStreamByRendererState(device, info.sessionId, true);
            break;
        default:
            return;
    }
}

/* 数据为0的静音流回调 */
void SleAudioFrameworkAdapter::SleAudioRendererDataTransferListener::OnDataTransferStateChange(
    const AudioStandard::AudioRendererDataTransferStateChangeInfo &info)
{
    HILOGI("[SleAudioFrameworkAdapter] enter");
    DoInAudioFwAdapterThread([info]() {
        SleAudioFrameworkAdapter::GetInstance().HandleDataTransferStateInner(info);
    });
}

void SleAudioFrameworkAdapter::JudgeVolumeMuteStreamByRendererState(
    const RawAddress &device, const uint32_t streamId, bool isStart)
{
    HILOGI("[SleAudioFrameworkAdapter] streamId:%{public}d isStart:%{public}d", streamId, isStart);
    if (!isStart) {
        StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_VOL_ZERO);
        McpServerService *mcpService = McpServerService::GetService();
        NL_CHECK_RETURN(mcpService != nullptr, "[SleAudioFrameworkAdapter] mcpService is null!");
        std::string appName = GetBundleName(streamId);
        NearlinkDftUe::GetInstance().WriteAudioMuteStreamUe(device, MUTE_STREAM, VOLUME_ZERO_RESUME,
            MCP_PLAYBACK_PAUSE_DEBOUNCE_TIMEOUT, appName);
        HILOGI("[SleAudioFrameworkAdapter] MCP_PLAYBACK_STATE_PLAY");
        mcpService->GetMediaLoader()->RendererStreamStateChange(MCP_PLAYBACK_STATE_PLAY);
        return;
    }
    for (const auto &it : GetRouteFlagList()) {
        if (isCallStreamType(it.streamUsage)) {
            StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_VOL_ZERO);
            return;
        }
        if (it.streamId == streamId) {
            continue;
        }
        if (it.status == AudioStandard::RendererState::RENDERER_RUNNING) {
            isStart = false;
            return;
        }
    }
    StartEmptyMuteScMcpPauseTimer(device, isStart, streamId, MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_VOL_ZERO);
}

void SleAudioFrameworkAdapter::HandleVolumeMuteStateInner(const uint32_t streamId,
    const bool &isMuted)
{
    ASCService *ascService = ASCService::GetService();
    NL_CHECK_RETURN(ascService != nullptr, "[SleAudioFrameworkAdapter] Get ascService error!");
    RawAddress device = ascService->GetActiveSinkDevice();
    NL_CHECK_RETURN(IsValidAddress(device.GetAddress()), "[SleAudioFrameworkAdapter] Invalid Address!");
    if (!SleRemoteDeviceAdapter::GetInstance()->IsVendorDevice(device)) {
        HILOGD("[SleAudioFrameworkAdapter] not VendorDevice device");
        return;
    }
    JudgeVolumeMuteStreamByRendererState(device, streamId, isMuted);
}

/* 音量为0的静音流回调 */
void SleAudioFrameworkAdapter::SleAudioRendererDataTransferListener::OnMuteStateChange(const int32_t &uid,
    const uint32_t &streamId, const bool &isMuted)
{
    DoInAudioFwAdapterThread([isMuted, streamId]() {
        SleAudioFrameworkAdapter::GetInstance().HandleVolumeMuteStateInner(streamId, isMuted);
    });
}

std::shared_ptr<NearlinkTimer> SleAudioFrameworkAdapter::GetEmptyMuteCheckTimer(MCP_MUTE_CONTROL type)
{
    std::shared_ptr<NearlinkTimer> timer = nullptr;
    switch (type) {
        case MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM:
            timer = emptyCheckTimer_;
            break;
        case MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_DATA_ZERO:
            timer = dataMuteCheckTimer_;
            break;
        case MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_VOL_ZERO:
            timer = volumeMuteCheckTimer_;
            break;
        default:
            break;
    }
    return timer;
}

void SleAudioFrameworkAdapter::SetEmptyMuteCheckTimer(MCP_MUTE_CONTROL type, std::shared_ptr<NearlinkTimer> timer)
{
    switch (type) {
        case MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM:
            emptyCheckTimer_ = timer;
            break;
        case MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_DATA_ZERO:
            dataMuteCheckTimer_ = timer;
            break;
        case MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_VOL_ZERO:
            volumeMuteCheckTimer_ = timer;
            break;
        default:
            break;
    }
}

void SleAudioFrameworkAdapter::DftEmptyMuteCheck(const RawAddress &device, MCP_MUTE_CONTROL type, uint32_t streamId)
{
    int sceneCode = 0;
    int subSceneCode = 0;
    std::string appName = GetBundleName(streamId);
    switch (type) {
        case MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM:
            sceneCode = EMPTY_STREAM;
            subSceneCode = STREAM_NO_DATA;
            break;
        case MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_DATA_ZERO:
            sceneCode = MUTE_STREAM;
            subSceneCode = STREAM_DATA_MUTE;
            break;
        case MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_VOL_ZERO:
            sceneCode = MUTE_STREAM;
            subSceneCode = STREAM_VOLUME_ZERO;
            break;
        default:
            return;
    }
    NearlinkDftUe::GetInstance().WriteAudioMuteStreamUe(device, sceneCode, subSceneCode,
        MCP_PLAYBACK_PAUSE_DEBOUNCE_TIMEOUT, appName);
}

/* 开始空静音流控-stream control发送MCP pause防抖定时器 */
void SleAudioFrameworkAdapter::StartEmptyMuteScMcpPauseTimer(const RawAddress &device,
    bool isStart, uint32_t streamId, MCP_MUTE_CONTROL type)
{
    if (!isStart) {
        return;
    }
    McpServerService *mcpService = McpServerService::GetService();
    NL_CHECK_RETURN(mcpService != nullptr, "[SleAudioFrameworkAdapter] mcpService is null!");
    if (GetEmptyMuteCheckTimer(type) != nullptr) {
        HILOGD("[SleAudioFrameworkAdapter] type=%{public}d timer exists, skipping create", static_cast<int>(type));
        return;
    }

    auto timeoutFunc = [this, device, type, streamId]() {
        McpServerService *mcpService = McpServerService::GetService();
        NL_CHECK_RETURN(mcpService != nullptr, "[SleAudioFrameworkAdapter] mcpService is null!");
        // MCP无感知仍然在播放状态
        if (mcpService->GetMediaLoader()->GetMcpRealTimeRenderState() != McpRenderState::KPlaying) {
            HILOGD("[SleAudioFrameworkAdapter] mcp render state not play");
            return;
        }
        if (type != NL_SLE_MCP_EMPTY_STREAM) {
            mcpService->GetMediaLoader()->RendererStreamStateChange(MCP_PLAYBACK_STATE_PAUSE);
        }
        DoInAudioFwAdapterThread([this, device, type, streamId]() {
            HILOGI("[SleAudioFrameworkAdapter] type=%{public}d timeout mcp pause trigger", static_cast<int>(type));
            SetEmptyMuteCheckTimer(type, nullptr);
            DftEmptyMuteCheck(device, type, streamId);
        });
    };
    SetEmptyMuteCheckTimer(type, std::make_shared<NearlinkTimer>(timeoutFunc));
    int32_t time = MCP_PLAYBACK_PAUSE_DEBOUNCE_TIMEOUT;
    auto timer = GetEmptyMuteCheckTimer(type);
    NL_CHECK_RETURN(timer != nullptr, "[SleAudioFrameworkAdapter] timer is null!");
    timer->Start(time, false);
    HILOGI("[SleAudioFrameworkAdapter] type=%{public}d start check timer", static_cast<int>(type));
}

void SleAudioFrameworkAdapter::StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL type)
{
    if (GetEmptyMuteCheckTimer(type) != nullptr) {
        HILOGI("[SleAudioFrameworkAdapter] type=%{public}d stop timer", static_cast<int>(type));
        GetEmptyMuteCheckTimer(type)->Stop();
        SetEmptyMuteCheckTimer(type, nullptr);
    }
}

void SleAudioFrameworkAdapter::RegisterRendererDataTransferListener()
{
    AudioStandard::DataTransferMonitorParam param;
    param.clientUID = ALL_UID;
    param.badDataTransferTypeBitMap = MUTE_MAP;
    param.timeInterval = CHECK_TIME_INTERVAL;  // 检视时间
    param.badFramesRatio = BAD_FRAMES_RATIO;   // 检测异常占比
    audioRendererDataCallback_ = std::make_shared<SleAudioRendererDataTransferListener>();
    auto ret = OHOS::AudioStandard::AudioStreamClientManager::GetInstance().RegisterRendererDataTransferCallback(
        param, audioRendererDataCallback_);
    HILOGI("[SleAudioFrameworkAdapter] RegisterRendererDataTransferListener ret=%{public}d", ret);
}

void SleAudioFrameworkAdapter::UnregisterRendererDataTransferListener()
{
    if (!audioRendererDataCallback_) {
        HILOGW("[SleAudioFrameworkAdapter] audioRendererDataCallback_ is null, nothing to unregister");
        return;
    }
    auto ret = OHOS::AudioStandard::AudioStreamClientManager::GetInstance().UnregisterRendererDataTransferCallback(
        audioRendererDataCallback_);
    audioRendererDataCallback_ = nullptr;

    HILOGI("[SleAudioFrameworkAdapter] UnregisterRendererDataTransferListener ret=%{public}d", ret);
}
void SleAudioFrameworkAdapter::RegisterCollaborativeAudioListener()
{
    collaborativeAudioModeCallback_ = std::make_shared<SleColAudioEnableListener>();
    AudioStandard::AudioCollaborativeManager::GetInstance()->
        RegisterCollaborationEnabledForCurrentDeviceEventListener(collaborativeAudioModeCallback_);
    HILOGI("[SleAudioFrameworkAdapter] RegisterCollaborativeAudioListener done");
}

void SleAudioFrameworkAdapter::SleColAudioEnableListener::OnCollaborationEnabledChangeForCurrentDevice(
    const bool &enabled)
{
    HILOGI("[SleAudioFrameworkAdapter] enter");
    DoInAudioFwAdapterThread([enabled]() {
        SleAudioFrameworkAdapter::GetInstance().HandleColAudioEnabledChangeInner(enabled);
    });
}

void SleAudioFrameworkAdapter::HandleColAudioEnabledChangeInner(const bool enabled)
{
    ProfileASC *ascProfileService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN(ascProfileService, "cant find ASC service");
    RawAddress device = ascProfileService->GetActiveSinkDevice();
    HILOGI("[SleAudioFrameworkAdapter], device=%{public}s, enable=%{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), enabled);
    ASCService *ascService = static_cast<ASCService *>(ascProfileService);
    NL_CHECK_RETURN(ascService, "ASC service null");
    ASCMessage event(ASC_COL_AUDIO_SWITCH_CHANGE_EVT);
    event.dev_ = device.GetAddress();
    event.result_ = enabled;
    ascService->PostEvent(event);
}
}
}