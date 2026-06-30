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
#ifndef ADAPTER_STATE_MACHINE_H
#define ADAPTER_STATE_MACHINE_H

#include <memory>
#include "state_machine.h"
#include "nearlink_timer.h"
#include "ProfileServiceManager.h"
#include "SleInterfaceAdapter.h"

namespace OHOS {
namespace Nearlink {
// adapter state machine each state name
const std::string TURNING_ON_STATE = "TurningOn";
const std::string TURN_ON_STATE = "TurnOn";
const std::string TURNING_OFF_STATE = "TurningOff";
const std::string TURN_OFF_STATE = "TurnOff";
const std::string TURNING_HALF_TO_OFF_STATE = "TurningHalfToOff";
const std::string TURNING_OFF_TO_HALF_STATE = "TurningOffToHalf";
const std::string TURNING_HALF_TO_ON_STATE = "TurningHalfToOn";
const std::string TURN_HALF_STATE = "TurnHalf";

/**
 * @brief Represents adapter state machine.
 *
 * @since 6
 */
class AdapterStateMachine : public utility::StateMachine,
                            public std::enable_shared_from_this<AdapterStateMachine> {
public:
    // define adapter state machine message kinds
    enum class AdapterStateMessage {
        MSG_ENABLE_REQ,
        MSG_DISABLE_REQ,
        MSG_HALF_DISABLE_REQ,
        MSG_ADAPTER_ENABLE_CMP,
        MSG_ADAPTER_DISABLE_CMP,
        MSG_PROFILE_ENABLE_CMP,
        MSG_PROFILE_DISABLE_CMP,
        MSG_TURNING_TASK_CMP,
        MSG_ADAPTER_ENABLE_TIME_OUT,
        MSG_PROFILE_ENABLE_TIME_OUT,
        MSG_ADAPTER_DISABLE_TIME_OUT,
        MSG_PROFILE_DISABLE_TIME_OUT,
        MSG_TURNING_TASK_TIME_OUT,
    };

    /**
     * @brief A constructor used to create an <b>AdapterStateMachine</b> instance.
     *
     * @since 6
     */
    AdapterStateMachine() {}

    /**
     * @brief A destructor used to delete the <b>AdapterStateMachine</b> instance.
     *
     * @since 6
     */
    ~AdapterStateMachine() = default;

    /**
     * @brief A constructor used to create an <b>AdapterStateMachine</b> instance.
     *
     * @param adapter Adapter pointer(sle adapter).
     * @since 6
     */
    void Init(std::shared_ptr<SleInterfaceAdapter> adapter);

    /**
     * @brief when receive change request during a internal state, temporarily save target state
     *
     * @param targetState SleStateID::STATE_TURN_OFF or SleStateID::STATE_TURN_ON or SleStateID::STATE_TURN_HALF
     * @since 6
     */
    void SetNextTargetState(const SleStateID& targetState);

    /**
     * @brief get next target state
     *
     * @return target state, SleStateID::STATE_TURN_OFF or SleStateID::STATE_TURN_ON or SleStateID::STATE_TURN_HALF
     */
    SleStateID GetNextTargetState() const;

    /**
     * @brief post message for current target state, which should happen only in stable states
     */
    void PostTargetMessage() const;

private:
    std::atomic<SleStateID> nextTargetState_ = SleStateID::STATE_TURN_OFF;
};

/**
 * @brief Represents basic adapter state.
 *
 * @since 6
 */
class AdapterState : public utility::StateMachine::State {
public:
    /**
     * @brief A constructor used to create an <b>AdapterState</b> instance.
     *
     * @param name Name of adapter state.
     * @param stateMachine The state machine which this state belong to.
     * @param adapter Adapter pointer(sle adapter).
     * @since 6
     */
    AdapterState(const std::string &name,
            std::shared_ptr<AdapterStateMachine> stateMachine,
            std::shared_ptr<SleInterfaceAdapter> adapter)
        : State(name, *stateMachine), adapter_(std::move(adapter)), adapterStateMachine_(std::move(stateMachine)){};
    /**
     * @brief A destructor used to delete the <b>AdapterState</b> instance.
     *
     * @since 6
     */
    ~AdapterState() = default;

protected:
    std::shared_ptr<SleInterfaceAdapter> adapter_;
    std::shared_ptr<AdapterStateMachine> adapterStateMachine_;
};

class AdapterTurningOnState : public AdapterState {
public:
    /**
     * @brief A constructor used to create an <b>AdapterTurningOnState</b> instance.
     *
     * @param stateMachine The state machine which this state belong to.
     * @param adapter Adapter pointer(sle adapter).
     * @since 6
     */
    AdapterTurningOnState(std::shared_ptr<AdapterStateMachine> stateMachine,
            std::shared_ptr<SleInterfaceAdapter> adapter);

