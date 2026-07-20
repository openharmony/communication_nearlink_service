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
#include "McpServerServiceManager.h"

#include <memory>
#include <cstring>
#include <cmath>
#include <unordered_map>
#include "ipc_skeleton.h"
#include "SleInterfaceProfileManager.h"
#include "ClassCreator.h"
#include "ThreadUtil.h"
#include "McpDefines.h"
#include "avsession_errors.h"
#include "avsession_manager.h"
#include "avsession_controller.h"
#include "avplayback_state.h"
#include "nlstk_mcp_media_server.h"
#include "audio_stream_client_policy.h"
#include "nearlink_dft_ue.h"
#include "nearlink_dft_exception.h"
#include "nearlink_timer.h"
#include <future>

namespace OHOS {
namespace Nearlink {
struct McpServerServiceManager::impl : public std::enable_shared_from_this<McpServerServiceManager::impl>{
public:
    /* 初始化 */
    void Init(std::weak_ptr<McpServerServiceManager::impl> impl);
    void CleanUp();
    void InitMediaListener();
    /* 收到控制事件（对端或者佩戴检测触发）后，转发给AvSession */
    void SendKeyEventToAvSession(const RawAddress &device, uint8_t key, uint16_t requestId = 0, int32_t instanceId = 0);

    /* AVSession相关回调处理 */
    void OnSessionRelease(const AVSession::AVSessionDescriptor &descriptor);
    void OnTopSessionChange(const AVSession::AVSessionDescriptor &descriptor);
    void OnMetaDataChange(const AVSession::AVMetaData &data);
    void OnPlaybackStateChange(const AVSession::AVPlaybackState &state);
    void OnRendererStateChange(const std::vector<std::shared_ptr<AudioStandard::AudioRendererChangeInfo>>
            &audioRendererChangeInfos);

    void DftMediaPlayerStatusChange(int mediaPlayerSubScene);
    void UpdateMediaPlaybackState(int32_t state);

    bool CheckNowHasDeviceConnected();
    bool GetAVPlaybackState(AVSession::AVPlaybackState &state);
    void SetTwsProfileFunc(GetTwsFunc func);
    void SetCdsmProfileFunc(GetCdsmFunc func);
    void SetAscProfileFunc(GetAscFunc func);

    void ProcessEvent(const McpMessage &event);
    McpRenderState GetMcpRealTimeRenderState();

    /**
    * @brief This class implement the <b>SessionListener</b> interface for observing the state change.
    */
    class AVSessionObserverImpl : public AVSession::SessionListener {
    public:
        explicit AVSessionObserverImpl(std::weak_ptr<McpServerServiceManager::impl> implPtr) : pimpl_(implPtr) {}
        ~AVSessionObserverImpl() override = default;

        void OnSessionCreate(const AVSession::AVSessionDescriptor &descriptor) override { }

        void OnSessionRelease(const AVSession::AVSessionDescriptor &descriptor) override
        {
            HILOGI("[McpServer]OnSessionRelease");
            auto ptr = pimpl_.lock();
            if (ptr) {
                auto func = [ptr](const AVSession::AVSessionDescriptor &descriptor) {
                    ptr->OnSessionRelease(descriptor);
                };
                DoInMcpThread([func, descriptor]() { func(descriptor); });
            }
        }
        void OnTopSessionChange(const AVSession::AVSessionDescriptor &descriptor) override
        {
            HILOGI("[McpServer]OnTopSessionChange");
            auto ptr = pimpl_.lock();
            if (ptr) {
                auto func = [ptr](const AVSession::AVSessionDescriptor &descriptor) {
                    ptr->OnTopSessionChange(descriptor);
                };
                DoInMcpThread([func, descriptor]() { func(descriptor); });
            }
        }

    private:
        std::weak_ptr<McpServerServiceManager::impl> pimpl_;
    };

    /**
    * @brief This class implement the <b>SessionListener</b> interface for observing the state change.
    */
    class AVControllerObserverImpl : public AVSession::AVControllerCallback {
    public:
        explicit AVControllerObserverImpl(std::weak_ptr<McpServerServiceManager::impl> implPtr) : pimpl_(implPtr) {}
        ~AVControllerObserverImpl() override = default;

        void OnMetaDataChange(const AVSession::AVMetaData &data) override
        {
            HILOGI("[McpServer]received metaDataChange event");
            auto ptr = pimpl_.lock();
            if (ptr) {
                NL_CHECK_RETURN(ptr->CheckNowHasDeviceConnected(), "current connect nearlink device is empty.");
                auto func = [ptr](const AVSession::AVMetaData &data) {
                    ptr->OnMetaDataChange(data);
                };
                DoInMcpThread([func, data]() { func(data); });
            }
        }

        void OnPlaybackStateChange(const AVSession::AVPlaybackState &state) override
        {
            HILOGI("[McpServer]OnPlaybackStateChange state:%{public}d", state.GetState());
            auto ptr = pimpl_.lock();
            if (ptr) {
                NL_CHECK_RETURN(ptr->CheckNowHasDeviceConnected(), "current connect nearlink device is empty.");
                auto func = [ptr](const AVSession::AVPlaybackState &state) {
                    ptr->OnPlaybackStateChange(state);
                };
                HILOGI("[McpServer]switch");
                DoInMcpThread([func, state]() { func(state); });
            } else {
                HILOGI("[McpServer]ptr is null");
            }
        }

        void OnSessionDestroy() override {}
        void OnActiveStateChange(bool isActive) override {}
        void OnValidCommandChange(const std::vector<int32_t> &cmds) override {}
        void OnOutputDeviceChange(const int32_t, const AVSession::OutputDeviceInfo &) override {}
        void OnSessionEventChange(const std::string &event, const AAFwk::WantParams &args) override {}
        void OnQueueItemsChange(const std::vector<AVSession::AVQueueItem> &items) override {}
        void OnQueueTitleChange(const std::string &title) override {}
        void OnExtrasChange(const AAFwk::WantParams &extras) override {}
        void OnAVCallStateChange(const AVSession::AVCallState &avCallState) override {}
        void OnAVCallMetaDataChange(const AVSession::AVCallMetaData &avCallMetaData) override {}

    private:
        std::weak_ptr<McpServerServiceManager::impl> pimpl_;
    };

    class NearlinkAudioRendererStateListener : public AudioStandard::AudioRendererStateChangeCallback {
    public:
        explicit NearlinkAudioRendererStateListener(std::weak_ptr<impl> implPtr) : pimpl_(implPtr) {}
        ~NearlinkAudioRendererStateListener() override = default;

