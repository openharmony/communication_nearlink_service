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
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "securec.h"
#include "sdf_mem.h"
#include "ssap_utils.h"
#include "nlstk_ssap_app_server.h"
#include "port_server.h"
#include "port_type.h"
#include "nlstk_port_server.h"

NLSTK_Errcode_E NLSTK_PortAddByUuid(NLSTK_SsapUuid_S *uuid, uint16_t manufactureId, uint16_t portId)
{
    NLSTK_CHECK_RETURN(uuid != NULL, NLSTK_ERRCODE_POINTER_NULL, "[PORT] uuid is null");
    PortPrivateInfo_S *param = (PortPrivateInfo_S *)SDF_MemZalloc(sizeof(PortPrivateInfo_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[PORT] param malloc fail");
    (void)memcpy_s(&param->uuid, sizeof(NLSTK_SsapUuid_S), uuid, sizeof(NLSTK_SsapUuid_S));
    param->portId = portId;
    param->manufactureId = manufactureId;
    NLSTK_LOG_INFO("[PORT] add portId %u, manufactureId %u, uuid: %s", portId, manufactureId, SSAP_GET_ENC_UUID(uuid));
    NLSTK_CHECK_RETURN(SchedulePostTask(PortAddByUuidInner, param, SDF_MemFree) == NLSTK_OK,
        NLSTK_ERRCODE_TASK_FAIL, "[PORT] SchedulePostTask failed");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_PortDeleteByUuid(NLSTK_SsapUuid_S *uuid)
{
    NLSTK_CHECK_RETURN(uuid != NULL, NLSTK_ERRCODE_POINTER_NULL, "[PORT] uuid is null");
    NLSTK_SsapUuid_S *copyUuid = (NLSTK_SsapUuid_S *)SDF_MemZalloc(sizeof(NLSTK_SsapUuid_S));
    NLSTK_CHECK_RETURN(copyUuid != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[PORT] copyUuid malloc fail");
    (void)memcpy_s(copyUuid, sizeof(NLSTK_SsapUuid_S), uuid, sizeof(NLSTK_SsapUuid_S));
    NLSTK_CHECK_RETURN(SchedulePostTask(PortDeleteByUuidInner, copyUuid, SDF_MemFree) == NLSTK_OK,
        NLSTK_ERRCODE_TASK_FAIL, "[PORT] SchedulePostTask failed");
    return NLSTK_ERRCODE_SUCCESS;
}