    /**
     * @brief A destructor used to delete the <b>AdapterTurningOnState</b> instance.
     *
     * @since 6
     */
    ~AdapterTurningOnState() = default;
    /**
     * @brief Entry adapter turning on state.
     *
     * @since 6
     */
    virtual void Entry();

    /**
     * @brief Exit adapter turning on state.
     *
     * @since 6
     */
    virtual void Exit(){};

    /**
     * @brief Adapter turning on state's dispatch.
     *
     * @param msg Message context which is used in dispath.
     * @return Returns <b>true</b> if the operation is accepted;
     *         returns <b>false</b> if the operation is rejected.
     * @since 6
     */
    virtual bool Dispatch(const utility::Message &msg);

private:
    void HandleAdapterEnableCmpMsg(SleTransport transport, const utility::Message &msg);
    void HandleProfileEnableCmpMsg(SleTransport transport, const utility::Message &msg);
    // adapter turning off state timer
    std::unique_ptr<NearlinkTimer> adapterTimer_ = nullptr;
    // profile turning off state timer
    std::unique_ptr<NearlinkTimer> profileTimer_ = nullptr;
};

class AdapterTurnOnState : public AdapterState {
public:
    /**
     * @brief A constructor used to create an <b>AdapterTurnOnState</b> instance.
     *
     * @param stateMachine The state machine which this state belong to.
     * @param adapter Adapter pointer(sle adapter).
     * @since 6
     */
    AdapterTurnOnState(std::shared_ptr<AdapterStateMachine> stateMachine,
            std::shared_ptr<SleInterfaceAdapter> adapter)
        : AdapterState(TURN_ON_STATE, stateMachine, adapter){};

    /**
     * @brief A destructor used to delete the <b>AdapterTurnOnState</b> instance.
     *
     * @since 6
     */
    ~AdapterTurnOnState() = default;
    /**
     * @brief Entry adapter turn on state.
     *
     * @since 6
     */
    virtual void Entry();

    /**
     * @brief Exit adapter turn on state.
     *
     * @since 6
     */
    virtual void Exit(){};

    /**
     * @brief Adapter turn on state's dispatch.
     *
     * @param msg Message context which is used in dispath.
     * @return Returns <b>true</b> if the operation is accepted;
     *         returns <b>false</b> if the operation is rejected.
     * @since 6
     */
    virtual bool Dispatch(const utility::Message &msg);
};

class AdapterTurningOffState : public AdapterState {
public:
    /**
     * @brief A constructor used to create an <b>AdapterTurningOffState</b> instance.
     *
     * @param stateMachine The state machine which this state belong to.
     * @param adapter Adapter pointer(sle adapter).
     * @since 6
     */
    AdapterTurningOffState(std::shared_ptr<AdapterStateMachine> stateMachine,
            std::shared_ptr<SleInterfaceAdapter> adapter);

    /**
     * @brief A destructor used to delete the <b>AdapterTurningOffState</b> instance.
     *
     * @since 6
     */
    ~AdapterTurningOffState() = default;

    /**
     * @brief Entry adapter turning off state.
     *
     * @since 6
     */
    virtual void Entry();

