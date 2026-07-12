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

#include "qosm_autorate_notify.h"
#include "qosm_autorate_timeout_process.h"

#include <stdatomic.h>

#include "byte_codec.h"
#include "cm_def.h"
#include "cm_errno.h"
#include "cm_icb_api.h"
#include "cm_icb_mgr.h"
#include "cm_logic_link_api.h"
#include "qosm_errno.h"
#include "qosm_log.h"
#include "qosm_table_mgr.h"
#include "securec.h"
#include "qosm_audio_dfx.h"
#include "nlstk_cfgdb_api.h"
#include "qosm_icg_callback.h"
#include "qosm_icg_mgr.h"
#include "qosm_autorate_report.h"
#include "qosm_autorate_common.h"

static atomic_uint_least16_t g_maxBitrate = QOSM_MAX_BITRATE;

static uint16_t QOSM_GetRealBitrate(CM_ICBType icbType, uint16_t bitrate, uint8_t linkCnt)
{
    return icbType == CM_IOB ? bitrate * linkCnt : bitrate;
}

static bool QOSM_IsBitrateAvailable(QOSM_ICGInfo *icgInfo, uint16_t downwardBitrate)
{
    /* 5G场景，媒体可用最大码率1.5Mpbs, 通话可用最小码率64kbps */
    return downwardBitrate <= (icgInfo->is5G ? QOSM_5G_MAX_BITRATE : QOSM_MAX_BITRATE) &&
        downwardBitrate >= (icgInfo->is5G ? QOSM_5G_MIN_BITRATE : QOSM_MIN_BITRATE) &&
        downwardBitrate <= atomic_load(&g_maxBitrate);
}

bool QOSM_IsLevelSupported(QOSM_ICGInfo *icgInfo, struct QosLevelLabel *level, QOSM_QosLevel targetLevel)
{
    if (icgInfo == NULL || level == NULL) {
        QOSM_LOGE("icgInfo or level is null");
        return false;
    }
    if (!QOSM_IsAutorateEnabled() && level->qosParam->qosLevel != targetLevel) {
        QOSM_LOGI("qos level %u not match start level %u when autorate is disabled",
            level->qosParam->qosLevel, targetLevel);
        return false;
    }

    return level->isSupported;
}

void QOSM_CopySupportedBitrate(QOSM_ICGInfo *icgInfo, const uint16_t *supportedBitrate,
    uint32_t supportedBitrateCnt)
{
    if (icgInfo == NULL || supportedBitrate == NULL) {
        QOSM_LOGE("icgInfo or supportedBitrate is null");
        return;
    }
    icgInfo->earphoneBitrate.supportedBitrateCnt = supportedBitrateCnt;
    for (uint8_t i = 0; i < supportedBitrateCnt && i < QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT; i++) {
        icgInfo->earphoneBitrate.supportedBitrate[i] = supportedBitrate[i];
    }
}

void QOSM_SetLevelSupported(QOSM_ICGInfo *icgInfo, struct QosLevelLabel *level, uint32_t supportedBitrateCnt,
    const uint16_t *supportedBitrate, QOSM_QosLevel targetLevel)
{
    if (icgInfo == NULL || level == NULL || supportedBitrate == NULL) {
        QOSM_LOGE("icgInfo or level or supportedBitrate is null");
        return;
    }
    for (uint8_t i = 0; i < supportedBitrateCnt && i < QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT; i++) {
        if (level->qosParam->downwardBitrate ==
            QOSM_GetRealBitrate(icgInfo->icbType, supportedBitrate[i], icgInfo->linkCnt)) {

            if ((!QOSM_IsAutorateEnabled() || !icgInfo->supportAutorate) && level->qosParam->qosLevel != targetLevel) {
                QOSM_LOGI("level %u bitrate %u not match start level %u, ignore it",
                    level->qosParam->qosLevel, level->qosParam->downwardBitrate, targetLevel);
                break;
            }

            level->isSupported = true;
            level->isAvailable = QOSM_IsBitrateAvailable(icgInfo, level->qosParam->downwardBitrate) &&
                QOSM_ICGMgrIsDutyCycleMatch(level->qosParam->dutyCycle);
            QOSM_LOGI("level %u bitrate %u is supported %u, available %u", level->qosParam->qosLevel,
                level->qosParam->downwardBitrate, level->isSupported, level->isAvailable);
            return;
        }
    }
    level->isSupported = false;
    level->isAvailable = false;
}

