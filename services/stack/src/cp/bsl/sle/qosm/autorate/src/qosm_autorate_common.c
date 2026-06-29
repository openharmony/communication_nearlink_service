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

/****************************************************************************
 *
 * this file contains common functions shared between autorate modules.
 *
 ***************************************************************************/

#include "qosm_autorate_common.h"

#include "qosm_autorate_notify.h"
#include "qosm_audio_dfx.h"
#include "qosm_log.h"
#include "qosm_icg_mgr.h"
#include "qosm_icg_callback.h"
#include "qosm_errno.h"
#include "cm_icb_api.h"
#include "cm_errno.h"


static bool g_dspIsOn = false;

void QOSM_LevelDownProc(QOSM_ICGInfo *icgInfo, bool isByReport)
{
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is null");
        return;
    }
    uint16_t direction = isByReport ? icgInfo->reportedDirection : icgInfo->updatedDirection;
    QOSM_LinkParam *qosParam = isByReport ? icgInfo->reportedQosParam : icgInfo->updatedQosParam;
    uint8_t labelId = isByReport ? icgInfo->reportedLabelId : icgInfo->updatedLabelId;
    if (qosParam == NULL) {
        QOSM_LOGE("qosParam is null");
        return;
    }
    QOSM_LOGI("notify bitrate decreased, down bitrate %huKbps, up bitrate %huKbps, duty cycle %hu, "
        "target label id: %hhu", qosParam->downwardBitrate, qosParam->upwardBitrate, qosParam->dutyCycle, labelId);
    // 通话降码率，通知调用方降对端码率
    if (QOSM_GetICBTypeByIndex(icgInfo->qosIndex) == CM_IMB && icgInfo->isSupportFrame4) {
        QOSM_LOGI("voice call case, notify peer and local dsp level down bitrate");
        QOSM_SendAutoRateMsg(icgInfo, direction, labelId, CHANGE_PEER_BITRATE_REQ, QOSM_SUCCESS);
    }
    QOSM_NotifyReportedBitrateChangedCb(icgInfo, direction, labelId, qosParam);
    QOSM_ChangeLabelSetTimer(&icgInfo->changeLableTimerId, icgInfo->qosId, direction);
}

bool QOSM_LevelUpProc(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL || icgInfo->reportedQosParam == NULL) {
        QOSM_LOGE("icgInfo or reportedQosParam is null");
        return false;
    }
    QOSM_LOGI("notify bitrate upgraded, down bitrate %huKbps, up bitrate %huKbps, duty cycle %hu, "
        "target label id: %hhu", icgInfo->reportedQosParam->downwardBitrate, icgInfo->reportedQosParam->upwardBitrate,
        icgInfo->reportedQosParam->dutyCycle, icgInfo->reportedLabelId);
    // 通话码率自适应: 升码率，通知调用方切帧格式，等待帧格式切换成功后，再切同步链路label id
    if (QOSM_GetICBTypeByIndex(icgInfo->qosIndex) == CM_IMB && icgInfo->isSupportFrame4) {
        QOSM_LOGI("voice call case, notify asc change framType and wait for rsp");
        QOSM_SendAutoRateMsg(icgInfo, icgInfo->reportedDirection, icgInfo->reportedLabelId,
            CHANGE_FRAME_TYPE_REQ, QOSM_SUCCESS);
        return true;
    }
    uint32_t ret = QOSM_UpdateLabelId(icgInfo);
    if (ret != QOSM_SUCCESS) {
        QOSM_ResetReportParam(icgInfo);
        QOSM_LOGE("update qos param failed");
        return false;
    }
    QOSM_ChangeLabelSetTimer(&icgInfo->changeLableTimerId, icgInfo->qosId, icgInfo->reportedDirection);
    return true;
}

