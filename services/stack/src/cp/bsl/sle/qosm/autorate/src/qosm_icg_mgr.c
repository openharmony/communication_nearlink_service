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

#include "qosm_icg_mgr.h"

#include <stdatomic.h>

#include "byte_codec.h"
#include "cm_def.h"
#include "cm_errno.h"
#include "cm_icb_api.h"
#include "cm_icb_mgr.h"
#include "cm_logic_link_api.h"
#include "cm_inner_api.h"
#include "cp_worker.h"
#include "qosm_errno.h"
#include "qosm_log.h"
#include "qosm_table_mgr.h"
#include "sdf_dlist.h"
#include "sdf_mem.h"
#include "securec.h"
#include "qosm_audio_dfx.h"
#include "nlstk_cfgdb_api.h"
#include "qosm_icg_types.h"
#include "qosm_icg_callback.h"
#include "qosm_autorate_notify.h"
#include "qosm_autorate_report.h"
#include "qosm_autorate_timeout_process.h"
#include "common_ext_func_wrapper.h"
#include "qosm_ext_func_wrapper.h"
#include "dli_reg_ext_func.h"
#include "dli_cmd_struct.h"
#include "dli_errno.h"

#define QOSM_MAX_RECOMMEND_ENTER_5G_TIMES 3

struct __attribute__((packed)) QOSM_PowerLevelInfo {
    uint16_t connHandle;
    uint8_t level;
};

static SDF_DListHead_S g_qosICGList = {{&(g_qosICGList.list), &(g_qosICGList.list)}, 0};
static uint8_t g_connectSeq = QOSM_INVALID_CONNECT_SEQ;

static void QOSM_ICGMgrSetParamInner(QOSM_ICGMgrParam *autoRateParam, bool isResetParam);
static void QOSM_ICGMgrSetTestParamInner(QOSM_ICGMgrParam *autoRateParam, bool isResetParam);
static void QOSM_ICGMgrAddConnectionInner(QOSM_ICGInfo *icgInfo, QOSM_AutoRateConnParam *connParam);
static void QOSM_TrytoStopAddConnectionTimer(QOSM_ICGInfo *icgInfo, uint8_t connectSeq);
static void QOSM_DestroyDelayConnTask(QOSM_ICGInfo *icgInfo, bool shouldNotify);
static QOSM_ConnParam *QOSM_GetDelayConnTask(QOSM_ICGInfo *icgInfo, uint16_t connHandle);
static bool QOSM_HasICBInGroupConnectProcess(const QOSM_ICGInfo *icgInfo, uint8_t connectSeq);
static uint32_t QOSM_ICBAddConnectionProc(QOSM_ICGInfo *icgInfo, uint8_t connectSeq);
static void QOSM_ICGMgrSetLabel(QOSM_ICGInfo *icgInfo, CM_ICGLabelParam *labelParam, uint8_t connectSeq);
static CM_ICGLabelParam *QOSM_MallocICGLabelParam();
static void QOSM_FreeICGLabelParam(CM_ICGLabelParam *labelParam);
static uint32_t QOSM_ICGMgrRemoveParamInner(QOSM_ICGInfo *icgInfo);
static void QOSM_ExecuteDelayParamTask(QOSM_ICGInfo *icgInfo);

bool QOSM_HasICBConnecting(const QOSM_ICGInfo *icgInfo)
{
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connectStatus == LINK_LABEL_SETTING ||
            icgInfo->link[i].connectStatus == LINK_CONNECTING) {
            QOSM_LOGD("icb is connecting");
            return true;
        }
    }
    return false;
}

static QOSM_ICGInfo *QOSM_ICGListPrint(void)
{
    QOSM_ICGInfo *icgInfo = NULL;
    SDF_DListElmForeach(icgInfo, &g_qosICGList, entry) {
        QOSM_LOGI("type: %u, id: %u, qos index: %u, level count: %u, total level count: %u,"
            " link count: %u, connected count: %u",
            icgInfo->icbType, icgInfo->icgId, icgInfo->qosIndex, icgInfo->levelCnt, icgInfo->totalLevelCnt,
            icgInfo->linkCnt, icgInfo->connectedCnt);
        for (uint32_t i = 0; i < icgInfo->linkCnt; i++) {
            QOSM_LOGI("link [%u] icb id: %u, lcid: 0x%04x, conn handler: 0x%04x,"
                " ack rate over cnt: %u, connect status: %u",
                i, icgInfo->link[i].icbId, icgInfo->link[i].lcid, icgInfo->link[i].connHandle,
                icgInfo->link[i].ackRateOverCnt, icgInfo->link[i].connectStatus);
        }
    }
    return NULL;
}

static void QOSM_DestroyDelaySetParamTask(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo->delayParamTaskTimerId != QOSM_INVALID_TIMER_HANDLE) {
        CP_TimerDel(icgInfo->delayParamTaskTimerId);
        icgInfo->delayParamTaskTimerId = QOSM_INVALID_TIMER_HANDLE;
    }
    if (icgInfo->delaySetParam != NULL) {
        SDF_MemFree(icgInfo->delaySetParam);
        icgInfo->delaySetParam = NULL;
    }
}

static void QOSM_StopAddConnectionTimer(struct QosICBInfo *link)
{
    if (link->addConnectionTimerId != QOSM_INVALID_TIMER_HANDLE) {
        QOSM_LOGI("connHandle %hu, connectSeq %u, del timer", link->connHandle, link->connectSeq);
        CP_TimerDel(link->addConnectionTimerId);
        link->addConnectionTimerId = QOSM_INVALID_TIMER_HANDLE;
    }
}

static void QOSM_DestroyQosICGInfo(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo == NULL) {
        return;
    }
    QOSM_DestroyDelaySetParamTask(icgInfo);
    QOSM_DestroyDelayConnTask(icgInfo, true);
    QOSM_ChangeLabelStopTimer(icgInfo);
    QOSM_ICBUpLevelDelayStopTimer(icgInfo);
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        QOSM_StopAddConnectionTimer(&icgInfo->link[i]);
        if (icgInfo->link[i].retryConnectTimerId != QOSM_INVALID_TIMER_HANDLE) {
            CP_TimerDel(icgInfo->link[i].retryConnectTimerId);
            icgInfo->link[i].retryConnectTimerId = QOSM_INVALID_TIMER_HANDLE;
        }
    }
    SDF_MemFree(icgInfo->level);
    SDF_MemFree(icgInfo->link);
    SDF_MemFree(icgInfo);
}

static QOSM_ICGInfo *QOSM_NewQosICGInfo(const QOSM_ICGMgrParam *param)
{
    QOSM_ICGInfo *icgInfo = (QOSM_ICGInfo *)SDF_MemZalloc(sizeof(QOSM_ICGInfo));
    QOSM_CHECK_RETURN_RET(icgInfo != NULL, NULL, "malloc icg info failed");

    icgInfo->level = (struct QosLevelLabel *)SDF_MemZalloc(param->startParam.levelCnt * sizeof(struct QosLevelLabel));
    if (icgInfo->level == NULL) {
        QOSM_DestroyQosICGInfo(icgInfo);
        QOSM_LOGE("malloc level info failed");
        return NULL;
    }
    icgInfo->totalLevelCnt = param->startParam.levelCnt;

    icgInfo->link = (struct QosICBInfo *)SDF_MemZalloc(param->autorateParam.linkCnt * sizeof(struct QosICBInfo));
    if (icgInfo->link == NULL) {
        QOSM_DestroyQosICGInfo(icgInfo);
        QOSM_LOGE("malloc icb info failed");
        return NULL;
    }
    icgInfo->linkCnt = param->autorateParam.linkCnt;
    return icgInfo;
}

static void QOSM_SetQosICGInfo(QOSM_ICGInfo *icgInfo, const QOSM_ICGMgrParam *param, bool isTest, bool isResetParam)
{
    icgInfo->qosId = param->autorateParam.qosId;
    icgInfo->icgId = icgInfo->qosId; // 由于全局只有一个调用方，所以qosId充当icgId
    icgInfo->icbType = QOSM_GetICBTypeByIndex(param->autorateParam.qosIndex);
    icgInfo->qosIndex = param->autorateParam.qosIndex;
    icgInfo->levelCnt = 0;
    icgInfo->isTest = isTest;
    icgInfo->isResetParam = isResetParam;

    icgInfo->supportSubrate =
        CM_InnerGetRemotePrivateFeature(param->autorateParam.lcid[0], CM_PRIVATE_FEATURES_BIT_SUBRATE);
    icgInfo->supportAutorate =
        CM_InnerGetRemotePrivateFeature(param->autorateParam.lcid[0], CM_PRIVATE_FEATURES_BIT_AUTORATE);
    QOSM_LOGI("supportSubrate: %d, supportAutorate: %d", icgInfo->supportSubrate, icgInfo->supportAutorate);

    icgInfo->delaySetParam = NULL;
    icgInfo->delayConnParam = NULL;
    icgInfo->delayParamTaskTimerId = QOSM_INVALID_TIMER_HANDLE;
    icgInfo->delayConnTaskTimerId = QOSM_INVALID_TIMER_HANDLE;
    icgInfo->changeLableTimerId = QOSM_INVALID_TIMER_HANDLE;
    icgInfo->upLevelDelayTimerId = QOSM_INVALID_TIMER_HANDLE;
    icgInfo->qosParam = NULL;
    icgInfo->recommendEnter5GTimes = 0;
    QOSM_CopySupportedBitrate(icgInfo, param->autorateParam.supportedBitrate,
        param->autorateParam.supportedBitrateCnt);
}

static QOSM_ICGInfo *QOSM_CreateQosICGInfo(const QOSM_ICGMgrParam *param, bool isTest, bool isResetParam)
{
    QOSM_CHECK_RETURN_RET(param->startParam.levelCnt != 0 && param->startParam.levelCnt <= QOSM_AUTORATE_MAX_LEVEL_CNT,
        NULL, "level cnt is illegal");
    QOSM_CHECK_RETURN_RET(param->autorateParam.linkCnt != 0 &&
        param->autorateParam.linkCnt <= QOSM_AUTORATE_MAX_LINK_CNT, NULL, "link cnt is illegal");

    QOSM_ICGInfo *icgInfo = QOSM_NewQosICGInfo(param);
    QOSM_CHECK_RETURN_RET(icgInfo != NULL, NULL, "malloc icg info failed");

    QOSM_SetQosICGInfo(icgInfo, param, isTest, isResetParam);

    for (uint8_t i = 0, labelId = 0; i < param->startParam.levelCnt; i++) {
        icgInfo->level[i].qosParam = QOSM_GetQosParamByIndex(param->autorateParam.qosIndex, i);
        if (icgInfo->level[i].qosParam == NULL) {
            QOSM_DestroyQosICGInfo(icgInfo);
            QOSM_LOGE("qos param is null, qos index: %u, qos level index: %u", param->autorateParam.qosIndex, i);
            return NULL;
        }

        QOSM_SetLevelSupported(icgInfo, &icgInfo->level[i], param->autorateParam.supportedBitrateCnt,
            param->autorateParam.supportedBitrate, param->startParam.startLevel);

        if (!QOSM_IsLevelSupported(icgInfo, &icgInfo->level[i], param->startParam.startLevel)) {
            QOSM_LOGI("qosId %hhu, qos index %u, qos level %hu, bitrate %hu is not supported", icgInfo->qosId,
                icgInfo->qosIndex, icgInfo->level[i].qosParam->qosLevel, icgInfo->level[i].qosParam->downwardBitrate);
            continue;
        }

        if (isTest) {
            icgInfo->level[i].testLabelId = labelId++;
        }

        icgInfo->levelCnt++;

        if (icgInfo->qosParam != NULL) {
            continue;
        }
        if (icgInfo->level[i].qosParam->qosLevel == param->startParam.startLevel) {
            icgInfo->qosParam = icgInfo->level[i].qosParam;
        }
    }

    if (icgInfo->qosParam == NULL) {
        QOSM_DestroyQosICGInfo(icgInfo);
        QOSM_LOGE("not found start qos param, qosId %hhu, qos index %u", param->autorateParam.qosId,
            param->autorateParam.qosIndex);
        return NULL;
    }
    for (uint8_t i = 0, icbId = 0; i < param->autorateParam.linkCnt; i++) {
        icgInfo->link[i].icbId = icbId++;
        icgInfo->link[i].freqBand = QOSM_DEFAULT_BAND;
        icgInfo->link[i].retryConnectTimerId = QOSM_INVALID_TIMER_HANDLE;
        icgInfo->link[i].addConnectionTimerId = QOSM_INVALID_TIMER_HANDLE;
    }
    QOSM_LOGI("qosId %hhu, using level cnt: %hhu", icgInfo->qosId, icgInfo->levelCnt);

    SDF_DListEntryInit(&icgInfo->entry);
    return icgInfo;
}

QOSM_ICGInfo *QOSM_FindQosIcbInfoByQosId(uint8_t qosId)
{
    QOSM_ICGInfo *icgInfo = NULL;
    SDF_DListElmForeach(icgInfo, &g_qosICGList, entry) {
        if (icgInfo->qosId == qosId) {
            return icgInfo;
        }
    }
    return NULL;
}

