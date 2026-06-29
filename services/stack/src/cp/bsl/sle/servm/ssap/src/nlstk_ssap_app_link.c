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
#include "nlstk_public_define.h"
#include "ssapc_app.h"
#include "ssap_app_link.h"
#include "securec.h"
#include "sdf_mem.h"
#include "nlstk_schedule.h"
#include "nlstk_log.h"

NLSTK_Errcode_E NLSTK_SsapClientConnect(int32_t appId)
{
    NLSTK_CHECK_RETURN(IsAppIdValid(appId), NLSTK_ERRCODE_PARAM_ERR, "appId(%d) is invalid in SsapClientConnect",
                         appId);
    int32_t *param = (int32_t *)SDF_MemZalloc(sizeof(int32_t));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "memory alloc error");
    *param = appId;
    if (SchedulePostTask(SsapClientConnect, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("CP_PostTask failed in NLSTK_SsapClientConnect function");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientDisconnect(int32_t appId)
{
    NLSTK_CHECK_RETURN(IsAppIdValid(appId), NLSTK_ERRCODE_PARAM_ERR, "appId(%d) is invalid in SsapClientDisConnect",
                         appId);
    int32_t *param = (int32_t *)SDF_MemZalloc(sizeof(int32_t));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "memory alloc error");
    *param = appId;
    if (SchedulePostTask(SsapClientDisconnect, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("CP_PostTask failed in NLSTK_SsapClientDisconnect function");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