void QOSM_UpdateICGComplete(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is null");
        return;
    }
    icgInfo->updatedCnt = 0;
    QOSM_LOGI("update complete, update status: %hhu, reported direction: %hu",
        icgInfo->updateStatus, icgInfo->reportedDirection);

    QOSM_ResetAllICBAckRateOverCnt(icgInfo);
    // 耳机反馈、共存模块触发的降码率 切label id成功
    if ((icgInfo->updateStatus & UPDATE_BY_CALL) != 0) {
        if (QOSM_GetICBTypeByIndex(icgInfo->qosIndex) == CM_IMB && icgInfo->isSupportFrame4) {
            QOSM_SendAutoRateMsg(icgInfo, icgInfo->updatedDirection, icgInfo->updatedLabelId,
                CHANGE_LABEL_ID_RSP, QOSM_SUCCESS);
        }
        icgInfo->qosParam = icgInfo->updatedQosParam;
        QOSM_ResetUpdateParam(icgInfo);
        QOSM_LOGI("notify bitrate decreased done");
    }
    if ((icgInfo->updateStatus & UPDATE_BY_REPORT) != 0) {
        if (QOSM_GetICBTypeByIndex(icgInfo->qosIndex) == CM_IMB && icgInfo->isSupportFrame4) {
            QOSM_SendAutoRateMsg(icgInfo, icgInfo->reportedDirection, icgInfo->reportedLabelId,
                CHANGE_LABEL_ID_RSP, QOSM_SUCCESS);
        }
        // 升码率需要等待2个500ms周期上报以后再处理
        if (icgInfo->reportedDirection == LEVEL_UP) {
            QOSM_ChangeLabelStopTimer(icgInfo);
            QOSM_ICBUpLevelDelaySetTimer(&icgInfo->upLevelDelayTimerId, icgInfo->qosId);
            return;
        }
        // 降码率成功，更新当前码率
        icgInfo->qosParam = icgInfo->reportedQosParam;
        QOSM_ResetReportParam(icgInfo);
        QOSM_LOGI("notify bitrate decreased done");
    }
    QOSM_ChangeLabelStopTimer(icgInfo);

    // 升降码率过程中屏蔽了call通知降码率的处理，这里补上
    if (!QOSM_HandleDownwardBitrateConstraint(icgInfo)) {
        QOSM_ExecuteDelayConnTask(icgInfo);
    }
}

static bool QOSM_ShouldRetryUpdateICB(QOSM_ICGInfo *icgInfo, CM_ICBChannel *channel)
{
    uint8_t labelId = 0;
    QOSM_LinkParam *qosParam = (icgInfo->updateStatus & UPDATE_BY_CALL) != 0 ? icgInfo->updatedQosParam :
        icgInfo->reportedQosParam;
    if (!QOSM_GetLabelId(icgInfo, qosParam, &labelId)) {
        QOSM_LOGE("unexpected error, get label id failed, qos id: %hhu", icgInfo->qosId);
        return false;
    }

    if (channel->labelId == labelId) {
        QOSM_LOGI("conn handler 0x%04x update complete", channel->connHandle);
        return false;
    }

    // 检查链路的labelId是否是预期的labelId，不是的话需重新更新链路参数
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (channel->connHandle != icgInfo->link[i].connHandle) {
            continue;
        }
        if (icgInfo->reportedDirection != LEVEL_UP) {
            return false;
        }
        bool ret = QOSM_LevelUpProc(icgInfo);
        QOSM_LOGI("retry update qos param result: %u, reported label id: %u, target label id: %u", ret,
            channel->labelId, labelId);
        return ret;
    }
    return false;
}

