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
 * this file contains autorate report decision logic:
 * QOSM_ICBQualityReport periodic quality report to decision up/down bitrate.
 *
 ***************************************************************************/

#include "qosm_autorate_report.h"

#include "qosm_autorate_notify.h"
#include "qosm_autorate_timeout_process.h"
#include "qosm_icg_mgr.h"
#include "qosm_table_mgr.h"
#include "qosm_icg_callback.h"
#include "qosm_log.h"
#include "qosm_errno.h"
#include "qosm_audio_dfx.h"


static void QOSM_DspStatusProc(QOSM_ICGInfo *icgInfo, void *data)
{
    (void)data;
    if (QOSM_HasICBConnecting(icgInfo) || icgInfo->updateStatus != UPDATE_NONE || icgInfo->linkCnt == 0 ||
        !QOSM_IsAutorateSupported(icgInfo) || icgInfo->qosParam == NULL) {
        return;
    }
    QOSM_LOGI("icgInfo->updateStatus = %d", icgInfo->updateStatus);
    (void)QOSM_HandleDownwardBitrateConstraint(icgInfo);
}

void QOSM_DspStatusCbk(bool isOn)
{
    QOSM_AutorateSetDspOn(isOn);
    if (!isOn) {
        return;
    }

    QOSM_ICGMgrIterate(QOSM_DspStatusProc, NULL);
}

void QOSM_DspFlowCtrlCbk(uint16_t connHandle, bool enterFlowCtrl)
{
    if (!enterFlowCtrl) {
        return;
    }

    uint8_t linkIndex = 0;
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByConnHandle(connHandle, &linkIndex);
    if (icgInfo == NULL) {
        return;
    }
    if (icgInfo->qosParam->downwardBitrate < BITRATE_ENTER_DSP_FLOW_CTRL_MIN) {
        QOSM_LOGI("qosId %hhu, conn handle %hu, bitrate %hu, ignore", icgInfo->qosId, connHandle,
            icgInfo->qosParam->downwardBitrate);
        return;
    }
    CM_ICBQuality param = {};
    param.connHandle = connHandle;
    param.ackRate = 1;
    QOSM_ICBQualityReport(&param);
}

static bool QOSM_IsAllICBExcellent(const QOSM_ICGInfo *icgInfo)
{
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].ackRateOverCnt < MAX_ACK_RATE_OVER_CNT && icgInfo->link[i].connectStatus != LINK_NONE) {
            return false;
        }
    }
    return true;
}

static bool QOSM_AutoRateNeedDownWard(const struct QOSM_AutoRateThreshold *thresh, const CM_ICBQuality *param,
    QOSM_LinkParam *linkParam, uint16_t bitrate)
{
    if (param->icbType == CM_IMB) {
        return param->rssi <= thresh->downRssi && param->errPacketRate >= ERR_PACKEY_RATE_THRESHOLD;
    }
    return (param->rssi <= RSSI_THRESHOLD_4M_16QAM && bitrate >= BITRATE_4M_16QAM_MIN &&
        bitrate <= BITRATE_4M_16QAM_MAX) ||
        (param->rssi <= RSSI_THRESHOLD_4M_QPSK && bitrate >= BITRATE_4M_QPSK_MIN &&
        bitrate <= BITRATE_4M_QPSK_MAX) ||
        (param->diffMax >= QOSM_HALF_ROUND_UP(linkParam->ftG2T)) ||
        (param->ackRate < thresh->downAckRate);
}

static bool QOSM_AutoRateCanUpward(const struct QOSM_AutoRateThreshold *thresh, const CM_ICBQuality *param)
{
    if (param->icbType == CM_IMB) {
        return param->rssi >= thresh->upRssi;
    }
    return param->ackRate >= thresh->upAckRate && param->diffMax <= thresh->upDiffMax &&
        param->rssi >= thresh->upRssi;
}