    private:
        void OnRendererStateChange(const std::vector<std::shared_ptr<AudioStandard::AudioRendererChangeInfo>>
            &audioRendererChangeInfos) override
        {
            HILOGD("[McpServer]OnRendererStateChange");
            auto ptr = pimpl_.lock();
            if (ptr) {
                NL_CHECK_RETURN_LOGD(ptr->CheckNowHasDeviceConnected(), "current connect nearlink device is empty");
                auto func = [ptr](const std::vector<std::shared_ptr<AudioStandard::AudioRendererChangeInfo>> &infos) {
                    ptr->OnRendererStateChange(infos);
                };
                DoInMcpThread([func, audioRendererChangeInfos]() { func(audioRendererChangeInfos); });
            }
        }

    private:
        std::weak_ptr<impl> pimpl_;
    };

private:
    /* 消息处理 */
    bool IsActiveDevice(std::string &addr);
    void NotifyMediaStackChanged(const McpMessage &event);
    void WriteAudioSinkDeviceUe(int mcpEvt, std::string addr, int32_t controlResult);
    void SendKeyEvent(const McpMessage &event);
    void NotifyAudioDeviceAction(const RawAddress &devAddr, int action);

    /* 初始化媒体控制实例 */
    void CreateMcpInstance(uint8_t instanceId);
    /* 初始化AvSession相关 */
    void RegisterAvSessionListener();
    void RegisterAudioRendererEventListener();
    void AttemptCreateAvSessionController();
    void CreateAvSessionController(const AVSession::AVSessionDescriptor &descriptor);
    void SetAVSessionFilter();
    void ProcessMediaStateChange();
    /* 判断AVSession返回的播放状态是否变化 */
    bool CheckPlaybackStateChanged(const AVSession::AVPlaybackState &state);
    bool CheckMetaDataChanged(const AVSession::AVMetaData &data);
    /* 更新对端相关属性 */
    void UpdateMediaBasicInfo();
    int32_t ConvertAVPlaybackStateToMcpState(int32_t state);
    void UpdateMediaPlaybackLocation();
    void UpdateMediaControlResult(const RawAddress &addr, uint16_t requestId, int32_t instanceId, uint8_t errorCode);
    void StartStateTimer();

private:
    std::shared_ptr<AVSession::AVSessionController> avSessionController_{ nullptr };
    std::shared_ptr<AVControllerObserverImpl> avControllerObserver_{ nullptr };
    std::shared_ptr<AVSessionObserverImpl> avSessionObserver_{ nullptr };
    std::shared_ptr<NearlinkAudioRendererStateListener> rendererStateCallback_{ nullptr };
    GetTwsFunc getTwsProfileFunc_ = nullptr;
    GetCdsmFunc getCdsmProfileFunc_ = nullptr;
    GetAscFunc getAscProfileFunc_ = nullptr;
    bool isCreatedAVSession = false;

    // mcpInstanceId等于0为非法值
    uint8_t mcpInstanceId_ = MCP_INSTANCE_ID;

    AVSession::AVSessionDescriptor currDescriptor_;
    AVSession::AVPlaybackState currPlaybackState_;
    AVSession::AVMetaData currMateData_;
    std::shared_ptr<NearlinkTimer> stateTimer_ = nullptr;
    McpRenderState renderState_ = McpRenderState::KUninitalize;
    McpRenderState realTimeRenderState_ = McpRenderState::KUninitalize;
    NLSTK_McpReadyState_E currentReportState_ = NLSTK_MCP_STATE_UNINITIALIZED;
    int64_t currPosition_;

    /* AVSession MediaType与Stack映射表 */
    const std::unordered_map<int32_t, NLSTK_McpMediaType_E> avMediaTypeToStackMap_ = {
        { OHOS::AVSession::AVSession::SESSION_TYPE_AUDIO, NLSTK_MEDIA_AUDIO },
        { OHOS::AVSession::AVSession::SESSION_TYPE_VIDEO, NLSTK_MEDIA_VIDEO },
        { OHOS::AVSession::AVSession::SESSION_TYPE_VOICE_CALL, NLSTK_MEDIA_AUDIO },
        { OHOS::AVSession::AVSession::SESSION_TYPE_VIDEO_CALL, NLSTK_MEDIA_AUDIO_VIDEO }
    };

    /* AVSession State与Stack映射表 */
    const std::unordered_map<int32_t, NLSTK_McpReadyState_E> avStateToStackMap_ = {
        { AVSession::AVPlaybackState::PLAYBACK_STATE_INITIAL, NLSTK_MCP_STATE_READY },
        { AVSession::AVPlaybackState::PLAYBACK_STATE_PLAY, NLSTK_MCP_STATE_PLAYING },
        { AVSession::AVPlaybackState::PLAYBACK_STATE_PAUSE, NLSTK_MCP_STATE_READY },
        { AVSession::AVPlaybackState::PLAYBACK_STATE_FAST_FORWARD, NLSTK_MCP_STATE_SEEKING },
        { AVSession::AVPlaybackState::PLAYBACK_STATE_REWIND, NLSTK_MCP_STATE_SEEKING },
        { AVSession::AVPlaybackState::PLAYBACK_STATE_STOP, NLSTK_MCP_STATE_READY },
        { AVSession::AVPlaybackState::PLAYBACK_STATE_COMPLETED, NLSTK_MCP_STATE_READY },
        { AVSession::AVPlaybackState::PLAYBACK_STATE_BUFFERING, NLSTK_MCP_STATE_PLAYING },
    };

