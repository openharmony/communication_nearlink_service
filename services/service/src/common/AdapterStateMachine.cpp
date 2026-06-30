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
#include "AdapterStateMachine.h"

#include "log_util.h"

#include "SleServiceManager.h"
#include "ProfileServiceManager.h"
#include "ThreadUtil.h"
#include "SleServiceFfrtLog.h"
#include "nearlink_dft_exception.h"

namespace OHOS {
namespace Nearlink {
#ifdef PC_STANDARD
const int ENABLE_DISABLE_TIMEOUT_TIME = 70000;
#else
#ifdef WATCH_STANDARD
const int ENABLE_DISABLE_TIMEOUT_TIME = 70000;
#else
const int ENABLE_DISABLE_TIMEOUT_TIME = 10000;
#endif
#endif
const int TURNING_STATE_TIMEOUT_TIME = 1000;

void AdapterStateMachine::Init(std::shared_ptr<SleInterfaceAdapter> adapter)
{
    auto self = shared_from_this();
    std::unique_ptr<utility::StateMachine::State> turningOn = std::make_unique<AdapterTurningOnState>(self, adapter);
    Move(turningOn);
    std::unique_ptr<utility::StateMachine::State> turnOn = std::make_unique<AdapterTurnOnState>(self, adapter);
    Move(turnOn);
    std::unique_ptr<utility::StateMachine::State> turningOff = std::make_unique<AdapterTurningOffState>(self, adapter);
    Move(turningOff);
    std::unique_ptr<utility::StateMachine::State> turnOff = std::make_unique<AdapterTurnOffState>(self, adapter);
    Move(turnOff);
    std::unique_ptr<utility::StateMachine::State> turningHalfToOff =
        std::make_unique<AdapterTurningHalfToOffState>(self, adapter);
    Move(turningHalfToOff);
    std::unique_ptr<utility::StateMachine::State> turningOffToHalf =
        std::make_unique<AdapterTurningOffToHalfState>(self, adapter);
    Move(turningOffToHalf);
    std::unique_ptr<utility::StateMachine::State> turningHalfToOn =
        std::make_unique<AdapterTurningHalfToOnState>(self, adapter);
    Move(turningHalfToOn);
    std::unique_ptr<utility::StateMachine::State> turnHalf = std::make_unique<AdapterTurnHalfState>(self, adapter);
    Move(turnHalf);
    InitState(TURN_OFF_STATE);
}

static bool IsStateStable(const SleStateID& state)
{
    if (state == SleStateID::STATE_TURN_ON ||
        state == SleStateID::STATE_TURN_OFF ||
        state == SleStateID::STATE_TURN_HALF) {
        return true;
    }
    return false;
}

void AdapterStateMachine::SetNextTargetState(const SleStateID& targetState)
{
    if (IsStateStable(targetState)) {
        HILOGI("set next target state as %{public}s", GetStateString(targetState).c_str());
        nextTargetState_ = targetState;
    } else {
        HILOGE("not stable state");
    }
}

SleStateID AdapterStateMachine::GetNextTargetState() const
{
    return nextTargetState_;
}

void AdapterStateMachine::PostTargetMessage() const
{
    HILOGI("enter");
    AdapterStateMessage stateMsg = AdapterStateMessage::MSG_DISABLE_REQ;
    SleStateID targetState = nextTargetState_.load();
    switch (targetState) {
        case SleStateID::STATE_TURN_ON:
            stateMsg = AdapterStateMessage::MSG_ENABLE_REQ;
            break;
        case SleStateID::STATE_TURN_OFF:
            stateMsg = AdapterStateMessage::MSG_DISABLE_REQ;
            break;
        case SleStateID::STATE_TURN_HALF:
            stateMsg = AdapterStateMessage::MSG_HALF_DISABLE_REQ;
            break;
        default:
            HILOGE("invalid target state: %{public}s", GetStateString(targetState).c_str());
    }
    int msgId = static_cast<int>(stateMsg);
    utility::StateMachine::ProcessMessage(utility::Message(msgId));
    return;
}

void AdapterTurnOffState::Entry()
{
    HILOGI("[AdapterTurnOffState] enter");
    SleTransport transport =
        (adapter_->GetContext()->Name() == ADAPTER_NAME_SLB) ? SleTransport::ADAPTER_SLB : SleTransport::ADAPTER_SLE;
    SleServiceManager::GetInstance()->OnAdapterStateChangeTask(transport, SleStateID::STATE_TURN_OFF);
    DftReportSwitchInfo(SWITCH_SUCCESS, DFT_MSG_SWITCH_SUCCESS, SWITCH_CLOSE);

    if (adapterStateMachine_->GetNextTargetState() != SleStateID::STATE_TURN_OFF) {
        adapterStateMachine_->PostTargetMessage();
        return;
    };
}

bool AdapterTurnOffState::Dispatch(const utility::Message &msg)
{
    HILOGI("[AdapterTurnOffState] enter");
    switch (static_cast<AdapterStateMachine::AdapterStateMessage>(msg.whatM)) {
        case AdapterStateMachine::AdapterStateMessage::MSG_ENABLE_REQ:
            Transition(TURNING_ON_STATE);
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_HALF_DISABLE_REQ:
            Transition(TURNING_OFF_TO_HALF_STATE);
            break;
        default:
            return false;
    }
    return true;
}

AdapterTurningOnState::AdapterTurningOnState(std::shared_ptr<AdapterStateMachine> stateMachine,
                                             std::shared_ptr<SleInterfaceAdapter> adapter)
    : AdapterState(TURNING_ON_STATE, stateMachine, adapter)
{
    int adapterMsgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_ENABLE_TIME_OUT);
    adapterTimer_ = std::make_unique<NearlinkTimer>([stateMachine, adapterMsgId]() -> void {
        DoInServiceManagerThread([stateMachine, adapterMsgId]() -> void {
            stateMachine->ProcessMessage(utility::Message(adapterMsgId));
        });
    });
    int profileMsgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_ENABLE_TIME_OUT);
    profileTimer_ = std::make_unique<NearlinkTimer>([stateMachine, profileMsgId]() -> void {
        DoInServiceManagerThread([stateMachine, profileMsgId]() -> void {
            stateMachine->ProcessMessage(utility::Message(profileMsgId));
        });
    });
}