static void QOSM_GetTargetDownLevel(QOSM_ICGInfo *icgInfo, QOSM_LinkParam *qosParam, uint16_t *reportedDirection,
    uint8_t *targetQosLevel)
{
    QOSM_LOGI("current qos level %hhu, total level cnt %hhu, level cnt %hhu",
        qosParam->qosLevel, icgInfo->totalLevelCnt, icgInfo->levelCnt);

    QOSM_QosLevel downQosLevel = qosParam->downQosLevel;
    for (uint8_t i = 0; i < icgInfo->totalLevelCnt; i++) {
        if (icgInfo->level[i].qosParam == NULL) {
            QOSM_LOGE("qosParam is null");
            continue;
        }

        if (icgInfo->level[i].qosParam->qosLevel != downQosLevel) {
            continue;
        }

        if (!icgInfo->level[i].isAvailable) {
            QOSM_LOGI("try down bitrate %huKbps, but not available", icgInfo->level[i].qosParam->downwardBitrate);
            downQosLevel = icgInfo->level[i].qosParam->downQosLevel;
            continue;
        }
        QOSM_LOGI("down bitrate %huKbps is available", icgInfo->level[i].qosParam->downwardBitrate);
        *targetQosLevel = QOSM_MIN(*targetQosLevel, icgInfo->level[i].qosParam->qosLevel);
        *reportedDirection = LEVEL_DOWN;
        break;
    }
    QOSM_LOGI("target qos level: %u, reported direction: %u", *targetQosLevel, *reportedDirection);
}

static void QOSM_GetTargetUpLevel(QOSM_ICGInfo *icgInfo, uint16_t *reportedDirection, uint8_t *targetQosLevel)
{
    QOSM_LOGI("current level %hhu, total level cnt %hhu, level cnt %hhu",
        icgInfo->qosParam->qosLevel, icgInfo->totalLevelCnt, icgInfo->levelCnt);

    if (icgInfo->totalLevelCnt == 0) {
        QOSM_LOGE("totalLevelCnt is 0");
        return;
    }
    QOSM_QosLevel upQosLevel = icgInfo->qosParam->upQosLevel;
    for (int i = icgInfo->totalLevelCnt - 1; i >= 0; i--) {
        if (icgInfo->level[i].qosParam == NULL) {
            QOSM_LOGE("qosParam is null, i: %d", i);
            continue;
        }
        if (icgInfo->qosParam->dutyCycle == QOS_DUTY_CYCLE_ANY) {
            if (icgInfo->level[i].qosParam->qosLevel <= icgInfo->qosParam->qosLevel) {
                continue;
            }
        } else {
            if (icgInfo->level[i].qosParam->qosLevel != upQosLevel) {
                continue;
            }
        }
        if (!icgInfo->level[i].isAvailable) {
            QOSM_LOGI("try down bitrate %huKbps, but not available", icgInfo->level[i].qosParam->downwardBitrate);
            upQosLevel = icgInfo->level[i].qosParam->upQosLevel;
            continue;
        }
        QOSM_LOGI("down bitrate %huKbps is available", icgInfo->level[i].qosParam->downwardBitrate);
        *targetQosLevel = QOSM_MAX(*targetQosLevel, icgInfo->level[i].qosParam->qosLevel);
        *reportedDirection = LEVEL_UP;
        break;
    }
    QOSM_LOGI("target qos level: %u, reported direction: %u", *targetQosLevel, *reportedDirection);
}

