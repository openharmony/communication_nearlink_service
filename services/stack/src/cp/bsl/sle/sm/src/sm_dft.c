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
#include "sdf_addr.h"

#include "sm_stm.h"
#include "sm_struct.h"
#include "sm_dft.h"

NLSTK_DftEventId_E SmDftFindEvent(SmState_E curState, SmNodeType_E nodeType)
{
    if (nodeType == SM_G_NODE && curState == SM_STATE_NEGO) {
        return NLSTK_DFT_EVENT_SM_G_NEGO_EXCEP;
    } else if (nodeType == SM_T_NODE && curState == SM_STATE_NEGO) {
        return NLSTK_DFT_EVENT_SM_T_NEGO_EXCEP;
    } else if (nodeType == SM_G_NODE && curState == SM_STATE_AUTH) {
        return NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP;
    } else if (nodeType == SM_T_NODE && curState == SM_STATE_AUTH) {
        return NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP;
    } else if (curState == SM_STATE_ENCP) {
        return NLSTK_DFT_EVENT_SM_ENCP_EXCEP;
    }
    return NLSTK_DFT_EVENT_MAX;
};

void SmDftCacheU16(SLE_Addr_S *addr, NLSTK_DftEventId_E eventId, uint16_t paramId, uint16_t param)
{
    DftCache(addr, eventId, paramId, NLSTK_DFT_PARAM_VALUE_TYPE_UINT16, &param);
}

void SmDftCacheTimestamp(SLE_Addr_S *addr, NLSTK_DftEventId_E eventId, uint16_t paramId)
{
    DftCacheTimestamp(addr, eventId, paramId);
}

void SmDftReport(SLE_Addr_S *addr, SmState_E curState, SmNodeType_E nodeType, uint16_t errVal)
{
    NLSTK_DftEventId_E eventId = SmDftFindEvent(curState, nodeType);
    uint16_t errParamId;
    uint16_t resParamId;
    // 此处eventId固定，不会增加，不做表驱动
    switch (eventId) {
        case NLSTK_DFT_EVENT_SM_G_NEGO_EXCEP:
            errParamId = SM_DFT_G_NEGO_ERR_CODE;
            resParamId = SM_DFT_G_NEGO_RES;
            break;
        case NLSTK_DFT_EVENT_SM_T_NEGO_EXCEP:
            errParamId = SM_DFT_T_NEGO_ERR_CODE;
            resParamId = SM_DFT_T_NEGO_RES;
            break;
        case NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP:
            errParamId = SM_DFT_G_AUTH_ERR_CODE;
            resParamId = SM_DFT_G_AUTH_RES;
            break;
        case NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP:
            errParamId = SM_DFT_T_AUTH_ERR_CODE;
            resParamId = SM_DFT_T_AUTH_RES;
            break;
        case NLSTK_DFT_EVENT_SM_ENCP_EXCEP:
            errParamId = SM_DFT_ENCP_ERR_CODE;
            resParamId = SM_DFT_ENCP_RES;
            break;
        default:
            return;
    }
    DftCache(addr, eventId, errParamId, NLSTK_DFT_PARAM_VALUE_TYPE_UINT16, &errVal);
    DftReport(addr, eventId, resParamId, SM_DFT_RES_FAIL);
}