void AdapterTurningOnState::Entry()
{
    HILOGI("[AdapterTurningOnState] enter");
    SleTransport transport =
        (adapter_->GetContext()->Name() == ADAPTER_NAME_SLB) ? SleTransport::ADAPTER_SLB : SleTransport::ADAPTER_SLE;
    SleServiceManager::GetInstance()->OnAdapterStateChangeTask(transport, SleStateID::STATE_TURNING_ON);
    LOG_DEBUG("AdapterStateMachine::Timer enable adapter start transport is %{public}d", transport);
    adapterTimer_->Start(ENABLE_DISABLE_TIMEOUT_TIME, false);
    adapter_->GetContext()->Enable();
}

void AdapterTurningOnState::HandleAdapterEnableCmpMsg(SleTransport transport, const utility::Message &msg)
{
    HILOG_COMM_INFO("[%{public}s:%{public}d] enter", __FUNCTION__, __LINE__);
    LOG_DEBUG("AdapterStateMachine::Timer enable adapter stop transport is %{public}d", transport);
    adapterTimer_->Stop();
    if (msg.arg1M) {
        LOG_DEBUG("AdapterStateMachine::Timer enable profile start transport is %{public}d", transport);
        profileTimer_->Start(ENABLE_DISABLE_TIMEOUT_TIME, false);
        ProfileServiceManager::GetInstance().Enable(transport);
    } else {
        LOG_ERROR("AdapterStateMachine: MSG_ADAPTER_ENABLE_CMP failed");
        adapterStateMachine_->SetNextTargetState(SleStateID::STATE_TURN_OFF);
        Transition(TURNING_OFF_STATE);
    }
}

void AdapterTurningOnState::HandleProfileEnableCmpMsg(SleTransport transport, const utility::Message &msg)
{
    HILOG_COMM_INFO("[%{public}s:%{public}d] enter", __FUNCTION__, __LINE__);
    LOG_DEBUG("AdapterStateMachine::Timer enable profile stop transport is %{public}d", transport);
    profileTimer_->Stop();
    if (msg.arg1M) {
        Transition(TURN_ON_STATE);
    } else {
        LOG_ERROR("AdapterStateMachine: MSG_PROFILE_ENABLE_CMP failed");
        adapterStateMachine_->SetNextTargetState(SleStateID::STATE_TURN_OFF);
        ProfileServiceManager::GetInstance().OnAllEnabled(transport);
        Transition(TURNING_OFF_STATE);
    }
}

