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
#include "nlstk_dft.h"
#include "hadm_dft.h"

void HadmDftReportExcep(SLE_Addr_S *addr, uint16_t errCode, uint16_t res)
{
    DftCache(addr, NLSTK_DFT_EVENT_HADM_EXCEP, HADM_DFT_EXCEP_ERR_CODE,
        NLSTK_DFT_PARAM_VALUE_TYPE_UINT16, &errCode);
    DftReport(addr, NLSTK_DFT_EVENT_HADM_EXCEP, HADM_DFT_EXCEP_RES, res);
}