    /**
     * @brief Exit adapter turning off state.
     *
     * @since 6
     */
    virtual void Exit(){};

    /**
     * @brief Adapter turning off state's dispatch.
     *
     * @param msg Message context which is used in dispath.
     * @return Returns <b>true</b> if the operation is accepted;
     *         returns <b>false</b> if the operation is rejected.
     * @since 6
     */
    virtual bool Dispatch(const utility::Message &msg);

private:
    // adapter turning off state timer
    std::unique_ptr<NearlinkTimer> adapterTimer_ = nullptr;
    // profile turning off state timer
    std::unique_ptr<NearlinkTimer> profileTimer_ = nullptr;
};

class AdapterTurnOffState : public AdapterState {
public:
    /**
     * @brief A constructor used to create an <b>AdapterTurnOffState</b> instance.
     *
     * @param stateMachine The state machine which this state belong to.
     * @param adapter Adapter pointer(sle adapter).
     * @since 6
     */
    AdapterTurnOffState(std::shared_ptr<AdapterStateMachine> stateMachine,
            std::shared_ptr<SleInterfaceAdapter> adapter)
        : AdapterState(TURN_OFF_STATE, stateMachine, adapter){};

    /**
     * @brief A destructor used to delete the <b>AdapterTurnOffState</b> instance.
     *
     * @since 6
     */
    ~AdapterTurnOffState() = default;

    /**
     * @brief Entry adapter turn off state.
     *
     * @since 6
     */
    virtual void Entry();

    /**
     * @brief Exit adapter turn off state.
     *
     * @since 6
     */
    virtual void Exit(){};

    /**
     * @brief Adapter turn off state's dispatch.
     *
     * @param msg Message context which is used in dispath.
     * @return Returns <b>true</b> if the operation is accepted;
     *         returns <b>false</b> if the operation is rejected.
     * @since 6
     */
    virtual bool Dispatch(const utility::Message &msg);
};

class AdapterTurningHalfToOffState : public AdapterState {
public:
    /**
     * @brief A constructor used to create an <b>AdapterTurningHalfToOffState</b> instance.
     *
     * @param stateMachine The state machine which this state belong to.
     * @param adapter Adapter pointer(sle adapter).
     * @since 6
     */
    AdapterTurningHalfToOffState(std::shared_ptr<AdapterStateMachine> stateMachine,
            std::shared_ptr<SleInterfaceAdapter> adapter);

    /**
     * @brief A destructor used to delete the <b>AdapterTurningHalfToOffState</b> instance.
     *
     * @since 6
     */
    ~AdapterTurningHalfToOffState() = default;

    /**
     * @brief Entry adapter turning half_to_off state.
     *
     * @since 6
     */
    virtual void Entry();

    /**
     * @brief Exit adapter turning half_to_off state.
     *
     * @since 6
     */
    virtual void Exit(){};

    /**
     * @brief Adapter turning half_to_off state's dispatch.
     *
     * @param msg Message context which is used in dispath.
     * @return Returns <b>true</b> if the operation is accepted;
     *         returns <b>false</b> if the operation is rejected.
     * @since 6
     */
    virtual bool Dispatch(const utility::Message &msg);

private:
    // adapter turning half_to_off state timer
    std::unique_ptr<NearlinkTimer> adapterTimer_ = nullptr;
    // profile turning half_to_off state timer
    std::unique_ptr<NearlinkTimer> profileTimer_ = nullptr;
};

class AdapterTurningOffToHalfState : public AdapterState {
public:
    /**
     * @brief A constructor used to create an <b>AdapterTurningOffToHalfState</b> instance.
     *
     * @param stateMachine The state machine which this state belong to.
     * @param adapter Adapter pointer(sle adapter).
     * @since 6
     */
    AdapterTurningOffToHalfState(std::shared_ptr<AdapterStateMachine> stateMachine,
            std::shared_ptr<SleInterfaceAdapter> adapter);