uint32_t QOSM_UpdateLabelId(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is null");
        return QOSM_FAIL;
    }
    if (icgInfo->connectedCnt == 0 || icgInfo->connectedCnt > QOSM_AUTORATE_MAX_LINK_CNT) {
        QOSM_LOGI("icg: %hhu no connected icb", icgInfo->icgId);
        return QOSM_FAIL;
    }
    uint16_t connHandle[icgInfo->connectedCnt];
    for (uint8_t i = 0, index = 0; i < icgInfo->linkCnt && index < icgInfo->connectedCnt; i++) {
        if (!QOSM_IsICBConnected(&icgInfo->link[i])) {
            continue;
        }
        connHandle[index++] = icgInfo->link[i].connHandle;
    }
    CM_ICGUpdatedParam updatedParam = {};
    updatedParam.type = icgInfo->icbType;
    updatedParam.id = icgInfo->icgId;
    updatedParam.labelId = icgInfo->reportedLabelId;
    updatedParam.icbCnt = icgInfo->connectedCnt;
    updatedParam.connHandle = connHandle;

    // notify chip change labelId here
    uint32_t ret = CM_ICGUpdateParam(&updatedParam);
    return ret == CM_SUCCESS ? QOSM_SUCCESS : QOSM_FAIL;
}

void QOSM_ResetReportParam(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is null");
        return;
    }
    QOSM_ICBUpLevelDelayStopTimer(icgInfo);
    icgInfo->reportedQosParam = NULL;
    icgInfo->reportedDirection = LEVEL_NONE;
    icgInfo->updateStatus &= ~UPDATE_BY_REPORT;
}

void QOSM_ICBQualityReportCheck(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL || icgInfo->qosParam == NULL || icgInfo->reportedQosParam == NULL) {
        QOSM_LOGE("icgInfo or qosParam or reportedQosParam is null");
        return;
    }
    QOSM_LOGI("icgInfo->qosParam->downwardBitrate = %d, icgInfo->reportedQosParam->downwardBitrate = %d",
        icgInfo->qosParam->downwardBitrate, icgInfo->reportedQosParam->downwardBitrate);
    if (icgInfo->reportedDirection == LEVEL_NONE ||
        icgInfo->qosParam->downwardBitrate == icgInfo->reportedQosParam->downwardBitrate) {
        QOSM_LOGD("reportedDirection: %hu, qos downwardBitrate: %hu, reported downwardBitrate: %hu",
            icgInfo->reportedDirection, icgInfo->qosParam->downwardBitrate, icgInfo->reportedQosParam->downwardBitrate);
        QOSM_ResetReportParam(icgInfo);
        return;
    }
    icgInfo->updateStatus |= UPDATE_BY_REPORT;

    // 降码率：回调通知DSP，DSP收到通知后执行降码率，执行后本模块会收到update event，
    // 等icg底下所有建链的链路都收到event了再更新label
    if (icgInfo->reportedDirection == LEVEL_DOWN) {
        QOSM_LevelDownProc(icgInfo, true);
        return;
    }

    // 升码率：通知芯片更新参数，更新成功会收到update event，等icg底下所有建链的链路都收到event了再回调通知
    QOSM_LevelUpProc(icgInfo);
}

bool QOSM_IsAutorateSupported(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is null");
        return false;
    }

    if (!QOSM_AutorateIsDspOn()) {
        QOSM_LOGD("dsp is off, not support autorate");
        return false;
    }

    if (!icgInfo->supportAutorate) {
        return false;
    }

    if (icgInfo->qosIndex != QOSM_QOSINDEX_AUDIO && icgInfo->qosIndex != QOSM_QOSINDEX_LOW_LATENCY
        && icgInfo->qosIndex != QOSM_QOSINDEX_SPATIAL_AUDIO && icgInfo->qosIndex != QOSM_QOSINDEX_OTHERS &&
        icgInfo->qosIndex != QOSM_QOSINDEX_HD_RECORDING &&
        !(icgInfo->qosIndex == QOSM_QOSINDEX_CALL && icgInfo->isSupportFrame4)) {
        QOSM_LOGD("ignore qos index: %u, qosId: %hhu", icgInfo->qosIndex, icgInfo->qosId);
        return false;
    }

    return true;
}

bool QOSM_AutorateIsDspOn(void)
{
    return g_dspIsOn;
}

void QOSM_AutorateSetDspOn(bool on)
{
    g_dspIsOn = on;
}

void QOSM_AutorateUpdateDspStatus(void)
{
    QOSM_AudioDfxGetDspStatusInner((void *)&g_dspIsOn);
}

void QOSM_ResetUpdateParam(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is null");
        return;
    }
    icgInfo->updatedQosParam = NULL;
    icgInfo->updatedDirection = LEVEL_NONE;
    icgInfo->updateStatus &= ~UPDATE_BY_CALL;
}

