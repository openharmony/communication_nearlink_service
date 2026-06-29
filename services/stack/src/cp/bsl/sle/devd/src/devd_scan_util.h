/****************************************************************************
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
****************************************************************************/

/****************************************************************************
 *
 * this file contains the DEVD SCAN util functions
 *
 ***************************************************************************/

#ifndef DEVD_SCAN_UTIL_H
#define DEVD_SCAN_UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include "sdf_vector.h"
#include "nlstk_devd_scan_type.h"

void DevdFreeDevdScanner(void *ptr);

void DevdFreeStartScanParam(void *ptr);

NLSTK_DevdScanSettingInner_S *FetchFrame1TargetScanSettings(SDF_Vector_S *scanners);

NLSTK_DevdScanSettingInner_S *FetchFrame4TargetScanSettings(SDF_Vector_S *scanners);

bool CheckFiltersLegal(NLSTK_DevdScanFilter_S *filters, uint16_t filtersNum);

SDF_Vector_S *DevdConvertFiltersToVector(NLSTK_DevdScanFilter_S *filters, uint16_t filtersNum);

SDF_Vector_S *DevdCloneFiltersVector(SDF_Vector_S *filters);

#endif  // DEVD_SCAN_UTIL_H