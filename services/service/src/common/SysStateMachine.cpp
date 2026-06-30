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
#include "SysStateMachine.h"
#include "SleServiceManager.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
void SysStateMachine::Init(SleServiceManager &am)
{
    std::unique_ptr<utility::StateMachine::State> stopped = std::make_unique<SysStoppedState>(*this, am);
    Move(stopped);
    std::unique_ptr<utility::StateMachine::State> started = std::make_unique<SysStartedState>(*this, am);
    Move(started);
    std::unique_ptr<utility::StateMachine::State> stopping = std::make_unique<SysStoppingState>(*this, am);
    Move(stopping);
    std::unique_ptr<utility::StateMachine::State> resetting = std::make_unique<SysResettingState>(*this, am);
    Move(resetting);
    std::unique_ptr<utility::StateMachine::State> factoryResetting =
        std::make_unique<SysFactoryResettingState>(*this, am);
    Move(factoryResetting);
    InitState(SYS_STATE_STOPPED);
}

void SysStoppingBaseState::StoppingBaseProcess(int stateSle, SleEventType reason)
{
    if (stateSle == SleStateID::STATE_TURN_OFF) {
        LOG_INFO("[SysStoppingBaseState] all off");
        am_.AdapterStop();
    } else if ((stateSle == SleStateID::STATE_TURN_ON) || (stateSle == SleStateID::STATE_TURN_HALF)) {
        SwitchCallerInfo callerInfo = SleInterfaceManager::GetCallerInfo();
        am_.SwitchToOffTask(SleTransport::ADAPTER_SLE, reason, callerInfo);
    } else {
        LOG_INFO("[SysStoppingBaseState] turning state, do nothing");
    }
}

void SysStoppedState::Entry()
{
    LOG_INFO("[SysStoppedState] Entry");
}

bool SysStoppedState::Dispatch(const utility::Message &msg)
{
    LOG_INFO("[SysStoppedState] Dispatch %{public}d", msg.whatM);
    switch (msg.whatM) {
        case SysStateMachine::MSG_SYS_START_CMP:
            Transition(SYS_STATE_STARTED);
            return true;
        default:
            return false;
    }
}

void SysStartedState::Entry()
{
    LOG_INFO("[SysStartedState] Entry");
    am_.OnSysStateChange(SYS_STATE_STARTED);
}

bool SysStartedState::Dispatch(const utility::Message &msg)
{
    LOG_INFO("[SysStartedState] Dispatch %{public}d", msg.whatM);
    switch (msg.whatM) {
        case SysStateMachine::MSG_SYS_RESET_REQ:
            Transition(SYS_STATE_RESETTING);
            break;
        case SysStateMachine::MSG_SYS_FACTORY_RESET_REQ:
            Transition(SYS_STATE_FRESETTING);
            break;
        case SysStateMachine::MSG_SYS_STOP_REQ:
            Transition(SYS_STATE_STOPPING);
            break;
        default:
            return false;
    }
    return true;
}

void SysStoppingState::Entry()
{
    LOG_INFO("[SysStoppingState] Entry");

    int stateSle = am_.GetState(SleTransport::ADAPTER_SLE);

    am_.OnSysStateChange(SYS_STATE_STOPPING);
    StoppingBaseProcess(stateSle, SleEventType::SYS_STOP_TRIGGERED);
}

void SysStoppingState::Exit()
{
    am_.OnSysStateExit(SYS_STATE_STOPPING);
}

bool SysStoppingState::Dispatch(const utility::Message &msg)
{
    LOG_INFO("[SysStoppingState] Dispatch %{public}d", msg.whatM);
    int stateSle = SleStateID::STATE_TURN_OFF;
    switch (msg.whatM) {
        case SysStateMachine::MSG_SYS_STOP_CMP:
            LOG_INFO("[SysResettingState] MSG_SYS_STOP_CMP");
            am_.OnSysStateChange(SYS_STATE_STOPPED);
            Transition(SYS_STATE_STOPPED);
            return true;
        case SysStateMachine::MSG_SYS_ADAPTER_STATE_CHANGE_REQ:
            stateSle = static_cast<unsigned int>(msg.arg1M) & 0x0F;
            StoppingBaseProcess(stateSle, SleEventType::SYS_STOP_TRIGGERED);
            return true;
        default:
            return false;
    }
}

void SysResettingState::Entry()
{
    LOG_INFO("[SysResettingState] Entry");

    int stateSle = am_.GetState(SleTransport::ADAPTER_SLE);

    am_.OnSysStateChange(SYS_STATE_RESETTING);
    StoppingBaseProcess(stateSle, SleEventType::SYS_RESET_TRIGGERED);
}

bool SysResettingState::Dispatch(const utility::Message &msg)
{
    LOG_INFO("[SysResettingState] Dispatch %{public}d %{public}d", msg.whatM, msg.arg1M);
    int stateSle = SleStateID::STATE_TURN_OFF;
    switch (msg.whatM) {
        case SysStateMachine::MSG_SYS_ADAPTER_STATE_CHANGE_REQ:
            stateSle = static_cast<unsigned int>(msg.arg1M) & 0x0F;
            StoppingBaseProcess(stateSle, SleEventType::SYS_RESET_TRIGGERED);
            break;
        case SysStateMachine::MSG_SYS_STOP_CMP:
            am_.StartTask();
            Transition(SYS_STATE_STARTED);
            break;
        case SysStateMachine::MSG_SYS_STOP_REQ:
            Transition(SYS_STATE_STOPPING);
            break;
        default:
            return false;
    }
    return true;
}

// SysResettingState
void SysFactoryResettingState::Entry()
{
    LOG_INFO("[SysFactoryResettingState] Entry");

    int stateSle = am_.GetState(SleTransport::ADAPTER_SLE);

    am_.OnSysStateChange(SYS_STATE_FRESETTING);
    StoppingBaseProcess(stateSle, SleEventType::SYS_FACTORY_RESET_TRIGGERED);
}

void SysFactoryResettingState::Exit()
{
    am_.OnSysStateExit(SYS_STATE_FRESETTING);
}

bool SysFactoryResettingState::Dispatch(const utility::Message &msg)
{
    LOG_INFO("[SysFactoryResettingState] Dispatch %{public}d", msg.whatM);
    int stateSle = SleStateID::STATE_TURN_OFF;
    switch (msg.whatM) {
        case SysStateMachine::MSG_SYS_ADAPTER_STATE_CHANGE_REQ:
            stateSle = static_cast<unsigned int>(msg.arg1M) & 0x0F;
            if (stateSle == SleStateID::STATE_TURN_OFF) {
                am_.ClearAllStorage();
            }
            StoppingBaseProcess(stateSle, SleEventType::SYS_FACTORY_RESET_TRIGGERED);
            break;
        case SysStateMachine::MSG_SYS_STOP_CMP:
            Transition(SYS_STATE_STOPPED);
            am_.OnSysStateChange(SYS_STATE_STOPPED);
            break;
        case SysStateMachine::MSG_SYS_STOP_REQ:
            Transition(SYS_STATE_STOPPING);
            break;
        case SysStateMachine::MSG_SYS_CLEAR_ALL_STORAGE_CMP:
            break;
        default:
            return false;
    }
    return true;
}
}  // namespace Sle
}  // namespace OHOS