static uint8_t QOSM_GetTargetQosLevel(CM_ICBQuality *param, QOSM_ICGInfo *icgInfo, struct QosICBInfo *link)
{
    uint16_t reportedDirection = LEVEL_NONE;
    uint16_t bitrate = icgInfo->qosParam->downwardBitrate;
    uint8_t targetQosLevel = icgInfo->qosParam->qosLevel;
    QOSM_LinkParam *linkParam = icgInfo->qosParam;
    const struct QOSM_AutoRateThreshold *thresh = QOSM_GetAutorateThreshold(icgInfo->qosIndex, bitrate);
    if (thresh == NULL) {
        return targetQosLevel;
    }
    // 降码率
    if (QOSM_AutoRateNeedDownWard(thresh, param, linkParam, bitrate)) {
        QOSM_GetTargetDownLevel(icgInfo, icgInfo->qosParam, &reportedDirection, &targetQosLevel);
    }

    // 升码率
    if (reportedDirection == LEVEL_NONE && QOSM_AutoRateCanUpward(thresh, param)) {
        link->ackRateOverCnt++;
        bool isAllICBExcellent = QOSM_IsAllICBExcellent(icgInfo);
        QOSM_LOGI("connHnadle: %hu, ackRateOverCnt is: %u, is all icb quality excellent: %d, "
            "cur bitrate: %u, up thresh: %u, down thresh: %u",
            link->connHandle, link->ackRateOverCnt, isAllICBExcellent,
            thresh->bitrate, thresh->upAckRate, thresh->downAckRate);
        if (isAllICBExcellent && link->ackRateOverCnt >= MAX_ACK_RATE_OVER_CNT) {
            QOSM_ResetAllICBAckRateOverCnt(icgInfo);
            QOSM_GetTargetUpLevel(icgInfo, &reportedDirection, &targetQosLevel);
        }
    } else {
        link->ackRateOverCnt = 0;
    }
    // icg底下有多条同步链路，选择其中最低的level为目标level
    QOSM_LOGI("targetQosLevel: %hhu, reportedDirection: %hu, icg reportedDirection:%hu, "
        "cur bitrate: %u, up thresh: %u, down thresh: %u",
        targetQosLevel, reportedDirection, icgInfo->reportedDirection,
        thresh->bitrate, thresh->upAckRate, thresh->downAckRate);
    if (icgInfo->reportedDirection != LEVEL_NONE && icgInfo->reportedQosParam != NULL) {
        QOSM_LOGI("icg reportedQosLevel: %hu", icgInfo->reportedQosParam->qosLevel);
        if (targetQosLevel < icgInfo->reportedQosParam->qosLevel) {
            icgInfo->reportedDirection = reportedDirection;
        } else {
            targetQosLevel = icgInfo->reportedQosParam->qosLevel;
        }
    } else {
        icgInfo->reportedDirection = reportedDirection;
    }
    return targetQosLevel;
}

void QOSM_PrintQualityReportParam(CM_ICBQuality *param)
{
    if (param == NULL) {
        QOSM_LOGE("param is null");
        return;
    }
    QOSM_LOGI("conn handle: %hu, diff total: %u, diff max: %u, diff avg: %u, tx flushed: %hu, rx loss pkt"
        " cnt/continous: %hu/%hu, rssi: %hhd, ack rate: %hhu, reserved: 0x%08x 0x%08x 0x%08x 0x%08x",
        param->connHandle, param->diffTotal, param->diffMax, param->diffAvg,
        param->txFlushed, param->rxLossPktCnt, param->rxLossMaxContPkt, param->rssi, param->ackRate,
        param->reserve1, param->reserve2, param->reserve3, param->reserve4);
}

static bool QOSM_ICBQualityReportParamValid(CM_ICBQuality *param)
{
    if (!QOSM_AutorateIsBitrateChangeCbkValid() || !QOSM_IsAutorateEnabled()) {
        QOSM_LOGD("ignore quality event when cbk is null or autorate disable");
        return false;
    }

    if (param->icbType == CM_IOB && param->txFlushed == 0 && param->ackRate == 0) {
        QOSM_LOGD("ignore quality event when tx flushed and ack rate are 0");
        return false;
    }

    return true;
}

static bool QOSM_GetReportedQosParam(QOSM_ICGInfo *icgInfo, uint8_t targetQosLevel)
{
    for (uint8_t i = 0; i < icgInfo->totalLevelCnt; i++) {
        if (targetQosLevel != icgInfo->level[i].qosParam->qosLevel) {
            continue;
        }
        icgInfo->reportedQosParam = icgInfo->level[i].qosParam;
        icgInfo->reportedLabelId = QOSM_GetRealLabelId(icgInfo, &icgInfo->level[i]);
        return true;
    }
    QOSM_ResetReportParam(icgInfo);
    QOSM_LOGE("qosId %hhu, qosIndex %hhu, unknown target level %hhu", icgInfo->qosId, icgInfo->qosIndex,
        targetQosLevel);
    return false;
}

