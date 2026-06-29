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
#include "sdf_mem.h"
#include "nlstk_log.h"
#include "nlstk_public_define.h"
#include "nlstk_schedule.h"
#include "securec.h"
#include "micp_client.h"
#include "micp_callback.h"
#include "nlstk_micp_client.h"

uint32_t NLSTK_MicpConnect(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MICP] addr is null");
    SLE_Addr_S *addrIn = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(addrIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MICP] malloc failed");
    (void)memcpy_s(addrIn, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(MicpConnectTask, addrIn, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MICP] post task failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_MicpDisconnect(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MICP] addr is null");
    SLE_Addr_S *addrIn = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(addrIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MICP] malloc failed");
    (void)memcpy_s(addrIn, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(MicpDisconnectTask, addrIn, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MICP] post task failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_MicpRegisterCallback(NLSTK_MicpCbk_S *cbk)
{
    NLSTK_CHECK_RETURN(cbk != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MICP] cbk is null");
    NLSTK_MicpCbk_S *cbkIn = (NLSTK_MicpCbk_S *)SDF_MemZalloc(sizeof(NLSTK_MicpCbk_S));
    NLSTK_CHECK_RETURN(cbkIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MICP] malloc failed");
    cbkIn->eventCbk = cbk->eventCbk;
    cbkIn->micStateCbk = cbk->micStateCbk;
    if (SchedulePostTask(MicpRegCallback, cbkIn, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MICP] post task failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}