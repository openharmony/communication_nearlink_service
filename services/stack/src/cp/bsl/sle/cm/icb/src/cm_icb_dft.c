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
#include <stddef.h>
#include "nlstk_dft.h"
#include "cm_icb_dft.h"

void CMIcbDftReportExcep(uint16_t scene, uint8_t id, uint16_t status)
{
    // 当前故障场景无设备上下文、设备信息，仅上报故障场景、故障ID和故障码
    uint16_t icbId = (uint16_t)id;
    DftCache(NULL, NLSTK_DFT_EVENT_ICB_EXCEP, ICB_DFT_EXCEP_SCENE,
        NLSTK_DFT_PARAM_VALUE_TYPE_UINT16, &scene);
    DftCache(NULL, NLSTK_DFT_EVENT_ICB_EXCEP, ICB_DFT_EXCEP_ID,
        NLSTK_DFT_PARAM_VALUE_TYPE_UINT16, &icbId);
    DftReport(NULL, NLSTK_DFT_EVENT_ICB_EXCEP, ICB_DFT_EXCEP_ERRORCODE, status);
}
