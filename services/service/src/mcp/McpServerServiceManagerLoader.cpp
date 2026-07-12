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
#include "McpServerServiceManagerLoader.h"
#include "log.h"
#include "SleInterfaceProfileCdsm.h"
#include "SleInterfaceProfileASC.h"
#include "SleInterfaceProfileTws.h"
#include "SleInterfaceProfileManager.h"

namespace OHOS {
namespace Nearlink {

McpServerServiceManagerLoader::McpServerServiceManagerLoader()
    : loader_(DEFAULT_LIB_NAME, DEFAULT_UNLOAD_TIMER_MS, DEFAULT_LIB_CREATE_FUNC_NAME, DEFAULT_LIB_DESTROY_FUNC_NAME)
{}

void McpServerServiceManagerLoader::LoadMediaInterfaceLib()
{
    HILOGI("enter");
    loader_.OpenLib();
    auto mcpServerServiceManager = loader_.GetLibInstance();
    NL_CHECK_RETURN(mcpServerServiceManager, "Get avrcp media interface failed");
    mcpServerServiceManager->SetTwsProfileFunc([]() {
        return static_cast<ProfileTws *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_TWS));
    });
    mcpServerServiceManager->SetCdsmProfileFunc([]() {
        return static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    });
    mcpServerServiceManager->SetAscProfileFunc([]() {
        return static_cast<ProfileASC *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    });
}

bool McpServerServiceManagerLoader::IsLibraryLoaded(void)
{
    return loader_.IsLibraryLoaded();
}

#define CALL_FUNC_IF_AVSESSION_MEDIA_LIB_LOADED(func, ...) \
do { \
    auto mcpServerServiceManager = loader_.GetLibInstance(); \
    NL_CHECK_RETURN(mcpServerServiceManager, "failed"); \
    mcpServerServiceManager->func(__VA_ARGS__); \
} while (0)

void McpServerServiceManagerLoader::Init()
{
    CALL_FUNC_IF_AVSESSION_MEDIA_LIB_LOADED(Init);
}

void McpServerServiceManagerLoader::DeInit()
{
    CALL_FUNC_IF_AVSESSION_MEDIA_LIB_LOADED(DeInit);
}

void McpServerServiceManagerLoader::InitMediaListener()
{
    CALL_FUNC_IF_AVSESSION_MEDIA_LIB_LOADED(InitMediaListener);
}

void McpServerServiceManagerLoader::SendKeyEventToAvSession(const RawAddress &device, uint8_t key)
{
    CALL_FUNC_IF_AVSESSION_MEDIA_LIB_LOADED(SendKeyEventToAvSession, device, key);
}

bool McpServerServiceManagerLoader::IsAVPlaybackStatePlay()
{
    auto mcpServerServiceManager = loader_.GetLibInstance();
    NL_CHECK_RETURN_RET(mcpServerServiceManager, false, "failed");
    return mcpServerServiceManager->IsAVPlaybackStatePlay();
}

void McpServerServiceManagerLoader::PostEvent(const McpMessage &event)
{
    CALL_FUNC_IF_AVSESSION_MEDIA_LIB_LOADED(PostEvent, event);
}

void McpServerServiceManagerLoader::RendererStreamStateChange(int state)
{
    CALL_FUNC_IF_AVSESSION_MEDIA_LIB_LOADED(RendererStreamStateChange, state);
}

McpRenderState McpServerServiceManagerLoader::GetMcpRealTimeRenderState()
{
    auto mcpServerServiceManager = loader_.GetLibInstance();
    NL_CHECK_RETURN_RET(mcpServerServiceManager, McpRenderState::KUninitalize, "failed");
    return mcpServerServiceManager->GetMcpRealTimeRenderState();
}

}  // namespace Nearlink
}  // namespace OHOS
