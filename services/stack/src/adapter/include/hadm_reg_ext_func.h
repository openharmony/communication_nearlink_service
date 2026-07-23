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

#ifndef HADM_REG_EXT_FUNC_H
#define HADM_REG_EXT_FUNC_H

#include <stdint.h>
#include <stdbool.h>
#include "dli_event_struct.h"
#include "hadm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*HADM_ReadRemoteExtFeaturesPtr)(uint16_t companyid, uint16_t subversion, uint16_t lcid);
typedef bool (*HADM_ProcessCsCapsPtr)(const uint8_t *rawData, uint32_t len, bool isLocal);
typedef void (*HADM_CheckAndUpdateMultiToneConfigPtr)(uint16_t lcid,
    uint8_t *pmInitSignal2Tone, uint8_t *pmReflSignal2Tone, uint16_t *occurrenceGroupPeriod);
typedef void (*HADM_ClearRemoteCsCapsPtr)(uint16_t lcid);

typedef struct {
    HADM_ReadRemoteExtFeaturesPtr readRemoteExtFeatures;
    HADM_ProcessCsCapsPtr processCsCaps;
    HADM_CheckAndUpdateMultiToneConfigPtr checkAndUpdateMultiToneConfig;
    HADM_ClearRemoteCsCapsPtr clearRemoteCsCaps;
} HADM_ExtFuncList;

void HADM_RegisterExtFunc(void *soHandle);
void HADM_DeregisterExtFunc(void);
HADM_ExtFuncList *HADM_GetExtFuncList(void);

#ifdef __cplusplus
}
#endif

#endif /* HADM_REG_EXT_FUNC_H */