    /**
     * @brief A destructor used to delete the <b>AdapterTurningOffToHalfState</b> instance.
     *
     * @since 6
     */
    ~AdapterTurningOffToHalfState() = default;

    /**
     * @brief Entry adapter turning off_to_half state.
     *
     * @since 6
     */
    virtual void Entry();

    /**
     * @brief Exit adapter turning off_to_half state.
     *
     * @since 6
     */
    virtual void Exit(){};

    /**
     * @brief Adapter turning off_to_half state's dispatch.
     *
     * @param msg Message context which is used in dispath.
     * @return Returns <b>true</b> if the operation is accepted;
     *         returns <b>false</b> if the operation is rejected.
     * @since 6
     */
    virtual bool Dispatch(const utility::Message &msg);

private:
    void HandleAdapterEnableCmpMsg(SleTransport transport, const utility::Message &msg);
    void HandleProfileEnableCmpMsg(SleTransport transport, const utility::Message &msg);
    // adapter turning off_to_half state timer
    std::unique_ptr<NearlinkTimer> adapterTimer_ = nullptr;
    // profile turning off_to_half state timer
    std::unique_ptr<NearlinkTimer> profileTimer_ = nullptr;
};

class AdapterTurningHalfToOnState : public AdapterState {
public:
    /**
     * @brief A constructor used to create an <b>AdapterTurningHalfToOnState</b> instance.
     *
     * @param stateMachine The state machine which this state belong to.
     * @param adapter Adapter pointer(sle adapter).
     * @since 6
     */
    AdapterTurningHalfToOnState(std::shared_ptr<AdapterStateMachine> stateMachine,
            std::shared_ptr<SleInterfaceAdapter> adapter);

    /**
     * @brief A destructor used to delete the <b>AdapterTurningHalfToOnState</b> instance.
     *
     * @since 6
     */
    ~AdapterTurningHalfToOnState() = default;

    /**
     * @brief Entry adapter turning half_to_on state.
     *
     * @since 6
     */
    virtual void Entry();

    /**
     * @brief Exit adapter turning half_to_on state.
     *
     * @since 6
     */
    virtual void Exit(){};

    /**
     * @brief Adapter turning half_to_on state's dispatch.
     *
     * @param msg Message context which is used in dispath.
     * @return Returns <b>true</b> if the operation is accepted;
     *         returns <b>false</b> if the operation is rejected.
     * @since 6
     */
    virtual bool Dispatch(const utility::Message &msg);

private:
    // turning half_to_on state timer
    std::unique_ptr<NearlinkTimer> turningTaskTimer_ = nullptr;
};

class AdapterTurnHalfState : public AdapterState {
public:
    /**
     * @brief A constructor used to create an <b>AdapterTurnHalfState</b> instance.
     *
     * @param stateMachine The state machine which this state belong to.
     * @param adapter Adapter pointer(sle adapter).
     * @since 6
     */
    AdapterTurnHalfState(std::shared_ptr<AdapterStateMachine> stateMachine,
            std::shared_ptr<SleInterfaceAdapter> adapter)
        : AdapterState(TURN_HALF_STATE, stateMachine, adapter){};

    /**
     * @brief A destructor used to delete the <b>AdapterTurnHalfState</b> instance.
     *
     * @since 6
     */
    ~AdapterTurnHalfState() = default;

    /**
     * @brief Entry adapter turn off state.
     *
     * @since 6
     */
    virtual void Entry();

    /**
     * @brief Exit adapter turn off state.
     *
     * @since 6
     */
    virtual void Exit(){};

    /**
     * @brief Adapter turn off state's dispatch.
     *
     * @param msg Message context which is used in dispath.
     * @return Returns <b>true</b> if the operation is accepted;
     *         returns <b>false</b> if the operation is rejected.
     * @since 6
     */
    virtual bool Dispatch(const utility::Message &msg);
};

} // namespace Nearlink
} // namespace OHOS
#endif