bool QOSM_RollbackWhenDownLevel(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is null");
        return false;
    }
    if (!QOSM_GetLabelId(icgInfo, icgInfo->qosParam, &icgInfo->reportedLabelId)) {
        QOSM_LOGE("qos id: %hhu, rollback, get reported label id failed", icgInfo->qosId);
        return false;
    }
    icgInfo->updateStatus |= UPDATE_BY_REPORT;
    icgInfo->reportedQosParam = icgInfo->qosParam;
    icgInfo->reportedDirection = LEVEL_UP;
    return QOSM_LevelUpProc(icgInfo);
}

void QOSM_ResetAllICBAckRateOverCnt(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is null");
        return;
    }
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        icgInfo->link[i].ackRateOverCnt = 0;
    }
}

bool QOSM_ICGMgrIsDutyCycleMatch(uint8_t dutyCycle)
{
    return !QOSM_IsAutorateEnabled() || QOSM_GetDutyCycle() == QOS_DUTY_CYCLE_ANY || dutyCycle == QOS_DUTY_CYCLE_ANY ||
        QOSM_GetDutyCycle() == dutyCycle;
}

bool QOSM_HandleDownwardBitrateConstraint(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL || icgInfo->qosParam == NULL || icgInfo->totalLevelCnt == 0) {
        QOSM_LOGE("icgInfo is NULL or icg qos param is NULL or totalLevelCnt is 0");
        return false;
    }
    for (uint8_t i = 0; i < icgInfo->totalLevelCnt; i++) {
        if (icgInfo->qosParam->downwardBitrate == icgInfo->level[i].qosParam->downwardBitrate &&
            QOSM_ICGMgrIsDutyCycleMatch(icgInfo->qosParam->dutyCycle) &&
            icgInfo->level[i].isAvailable) {
            // 支持码率范围发生变化，HOST通知DSP，否则DSP不知道可用码率变化，DSP会一直卡在低码率不升档
            QOSM_NotifyReportedBitrateChangedCb(icgInfo, LEVEL_NONE, NEARLINK_INVALID_LABEL, icgInfo->qosParam);
            QOSM_LOGI("no need to update param");
            return false;
        }
    }

    for (int i = icgInfo->totalLevelCnt - 1; i >= 0; i--) {
        if (!icgInfo->level[i].isAvailable) {
            QOSM_LOGI("qosId %hhu, qosIndex %hu, qos level %hu, bit rate %hu is not supported", icgInfo->qosId,
                icgInfo->qosIndex, icgInfo->level[i].qosParam->qosLevel, icgInfo->level[i].qosParam->downwardBitrate);
            continue;
        }
        QOSM_LOGI("qosId %hhu, qosIndex %hu, qos level %hu, bit rate %hu is supported", icgInfo->qosId,
            icgInfo->qosIndex, icgInfo->level[i].qosParam->qosLevel, icgInfo->level[i].qosParam->downwardBitrate);

        if (icgInfo->level[i].qosParam->downwardBitrate <= icgInfo->qosParam->downwardBitrate &&
            QOSM_ICGMgrIsDutyCycleMatch(icgInfo->level[i].qosParam->dutyCycle)) {
            icgInfo->updatedQosParam = icgInfo->level[i].qosParam;
            icgInfo->updatedLabelId = QOSM_GetRealLabelId(icgInfo, &icgInfo->level[i]);
        }
    }

    if (icgInfo->updatedQosParam != NULL) {
        icgInfo->updateStatus |= UPDATE_BY_CALL;
        icgInfo->updatedDirection = LEVEL_DOWN;

        // 降码率，回调通知，由DSP执行降码率，执行后本模块会收到update event，等所有已建链链路都更新后再更新链路参数
        QOSM_LevelDownProc(icgInfo, false);
        return true;
    }
    // 上一次耳机通知过来96kbps可用，且码率进入了96kbps，但这一次耳机通知过来192kbps及以上可用时，会走到这里
    // 需要通知一次DSP，否则DSP不知道可用码率变化，DSP会一直卡在低码率不升档
    QOSM_NotifyReportedBitrateChangedCb(icgInfo, LEVEL_NONE, NEARLINK_INVALID_LABEL, icgInfo->qosParam);
    return false;
}

