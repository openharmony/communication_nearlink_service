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

#include "qosm_ext_func_wrapper.h"

#include <stdio.h>
#include "qosm_reg_ext_func.h"
#include "adapter_log.h"

void QOSM_EXT_AntennaDfxSendQueryCmd(void)
{
    QOSM_ExtFuncList *funcList = QOSM_GetExtFuncList();
    if (funcList == NULL || funcList->antennaDfxSendQueryCmd == NULL) {
        ADAPTER_LOGW("get func antennaDfxSendQueryCmd failed");
        return;
    }

    return funcList->antennaDfxSendQueryCmd();
}

int QOSM_EXT_AntennaDfxGetAntennaPolicy(void)
{
    QOSM_ExtFuncList *funcList = QOSM_GetExtFuncList();
    if (funcList == NULL || funcList->antennaDfxGetAntennaPolicy == NULL) {
        ADAPTER_LOGW("get func antennaDfxGetAntennaPolicy failed");
        return -1;
    }

    return funcList->antennaDfxGetAntennaPolicy();
}

bool QOSM_IsLastEnableFreqBandByRecommend(void)
{
    QOSM_ExtFuncList *funcList = QOSM_GetExtFuncList();
    if (funcList == NULL || funcList->isLastEnableFreqBandByRecommend == NULL) {
        ADAPTER_LOGW("get func isLastEnableFreqBandByRecommend failed");
        return false;
    }

    return funcList->isLastEnableFreqBandByRecommend();
}

bool QOSM_IsNeedRecoverFreqBandAbility(void)
{
    QOSM_ExtFuncList *funcList = QOSM_GetExtFuncList();
    if (funcList == NULL || funcList->isNeedRecoverFreqBandAbility == NULL) {
        ADAPTER_LOGW("get func isNeedRecoverFreqBandAbility failed");
        return false;
    }

    return funcList->isNeedRecoverFreqBandAbility();
}