QOSM_ICGInfo *QOSM_FindQosIcbInfoByIcgId(uint16_t icgId)
{
    QOSM_ICGInfo *icgInfo = NULL;
    SDF_DListElmForeach(icgInfo, &g_qosICGList, entry) {
        if (icgInfo->icgId == icgId) {
            return icgInfo;
        }
    }
    return NULL;
}

bool QOSM_IsICBConnected(const struct QosICBInfo *link)
{
    return link->connectStatus == LINK_CONNECTED || link->connectStatus == LINK_SUCCESS;
}

QOSM_ICGInfo *QOSM_FindQosIcbInfoByConnHandle(uint16_t connHandle, uint8_t *linkIndex)
{
    QOSM_ICGInfo *icgInfo = NULL;
    SDF_DListElmForeach(icgInfo, &g_qosICGList, entry) {
        for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
            if (icgInfo->link[i].connHandle != connHandle || !QOSM_IsICBConnected(&icgInfo->link[i])) {
                continue;
            }
            *linkIndex = i;
            return icgInfo;
        }
    }
    return NULL;
}

static void QOSM_ConfigICG(CM_ICBConnection *connection)
{
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByIcgId(connection->id);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        QOSM_NotifyParamChangedFailCb(connection->id, QOSM_PARAM_SETTED);
        return;
    }

    QOSM_LOGI("type: %u, id: %u, test param cbk cnt: %u, level cnt: %u",
        icgInfo->icbType, icgInfo->icgId, icgInfo->testParamCbkCnt, icgInfo->levelCnt);
    // set test param会回调多次，中途不会失败，收到最后一次回调才进入处理流程，若其中有一次失败则整体失败
    if (icgInfo->isTest) {
        if (connection->errorCode != CM_ICB_SUCCESS) {
            icgInfo->setTestParamFailed = true;
        }
        icgInfo->testParamCbkCnt++;
        if (icgInfo->testParamCbkCnt < icgInfo->levelCnt) {
            return;
        }
        icgInfo->testParamCbkCnt = 0;
    }
    if (connection->errorCode != CM_ICB_SUCCESS || (icgInfo->isTest && icgInfo->setTestParamFailed)) {
        QOSM_LOGE("failed, errorCode=%u", connection->errorCode);
        QOSM_NotifyParamChangedFailCb(icgInfo->qosId, QOSM_PARAM_SETTED);
        SDF_DListElmDel(&g_qosICGList, icgInfo, entry);
        QOSM_DestroyQosICGInfo(icgInfo);
        return;
    }
    if (icgInfo->linkCnt != connection->channelCnt || icgInfo->linkCnt == 0 ||
        icgInfo->linkCnt > QOSM_AUTORATE_MAX_LINK_CNT) {
        QOSM_LOGE("linkCnt=%d, connection->channelCnt=%d", icgInfo->linkCnt, connection->channelCnt);
        QOSM_NotifyParamChangedFailCb(icgInfo->qosId, QOSM_PARAM_SETTED);
        SDF_DListElmDel(&g_qosICGList, icgInfo, entry);
        QOSM_DestroyQosICGInfo(icgInfo);
        return;
    }
    icgInfo->gHandle = connection->gHandle;
    if (icgInfo->icbType == CM_IMB) {
        QOSM_LOGI("save ghandle 0x%04x of IMG", icgInfo->gHandle);
    }
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        icgInfo->link[i].connHandle = connection->channel[i].connHandle;
        QOSM_LOGI("save [%u] conn hander 0x%04x", i, icgInfo->link[i].connHandle);
    }
    QOSM_AutorateSetDspOn(false);
    QOSM_NotifyParamChangedSuccessCb(icgInfo, QOSM_PARAM_SETTED);
}

static void QOSM_IncreaseConnectSeq(void)
{
    g_connectSeq++;
    if (g_connectSeq == QOSM_INVALID_CONNECT_SEQ) {
        g_connectSeq++;
    }
}

static bool QOSM_IsLinkConnectStatusEqualInGroup(QOSM_ICGInfo *icgInfo, uint8_t connectSeq,
    QOSM_LinkConnectStatus connectStatus)
{
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connectSeq != connectSeq) {
            continue;
        }
        if (!icgInfo->link[i].isDisconnectedByAcb && icgInfo->link[i].connectStatus != connectStatus) {
            QOSM_LOGI("connHandle: 0x%4x, current status: %u, expected status: %u",
                icgInfo->link[i].connHandle, icgInfo->link[i].connectStatus, connectStatus);
            return false;
        }
    }
    return true;
}

static bool QOSM_IsLinkNone(const QOSM_ICGInfo *icgInfo, uint16_t connHandle)
{
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connHandle == connHandle && icgInfo->link[i].connectStatus == LINK_NONE) {
            return true;
        }
    }
    return false;
}

static void QOSM_SetLinkConnectStatus(QOSM_ICGInfo *icgInfo, uint16_t connHandle, QOSM_LinkConnectStatus status)
{
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connHandle == connHandle) {
            icgInfo->link[i].connectStatus = status;
        }
    }
}

static void QOSM_FreqBandRecoverIfNeed(void)
{
    if (!QOSM_IsLastEnableFreqBandByRecommend() || !QOSM_IsNeedRecoverFreqBandAbility()) {
        return;
    }

    QOSM_LOGI("set sle 5G to 5 for recovery");
    DLI_FreqBandExtParam freqParam = {QOSM_FREQ_BAND_2D4_5D8_ADAPTIVE, QOSM_ENABLE_FREQ_BAND_BY_RECOVER};
    if (DLI_GetExtFuncList()->enableFreqBandExt == NULL) {
        QOSM_LOGE("QOSM_FreqBandRecoverIfNeed failed, registerfunc is null");
        return;
    }
    uint32_t ret = DLI_GetExtFuncList()->enableFreqBandExt(&freqParam);
    if (ret != DLI_SUCCESS) {
        return;
    }
}

static void QOSM_RemoveICGParam(QOSM_ICGInfo *icgInfo)
{
    QOSM_FreqBandRecoverIfNeed();
    bool isAllDisconnected = true;
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connectStatus != LINK_NONE) {
            isAllDisconnected = false;
            break;
        }
    }
    if (!isAllDisconnected) {
        return;
    }
    if (icgInfo->delayConnParam != NULL) {
        QOSM_LOGI("has delay conn task");
        QOSM_ExecuteDelayConnTask(icgInfo);
        return;
    }
    QOSM_LOGI("all connection is removed, now remove param, icg type: %u, id: %u", icgInfo->icbType, icgInfo->icgId);
    if (QOSM_ICGMgrRemoveParamInner(icgInfo) != CM_SUCCESS) {
        QOSM_ExecuteDelayParamTask(icgInfo);
    }
    QOSM_AudioDfxStop();
}

static void QOSM_ResetICBLinkStatus(struct QosICBInfo *link)
{
    link->lcid = 0;
    link->isInGroupConnecting = false;
    link->establishFailCnt = 0;
    if (link->retryConnectTimerId != QOSM_INVALID_TIMER_HANDLE) {
        QOSM_LOGI("delete connHandle %hu retry connect timer", link->connHandle);
        CP_TimerDel(link->retryConnectTimerId);
        link->retryConnectTimerId = QOSM_INVALID_TIMER_HANDLE;
    }
    link->hasIcbDisconnectEvent = false;
    link->connectStatus = LINK_NONE;
    link->hasCreateICB = false;
}

static bool QOSM_ICBIsNeedDisconnect(struct QosICBInfo *link)
{
    if (link->connectStatus == LINK_CONNECTED || link->connectStatus == LINK_SUCCESS) {
        QOSM_LOGI("connHandle 0x%04x lcid 0x%04x is connected, need disconnect", link->connHandle, link->lcid);
        return true;
    }
    if (link->connectStatus == LINK_CONNECTING && (link->establishFailCnt == 0 ||
        (link->establishFailCnt == 1 && link->retryConnectTimerId == QOSM_INVALID_TIMER_HANDLE))) {
        QOSM_LOGI("connHandle 0x%04x lcid 0x%04x is connecting, need disconnect", link->connHandle, link->lcid);
        return true;
    }
    return false;
}

static bool QOSM_HasICBCreate(QOSM_ICGInfo *icgInfo, uint8_t connectSeq)
{
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connectSeq != connectSeq) {
            continue;
        }
        if (icgInfo->link[i].connectStatus == LINK_LABEL_SETTING ||
            (icgInfo->link[i].connectStatus == LINK_CONNECTING && !icgInfo->link[i].hasCreateICB)) {
            return false;
        }
    }
    return true;
}

static void QOSM_TerminateConnectProcess(QOSM_ICGInfo *icgInfo, uint8_t linkIndex, bool shouldReset)
{
    QOSM_LinkConnectStatus connectStatus = icgInfo->link[linkIndex].connectStatus;
    uint16_t connHandle = icgInfo->link[linkIndex].connHandle;
    uint16_t lcid = icgInfo->link[linkIndex].lcid;

    bool hasCreateICB = QOSM_HasICBCreate(icgInfo, icgInfo->link[linkIndex].connectSeq);
    bool needDisconnect = QOSM_ICBIsNeedDisconnect(&icgInfo->link[linkIndex]);

    if (shouldReset) {
        QOSM_ResetICBLinkStatus(&icgInfo->link[linkIndex]);
    }

    if (connectStatus == LINK_LABEL_SETTING || !hasCreateICB) {
        if (icgInfo->link[linkIndex].isDisconnectedByAcb &&
            QOSM_HasICBInGroupConnectProcess(icgInfo, icgInfo->link[linkIndex].connectSeq)) {
            // 检查icg底下的所有链路是否都已set label成功
            if (QOSM_IsLinkConnectStatusEqualInGroup(icgInfo, icgInfo->link[linkIndex].connectSeq, LINK_CONNECTING)) {
                QOSM_ICBAddConnectionProc(icgInfo, icgInfo->link[linkIndex].connectSeq);
            }
        } else {
            QOSM_RemoveICGParam(icgInfo);
        }
    } else if (needDisconnect) {
        CM_ICBConnectionParam icbParam = {};
        icbParam.type = icgInfo->icbType;
        icbParam.id = icgInfo->icgId;
        icbParam.channelCnt = 1;
        CM_ICBChannel channel = {};
        channel.connHandle = connHandle;
        channel.lcid = lcid;
        icbParam.channel = &channel;
        uint32_t ret = CM_ICBDelConnection(&icbParam);
        QOSM_LOGI("disconnect, id: %hu, conn handle: 0x%04x, lcid: 0x%04x, ret: %u",
            icgInfo->icgId, connHandle, lcid, ret);
        return;
    }
    QOSM_LOGI("connHandle %4x, lcid %4x, no need disconnect", connHandle, lcid);
}

static void QOSM_RecalculateConnectedCnt(QOSM_ICGInfo *icgInfo)
{
    icgInfo->connectedCnt = 0;
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (QOSM_IsICBConnected(&icgInfo->link[i])) {
            icgInfo->connectedCnt++;
        }
    }
}

static void QOSM_ConnectFailInGroup(QOSM_ICGInfo *icgInfo, uint8_t connectSeq)
{
    QOSM_ConnParam link[QOSM_AUTORATE_MAX_LINK_CNT] = {};
    uint8_t linkCnt = 0;
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connectSeq != connectSeq) {
            continue;
        }
        link[linkCnt].connHandle = icgInfo->link[i].connHandle;
        linkCnt++;

        QOSM_LOGI("link [%u] conn handle 0x%4x lcid 0x%04x connect fail, status: %u, connect seq: %u",
            i, icgInfo->link[i].connHandle, icgInfo->link[i].lcid, icgInfo->link[i].connectStatus, connectSeq);

        QOSM_TerminateConnectProcess(icgInfo, i, true);
    }

    QOSM_TrytoStopAddConnectionTimer(icgInfo, connectSeq);

    QOSM_RecalculateConnectedCnt(icgInfo);
    QOSM_NotifyConnCbk(icgInfo, QOSM_CONNECTION_ADDED, QOSM_FAIL, link, linkCnt);
}

/*
 * 1、只要有一个链路建链失败，就把同一批次的其他链路都做失败处理，然后callback
 * 2、将connectStatus置为LINK_NONE（用于后续的event上报时过滤，避免重复callback）
 */
static void QOSM_ConnectSuccessInGroup(QOSM_ICGInfo *icgInfo, uint8_t connectSeq)
{
    QOSM_ConnParam link[QOSM_AUTORATE_MAX_LINK_CNT] = {};
    uint8_t linkCnt = 0;
    QOSM_ConnParam discconnectLink[QOSM_AUTORATE_MAX_LINK_CNT] = {};
    uint8_t discconnectLinkCnt = 0;
    for (uint8_t i = 0; i < icgInfo->linkCnt && i < QOSM_AUTORATE_MAX_LINK_CNT; i++) {
        if (icgInfo->link[i].connectSeq != connectSeq) {
            continue;
        }
        if (icgInfo->link[i].connectStatus != LINK_CONNECTED) {
            QOSM_ResetICBLinkStatus(&icgInfo->link[i]);
            continue;
        }
        icgInfo->link[i].connectStatus = LINK_SUCCESS;
        link[linkCnt].connHandle = icgInfo->link[i].connHandle;
        link[linkCnt].lcid = icgInfo->link[i].lcid;
        QOSM_LOGI("connHandle: %u, lcid: %u", link[linkCnt].connHandle, link[linkCnt].lcid);
        linkCnt++;

        if (icgInfo->link[i].hasIcbDisconnectEvent) {
            discconnectLink[discconnectLinkCnt].connHandle = icgInfo->link[i].connHandle;
            discconnectLink[discconnectLinkCnt].lcid = icgInfo->link[i].lcid;
            discconnectLinkCnt++;
            QOSM_LOGI("conn handle 0x%4x lcid 0x%04x has icb disconnect event, status: %u, connect seq: %u",
                icgInfo->link[i].connHandle, icgInfo->link[i].lcid, icgInfo->link[i].connectStatus, connectSeq);
            QOSM_ResetICBLinkStatus(&icgInfo->link[i]);
        }
    }
    QOSM_TrytoStopAddConnectionTimer(icgInfo, connectSeq);
    QOSM_NotifyConnCbk(icgInfo, QOSM_CONNECTION_ADDED, QOSM_SUCCESS, link, linkCnt);
    if (discconnectLinkCnt != 0) {
        QOSM_NotifyConnCbk(icgInfo, QOSM_CONNECTION_DELETED, QOSM_SUCCESS, discconnectLink, discconnectLinkCnt);
        QOSM_RecalculateConnectedCnt(icgInfo);
    }
}

