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
#ifndef MCP_MESSAGE_H
#define MCP_MESSAGE_H

#include "message.h"

namespace OHOS {
namespace Nearlink {
class McpMessage : public utility::Message {
public:
    explicit McpMessage(int what = 0, int arg1 = 0, void *arg2 = nullptr) : utility::Message(what, arg1, arg2) {}

    McpMessage(const McpMessage &src) : utility::Message(src.whatM, src.arg1M, src.arg2M)
    {
        whatM = src.whatM;
        arg1M = src.arg1M;
        arg2M = src.arg2M;
        device_ = src.device_;
        requestId_ = src.requestId_;
        instanceId_ = src.instanceId_;
    }

    ~McpMessage() = default;

    /* 重载运算符：= */
    McpMessage& operator=(const McpMessage &src)
    {
        if (this != &src) {
            whatM = src.whatM;
            arg1M = src.arg1M;
            arg2M = src.arg2M;
            device_ = src.device_;
            requestId_ = src.requestId_;
            instanceId_ = src.instanceId_;
        }
        return *this;
    }

    std::string device_ {""};
    uint16_t requestId_ = 0;
    int32_t instanceId_ = 0;
};
} // namespace Nearlink
} // namespace OHOS

#endif // MCP_MESSAGE_H