    /* MCP事件与KeyEvent映射表 */
    const std::unordered_map<int32_t, int32_t> mcpToAKeyEventCodeMap_ = {
        { MCP_ID_PLAY, MMI::KeyEvent::KEYCODE_MEDIA_PLAY },
        { MCP_ID_STOP, MMI::KeyEvent::KEYCODE_MEDIA_STOP },
        { MCP_ID_PAUSE, MMI::KeyEvent::KEYCODE_MEDIA_PAUSE },
        { MCP_ID_FAST_FOR, MMI::KeyEvent::KEYCODE_MEDIA_FAST_FORWARD },
        { MCP_ID_NEXT_MEDIA, MMI::KeyEvent::KEYCODE_MEDIA_NEXT },
        { MCP_ID_PRE_MEDIA, MMI::KeyEvent::KEYCODE_MEDIA_PREVIOUS },
    };
};

void McpServerServiceManager::impl::Init(std::weak_ptr<McpServerServiceManager::impl> impl)
{
    HILOGI("[McpServer]enter, start init");
    // 创建通用媒体播放控制实例
    avSessionObserver_ = std::make_shared<AVSessionObserverImpl>(impl);
    avControllerObserver_ = std::make_shared<AVControllerObserverImpl>(impl);
    rendererStateCallback_ = std::make_shared<NearlinkAudioRendererStateListener>(impl);
}

void McpServerServiceManager::impl::CleanUp()
{
    HILOGI("[McpServer]enter");
    if (avSessionController_) {
        avSessionController_->Destroy();
        avSessionController_ = nullptr;
    }
    avSessionObserver_ = nullptr;
    avControllerObserver_ = nullptr;
    rendererStateCallback_ = nullptr;
    isCreatedAVSession = false;
}

void McpServerServiceManager::impl::InitMediaListener()
{
    if (isCreatedAVSession) {
        HILOGD("[McpServer]AvSessionController is Created!");
        ProcessMediaStateChange();
        return;
    }
    // 注册AVSession相关监听
    RegisterAvSessionListener();
    AttemptCreateAvSessionController();
    RegisterAudioRendererEventListener();
    isCreatedAVSession = true;
}

bool McpServerServiceManager::impl::CheckNowHasDeviceConnected()
{
    NL_CHECK_RETURN_RET(getAscProfileFunc_, false, "[McpServer]getAscProfileFunc_ is null");
    ProfileASC *ascService = getAscProfileFunc_();
    NL_CHECK_RETURN_RET(ascService != nullptr, false, "[McpServer]Get ascService error!");
    // MCP做服务端，Service中不感知当前已连接的设备，所以通过ASC中获取来判断
    std::list<RawAddress> connectDevices = ascService->GetConnectDevices();
    return !connectDevices.empty();
}

void McpServerServiceManager::impl::DftMediaPlayerStatusChange(int mediaPlayerSubScene)
{
    NL_CHECK_RETURN(getAscProfileFunc_, "[McpServer]getAscProfileFunc_ is null");
    ProfileASC *ascService = getAscProfileFunc_();
    NL_CHECK_RETURN(ascService != nullptr, "[McpServer]Get ascService error!");
    NearlinkRawAddress device = ascService->GetActiveSinkDevice();
    RawAddress addr(device.GetAddress());
    NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(addr, addr, MEDIA_PLAYER_STATUS_CHANGE,
        mediaPlayerSubScene);
}

void McpServerServiceManager::impl::OnSessionRelease(const AVSession::AVSessionDescriptor &descriptor)
{
    HILOGI("[McpServer]enter");
    auto sessionId = descriptor.sessionId_;
    NL_CHECK_RETURN(!sessionId.empty(), "[McpServer]AvSession sessionId is empty");
    NL_CHECK_RETURN(avSessionController_, "[McpServer]current avSessionController is null");
    if (avSessionController_->GetSessionId() == sessionId) {
        avSessionController_->Destroy();
        avSessionController_ = nullptr;
        currPlaybackState_.SetState(OHOS::AVSession::AVPlaybackState::PLAYBACK_STATE_STOP);
        DftMediaPlayerStatusChange(MEDIA_PLAYER_RELEASE);
    }
}

void McpServerServiceManager::impl::OnTopSessionChange(const AVSession::AVSessionDescriptor &descriptor)
{
    auto sessionId = descriptor.sessionId_;
    NL_CHECK_RETURN(!sessionId.empty(), "[McpServer]AvSession sessionId is empty");
    if (!avSessionController_) {
        CreateAvSessionController(descriptor);
        HILOGI("[McpServer]first create session controller sessionId:%{public}s", sessionId.c_str());
        return;
    }
    if (avSessionController_->GetSessionId() == sessionId) {
        HILOGI("[McpServer]same session, sessionId:%{public}s", sessionId.c_str());
        return;
    }

    avSessionController_->Destroy();
    avSessionController_ = nullptr;
    currPlaybackState_.SetState(OHOS::AVSession::AVPlaybackState::PLAYBACK_STATE_STOP);
    CreateAvSessionController(descriptor);
    DftMediaPlayerStatusChange(MEDIA_PLAYER_CHANGE);
    HILOGI("[McpServer]re create session controller sessionId:%{public}s", sessionId.c_str());
}

void McpServerServiceManager::impl::OnMetaDataChange(const AVSession::AVMetaData &data)
{
    HILOGI("[McpServer]enter");
    NL_CHECK_RETURN(avSessionController_, "[McpServer]current avSessionController is null");
    CheckMetaDataChanged(data);

    AVSession::AVPlaybackState state;
    int32_t ret = avSessionController_->GetAVPlaybackState(state);
    if (ret != AVSession::AVSESSION_SUCCESS) {
        DftReportAudioError("", ERROR_AUDIO_FWK, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]GetAVPlaybackState error %{public}d.", ret);
        currPlaybackState_.SetState(OHOS::AVSession::AVPlaybackState::PLAYBACK_STATE_STOP);
    } else {
        CheckPlaybackStateChanged(state);
    }
    /* 播放位置 */
    UpdateMediaPlaybackLocation();
}

void McpServerServiceManager::impl::StartStateTimer()
{
    auto timeoutFunc = [pimplSptr = weak_from_this()]() -> void {
        HILOGI("[McpServer] renderState change timeout");
        auto func = [pimplSptr]() {
            auto ptr = pimplSptr.lock();
            NL_CHECK_RETURN(ptr, "[McpServer]ptr is null");
            ptr->renderState_ = McpRenderState::KPausing;
            ptr->stateTimer_ = nullptr;
            ptr->UpdateMediaPlaybackState(AVSession::AVPlaybackState::PLAYBACK_STATE_PAUSE);
        };
        DoInMcpThread([func]() { func(); });
    };
    stateTimer_ = std::make_shared<NearlinkTimer>(timeoutFunc);
    int32_t time = MCP_STATE_CHANGE_WAIT_TIMEOUT;
    stateTimer_->Start(time, false);
}

void McpServerServiceManager::impl::OnRendererStateChange(
    const std::vector<std::shared_ptr<AudioStandard::AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    bool isMusicRendererRunning = false;
    NL_CHECK_RETURN(getAscProfileFunc_, "[McpServer]getAscProfileFunc_ is null");
    ProfileASC *ascService = getAscProfileFunc_();
    NL_CHECK_RETURN(ascService != nullptr, "[McpServer]Get ascService error!");

    for (const auto &audioRendererChangeInfo : audioRendererChangeInfos) {
        NL_CHECK_RETURN(audioRendererChangeInfo, "[McpServer]audioRendererChangeInfo is null");
        HILOGI("rendererState=%{public}d, streamUsage=%{public}d, sessionId=%{public}d",
            audioRendererChangeInfo->rendererState, audioRendererChangeInfo->rendererInfo.streamUsage,
            audioRendererChangeInfo->sessionId);
        if (audioRendererChangeInfo->rendererState == AudioStandard::RendererState::RENDERER_RUNNING) {
            isMusicRendererRunning = !ascService->IsCalling();
        }
    }

    if (!isMusicRendererRunning) {
        realTimeRenderState_ = McpRenderState::KPausing;
        NL_CHECK_RETURN(renderState_ == McpRenderState::KPlaying, "[McpServer] current state is pause, ignore");

        if (stateTimer_ != nullptr) {
            HILOGI("[McpServer] stop timer");
            stateTimer_->Stop();
        }
        StartStateTimer();
    } else {
        if (stateTimer_ != nullptr) {
            HILOGI("[McpServer] stop timer and ignore");
            stateTimer_->Stop();
            stateTimer_ = nullptr;
            return;
        }
        if (ascService->IsCalling() && renderState_ != McpRenderState::KPlaying) {
            renderState_ = McpRenderState::KPlaying;
            UpdateMediaPlaybackState(AVSession::AVPlaybackState::PLAYBACK_STATE_PLAY);
            HILOGI("[McpServer] update play state while calling.");
            return;
        }
        realTimeRenderState_ = McpRenderState::KPlaying;
        renderState_ = McpRenderState::KPlaying;
        UpdateMediaPlaybackState(AVSession::AVPlaybackState::PLAYBACK_STATE_PLAY);
    }
}

void McpServerServiceManager::impl::OnPlaybackStateChange(const AVSession::AVPlaybackState &)
{
    HILOGI("[McpServer]enter");
    ProcessMediaStateChange();
}

void McpServerServiceManager::impl::RegisterAudioRendererEventListener()
{
    HILOGD("[McpServer]enter");
    const int32_t clientPid = IPCSkeleton::GetCallingPid();
    int32_t result = AudioStandard::AudioStreamPolicyClient::GetInstance().RegisterAudioRendererEventListener(
        clientPid, rendererStateCallback_);
    if (result != AVSession::AVSESSION_SUCCESS) {
        DftReportAudioError("", ERROR_AUDIO_FWK, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]McpServerService RegisterAudioRendererEventListener fail");
        return;
    }
    HILOGI("[McpServer]McpServerService RegisterAudioRendererEventListener success, result=%{public}d", result);
}

void McpServerServiceManager::impl::RegisterAvSessionListener()
{
    HILOGI("[McpServer]enter");
    auto res = AVSession::AVSessionManager::GetInstance().RegisterSessionListenerForAllUsers(avSessionObserver_);
    if (res != AVSession::AVSESSION_SUCCESS) {
        DftReportAudioError("", ERROR_AUDIO_FWK, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]McpServerService RegisterSessionListener fail");
        return;
    }
    HILOGI("[McpServer]McpServerService RegisterSessionListener success, result=%{public}d", res);
}

void McpServerServiceManager::impl::AttemptCreateAvSessionController()
{
    HILOGD("[McpServer]enter");
    std::vector<AVSession::AVSessionDescriptor> avSessionDescriptor{};
    auto res = AVSession::AVSessionManager::GetInstance().GetAllSessionDescriptors(avSessionDescriptor);
    if (res != AVSession::AVSESSION_SUCCESS) {
        DftReportAudioError("", ERROR_AUDIO_FWK, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]McpServerService GetAllSessionDescriptors fail");
        return;
    }

    // if session created, find top session to create controller
    for (const auto& descriptor : avSessionDescriptor) {
        HILOGI("[McpServer]descriptor list sessionId_:%{public}s, sessionType_:%{public}d, sessionTag_:%{public}s,"
               "isTopSession_:%{public}d.", descriptor.sessionId_.c_str(), descriptor.sessionType_,
               descriptor.sessionTag_.c_str(), descriptor.isTopSession_);
        if (descriptor.isTopSession_) {
            HILOGI("[McpServer]find top session, try to createAvSessionController.");
            CreateAvSessionController(descriptor);
            break;
        }
    }
}

void McpServerServiceManager::impl::CreateAvSessionController(const AVSession::AVSessionDescriptor &descriptor)
{
    auto sessionId = descriptor.sessionId_;
    NL_CHECK_RETURN(!sessionId.empty(), "[McpServer]AvSession sessionId is empty");
    auto ret = AVSession::AVSessionManager::GetInstance().CreateController(sessionId, avSessionController_);
    if ((ret != AVSession::AVSESSION_SUCCESS && ret != AVSession::ERR_CONTROLLER_IS_EXIST) ||
        avSessionController_ == nullptr) {
        DftReportAudioError("", ERROR_AUDIO_FWK, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]AvSession CreateController failed, ret(%{public}d)", ret);
        return;
    }
    /* 更新内部AVSessionDescriptor */
    currDescriptor_ = descriptor;
    ret = avSessionController_->RegisterCallback(avControllerObserver_);
    if (ret != AVSession::AVSESSION_SUCCESS) {
        DftReportAudioError("", ERROR_AUDIO_FWK, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]AvSession RegisterCallback failed, ret(%{public}d)", ret);
        return;
    }
    SetAVSessionFilter();
    ProcessMediaStateChange();
}

void McpServerServiceManager::impl::SetAVSessionFilter()
{
    NL_CHECK_RETURN(avSessionController_, "[McpServer]current avSessionController is null");

    AVSession::AVPlaybackState::PlaybackStateMaskType filter;
    filter.set(AVSession::AVPlaybackState::PLAYBACK_KEY_STATE);
    filter.set(AVSession::AVPlaybackState::PLAYBACK_KEY_SPEED);
    filter.set(AVSession::AVPlaybackState::PLAYBACK_KEY_POSITION);
    filter.set(AVSession::AVPlaybackState::PLAYBACK_KEY_LOOP_MODE);
    auto ret = avSessionController_->SetPlaybackFilter(filter);
    if (ret != AVSession::AVSESSION_SUCCESS) {
        DftReportAudioError("", ERROR_AUDIO_FWK, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]AvSession SetPlaybackFilter failed, ret(%{public}d)", ret);
    }
    AVSession::AVMetaData::MetaMaskType metaFilter;
    metaFilter.set(AVSession::AVMetaData::META_KEY_ASSET_ID);
    metaFilter.set(AVSession::AVMetaData::META_KEY_ARTIST);
    metaFilter.set(AVSession::AVMetaData::META_KEY_ALBUM);
    metaFilter.set(AVSession::AVMetaData::META_KEY_TITLE);
    metaFilter.set(AVSession::AVMetaData::META_KEY_SINGLE_LYRIC_TEXT);
    ret = avSessionController_->SetMetaFilter(metaFilter);
    if (ret != AVSession::AVSESSION_SUCCESS) {
        DftReportAudioError("", ERROR_AUDIO_FWK, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]AvSession SetMetaFilter failed, ret(%{public}d)", ret);
    }
}

void McpServerServiceManager::impl::SendKeyEventToAvSession(const RawAddress &device, uint8_t key, uint16_t requestId,
    int32_t instanceId)
{
    // 下发控制指令的设备和该设备是否为激活设备无关，即使非激活设备有可以进行控制，因此不需要是否激活设备的校验
    HILOGI("[McpServer][SendKeyEventToAvSession]key=%{public}d, requestId=%{public}d, instanceId=%{public}d",
           key, requestId, instanceId);
    auto iter = mcpToAKeyEventCodeMap_.find(key);
    if (iter == mcpToAKeyEventCodeMap_.end()) {
        DftReportAudioError(device.GetAddress(), ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE5, std::to_string(key));
        HILOGW("[McpServer]not find key in map, don't deal key %{public}d.", key);
        return;
    }
    auto keyEvent = OHOS::MMI::KeyEvent::Create();
    NL_CHECK_RETURN(keyEvent != nullptr, "[McpServer]KeyEvent::Create error.");
    keyEvent->SetKeyCode(iter->second);
    keyEvent->SetKeyAction(MMI::KeyEvent::KEY_ACTION_DOWN);

    auto keyItem = MMI::KeyEvent::KeyItem();
    keyItem.SetKeyCode(iter->second);
    keyItem.SetDownTime(1);
    keyItem.SetPressed(true);
    keyEvent->AddKeyItem(keyItem);

    NL_CHECK_RETURN(getAscProfileFunc_, "[McpServer]getAscProfileFunc_ is null");
    ProfileASC *ascService = getAscProfileFunc_();
    NL_CHECK_RETURN(ascService != nullptr, "[McpServer]Get ascService error!");
    NearlinkRawAddress activeDevice = ascService->GetActiveSinkDevice();
    int32_t result = 0;
    // 判断激活设备是否为空
    if (IsValidAddress(activeDevice.GetAddress()) && activeDevice.GetAddress() != INVALID_MAC_ADDRESS) {
        AAFwk::Want addressParams;
        addressParams.SetParam("deviceId", activeDevice.GetAddress());
        result = AVSession::AVSessionManager::GetInstance().SendSystemAVKeyEvent(*keyEvent, addressParams);
    } else {
        result = AVSession::AVSessionManager::GetInstance().SendSystemAVKeyEvent(*keyEvent);
    }
    HILOGI("[McpServer]send av key event result = %{public}d.", result);
    WriteAudioSinkDeviceUe(key, device.GetAddress(), result);
    if (result != AVSession::AVSESSION_SUCCESS) {
        HILOGE("[McpServer]execute SendSystemAVKeyEvent error result=%{public}d", result);
        // 通知stack处理结果
        UpdateMediaControlResult(device, requestId, instanceId, NLSTK_MCP_CONTROL_FAILED);
        return;
    }
    // 通知stack处理结果
    UpdateMediaControlResult(device, requestId, instanceId, NLSTK_MCP_CONTROL_SUCCESS);
}

bool McpServerServiceManager::impl::CheckMetaDataChanged(const AVSession::AVMetaData &data)
{
    HILOGD("[McpServer]Enter");
    std::string newTitle = data.GetTitle();
    std::string newArtist = data.GetArtist();
    bool isAssetIdChange = data.GetMetaMask().test(AVSession::AVMetaData::META_KEY_ASSET_ID) &&
        currMateData_.GetAssetId() != data.GetAssetId();
    bool isArtistChange = data.GetMetaMask().test(AVSession::AVMetaData::META_KEY_ARTIST) &&
        currMateData_.GetArtist() != newArtist;
    bool isAlbumChange = data.GetMetaMask().test(AVSession::AVMetaData::META_KEY_ALBUM) &&
        currMateData_.GetAlbum() != data.GetAlbum();
    bool isTitleChange = data.GetMetaMask().test(AVSession::AVMetaData::META_KEY_TITLE) &&
        currMateData_.GetTitle() != newTitle;
    bool isNeedSendChange = isAssetIdChange || isArtistChange || isAlbumChange || isTitleChange;
    if (isNeedSendChange) {
        currMateData_ = data;
        if (!newTitle.empty() && !newArtist.empty()) {
            std::string titleAndArtist = newTitle + " - " + newArtist;
            currMateData_.SetTitle(titleAndArtist);
        }
        NL_CHECK_RETURN_RET(!currMateData_.GetTitle().empty(), false, "[McpServer]new mete data title is empty!");
        UpdateMediaBasicInfo();
        return true;
    }
    HILOGI("[McpServer]media meta data not changed");
    return false;
}

void McpServerServiceManager::impl::UpdateMediaBasicInfo()
{
    NLSTK_McpMediaBaseInfo_S basicInfo{};
    int32_t sessionType = currDescriptor_.sessionType_;
    auto iter = avMediaTypeToStackMap_.find(sessionType);
    NL_CHECK_RETURN(iter != avMediaTypeToStackMap_.end(),
        "[McpServer]AvSession invalid sessionType=%{public}d.", sessionType);
    basicInfo.mediaType = iter->second;

    int64_t duration = currMateData_.GetDuration();
    NL_CHECK_RETURN(duration > 0 && duration <= UINT32_MAX,
        "[McpServer][UpdateMediaBasicInfo]: currMateData duration(%{public}lld) error", duration);
    basicInfo.duration = static_cast<uint32_t>(duration);

    size_t length = currMateData_.GetTitle().length() + 1; // 预留结尾'\0'终止符
    basicInfo.mediaNameLen = length;
    basicInfo.mediaName = new (std::nothrow) uint8_t[length];
    NL_CHECK_RETURN(basicInfo.mediaName != nullptr, "[McpServer][UpdateMediaBasicInfo]: alloc mediaName error");
    if (strcpy_s(reinterpret_cast<char *>(basicInfo.mediaName), length, currMateData_.GetTitle().c_str()) != EOK) {
        delete[] basicInfo.mediaName;
        HILOGE("[McpServer][UpdateMediaBasicInfo]: memcpy mediaName error");
        return;
    }
    HILOGI("[McpServer][UpdateMediaBasicInfo] mcpInstanceId=%{public}d, title=%{public}s, duration=%{public}u",
        mcpInstanceId_, basicInfo.mediaName, basicInfo.duration);
    uint32_t ret = NLSTK_McpUpdateMediaProperty(mcpInstanceId_, NLSTK_MCP_MEDIA_BASIC_INFO, &basicInfo);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        delete[] basicInfo.mediaName;
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]McpUpdateMediaProperty-MediaBasicInfo error ret=%{public}d.", ret);
        return;
    }
    delete[] basicInfo.mediaName;
    HILOGD("[McpServer]McpUpdateMediaProperty-MediaBasicInfo success.");
}