bool AdapterTurningOnState::Dispatch(const utility::Message &msg)
{
    HILOGI("[AdapterTurningOnState] enter");
    SleTransport transport =
        (adapter_->GetContext()->Name() == ADAPTER_NAME_SLB) ? SleTransport::ADAPTER_SLB : SleTransport::ADAPTER_SLE;

    switch (static_cast<AdapterStateMachine::AdapterStateMessage>(msg.whatM)) {
        case AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_ENABLE_CMP:
            HandleAdapterEnableCmpMsg(transport, msg);
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_ENABLE_CMP:
            HandleProfileEnableCmpMsg(transport, msg);
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_ENABLE_TIME_OUT:
            LOG_ERROR("AdapterStateMachine::Timer enable adapter time out transport is %{public}d", transport);
            DftReportSwitchInfo(ENABLE_TIME_OUT, DFT_MSG_ENABLE_TIME_OUT, SWITCH_OPEN, SWITCH_OPER_FAIL);
            SleServiceManager::GetInstance()->Reset();
            adapterStateMachine_->SetNextTargetState(SleStateID::STATE_TURN_OFF);
            Transition(TURNING_OFF_STATE);
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_ENABLE_TIME_OUT:
            LOG_ERROR("AdapterStateMachine::Timer enable profile time out transport is %{public}d", transport);
            DftReportSwitchInfo(ENABLE_TIME_OUT, DFT_MSG_ENABLE_TIME_OUT, SWITCH_OPEN, SWITCH_OPER_FAIL);
            SleServiceManager::GetInstance()->Reset();
            ProfileServiceManager::GetInstance().OnAllEnabled(transport);
            adapterStateMachine_->SetNextTargetState(SleStateID::STATE_TURN_OFF);
            Transition(TURNING_OFF_STATE);
            break;
        default:
            return false;
    }
    return true;
}

void AdapterTurnOnState::Entry()
{
    HILOGI("[AdapterTurnOnState] enter");
    SleTransport transport =
        (adapter_->GetContext()->Name() == ADAPTER_NAME_SLB) ? SleTransport::ADAPTER_SLB : SleTransport::ADAPTER_SLE;
    SleServiceManager::GetInstance()->OnAdapterStateChangeTask(transport, SleStateID::STATE_TURN_ON);
    DftReportSwitchInfo(SWITCH_SUCCESS, DFT_MSG_SWITCH_SUCCESS, SWITCH_OPEN);

    if (adapterStateMachine_->GetNextTargetState() != SleStateID::STATE_TURN_ON) {
        adapterStateMachine_->PostTargetMessage();
        return;
    };

    adapter_->GetContext()->PostEnable(); // 星闪开启完成，启动已配对设备的回连操作
}

bool AdapterTurnOnState::Dispatch(const utility::Message &msg)
{
    HILOGI("[AdapterTurnOnState] enter");
    switch (static_cast<AdapterStateMachine::AdapterStateMessage>(msg.whatM)) {
        case AdapterStateMachine::AdapterStateMessage::MSG_DISABLE_REQ:
            Transition(TURNING_OFF_STATE);
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_HALF_DISABLE_REQ:
            Transition(TURNING_OFF_STATE); // 星闪开启到半关流程：ON->OFF->HALF
            break;
        default:
            return false;
    }
    return true;
}

AdapterTurningOffState::AdapterTurningOffState(std::shared_ptr<AdapterStateMachine> stateMachine,
                                               std::shared_ptr<SleInterfaceAdapter> adapter)
    : AdapterState(TURNING_OFF_STATE, stateMachine, adapter)
{
    int adapterMsgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_DISABLE_TIME_OUT);
    adapterTimer_ = std::make_unique<NearlinkTimer>([stateMachine, adapterMsgId]() -> void {
        DoInServiceManagerThread([stateMachine, adapterMsgId]() -> void {
            stateMachine->ProcessMessage(utility::Message(adapterMsgId));
        });
    });
    int profileMsgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_DISABLE_TIME_OUT);
    profileTimer_ = std::make_unique<NearlinkTimer>([stateMachine, profileMsgId]() -> void {
        DoInServiceManagerThread([stateMachine, profileMsgId]() -> void {
            stateMachine->ProcessMessage(utility::Message(profileMsgId));
        });
    });
}