static bool QOSM_LevelUpingTerminateByLevelDown(QOSM_ICGInfo *icgInfo, CM_ICBQuality *param)
{
    uint16_t reportedDirection = LEVEL_NONE;
    uint16_t bitrate = icgInfo->reportedQosParam->downwardBitrate;
    uint8_t targetQosLevel = icgInfo->reportedQosParam->qosLevel;
    const struct QOSM_AutoRateThreshold *thresh = QOSM_GetAutorateThreshold(icgInfo->qosIndex, bitrate);
    if (thresh == NULL) {
        return false;
    }

    if (QOSM_AutoRateNeedDownWard(thresh, param, icgInfo->reportedQosParam, bitrate)) {
        QOSM_GetTargetDownLevel(icgInfo, icgInfo->reportedQosParam, &reportedDirection, &targetQosLevel);
    }
    if (reportedDirection != LEVEL_DOWN) {
        return false;
    }
    QOSM_LinkParam *reportedQosParam = NULL;
    uint8_t reportedLabelId = 0;
    for (uint8_t i = 0; i < icgInfo->totalLevelCnt; i++) {
        if (targetQosLevel != icgInfo->level[i].qosParam->qosLevel) {
            continue;
        }
        reportedQosParam = icgInfo->level[i].qosParam;
        reportedLabelId = QOSM_GetRealLabelId(icgInfo, &icgInfo->level[i]);
        break;
    }
    if (reportedQosParam == NULL) {
        return false;
    }
    icgInfo->reportedDirection = reportedDirection;
    icgInfo->reportedQosParam = reportedQosParam;
    icgInfo->reportedLabelId = reportedLabelId;
    if (reportedQosParam->qosLevel == icgInfo->qosParam->qosLevel) {
        // 芯片已升档，DSP暂未升，terminate有效，只需要芯片降档
        // DLI命令降档，若降档失败，依靠超时恢复
        if (QOSM_GetICBTypeByIndex(icgInfo->qosIndex) == CM_IMB && icgInfo->isSupportFrame4) {
            QOSM_LOGI("notify asc levelup");
            QOSM_SendAutoRateMsg(icgInfo, icgInfo->reportedDirection, icgInfo->reportedLabelId,
                CHANGE_FRAME_TYPE_REQ, QOSM_SUCCESS);
            return true;
        }
        (void)QOSM_UpdateLabelId(icgInfo);
        QOSM_ChangeLabelSetTimer(&icgInfo->changeLableTimerId, icgInfo->qosId, icgInfo->reportedDirection);
    } else if (reportedQosParam->qosLevel < icgInfo->qosParam->qosLevel) {
        // 芯片已升档，DSP暂未升，但需要降到更低的档，故通知DSP降档（DSP会负责通知芯片降档）
        QOSM_LevelDownProc(icgInfo, true);
    } else {
        QOSM_LOGE("unexpected branch");
    }
    QOSM_LOGI("terminate level up by level down");
    QOSM_ICBUpLevelDelayStopTimer(icgInfo);
    return true;
}

/*
 * 芯片每隔500ms，上报所有同步链路的质量参数
 * 1.更新参数时，若处于更新参数期间（耳机或共存触发），在参数更新回调里决策，若芯片上报参数算出来的档位更低，需触发更新参数
 */
void QOSM_ICBQualityReport(CM_ICBQuality *param)
{
    if (param == NULL) {
        QOSM_LOGE("param is null");
        return;
    }
    QOSM_PrintQualityReportParam(param);
    if (param->icbType == CM_IOB) {
        QOSM_AudioDfxQualityReport(param->txFlushed, param->ackRate);
    }
    if (!QOSM_ICBQualityReportParamValid(param)) {
        return;
    }

    uint8_t linkIndex = 0;
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByConnHandle(param->connHandle, &linkIndex);

    QOSM_CHECK_RETURN(icgInfo != NULL && icgInfo->qosParam != NULL, "can't find icg info or qos param is NULL");
    QOSM_CHECK_RETURN(icgInfo->levelCnt != 0 && icgInfo->levelCnt <= QOSM_AUTORATE_MAX_LEVEL_CNT,
        "illegal level count");

    if (param->txFlushed != 0) {
        struct QOSM_AudioDfxChoppyInfo info = { param->connHandle, param->txFlushed, param->ackRate, param->rssi };
        QOSM_AudioDfxNotifyChoppy(&info);
        param->ackRate = 1;
        QOSM_LOGI("txFlushed %hu, set ackRate 1", param->txFlushed);
    }

    if (!QOSM_IsAutorateSupported(icgInfo) || QOSM_HasICBConnecting(icgInfo)) {
        return;
    }

    if (icgInfo->upLevelDelayTimerId != QOSM_INVALID_TIMER_HANDLE) {
        if (QOSM_LevelUpingTerminateByLevelDown(icgInfo, param)) {
            return;
        }
    }

    if (icgInfo->updateStatus != UPDATE_NONE) {
        QOSM_LOGD("icg is updating");
        return;
    }

    // 得到目标qosLevel
    uint8_t targetQosLevel = QOSM_GetTargetQosLevel(param, icgInfo, &icgInfo->link[linkIndex]);

    if (icgInfo->reportedDirection == LEVEL_NONE) {
        return;
    }

    // 根据目标qosLevel，得到上报参数
    // 至此已获得码率变化三元组 reportedQosParam reportedLabelId reportedDirection
    if (!QOSM_GetReportedQosParam(icgInfo, targetQosLevel)) {
        return;
    }

    // 等icg底下所有建链的链路都上报后，再进行下一步
    QOSM_LOGI("connHandle: %hu, target qos level: %hhu, connected cnt: %hhu, direction: %hu,"
        " connect status: %hu, update status: %hhu", icgInfo->link[linkIndex].connHandle, targetQosLevel,
        icgInfo->connectedCnt, icgInfo->reportedDirection,
        icgInfo->link[linkIndex].connectStatus, icgInfo->updateStatus);

    QOSM_ICBQualityReportCheck(icgInfo);
}

