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
#include "McpServerService.h"
#include "log_util.h"
#include "ClassCreator.h"
#include "McpDefines.h"
#include <future>
#include "ThreadUtil.h"
#include "SleInterfaceProfileManager.h"
#include "SleAudioFrameworkAdapter.h"
#include "nlstk_mcp_media_server.h"
#include "nearlink_dft_exception.h"
#include "McpDefines.h"
#include "McpServerServiceManagerLoader.h"

namespace OHOS {
namespace Nearlink {

namespace {
}

struct McpServerService::impl {
public:
    impl();
    ~impl() = default;
    void McpInit();
    void McpDeInit();
    /* 协议栈事件的回调 */
    static void PlayCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
    static void StopCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
    static void PauseCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
    static void FastForwardCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
    static void PreviousMediaCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
    static void NextMediaCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
    static void StartMediaInstCallback(int32_t instanceId, NLSTK_Errcode_E ret);
    static void AuthorizeCallback(uint16_t requestId, int32_t instanceId, NLSTK_McpPropertyType_E property,
                                  NLSTK_ServicePropertyOpType_E operation);

    std::shared_ptr<McpServerServiceManagerLoader> mediaLoader_ {nullptr};
    // service status
    std::atomic_bool isStarted_ = ATOMIC_FLAG_INIT;
    // service status
    std::atomic_bool isShuttingDown_ = ATOMIC_FLAG_INIT;
    uint32_t instanceId_ = 0;
};

McpServerService::impl::impl() : mediaLoader_(std::make_shared<McpServerServiceManagerLoader>())
{}

McpServerService::McpServerService() : utility::Context(PROFILE_NAME_MCP_SERVER, "1.0.0")
{
    pimpl = std::make_shared<impl>();
    HILOGI("[McpServer]%{public}s:%{public}s Create", PROFILE_NAME_MCP_SERVER.c_str(), Name().c_str());
}

McpServerService::~McpServerService()
{
    HILOGI("[McpServer]%{public}s:%{public}s Destroy", PROFILE_NAME_MCP_SERVER.c_str(), Name().c_str());
}

utility::Context *McpServerService::GetContext()
{
    return this;
}

McpServerService *McpServerService::GetService()
{
    return static_cast<McpServerService *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_MCP_SERVER));
}

void McpServerService::Enable()
{
    HILOGD("Enter");
    McpMessage event(MCP_EVT_STARTUP);
    PostEvent(event);
}

void McpServerService::Disable()
{
    HILOGD("Enter");
    McpMessage event(MCP_EVT_SHUTDOWN);
    PostEvent(event);
}

void McpServerService::PostEvent(const McpMessage &event)
{
    DoInMcpThread([this, event] ()-> void {
        this->ProcessEvent(event);
    });
}

void McpServerService::ProcessEvent(const McpMessage &event)
{
    switch (event.whatM) {
        case MCP_EVT_STARTUP:
            ProcessEnableEvent();
            break;
        case MCP_EVT_SHUTDOWN:
            ProcessDisableEvent();
            break;
        default:
            break;
    }
}

void McpServerService::ProcessEnableEvent()
{
    HILOGD("Enter");
    if (pimpl->isStarted_.load()) {
        GetContext()->OnEnable(PROFILE_NAME_MCP_SERVER, true);
        HILOGW("McpServerService has already been started before.");
        return;
    }
    pimpl->McpInit();
    GetContext()->OnEnable(PROFILE_NAME_MCP_SERVER, true);
    pimpl->isStarted_.store(true);
    HILOGI("McpServerService started");
}

void McpServerService::ProcessDisableEvent()
{
    HILOGD("Enter");
    if (!pimpl->isStarted_.load()) {
        GetContext()->OnDisable(PROFILE_NAME_MCP_SERVER, true);
        HILOGW("McpServerService has already been shutdown before.");
        return;
    }
    pimpl->isShuttingDown_.store(true);
    pimpl->McpDeInit();
    SleAudioFrameworkAdapter::GetInstance().UnregisterAudioFrameworkAdapterListener();
    GetContext()->OnDisable(PROFILE_NAME_MCP_SERVER, true);
    pimpl->isStarted_.store(false);
    pimpl->isShuttingDown_.store(false);
    HILOGI("McpServerService shutdown");
}

void McpServerService::InitMediaListener()
{
    HILOGD("Enter");
    DoInMcpThread([this] ()-> void {
        if (pimpl->instanceId_ == MCP_INSTANCE_ID) {
            McpMessage event(MCP_EVT_START_SUCCESS);
            event.instanceId_ = pimpl->instanceId_;
            pimpl->mediaLoader_->PostEvent(event);
        }
        pimpl->mediaLoader_->InitMediaListener();
        SleAudioFrameworkAdapter::GetInstance().RegisterAudioFrameworkAdapterListener();
    });
}

