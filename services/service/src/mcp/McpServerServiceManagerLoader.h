/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef MCP_SERVER_SERVICE_MANAGER_LOADER
#define MCP_SERVER_SERVICE_MANAGER_LOADER

#include "DynamicLibraryLoader.h"
#include "ThreadUtil.h"
#include "McpServerServiceManager.h"

namespace OHOS {
namespace Nearlink {

class McpServerServiceManagerLoader {
public:
    const char* const DEFAULT_LIB_NAME = "libnearlink_mcp_manager.z.so";
    const char* const DEFAULT_LIB_CREATE_FUNC_NAME = "CreateMcpMediaInterface";
    const char* const DEFAULT_LIB_DESTROY_FUNC_NAME = "DestroyMcpMediaInterface";
    // Unload the dynamic library after 5min (300000ms) Calling the UnloadMediaInterfaceLib interface
    const uint32_t DEFAULT_UNLOAD_TIMER_MS = 300000;  // 5min

    McpServerServiceManagerLoader();
    ~McpServerServiceManagerLoader() = default;

    void Init();
    void DeInit();
    void InitMediaListener();
    void SendKeyEventToAvSession(const RawAddress &device, uint8_t key);
    bool IsAVPlaybackStatePlay();
    void PostEvent(const McpMessage &event);
    void RendererStreamStateChange(int state);
    McpRenderState GetMcpRealTimeRenderState();

    /**
     * @brief Load the dynamic library, may cost 20ms ~ 50ms。
     *
     * @return void
     */
    void LoadMediaInterfaceLib(void);

    bool IsLibraryLoaded(void);

private:
    CxxDynamicLibraryLoader<McpServerServiceManagerInterface> loader_;
};

}  // namespace Nearlink
}  // namespace OHOS
#endif