void McpServerServiceManager::impl::ProcessMediaStateChange()
{
    HILOGD("[McpServer]Enter");
    NL_CHECK_RETURN_LOGD(avSessionController_, "[McpServer]current avSessionController is null");
    /* 媒体元信息 */
    AVSession::AVMetaData data;
    int32_t ret = avSessionController_->GetAVMetaData(data);
    if (ret != AVSession::AVSESSION_SUCCESS) {
        DftReportAudioError("", ERROR_AUDIO_FWK, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]GetAVMetaData error %{public}d.", ret);
    } else {
        CheckMetaDataChanged(data);
    }

    /* 播放状态 */
    AVSession::AVPlaybackState state;
    ret = avSessionController_->GetAVPlaybackState(state);
    if (ret != AVSession::AVSESSION_SUCCESS) {
        DftReportAudioError("", ERROR_AUDIO_FWK, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]GetAVPlaybackState error %{public}d.", ret);
        currPlaybackState_.SetState(OHOS::AVSession::AVPlaybackState::PLAYBACK_STATE_STOP);
    } else {
        CheckPlaybackStateChanged(state);
    }
    /* 播放位置 */
    UpdateMediaPlaybackLocation();
}

bool McpServerServiceManager::impl::CheckPlaybackStateChanged(const AVSession::AVPlaybackState &state)
{
    HILOGI("[McpServer][CheckPlaybackStateChanged] state=%{public}d", state.GetState());
    if (state.GetMask().test(AVSession::AVPlaybackState::PLAYBACK_KEY_STATE) &&
        currPlaybackState_.GetState() != state.GetState()) {
        int32_t avState = state.GetState();
        HILOGI("[McpServer]Playback state has changed, prepared to update, new avState=%{public}d", avState);
        // 更新播放状态
        int32_t oldMcpState = ConvertAVPlaybackStateToMcpState(currPlaybackState_.GetState());
        int32_t newMcpState = ConvertAVPlaybackStateToMcpState(state.GetState());
        if (oldMcpState != newMcpState) {
            int32_t reportState = state.GetState();
            if (newMcpState != NLSTK_MCP_STATE_SEEKING && renderState_ != McpRenderState::KUninitalize) {
                reportState = (renderState_ == McpRenderState::KPausing ?
                    AVSession::AVPlaybackState::PLAYBACK_STATE_PAUSE : AVSession::AVPlaybackState::PLAYBACK_STATE_PLAY);
            }
            UpdateMediaPlaybackState(reportState);
        }
        currPlaybackState_ = state;

        /* 播放状态变化，处理暂停记录 */
        NL_CHECK_RETURN_RET(getTwsProfileFunc_, false, "[McpServer]getTwsProfileFunc_ is null");
        ProfileTws *twsService = getTwsProfileFunc_();
        if (avState == AVSession::AVPlaybackState::PLAYBACK_STATE_PLAY && twsService != nullptr) {
            twsService->ProcPauseRecordMap();
        }

        return true;
    }
    HILOGD("[McpServer]Playback state not changed");
    return false;
}