void McpServerService::SendKeyEventByWearDetection(const RawAddress &device, uint8_t key)
{
    HILOGD("Enter");
    DoInMcpThread([this, device, key] ()-> void {
        pimpl->mediaLoader_->SendKeyEventToAvSession(device, key);
    });
}

std::shared_ptr<McpServerServiceManagerLoader> McpServerService::GetMediaLoader()
{
    HILOGD("Enter");
    return pimpl->mediaLoader_;
}

void McpServerService::impl::McpInit()
{
    HILOGI("[McpServer]enter");
    NLSTK_McpMediaInfo_S mcpMediaInfo{};
    (void)memset_s(&mcpMediaInfo, sizeof(mcpMediaInfo), 0x00, sizeof(mcpMediaInfo));
    mcpMediaInfo.type = NLSTK_MCP_COMMON_SERVICE;
    char instanceName[] = "NearlinkAudio";  // 实例名称暂定为 NearlinkAudio
    mcpMediaInfo.instanceName.data = reinterpret_cast<uint8_t*>(instanceName);
    mcpMediaInfo.instanceName.len = strlen(instanceName);
    mcpMediaInfo.mediaBaseInfo.mediaType = NLSTK_MEDIA_UNSPECIFIED;   // 初始化媒体类型为---未指定媒体
    mcpMediaInfo.playbackState = NLSTK_MCP_STATE_UNINITIALIZED;
    mcpMediaInfo.featuresSupported.FeatureType = NLSTK_MCP_FEATURE_TYPE_PLAY_CTL | NLSTK_MCP_FEATURE_TYPE_PLAY_MODE;
    mcpMediaInfo.featuresSupported.playMode = NLSTK_MCP_SINGLE_PLAY | NLSTK_MCP_SINGLE_LOOP | NLSTK_MCP_SINGLE_LIST
                        | NLSTK_MCP_LOOP_LIST | NLSTK_MCP_RANDOM_PLAY;
    mcpMediaInfo.featuresSupported.playCtl = NLSTK_MCP_PLAY | NLSTK_MCP_STOP | NLSTK_MCP_PAUSE | NLSTK_MCP_FAST_FORWARD
                        | NLSTK_MCP_PREVIOUS_MEDIA | NLSTK_MCP_NEXT_MEDIA;
    mcpMediaInfo.mediaInstanceId = MCP_INSTANCE_ID;
    /* 媒体播放控制点 */
    mcpMediaInfo.playbackControlPoint.play = &PlayCallback;
    mcpMediaInfo.playbackControlPoint.pause = &PauseCallback;
    mcpMediaInfo.playbackControlPoint.stop = &StopCallback;
    mcpMediaInfo.playbackControlPoint.fastForward = &FastForwardCallback;
    mcpMediaInfo.playbackControlPoint.previousMedia = &PreviousMediaCallback;
    mcpMediaInfo.playbackControlPoint.nextMedia = &NextMediaCallback;
    /* 创建实例回调函数 */
    mcpMediaInfo.startMediaInst = &StartMediaInstCallback; // 马上返回事件MCP_EVT_START_SUCCESS
    /* 授权回调函数 */
    mcpMediaInfo.authorize = &AuthorizeCallback;
    /* 创建通用媒体播放控制实例---目前只支持通用媒体控制实例 */
    for (int i = 0; i < NLSTK_MCP_MEDIA_MAX_PROPERTY; i++) {
        mcpMediaInfo.propertyRights[i] = NLSTK_MCP_READ_AUTHEN | NLSTK_MCP_READ_ENCRYPT;
    }
    uint32_t ret = NLSTK_McpCreateMediaInstance(&mcpMediaInfo);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]McpCreateMediaInstance error %{public}d.", ret);
        return;
    }
}

void McpServerService::impl::McpDeInit()
{
    uint32_t ret = NLSTK_McpDeleteMediaInstance(instanceId_);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[McpServer]McpDeleteMediaInstance error %{public}d.", ret);
    }
}

void McpServerService::impl::PlayCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId)
{
    HILOGI("enter");
    NL_CHECK_RETURN(addr != nullptr, "[McpServer]PlayCallback addr is null.");
    const RawAddress peerAddr = RawAddress::ConvertToString(addr->addr);
    McpMessage event(MCP_EVT_PLAY);
    event.device_ = peerAddr.GetAddress();
    event.instanceId_ = instanceId;
    event.requestId_ = requestId;
    McpServerService* mcpService = McpServerService::GetService();
    NL_CHECK_RETURN(mcpService, "mcpService is null.");
    mcpService->PostMediaEvent(event);
}

