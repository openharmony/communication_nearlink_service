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
#include "nlstk_log.h"
#include "cm_errno.h"
#include "securec.h"
#include "cm_api.h"
#include "hadm_config_cm.h"

uint32_t HadmSetConnectionParamToCm(SLE_Addr_S *addr, HadmConnectionParam_S *param)
{
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_POINTER_NULL, "param is null");
    CM_ConnectUpdateParamReq_S connParam = { 0 };
    (void)memcpy_s(&(connParam.addr), sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    connParam.version = param->version;
    connParam.localIndex = param->localIndex;
    connParam.intervalMin = param->intervalMin;
    connParam.intervalMax = param->intervalMax;
    connParam.txRxInterval = param->eventIfs;
    connParam.eventInterval = param->eventEfs;
    connParam.maxLatency = param->maxLatency;
    connParam.supervisionTimeout = param->supervisionTimeout;
    connParam.systemTimeUnit = param->systemTimeUint;
    connParam.txRxFlag = param->txRxFlag;
    uint32_t ret = CM_ConnectUpdateParamReq(&connParam);
    NLSTK_CHECK_RETURN(ret == CM_SUCCESS, NLSTK_HADM_ERRCODE_CONFIG_CM_FAIL,
                         "CM_ConnectParamUpdateReq failed, ret:0x%08x", ret);
    return NLSTK_ERRCODE_SUCCESS;
}