int32_t McpServerServiceManager::impl::ConvertAVPlaybackStateToMcpState(int32_t state)
{
    auto iter = avStateToStackMap_.find(state);
    if (iter == avStateToStackMap_.end()) {
        HILOGI("[McpServer]AvSession invalid avState=%{public}d.", state);
        return NLSTK_MCP_STATE_UNINITIALIZED;
    }
    return iter->second;
}

void McpServerServiceManager::impl::UpdateMediaPlaybackState(int32_t state)
{
    auto iter = avStateToStackMap_.find(state);
    NL_CHECK_RETURN(iter != avStateToStackMap_.end(), "[McpServer]AvSession invalid state=%{public}d.", state);
    NLSTK_McpReadyState_E mcpState = iter->second;
    if (currentReportState_ == mcpState) {
        HILOGI("[McpServer] repeat media state, ignore.");
        return;
    }
    HILOGI("[McpServer][UpdateMediaPlaybackState] mcpInstanceId=%{public}d, avState=%{public}d, mcpState=%{public}d",
           mcpInstanceId_, state, mcpState);
    uint32_t ret = NLSTK_McpUpdateMediaProperty(mcpInstanceId_, NLSTK_MCP_PLAYBACK_STATE, &mcpState);
    currentReportState_ = mcpState;
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]McpUpdateMediaProperty-PlaybackState error ret=%{public}d.", ret);
        return;
    }
    HILOGI("[McpServer]McpUpdateMediaProperty-PlaybackState success.");
}

