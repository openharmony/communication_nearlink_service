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
#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "securec.h"
#include "sdf_mem.h"
#include "ssap_type.h"
#include "port_client.h"
#include "port_type.h"
#include "nlstk_port_client.h"

NLSTK_Errcode_E NLSTK_PortClientRegCbk(NLSTK_PortClientCallBack_S *clientCallback)
{
    NLSTK_CHECK_RETURN(clientCallback != NULL, NLSTK_ERRCODE_POINTER_NULL, "[PORT] cbk is null");
    NLSTK_PortClientCallBack_S *copyCbk =
        (NLSTK_PortClientCallBack_S *)SDF_MemZalloc(sizeof(NLSTK_PortClientCallBack_S));
    NLSTK_CHECK_RETURN(copyCbk != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[PORT] cbk malloc fail");
    (void)memcpy_s(copyCbk, sizeof(NLSTK_PortClientCallBack_S), clientCallback, sizeof(NLSTK_PortClientCallBack_S));
    NLSTK_CHECK_RETURN(SchedulePostTask(PortClientRegCbkInner, copyCbk, SDF_MemFree) == NLSTK_OK,
        NLSTK_ERRCODE_TASK_FAIL, "[PORT] SchedulePostTask failed");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_PortClientDeregCbk(void)
{
    void *p = NULL;
    NLSTK_CHECK_RETURN(SchedulePostTask(PortClientDeregCbkInner, p, NULL) == NLSTK_OK,
        NLSTK_ERRCODE_TASK_FAIL, "[PORT] SchedulePostTask failed");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_PortConnect(const SLE_Addr_S *addr, const NLSTK_ConnParam_S *connParam)
{
    NLSTK_CHECK_RETURN(connParam != NULL, NLSTK_ERRCODE_POINTER_NULL, "[PORT] connParam is null");
    NLSTK_ConnAddrParam_S *copyParam = (NLSTK_ConnAddrParam_S *)SDF_MemZalloc(sizeof(NLSTK_ConnAddrParam_S));
    NLSTK_CHECK_RETURN(copyParam != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[PORT] connParam malloc fail");
    (void)memcpy_s(&copyParam->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(&copyParam->param, sizeof(NLSTK_ConnParam_S), connParam, sizeof(NLSTK_ConnParam_S));
    NLSTK_CHECK_RETURN(SchedulePostTask(PortConnectInner, copyParam, SDF_MemFree) == NLSTK_OK,
        NLSTK_ERRCODE_TASK_FAIL, "[PORT] SchedulePostTask failed");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_PortDisconnect(const SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[PORT] addr is null");
    SLE_Addr_S *copyAddr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(copyAddr != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[PORT] addr malloc fail");
    (void)memcpy_s(copyAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(SchedulePostTask(PortDisconnectInner, copyAddr, SDF_MemFree) == NLSTK_OK,
        NLSTK_ERRCODE_TASK_FAIL, "[PORT] SchedulePostTask failed");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_PortGetConnectState(const SLE_Addr_S *addr, int *state)
{
    NLSTK_CHECK_RETURN(addr != NULL && state != NULL, NLSTK_ERRCODE_POINTER_NULL, "[PORT] addr or state is null");
    *state = PORT_DISCONNECTED; // 初始化为断连状态
    PortGetConnStateParam_S *param = (PortGetConnStateParam_S *)SDF_MemZalloc(sizeof(PortGetConnStateParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[PORT] param malloc fail");
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    param->state = PORT_DISCONNECTED;
    // 同步接口的内存，由调用点释放
    uint32_t ret = SchedulePostTaskBlocked(PortGetConnectStateInner, (void *)param, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        // 在异常场景下，无法再处理，因此不判断返回值，此时param的内存会由SDF_MemFree释放
        (void)SchedulePostTask(NULL, (void *)param, SDF_MemFree);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_OK) {
        SDF_MemFree(param);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *state = param->state;
    SDF_MemFree(param);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_PortGetDevicePortIdByUuid(const SLE_Addr_S *addr, NLSTK_SsapUuid_S *uuid, uint16_t *portId)
{
    NLSTK_CHECK_RETURN(addr != NULL && uuid != NULL && portId != NULL, NLSTK_ERRCODE_POINTER_NULL,
        "[PORT] param is null");
    *portId = 0; // 初始化为非法值
    PortGetPortIdParam_S *param = (PortGetPortIdParam_S *)SDF_MemZalloc(sizeof(PortGetPortIdParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[PORT] param malloc fail");
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(&param->uuid, sizeof(NLSTK_SsapUuid_S), uuid, sizeof(NLSTK_SsapUuid_S));
    param->portId = 0;
    // 同步接口的内存，由调用点释放
    uint32_t ret = SchedulePostTaskBlocked(PortGetDevicePortIdByUuidInner, (void *)param, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        // 在异常场景下，无法再处理，因此不判断返回值，此时param的内存会由SDF_MemFree释放
        (void)SchedulePostTask(NULL, (void *)param, SDF_MemFree);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_OK || param->portId == 0) {
        SDF_MemFree(param);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *portId = param->portId;
    SDF_MemFree(param);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_PortGetConnectDeviceNum(const SLE_Addr_S *addr, int *portConnectNum)
{
    NLSTK_CHECK_RETURN(addr != NULL && portConnectNum != NULL, NLSTK_ERRCODE_POINTER_NULL, "[PORT] param is null");
    *portConnectNum = 0; // 初始化为0
    PortGetConnDevNumParam_S *param = (PortGetConnDevNumParam_S *)SDF_MemZalloc(sizeof(PortGetConnDevNumParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[PORT] param malloc fail");
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    param->portConnectNum = 0;
    // 同步接口的内存，由调用点释放
    uint32_t ret = SchedulePostTaskBlocked(PortGetConnectDeviceNumInner, (void *)param, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        // 在异常场景下，无法再处理，因此不判断返回值，此时param的内存会由SDF_MemFree释放
        (void)SchedulePostTask(NULL, (void *)param, SDF_MemFree);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_OK) {
        SDF_MemFree(param);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *portConnectNum = param->portConnectNum;
    SDF_MemFree(param);
    return NLSTK_ERRCODE_SUCCESS;
}