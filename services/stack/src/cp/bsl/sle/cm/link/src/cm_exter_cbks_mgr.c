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

#include "cm_exter_cbks_mgr.h"
#include <stddef.h>
#include "securec.h"
#include "cm_errno.h"
#include "cm_common.h"
#include "cm_log.h"

static CM_ConnectCbks_S g_sleCbks = { 0 };

void CM_RegExterCbks(const CM_ConnectCbks_S *cbks)
{
    g_sleCbks = *cbks;
}

void CM_UnRegExterCbks(void)
{
    g_sleCbks.connCancelCbk = NULL;
    g_sleCbks.connRemoteUpdateParamReqCbk = NULL;
    g_sleCbks.connUpdateParamCbk = NULL;
    g_sleCbks.readRemoteFeatureVersionCbk = NULL;
    g_sleCbks.setPhyCbk = NULL;
    g_sleCbks.readLocalFeatureCbk = NULL;
    g_sleCbks.enableConnHighPowerCbk = NULL;
    g_sleCbks.setPeerDevTypeCbk = NULL;
    g_sleCbks.setRxDataFilterCbk = NULL;
    g_sleCbks.setAcbSubrateCbk = NULL;
    g_sleCbks.reqAcbSubrateCbk = NULL;
    g_sleCbks.readAcceptFilterListSizeCbk = NULL;
    g_sleCbks.readRemoteRssiCbk = NULL;
}

void CM_ExecuteEventCbk(uint8_t event, void *param)
{
    CM_LOGD("event:%hhu", event);
    CM_ConnectCbks_S *cbks = &g_sleCbks;
    switch (event) {
        case CM_SLE_CBK_EVENT_CONNECT_CANCEL:
            CM_CbksFunc(cbks, connCancelCbk, (uint8_t *)param);
            break;
        case CM_SLE_CBK_EVENT_CONNECT_REMOTE_UPDATE_PARAM_REQ:
            CM_CbksFunc(cbks, connRemoteUpdateParamReqCbk, (CM_ConnectRemoteUpdateParamReq_S *)param);
            break;
        case CM_SLE_CBK_EVENT_CONNECT_PARAM_UPDATE:
            CM_CbksFunc(cbks, connUpdateParamCbk, (CM_ConnectUpdateParamRsp_S *)param);
            break;
        case CM_SLE_CBK_EVENT_READ_REMOTE_FEATURE_VERSION:
            CM_CbksFunc(cbks, readRemoteFeatureVersionCbk, (CM_ReadRemoteFeatureVersionRsp_S *)param);
            break;
        case CM_SLE_CBK_EVENT_SET_PHY:
            CM_CbksFunc(cbks, setPhyCbk, (CM_SetPhyRsp_S *)param);
            break;
        case CM_SLE_CBK_EVENT_READ_LOCAL_FEATURE:
            CM_CbksFunc(cbks, readLocalFeatureCbk, (CM_ReadLocalFeatureRsp_S *)param);
            break;
        case CM_SLE_CBK_EVENT_ENABLE_CONN_HIGH_POWER:
            CM_CbksFunc(cbks, enableConnHighPowerCbk, (CM_EnableConnHighPowerRsp_S *)param);
            break;
        case CM_SLE_CBK_EVENT_SET_PEER_DEV_TYPE:
            CM_CbksFunc(cbks, setPeerDevTypeCbk, (CM_SetPeerDevType_S *)param);
            break;
        case CM_SLE_CBK_EVENT_SET_RX_DATA_FILTER:
            CM_CbksFunc(cbks, setRxDataFilterCbk, (CM_SetRxDataFilterRsp_S *)param);
            break;
        case CM_SLE_CBK_EVENT_SET_SUBRATE:
            CM_CbksFunc(cbks, setAcbSubrateCbk, (CM_AcbSubrateCbParam_S *)param);
            break;
        case CM_SLE_CBK_EVENT_REQ_SUBRATE:
            CM_CbksFunc(cbks, reqAcbSubrateCbk, (CM_AcbSubrateCbParam_S *)param);
            break;
        case CM_SLE_CBK_EVENT_READ_ACCEPT_FLT_LIST_SIZE:
            CM_CbksFunc(cbks, readAcceptFilterListSizeCbk, (CM_ReadAcceptFilterListSize_S *)param);
            break;
        case CM_SLE_CBK_EVENT_READ_REMOTE_RSSI:
            CM_CbksFunc(cbks, readRemoteRssiCbk, (CM_ReadRemoteRssiRsp_S *)param);
            break;
        default:
            break;
    }
}