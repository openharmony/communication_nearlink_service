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

#ifndef SDF_STM_H
#define SDF_STM_H

#include <stdint.h>
#include <stdbool.h>

#include "sdf_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STM_MFUNC(this, field, ...) (((StateMachine *)(this))->field)((StateMachine *)(this), ##__VA_ARGS__)

void StateMachineSoftBaseDtor(StateMachine *stm);
bool StateMachineSoftBaseCtor(StateMachine *stm);

void StateDtor(State *state);
State *StateCtor(StateMachine *stm, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* SDF_STM_H */
