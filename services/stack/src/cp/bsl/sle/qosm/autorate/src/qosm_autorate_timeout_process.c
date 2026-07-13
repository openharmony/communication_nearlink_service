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

/****************************************************************************
 *
 * this file contains timeout process functions for autorate modules.
 *
 ***************************************************************************/

#include "qosm_autorate_timeout_process.h"

#include "qosm_icg_mgr.h"
#include "qosm_autorate_common.h"
#include "qosm_autorate_report.h"
#include "qosm_log.h"
#include "qosm_errno.h"
#include "qosm_icg_callback.h"
#include "cp_worker.h"

void QOSM_ChangeLabelStopTimer(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is null");
        return;
    }
    if (icgInfo->changeLableTimerId != QOSM_INVALID_TIMER_HANDLE) {
        CP_TimerDel(icgInfo->changeLableTimerId);
        icgInfo->changeLableTimerId = QOSM_INVALID_TIMER_HANDLE;
    }
}

static QOSM_ICGInfo *QOSM_ChangeLabelStopTimerByQosId(uint8_t qosId)
{
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(qosId);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        return NULL;
    }

    QOSM_ChangeLabelStopTimer(icgInfo);

    return icgInfo;
}

void QOSM_LevelUpExceptionProc(uint8_t qosId)
{
    QOSM_ICGInfo *icgInfo = QOSM_ChangeLabelStopTimerByQosId(qosId);
    if (icgInfo == NULL) {
        return;
    }
    // 升档逻辑：先升芯片，再升DSP。升芯片过程超时，重置Report状态即可
    QOSM_LOGE("qos id %hhu upgrade level error, reset report param", qosId);
    QOSM_ResetReportParam(icgInfo);
    QOSM_ResetAllICBAckRateOverCnt(icgInfo);
}

void QOSM_LevelDownExceptionProc(uint8_t qosId)
{
    QOSM_ICGInfo *icgInfo = QOSM_ChangeLabelStopTimerByQosId(qosId);
    if (icgInfo == NULL) {
        return;
    }
    // 降档逻辑：先降DSP，再由DSP降芯片。降DSP过程超时，重置状态，回滚DSP
    if ((icgInfo->updateStatus & UPDATE_BY_CALL) != 0) {
        QOSM_LOGE("qos id %hhu downgrade level timeout, reset update param", qosId);
        QOSM_ResetUpdateParam(icgInfo);
    } else {
        QOSM_LOGE("qos id %hhu downgrade level timeout, reset report param", qosId);
        QOSM_ResetReportParam(icgInfo);
        QOSM_ResetAllICBAckRateOverCnt(icgInfo);
    }

    (void)QOSM_RollbackWhenDownLevel(icgInfo);
    QOSM_LOGI("rollback qos param, qos id %hhu", qosId);
}

static void QOSM_UpgradeLevelTimeout(void *arg)
{
    uint8_t qosId = (uint8_t)(uintptr_t)arg;
    QOSM_LOGE("qos id %hhu upgrade level timeout", qosId);

    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(qosId);
    if (icgInfo == NULL) {
        return;
    }
    if (QOSM_GetICBTypeByIndex(icgInfo->qosIndex) == CM_IMB && icgInfo->isSupportFrame4) {
        QOSM_LOGI("upwards change label id error");
        QOSM_SendAutoRateMsg(icgInfo, icgInfo->reportedDirection, icgInfo->reportedLabelId,
            CHANGE_LABEL_ID_RSP, QOSM_FAIL);
    }
    QOSM_LevelUpExceptionProc(qosId);
}

static void QOSM_DowngradeLevelTimeout(void *arg)
{
    uint8_t qosId = (uint8_t)(uintptr_t)arg;
    QOSM_LOGE("qos id %hhu downgrade level timeout", qosId);

    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(qosId);
    if (icgInfo == NULL) {
        return;
    }

    if (QOSM_GetICBTypeByIndex(icgInfo->qosIndex) == CM_IMB && icgInfo->isSupportFrame4) {
        uint16_t direction = (icgInfo->updateStatus & UPDATE_BY_REPORT) != 0 ?
            icgInfo->reportedDirection : icgInfo->updatedDirection;
        uint8_t lableId = (icgInfo->updateStatus & UPDATE_BY_REPORT) != 0 ?
            icgInfo->reportedLabelId : icgInfo->updatedLabelId;
        QOSM_LOGI("downward change label id error");
        QOSM_SendAutoRateMsg(icgInfo, direction, lableId, CHANGE_LABEL_ID_RSP, QOSM_FAIL);
    }
    QOSM_LevelDownExceptionProc(qosId);
}

/*
 * 降档失败（即没有收到芯片上报的update complete事件），恢复原档位，并且通知调用方当前使用的码率
 * 升档失败，恢复原档位，不需要通知调用方
 */
