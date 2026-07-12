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

#include "devd_ext_func_wrapper.h"

#include <stdio.h>
#include "devd_reg_ext_func.h"
#include "adapter_log.h"

uint32_t DevdScanFilterInit(const NLSTK_DevdCollabScanFilterFunc_S *filterFunc)
{
    Devd_ExtFuncList *funcList = Devd_GetExtFuncList();
    if (funcList == NULL || funcList->devdScanFilterInit == NULL) {
        ADAPTER_LOGW("get func devdScanFilterInit failed");
        return NLSTK_ERRCODE_FAIL;
    }
    return funcList->devdScanFilterInit(filterFunc);
}

void DevdScanFilterDeInit(void)
{
    Devd_ExtFuncList *funcList = Devd_GetExtFuncList();
    if (funcList == NULL || funcList->devdScanFilterDeInit == NULL) {
        ADAPTER_LOGW("get func devdScanFilterDeInit failed");
        return;
    }
    funcList->devdScanFilterDeInit();
    return;
}

NLSTK_Errcode_E DevdSetScanfilterSwitch(NLSTK_DevdScanFilterSwitchAction_E action)
{
    Devd_ExtFuncList *funcList = Devd_GetExtFuncList();
    if (funcList == NULL || funcList->devdSetScanfilterSwitch == NULL) {
        ADAPTER_LOGW("get func devdSetScanfilterSwitch failed");
        return NLSTK_ERRCODE_FAIL;
    }
    return funcList->devdSetScanfilterSwitch(action);
}

bool DevdIsSupportScanFilter(void)
{
    Devd_ExtFuncList *funcList = Devd_GetExtFuncList();
    if (funcList == NULL || funcList->devdIsSupportScanFilter == NULL) {
        ADAPTER_LOGW("get func devdIsSupportScanFilter failed");
        return false;
    }
    return funcList->devdIsSupportScanFilter();
}

void DevdSetScanfilter(NLSTK_DevdScanFilter_S *filter)
{
    Devd_ExtFuncList *funcList = Devd_GetExtFuncList();
    if (funcList == NULL || funcList->devdSetScanfilter == NULL) {
        ADAPTER_LOGW("get func devdSetScanfilter failed");
        return;
    }
    funcList->devdSetScanfilter(filter);
    return;
}


void DevdAddChipFilters(SDF_Vector_S *scanners, NLSTK_DevdScanner_S *scanner, NLSTK_DevdScanSetting_S *setting,
    SDF_Vector_S *filters)
{
    Devd_ExtFuncList *funcList = Devd_GetExtFuncList();
    if (funcList == NULL || funcList->devdAddChipFilters == NULL) {
        ADAPTER_LOGW("get func devdAddChipFilters failed");
        return;
    }

    funcList->devdAddChipFilters(scanners, scanner, setting, filters);
}

void DevdDeleteChipFilters(NLSTK_DevdScanManager_S *manager, NLSTK_DevdScanner_S *scanner)
{
    Devd_ExtFuncList *funcList = Devd_GetExtFuncList();
    if (funcList == NULL || funcList->devdDeleteChipFilters == NULL) {
        ADAPTER_LOGW("get func devdDeleteChipFilters failed");
        return;
    }

    funcList->devdDeleteChipFilters(manager, scanner);
}