static void QOSM_DfxUpdateConnnected(uint16_t connHandle, uint16_t lcid)
{
    CM_LogicLink_S link = {};
    if (CM_GetLogicLinkByLcid(lcid, &link) != CM_SUCCESS) {
        return;
    }

    struct QOSM_AudioDfxConn conn = {};
    conn.connHandle = connHandle;
    conn.lcid = lcid;
    if (memcpy_s(conn.addr, sizeof(conn.addr), link.addr.addr, sizeof(link.addr.addr)) != EOK) {
        return;
    }
    QOSM_AudioDfxUpdateConn(&conn, true);
}

static void QOSM_RetryConnectTimeoutCbk(void *arg)
{
    uint16_t input = (uint16_t)(uintptr_t)arg;
    uint8_t qosId = (uint8_t)(input >> QOSM_QOSID_OFFSET);
    uint8_t linkIndex = (uint8_t)input;
    QOSM_LOGE("qos id %hhu, retry connect timeout", qosId);

    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(qosId);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        return;
    }

    if (linkIndex >= icgInfo->linkCnt) {
        QOSM_LOGE("linkIndex %hhu >= linkCnt %hhu", linkIndex, icgInfo->linkCnt);
        return;
    }

    icgInfo->link[linkIndex].retryConnectTimerId = QOSM_INVALID_TIMER_HANDLE;
    if ((icgInfo->link[linkIndex].connectStatus != LINK_LABEL_SETTING &&
        icgInfo->link[linkIndex].connectStatus != LINK_CONNECTING) || icgInfo->link[linkIndex].isDisconnectedByAcb) {
        QOSM_LOGE("connHandle 0x%04x, lcid 0x%04x, connectStatus %u, isDisconnectedByAcb %u",
            icgInfo->link[linkIndex].connHandle, icgInfo->link[linkIndex].lcid,
            icgInfo->link[linkIndex].connectStatus, icgInfo->link[linkIndex].isDisconnectedByAcb);
        return;
    }

    if (icgInfo->link[linkIndex].connectStatus == LINK_LABEL_SETTING) {
        CM_ICGLabelParam *labelParam = QOSM_MallocICGLabelParam();
        QOSM_CHECK_RETURN(labelParam != NULL, "malloc labelParam failed");
        labelParam->type = icgInfo->icbType;
        labelParam->id = icgInfo->icgId;
        labelParam->icbCnt = 1;
        labelParam->icb[0].connHandle = icgInfo->link[linkIndex].connHandle;
        labelParam->icb[0].lcid = icgInfo->link[linkIndex].lcid;
        QOSM_ICGMgrSetLabel(icgInfo, labelParam, icgInfo->link[linkIndex].connectSeq);
        QOSM_FreeICGLabelParam(labelParam);
        return;
    }
    CM_ICBConnectionParam param = {};
    param.type = icgInfo->icbType;
    param.id = icgInfo->icgId;
    param.channelCnt = 1;
    CM_ICBChannel channel = {};
    channel.connHandle = icgInfo->link[linkIndex].connHandle;
    channel.lcid = icgInfo->link[linkIndex].lcid;
    param.channel = &channel;

    uint32_t ret = CM_ICBAddConnection(&param, icgInfo->supportAutorate);
    QOSM_LOGI("qos id %hhu, retry connect connHandle %u, ret %u", qosId, channel.connHandle, ret);
}

static bool QOSM_IsAllICBFailIngroup(QOSM_ICGInfo *icgInfo, uint8_t linkIndex)
{
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connectSeq != icgInfo->link[linkIndex].connectSeq) {
            continue;
        }
        if (icgInfo->link[i].establishFailCnt <= QOSM_MAX_ESTABLISH_FAIL_CNT) {
            return false;
        }
    }
    return true;
}

static void QOSM_ICBRetryConnect(QOSM_ICGInfo *icgInfo, uint8_t linkIndex)
{
    if (icgInfo->link[linkIndex].retryConnectTimerId != QOSM_INVALID_TIMER_HANDLE) {
        QOSM_LOGE("connHandle 0x%04x, lcid 0x%04x, retry connect timer is running",
            icgInfo->link[linkIndex].connHandle, icgInfo->link[linkIndex].lcid);
        return;
    }
    if ((icgInfo->link[linkIndex].connectStatus != LINK_LABEL_SETTING &&
        icgInfo->link[linkIndex].connectStatus != LINK_CONNECTING) || icgInfo->link[linkIndex].isDisconnectedByAcb) {
        QOSM_LOGE("connHandle 0x%04x, lcid 0x%04x, connectStatus %u, isDisconnectedByAcb %u",
            icgInfo->link[linkIndex].connHandle, icgInfo->link[linkIndex].lcid,
            icgInfo->link[linkIndex].connectStatus, icgInfo->link[linkIndex].isDisconnectedByAcb);
        return;
    }
    SDF_TimerParam param = {};
    param.period = false;
    param.expires = QOSM_RETRY_CONNECT_TIMEOUT_MS;
    param.args = (void *)(uintptr_t)(((uint8_t)icgInfo->qosId << QOSM_QOSID_OFFSET) | linkIndex);
    param.callback = QOSM_RetryConnectTimeoutCbk;
    uint32_t ret = CP_TimerAdd(&icgInfo->link[linkIndex].retryConnectTimerId, &param);
    QOSM_LOGI("add retry connect timer, qos id %hhu, connHandle 0x%04x, lcid 0x%04x, ret %u",
        icgInfo->qosId, icgInfo->link[linkIndex].connHandle, icgInfo->link[linkIndex].lcid, ret);
}

static void QOSM_CheckRetryConnectInGroup(QOSM_ICGInfo *icgInfo, struct QosICBInfo *link)
{
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connectSeq != link->connectSeq ||
            icgInfo->link[i].connHandle == link->connHandle) {
            continue;
        }
        if (icgInfo->link[i].establishFailCnt > 0 &&
            icgInfo->link[i].establishFailCnt <= QOSM_MAX_ESTABLISH_FAIL_CNT ) {
            QOSM_ICBRetryConnect(icgInfo, i);
            return;
        }
    }
}

static bool QOSM_HasICBRetryConnect(QOSM_ICGInfo *icgInfo, uint8_t connectSeq)
{
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connectSeq != connectSeq) {
            continue;
        }
        if (icgInfo->link[i].retryConnectTimerId != QOSM_INVALID_TIMER_HANDLE) {
            return true;
        }
    }
    return false;
}

static void QOSM_SetupICBFailProc(QOSM_ICGInfo *icgInfo, uint8_t linkIndex)
{
    icgInfo->link[linkIndex].hasCreateICB = false;
    icgInfo->link[linkIndex].establishFailCnt++;
    if (icgInfo->link[linkIndex].establishFailCnt > QOSM_MAX_ESTABLISH_FAIL_CNT) {
        QOSM_LOGE("connHandle 0x%4x, lcid 0x%04x, establish fail too much times, give up",
            icgInfo->link[linkIndex].connHandle, icgInfo->link[linkIndex].lcid);
        if (QOSM_IsAllICBFailIngroup(icgInfo, linkIndex)) {
            QOSM_ConnectFailInGroup(icgInfo, icgInfo->link[linkIndex].connectSeq);
        } else {
            QOSM_CheckRetryConnectInGroup(icgInfo, &icgInfo->link[linkIndex]);
        }
        return;
    }
    if (QOSM_HasICBRetryConnect(icgInfo, icgInfo->link[linkIndex].connectSeq)) {
        return;
    }
    QOSM_ICBRetryConnect(icgInfo, linkIndex);
}

// 当前未处理被动建链（即收到connect req事件，状态置为connecting，connectSeq从g_connectSeq+1/2*UINT8_MAX开始分配）
static void QOSM_SetupICB(CM_ICBConnection *connection)
{
    QOSM_CHECK_RETURN(connection->channelCnt != 0 && connection->channelCnt <= QOSM_AUTORATE_MAX_LINK_CNT,
        "connection channel count: %u is invalid", connection->channelCnt);
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByIcgId(connection->id);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        uint16_t connHandle[QOSM_AUTORATE_MAX_LINK_CNT] = {};
        for (uint8_t i = 0; i < connection->channelCnt && i < QOSM_AUTORATE_MAX_LINK_CNT; i++) {
            connHandle[i] = connection->channel[i].connHandle;
        }
        QOSM_NotifyConnFailCbk(connection->id, QOSM_CONNECTION_ADDED, connHandle, connection->channelCnt);
        return;
    }

    for (uint8_t i = 0; i < connection->channelCnt; i++) {
        for (uint8_t j = 0; j < icgInfo->linkCnt; j++) {
            if (connection->channel[i].connHandle != icgInfo->link[j].connHandle) {
                continue;
            }

            if (icgInfo->link[j].connectStatus == LINK_NONE) {
                QOSM_LOGI("connHandle: %u no need to do anything", connection->channel[i].connHandle);
                continue;
            }

            if (connection->errorCode != CM_ICB_SUCCESS) {
                QOSM_LOGE("conn handler 0x%04x connect failed, errorCode=%u", connection->channel[i].connHandle,
                    connection->errorCode);
                QOSM_SetupICBFailProc(icgInfo, j);
                return;
            }

            icgInfo->connectedCnt++;
            icgInfo->link[j].isInGroupConnecting = false;
            icgInfo->link[j].connectStatus = LINK_CONNECTED;
            icgInfo->link[j].lcid = connection->channel[i].lcid;
            QOSM_LOGI("conn handler 0x%04x connected, lcid: 0x%04x, total connected cnt: %u",
                connection->channel[i].connHandle, connection->channel[i].lcid, icgInfo->connectedCnt);

            QOSM_DfxUpdateConnnected(connection->channel[i].connHandle, connection->channel[i].lcid);

            if (QOSM_IsLinkConnectStatusEqualInGroup(icgInfo, icgInfo->link[j].connectSeq, LINK_CONNECTED)) {
                QOSM_ConnectSuccessInGroup(icgInfo, icgInfo->link[j].connectSeq);
                return;
            }
            QOSM_CheckRetryConnectInGroup(icgInfo, &icgInfo->link[j]);
        }
    }
}

static void QOSM_StartICB(CM_ICBConnection *connection)
{
    QOSM_CHECK_RETURN(connection->channelCnt != 0 && connection->channelCnt <= QOSM_AUTORATE_MAX_LINK_CNT,
        "connection channel count: %u is invalid", connection->channelCnt);
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByIcgId(connection->id);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        QOSM_NotifyDataPathChangedCb(connection->id, QOSM_DATAPATH_ADDED, QOSM_FAIL, 0, 0);
        return;
    }
    if (connection->errorCode != CM_ICB_SUCCESS) {
        QOSM_LOGE("errorCode=%u", connection->errorCode);
        QOSM_NotifyDataPathChangedCb(icgInfo->qosId, QOSM_DATAPATH_ADDED, QOSM_FAIL, 0, 0);
        return;
    }
    for (uint8_t i = 0; i < connection->channelCnt; i++) {
        QOSM_NotifyDataPathChangedCb(icgInfo->qosId, QOSM_DATAPATH_ADDED, QOSM_SUCCESS,
            connection->channel[i].connHandle, connection->channel[i].direction);
    }
}

void QOSM_ExecuteDelayConnTask(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo->delayConnTaskTimerId != QOSM_INVALID_TIMER_HANDLE) {
        CP_TimerDel(icgInfo->delayConnTaskTimerId);
        icgInfo->delayConnTaskTimerId = QOSM_INVALID_TIMER_HANDLE;
    }
    if (icgInfo->delayConnParam == NULL) {
        return;
    }
    QOSM_LOGI("start delay connection");
    QOSM_ICGMgrAddConnectionInner(icgInfo, icgInfo->delayConnParam);
    SDF_MemFree(icgInfo->delayConnParam->link);
    SDF_MemFree(icgInfo->delayConnParam);
    icgInfo->delayConnParam = NULL;
}