static void QOSM_CorrectICGQosParam(QOSM_ICGInfo *icgInfo, CM_ICBChannel *channel)
{
    uint8_t labelId = 0;
    if (!QOSM_GetLabelId(icgInfo, icgInfo->qosParam, &labelId)) {
        QOSM_LOGE("unexpected error, get label id failed, qos id: %hhu", icgInfo->qosId);
        return;
    }

    if (channel->labelId == labelId) {
        return;
    }

    // 检查上报的labelId是否是当前链路使用的labelId，不是的话需矫正为当前使用的链路参数
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (channel->connHandle != icgInfo->link[i].connHandle) {
            continue;
        }
        if (labelId < channel->labelId) {
            bool ret = QOSM_RollbackWhenDownLevel(icgInfo);
            QOSM_LOGI("correct qos param result: %u, reported label id: %u, target label id: %u", ret,
                channel->labelId, labelId);
        } else {
            // 升档不做处理，无影响
        }
    }
}

/*
 * 两个关键点：升码率由host调用dli、降码率由dsp调用dli
 * 其中：
 *     1、升码率只通过500ms周期上报（report）触发
 *     2、降码率由调用（call）或者500ms周期上报（report）触发
 */
void QOSM_UpdateICG(CM_ICBConnection *connection)
{
    QOSM_CHECK_RETURN(connection->channelCnt != 0 && connection->channelCnt <= QOSM_AUTORATE_MAX_LINK_CNT,
        "connection channel count: %u is invalid", connection->channelCnt);
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByIcgId(connection->id);
    QOSM_CHECK_RETURN(icgInfo != NULL, "icgInfo is NULL");
    QOSM_CHECK_RETURN(connection->errorCode == CM_ICB_SUCCESS, "errorCode=%u", connection->errorCode);

    if (icgInfo->updateStatus == UPDATE_NONE) {
        for (uint8_t i = 0; i < connection->channelCnt; i++) {
            QOSM_CorrectICGQosParam(icgInfo, &connection->channel[i]);
        }
        return;
    }

    for (uint8_t i = 0; i < connection->channelCnt; i++) {
        if (!QOSM_ShouldRetryUpdateICB(icgInfo, &connection->channel[i])) {
            icgInfo->updatedCnt++;
        }
    }
    if (icgInfo->updatedCnt < icgInfo->connectedCnt) {
        return;
    }
    
    // 所有ICB链路都收到完成事件后，才更新ICG状态
    QOSM_UpdateICGComplete(icgInfo);
}

static void QOSM_SetLevelAvailable(QOSM_ICGInfo *icgInfo, struct QosLevelLabel *level)
{
    if (!level->isSupported) {
        level->isAvailable = false;
        return;
    }
    for (uint8_t i = 0; i < icgInfo->earphoneBitrate.supportedBitrateCnt &&
        i < QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT; i++) {
        if (level->qosParam->downwardBitrate ==
            QOSM_GetRealBitrate(icgInfo->icbType, icgInfo->earphoneBitrate.supportedBitrate[i], icgInfo->linkCnt) &&
            QOSM_IsBitrateAvailable(icgInfo, level->qosParam->downwardBitrate) &&
            QOSM_ICGMgrIsDutyCycleMatch(level->qosParam->dutyCycle)) {
            QOSM_LOGI("SetLevelAvailable, qos level %hu, bit rate %hu is available", level->qosParam->qosLevel,
                level->qosParam->downwardBitrate);
            level->isAvailable = true;
            return;
        }
    }
    QOSM_LOGI("SetLevelAvailable, qos level %hu, bit rate %hu is unavailable", level->qosParam->qosLevel,
        level->qosParam->downwardBitrate);
    level->isAvailable = false;
}

