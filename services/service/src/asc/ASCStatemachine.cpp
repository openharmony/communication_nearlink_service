/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "ASCStatemachine.h"
#include "ASCService.h"
#include "log_util.h"
#include "nearlink_dft_exception.h"

namespace OHOS {
namespace Nearlink {
ASCStateMachine::ASCStateMachine(const std::string &address)
    : address_(address)
{}

void ASCStateMachine::Init()
{
    std::unique_ptr<utility::StateMachine::State> connectingState =
        std::make_unique<ASCConnectingState>(connecting, *this);
    std::unique_ptr<utility::StateMachine::State> connectedState =
        std::make_unique<ASCConnectedState>(connected, *this);

    Move(connectingState);
    Move(connectedState);

    InitState(ASCconnected);
}

std::string ASCStateMachine::GetDeviceAddress()
{
    return address_;
}

std::string ASCStateMachine::GetEventName(int what)
{
    switch (what) {
        case ASC_SERVICE_STARTUP_EVT:
            return "ASC_SERVICE_STARTUP_EVT";
        case ASC_SERVICE_SHUTDOWN_EVT:
            return "ASC_SERVICE_SHUTDOWN_EVT";
        case ASC_CONNECT_START_EVT:
            return "ASC_CONNECT_START_EVT";
        case ASC_DISCONNECT_START_EVT:
            return "ASC_DISCONNECT_START_EVT";
        case ASC_START_PLAYING_EVT:
            return "ASC_START_PLAYING_EVT";
        case ASC_STOP_PLAYING_EVT:
            return "ASC_STOP_PLAYING_EVT";
        case ASC_STACK_EVENT_CBK_EVT:
            return "ASC_STACK_EVENT_CBK_EVT";
        case ASC_STACK_PROP_CBK_EVT:
            return "ASC_STACK_PROP_CBK_EVT";
        case ASC_SET_ACTIVE_SINK_DEVICE_EVT:
            return "ASC_SET_ACTIVE_SINK_DEVICE_EVT";
        default:
            return "Unknown";
    }
}

void ASCStateMachine::NotifyStateTransitions()
{
    ASCService *ASCService = ASCService::GetService();
    int toState = GetDeviceStateInt();
    if (ASCService != nullptr) {
        RawAddress device(address_);
        if ((preState_ != toState) && (preState_ <= NL_SLE_ASC_RECONFIGURING) &&
            (toState <= NL_SLE_ASC_RECONFIGURING)) {
            ASCService->NotifyStateChanged(device, toState, preState_);
        }
    }

    preState_ = toState;
}

void ASCConnectedState::Entry()
{
    stateMachine_.NotifyStateTransitions();
}

void ASCConnectedState::Exit()
{
}

bool ASCConnectedState::ASCpatch(const utility::Message &msg)
{
    return ASCpatchASC(msg);
}

bool ASCConnectedState::ASCpatchASC(const utility::Message &msg)
{
    ASCMessage &event = static_cast<ASCMessage &>(msg);
    HILOGI("[ASC Machine]:[ASCconnected][%{public}s]",
        ASCStateMachine::GetEventName(event.whatM).c_str());
    switch (event.whatM) {
        case ASC_CONNECT_START_EVT:
            if (stateMachine_.ProcessASCOpenDeviceReq(event) == 0) {
                Transition(ASCStateMachine::connecting);
            }
            break;
        default:
            break;
    }
    return true;
}

void ASCConnectingState::Entry()
{
    stateMachine_.NotifyStateTransitions();
}
void ASCConnectingState::Exit()
{
}

bool ASCConnectingState::ASCpatch(const utility::Message &msg)
{
    return ASCpatchASC(msg);
}

bool ASCConnectingState::ASCpatchASC(const utility::Message &msg)
{
    ASCMessage &event = static_cast<ASCMessage &>(msg);
    HILOGD("[ASC Machine]:[Connecting][%{public}s]",
        ASCStateMachine::GetEventName(event.whatM).c_str());

    return true;
}

void ASCDisconnectedState::Entry()
{
    stateMachine_.NotifyStateTransitions();
}

void ASCDisconnectedState::Exit()
{
}

bool ASCDisconnectedState::ASCpatch(const utility::Message &msg)
{
    return ASCpatchASC(msg);
}

bool ASCDisconnectedState::ASCpatchASC(const utility::Message &msg)
{
    ASCMessage &event = static_cast<ASCMessage &>(msg);
    HILOGD("[ASC Machine]:[DisConnected][%{public}s]",
        ASCStateMachine::GetEventName(event.whatM).c_str());

    return true;
}

}
}
