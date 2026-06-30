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
#include "nlstk_bas_client.h"
#include "bas_type.h"
#include "nlstk_public_define.h"
#include "bas_def.h"

NLSTK_Errcode_E NLSTK_BasRegisterCallBack(BasClientCallBack_S *clientCallback)
{
    NLSTK_CHECK_RETURN(clientCallback != NULL, NLSTK_ERRCODE_POINTER_NULL, "[BAS] clientCallback is null");
    BasClientCallBack_S *cbk = (BasClientCallBack_S *)SDF_MemZalloc(sizeof(BasClientCallBack_S));
    NLSTK_CHECK_RETURN(cbk != NULL, NLSTK_ERRCODE_POINTER_NULL, "[BAS] cbk is null");
    (void)memcpy_s(cbk, sizeof(BasClientCallBack_S), clientCallback, sizeof(BasClientCallBack_S));
    if (SchedulePostTask(BasRegClientCbk, cbk, SDF_MemFree) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[BAS] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_BasProfileConnect(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[BAS] NLSTK_BasProfileConnect: addr is null");
    SLE_Addr_S *addrIn = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(addrIn != NULL, NLSTK_ERRCODE_POINTER_NULL, "[BAS] addr is null");
    (void)memcpy_s(addrIn, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(BasConnectTask, addrIn, SDF_MemFree) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[BAS] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_BasProfileDisconnect(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[BAS] NLSTK_BasProfileDisconnect: addr is null");
    SLE_Addr_S *addrIn = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(addrIn != NULL, NLSTK_ERRCODE_POINTER_NULL, "[BAS] addr is null");
    (void)memcpy_s(addrIn, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(BasDisconnectTask, addrIn, SDF_MemFree) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[BAS] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_GetConnectedBasDeviceNum(uint8_t *num)
{
    NLSTK_CHECK_RETURN(num != NULL, NLSTK_ERRCODE_POINTER_NULL, "[BAS] NLSTK_GetConnectedBasDeviceNum: addr is null");
    //该指针生命周期由外部管理
    uint32_t ret = SchedulePostTaskBlocked(BasCountConnectedDevicesTask, num, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_GetBatteryLevel(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_FAIL, "[BAS] NLSTK_BasGetBatteryLevel: addr is null");
    SLE_Addr_S *addrIn = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(addrIn != NULL, NLSTK_ERRCODE_POINTER_NULL, "[BAS] addr is null");
    (void)memcpy_s(addrIn, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(BasGetBatteryLevelTask, addrIn, SDF_MemFree) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[BAS] SchedulePostTask BasGetBatteryLevel failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}