void McpServerServiceManager::impl::UpdateMediaPlaybackLocation()
{
    HILOGI("[McpServer][UpdateMediaPlaybackLocation] mcpInstanceId=%{public}d", mcpInstanceId_);
    int64_t position = 0;
    if (currPlaybackState_.GetState() == AVSession::AVPlaybackState::PLAYBACK_STATE_PAUSE) {
        position = currPlaybackState_.GetPosition().elapsedTime_;
    } else {
        NL_CHECK_RETURN(avSessionController_, "[McpServer]current avSessionController is null");
        position = avSessionController_->GetRealPlaybackPosition();
    }
    if (position == currPosition_) {
        HILOGI("[McpServer][UpdateMediaPlaybackLocation]: Playback position not changed");
        return;
    }
    if (position <= 0 || position > UINT32_MAX) {
        HILOGE("[McpServer][UpdateMediaPlaybackLocation]: currPlaybackState position(%{public}lld) error", position);
        return;
    }
    uint32_t playbackLocation = static_cast<uint32_t>(position);
    HILOGI("[McpServer][UpdateMediaPlaybackLocation] new position=%{public}u",playbackLocation);
    uint32_t ret = NLSTK_McpUpdateMediaProperty(mcpInstanceId_, NLSTK_MCP_MEDIA_PLAYBACK_POSITION, &playbackLocation);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]McpUpdateMediaProperty-PlaybackPosition error ret=%{public}d.", ret);
        return;
    }
    currPosition_ = position;
    HILOGI("[McpServer]McpUpdateMediaProperty-PlaybackPosition success.");
}

