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
#include "securec.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "nlstk_schedule.h"
#include "nlstk_icce_def.h"
#include "icce_server.h"
#include "nlstk_icce_server.h"

uint32_t NLSTK_IcceCreateIcceInstance(NLSTK_IcceServiceInfo_S *icceServiceInfo)
{
    NLSTK_CHECK_RETURN(icceServiceInfo != NULL, NLSTK_ERRCODE_FAIL, "[DIS] Invalid device info");
    NLSTK_IcceServiceInfo_S *inIcceInfo = (NLSTK_IcceServiceInfo_S *)SDF_MemZalloc(sizeof(NLSTK_IcceServiceInfo_S));
    NLSTK_CHECK_RETURN(inIcceInfo != NULL, NLSTK_ERRCODE_FAIL, "[DIS] mem alloc failed");
    (void)memcpy_s(inIcceInfo, sizeof(NLSTK_IcceServiceInfo_S), icceServiceInfo, sizeof(NLSTK_IcceServiceInfo_S));
    if (SchedulePostTask(IcceSaveInfo, inIcceInfo, SDF_MemFree) != 0) {
        NLSTK_LOG_ERROR("[ICCE] SchedulPostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }

    if (SchedulePostTask(IcceAddService, NULL, NULL) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[ICCE] SchedulPostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_IcceDestroyIcceInstance(void)
{
    if (SchedulePostTask(IcceRemoveService, NULL, NULL) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[ICCE] SchedulPostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}