void AdapterTurningOffState::Entry()
{
    HILOGI("[AdapterTurningOffState] enter");
    SleTransport transport =
        (adapter_->GetContext()->Name() == ADAPTER_NAME_SLB) ? SleTransport::ADAPTER_SLB : SleTransport::ADAPTER_SLE;

    SleServiceManager::GetInstance()->OnAdapterStateChangeTask(transport, SleStateID::STATE_TURNING_OFF);
    LOG_DEBUG("AdapterStateMachine::Timer disable profile start transport is %{public}d", transport);
    profileTimer_->Start(ENABLE_DISABLE_TIMEOUT_TIME, false);
    ProfileServiceManager::GetInstance().Disable(transport);
}

bool AdapterTurningOffState::Dispatch(const utility::Message &msg)
{
    HILOGI("[AdapterTurningOffState] enter");
    SleTransport transport =
        (adapter_->GetContext()->Name() == ADAPTER_NAME_SLB) ? SleTransport::ADAPTER_SLB : SleTransport::ADAPTER_SLE;

    switch (static_cast<AdapterStateMachine::AdapterStateMessage>(msg.whatM)) {
        case AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_DISABLE_CMP:
            LOG_DEBUG("AdapterStateMachine::Timer disable profile stop transport is %{public}d", transport);
            profileTimer_->Stop();
            LOG_DEBUG("AdapterStateMachine::Timer disable adapter start transport is %{public}d", transport);
            adapterTimer_->Start(ENABLE_DISABLE_TIMEOUT_TIME, false);
            adapter_->GetContext()->Disable();
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_DISABLE_CMP:
            LOG_DEBUG("AdapterStateMachine::Timer disable adapter stop transport is %{public}d", transport);
            adapterTimer_->Stop();
            Transition(TURN_OFF_STATE);
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_DISABLE_TIME_OUT:
            LOG_ERROR("AdapterStateMachine::Timer disable profile time out transport is %{public}d", transport);
            DftReportSwitchInfo(DISABLE_TIME_OUT, DFT_MSG_DISABLE_TIME_OUT, SWITCH_CLOSE, SWITCH_OPER_FAIL);
            ProfileServiceManager::GetInstance().OnAllDisabled(transport);
            adapterTimer_->Start(ENABLE_DISABLE_TIMEOUT_TIME, false);
            adapter_->GetContext()->Disable();
            SleServiceManager::GetInstance()->Reset();
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_DISABLE_TIME_OUT:
            LOG_ERROR("AdapterStateMachine::Timer disable adapter time out transport is %{public}d", transport);
            DftReportSwitchInfo(DISABLE_TIME_OUT, DFT_MSG_DISABLE_TIME_OUT, SWITCH_CLOSE, SWITCH_OPER_FAIL);
            SleServiceManager::GetInstance()->Reset();
            Transition(TURN_OFF_STATE);
            break;
        default:
            return false;
    }
    return true;
}

AdapterTurningHalfToOffState::AdapterTurningHalfToOffState(std::shared_ptr<AdapterStateMachine> stateMachine,
                                                           std::shared_ptr<SleInterfaceAdapter> adapter)
    : AdapterState(TURNING_HALF_TO_OFF_STATE, stateMachine, adapter)
{
    int adapterMsgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_DISABLE_TIME_OUT);
    adapterTimer_ = std::make_unique<NearlinkTimer>([stateMachine, adapterMsgId]() -> void {
        DoInServiceManagerThread([stateMachine, adapterMsgId]() -> void {
            stateMachine->ProcessMessage(utility::Message(adapterMsgId));
        });
    });
    int profileMsgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_DISABLE_TIME_OUT);
    profileTimer_ = std::make_unique<NearlinkTimer>([stateMachine, profileMsgId]() -> void {
        DoInServiceManagerThread([stateMachine, profileMsgId]() -> void {
            stateMachine->ProcessMessage(utility::Message(profileMsgId));
        });
    });
}

