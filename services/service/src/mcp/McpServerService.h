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
#ifndef MCP_SERVER_SERVICE_H
#define MCP_SERVER_SERVICE_H

#include <atomic>
#include <list>
#include "context.h"
#include "nearlink_types.h"
#include "SleInterfaceProfile.h"
#include "McpMessage.h"
#include "McpServerServiceManagerLoader.h"

namespace OHOS {
namespace Nearlink {
class McpServerService : public SleInterfaceProfile, public utility::Context {
public:
    explicit McpServerService();
    ~McpServerService() override;
    static McpServerService *GetService();
    utility::Context *GetContext() override;

    void Enable() override;
    void Disable() override;

    /* 作为MCPServer时，内部没有连接相关的信息 */
    int Connect(const RawAddress &device) override { return 0; }
    int Disconnect(const RawAddress &device) override { return 0; }
    std::list<RawAddress> GetConnectDevices() override { return {}; }
    int GetConnectState() override { return 0; }

    void InitMediaListener();

    void SendKeyEventByWearDetection(const RawAddress &device, uint8_t key);
    void PostMediaEvent(const McpMessage &event);
    void LoadMediaSo() const;
    std::shared_ptr<McpServerServiceManagerLoader> GetMediaLoader();

private:
    void PostEvent(const McpMessage &event);
    void ProcessEvent(const McpMessage &event);
    void ProcessEnableEvent();
    void ProcessDisableEvent();

    struct impl;
    std::shared_ptr<impl> pimpl = nullptr;

    NEARLINK_DISALLOW_COPY_AND_ASSIGN(McpServerService);
};

}
}

#endif //MCP_SERVER_SERVICE_H
