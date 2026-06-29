/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "sle_connect_param.h"
#include "sysdep.h"
#include "dli_cmd_struct.h"

DLI_ConnectionCreateParam g_sleDliConnectParam = { 0 };

void SleDliConnectParamInit(void)
{
    g_sleDliConnectParam.initiatorFilterPolicy = SLE_INITIATOR_FILTER_POLICY;
    g_sleDliConnectParam.initiatingPhys = SLE_INITIATE_PHYS;
    g_sleDliConnectParam.gtNegotiateInd = SLE_NEGOTIATE;
    encode2byte_little(&g_sleDliConnectParam.scanInterval, SLE_SCAN_INTERVAL);
    encode2byte_little(&g_sleDliConnectParam.scanWindow, SLE_SCAN_WINDOW);
    encode2byte_little(&g_sleDliConnectParam.connectionIntervalMin, SLE_CONNECTION_INTERVAL_MIN);
    encode2byte_little(&g_sleDliConnectParam.connectionIntervalMax, SLE_CONNECTION_INTERVAL_MAX);
    encode2byte_little(&g_sleDliConnectParam.supervisionTimeout, SLE_SUPERVISION_TIMEOUT);
}

uint8_t SleGetInitiatorFilterPolicy(void)
{
    return g_sleDliConnectParam.initiatorFilterPolicy;
}

DLI_ConnectionCreateParam *SleGetDliConnectParam(void)
{
    return &g_sleDliConnectParam;
}

void SleSetDliConnectParam(CM_ConnectSetParamReq_S *setParam)
{
    g_sleDliConnectParam.bitFrameType = setParam->bitFrameType;
    g_sleDliConnectParam.initiatorFilterPolicy = setParam->enableFilterPolicy;
    g_sleDliConnectParam.initiatingPhys = setParam->initiatingPhys;
    g_sleDliConnectParam.gtNegotiateInd = setParam->gtNegotiate;
    encode2byte_little(&g_sleDliConnectParam.scanInterval, setParam->scanInterval);
    encode2byte_little(&g_sleDliConnectParam.scanWindow, setParam->scanWindow);
    encode2byte_little(&g_sleDliConnectParam.connectionIntervalMin, setParam->minInterval);
    encode2byte_little(&g_sleDliConnectParam.connectionIntervalMax, setParam->maxInterval);
    encode2byte_little(&g_sleDliConnectParam.supervisionTimeout, setParam->timeout);
}