static void QOSM_StopICB(CM_ICBConnection *connection)
{
    QOSM_CHECK_RETURN(connection->channelCnt != 0 && connection->channelCnt <= QOSM_AUTORATE_MAX_LINK_CNT,
        "connection channel count: %u is invalid", connection->channelCnt);
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByIcgId(connection->id);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        QOSM_NotifyDataPathChangedCb(connection->id, QOSM_DATAPATH_DELETED, QOSM_FAIL, 0, 0);
        return;
    }
    if (connection->errorCode != CM_ICB_SUCCESS) {
        QOSM_LOGE("errorCode=%u", connection->errorCode);
        QOSM_NotifyDataPathChangedCb(icgInfo->qosId, QOSM_DATAPATH_DELETED, QOSM_FAIL, 0, 0);
        return;
    }
    for (uint8_t i = 0; i < connection->channelCnt; i++) {
        QOSM_NotifyDataPathChangedCb(icgInfo->qosId, QOSM_DATAPATH_DELETED, QOSM_SUCCESS,
            connection->channel[i].connHandle, connection->channel[i].direction);
    }
}

static void QOSM_ExecuteDelayParamTask(QOSM_ICGInfo *icgInfo)
{
    if (icgInfo->delaySetParam != NULL) {
        QOSM_LOGI("now set param, icg type: %u, id: %u", icgInfo->icbType, icgInfo->icgId);
        if (icgInfo->isTest) {
            QOSM_ICGMgrSetTestParamInner(icgInfo->delaySetParam, icgInfo->isResetParam);
        } else {
            QOSM_ICGMgrSetParamInner(icgInfo->delaySetParam, icgInfo->isResetParam);
        }
    }
    SDF_DListElmDel(&g_qosICGList, icgInfo, entry);
    QOSM_DestroyQosICGInfo(icgInfo);
}

static bool QOSM_OnlyCurrentICBInGroupConnectProcess(const QOSM_ICGInfo *icgInfo, uint8_t connectSeq,
    uint16_t connHandle)
{
    bool onlyCurrent = false;
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connectSeq != connectSeq) {
            continue;
        }
        if (icgInfo->link[i].isInGroupConnecting) {
            if (icgInfo->link[i].connHandle != connHandle) {
                return false;
            } else {
                onlyCurrent = true;
            }
        }
    }
    return onlyCurrent;
}

static bool QOSM_HasICBInGroupConnectProcess(const QOSM_ICGInfo *icgInfo, uint8_t connectSeq)
{
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connectSeq != connectSeq) {
            continue;
        }
        if (icgInfo->link[i].isInGroupConnecting) {
            return true;
        }
    }
    return false;
}

static bool QOSM_GroupConnectProcess(CM_ICBConnection *connection, QOSM_ICGInfo *icgInfo, uint8_t linkIndex)
{
    if (connection->disconnectType == CM_LINK_ICB &&
        QOSM_HasICBInGroupConnectProcess(icgInfo, icgInfo->link[linkIndex].connectSeq)) {
        if (QOSM_OnlyCurrentICBInGroupConnectProcess(icgInfo, icgInfo->link[linkIndex].connectSeq,
            icgInfo->link[linkIndex].connHandle)) {
            return false;
        }
        icgInfo->link[linkIndex].hasCreateICB = false;
        icgInfo->link[linkIndex].hasIcbDisconnectEvent = true;
        QOSM_LOGI("connHandle 0x%04x, lcid 0x%04x, has icb disconnect event",
            icgInfo->link[linkIndex].connHandle, icgInfo->link[linkIndex].lcid);
        return true;
    }

    // 同一组有链路还处于组建链流程，此时收到异步链路断链事件，回调对应的同步链路建链失败
    if (connection->disconnectType == CM_LINK_ACB &&
        QOSM_HasICBInGroupConnectProcess(icgInfo, icgInfo->link[linkIndex].connectSeq)) {
        icgInfo->link[linkIndex].hasIcbDisconnectEvent = false;
        if (QOSM_OnlyCurrentICBInGroupConnectProcess(icgInfo, icgInfo->link[linkIndex].connectSeq,
            icgInfo->link[linkIndex].connHandle)) {
            QOSM_ConnectSuccessInGroup(icgInfo, icgInfo->link[linkIndex].connectSeq);
        }
        QOSM_ResetICBLinkStatus(&icgInfo->link[linkIndex]);

        icgInfo->link[linkIndex].isDisconnectedByAcb = true; // 用于后续建链流程中过滤该链路
        QOSM_CheckRetryConnectInGroup(icgInfo, &icgInfo->link[linkIndex]);
        QOSM_NotifyConnFailCbk(connection->id, QOSM_CONNECTION_ADDED, &icgInfo->link[linkIndex].connHandle, 1);

        // 检查icg底下的所有链路是否都已set label成功
        if (QOSM_IsLinkConnectStatusEqualInGroup(icgInfo, icgInfo->link[linkIndex].connectSeq, LINK_CONNECTING) &&
            !QOSM_HasICBCreate(icgInfo, icgInfo->link[linkIndex].connectSeq)) {
            QOSM_LOGI("continue add connection");
            QOSM_ICBAddConnectionProc(icgInfo, icgInfo->link[linkIndex].connectSeq);
        }
        return true;
    }
    return false;
}

static void QOSM_DestroyICB(CM_ICBConnection *connection)
{
    QOSM_CHECK_RETURN(connection->channelCnt != 0 && connection->channelCnt <= QOSM_AUTORATE_MAX_LINK_CNT,
        "connection channel count: %u is invalid", connection->channelCnt);
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByIcgId(connection->id);
    uint16_t connHandle[QOSM_AUTORATE_MAX_LINK_CNT] = {};
    for (uint8_t i = 0; i < connection->channelCnt && i < QOSM_AUTORATE_MAX_LINK_CNT; i++) {
        connHandle[i] = connection->channel[i].connHandle;
    }
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        QOSM_NotifyConnFailCbk(connection->id, QOSM_CONNECTION_DELETED, connHandle, connection->channelCnt);
        return;
    }

    for (uint8_t i = 0; i < connection->channelCnt; i++) {
        QOSM_LOGI("process conn handle 0x%04x disconnection", connection->channel[i].connHandle);
        for (uint8_t j = 0; j < icgInfo->linkCnt; j++) {
            if (connection->channel[i].connHandle != icgInfo->link[j].connHandle) {
                continue;
            }

            if (QOSM_GroupConnectProcess(connection, icgInfo, j)) {
                continue;
            }

            if (connection->errorCode != CM_ICB_SUCCESS) {
                QOSM_LOGE("errorCode=%u", connection->errorCode);
                QOSM_NotifyConnFailCbk(connection->id, QOSM_CONNECTION_DELETED, &icgInfo->link[j].connHandle, 1);
                continue;
            }

            QOSM_TrytoStopAddConnectionTimer(icgInfo, icgInfo->link[j].connectSeq);

            if (QOSM_IsICBConnected(&icgInfo->link[j])) {
                icgInfo->connectedCnt = QOSM_MAX(0, icgInfo->connectedCnt - 1);
            }

            // 检查同一组底下是否除了这条链路外，都更新完了，若都更新完了，走更新结束流程
            if (icgInfo->updatedCnt != 0 && icgInfo->updatedCnt == icgInfo->connectedCnt) {
                QOSM_UpdateICGComplete(icgInfo);
            }

            QOSM_ConnParam link = {};
            link.connHandle = connection->channel[i].connHandle;
            link.lcid = connection->channel[i].lcid;
            QOSM_NotifyConnCbk(icgInfo, QOSM_CONNECTION_DELETED, QOSM_SUCCESS, &link, 1);
            QOSM_ResetICBLinkStatus(&icgInfo->link[j]);
            QOSM_LOGI("disconnection done, conn handle 0x%04x, lcid 0x%04x", link.connHandle, link.lcid);

            struct QOSM_AudioDfxConn conn = {};
            conn.connHandle = connection->channel[i].connHandle;
            QOSM_AudioDfxUpdateConn(&conn, false);
        }
    }
    QOSM_RemoveICGParam(icgInfo);
}

static void QOSM_DestroyDelayConnTask(QOSM_ICGInfo *icgInfo, bool shouldNotify)
{
    if (icgInfo->delayConnTaskTimerId != QOSM_INVALID_TIMER_HANDLE) {
        CP_TimerDel(icgInfo->delayConnTaskTimerId);
        icgInfo->delayConnTaskTimerId = QOSM_INVALID_TIMER_HANDLE;
    }
    if (icgInfo->delayConnParam != NULL) {
        if (shouldNotify) {
            QOSM_NotifyConnCbk(icgInfo, QOSM_CONNECTION_ADDED, QOSM_FAIL, icgInfo->delayConnParam->link,
                icgInfo->delayConnParam->linkCnt);
        }
        SDF_MemFree(icgInfo->delayConnParam->link);
        SDF_MemFree(icgInfo->delayConnParam);
        icgInfo->delayConnParam = NULL;
    }
}

static void QOSM_UnConfigICG(CM_ICBConnection *connection)
{
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByIcgId(connection->id);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        QOSM_NotifyParamChangedFailCb(connection->id, QOSM_PARAM_REMOVED);
        return;
    }

    if (!icgInfo->isRemovingParam) {
        QOSM_LOGI("has not removed param but receive remove event");
        return;
    }
    icgInfo->isRemovingParam = false;

    if (icgInfo->isResetParam) {
        QOSM_LOGI("don't need to notify upper and then set param");
        icgInfo->isResetParam = false;
        QOSM_ExecuteDelayParamTask(icgInfo);
        return;
    }

    QOSM_LOGI("errorCode=%u", connection->errorCode);
    if (connection->errorCode != CM_ICB_SUCCESS) {
        QOSM_NotifyParamChangedFailCb(icgInfo->qosId, QOSM_PARAM_REMOVED);
    } else {
        QOSM_NotifyParamChangedSuccessCb(icgInfo, QOSM_PARAM_REMOVED);
    }
    QOSM_ExecuteDelayParamTask(icgInfo);
}

typedef struct {
    CM_ICBConnectionState state;
    void (*func)(CM_ICBConnection *connection);
} QOSM_ICBStateFunc;

static QOSM_ICBStateFunc g_icbStateFunc[] = {
    {CM_ICB_STATE_IOG_CREATED, QOSM_ConfigICG},
    {CM_ICB_STATE_IMG_CREATED, QOSM_ConfigICG},
    {CM_ICB_STATE_IOB_CREATED, QOSM_SetupICB},
    {CM_ICB_STATE_IMB_CREATED, QOSM_SetupICB},
    {CM_ICB_STATE_ICB_DATA_PATH_SETUP, QOSM_StartICB},
    {CM_ICB_STATE_IOG_UPDATED, QOSM_UpdateICG},
    {CM_ICB_STATE_IMG_UPDATED, QOSM_UpdateICG},
    {CM_ICB_STATE_ICB_DATA_PATH_REMOVED, QOSM_StopICB},
    {CM_ICB_STATE_ICB_DELETED, QOSM_DestroyICB},
    {CM_ICB_STATE_IOG_REMOVED, QOSM_UnConfigICG},
    {CM_ICB_STATE_IMG_REMOVED, QOSM_UnConfigICG},
};

static void QOSM_ICBConnectionCbk(CM_ICBConnection *connection)
{
    QOSM_CHECK_RETURN(connection != NULL, "connection is NULL");
    QOSM_LOGI("Set Label id call back ret = %d", connection->errorCode);
    static uint8_t funcs = sizeof(g_icbStateFunc) / sizeof(QOSM_ICBStateFunc);
    for (uint8_t i = 0; i < funcs; i++) {
        if (connection->state == g_icbStateFunc[i].state) {
            g_icbStateFunc[i].func(connection);
            return;
        }
    }
}

static bool QOSM_IsQosParamEqual(const QOSM_LinkParam *linkParam, const CM_ICBLabelReportParam *reportParam,
    const CM_ICBLabel *labelParam)
{
    if (linkParam->icbInterval != reportParam->icbInterval ||
        linkParam->bnG2T != reportParam->bnG2T ||
        linkParam->bnT2G != reportParam->bnT2G ||
        linkParam->ftG2T != reportParam->ftG2T ||
        linkParam->ftT2G != reportParam->ftT2G) {
        return false;
    }
    if (linkParam->phyG2T != labelParam->txPhy ||
        linkParam->phyT2G != labelParam->rxPhy ||
        linkParam->mcsG2T != labelParam->txMcs ||
        linkParam->mcsT2G != labelParam->rxMcs ||
        linkParam->frameG2T != labelParam->txFrame||
        linkParam->frameT2G != labelParam->rxFrame ||
        linkParam->maxSduG2T != labelParam->maxSduG2T ||
        linkParam->maxSduT2G != labelParam->maxSduT2G ||
        linkParam->maxPduG2T != labelParam->maxPduG2T ||
        linkParam->maxPduT2G != labelParam->maxPduT2G ||
        linkParam->nse != labelParam->nse) {
        return false;
    }
    return true;
}

uint8_t QOSM_GetRealLabelId(QOSM_ICGInfo *icgInfo, struct QosLevelLabel *level)
{
    return icgInfo->isTest ? level->testLabelId : level->labelId;
}

bool QOSM_GetLabelId(QOSM_ICGInfo *icgInfo, QOSM_LinkParam *qosParam, uint8_t *labelId)
{
    for (uint8_t i = 0; i < icgInfo->totalLevelCnt; i++) {
        if (memcmp(qosParam, icgInfo->level[i].qosParam, sizeof(QOSM_LinkParam)) != EOK) {
            continue;
        }
        *labelId = QOSM_GetRealLabelId(icgInfo, &icgInfo->level[i]);
        return true;
    }
    return false;
}