void McpServerService::impl::StopCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId)
{
    HILOGI("enter");
    NL_CHECK_RETURN(addr != nullptr, "[McpServer]StopCallback addr is null.");
    const RawAddress peerAddr = RawAddress::ConvertToString(addr->addr);
    McpMessage event(MCP_EVT_STOP);
    event.device_ = peerAddr.GetAddress();
    event.instanceId_ = instanceId;
    event.requestId_ = requestId;
    McpServerService* mcpService = McpServerService::GetService();
    NL_CHECK_RETURN(mcpService, "mcpService is null.");
    mcpService->PostMediaEvent(event);
}

void McpServerService::impl::PauseCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId)
{
    HILOGI("enter");
    NL_CHECK_RETURN(addr != nullptr, "[McpServer]PauseCallback addr is null.");
    const RawAddress peerAddr = RawAddress::ConvertToString(addr->addr);
    McpMessage event(MCP_EVT_PAUSE);
    event.device_ = peerAddr.GetAddress();
    event.instanceId_ = instanceId;
    event.requestId_ = requestId;
    McpServerService* mcpService = McpServerService::GetService();
    NL_CHECK_RETURN(mcpService, "mcpService is null.");
    mcpService->PostMediaEvent(event);
}

void McpServerService::impl::FastForwardCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId)
{
    HILOGI("enter");
    NL_CHECK_RETURN(addr != nullptr, "[McpServer]FastForwardCallback addr is null.");
    const RawAddress peerAddr = RawAddress::ConvertToString(addr->addr);
    McpMessage event(MCP_EVT_FAST_FOR);
    event.device_ = peerAddr.GetAddress();
    event.instanceId_ = instanceId;
    event.requestId_ = requestId;
    McpServerService* mcpService = McpServerService::GetService();
    NL_CHECK_RETURN(mcpService, "mcpService is null.");
    mcpService->PostMediaEvent(event);
}

void McpServerService::impl::PreviousMediaCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId)
{
    HILOGI("enter");
    NL_CHECK_RETURN(addr != nullptr, "[McpServer]PreviousMediaCallback addr is null.");
    const RawAddress peerAddr = RawAddress::ConvertToString(addr->addr);
    McpMessage event(MCP_EVT_PRE_MEDIA);
    event.device_ = peerAddr.GetAddress();
    event.instanceId_ = instanceId;
    event.requestId_ = requestId;

    McpServerService* mcpService = McpServerService::GetService();
    NL_CHECK_RETURN(mcpService, "mcpService is null.");
    mcpService->PostMediaEvent(event);
}

void McpServerService::impl::NextMediaCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId)
{
    HILOGI("enter");
    NL_CHECK_RETURN(addr != nullptr, "[McpServer]NextMediaCallback addr is null.");
    const RawAddress peerAddr = RawAddress::ConvertToString(addr->addr);
    McpMessage event(MCP_EVT_NEXT_MEDIA);
    event.device_ = peerAddr.GetAddress();
    event.instanceId_ = instanceId;
    event.requestId_ = requestId;

    McpServerService* mcpService = McpServerService::GetService();
    NL_CHECK_RETURN(mcpService, "mcpService is null.");
    mcpService->PostMediaEvent(event);
}

void McpServerService::impl::StartMediaInstCallback(int32_t instanceId, NLSTK_Errcode_E ret)
{
    NL_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, "[McpServer]StartMediaInstCallback error %{public}d.", ret);
    HILOGI("[McpServer]StartMediaInstCallback success, mcpInstanceId=%{public}d.", instanceId);
    DoInMcpThread([instanceId] ()-> void {
        McpServerService *mcpService = McpServerService::GetService();
        NL_CHECK_RETURN(mcpService, "mcpService is null.");
        mcpService->pimpl->instanceId_ = instanceId;
    });
}

void McpServerService::impl::AuthorizeCallback(uint16_t requestId, int32_t instanceId,
                                                      NLSTK_McpPropertyType_E property,
                                                      NLSTK_ServicePropertyOpType_E operation)
{
    HILOGI("enter");
}

void McpServerService::PostMediaEvent(const McpMessage &event)
{
    DoInMcpThread([mediaLoader = pimpl->mediaLoader_, event] ()-> void {
        mediaLoader->PostEvent(event);
    });
}

void McpServerService::LoadMediaSo(void) const
{
    pimpl->mediaLoader_->LoadMediaInterfaceLib();
}

REGISTER_CLASS_CREATOR(McpServerService);

}
}