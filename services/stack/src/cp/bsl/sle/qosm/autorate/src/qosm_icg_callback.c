/****************************************************************************
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
****************************************************************************/

#include "qosm_icg_callback.h"
#include "qosm_errno.h"
#include "qosm_log.h"
#include "qosm_table_mgr.h"
#include "sdf_mem.h"
#include "securec.h"
#include "qosm_audio_dfx.h"
#include "nlstk_cfgdb_api.h"

static QOSM_AutoRateCallback g_autoRateCallback = {NULL};

void QOSM_NotifyParamChangedSuccessCb(QOSM_ICGInfo *icgInfo, QOSM_ParamState state)
{
    QOSM_ParamCb changedParam = {};
    changedParam.qosId = icgInfo->qosId;
    changedParam.state = state;
    changedParam.result = QOSM_SUCCESS;
    changedParam.isIMG = QOSM_GetICBTypeByIndex(icgInfo->qosIndex);
    if (changedParam.isIMG) {
        changedParam.gHandle = icgInfo->gHandle;
    }
    changedParam.linkCnt = icgInfo->linkCnt;
    uint16_t connHandle[QOSM_AUTORATE_MAX_LINK_CNT] = {};
    changedParam.connHandle = connHandle;
    for (uint8_t i = 0; i < icgInfo->linkCnt && i < QOSM_AUTORATE_MAX_LINK_CNT; i++) {
        connHandle[i] = icgInfo->link[i].connHandle;
    }
    if (g_autoRateCallback.paramChangedCbk != NULL) {
        g_autoRateCallback.paramChangedCbk(&changedParam);
    }
}

void QOSM_NotifyParamChangedFailCb(uint8_t qosId, QOSM_ParamState state)
{
    QOSM_CHECK_RETURN(g_autoRateCallback.paramChangedCbk != NULL, "param changed callback is null");
    QOSM_ParamCb changedParam = {};
    changedParam.qosId = qosId;
    changedParam.state = state;
    changedParam.result = QOSM_FAIL;
    g_autoRateCallback.paramChangedCbk(&changedParam);
}

void QOSM_NotifyConnFailCbk(uint8_t qosId, QOSM_ConnectionState state, uint16_t *connHandle, uint8_t linkCnt)
{
    QOSM_CHECK_RETURN(g_autoRateCallback.connChangedCbk != NULL, "conn changed callback is null");
    QOSM_CHECK_RETURN(linkCnt != 0 && linkCnt <= QOSM_AUTORATE_MAX_LINK_CNT, "link count: %u is invalid", linkCnt);
    QOSM_ConnParam link[QOSM_AUTORATE_MAX_LINK_CNT] = {};
    for (uint8_t i = 0; i < linkCnt && i < QOSM_AUTORATE_MAX_LINK_CNT; i++) {
        link[i].connHandle = connHandle[i];
    }
    QOSM_ConnParamCb param = {};
    param.qosId = qosId;
    param.state = state;
    param.result = QOSM_FAIL;
    param.linkCnt = linkCnt;
    param.link = link;
    QOSM_LOGE("notify qos id %hhu conn failed", qosId);
    g_autoRateCallback.connChangedCbk(&param);
}

void QOSM_NotifyConnCbk(QOSM_ICGInfo *icgInfo, QOSM_ConnectionState state, uint32_t result,
    QOSM_ConnParam *link, uint8_t linkCnt)
{
    QOSM_CHECK_RETURN(g_autoRateCallback.connChangedCbk != NULL, "conn changed callback is null");
    QOSM_CHECK_RETURN(linkCnt != 0 && linkCnt <= QOSM_AUTORATE_MAX_LINK_CNT, "link count: %u is invalid", linkCnt);
    QOSM_ConnParamCb connParam = {};
    connParam.qosId = icgInfo->qosId;
    connParam.state = state;
    connParam.result = result;
    connParam.isIMG = QOSM_GetICBTypeByIndex(icgInfo->qosIndex) == CM_IMB;
    connParam.gHandle = icgInfo->gHandle;
    connParam.linkCnt = linkCnt;
    connParam.link = link;
    if (icgInfo->qosParam != NULL) {
        connParam.bitrate = QOSM_GetOriginalBitrate(icgInfo->icbType, icgInfo->qosParam->downwardBitrate,
            icgInfo->linkCnt);
    }
    QOSM_LOGE("notify qos id %hhu conn cbk, state: %u, result: %u, bitrate: %u",
        icgInfo->qosId, state, result, connParam.bitrate);
    g_autoRateCallback.connChangedCbk(&connParam);
}

void QOSM_NotifyDataPathChangedCb(uint8_t qosId, QOSM_DataPathState state,
    uint32_t result, uint16_t connHandle, uint8_t direction)
{
    QOSM_CHECK_RETURN(g_autoRateCallback.dataPathChangedCbk != NULL, "datapath changed callback is null");
    QOSM_DataPathParamCb param = {};
    param.qosId = qosId;
    param.state = state;
    param.result = result;
    param.connHandle = connHandle;
    param.direction = direction;
    g_autoRateCallback.dataPathChangedCbk(&param);
}

