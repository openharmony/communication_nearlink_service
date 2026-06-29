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

#ifndef ASC_STATEMACHINE_H
#define ASC_STATEMACHINE_H

#include <list>
#include <memory>
#include <string>

#include "ASCMessage.h"
#include "SleInterfaceAdapter.h"
#include "state_machine.h"
#include "ASCDefines.h"
#include "SleInterfaceManager.h"

namespace OHOS {
namespace Nearlink {
class ASCService;
class ASCStateMachine : public utility::StateMachine {
public:
    /**
     * @brief Construct a new ASCStateMachine object.
     *
     * @param address Device address.
     */
    explicit ASCStateMachine(const std::string &address);

    /**
     * @brief Destroy the ASCStateMachine object.
     */
    ~ASCStateMachine() = default;

    /**
     * @brief Initialise the state machine.
     */
    void Init();

    void NotifyStateTransitions();
    static std::string GetEventName(int what);

    int ProcessASCOpenDeviceReq(const ASCMessage &msg);
    int ProcessASCCloseDeviceReq(const ASCMessage &msg);
    void ProcessASCOpenComplete(const ASCMessage &msg);

    inline static const std::string disabled = "Disabled";
    inline static const std::string disabling = "Disabling";
    inline static const std::string enabled = "Enabled";
    inline static const std::string enabling = "Enabling";
    inline static const std::string connected = "Connected";
    inline static const std::string connecting = "Connecting";
    inline static const std::string disconnected = "Disconnected";
    inline static const std::string disconnecting = "Disconnecting";
    inline static const std::string configed = "Configed";
    inline static const std::string configuring = "Configuring";
    inline static const std::string opened = "Opened";
    inline static const std::string opening = "Opening";
    inline static const std::string started = "Started";
    inline static const std::string starting = "Starting";
    inline static const std::string stopped = "Stopped";
    inline static const std::string stopping = "Stopping";
    inline static const std::string released = "Released";
    inline static const std::string releasing = "Releasing";
    inline static const std::string reconfigStopping = "ReconfigStopping";
    inline static const std::string reconfigStopped = "ReconfigStopped";
    inline static const std::string reconfiged = "Reconfiged";
    inline static const std::string reconfiguring = "Reconfiguring";

private:
    std::string address_;
    int preState_ {0};

    SLE_ASCALLOW_COPY_AND_ASSIGN(ASCStateMachine);
}; // class ASCStateMachine

class ASCState : public utility::StateMachine::State {
public:
    ASCState(const std::string &name, utility::StateMachine &statemachine, int stateInt,
        utility::StateMachine::State &parent)
        : State(name, statemachine, parent), stateInt_(stateInt), stateMachine_((ASCStateMachine &)statemachine)
    {}

    ASCState(const std::string &name, utility::StateMachine &statemachine, int stateInt)
        : State(name, statemachine), stateInt_(stateInt), stateMachine_((ASCStateMachine &)statemachine)
    {}

    virtual ~ASCState()
    {}

    int GetStateInt() const
    {
        return stateInt_;
    }

protected:
    int stateInt_ {NL_SLE_ASC_DISABLED};
    ASCStateMachine &stateMachine_;
}; // class ASCState

class ASCConnectedState : public ASCState {
public:
    ASCConnectedState(const std::string &name, utility::StateMachine &statemachine)
        : ASCState(name, statemachine, NL_SLE_ASC_CONNECTED)
    {}
    ~ASCConnectedState() override = default;
    void Entry() override;
    void Exit() override;
    bool ASCpatch(const utility::Message &msg) override;

private:
    bool ASCpatchASC(const utility::Message &msg);
};

class ASCConnectingState : public ASCState {
public:
    ASCConnectingState(const std::string &name, utility::StateMachine &statemachine)
        : ASCState(name, statemachine, NL_SLE_ASC_CONNECTING)
    {}
    ~ASCConnectingState() override = default;
    void Entry() override;
    void Exit() override;
    bool ASCpatch(const utility::Message &msg) override;

private:
    bool ASCpatchASC(const utility::Message &msg);
};

class ASCDisconnectingState : public ASCState {
public:
    ASCDisconnectingState(const std::string &name, utility::StateMachine &statemachine)
        : ASCState(name, statemachine, NL_SLE_ASC_DISCONNECTING)
    {}
    ~ASCDisconnectingState() override = default;
    void Entry() override;
    void Exit() override;
    bool ASCpatch(const utility::Message &msg) override;

private:
    bool ASCpatchASC(const utility::Message &msg);
};

class ASCDisconnectedState : public ASCState {
public:
    ASCDisconnectedState(const std::string &name, utility::StateMachine &statemachine)
        : ASCState(name, statemachine, NL_SLE_ASC_DISCONNECTED)
    {}
    ~ASCDisconnectedState() override = default;
    void Entry() override;
    void Exit() override;
    bool ASCpatch(const utility::Message &msg) override;

private:
    bool ASCpatchASC(const utility::Message &msg);
};

} // namespace Sle
} // namespace OHOS
#endif
