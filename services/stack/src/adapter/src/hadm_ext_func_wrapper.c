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

#include "hadm_ext_func_wrapper.h"
#include "hadm_reg_ext_func.h"
#include "adapter_log.h"

bool DLI_ReadRemoteExtFeatures(uint16_t companyid, uint16_t subversion, uint16_t lcid)
{
    HADM_ExtFuncList *funcList = HADM_GetExtFuncList();
    if (funcList == NULL || funcList->readRemoteExtFeatures == NULL) {
        ADAPTER_LOGW("HADM readRemoteExtFeatures not registered, skip private features");
        return false;
    }
    return funcList->readRemoteExtFeatures(companyid, subversion, lcid);
}

void HADM_ExtCheckAndUpdateMultiToneConfig(uint16_t lcid, uint8_t *pmInitSignal2Tone, uint8_t *pmReflSignal2Tone,
    uint16_t *occurrenceGroupPeriod)
{
    HADM_ExtFuncList *funcList = HADM_GetExtFuncList();
    if (funcList == NULL || funcList->checkAndUpdateMultiToneConfig == NULL) {
        ADAPTER_LOGW("HADM checkAndUpdateMultiToneConfig not registered, skip multi-tone config");
        return;
    }
    funcList->checkAndUpdateMultiToneConfig(lcid, pmInitSignal2Tone, pmReflSignal2Tone, occurrenceGroupPeriod);
}

void HADM_ExtClearRemoteCsCaps(uint16_t lcid)
{
    HADM_ExtFuncList *funcList = HADM_GetExtFuncList();
    if (funcList == NULL || funcList->clearRemoteCsCaps == NULL) {
        ADAPTER_LOGW("HADM clearRemoteCsCaps not registered, skip clearing remote CS caps");
        return;
    }
    funcList->clearRemoteCsCaps(lcid);
}