bool QOSM_ExecuteBitrateChangeDecision(uint16_t connHandle, uint16_t qosIndex, uint16_t reportedDirection,
    uint16_t reportedQosLevel)
{
    QOSM_LOGI("connHandle: %hu, qosIndex: %hu, reportedDirection: %hu, reportedQosLevel: %hu",
        connHandle, qosIndex, reportedDirection, reportedQosLevel);
    // 1. 根据connHandle查找icgInfo
    uint8_t linkIndex = 0;
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByConnHandle(connHandle, &linkIndex);
    if (icgInfo == NULL || icgInfo->qosParam == NULL) {
        QOSM_LOGE("icgInfo is null");
        return false;
    }

    if (icgInfo->levelCnt == 0 || icgInfo->levelCnt > QOSM_AUTORATE_MAX_LEVEL_CNT) {
        QOSM_LOGE("illegal level count");
        return false;
    }

    if (!QOSM_IsAutorateSupported(icgInfo)) {
        QOSM_LOGI("autorate not supported");
        return false;
    }

    if (QOSM_HasICBConnecting(icgInfo)) {
        QOSM_LOGI("icb is connecting");
        return false;
    }

    if (icgInfo->qosIndex != qosIndex) {
        QOSM_LOGE("Check qosIndex error! Expect: %hu, Actual: %hu", icgInfo->qosIndex, qosIndex);
        return false;
    }

    if (icgInfo->upLevelDelayTimerId != QOSM_INVALID_TIMER_HANDLE && reportedDirection == LEVEL_DOWN) {
        // 需要截止升码率，转为降码率
        QOSM_LOGI("level uping terminate by level down");
        QOSM_ICBUpLevelDelayStopTimer(icgInfo);
        icgInfo->reportedDirection = reportedDirection;
        if (!QOSM_GetReportedQosParam(icgInfo, reportedQosLevel)) {
            return false;
        }
        QOSM_ICBQualityReportCheck(icgInfo);
        return true;
    }

    if (icgInfo->updateStatus != UPDATE_NONE) {
        QOSM_LOGI("icg is updating"); // dsp侧，需要设定三秒超时重置
        return false;
    }

    // 2. 获得码率变化三元组 reportedQosParam reportedLabelId reportedDirection
    icgInfo->reportedDirection = reportedDirection;
    if (!QOSM_GetReportedQosParam(icgInfo, reportedQosLevel)) {
        return false;
    }

    // 3. 执行码率变化
    QOSM_LOGI("connHandle: %hu, target qos level: %hhu, connected cnt: %hhu, direction: %hu,"
        " connect status: %hu, update status: %hhu", icgInfo->link[linkIndex].connHandle, reportedQosLevel,
        icgInfo->connectedCnt, icgInfo->reportedDirection,
        icgInfo->link[linkIndex].connectStatus, icgInfo->updateStatus);

    QOSM_ICBQualityReportCheck(icgInfo);
    return true;
}