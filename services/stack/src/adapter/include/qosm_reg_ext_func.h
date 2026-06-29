/**
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

#ifndef QOSM_REG_EXT_FUNC_H
#define QOSM_REG_EXT_FUNC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*QOSM_AntennaDfxSendQueryCmdPtr)(void);
typedef int (*QOSM_AntennaDfxGetAntennaPolicyPtr)(void);
typedef bool (*QOSM_IsLastEnableFreqBandByRecommendPtr)(void);
typedef bool (*QOSM_IsNeedRecoverFreqBandAbilityPtr)(void);

typedef struct {
    QOSM_AntennaDfxSendQueryCmdPtr antennaDfxSendQueryCmd;
    QOSM_AntennaDfxGetAntennaPolicyPtr antennaDfxGetAntennaPolicy;
    QOSM_IsLastEnableFreqBandByRecommendPtr isLastEnableFreqBandByRecommend;
    QOSM_IsNeedRecoverFreqBandAbilityPtr isNeedRecoverFreqBandAbility;
} QOSM_ExtFuncList;

void QOSM_RegisterExtFunc(void *soHandle);

void QOSM_DeregisterExtFunc(void);

QOSM_ExtFuncList *QOSM_GetExtFuncList(void);

#ifdef __cplusplus
}
#endif

#endif