void McpServerServiceManager::SetTwsProfileFunc(GetTwsFunc twsFunc)
{
    pimpl->SetTwsProfileFunc(twsFunc);
}
void McpServerServiceManager::SetCdsmProfileFunc(GetCdsmFunc cdsmFunc)
{
    pimpl->SetCdsmProfileFunc(cdsmFunc);
}

void McpServerServiceManager::SetAscProfileFunc(GetAscFunc ascFunc)
{
    pimpl->SetAscProfileFunc(ascFunc);
}

void McpServerServiceManager::impl::UpdateMediaControlResult(const RawAddress &addr, uint16_t requestId,
    int32_t instanceId, uint8_t errorCode)
{
    if (requestId == 0 && instanceId == 0) {    // 由佩戴检测触发时，不会携带这两个信息，因此不给对端下发控制结果
        return ;
    }
    uint32_t ret = NLSTK_McpPlayControlResult(requestId, instanceId, errorCode);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DftReportAudioError(addr.GetAddress(), ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]McpPlayControlResult error %{public}d.", ret);
        return;
    }
}

void McpServerServiceManager::impl::SetTwsProfileFunc(GetTwsFunc twsFunc)
{
    getTwsProfileFunc_ = twsFunc;
}
void McpServerServiceManager::impl::SetCdsmProfileFunc(GetCdsmFunc cdsmFunc)
{
    getCdsmProfileFunc_ = cdsmFunc;
}

void McpServerServiceManager::impl::SetAscProfileFunc(GetAscFunc ascFunc)
{
    getAscProfileFunc_ = ascFunc;
}

void McpServerServiceManager::PostEvent(const McpMessage &event)
{
    pimpl->ProcessEvent(event);
}

void McpServerServiceManager::impl::WriteAudioSinkDeviceUe(int mcpEvt, std::string addr, int32_t controlResult)
{
    static std::map<int, int> mediaControlSubSceneMap = {
        { MCP_EVT_PLAY, MEDIA_CONTROL_SUB_SCENE_PLAY },
        { MCP_EVT_STOP, MEDIA_CONTROL_SUB_SCENE_STOP },
        { MCP_EVT_PAUSE, MEDIA_CONTROL_SUB_SCENE_PAUSE },
        { MCP_EVT_FAST_FOR, MEDIA_CONTROL_SUB_SCENE_FAST_FOR },
        { MCP_EVT_PRE_MEDIA, MEDIA_CONTROL_SUB_SCENE_PRE_MEDIA },
        { MCP_EVT_NEXT_MEDIA, MEDIA_CONTROL_SUB_SCENE_NEXT_MEDIA },
    };

    auto it = mediaControlSubSceneMap.find(mcpEvt);
    if (it == mediaControlSubSceneMap.end()) {
        return;
    }

    RawAddress reportAddr(addr);
    NL_CHECK_RETURN(getCdsmProfileFunc_, "[McpServer]getCdsmProfileFunc_ is null");
    ProfileCdsm *cdsmService = getCdsmProfileFunc_();
    if (cdsmService != nullptr) {
        cdsmService->CdsmGetReportAddr(RawAddress(addr), reportAddr);
    }
    NlErrCode ret = (controlResult == AVSession::AVSESSION_SUCCESS) ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
    NearlinkDftUe::GetInstance().WriteAudioControlUeAndExcep(reportAddr, MEDIA_CONTROL_SCENE, it->second,
        UPDATE_VOICE_STACK_REASON_INVALID, ret);
}

void McpServerServiceManager::impl::SendKeyEvent(const McpMessage &event)
{
    uint16_t requestId = event.requestId_;
    int32_t instanceId = event.instanceId_;
    std::string addr = event.device_;
    RawAddress device(addr);
    switch (event.whatM) {
        case MCP_EVT_PLAY:
            HILOGI("[McpServer]Execute MCP_EVT_PLAY.");
            SendKeyEventToAvSession(device, MCP_ID_PLAY, requestId, instanceId);
            break;
        case MCP_EVT_STOP:
            HILOGI("[McpServer]Execute MCP_EVT_STOP.");
            SendKeyEventToAvSession(device, MCP_ID_STOP, requestId, instanceId);
            break;
        case MCP_EVT_PAUSE:
            HILOGI("[McpServer]Execute MCP_EVT_PAUSE.");
            SendKeyEventToAvSession(device, MCP_ID_PAUSE, requestId, instanceId);
            break;
        case MCP_EVT_FAST_FOR:
            HILOGI("[McpServer]Execute MCP_EVT_FAST_FOR.");
            SendKeyEventToAvSession(device, MCP_ID_FAST_FOR, requestId, instanceId);
            break;
        case MCP_EVT_PRE_MEDIA:
            HILOGI("[McpServer]Execute MCP_EVT_PRE_MEDIA.");
            SendKeyEventToAvSession(device, MCP_ID_PRE_MEDIA, requestId, instanceId);
            break;
        case MCP_EVT_NEXT_MEDIA:
            HILOGI("[McpServer]Execute MCP_EVT_NEXT_MEDIA.");
            SendKeyEventToAvSession(device, MCP_ID_NEXT_MEDIA, requestId, instanceId);
            break;
        default:
            HILOGW("[McpServer]Wrong Event.");
            break;
    }
}

void McpServerServiceManager::impl::ProcessEvent(const McpMessage &event)
{
    if (event.whatM == MCP_EVT_START_SUCCESS) {
        NL_CHECK_RETURN(event.instanceId_ == mcpInstanceId_, "[McpServer]invalid instanceId.");
        // 将播放状态置为就绪
        currPlaybackState_.SetState(AVSession::AVPlaybackState::PLAYBACK_STATE_INITIAL);
        currentReportState_ = NLSTK_MCP_STATE_UNINITIALIZED;
        UpdateMediaPlaybackState(AVSession::AVPlaybackState::PLAYBACK_STATE_INITIAL);
        HILOGI("[McpServer]start init success.");
        return;
    }
    SendKeyEvent(event);
    if (event.whatM == MCP_EVT_PLAY) {
        NotifyMediaStackChanged(event);
    }
}

