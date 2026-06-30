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
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_PortClientDeregCbk(void)
{
    void *p = NULL;
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_PortConnect(const SLE_Addr_S *addr, const NLSTK_ConnParam_S *connParam)
{
    NLSTK_CHECK_RETURN(connParam != NULL, NLSTK_ERRCODE_POINTER_NULL, "[PORT] connParam is null");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_PortDisconnect(const SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[PORT] addr is null");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_PortGetConnectState(const SLE_Addr_S *addr, int *state)
{
    NLSTK_CHECK_RETURN(addr != NULL && state != NULL, NLSTK_ERRCODE_POINTER_NULL, "[PORT] addr or state is null");
    *state = PORT_DISCONNECTED; // 初始化为断连状态
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_PortGetDevicePortIdByUuid(const SLE_Addr_S *addr, NLSTK_SsapUuid_S *uuid, uint16_t *portId)
{
    NLSTK_CHECK_RETURN(addr != NULL && uuid != NULL && portId != NULL, NLSTK_ERRCODE_POINTER_NULL,
        "[PORT] param is null");
    *portId = 30300;
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_PortGetConnectDeviceNum(const SLE_Addr_S *addr, int *portConnectNum)
{
    NLSTK_CHECK_RETURN(addr != NULL && portConnectNum != NULL, NLSTK_ERRCODE_POINTER_NULL, "[PORT] param is null");
    *portConnectNum = 0; // 初始化为0
    return NLSTK_ERRCODE_SUCCESS;
}