static uint32_t QOSM_ICBAddConnectionProc(QOSM_ICGInfo *icgInfo, uint8_t connectSeq)
{
    if (icgInfo->linkCnt == 0) {
        QOSM_LOGE("icg link cnt is 0");
        return QOSM_FAIL;
    }
    CM_ICBConnectionParam param = {};
    param.type = icgInfo->icbType;
    param.id = icgInfo->icgId;
    param.channelCnt = 0;
    CM_ICBChannel channel[icgInfo->linkCnt];
    param.channel = channel;
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connectStatus != LINK_CONNECTING || icgInfo->link[i].connectSeq != connectSeq) {
            QOSM_LOGI("skip, connHandle %u, connectStatus %u, connectSeq %u", icgInfo->link[i].connHandle,
                icgInfo->link[i].connectStatus, icgInfo->link[i].connectSeq);
            continue;
        }

        if (icgInfo->link[i].retryConnectTimerId != QOSM_INVALID_TIMER_HANDLE) {
            QOSM_LOGI("delete connHandle %hu retry connect timer", icgInfo->link[i].connHandle);
            CP_TimerDel(icgInfo->link[i].retryConnectTimerId);
            icgInfo->link[i].retryConnectTimerId = QOSM_INVALID_TIMER_HANDLE;
        }

        channel[param.channelCnt].connHandle = icgInfo->link[i].connHandle;
        channel[param.channelCnt].lcid = icgInfo->link[i].lcid;
        QOSM_LOGI("connHandle: %u, lcid: %u", channel[param.channelCnt].connHandle, channel[param.channelCnt].lcid);
        param.channelCnt++;
    }

    if (param.channelCnt == 0) {
        QOSM_LOGE("No connecting links found");
        return QOSM_FAIL;
    }

    if (!QOSM_GetLabelId(icgInfo, icgInfo->qosParam, &param.labelId)) {
        QOSM_LOGE("get label id failed, qosId: %u", icgInfo->qosId);
        return QOSM_FAIL;
    }

    uint32_t ret = CM_ICBAddConnection(&param, icgInfo->supportAutorate);
    if (ret != CM_SUCCESS) {
        QOSM_LOGE("CM_ICBAddConnection failed, ret=%u", ret);
        return QOSM_FAIL;
    }
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        for (uint8_t j = 0; j < param.channelCnt; j++) {
            if (icgInfo->link[i].connHandle != param.channel[j].connHandle) {
                continue;
            }
            icgInfo->link[i].hasCreateICB = true;
        }
    }
    return QOSM_SUCCESS;
}

static uint32_t QOSM_ICBLabelReportCbkProc(QOSM_ICGInfo *icgInfo, CM_ICBLabelReportParam *labelParam,
    uint8_t connectSeq)
{
    if (icgInfo->levelCnt == 0 || icgInfo->levelCnt > QOSM_AUTORATE_MAX_LEVEL_CNT ||
        icgInfo->linkCnt == 0  || icgInfo->linkCnt > QOSM_AUTORATE_MAX_LINK_CNT) {
        QOSM_LOGE("invalid level count: %u or link count: %u", icgInfo->levelCnt, icgInfo->linkCnt);
        return QOSM_INVALID_COUNT_ERR;
    }

    if (labelParam->labelCnt > icgInfo->levelCnt || labelParam->labelCnt == 0 ||
        labelParam->labelCnt > QOSM_AUTORATE_MAX_LABEL_CNT) {
        QOSM_LOGE("invalid label count: %u", labelParam->labelCnt);
        return QOSM_INVALID_COUNT_ERR;
    }

    // icg的所有level一一对应记录labelId
    for (uint8_t i = 0; i < labelParam->labelCnt; i++) {
        for (uint8_t j = 0; j < icgInfo->totalLevelCnt; j++) {
            if (!QOSM_IsQosParamEqual(icgInfo->level[j].qosParam, labelParam, &labelParam->label[i])) {
                continue;
            }
            icgInfo->level[j].labelId = labelParam->label[i].labelId;
        }
    }

    // 检查icg底下的所有链路是否都已set label成功
    if (!QOSM_IsLinkConnectStatusEqualInGroup(icgInfo, connectSeq, LINK_CONNECTING)) {
        return QOSM_SUCCESS;
    }

    return QOSM_ICBAddConnectionProc(icgInfo, connectSeq);
}

static void QOSM_ICBLabelReportCbk(CM_ICBLabelReportParam *param)
{
    QOSM_LOGI("errorCode: %u, lcid: %u, connHandle: %u", param->errorCode, param->lcid, param->connHandle);
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByIcgId(param->icgId);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        QOSM_NotifyConnFailCbk(param->icgId, QOSM_CONNECTION_ADDED, &param->connHandle, 1);
        return;
    }

    uint8_t connectSeq = QOSM_INVALID_CONNECT_SEQ;
    uint8_t linkIndex = 0;
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connHandle == param->connHandle) {
            connectSeq = icgInfo->link[i].connectSeq;
            linkIndex = i;
            break;
        }
    }
    if (connectSeq == QOSM_INVALID_CONNECT_SEQ) {
        QOSM_LOGI("qos id %hhu, unknown conn handle %hu", icgInfo->qosId, param->connHandle);
        QOSM_NotifyConnFailCbk(param->icgId, QOSM_CONNECTION_ADDED, &param->connHandle, 1);
        return;
    }

    if (QOSM_IsLinkNone(icgInfo, param->connHandle)) {
        QOSM_LOGI("connHandle: %u no need to do anything", param->connHandle);
        return;
    }

    if (param->errorCode != CM_ICB_SUCCESS) {
        QOSM_LOGE("label report error, errorCode: %d, connHandle: %u", param->errorCode, param->connHandle);
        QOSM_SetupICBFailProc(icgInfo, linkIndex);
        return;
    }

    QOSM_SetLinkConnectStatus(icgInfo, param->connHandle, LINK_CONNECTING);
    uint32_t ret = QOSM_ICBLabelReportCbkProc(icgInfo, param, connectSeq);
    if (ret != QOSM_SUCCESS) {
        QOSM_ConnectFailInGroup(icgInfo, connectSeq);
        return;
    }
    QOSM_CheckRetryConnectInGroup(icgInfo, &icgInfo->link[linkIndex]);
}

static void QOSM_ICBRecommandEnter5GIfNeed(QOSM_ICGInfo *icgInfo, uint16_t connHandle)
{
    if (QOSM_IsLastEnableFreqBandByRecommend()) {
        QOSM_LOGE("last enable freqBand");
        return;
    }
    if (icgInfo == NULL || !icgInfo->is5G || icgInfo->icbType != CM_IOB) {
        QOSM_LOGE("input icgInfo param is invalid");
        return;
    }
    if (icgInfo->recommendEnter5GTimes >= QOSM_MAX_RECOMMEND_ENTER_5G_TIMES) {
        QOSM_LOGE("Over Recommend max times, no need Recommend enter 5G");
        return;
    }

    bool isNeedRecommendEnter5G = false;
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        QOSM_LOGI("lcid %d, connHandle %hu, freqBand %hu",
            icgInfo->link[i].lcid, icgInfo->link[i].connHandle, icgInfo->link[i].freqBand);
        if (icgInfo->link[i].connHandle != connHandle && icgInfo->link[i].freqBand == QOS_BAND_2D4
            && QOSM_IsICBConnected(&icgInfo->link[i])) {
            isNeedRecommendEnter5G = true;
            break;
        }
    }

    QOSM_CHECK_RETURN(isNeedRecommendEnter5G, "Already in 5G, no need Recommend enter 5G");

    DLI_FreqBandExtParam freqParam = {QOSM_FREQ_BAND_5D8_RECOMMEND, QOSM_ENABLE_FREQ_BAND_BY_RECOMMEND};
    if (DLI_GetExtFuncList()->enableFreqBandExt == NULL) {
        QOSM_LOGE("QOSM_ICBRecommandEnter5GIfNeed failed, registerfunc is null");
        return;
    }
    uint32_t ret = DLI_GetExtFuncList()->enableFreqBandExt(&freqParam);
    if (ret != DLI_SUCCESS) {
        QOSM_LOGE("enable freqBand failed");
        return;
    }
    icgInfo->recommendEnter5GTimes++;
    QOSM_LOGI("set sle 5G to 4, recommend Times %d", icgInfo->recommendEnter5GTimes);
}

static void QOSM_FreqBandListener(CM_FreqBandSwitchParam *param)
{
    uint16_t connHandle = DECODE2BYTE_LITTLE(&param->connHandle);
    QOSM_LOGI("conn handle: %hu, freq band: %hu", connHandle, param->newFreqBand);
    QOSM_FreqBandRecoverIfNeed();
    QOSM_ICGInfo *icgInfo = NULL;
    SDF_DListElmForeach(icgInfo, &g_qosICGList, entry) {
        for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
            if (icgInfo->link[i].connHandle != connHandle && icgInfo->link[i].lcid != connHandle) {
                continue;
            }
            QOSM_LOGI("conn handle: %hu, lcid: %hu, freq band: %hu", icgInfo->link[i].connHandle,
                icgInfo->link[i].lcid, param->newFreqBand);
            icgInfo->link[i].freqBand = param->newFreqBand;
            break;
        }
    }

    SDF_DListElmForeach(icgInfo, &g_qosICGList, entry) {
        bool is5G = false;
        for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
            if (icgInfo->link[i].freqBand == QOS_BAND_5D1 || icgInfo->link[i].freqBand == QOS_BAND_5D8) {
                is5G = true;
                break;
            }
        }
        icgInfo->is5G = is5G;
        QOSM_ICGMgrNotifyParam notifyParam = {};
        notifyParam.type = NOTIFY_TYPE_5G;
        QOSM_LOGI("5G maxBitrate change to: %hu", is5G ? QOSM_5G_MAX_BITRATE : QOSM_MAX_BITRATE);
        QOSM_ICGMgrParamNotify(&notifyParam);
    }

    if (!SDF_DListIsEmpty(&g_qosICGList)) {
        QOSM_AudioDfxUpdateBand(param->newFreqBand);
    }

    if (param->newFreqBand == QOS_BAND_5D1 || param->newFreqBand == QOS_BAND_5D8) {
        uint8_t linkIndex = 0;
        QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByConnHandle(connHandle, &linkIndex);
        if (icgInfo == NULL) {
            return;
        }
        QOSM_ICBRecommandEnter5GIfNeed(icgInfo, connHandle);
    }
}

void QOSM_ICGMgrEnable(void)
{
    QOSM_LOGI("enter");
    QOSM_AutorateUpdateDspStatus();
}

static void QOSM_FreeQosICGInfo(SDF_DListEntry_S *param)
{
    QOSM_ICGInfo *icgInfo = (QOSM_ICGInfo *)param;
    QOSM_DestroyQosICGInfo(icgInfo);
}

void QOSM_ICGMgrDisable(void)
{
    QOSM_LOGI("enter");
    SDF_DListDestroy(&g_qosICGList, QOSM_FreeQosICGInfo);
    SDF_DListHeadInit(&g_qosICGList);
    g_connectSeq = QOSM_INVALID_CONNECT_SEQ;
}

uint32_t QOSM_ICGMgrRegisterCallback(const QOSM_AutoRateCallback *callback)
{
    QOSM_LOGI("enter");
    uint32_t ret = CM_ICBMgrListenFreqBandSwitchEvent(QOSM_FreqBandListener);
    if (ret != CM_SUCCESS) {
        QOSM_LOGE("CM_ICBMgrListenFreqBandSwitchEvent failed, ret=%08x", ret);
        return QOSM_REGISTER_CBK_ERR;
    }
    CM_ICBCallback cb = {};
    cb.connectionCbk = QOSM_ICBConnectionCbk;
    cb.labelReportCbk = QOSM_ICBLabelReportCbk;
    cb.qualityReportCbk = QOSM_ICBQualityReport;
    ret = CM_ICBRegisterCbk(&cb);
    if (ret != CM_SUCCESS) {
        (void)CM_ICBMgrUnlistenFreqBandSwitchEvent(QOSM_FreqBandListener);
        QOSM_LOGE("CM_ICBRegisterCbk failed, ret=%08x", ret);
        return QOSM_REGISTER_CBK_ERR;
    }
    QOSM_AutorateUplayerCallbackInit(callback);
    return QOSM_SUCCESS;
}

uint32_t QOSM_ICGMgrUnregisterCallback(void)
{
    QOSM_LOGI("enter");
    (void)CM_ICBMgrUnlistenFreqBandSwitchEvent(QOSM_FreqBandListener);
    uint32_t ret = CM_ICBUnregisterCbk();
    QOSM_CHECK_RETURN_RET(ret == CM_SUCCESS, QOSM_UNREGISTER_CBK_ERR, "CM_ICBUnregisterCbk failed, ret=%08x", ret);
    QOSM_AutorateUplayerCallbackUninit();
    return QOSM_SUCCESS;
}

