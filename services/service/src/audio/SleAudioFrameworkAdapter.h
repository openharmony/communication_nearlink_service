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
#ifndef SLE_AUDIO_FRAMEWORK_ADAPTER_H
#define SLE_AUDIO_FRAMEWORK_ADAPTER_H

#include "audio_routing_manager.h"
#include "audio_engine_callback_types.h"
#include "audio_engine_client_manager.h"
#include "audio_stream_enum.h"
#include "audio_collaborative_manager.h"

#include "nearlink_types.h"
#include "McpMessage.h"
#include "raw_address.h"
#include "SleServiceFfrtLog.h"
#include "nearlink_timer.h"
#include "McpDefines.h"
#include "ASCService.h"

namespace OHOS {
namespace Nearlink {

struct RouteFlagInfo {
    uint32_t routeFlag;
    uint32_t streamId;
    uint32_t streamUsage;
    AudioStandard::RendererState status;
    std::string bundleName;
};

class SleAudioFrameworkAdapter {
public:
    explicit SleAudioFrameworkAdapter() {}
    ~SleAudioFrameworkAdapter() {}
    static SleAudioFrameworkAdapter &GetInstance();

    void RegisterAudioFrameworkAdapterListener();
    void UnregisterAudioFrameworkAdapterListener();
    void GetCurrentOutputPipeInfos();
    bool IsMusicActive();
    bool IsVoiceCallActive();
    bool IsAudioServiceActivate();
    bool IsNearlinkOut();
    bool IsBtOut();
    void RegisterCollaborativeAudioListener();

private:
    void RegisterOutputPipeChangeListener();
    void UnregisterOutputPipeChangeListener();
    void RegisterRendererDataTransferListener();
    void UnregisterRendererDataTransferListener();
    void UpdateRouteFlagList(uint32_t flag,
        const std::map<uint32_t, AudioStandard::RendererStreamInfo>& rendererStreams);
    void GetCurrentOutputPipeInfosInner();
    void AddMemberRouteFlagList(const RouteFlagInfo &newInfo);
    void RemoveMemberRouteFlagList(const uint32_t &flag);
    const std::vector<RouteFlagInfo> &GetRouteFlagList() const;
    std::string GetBundleName(uint32_t streamId);
    void ClearRouteFlagList();
    bool isCallStreamType(uint32_t streamUsage);
    std::shared_ptr<NearlinkTimer> GetEmptyMuteCheckTimer(MCP_MUTE_CONTROL type);
    void SetEmptyMuteCheckTimer(MCP_MUTE_CONTROL type, std::shared_ptr<NearlinkTimer> timer);
    void StartEmptyMuteScMcpPauseTimer(const RawAddress &device,
        bool isStart, uint32_t streamId, MCP_MUTE_CONTROL type);
    void StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL type);
    void DftEmptyMuteCheck(const RawAddress &device, MCP_MUTE_CONTROL type, uint32_t streamId);
    void HandleAudioPipeStatusChangeInner(const AudioStandard::AudioPipeChangeType changeType,
        const std::shared_ptr<AudioStandard::AudioOutputPipeInfo> &changedPipeInfo);

    void JudgeEmptyStreamByRendererState(const RawAddress &device);
    bool IsSlePipe(AudioStandard::HdiAdapterType adapterType);

    void HandleDataTransferStateInner(const AudioStandard::AudioRendererDataTransferStateChangeInfo &info);
    void JudgeDataMuteStreamByRendererState(const RawAddress &device, const uint32_t streamId, bool isStart);
    void HandleVolumeMuteStateInner(const uint32_t streamId, const bool &isMuted);
    void JudgeVolumeMuteStreamByRendererState(const RawAddress &device, const uint32_t streamId, bool isStart);
    void HandleColAudioEnabledChangeInner(const bool enabled);

private:
    class SleAudioOutputPipeListener : public AudioStandard::AudioOutputPipeCallback {
    public:
        SleAudioOutputPipeListener() = default;
        ~SleAudioOutputPipeListener() override = default;
    private:
        void OnOutputPipeChange(const AudioStandard::AudioPipeChangeType changeType,
            const std::shared_ptr<AudioStandard::AudioOutputPipeInfo> &changedPipeInfo) override;
    };

    class SleAudioRendererDataTransferListener : public AudioStandard::AudioRendererDataTransferStateChangeCallback {
    public:
        SleAudioRendererDataTransferListener() = default;
        ~SleAudioRendererDataTransferListener() override = default;
    private:
        void OnDataTransferStateChange(const AudioStandard::AudioRendererDataTransferStateChangeInfo &info) override;
        void OnMuteStateChange(const int32_t &uid, const uint32_t &sessionId, const bool &isMuted) override;
    };

    class SleColAudioEnableListener
        : public AudioStandard::AudioCollaborationEnabledChangeForCurrentDeviceCallback {
    public:
        SleColAudioEnableListener() = default;
        ~SleColAudioEnableListener() override = default;
    private:
        void OnCollaborationEnabledChangeForCurrentDevice(const bool &enabled) override;
    };

    std::shared_ptr<NearlinkTimer> emptyCheckTimer_ = nullptr;
    std::shared_ptr<NearlinkTimer> dataMuteCheckTimer_ = nullptr;
    std::shared_ptr<NearlinkTimer> volumeMuteCheckTimer_ = nullptr;
    std::vector<RouteFlagInfo> routeFlagList_ {};
    std::shared_ptr<AudioStandard::AudioOutputPipeCallback> audioOutputPipeCallback_ = nullptr;
    std::shared_ptr<AudioStandard::AudioRendererDataTransferStateChangeCallback> audioRendererDataCallback_ = nullptr;
    std::shared_ptr<AudioStandard::AudioCollaborationEnabledChangeForCurrentDeviceCallback>
        collaborativeAudioModeCallback_ = nullptr;
};
}
}

#endif // SLE_AUDIO_FRAMEWORK_ADAPTER_H