void QOSM_ProcAutoRateMsg(void *param)
{
    QOSM_LOGD("recv asc autorate msg");
    QOSM_AutoRateRecvMsgParam *labelParam = (QOSM_AutoRateRecvMsgParam *)param;
    QOSM_CHECK_RETURN(labelParam != NULL, "param is null");
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByIcgId(labelParam->qosId);

    QOSM_CHECK_RETURN(icgInfo != NULL && QOSM_GetICBTypeByIndex(icgInfo->qosIndex) == CM_IMB, "icgInfo is NULL");
    QOSM_CHECK_RETURN(icgInfo->qosIndex == labelParam->qosIndex, "icgInfo qosIndex not match");
    uint8_t msgType = labelParam->msgType;
    bool isLevelUp = false;
    if (msgType != NTF_VOICE_CALL_AUTORATE_ABILITY) {
        QOSM_CHECK_RETURN(icgInfo->reportedLabelId == labelParam->labelId, "icgInfo label id not match");
        if (icgInfo->reportedDirection != LEVEL_NONE) {
            isLevelUp = icgInfo->reportedDirection == LEVEL_UP;
        } else if (icgInfo->updatedDirection != LEVEL_NONE) {
            isLevelUp = icgInfo->updatedDirection == LEVEL_UP;
        } else {
            QOSM_LOGE("direction none");
            return;
        }
    }
    switch (msgType) {
        case CHANGE_FRAME_TYPE_RSP:
            QOSM_HandleFrameTypeChanged(icgInfo, labelParam->result, isLevelUp);
            break;
        case CHANGE_PEER_BITRATE_RSP:
            QOSM_HandlePeerBitrateChanged(icgInfo, labelParam->labelId, labelParam->result, isLevelUp);
            break;
        case NTF_VOICE_CALL_AUTORATE_ABILITY:
            QOSM_SetVoiceCallAutorateAbility(icgInfo);
            break;
        default:
            break;
    }
}

void QOSM_HandlePeerBitrateChanged(QOSM_ICGInfo *icgInfo, uint8_t targetLabel, uint8_t result, bool isLevelUp)
{
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is null");
        return;
    }
    if (result != QOSM_SUCCESS) {
        if (isLevelUp) {
            QOSM_LevelUpExceptionProc(icgInfo->qosId);
        } else {
            QOSM_LevelDownExceptionProc(icgInfo->qosId);
        }
        return;
    }
    QOSM_LOGI("qosId %hhu, peer bitrate has updated ", icgInfo->qosId);
    if (icgInfo->reportedQosParam != NULL && icgInfo->reportedLabelId == targetLabel) {
        // 通话：已感知到对端dsp升码率成功
        if (icgInfo->reportedDirection == LEVEL_UP) {
            QOSM_LOGI("reset report param");
            QOSM_ResetReportParam(icgInfo);
        }
    }
}

void QOSM_HandleFrameTypeChanged(QOSM_ICGInfo *icgInfo, uint8_t result, bool isLevelUp)
{
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is null");
        return;
    }
    // 通话码率自适应: 调用方切帧格式成功, 升码率则开始切同步链路label id, 降码率则不操作
    QOSM_LOGI("isLevelUp :%d, result :%d", isLevelUp, result);
    if (result == QOSM_SUCCESS) {
        if (!isLevelUp) {
            return;
        }
        QOSM_LOGI("fram change succ when upwards, continue change lable id");
        if (QOSM_UpdateLabelId(icgInfo) != QOSM_SUCCESS) {
            QOSM_LOGE("update label id failed, reset report param");
            QOSM_ResetReportParam(icgInfo);
        }
        QOSM_ChangeLabelSetTimer(&icgInfo->changeLableTimerId, icgInfo->qosId, icgInfo->reportedDirection);
        return;
    }
    // 切帧格式失败, 触发异常处理逻辑, 清定时器，reset
    if (isLevelUp) {
        QOSM_LevelUpExceptionProc(icgInfo->qosId);
    } else {
        QOSM_LevelDownExceptionProc(icgInfo->qosId);
    }
}

void QOSM_SetVoiceCallAutorateAbility(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is null");
        return;
    }
    icgInfo->isSupportFrame4 = true;
    QOSM_LOGI("qosId %d support voice call frame4 and autorate", icgInfo->qosId);
}