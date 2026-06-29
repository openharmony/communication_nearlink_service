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
#include "nearlink_def_types.h"
#include "nearlink_utils.h"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr int addr5Pos = 15;
constexpr int addr4Pos = 12;
constexpr int addr0Pos = 0;
constexpr int byteLen = 2;
}

std::string GetEncryptAddr(const std::string &addr)
{
    if (addr.empty() || addr.length() != ADDRESS_LENGTH) {
        LOG_INFO("addr is invalid.");
        return std::string("");
    }
    std::string addrMask = ":**:**:**:";
    std::ostringstream out;
    // 将小端序的addr字符串改为05:04:**:**:**:00大端序LOG字符串输出，addr保持小端序不变
    out << addr.substr(addr5Pos, byteLen) << ":" << addr.substr(addr4Pos, byteLen) << addrMask
        << addr.substr(addr0Pos, byteLen);
    return out.str();
}

std::string GetStateString(int state)
{
    switch (state) {
        case SleStateID::STATE_TURNING_ON:
            return "STATE_TURNING_ON(0)";
        case SleStateID::STATE_TURN_ON:
            return "STATE_TURN_ON(1)";
        case SleStateID::STATE_TURNING_OFF:
            return "STATE_TURNING_OFF(2)";
        case SleStateID::STATE_TURN_OFF:
            return "STATE_TURN_OFF(3)";
        case SleStateID::STATE_TURNING_HALF_TO_OFF:
            return "STATE_TURNING_HALF_TO_OFF(4)";
        case SleStateID::STATE_TURNING_OFF_TO_HALF:
            return "STATE_TURNING_OFF_TO_HALF(5)";
        case SleStateID::STATE_TURNING_HALF_TO_ON:
            return "STATE_TURNING_HALF_TO_ON(6)";
        case SleStateID::STATE_TURNING_ON_TO_HALF:
            return "STATE_TURNING_ON_TO_HALF(7)";
        case SleStateID::STATE_TURN_HALF:
            return "STATE_TURN_HALF(8)";
        default:
            return "Unknown";
    }
}

std::string GetTransportString(int transport)
{
    switch (transport) {
        case SleTransport::ADAPTER_SLB:
            return "ADAPTER_SLB(0)";
        case SleTransport::ADAPTER_SLE:
            return "ADAPTER_SLE(1)";
        default:
            return "Unknown";
    }
}

std::string GetReasonString(int stateChangeReason)
{
    switch (stateChangeReason) {
        case static_cast<int>(SleEventType::INTERFACE_TRIGGERED):
            return "INTERFACE_TRIGGERED(0)";
        case static_cast<int>(SleEventType::AIRPLANE_TRIGGERED):
            return "AIRPLANE_TRIGGERED(1)";
        case static_cast<int>(SleEventType::COLLABORATION_TRIGGERED):
            return "COLLABORATION_TRIGGERED(2)";
        case static_cast<int>(SleEventType::RESTORE_TRIGGERED):
            return "RESTORE_TRIGGERED(3)";
        case static_cast<int>(SleEventType::CHIP_RESET_TRIGGERED):
            return "CHIP_RESET_TRIGGERED(4)";
        case static_cast<int>(SleEventType::SELF_HEALING_RETRY):
            return "SELF_HEALING_RETRY(5)";
        case static_cast<int>(SleEventType::SVC_TRIGGERED):
            return "SVC_TRIGGERED(6)";
        case static_cast<int>(SleEventType::SYS_STOP_TRIGGERED):
            return "SYS_STOP_TRIGGERED(7)";
        case static_cast<int>(SleEventType::SYS_RESET_TRIGGERED):
            return "SYS_RESET_TRIGGERED(8)";
        case static_cast<int>(SleEventType::SYS_FACTORY_RESET_TRIGGERED):
            return "SYS_FACTORY_RESET_TRIGGERED(9)";
        default:
            return "Unknown";
    }
}

std::string GetConnStateString(int state)
{
    switch (state) {
        case static_cast<int>(SleConnectState::CONNECTING):
            return "CONNECTING(0)";
        case static_cast<int>(SleConnectState::CONNECTED):
            return "CONNECTED(1)";
        case static_cast<int>(SleConnectState::DISCONNECTING):
            return "DISCONNECTING(2)";
        case static_cast<int>(SleConnectState::DISCONNECTED):
            return "DISCONNECTED(3)";
        default:
            return "Unknown";
    }
}

}  // namespace Nearlink
}  // namespace OHOS