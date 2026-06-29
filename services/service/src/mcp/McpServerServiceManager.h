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
#ifndef MCP_SERVER_SERVICE_MANAGER_H
#define MCP_SERVER_SERVICE_MANAGER_H

#include <cstdint>
#include "nearlink_types.h"
#include "McpMessage.h"
#include "raw_address.h"
#include "SleServiceFfrtLog.h"
#include "SleInterfaceProfileTws.h"
#include "SleInterfaceProfileCdsm.h"
#include "SleInterfaceProfileASC.h"
#include <functional>
#include "McpDefines.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief An interface class for managing MCP server services.
 *
 * @since 6
 */
class McpServerServiceManagerInterface {
public:
    /**
     * @brief A function type for getting a TWS profile.
     *
     * @since 6
     */
    using GetTwsFunc = std::function<ProfileTws *()>;

    /**
     * @brief A function type for getting a CDSM profile.
     *
     * @since 6
     */
    using GetCdsmFunc = std::function<ProfileCdsm *()>;

    /**
     * @brief A function type for getting an ASC profile.
     *
     * @since 6
     */
    using GetAscFunc = std::function<ProfileASC *()>;

    /**
     * @brief A virtual destructor used to delete the <b>McpServerServiceManagerInterface</b> instance.
     *
     * @since 6
     */
    virtual ~McpServerServiceManagerInterface() = default;

    /**
     * @brief Sets the function to get the TWS profile.
     *
     * @param func The function to get the TWS profile.
     *
     * @since 6
     */
    virtual void SetTwsProfileFunc(GetTwsFunc func) = 0;

    /**
     * @brief Sets the function to get the CDSM profile.
     *
     * @param func The function to get the CDSM profile.
     *
     * @since 6
     */
    virtual void SetCdsmProfileFunc(GetCdsmFunc func) = 0;

    /**
     * @brief Sets the function to get the ASC profile.
     *
     * @param func The function to get the ASC profile.
     *
     * @since 6
     */
    virtual void SetAscProfileFunc(GetAscFunc func) = 0;

    /**
     * @brief Initializes the service management interface.
     *
     * @since 6
     */
    virtual void Init() = 0;

    /**
     * @brief Deinitializes the service management interface.
     *
     * @since 6
     */
    virtual void DeInit() = 0;

    /**
     * @brief Initializes the media listener.
     *
     * @since 6
     */
    virtual void InitMediaListener();

    /**
     * @brief Sends a key event to the AV session.
     *
     * @param device The address of the device.
     * @param key The key event to send.
     *
     * @since 6
     */
    virtual void SendKeyEventToAvSession(const RawAddress &device, uint8_t key) = 0;

    /**
     * @brief change the renderer stream state.
     *
     * @since 6
     */
    virtual void RendererStreamStateChange(int state) = 0;

    /**
     * @brief Checks if the AV playback state is playing.
     *
     * @return True if the AV playback state is playing, false otherwise.
     *
     * @since 6
     */
    virtual bool IsAVPlaybackStatePlay() = 0;

    /**
     * @brief Posts an event.
     *
     * @param event The event to post.
     *
     * @since 6
     */
    virtual void PostEvent(const McpMessage &event) = 0;

    /**
     * @brief get mcp render state.
     *
     * @return mcp render state.
     *
     * @since 6
     */
    virtual McpRenderState GetMcpRealTimeRenderState() = 0;
};

class McpServerServiceManager : public McpServerServiceManagerInterface {
public:
    explicit McpServerServiceManager();
    ~McpServerServiceManager() override;

    void SetTwsProfileFunc(GetTwsFunc func) override;
    void SetCdsmProfileFunc(GetCdsmFunc func) override;
    void SetAscProfileFunc(GetAscFunc func) override;
    void Init() override;
    void DeInit() override;
    void InitMediaListener() override;
    void SendKeyEventToAvSession(const RawAddress &device, uint8_t key) override;
    void RendererStreamStateChange(int state) override;
    bool IsAVPlaybackStatePlay() override;
    void PostEvent(const McpMessage &event) override;
    McpRenderState GetMcpRealTimeRenderState() override;
private:
    struct impl;
    std::shared_ptr<impl> pimpl = nullptr;
};
}
}

#endif // MCP_SERVER_SERVICE_MANAGER_H