void QOSM_ChangeLabelSetTimer(int *timerHandle, uint8_t qosId, uint16_t direction)
{
    if (*timerHandle != QOSM_INVALID_TIMER_HANDLE) {
        CP_TimerDel(*timerHandle);
        *timerHandle = QOSM_INVALID_TIMER_HANDLE;
    }

    QOSM_LOGI("start change level timeout for qos id %hhu", qosId);

    SDF_TimerParam param;
    param.period = false;
    param.args = (void *)(uintptr_t)(qosId);
    if (direction == LEVEL_DOWN) {
        param.expires = QOSM_DOWNGRADE_LEVEL_TIMEOUT_MS;
        param.callback = QOSM_DowngradeLevelTimeout;
    } else {
        param.expires = QOSM_UPGRADE_LEVEL_TIMEOUT_MS;
        param.callback = QOSM_UpgradeLevelTimeout;
    }
    uint32_t ret = CP_TimerAdd(timerHandle, &param);
    if (ret != 0) {
        QOSM_LOGE("CP_TimerAdd for qos id %hhu, errno %u", qosId, ret);
        *timerHandle = QOSM_INVALID_TIMER_HANDLE;
    }
}

void QOSM_ICBUpLevelDelayStopTimer(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is null");
        return;
    }
    if (icgInfo->upLevelDelayTimerId != QOSM_INVALID_TIMER_HANDLE) {
        CP_TimerDel(icgInfo->upLevelDelayTimerId);
        icgInfo->upLevelDelayTimerId = QOSM_INVALID_TIMER_HANDLE;
    }
}

static void QOSM_LevelUpingContinueProcess(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL || icgInfo->reportedQosParam == NULL || icgInfo->qosParam == NULL) {
        QOSM_LOGE("Invalid icgInfo or reportedQosParam or qosParam");
        return;
    }
    QOSM_LOGI("notify bitrate upgraded, down bitrate %huKbps, up bitrate %huKbps, duty cycle %hu, "
        "target label id: %hhu", icgInfo->reportedQosParam->downwardBitrate,
        icgInfo->reportedQosParam->upwardBitrate, icgInfo->reportedQosParam->dutyCycle,
        icgInfo->reportedLabelId);

    if (icgInfo->qosParam->downwardBitrate != icgInfo->reportedQosParam->downwardBitrate) {
        // 通话码率自适应：通知调用方切对端码率
        if (QOSM_GetICBTypeByIndex(icgInfo->qosIndex) == CM_IMB && icgInfo->isSupportFrame4) {
            QOSM_LOGI("notify asc level up peer bitrate");
            QOSM_SendAutoRateMsg(icgInfo, icgInfo->reportedDirection, icgInfo->reportedLabelId,
                CHANGE_PEER_BITRATE_REQ, QOSM_SUCCESS);
        }
        QOSM_NotifyReportedBitrateChangedCb(icgInfo, icgInfo->reportedDirection,
            icgInfo->reportedLabelId, icgInfo->reportedQosParam);
    } else {
        // 通话码率回滚触发的升码率：不需要通知双端dsp切码率，通知asc清理缓存，并重置reportedQosParam
        if (QOSM_GetICBTypeByIndex(icgInfo->qosIndex) == CM_IMB && icgInfo->isSupportFrame4) {
            QOSM_SendAutoRateMsg(icgInfo, icgInfo->reportedDirection, icgInfo->reportedLabelId,
                CHANGE_PEER_BITRATE_REQ, QOSM_FAIL);
            QOSM_LOGI("reset report param");
            QOSM_ResetReportParam(icgInfo);
        }
    }

    // 升码率成功，更新当前码率
    QOSM_LOGI("notify bitrate upgraded done");
    icgInfo->qosParam = icgInfo->reportedQosParam;
    // 媒体：通知dsp升码率后重置reportedQosParam；通话：感知到对端dsp升码率成功后再重置reportedQosParam
    if (QOSM_GetICBTypeByIndex(icgInfo->qosIndex) != CM_IMB) {
        QOSM_LOGI("reset report param");
        QOSM_ResetReportParam(icgInfo);
    }

    // 升降码率过程中屏蔽了call通知降码率的处理，这里补上
    if (!QOSM_HandleDownwardBitrateConstraint(icgInfo)) {
        QOSM_ExecuteDelayConnTask(icgInfo);
    }
}

static void QOSM_LevelUpingContinueProcessTimeout(void *arg)
{
    uint8_t qosId = (uint8_t)(uintptr_t)arg;
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(qosId);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        return;
    }

    QOSM_ICBUpLevelDelayStopTimer(icgInfo);
    QOSM_LevelUpingContinueProcess(icgInfo);
}

void QOSM_ICBUpLevelDelaySetTimer(int *timerHandle, uint8_t qosId)
{
    if (*timerHandle != QOSM_INVALID_TIMER_HANDLE) {
        CP_TimerDel(*timerHandle);
        *timerHandle = QOSM_INVALID_TIMER_HANDLE;
    }

    QOSM_LOGI("start up level delay timer for qos id %hhu", qosId);

    SDF_TimerParam param;
    param.period = false;
    param.expires = QOSM_UPGRADE_LEVEL_TIMEOUT_MS;
    param.args = (void *)(uintptr_t)(qosId);
    param.callback = QOSM_LevelUpingContinueProcessTimeout;
    uint32_t ret = CP_TimerAdd(timerHandle, &param);
    if (ret != 0) {
        QOSM_LOGE("CP_TimerAdd for qos id %hhu, errno %u", qosId, ret);
        *timerHandle = QOSM_INVALID_TIMER_HANDLE;
    }
}