bool McpServerServiceManager::impl::IsActiveDevice(std::string &addr)
{
    RawAddress reportAddr(addr);
    NL_CHECK_RETURN_RET(getCdsmProfileFunc_, false, "[McpServer]getCdsmProfileFunc_ is null");
    ProfileCdsm *cdsmService = getCdsmProfileFunc_();
    if (cdsmService != nullptr) {
        cdsmService->CdsmGetReportAddr(RawAddress(addr), reportAddr);
    }
    NL_CHECK_RETURN_RET(getAscProfileFunc_, false, "[McpServer]getAscProfileFunc_ is null");
    ProfileASC *ascService = getAscProfileFunc_();
    NL_CHECK_RETURN_RET(ascService != nullptr, false, "[McpServer]Get ascService error!");
    NearlinkRawAddress device = ascService->GetActiveSinkDevice();
    if (reportAddr.GetAddress() == device.GetAddress()) {
        HILOGI("[McpServer]KeyEvent on the active device.");
        return true;
    }
    HILOGI("[McpServer]KeyEvent on the inactive device.");
    return false;
}

bool McpServerServiceManager::impl::GetAVPlaybackState(AVSession::AVPlaybackState &state)
{
    NL_CHECK_RETURN_RET(avSessionController_, false, "[McpServer]current avSessionController is null");
    int32_t ret = avSessionController_->GetAVPlaybackState(state);
    if (ret != AVSession::AVSESSION_SUCCESS) {
        DftReportAudioError("", ERROR_AUDIO_FWK, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]GetAVPlaybackState error %{public}d.", ret);
        return false;
    }
    return true;
}

void McpServerServiceManager::impl::NotifyMediaStackChanged(const McpMessage &event)
{
    RawAddress reportAddr(event.device_);
    NL_CHECK_RETURN(getCdsmProfileFunc_, "[McpServer]getCdsmProfileFunc_ is null");
    ProfileCdsm *cdsmService = getCdsmProfileFunc_();
    if (cdsmService != nullptr) {
        cdsmService->CdsmGetReportAddr(RawAddress(event.device_), reportAddr);
    }
    // 播放抢占
    NotifyAudioDeviceAction(reportAddr, static_cast<int>(UpdateOutputStackAction::ACTION_USER_OPERATION));
}

void McpServerServiceManager::impl::NotifyAudioDeviceAction(const RawAddress &devAddr, int action)
{
    HILOGI("[McpService]:NotifyAudioDeviceAction, addr:%{public}s, action:%{public}d",
        GetEncryptAddr(devAddr.GetAddress()).c_str(), action);
    NL_CHECK_RETURN(getAscProfileFunc_, "[McpServer]getAscProfileFunc_ is null");
    ProfileASC *ascService = getAscProfileFunc_();
    NL_CHECK_RETURN(ascService != nullptr, "cant find ASC service");
    ascService->SleAudioDeviceActionChanged(NearlinkRawAddress(devAddr), action);
}

McpRenderState McpServerServiceManager::impl::GetMcpRealTimeRenderState()
{
    return realTimeRenderState_;
}

McpServerServiceManager::McpServerServiceManager() : pimpl(std::make_shared<McpServerServiceManager::impl>())
{}

McpServerServiceManager::~McpServerServiceManager()
{}

void McpServerServiceManager::Init()
{
    HILOGI("enter");
    pimpl->Init(pimpl);
}

void McpServerServiceManager::DeInit()
{
    HILOGI("enter");
    pimpl->CleanUp();
}

void McpServerServiceManager::InitMediaListener()
{
    HILOGD("enter");
    pimpl->InitMediaListener();
}

void McpServerServiceManager::SendKeyEventToAvSession(const RawAddress &device, uint8_t key)
{
    HILOGI("enter");
    pimpl->SendKeyEventToAvSession(device, key);
}

bool McpServerServiceManager::IsAVPlaybackStatePlay()
{
    std::promise<void> promise;
    AVSession::AVPlaybackState state;
    state.SetState(AVSession::AVPlaybackState::PLAYBACK_STATE_INITIAL);
    DoInMcpThread([this, &promise, &state] ()-> void {
        pimpl->GetAVPlaybackState(state);
        promise.set_value();
    });
    promise.get_future().get();
    HILOGI("AVPlaybackState is %{public}d", static_cast<int32_t>(state.GetState()));
    return state.GetState() == AVSession::AVPlaybackState::PLAYBACK_STATE_PLAY;
}

void McpServerServiceManager::RendererStreamStateChange(int state)
{
    HILOGI("[McpServer] state=%{public}d ", state);
    int32_t avState = AVSession::AVPlaybackState::PLAYBACK_STATE_INITIAL;
    switch (state) {
        case MCP_PLAYBACK_STATE_PLAY:
            avState = AVSession::AVPlaybackState::PLAYBACK_STATE_PLAY;
            break;
        case MCP_PLAYBACK_STATE_PAUSE:
            avState = AVSession::AVPlaybackState::PLAYBACK_STATE_PAUSE;
            break;
        default:
            return;
    }
    DoInMcpThread([this, avState]() {
        pimpl->UpdateMediaPlaybackState(avState);
    });
    return;
}

extern "C" {
McpServerServiceManager *CreateMcpMediaInterface(void)
{
    HILOGI("enter");
    auto mcpManager = new McpServerServiceManager();
    mcpManager->Init();
    return mcpManager;
}

void DestroyMcpMediaInterface(McpServerServiceManager *mcpManager)
{
    HILOGI("enter");
    NL_CHECK_RETURN(mcpManager != nullptr, "mcpManager is nullptr");
    mcpManager->DeInit();
    delete mcpManager;
    AVSession::AVSessionManager::GetInstance().Close();
}

McpRenderState McpServerServiceManager::GetMcpRealTimeRenderState()
{
    std::promise<void> promise;
    McpRenderState state = McpRenderState::KUninitalize;
    DoInMcpThread([this, &promise, &state] ()-> void {
        state = pimpl->GetMcpRealTimeRenderState();
        promise.set_value();
    });
    promise.get_future().get();
    return state;
}

}
}
}