static bool QOSM_IsSupportedBitrateValid(QOSM_ICGInfo *icgInfo, QOSM_ICGMgrNotifyParam *notifyParam)
{
    for (uint8_t i = 0; i < notifyParam->supportedBitrateCnt; i++) {
        for (uint8_t j = 0; j < icgInfo->totalLevelCnt; j++) {
            if (icgInfo->level[j].qosParam->downwardBitrate ==
                QOSM_GetRealBitrate(icgInfo->icbType, notifyParam->supportedBitrate[i], icgInfo->linkCnt) &&
                !icgInfo->level[j].isSupported) {
                QOSM_LOGE("bitrate %u is invalid", notifyParam->supportedBitrate[i]);
                return false;
            }
        }
    }
    return true;
}

static void QOSM_ICGMgrParamNotifyInner(QOSM_ICGInfo *icgInfo, void *param)
{
    // 1.拦截重复调用，在上一次更新结束之前，本次又收到更新的情况下，不执行本次更新
    // 2.只处理降码率情况，升码率只在链路参数周期上报里处理
    QOSM_LOGI("Enter");
    QOSM_ICGMgrNotifyParam *notifyParam = (QOSM_ICGMgrNotifyParam *)param;
    if (notifyParam->type == NOTIFY_TYPE_EARPHONE) {
        if (!QOSM_IsSupportedBitrateValid(icgInfo, notifyParam)) {
            QOSM_LOGE("bitrate is invalid");
            return;
        }
        QOSM_CopySupportedBitrate(icgInfo, notifyParam->supportedBitrate, notifyParam->supportedBitrateCnt);
    }

    for (uint8_t i = 0; i < icgInfo->totalLevelCnt; i++) {
        QOSM_SetLevelAvailable(icgInfo, &icgInfo->level[i]);
    }

    if (QOSM_HasICBConnecting(icgInfo) || icgInfo->updateStatus != UPDATE_NONE || icgInfo->linkCnt == 0 ||
        !QOSM_IsAutorateSupported(icgInfo) || icgInfo->qosParam == NULL) {
        QOSM_LOGI("icgInfo->updateStatus = %d", icgInfo->updateStatus);
        return;
    }

    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (!QOSM_IsICBConnected(&icgInfo->link[i])) {
            return;
        }

        if (notifyParam->type == NOTIFY_TYPE_EARPHONE || notifyParam->type == NOTIFY_TYPE_5G) {
            (void)QOSM_HandleDownwardBitrateConstraint(icgInfo);
            return;
        }

        if (notifyParam->type == NOTIFY_TYPE_COEXIST && !QOSM_IsDutyCycleFixed()) {
            (void)QOSM_HandleDownwardBitrateConstraint(icgInfo);
            return;
        }
    }
}

void QOSM_ICGMgrParamNotify(void *param)
{
    QOSM_CHECK_RETURN(QOSM_IsAutorateEnabled(), "autorate disable, don't switch bitrate automatically");
    QOSM_CHECK_RETURN(param != NULL, "param is null");

    QOSM_ICGMgrNotifyParam *notifyParam = (QOSM_ICGMgrNotifyParam *)param;

    if (notifyParam->type == NOTIFY_TYPE_EARPHONE) {
        for (uint8_t i = 0; i < notifyParam->supportedBitrateCnt; i++) {
            QOSM_LOGI("Earphone notify i: %hhu, supported bit rate: %hu", i, notifyParam->supportedBitrate[i]);
        }
    } else if (notifyParam->type == NOTIFY_TYPE_COEXIST) {
        QOSM_LOGI("Coexist notify max bit rate: %hu, duty cycle: %hhu",
            notifyParam->maxBitrate, notifyParam->dutyCycle);
        QOSM_UpdateDutyCycle(notifyParam->dutyCycle);
        atomic_store(&g_maxBitrate, notifyParam->maxBitrate);
    } else if (notifyParam->type == NOTIFY_TYPE_5G) {
        QOSM_LOGI("5G notify max bit rate changed");
    }

    QOSM_ICGMgrIterate(QOSM_ICGMgrParamNotifyInner, param);
}