static bool QOSM_AssignICGParam(CM_ICGParam *icgParam, const QOSM_ICGInfo *icgInfo)
{
    icgParam->type = icgInfo->icbType;
    icgParam->id = icgInfo->icgId;
    icgParam->sduIntervalG2T = icgInfo->qosParam->sduIntervalG2T;
    icgParam->sduIntervalT2G = icgInfo->qosParam->sduIntervalT2G;
    icgParam->sca = icgInfo->qosParam->sca;
    icgParam->packing = icgInfo->qosParam->packing;
    icgParam->framing = icgInfo->qosParam->framing;
    icgParam->maxLatencyG2T = icgInfo->qosParam->maxLatencyG2T;
    icgParam->maxLatencyT2G = icgInfo->qosParam->maxLatencyT2G;
    icgParam->icbCnt = icgInfo->linkCnt;
    icgParam->paramCnt = icgInfo->levelCnt;
    icgParam->icbParam = SDF_MemZalloc(sizeof(CM_ICBParam) * icgParam->icbCnt);
    QOSM_CHECK_RETURN_RET(icgParam->icbParam != NULL, false, "malloc icbParam failed");
    for (uint8_t i = 0; i < icgParam->icbCnt; i++) {
        icgParam->icbParam[i].id = icgInfo->link[i].icbId;
        struct CM_ICB *param = (struct CM_ICB *)SDF_MemZalloc(icgParam->paramCnt * sizeof(struct CM_ICB));
        if (param == NULL) {
            for (uint8_t k = 0; k < i; k++) {
                SDF_MemFree(icgParam->icbParam[k].param);
            }
            SDF_MemFree(icgParam->icbParam);
            QOSM_LOGE("malloc icb param failed");
            return false;
        }
        icgParam->icbParam[i].param = param;
        for (uint8_t j = 0; j < icgParam->paramCnt; j++) {
            param[j].maxSduG2T = icgInfo->qosParam->maxSduG2T;
            param[j].maxSduT2G = icgInfo->qosParam->maxSduT2G;
            param[j].rtnG2T = icgInfo->qosParam->rtnG2T;
            param[j].rtnT2G = icgInfo->qosParam->rtnT2G;
        }
    }
    return true;
}

static void QOSM_FreeICGParam(CM_ICGParam *icgParam)
{
    if (icgParam->icbParam == NULL) {
        return;
    }
    for (uint8_t i = 0; i < icgParam->icbCnt; i++) {
        SDF_MemFree(icgParam->icbParam[i].param);
    }
    SDF_MemFree(icgParam->icbParam);
}

static uint32_t QOSM_ICGMgrRemoveParamInner(QOSM_ICGInfo *icgInfo)
{
    CM_ICGRemovedParam removedParam = {};
    removedParam.type = icgInfo->icbType;
    removedParam.id = icgInfo->icgId;
    uint32_t ret = CM_ICGRemoveParam(&removedParam);
    QOSM_LOGI("CM_ICGRemoveParam ret=%u", ret);
    if (ret == CM_SUCCESS) {
        icgInfo->isRemovingParam = true;
    }
    return ret;
}

static void QOSM_SetParamTimeoutCbkInner(void *args, bool isTest)
{
    uint8_t qosId = (uint8_t)(uintptr_t)args;
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(qosId);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        QOSM_NotifyParamChangedFailCb(qosId, QOSM_PARAM_SETTED);
        return;
    }
    icgInfo->delayParamTaskTimerId = QOSM_INVALID_TIMER_HANDLE;
    if (QOSM_ICGMgrRemoveParamInner(icgInfo) != CM_SUCCESS) {
        QOSM_ExecuteDelayParamTask(icgInfo);
    }
}

static void QOSM_SetParamTimeoutCbk(void *args)
{
    QOSM_SetParamTimeoutCbkInner(args, false);
}

static void QOSM_SetTestParamTimeoutCbk(void *args)
{
    QOSM_SetParamTimeoutCbkInner(args, true);
}

static bool QOSM_TerminateConnectingProcess(struct QosICBInfo *link)
{
    if (link->connectStatus == LINK_LABEL_SETTING) {
        QOSM_ResetICBLinkStatus(link);
        return true;
    }
    return false;
}

static bool QOSM_DelaySetParam(QOSM_ICGInfo *icgInfo, QOSM_ICGMgrParam *autoRateParam)
{
    // 先起2秒定时器，然后断链
    if (icgInfo->delayParamTaskTimerId != QOSM_INVALID_TIMER_HANDLE) {
        QOSM_LOGI("already waiting to delay set param");
        return true;
    }
    SDF_TimerParam param;
    param.period = false;
    param.expires = QOSM_DELAY_SET_PARAM_TIMEOUT_MS;
    param.args = (void *)(uintptr_t)icgInfo->qosId;
    param.callback = icgInfo->isTest ? QOSM_SetTestParamTimeoutCbk : QOSM_SetParamTimeoutCbk;
    uint32_t ret = CP_TimerAdd(&icgInfo->delayParamTaskTimerId, &param);
    if (ret != 0) {
        QOSM_LOGE("CP_TimerAdd failed, errno %u", ret);
        return false;
    }

    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (QOSM_GetDelayConnTask(icgInfo, icgInfo->link[i].connHandle) != NULL) {
            QOSM_DestroyDelayConnTask(icgInfo, true);
            continue;
        }

        if (icgInfo->link[i].connectStatus == LINK_NONE) {
            continue;
        }

        QOSM_TerminateConnectProcess(icgInfo, i, false);
        QOSM_TrytoStopAddConnectionTimer(icgInfo, icgInfo->link[i].connectSeq);
    }
    return true;
}

static void QOSM_ICGMgrSetParamInner(QOSM_ICGMgrParam *autoRateParam, bool isResetParam)
{
    QOSM_ICGInfo *icgInfo = QOSM_CreateQosICGInfo(autoRateParam, false, isResetParam);
    if (icgInfo == NULL) {
        QOSM_LOGE("QOSM_CreateQosICGInfo failed");
        QOSM_NotifyParamChangedFailCb(autoRateParam->autorateParam.qosId, QOSM_PARAM_SETTED);
        return;
    }
    SDF_DListElmTailInsert(&g_qosICGList, icgInfo, entry);

    CM_ICGParam icgParam = {};
    if (!QOSM_AssignICGParam(&icgParam, icgInfo)) {
        QOSM_LOGE("QOSM_AssignICGParam failed");
        goto FAILED;
    }
    uint32_t ret = CM_ICGSetParam(&icgParam);
    QOSM_FreeICGParam(&icgParam);
    if (ret != CM_SUCCESS) {
        QOSM_LOGE("CM_ICGSetParam failed, ret=%08x", ret);
        goto FAILED;
    }
    return;
FAILED:
    SDF_DListElmDel(&g_qosICGList, icgInfo, entry);
    QOSM_DestroyQosICGInfo(icgInfo);
    QOSM_NotifyParamChangedFailCb(autoRateParam->autorateParam.qosId, QOSM_PARAM_SETTED);
}

static void QOSM_AutoRateParamPrint(QOSM_ICGMgrParam *autoRateParam)
{
    for (uint8_t i = 0; i < autoRateParam->autorateParam.supportedBitrateCnt; i++) {
        QOSM_LOGI("qos Id: %hu, qos index: %hu, link cnt: %hhu, supported bit rate: %hu",
            autoRateParam->autorateParam.qosId, autoRateParam->autorateParam.qosIndex,
            autoRateParam->autorateParam.linkCnt, autoRateParam->autorateParam.supportedBitrate[i]);
    }
}

static bool QOSM_HasICBLabelSetted(const QOSM_ICGInfo *icgInfo)
{
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connectStatus == LINK_CONNECTING ||
            icgInfo->link[i].connectStatus == LINK_CONNECTED ||
            icgInfo->link[i].connectStatus == LINK_SUCCESS) {
            return true;
        }
    }
    return false;
}

static bool QOSM_CreateDelaySetParam(QOSM_ICGInfo *icgInfo, QOSM_ICGMgrParam *autoRateParam)
{
    if (icgInfo->delaySetParam != NULL) {
        SDF_MemFree(icgInfo->delaySetParam);
        icgInfo->delaySetParam = NULL;
        QOSM_LOGI("create delay set param again, id: %u", icgInfo->qosId);
    }
    icgInfo->delaySetParam = (QOSM_ICGMgrParam *)SDF_MemZalloc(sizeof(QOSM_ICGMgrParam));
    if (icgInfo->delaySetParam == NULL) {
        QOSM_LOGE("create delay set param failed, id: %u", icgInfo->qosId);
        return false;
    }
    (void)memcpy_s(icgInfo->delaySetParam, sizeof(QOSM_ICGMgrParam), autoRateParam, sizeof(QOSM_ICGMgrParam));
    QOSM_LOGI("create delay set param success, id: %u", icgInfo->qosId);
    return true;
}

void QOSM_ICGMgrSetParam(void *param)
{
    QOSM_ICGMgrParam *autoRateParam = (QOSM_ICGMgrParam *)param;
    QOSM_CHECK_RETURN(autoRateParam != NULL, "param is null");

    QOSM_ICGListPrint();
    QOSM_AutoRateParamPrint(autoRateParam);

    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(autoRateParam->autorateParam.qosId);
    if (icgInfo != NULL) {
        if (!QOSM_CreateDelaySetParam(icgInfo, autoRateParam)) {
            QOSM_NotifyParamChangedFailCb(icgInfo->qosId, QOSM_PARAM_SETTED);
            return;
        }

        if (icgInfo->isRemovingParam) {
            QOSM_LOGI("waiting to remove param, id: %u", icgInfo->qosId);
            return;
        }

        // 有create过链路，需先断链以后再remove param，等remove param成功以后再set param
        if (QOSM_HasICBLabelSetted(icgInfo)) {
            if (!QOSM_DelaySetParam(icgInfo, autoRateParam)) {
                SDF_MemFree(icgInfo->delaySetParam);
                icgInfo->delaySetParam = NULL;
                QOSM_NotifyParamChangedFailCb(icgInfo->qosId, QOSM_PARAM_SETTED);
            }
            return;
        }

        // 没有链路，或者没有create过链路，remove param成功以后再set param
        for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
            QOSM_StopAddConnectionTimer(&icgInfo->link[i]);
            (void)QOSM_TerminateConnectingProcess(&icgInfo->link[i]);
        }

        icgInfo->isResetParam = true;
        if (QOSM_ICGMgrRemoveParamInner(icgInfo) != CM_SUCCESS) {
            QOSM_ExecuteDelayParamTask(icgInfo);
        }
        return;
    }

    QOSM_ICGMgrSetParamInner(autoRateParam, false);
    QOSM_ICGListPrint();
}

static bool QOSM_AssignICGTestParam(CM_ICGTestParam *icgParam, const QOSM_ICGInfo *icgInfo,
    const struct QosLevelLabel *level)
{
    icgParam->type = icgInfo->icbType;
    icgParam->id = icgInfo->icgId;
    icgParam->labelId = level->testLabelId;
    icgParam->sduIntervalG2T = level->qosParam->sduIntervalG2T;
    icgParam->sduIntervalT2G = level->qosParam->sduIntervalT2G;
    icgParam->ftG2T = level->qosParam->ftG2T;
    icgParam->ftT2G = level->qosParam->ftT2G;
    icgParam->icbInterval = level->qosParam->icbInterval;
    icgParam->packing = level->qosParam->packing;
    icgParam->framing = level->qosParam->framing;
    icgParam->icbCnt = icgInfo->linkCnt;
    icgParam->icbParam = (CM_ICBTestParam *)SDF_MemZalloc(sizeof(CM_ICBTestParam) * icgParam->icbCnt);
    QOSM_CHECK_RETURN_RET(icgParam->icbParam != NULL, false, "malloc icbParam failed");
    for (uint8_t i = 0; i < icgParam->icbCnt; i++) {
        icgParam->icbParam[i].id = icgInfo->link[i].icbId;
        icgParam->icbParam[i].nse = level->qosParam->nse;
        icgParam->icbParam[i].maxSduG2T = level->qosParam->maxSduG2T;
        icgParam->icbParam[i].maxSduT2G = level->qosParam->maxSduT2G;
        icgParam->icbParam[i].maxPduG2T = level->qosParam->maxPduG2T;
        icgParam->icbParam[i].maxPduT2G = level->qosParam->maxPduT2G;
        icgParam->icbParam[i].frameG2T = level->qosParam->frameG2T;
        icgParam->icbParam[i].frameT2G = level->qosParam->frameT2G;
        icgParam->icbParam[i].phyG2T = level->qosParam->phyG2T;
        icgParam->icbParam[i].phyT2G = level->qosParam->phyT2G;
        icgParam->icbParam[i].mcsG2T = level->qosParam->mcsG2T;
        icgParam->icbParam[i].mcsT2G = level->qosParam->mcsT2G;
        icgParam->icbParam[i].pilotG2T = level->qosParam->pilotG2T;
        icgParam->icbParam[i].pilotT2G = level->qosParam->pilotT2G;
        icgParam->icbParam[i].bnG2T = level->qosParam->bnG2T;
        icgParam->icbParam[i].bnT2G = level->qosParam->bnT2G;
    }
    return true;
}

static void QOSM_ICGPowerLevelChangeCbk(void *args)
{
    NLSTK_CfgdbChanInfo_S *chanInfo = (NLSTK_CfgdbChanInfo_S *)args;
    if (chanInfo == NULL || chanInfo->data == NULL || chanInfo->len < sizeof(struct QOSM_PowerLevelInfo)) {
        QOSM_LOGE("invalid param");
        return;
    }

    struct QOSM_PowerLevelInfo *info  = (struct QOSM_PowerLevelInfo *)chanInfo->data;
    QOSM_LOGI("power level changed, conn handle=0x%04x, level=%hhu", info->connHandle, info->level);
    QOSM_AudioDfxUpdatePowerLevel(info->connHandle, info->level);
}

