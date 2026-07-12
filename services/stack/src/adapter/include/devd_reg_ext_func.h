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

#ifndef DEVD_REG_EXT_FUNC_H
#define DEVD_REG_EXT_FUNC_H

#include <stdint.h>
#include "sdf_vector.h"
#include "nlstk_stm_collab_ext.h"
#include "nlstk_reg_collab_stm_scan_ext.h"
#include "nlstk_devd_scan_type_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t (*DevdScanFilterInitPtr)(const NLSTK_DevdCollabScanFilterFunc_S *filterFunc);
typedef void (*DevdScanFilterDeInitPtr)(void);
typedef NLSTK_Errcode_E (*DevdSetScanfilterSwitchPtr)(NLSTK_DevdScanFilterSwitchAction_E action);
typedef bool (*DevdIsSupportScanFilterPtr)(void);
typedef void (*DevdSetScanfilterPtr)(NLSTK_DevdScanFilter_S *filter);
typedef void (*DevdAddChipFiltersPtr)(
    SDF_Vector_S *scanners, NLSTK_DevdScanner_S *scanner, NLSTK_DevdScanSetting_S *setting, SDF_Vector_S *filters);
typedef void (*DevdDeleteChipFiltersPtr)(NLSTK_DevdScanManager_S *manager, NLSTK_DevdScanner_S *scanner);

typedef struct {
    DevdScanFilterInitPtr devdScanFilterInit;
    DevdScanFilterDeInitPtr devdScanFilterDeInit;
    DevdSetScanfilterSwitchPtr devdSetScanfilterSwitch;
    DevdIsSupportScanFilterPtr devdIsSupportScanFilter;
    DevdSetScanfilterPtr devdSetScanfilter;
    DevdAddChipFiltersPtr devdAddChipFilters;
    DevdDeleteChipFiltersPtr devdDeleteChipFilters;
} Devd_ExtFuncList;

void Devd_RegisterExtFunc(void *soHandle);

void Devd_DeregisterExtFunc(void);

Devd_ExtFuncList *Devd_GetExtFuncList(void);

#ifdef __cplusplus
}
#endif

#endif