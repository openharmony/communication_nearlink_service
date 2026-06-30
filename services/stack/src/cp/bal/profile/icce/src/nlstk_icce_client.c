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
#include "nlstk_icce_client.h"
#include "nlstk_public_define.h"
#include "nlstk_icce_def.h"
#include "icce_stm.h"
#include "nlstk_schedule.h"
#include "ssap_app_link.h"
#include "ssapc_client_api.h"
#include "icce_client.h"
#include "nlstk_log.h"

uint32_t NLSTK_IcceRegisterReadInfoCallBack(NLSTK_IcceClientCallBack_S *clientCallback)
{
    NLSTK_CHECK_RETURN(clientCallback != NULL, NLSTK_ERR, "[ICCE] clientCallback is null");
    NLSTK_IcceClientCallBack_S *cbk = (NLSTK_IcceClientCallBack_S *)SDF_MemZalloc(sizeof(NLSTK_IcceClientCallBack_S));
    NLSTK_CHECK_RETURN(cbk != NULL, NLSTK_ERR, "[ICCE] cbk is null");
    (void)memcpy_s(cbk, sizeof(NLSTK_IcceClientCallBack_S), clientCallback, sizeof(NLSTK_IcceClientCallBack_S));
    if (SchedulePostTaskBlocked(IcceRegClientCbk, cbk, SDF_MemFree, NLSTK_API_TIME_OUT) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[ICCE] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_IcceConnect(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERR, "[ICCE] NLSTK_IcceConnect: addr is null");
    SLE_Addr_S *addrIn = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(addrIn != NULL, NLSTK_ERR, "[ICCE] addr is null");
    (void)memcpy_s(addrIn, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(IcceConnectTask, addrIn, SDF_MemFree) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[ICCE] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_IcceDisconnect(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERR, "[ICCE] NLSTK_IcceDisconnect: addr is null");
    SLE_Addr_S *addrIn = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(addrIn != NULL, NLSTK_ERR, "[ICCE] addr is null");
    (void)memcpy_s(addrIn, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(IcceDisconnectTask, addrIn, SDF_MemFree) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[ICCE] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_IcceGetPort(SLE_Addr_S *addr, int32_t *port)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_FAIL, "[ICCE] NLSTK_IcceGetPort: addr is null");
    IcceReadInfo_S *readInfo = (IcceReadInfo_S *)SDF_MemZalloc(sizeof(IcceReadInfo_S));
    NLSTK_CHECK_RETURN(readInfo != NULL, NLSTK_ERRCODE_FAIL, "[ICCE] NLSTK_IcceGetPort: readInfo is null");
    readInfo->port = port;
    readInfo->addr = addr;
    if (SchedulePostTaskBlocked(IcceReadIcceInfoTask, readInfo, SDF_MemFree, NLSTK_API_TIME_OUT)
        != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[ICCE] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint8_t NLSTK_GetConnectionsDeviceNum(void)
{
    uint8_t num = 0;
    if (SchedulePostTaskBlocked(IcceCountConnectedDevicesTask, (void *)&num, NULL, NLSTK_API_TIME_OUT)
        != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[ICCE] SchedulePostTask failed");
        return 0;
    }
    return num;
}