static void QOSM_AudioDfxInfoSet(struct QOSM_AudioDfxInfo *dfxInfo, QOSM_ICGInfo *icgInfo)
{
    if (icgInfo->qosParam != NULL) {
        dfxInfo->startBitrate = icgInfo->qosParam->downwardBitrate;
    }

    dfxInfo->codecType = (icgInfo->qosIndex == QOSM_QOSINDEX_CALL ||
        icgInfo->qosIndex == QOSM_QOSINDEX_VOICE_ASSISTANT) ? QOSM_AUDIO_DFX_CODEC_TYPE_BOTH :
        (icgInfo->qosIndex == QOSM_QOSINDEX_HD_RECORDING) ? QOSM_AUDIO_DFX_CODEC_TYPE_DECODER :
        QOSM_AUDIO_DFX_CODEC_TYPE_ENCODER;
    dfxInfo->dspStatusCb = QOSM_DspStatusCbk;
    dfxInfo->flowCtrlCb = QOSM_DspFlowCtrlCbk;
}

static void QOSM_ICGMgrSetTestParamInner(QOSM_ICGMgrParam *autoRateParam, bool isResetParam)
{
    QOSM_ICGInfo *icgInfo = QOSM_CreateQosICGInfo(autoRateParam, true, isResetParam);
    if (icgInfo == NULL) {
        QOSM_LOGE("QOSM_CreateQosICGInfo failed");
        QOSM_NotifyParamChangedFailCb(autoRateParam->autorateParam.qosId, QOSM_PARAM_SETTED);
        return;
    }
    if (icgInfo->levelCnt == 0 || icgInfo->levelCnt > QOSM_AUTORATE_MAX_LEVEL_CNT || icgInfo->qosParam == NULL) {
        QOSM_LOGE("qosParam is null or invalid level count: %u", icgInfo->levelCnt);
        QOSM_DestroyQosICGInfo(icgInfo);
        QOSM_NotifyParamChangedFailCb(autoRateParam->autorateParam.qosId, QOSM_PARAM_SETTED);
        return;
    }
    SDF_DListElmTailInsert(&g_qosICGList, icgInfo, entry);

    struct QOSM_AudioDfxInfo dfxInfo = {};
    QOSM_AudioDfxInfoSet(&dfxInfo, icgInfo);

    // 起播时上报一次QOS状态
    QOSM_NotifyReportedBitrateChangedCb(icgInfo, LEVEL_NONE, NEARLINK_INVALID_LABEL, icgInfo->qosParam);

    for (uint8_t i = 0; i < icgInfo->totalLevelCnt; i++) {
        if (!icgInfo->level[i].isSupported) {
            QOSM_LOGI("unsupported level %hhu, bitrate %hu", icgInfo->level[i].qosParam->qosLevel,
                icgInfo->level[i].qosParam->downwardBitrate);
            continue;
        }
        CM_ICGTestParam icgParam = {};
        if (!QOSM_AssignICGTestParam(&icgParam, icgInfo, &icgInfo->level[i])) {
            QOSM_LOGE("QOSM_AssignICGTestParam failed");
            goto FAILED;
        }
        dfxInfo.param.sduInterval = icgParam.sduIntervalG2T;
        dfxInfo.param.ft = icgParam.ftG2T;
        if (icgParam.icbCnt > 0) {
            dfxInfo.param.bn = icgParam.icbParam[0].bnG2T;
        }
        uint32_t ret = CM_ICGSetTestParam(&icgParam, icgInfo->supportAutorate);
        SDF_MemFree(icgParam.icbParam);
        if (ret != CM_SUCCESS) {
            QOSM_LOGE("CM_ICGSetTestParam failed, ret=%08x", ret);
            goto FAILED;
        }
    }

    QOSM_AudioDfxStart(&dfxInfo);
    NLSTK_CfgdbRegisterConfigListener(QOSM_ICGPowerLevelChangeCbk, NLSTK_CFGDB_MODULE_QOSM,
        NLSTK_CFGDB_CONFIG_POWER_LEVEL);

    return;
FAILED:
    SDF_DListElmDel(&g_qosICGList, icgInfo, entry);
    QOSM_DestroyQosICGInfo(icgInfo);
    QOSM_NotifyParamChangedFailCb(autoRateParam->autorateParam.qosId, QOSM_PARAM_SETTED);
}

void QOSM_ICGMgrSetTestParam(void *param)
{
    QOSM_ICGMgrParam *autoRateParam = (QOSM_ICGMgrParam *)param;
    QOSM_CHECK_RETURN(autoRateParam != NULL, "param is null");

    QOSM_ICGListPrint();
    QOSM_AutoRateParamPrint(autoRateParam);

    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(autoRateParam->autorateParam.qosId);
    if (icgInfo != NULL) {
        if (!QOSM_CreateDelaySetParam(icgInfo, autoRateParam)) {
            QOSM_NotifyParamChangedFailCb(icgInfo->qosId, QOSM_PARAM_SETTED);
            return;
        }

        if (icgInfo->isRemovingParam) {
            QOSM_LOGI("waiting to remove param, id: %u", icgInfo->qosId);
            return;
        }

        // 有create过链路，需先断链以后再remove param，等remove param成功以后再set param
        if (QOSM_HasICBLabelSetted(icgInfo)) {
            if (!QOSM_DelaySetParam(icgInfo, autoRateParam)) {
                SDF_MemFree(icgInfo->delaySetParam);
                icgInfo->delaySetParam = NULL;
                QOSM_NotifyParamChangedFailCb(icgInfo->qosId, QOSM_PARAM_SETTED);
            }
            return;
        }

        // 没有链路，或者没有create过链路，remove param成功以后再set param
        for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
            QOSM_StopAddConnectionTimer(&icgInfo->link[i]);
            (void)QOSM_TerminateConnectingProcess(&icgInfo->link[i]);
        }

        icgInfo->isResetParam = true;
        if (QOSM_ICGMgrRemoveParamInner(icgInfo) != CM_SUCCESS) {
            QOSM_ExecuteDelayParamTask(icgInfo);
        }
        return;
    }

    QOSM_ICGMgrSetTestParamInner(autoRateParam, false);
    QOSM_ICGListPrint();
}

static bool QOSM_DelayAddConnection(QOSM_ICGInfo *icgInfo, QOSM_AutoRateConnParam *connParam)
{
    if (icgInfo->delayConnParam != NULL) {
        QOSM_LOGE("already has delay connection task");
        return false;
    }
    icgInfo->delayConnParam = (QOSM_AutoRateConnParam *)SDF_MemZalloc(sizeof(QOSM_AutoRateConnParam));
    if (icgInfo->delayConnParam == NULL) {
        QOSM_LOGE("malloc delay conn param failed");
        return false;
    }
    icgInfo->delayConnParam->qosId = connParam->qosId;
    icgInfo->delayConnParam->linkCnt = connParam->linkCnt;
    icgInfo->delayConnParam->link = (QOSM_ConnParam *)SDF_MemZalloc(connParam->linkCnt * sizeof(QOSM_ConnParam));
    if (icgInfo->delayConnParam->link == NULL) {
        QOSM_LOGE("malloc delay link failed");
        SDF_MemFree(icgInfo->delayConnParam);
        icgInfo->delayConnParam = NULL;
        return false;
    }
    for (uint8_t i = 0; i < connParam->linkCnt; i++) {
        icgInfo->delayConnParam->link[i].connHandle = connParam->link[i].connHandle;
        icgInfo->delayConnParam->link[i].lcid = connParam->link[i].lcid;
    }
    return true;
}

/*
 * 定时器取消时机：
 *    1、更新完成
 *    2、param被remove
 */
static void QOSM_AddConnectionTimeoutCbk(void *args)
{
    uint8_t qosId = (uint8_t)(uintptr_t)args;
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(qosId);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        return;
    }
    icgInfo->delayConnTaskTimerId = QOSM_INVALID_TIMER_HANDLE;
    QOSM_ExecuteDelayConnTask(icgInfo);
}

static void QOSM_SetConnTaskTimer(QOSM_ICGInfo *icgInfo)
{
    SDF_TimerParam param;
    param.period = false;
    param.expires = QOSM_DELAY_ADD_CONN_TIMEOUT_MS;
    param.args = (void *)(uintptr_t)icgInfo->qosId;
    param.callback = QOSM_AddConnectionTimeoutCbk;
    uint32_t ret = CP_TimerAdd(&icgInfo->delayConnTaskTimerId, &param);
    if (ret != 0) {
        QOSM_LOGE("CP_TimerAdd failed, errno %u", ret);
    }
}

static void QOSM_AddConnectionTimeout(void *arg)
{
    uint16_t input = (uint16_t)(uintptr_t)arg;
    uint8_t qosId = (uint8_t)(input >> QOSM_QOSID_OFFSET);
    uint8_t connectSeq = (uint8_t)input;
    QOSM_LOGE("qos id %hhu, conn seq %hu add connection timeout", qosId, connectSeq);

    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(qosId);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        return;
    }

    QOSM_ConnectFailInGroup(icgInfo, connectSeq);
}

static void QOSM_TrytoStopAddConnectionTimer(QOSM_ICGInfo *icgInfo, uint8_t connectSeq)
{
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connectSeq != connectSeq) {
            continue;
        }
        QOSM_StopAddConnectionTimer(&icgInfo->link[i]);
    }
}

static void QOSM_ICGMgrStartAddConnectionTimer(struct QosICBInfo *link, uint8_t qosId)
{
    SDF_TimerParam param;
    param.period = false;
    param.expires = QOSM_ADD_CONNECTION_TIMEOUT_MS;
    param.args = (void *)(uintptr_t)(((uint8_t)qosId << QOSM_QOSID_OFFSET) | (uint8_t)link->connectSeq);
    param.callback = QOSM_AddConnectionTimeout;
    uint32_t ret = CP_TimerAdd(&link->addConnectionTimerId, &param);
    QOSM_LOGI("CP_TimerAdd for qos id %hhu, conn seq %hu, ret %u", qosId, link->connectSeq, ret);
}

static bool QOSM_IsConnHandleValid(const QOSM_ICGInfo *icgInfo, const QOSM_AutoRateConnParam *connParam)
{
    for (uint8_t i = 0; i < connParam->linkCnt; i++) {
        bool isValidConnHandle = false;
        for (uint8_t j = 0; j < icgInfo->linkCnt; j++) {
            if (connParam->link[i].connHandle == icgInfo->link[j].connHandle) {
                isValidConnHandle = true;
                break;
            }
        }

        if (!isValidConnHandle) {
            QOSM_LOGE("conn handle 0x%04x is invalid", connParam->link[i].connHandle);
            return false;
        }
    }

    return true;
}

static void QOSM_NotifyAddConnectionSuccess(QOSM_ICGInfo *icgInfo, QOSM_AutoRateConnParam *inputParam)
{
    QOSM_ConnParam link[QOSM_AUTORATE_MAX_LINK_CNT] = {};
    uint8_t linkCnt = 0;
    for (uint8_t i = 0; i < inputParam->linkCnt && i < QOSM_AUTORATE_MAX_LINK_CNT; i++) {
        link[linkCnt].connHandle = inputParam->link[i].connHandle;
        link[linkCnt].lcid = inputParam->link[i].lcid;
        linkCnt++;
    }
    QOSM_NotifyConnCbk(icgInfo, QOSM_CONNECTION_ADDED, QOSM_SUCCESS, link, linkCnt);
}

static void QOSM_ICGMgrSetLabel(QOSM_ICGInfo *icgInfo, CM_ICGLabelParam *labelParam, uint8_t connectSeq)
{
    uint32_t ret = CM_ICGSetLabel(labelParam, icgInfo->supportSubrate, icgInfo->supportAutorate);
    if (ret != CM_SUCCESS) {
        QOSM_LOGE("CM_ICGSetLabel failed, ret=%u", ret);
        QOSM_ConnParam link[QOSM_AUTORATE_MAX_LINK_CNT] = {};
        uint8_t linkCnt = 0;
        for (uint8_t i = 0; i < icgInfo->linkCnt && i < QOSM_AUTORATE_MAX_LINK_CNT; i++) {
            if (icgInfo->link[i].connectSeq == connectSeq) {
                QOSM_ResetICBLinkStatus(&icgInfo->link[i]);
                link[linkCnt].connHandle = icgInfo->link[i].connHandle;
                linkCnt++;
            }
        }
        QOSM_NotifyConnCbk(icgInfo, QOSM_CONNECTION_ADDED, QOSM_FAIL, link, linkCnt);
        return;
    }

    if (!icgInfo->supportAutorate) {
        for (uint8_t i = 0; i < icgInfo->linkCnt && i < QOSM_AUTORATE_MAX_LINK_CNT; i++) {
            if (icgInfo->link[i].connectSeq == connectSeq) {
                icgInfo->link[i].connectStatus = LINK_CONNECTING;
            }
        }
        ret = QOSM_ICBAddConnectionProc(icgInfo, connectSeq);
        if (ret != QOSM_SUCCESS) {
            QOSM_ConnectFailInGroup(icgInfo, connectSeq);
            return;
        }
    }

    // 同一组（connectSeq）多条链路同时建链，则将isInGroupConnecting标记置为true
    struct QosICBInfo *link = NULL;
    for (uint8_t i = 0; i < icgInfo->linkCnt; i++) {
        if (icgInfo->link[i].connectSeq != connectSeq) {
            continue;
        }
        if (labelParam->icbCnt > 1) {
            icgInfo->link[i].isInGroupConnecting = true;
        }
        link = &icgInfo->link[i];
    }
    if (link != NULL) {
        QOSM_TrytoStopAddConnectionTimer(icgInfo, connectSeq);
        QOSM_ICGMgrStartAddConnectionTimer(link, icgInfo->qosId);
    }
}

