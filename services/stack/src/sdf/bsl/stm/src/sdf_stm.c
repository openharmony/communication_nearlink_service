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

#include "securec.h"
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include "sdf_mem.h"
#include "sdf_stm.h"

#define SDF_STM_UNKNOWN_STATE_NAME "UnknownStateName"

static void StateMachineTransition(StateMachine *stm, const char *targetStateName);
static void StateMachineProcessMessage(StateMachine *stm, Message msg);
static bool StateMachineEmplaceNewState(StateMachine *stm, State *state);
static const char *StateMachineGetCurrentStateName(StateMachine *stm);
static void StateTransition(State *state, const char *targetStateName);
static void StateEntry(State *state);
static void StateExit(State *state);
static void StateDispatch(State *state, Message msg);

void StateMachineSoftBaseDtor(StateMachine *stm)
{
    for (State *cur = stm->states_; cur != NULL;) {
        State *next = cur->next;
        StateDtor(cur);
        cur = next;
    }
}

bool StateMachineSoftBaseCtor(StateMachine *stm)
{
    *stm = (StateMachine) {
        .Transition = StateMachineTransition,
        .ProcessMessage = StateMachineProcessMessage,
        .EmplaceNewState = StateMachineEmplaceNewState,
        .GetCurrentStateName = StateMachineGetCurrentStateName,
        .states_ = NULL,
        .current_ = NULL,
    };
    return true;
}

void StateDtor(State *state)
{
    if (state != NULL) {
        SDF_MemFree(state->name_);
        SDF_MemFree(state);
    }
}

State *StateCtor(StateMachine *stm, const char *name)
{
    State *state = SDF_MemZalloc(sizeof(State));
    if (state == NULL) {
        goto EXIT_LABEL;
    }

    *state = (State) {
        .Transition = StateTransition,
        .Entry = StateEntry,
        .Exit = StateExit,
        .Dispatch = StateDispatch,
        .stm_ = stm,
        .name_ = NULL,
        .next = NULL,
    };

    size_t nameLen = strlen(name);
    char *nameBuf = SDF_MemZalloc(sizeof(char) * (nameLen + 1));
    if (nameBuf == NULL) {
        goto EXIT_LABEL;
    }
    (void)memcpy_s(nameBuf, nameLen + 1, name, nameLen + 1);
    state->name_ = nameBuf;

    return state;

EXIT_LABEL:
    StateDtor(state);
    return NULL;
}

static State *StateMachineFindState(State *head, const char *name)
{
    for (State *cur = head; cur != NULL; cur = cur->next) {
        if (strncmp(cur->name_, name, strlen(cur->name_)) == 0) {
            return cur;
        }
    }
    return NULL;
}

static void StateMachineTransition(StateMachine *stm, const char *targetStateName)
{
    State *it = StateMachineFindState(stm->states_, targetStateName);
    if (it != NULL) {
        State *targetState = it;
        if (stm->current_) {
            stm->current_->Exit(stm->current_);
        }
        // if targetState is final state, stm may be destructed by entry func
        stm->current_ = targetState;
        targetState->Entry(targetState);
    }
}

static void StateMachineProcessMessage(StateMachine *stm, Message msg)
{
    if (stm->current_ && stm->current_->Dispatch) {
        stm->current_->Dispatch(stm->current_, msg);
    }
}

static bool StateMachineEmplaceNewState(StateMachine *stm, State *state)
{
    state->next = stm->states_;
    stm->states_ = state;
    return true;
}

static const char *StateMachineGetCurrentStateName(StateMachine *stm)
{
    if (stm->current_) {
        return stm->current_->name_;
    }
    return SDF_STM_UNKNOWN_STATE_NAME;
}

static void StateTransition(State *state, const char *targetStateName)
{
    state->stm_->Transition(state->stm_, targetStateName);
}

static void StateEntry(State *state)
{
    (void)state;
    // default implement
}

static void StateExit(State *state)
{
    (void)state;
    // default implement
}

static void StateDispatch(State *state, Message msg)
{
    (void)state;
    (void)msg;
    // default implement
}