void AdapterTurningHalfToOffState::Entry()
{
    HILOGI("[AdapterTurningHalfToOffState] enter");
    SleTransport transport =
        (adapter_->GetContext()->Name() == ADAPTER_NAME_SLB) ? SleTransport::ADAPTER_SLB : SleTransport::ADAPTER_SLE;

    SleServiceManager::GetInstance()->OnAdapterStateChangeTask(transport, SleStateID::STATE_TURNING_HALF_TO_OFF);
    LOG_DEBUG("AdapterStateMachine::Timer disable profile start transport is %{public}d", transport);
    profileTimer_->Start(ENABLE_DISABLE_TIMEOUT_TIME, false);
    ProfileServiceManager::GetInstance().Disable(transport);
}

bool AdapterTurningHalfToOffState::Dispatch(const utility::Message &msg)
{
    HILOGI("AdapterTurningHalfToOffState enter");
    SleTransport transport =
        (adapter_->GetContext()->Name() == ADAPTER_NAME_SLB) ? SleTransport::ADAPTER_SLB : SleTransport::ADAPTER_SLE;

    switch (static_cast<AdapterStateMachine::AdapterStateMessage>(msg.whatM)) {
        case AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_DISABLE_CMP:
            LOG_DEBUG("AdapterStateMachine::Timer disable profile stop transport is %{public}d", transport);
            profileTimer_->Stop();
            LOG_DEBUG("AdapterStateMachine::Timer disable adapter start transport is %{public}d", transport);
            adapterTimer_->Start(ENABLE_DISABLE_TIMEOUT_TIME, false);
            adapter_->GetContext()->Disable();
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_DISABLE_CMP:
            LOG_DEBUG("AdapterStateMachine::Timer disable adapter stop transport is %{public}d", transport);
            adapterTimer_->Stop();
            Transition(TURN_OFF_STATE);
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_DISABLE_TIME_OUT:
            LOG_ERROR("AdapterStateMachine::Timer disable profile time out transport is %{public}d", transport);
            DftReportSwitchInfo(DISABLE_TIME_OUT, DFT_MSG_DISABLE_TIME_OUT, SWITCH_CLOSE, SWITCH_OPER_FAIL);
            ProfileServiceManager::GetInstance().OnAllDisabled(transport);
            adapterTimer_->Start(ENABLE_DISABLE_TIMEOUT_TIME, false);
            adapter_->GetContext()->Disable();
            SleServiceManager::GetInstance()->Reset();
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_DISABLE_TIME_OUT:
            LOG_ERROR("AdapterStateMachine::Timer disable adapter time out transport is %{public}d", transport);
            DftReportSwitchInfo(DISABLE_TIME_OUT, DFT_MSG_DISABLE_TIME_OUT, SWITCH_CLOSE, SWITCH_OPER_FAIL);
            SleServiceManager::GetInstance()->Reset();
            Transition(TURN_OFF_STATE);
            break;
        default:
            return false;
    }
    return true;
}

AdapterTurningOffToHalfState::AdapterTurningOffToHalfState(std::shared_ptr<AdapterStateMachine> stateMachine,
                                                           std::shared_ptr<SleInterfaceAdapter> adapter)
    : AdapterState(TURNING_OFF_TO_HALF_STATE, stateMachine, adapter)
{
    int adapterMsgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_ENABLE_TIME_OUT);
    adapterTimer_ = std::make_unique<NearlinkTimer>([stateMachine, adapterMsgId]() -> void {
        DoInServiceManagerThread([stateMachine, adapterMsgId]() -> void {
            stateMachine->ProcessMessage(utility::Message(adapterMsgId));
        });
    });
    int profileMsgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_ENABLE_TIME_OUT);
    profileTimer_ = std::make_unique<NearlinkTimer>([stateMachine, profileMsgId]() -> void {
        DoInServiceManagerThread([stateMachine, profileMsgId]() -> void {
            stateMachine->ProcessMessage(utility::Message(profileMsgId));
        });
    });
}