static void QOSM_FreeICGLabelParam(CM_ICGLabelParam *labelParam)
{
    if (labelParam == NULL) {
        return;
    }
    if (labelParam->icb != NULL) {
        SDF_MemFree(labelParam->icb);
    }
    SDF_MemFree(labelParam);
}

static CM_ICGLabelParam *QOSM_MallocICGLabelParam()
{
    CM_ICGLabelParam *labelParam = (CM_ICGLabelParam *)SDF_MemZalloc(sizeof(CM_ICGLabelParam));
    QOSM_CHECK_RETURN_RET(labelParam != NULL, NULL, "malloc labelParam failed");
    labelParam->icb = (CM_ICBChannel *)SDF_MemZalloc(CM_MAX_CHANNEL_COUNT * sizeof(CM_ICBChannel));
    if (labelParam->icb == NULL) {
        QOSM_LOGE("malloc labelParam icb failed");
        QOSM_FreeICGLabelParam(labelParam);
        return NULL;
    }
    return labelParam;
}

static void QOSM_ICGMgrAddConnectionInner(QOSM_ICGInfo *icgInfo, QOSM_AutoRateConnParam *connParam)
{
    uint32_t inputLinkSuccessCnt = 0;
    CM_ICGLabelParam *labelParam = QOSM_MallocICGLabelParam();
    QOSM_CHECK_RETURN(labelParam != NULL, "malloc labelParam failed");
    labelParam->type = icgInfo->icbType;
    labelParam->id = icgInfo->icgId;
    labelParam->icbCnt = 0;
    QOSM_IncreaseConnectSeq();
    for (uint8_t i = 0; i < connParam->linkCnt; i++) {
        QOSM_LOGI("add connection [%u], type: %u, id: %u, conn handler: 0x%04x, lcid: 0x%04x",
            i, icgInfo->icbType, icgInfo->icgId, connParam->link[i].connHandle, connParam->link[i].lcid);
        for (uint8_t j = 0; j < icgInfo->linkCnt; j++) {
            if (icgInfo->link[j].connHandle != connParam->link[i].connHandle) {
                continue;
            }

            QOSM_LOGI("connection status: %u", icgInfo->link[j].connectStatus);
            if (icgInfo->link[j].connectStatus == LINK_SUCCESS) {
                QOSM_LOGI("conn handle: 0x%04x, lcid: 0x%04x already connect success",
                    icgInfo->link[j].connHandle, icgInfo->link[j].lcid);
                inputLinkSuccessCnt++;
                continue;
            }
            if (icgInfo->link[j].connectStatus != LINK_SUCCESS && icgInfo->link[j].connectStatus != LINK_NONE) {
                QOSM_LOGI("conn handle: 0x%04x, lcid: 0x%04x connection is inprogress",
                    icgInfo->link[j].connHandle, icgInfo->link[j].lcid);
                continue;
            }

            labelParam->icb[labelParam->icbCnt].connHandle = connParam->link[i].connHandle;
            labelParam->icb[labelParam->icbCnt].lcid = connParam->link[i].lcid;
            labelParam->icbCnt++;

            icgInfo->link[j].lcid = connParam->link[i].lcid;
            icgInfo->link[j].connectSeq = g_connectSeq;
            icgInfo->link[j].connectStatus = LINK_LABEL_SETTING;
            icgInfo->link[j].isDisconnectedByAcb = false;
        }
    }
    if (labelParam->icbCnt == 0) {
        QOSM_LOGI("conn handle(count: %u) is not idle, success count: %u", connParam->linkCnt, inputLinkSuccessCnt);
        if (inputLinkSuccessCnt == connParam->linkCnt) {
            QOSM_LOGI("all input conn handle is connected");
            QOSM_NotifyAddConnectionSuccess(icgInfo, connParam);
        }
    } else {
        QOSM_ICGMgrSetLabel(icgInfo, labelParam, g_connectSeq);
    }
    QOSM_FreeICGLabelParam(labelParam);
}

void QOSM_ICGMgrAddConnection(void *param)
{
    QOSM_AutoRateConnParam *connParam = (QOSM_AutoRateConnParam *)param;
    QOSM_CHECK_RETURN(connParam != NULL && connParam->link != NULL, "param is null");
    QOSM_CHECK_RETURN(connParam->linkCnt != 0 && connParam->linkCnt <= QOSM_AUTORATE_MAX_LINK_CNT,
        "link cnt is illegal");
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(connParam->qosId);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        QOSM_ICGInfo tmp = {};
        tmp.qosId = connParam->qosId;
        QOSM_NotifyConnCbk(&tmp, QOSM_CONNECTION_ADDED, QOSM_FAIL, connParam->link, connParam->linkCnt);
        return;
    }
    if (icgInfo->isRemovingParam) {
        QOSM_LOGE("is removing param, can not add connection, id:%u", connParam->qosId);
        QOSM_NotifyConnCbk(icgInfo, QOSM_CONNECTION_ADDED, QOSM_FAIL, connParam->link, connParam->linkCnt);
        return;
    }
    if (!QOSM_IsConnHandleValid(icgInfo, connParam)) {
        QOSM_LOGE("contain invalid conn handle");
        QOSM_NotifyConnCbk(icgInfo, QOSM_CONNECTION_ADDED, QOSM_FAIL, connParam->link, connParam->linkCnt);
        return;
    }
    if (icgInfo->updateStatus != UPDATE_NONE) {
        QOSM_LOGI("icg is updating link param, delay execute add connection");
        if (!QOSM_DelayAddConnection(icgInfo, connParam)) {
            QOSM_LOGE("delay connection task failed");
            QOSM_NotifyConnCbk(icgInfo, QOSM_CONNECTION_ADDED, QOSM_FAIL, connParam->link, connParam->linkCnt);
        } else {
            QOSM_SetConnTaskTimer(icgInfo);
        }
        return;
    }
    QOSM_ICGMgrAddConnectionInner(icgInfo, connParam);
}

static QOSM_ConnParam *QOSM_GetDelayConnTask(QOSM_ICGInfo *icgInfo, uint16_t connHandle)
{
    if (icgInfo->delayConnParam != NULL) {
        for (uint8_t i = 0; i < icgInfo->delayConnParam->linkCnt; i++) {
            if (icgInfo->delayConnParam->link[i].connHandle == connHandle) {
                return &icgInfo->delayConnParam->link[i];
            }
        }
    }
    return NULL;
}

void QOSM_ICGMgrDeleteConnection(void *param)
{
    QOSM_AutoRateConnParam *connParam = (QOSM_AutoRateConnParam *)param;
    QOSM_CHECK_RETURN(connParam != NULL && connParam->link != NULL, "param is null");
    QOSM_CHECK_RETURN(connParam->linkCnt != 0 && connParam->linkCnt <= QOSM_AUTORATE_MAX_LINK_CNT,
        "link cnt is illegal");
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(connParam->qosId);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        QOSM_ICGInfo tmp = {};
        tmp.qosId = connParam->qosId;
        QOSM_NotifyConnCbk(&tmp, QOSM_CONNECTION_DELETED, QOSM_FAIL, connParam->link, connParam->linkCnt);
        return;
    }
    for (uint8_t i = 0; i < connParam->linkCnt; i++) {
        QOSM_ConnParam *conn = QOSM_GetDelayConnTask(icgInfo, connParam->link[i].connHandle);
        if (conn != NULL) {
            QOSM_NotifyConnCbk(icgInfo, QOSM_CONNECTION_DELETED, QOSM_SUCCESS, &connParam->link[i], 1);
            QOSM_DestroyDelayConnTask(icgInfo, false);
            continue;
        }
        for (uint8_t j = 0; j < icgInfo->linkCnt; j++) {
            if (icgInfo->link[j].connHandle != connParam->link[i].connHandle) {
                continue;
            }
            if (!QOSM_IsICBConnected(&icgInfo->link[j])) {
                QOSM_NotifyConnCbk(icgInfo, QOSM_CONNECTION_DELETED, QOSM_FAIL, &connParam->link[i], 1);
                break;
            }

            CM_ICBConnectionParam icbParam = {};
            icbParam.type = icgInfo->icbType;
            icbParam.id = icgInfo->icgId;
            icbParam.channelCnt = 1;
            CM_ICBChannel channel = {};
            channel.connHandle = icgInfo->link[j].connHandle;
            channel.lcid = icgInfo->link[j].lcid;
            icbParam.channel = &channel;
            uint32_t ret = CM_ICBDelConnection(&icbParam);
            if (ret != CM_SUCCESS) {
                QOSM_LOGE("CM_ICBDelConnection failed, ret = %x", ret);
                QOSM_NotifyConnCbk(icgInfo, QOSM_CONNECTION_DELETED, QOSM_FAIL, &connParam->link[i], 1);
            }
        }
    }
}

void QOSM_ICGMgrRemoveParam(void *id)
{
    uint8_t qosId = (uint8_t)(uintptr_t)id;
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(qosId);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        QOSM_NotifyParamChangedFailCb(qosId, QOSM_PARAM_REMOVED);
        return;
    }
    uint32_t ret = QOSM_ICGMgrRemoveParamInner(icgInfo);
    if (ret != CM_SUCCESS) {
        QOSM_LOGE("QOSM_ICGMgrRemoveParamInner failed, ret=%u", ret);
        QOSM_NotifyParamChangedFailCb(qosId, QOSM_PARAM_REMOVED);
        return;
    }
}

void QOSM_ICGMgrAddDataPath(void *param)
{
    QOSM_AutoRateDataPath *dataPath = (QOSM_AutoRateDataPath *)param;
    QOSM_CHECK_RETURN(dataPath != NULL, "param is null");
    if (dataPath->codecConfigLen != 0) {
        QOSM_CHECK_RETURN(dataPath->codecConfigData != NULL, "codec config data is null");
    }
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(dataPath->qosId);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        QOSM_NotifyDataPathChangedCb(dataPath->qosId, QOSM_DATAPATH_ADDED, QOSM_FAIL, dataPath->connHandle,
            dataPath->direction);
        return;
    }

    CM_ICBDataPath icbDataPath = {};
    icbDataPath.connHandle = dataPath->connHandle;
    icbDataPath.direction = dataPath->direction;
    icbDataPath.pathId = dataPath->pathId;
    icbDataPath.codec.codecId = dataPath->codec.codecId;
    icbDataPath.codec.vendorId = dataPath->codec.vendorId;
    icbDataPath.codec.vendorCodecId = dataPath->codec.vendorCodecId;
    icbDataPath.controllerDelay = dataPath->controllerDelay;
    icbDataPath.codecConfigLen = dataPath->codecConfigLen;
    icbDataPath.codecConfigData = dataPath->codecConfigData;
    uint32_t ret = CM_ICBSetupDataPath(&icbDataPath);
    if (ret != CM_SUCCESS) {
        QOSM_LOGE("CM_ICBSetupDataPath failed, ret=%u", ret);
        QOSM_NotifyDataPathChangedCb(dataPath->qosId, QOSM_DATAPATH_ADDED, QOSM_FAIL, dataPath->connHandle,
            dataPath->direction);
        return;
    }
}

void QOSM_ICGMgrDeleteDataPath(void *param)
{
    QOSM_AutoRateDeletedDataPath *dataPath = (QOSM_AutoRateDeletedDataPath *)param;
    QOSM_CHECK_RETURN(dataPath != NULL, "param is null");
    QOSM_ICGInfo *icgInfo = QOSM_FindQosIcbInfoByQosId(dataPath->qosId);
    if (icgInfo == NULL) {
        QOSM_LOGE("icgInfo is NULL");
        QOSM_NotifyDataPathChangedCb(dataPath->qosId, QOSM_DATAPATH_DELETED, QOSM_FAIL, dataPath->connHandle,
            dataPath->direction);
        return;
    }

    CM_ICBRemovedDataPath icbDataPath = {};
    icbDataPath.connHandle = dataPath->connHandle;
    icbDataPath.direction = dataPath->direction;
    uint32_t ret = CM_ICBRemoveDataPath(&icbDataPath);
    if (ret != CM_SUCCESS) {
        QOSM_LOGE("CM_ICBRemoveDataPath failed, ret=%u", ret);
        QOSM_NotifyDataPathChangedCb(dataPath->qosId, QOSM_DATAPATH_DELETED, QOSM_FAIL, dataPath->connHandle,
            dataPath->direction);
        return;
    }
}

void QOSM_ICGMgrIterate(void (*func)(QOSM_ICGInfo *icgInfo, void *data), void *data)
{
    QOSM_ICGInfo *icgInfo = NULL;
    SDF_DListElmForeach(icgInfo, &g_qosICGList, entry) {
        func(icgInfo, data);
    }
}