static uint8_t QOSM_GetAvailableBitratesList(const QOSM_ICGInfo *icgInfo, uint16_t *availableBitrates)
{
    QOSM_CHECK_RETURN_RET(icgInfo != NULL && icgInfo->level != NULL, 0, "icgInfo is null or icgInfo->level is null");
    QOSM_CHECK_RETURN_RET(icgInfo->totalLevelCnt > 0, 0, "totalLevelCnt is 0");
    QOSM_CHECK_RETURN_RET(availableBitrates != NULL, 0, "availableBitrates is null");
    uint8_t count = 0;
    for (int i = icgInfo->totalLevelCnt - 1; i >= 0; i--) {
        if (icgInfo->level[i].isAvailable && count < QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT) {
            availableBitrates[count] = QOSM_GetOriginalBitrate(icgInfo->icbType,
                icgInfo->level[i].qosParam->downwardBitrate, icgInfo->linkCnt);
            count++;
        }
    }
    return count;
}

void QOSM_NotifyReportedBitrateChangedCb(const QOSM_ICGInfo *icgInfo, uint16_t reportedDirection,
    uint8_t labelId, const QOSM_LinkParam *qosParam)
{
    QOSM_CHECK_RETURN(g_autoRateCallback.bitrateChangedCbk != NULL, "bitrate changed callback is null");
    QOSM_CHECK_RETURN(icgInfo->linkCnt != 0, "link count is 0");
    QOSM_BitrateParamCb param = {};
    param.qosId = icgInfo->qosId;
    param.direction = reportedDirection;
    param.labelId = labelId; // need by DSP to control chip when LEVEL_DOWN
    param.downwardBitrate = QOSM_GetOriginalBitrate(icgInfo->icbType, qosParam->downwardBitrate,
        icgInfo->linkCnt);
    param.upwardBitrate = QOSM_GetOriginalBitrate(icgInfo->icbType, qosParam->upwardBitrate,
        icgInfo->linkCnt);
    param.qosIndex = icgInfo->qosIndex;
    param.qosLevel = qosParam->qosLevel;
    param.dutyCycle = qosParam->dutyCycle;
    param.availableBitratesCnt = QOSM_GetAvailableBitratesList(icgInfo, param.availableBitrates);

    // notify ASC and DSP change bitrate here
    g_autoRateCallback.bitrateChangedCbk(&param, 1);
    QOSM_AudioDfxUpdateBitrate(qosParam->downwardBitrate);
}

void QOSM_SendAutoRateMsg(const QOSM_ICGInfo *icgInfo, uint16_t direction, uint8_t labelId,
    uint8_t msgType, uint32_t result)
{
    QOSM_LOGI("Send direction = %d, msgType = %d, result = %d", direction, msgType, result);
    QOSM_CHECK_RETURN(g_autoRateCallback.callBitrateUpDownCbk != NULL, "call bitrate updown callback is null");
    QOSM_CHECK_RETURN(icgInfo->linkCnt != 0, "link count is 0");

    QOSM_AutoRateSendMsgCb param = {};
    param.qosId = icgInfo->qosId;
    param.linkCnt = icgInfo->linkCnt;
    for (uint8_t i = 0; i < icgInfo->linkCnt && i < QOSM_AUTORATE_MAX_LINK_CNT; i++) {
        param.lcid[i] = icgInfo->link[i].lcid;
    }
    param.direction = direction;
    param.labelId = labelId;
    param.qosIndex = icgInfo->qosIndex;
    if ((icgInfo->updateStatus & UPDATE_BY_CALL) != 0) {
        param.downwardBitrate = QOSM_GetOriginalBitrate(icgInfo->icbType, icgInfo->updatedQosParam->downwardBitrate,
            icgInfo->linkCnt);
        param.upwardBitrate = QOSM_GetOriginalBitrate(icgInfo->icbType, icgInfo->updatedQosParam->upwardBitrate,
            icgInfo->linkCnt);
    } else if ((icgInfo->updateStatus & UPDATE_BY_REPORT) != 0) {
        param.downwardBitrate = QOSM_GetOriginalBitrate(icgInfo->icbType, icgInfo->reportedQosParam->downwardBitrate,
            icgInfo->linkCnt);
        param.upwardBitrate = QOSM_GetOriginalBitrate(icgInfo->icbType, icgInfo->reportedQosParam->upwardBitrate,
            icgInfo->linkCnt);
    } else {
        QOSM_LOGE("not change bitrate, no need to send msg");
        return;
    }
    param.msgType = msgType;
    param.result = result;
    g_autoRateCallback.callBitrateUpDownCbk(&param);
}

bool QOSM_AutorateIsBitrateChangeCbkValid(void)
{
    return g_autoRateCallback.bitrateChangedCbk != NULL;
}

void QOSM_AutorateUplayerCallbackInit(const QOSM_AutoRateCallback *callback)
{
    g_autoRateCallback = *callback;
}

void QOSM_AutorateUplayerCallbackUninit(void)
{
    (void)memset_s(&g_autoRateCallback, sizeof(g_autoRateCallback), 0, sizeof(g_autoRateCallback));
}