void AdapterTurningOffToHalfState::Entry()
{
    HILOGI("[AdapterTurningOffToHalfState] enter");
    SleTransport transport =
        (adapter_->GetContext()->Name() == ADAPTER_NAME_SLB) ? SleTransport::ADAPTER_SLB : SleTransport::ADAPTER_SLE;
    SleServiceManager::GetInstance()->OnAdapterStateChangeTask(transport, SleStateID::STATE_TURNING_OFF_TO_HALF);
    LOG_DEBUG("AdapterStateMachine::Timer enable adapter start transport is %{public}d", transport);
    adapterTimer_->Start(ENABLE_DISABLE_TIMEOUT_TIME, false);
    adapter_->GetContext()->Enable();
}

void AdapterTurningOffToHalfState::HandleAdapterEnableCmpMsg(SleTransport transport, const utility::Message &msg)
{
    HILOG_COMM_INFO("[%{public}s:%{public}d] enter", __FUNCTION__, __LINE__);
    LOG_DEBUG("AdapterStateMachine::Timer enable adapter stop transport is %{public}d", transport);
    adapterTimer_->Stop();
    if (msg.arg1M) {
        LOG_DEBUG("AdapterStateMachine::Timer enable profile start transport is %{public}d", transport);
        profileTimer_->Start(ENABLE_DISABLE_TIMEOUT_TIME, false);
        ProfileServiceManager::GetInstance().Enable(transport);
    } else {
        LOG_ERROR("AdapterStateMachine: MSG_ADAPTER_ENABLE_CMP failed");
        adapterStateMachine_->SetNextTargetState(SleStateID::STATE_TURN_OFF);
        Transition(TURNING_HALF_TO_OFF_STATE);
    }
}

void AdapterTurningOffToHalfState::HandleProfileEnableCmpMsg(SleTransport transport, const utility::Message &msg)
{
    HILOG_COMM_INFO("[%{public}s:%{public}d] enter", __FUNCTION__, __LINE__);
    LOG_DEBUG("AdapterStateMachine::Timer enable profile stop transport is %{public}d", transport);
    profileTimer_->Stop();
    if (msg.arg1M) {
        Transition(TURN_HALF_STATE);
    } else {
        LOG_ERROR("AdapterStateMachine: MSG_PROFILE_ENABLE_CMP failed");
        adapterStateMachine_->SetNextTargetState(SleStateID::STATE_TURN_OFF);
        ProfileServiceManager::GetInstance().OnAllEnabled(transport);
        Transition(TURNING_HALF_TO_OFF_STATE);
    }
}

bool AdapterTurningOffToHalfState::Dispatch(const utility::Message &msg)
{
    HILOGI("[AdapterTurningOffToHalfState] enter");
    SleTransport transport =
        (adapter_->GetContext()->Name() == ADAPTER_NAME_SLB) ? SleTransport::ADAPTER_SLB : SleTransport::ADAPTER_SLE;

    switch (static_cast<AdapterStateMachine::AdapterStateMessage>(msg.whatM)) {
        case AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_ENABLE_CMP:
            HandleAdapterEnableCmpMsg(transport, msg);
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_ENABLE_CMP:
            HandleProfileEnableCmpMsg(transport, msg);
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_ENABLE_TIME_OUT:
            LOG_ERROR("AdapterStateMachine::Timer enable adapter time out transport is %{public}d", transport);
            DftReportSwitchInfo(HALF_TIME_OUT, DFT_MSG_HALF_TIME_OUT, SWITCH_HALF, SWITCH_OPER_FAIL);
            SleServiceManager::GetInstance()->Reset();
            adapterStateMachine_->SetNextTargetState(SleStateID::STATE_TURN_OFF);
            Transition(TURNING_HALF_TO_OFF_STATE);
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_ENABLE_TIME_OUT:
            LOG_ERROR("AdapterStateMachine::Timer enable profile time out transport is %{public}d", transport);
            DftReportSwitchInfo(HALF_TIME_OUT, DFT_MSG_HALF_TIME_OUT, SWITCH_HALF, SWITCH_OPER_FAIL);
            SleServiceManager::GetInstance()->Reset();
            ProfileServiceManager::GetInstance().OnAllEnabled(transport);
            adapterStateMachine_->SetNextTargetState(SleStateID::STATE_TURN_OFF);
            Transition(TURNING_HALF_TO_OFF_STATE);
            break;
        default:
            return false;
    }
    return true;
}

