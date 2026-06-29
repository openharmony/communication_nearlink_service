/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef DEVD_SCAN_FILTER_H
#define DEVD_SCAN_FILTER_H

#include "devd_scan_type.h"
#include "nlstk_stm_collab_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

bool DevdIsMatchFilters(NLSTK_DevdAdvResult_S *result, SDF_Vector_S *filters);

void DevdAddFilters(SDF_Vector_S *scanners, NLSTK_DevdScanner_S *scanner, NLSTK_DevdScanSetting_S *setting,
    SDF_Vector_S *filters);

void DevdDeleteFilters(NLSTK_DevdScanManager_S *manager, NLSTK_DevdScanner_S *scanner);

#ifdef __cplusplus
}
#endif
#endif