AdapterTurningHalfToOnState::AdapterTurningHalfToOnState(std::shared_ptr<AdapterStateMachine> stateMachine,
                                                         std::shared_ptr<SleInterfaceAdapter> adapter)
    : AdapterState(TURNING_HALF_TO_ON_STATE, stateMachine, adapter)
{
    int adapterMsgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_TURNING_TASK_TIME_OUT);
    turningTaskTimer_ = std::make_unique<NearlinkTimer>([stateMachine, adapterMsgId]() -> void {
        DoInServiceManagerThread([stateMachine, adapterMsgId]() -> void {
            stateMachine->ProcessMessage(utility::Message(adapterMsgId));
        });
    });
}

void AdapterTurningHalfToOnState::Entry()
{
    HILOGI("[AdapterTurningHalfToOnState] enter");
    SleTransport transport =
        (adapter_->GetContext()->Name() == ADAPTER_NAME_SLB) ? SleTransport::ADAPTER_SLB : SleTransport::ADAPTER_SLE;
    SleServiceManager::GetInstance()->OnAdapterStateChangeTask(transport, SleStateID::STATE_TURNING_HALF_TO_ON);
    LOG_DEBUG("AdapterStateMachine::Timer disable profile start transport is %{public}d", transport);
    turningTaskTimer_->Start(TURNING_STATE_TIMEOUT_TIME, false);
    int msgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_TURNING_TASK_CMP);
    adapterStateMachine_->ProcessMessage(utility::Message(msgId));
}

bool AdapterTurningHalfToOnState::Dispatch(const utility::Message &msg)
{
    HILOGI("[AdapterTurningHalfToOnState] enter");
    SleTransport transport =
        (adapter_->GetContext()->Name() == ADAPTER_NAME_SLB) ? SleTransport::ADAPTER_SLB : SleTransport::ADAPTER_SLE;

    switch (static_cast<AdapterStateMachine::AdapterStateMessage>(msg.whatM)) {
        case AdapterStateMachine::AdapterStateMessage::MSG_TURNING_TASK_CMP:
            LOG_DEBUG("AdapterStateMachine::Timer turning half_to_on stop transport is %{public}d", transport);
            turningTaskTimer_->Stop();
            Transition(TURN_ON_STATE);
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_TURNING_TASK_TIME_OUT:
            LOG_ERROR("AdapterStateMachine::Timer turning half_to_on time out transport is %{public}d", transport);
            DftReportSwitchInfo(ENABLE_TIME_OUT, DFT_MSG_ENABLE_TIME_OUT, SWITCH_OPEN, SWITCH_OPER_FAIL);
            Transition(TURNING_HALF_TO_OFF_STATE);
            break;
        default:
            return false;
    }
    return true;
}

void AdapterTurnHalfState::Entry()
{
    HILOGI("[AdapterTurnHalfState] enter");
    SleTransport transport =
        (adapter_->GetContext()->Name() == ADAPTER_NAME_SLB) ? SleTransport::ADAPTER_SLB : SleTransport::ADAPTER_SLE;
    SleServiceManager::GetInstance()->OnAdapterStateChangeTask(transport, SleStateID::STATE_TURN_HALF);
    DftReportSwitchInfo(SWITCH_SUCCESS, DFT_MSG_SWITCH_SUCCESS, SWITCH_HALF);

    if (adapterStateMachine_->GetNextTargetState() != SleStateID::STATE_TURN_HALF) {
        adapterStateMachine_->PostTargetMessage();
        return;
    };
}

bool AdapterTurnHalfState::Dispatch(const utility::Message &msg)
{
    HILOGI("[AdapterTurnHalfState] enter");
    switch (static_cast<AdapterStateMachine::AdapterStateMessage>(msg.whatM)) {
        case AdapterStateMachine::AdapterStateMessage::MSG_ENABLE_REQ:
            Transition(TURNING_HALF_TO_ON_STATE);
            break;
        case AdapterStateMachine::AdapterStateMessage::MSG_DISABLE_REQ:
            Transition(TURNING_HALF_TO_OFF_STATE);
            break;
        default:
            return false;
    }
    return true;
}

} // namespace Nearlink
} // namespace OHOS