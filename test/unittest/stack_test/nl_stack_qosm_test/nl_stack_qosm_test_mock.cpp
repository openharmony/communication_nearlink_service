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

#include "nl_stack_qosm_test_mock.h"

#include <string.h>
#include "securec.h"
#include "nlstk_cfgdb_api.h"
#include "nlstk_public_define.h"
#include "qosm_audio_dfx.h"
#include "qosm_errno.h"
#include "qosm_icg_callback.h"
#include "qosm_icg_mgr.h"
#include "sdf_mem.h"
#include "sdf_worker.h"

#define TEST_DELAY_PARAM_TIMEOUT_MS 2000
#define TEST_DELAY_CONN_TIMEOUT_MS 2000
#define TEST_ADD_CONN_TIMEOUT_MS 4000
#define TEST_RETRY_CONN_TIMEOUT_MS 100
#define TEST_UPGRADE_LABEL_TIMEOUT_MS 1000
#define TEST_DOWNGRADE_LABEL_TIMEOUT_MS 5000

CM_ICBCallback g_icbCbk = {};
CM_FreqBandListener g_freqBandListener = nullptr;
uint8_t g_levelCnt = 0;
uint8_t g_labelId = 0;
uint8_t g_qosId = 1;
uint16_t g_lcid1 = 3;
uint16_t g_lcid2 = 4;
uint16_t g_connHandle1 = 0xA3;
uint16_t g_connHandle2 = 0xA4;

uint32_t g_registerCbkRet = CM_SUCCESS;
uint32_t g_listenFreqBandRet = CM_SUCCESS;
uint32_t g_setParamRet = CM_SUCCESS;
uint32_t g_setAutorateParamRet = CM_SUCCESS;
uint32_t g_setLabelRet = CM_SUCCESS;
uint32_t g_addConnectionRet = CM_SUCCESS;
uint32_t g_cmSetDataPathRet = CM_SUCCESS;
uint32_t g_cmRemoveDataPathRet = CM_SUCCESS;
uint32_t g_removeParamRet = CM_SUCCESS;
uint32_t g_deleteConnectionRet = CM_SUCCESS;
uint32_t g_cpTimerRet = 0;
bool g_useDelayParamTimer = false;

SDF_TimerParam g_timerParam[QOSM_TEST_TIMER_NUM] = {};
CM_ICGLabelParam g_labelParam = {};
CM_ICBChannel g_labelIcb[2] = {};
CM_ICBConnectionParam g_createParam = {};
CM_ICBChannel g_createChannel[2] = {};
CM_ICGUpdatedParam g_updateParam = {};
uint16_t g_updateChannel[2] = {};
CM_ICBConnectionParam g_disconnectParam = {};
CM_ICBChannel g_disconnectChannel[2] = {};
CM_ICGParam g_settedParam = {};
CM_ICGAutorateParam g_settedAutorateParam = {};
CM_ICGRemovedParam g_removedParam = {};

QOSM_ParamCb g_paramCbk[2] = {};
QOSM_ConnParamCb g_connCbk[2] = {};
QOSM_ConnParam g_connCbkLink[4] = {};
QOSM_DataPathParamCb g_dataPathCbk[2] = {};
QOSM_BitrateParamCb g_bitrateCbk[QOSM_TEST_BITRATE_HISTORY_CNT] = {};
uint8_t g_bitrateCbkCnt = 0;
QOSM_AutoRateSendMsgCb g_sendMsgCbk[QOSM_TEST_BITRATE_HISTORY_CNT] = {};
uint8_t g_sendMsgCbkCnt = 0;
struct QOSM_AudioDfxInfo g_audioDfxInfo = {};
uint32_t g_dfxBand = 0;
uint8_t g_powerLevel = 0;
uint32_t g_dfxUpdateConnCnt = 0;

static uint16_t g_gHandle = 0xA2;

#ifdef __cplusplus
extern "C" {
#endif

uint32_t CM_ICBRegisterCbk(const CM_ICBCallback *cb)
{
    if (g_registerCbkRet != CM_SUCCESS) {
        return g_registerCbkRet;
    }
    if (memcpy_s(&g_icbCbk, sizeof(CM_ICBCallback), cb, sizeof(CM_ICBCallback)) != EOK) {
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_ICBUnregisterCbk(void)
{
    (void)memset_s(&g_icbCbk, sizeof(CM_ICBCallback), 0, sizeof(CM_ICBCallback));
    return CM_SUCCESS;
}

uint32_t CM_ICBMgrListenFreqBandSwitchEvent(CM_FreqBandListener listener)
{
    if (g_listenFreqBandRet != CM_SUCCESS) {
        return g_listenFreqBandRet;
    }
    g_freqBandListener = listener;
    return CM_SUCCESS;
}

uint32_t CM_ICBMgrUnlistenFreqBandSwitchEvent(CM_FreqBandListener listener)
{
    if (g_freqBandListener == listener) {
        g_freqBandListener = nullptr;
    }
    return CM_SUCCESS;
}

uint32_t CM_ICGSetParam(CM_ICGParam *icgParam)
{
    if (g_setParamRet != CM_SUCCESS) {
        return g_setParamRet;
    }
    if (memcpy_s(&g_settedParam, sizeof(CM_ICGParam), icgParam, sizeof(CM_ICGParam)) != EOK) {
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_ICGSetAutorateParam(CM_ICGAutorateParam *icgParam, bool supportAutorate)
{
    if (g_setAutorateParamRet != CM_SUCCESS) {
        return g_setAutorateParamRet;
    }
    if (memcpy_s(&g_settedAutorateParam, sizeof(CM_ICGAutorateParam), icgParam, sizeof(CM_ICGAutorateParam)) != EOK) {
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_ICGSetLabel(CM_ICGLabelParam *icgLabel, bool supportSubrate, bool supportAutorate)
{
    if (g_setLabelRet != CM_SUCCESS) {
        return g_setLabelRet;
    }
    if (icgLabel->icbCnt == 0) {
        return CM_FAIL;
    }
    if (memcpy_s(&g_labelParam, sizeof(CM_ICGLabelParam), icgLabel, sizeof(CM_ICGLabelParam)) != EOK) {
        return CM_FAIL;
    }
    for (uint8_t i = 0; i < icgLabel->icbCnt && i < 2; i++) {
        g_labelIcb[i] = icgLabel->icb[i];
    }
    return CM_SUCCESS;
}

uint32_t CM_ICBAddConnection(CM_ICBConnectionParam *connParam, bool supportAutorate)
{
    if (g_addConnectionRet != CM_SUCCESS) {
        return g_addConnectionRet;
    }
    if (connParam->channelCnt == 0) {
        return CM_FAIL;
    }
    if (memcpy_s(&g_createParam, sizeof(CM_ICBConnectionParam), connParam, sizeof(CM_ICBConnectionParam)) != EOK) {
        return CM_FAIL;
    }
    for (uint8_t i = 0; i < connParam->channelCnt && i < 2; i++) {
        g_createChannel[i] = connParam->channel[i];
    }
    return CM_SUCCESS;
}

uint32_t CM_ICGUpdateParam(CM_ICGUpdatedParam *icgParam)
{
    if (icgParam->icbCnt == 0) {
        return CM_FAIL;
    }
    if (memcpy_s(&g_updateParam, sizeof(CM_ICGUpdatedParam), icgParam, sizeof(CM_ICGUpdatedParam)) != EOK) {
        return CM_FAIL;
    }
    for (uint8_t i = 0; i < icgParam->icbCnt && i < 2; i++) {
        g_updateChannel[i] = icgParam->connHandle[i];
    }
    return CM_SUCCESS;
}

uint32_t CM_ICBSetupDataPath(CM_ICBDataPath *dataPath)
{
    return g_cmSetDataPathRet;
}

uint32_t CM_ICBRemoveDataPath(CM_ICBRemovedDataPath *dataPath)
{
    return g_cmRemoveDataPathRet;
}

uint32_t CM_ICBDelConnection(CM_ICBConnectionParam *connParam)
{
    if (g_deleteConnectionRet != CM_SUCCESS) {
        return g_deleteConnectionRet;
    }
    if (connParam->channelCnt == 0) {
        return CM_FAIL;
    }
    if (memcpy_s(&g_disconnectParam, sizeof(CM_ICBConnectionParam), connParam,
        sizeof(CM_ICBConnectionParam)) != EOK) {
        return CM_FAIL;
    }
    for (uint8_t i = 0; i < connParam->channelCnt && i < 2; i++) {
        g_disconnectChannel[i] = connParam->channel[i];
    }
    return CM_SUCCESS;
}

uint32_t CM_ICGRemoveParam(CM_ICGRemovedParam *icgParam)
{
    if (g_removeParamRet != CM_SUCCESS) {
        return g_removeParamRet;
    }
    if (memcpy_s(&g_removedParam, sizeof(CM_ICGRemovedParam), icgParam, sizeof(CM_ICGRemovedParam)) != EOK) {
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

bool CM_InnerGetRemotePrivateFeature(uint16_t lcid, uint16_t featureBit)
{
    return true;
}

bool CM_InnerGetRemoteStandardFeature(uint16_t lcid, uint16_t featureBit)
{
    return false;
}

uint32_t CP_TimerAdd(int *handle, SDF_TimerParam *param)
{
    if (g_cpTimerRet != 0) {
        return g_cpTimerRet;
    }
    int timerId = -1;
    if (param->expires == TEST_DELAY_PARAM_TIMEOUT_MS) {
        timerId = g_useDelayParamTimer ? QOSM_TEST_DELAY_PARAM_TIMER : QOSM_TEST_DELAY_CONN_TIMER;
    } else if (param->expires == TEST_ADD_CONN_TIMEOUT_MS) {
        timerId = QOSM_TEST_ADD_CONN_TIMER;
    } else if (param->expires == TEST_RETRY_CONN_TIMEOUT_MS) {
        timerId = QOSM_TEST_RETRY_CONN_TIMER;
    } else if (param->expires == TEST_UPGRADE_LABEL_TIMEOUT_MS) {
        timerId = QOSM_TEST_UPGRADE_LABEL_TIMER;
    } else if (param->expires == TEST_DOWNGRADE_LABEL_TIMEOUT_MS) {
        timerId = QOSM_TEST_DOWNGRADE_LABEL_TIMER;
    }
    if (timerId < 0) {
        return 1;
    }
    g_timerParam[timerId] = *param;
    *handle = timerId;
    return 0;
}

void CP_TimerDel(int handle)
{
    if (handle < 0 || handle >= QOSM_TEST_TIMER_NUM) {
        return;
    }
    (void)memset_s(&g_timerParam[handle], sizeof(SDF_TimerParam), 0, sizeof(SDF_TimerParam));
}

uint32_t CP_PostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (cb != nullptr) {
        cb(arg);
    }
    if (freeCb != nullptr) {
        freeCb(arg);
    }
    return 0;
}

NLSTK_Errcode_E NLSTK_CfgdbRegisterConfigListener(NLSTK_CfgdbConfigListenerFunc func, NLSTK_CfgdbModule_E module,
    NLSTK_CfgdbConfig_E config)
{
    return NLSTK_ERRCODE_SUCCESS;
}

int32_t PropertyGetInt32(const char *key, int32_t defaultValue)
{
    return defaultValue;
}

uint16_t DLI_DataNumGet(uint8_t type)
{
    return 0;
}

void QOSM_AudioDfxStart(struct QOSM_AudioDfxInfo *info)
{
    g_audioDfxInfo = *info;
}

void QOSM_AudioDfxUpdateBand(uint32_t band)
{
    g_dfxBand = band;
}

void QOSM_AudioDfxUpdatePowerLevel(uint16_t connHandle, uint8_t level)
{
    g_powerLevel = level;
}

void QOSM_AudioDfxQualityReport(uint16_t txFlushed, uint16_t ackrate)
{
}

void QOSM_AudioDfxNotifyChoppy(struct QOSM_AudioDfxChoppyInfo *info)
{
}

void QOSM_AudioDfxUpdateBitrate(uint32_t bitrate)
{
}

void QOSM_AudioDfxStop(void)
{
}

void QOSM_AudioDfxUpdateConn(struct QOSM_AudioDfxConn *conn, bool connected)
{
    g_dfxUpdateConnCnt++;
}

void QOSM_DfxUpdateConnected(uint16_t connHandle, uint16_t lcid)
{
}

void QOSM_AudioDfxInfoSet(struct QOSM_AudioDfxInfo *dfxInfo, struct QosICGInfo *icgInfo)
{
}

void QOSM_AudioDfxGetDspStatusInner(void *arg)
{
    *(bool *)arg = true;
}

#ifdef __cplusplus
}
#endif

static void TestParamChangedCbk(const QOSM_ParamCb *param)
{
    if (param->state == QOSM_PARAM_SETTED) {
        g_paramCbk[0] = *param;
    } else if (param->state == QOSM_PARAM_REMOVED) {
        g_paramCbk[1] = *param;
    }
}

static void TestConnChangedCbk(const QOSM_ConnParamCb *param)
{
    if (param->state == QOSM_CONNECTION_ADDED) {
        g_connCbk[0] = *param;
        for (uint8_t i = 0; i < param->linkCnt && i < 2; i++) {
            g_connCbkLink[i] = param->link[i];
        }
    } else if (param->state == QOSM_CONNECTION_DELETED) {
        g_connCbk[1] = *param;
        for (uint8_t i = 0; i < param->linkCnt && i < 2; i++) {
            g_connCbkLink[i + 2] = param->link[i];
        }
    }
}

static void TestDataPathChangedCbk(const QOSM_DataPathParamCb *param)
{
    if (param->state == QOSM_DATAPATH_ADDED) {
        g_dataPathCbk[0] = *param;
    } else if (param->state == QOSM_DATAPATH_DELETED) {
        g_dataPathCbk[1] = *param;
    }
}

static void TestBitrateChangedCbk(const QOSM_BitrateParamCb *param, uint8_t paramCnt)
{
    if (g_bitrateCbkCnt < QOSM_TEST_BITRATE_HISTORY_CNT) {
        g_bitrateCbk[g_bitrateCbkCnt] = *param;
        g_bitrateCbkCnt++;
    }
}

static void TestSendMsgCbk(const QOSM_AutoRateSendMsgCb *param)
{
    if (g_sendMsgCbkCnt < QOSM_TEST_BITRATE_HISTORY_CNT) {
        g_sendMsgCbk[g_sendMsgCbkCnt] = *param;
        g_sendMsgCbkCnt++;
    }
}

void TestResetMock(void)
{
    g_registerCbkRet = CM_SUCCESS;
    g_listenFreqBandRet = CM_SUCCESS;
    g_setParamRet = CM_SUCCESS;
    g_setAutorateParamRet = CM_SUCCESS;
    g_setLabelRet = CM_SUCCESS;
    g_addConnectionRet = CM_SUCCESS;
    g_cmSetDataPathRet = CM_SUCCESS;
    g_cmRemoveDataPathRet = CM_SUCCESS;
    g_removeParamRet = CM_SUCCESS;
    g_deleteConnectionRet = CM_SUCCESS;
    g_cpTimerRet = 0;
    g_useDelayParamTimer = false;
    g_labelId = 0;
    g_levelCnt = 0;
    g_bitrateCbkCnt = 0;
    g_sendMsgCbkCnt = 0;
    g_dfxUpdateConnCnt = 0;
    g_dfxBand = 0;
    g_powerLevel = 0;
    (void)memset_s(&g_audioDfxInfo, sizeof(g_audioDfxInfo), 0, sizeof(g_audioDfxInfo));
    (void)memset_s(g_timerParam, sizeof(g_timerParam), 0, sizeof(g_timerParam));
    (void)memset_s(&g_labelParam, sizeof(g_labelParam), 0, sizeof(g_labelParam));
    (void)memset_s(g_labelIcb, sizeof(g_labelIcb), 0, sizeof(g_labelIcb));
    (void)memset_s(&g_createParam, sizeof(g_createParam), 0, sizeof(g_createParam));
    (void)memset_s(g_createChannel, sizeof(g_createChannel), 0, sizeof(g_createChannel));
    (void)memset_s(&g_updateParam, sizeof(g_updateParam), 0, sizeof(g_updateParam));
    (void)memset_s(g_updateChannel, sizeof(g_updateChannel), 0, sizeof(g_updateChannel));
    (void)memset_s(&g_disconnectParam, sizeof(g_disconnectParam), 0, sizeof(g_disconnectParam));
    (void)memset_s(g_disconnectChannel, sizeof(g_disconnectChannel), 0, sizeof(g_disconnectChannel));
    (void)memset_s(&g_settedParam, sizeof(g_settedParam), 0, sizeof(g_settedParam));
    (void)memset_s(&g_settedAutorateParam, sizeof(g_settedAutorateParam), 0, sizeof(g_settedAutorateParam));
    (void)memset_s(&g_removedParam, sizeof(g_removedParam), 0, sizeof(g_removedParam));
    (void)memset_s(g_paramCbk, sizeof(g_paramCbk), 0, sizeof(g_paramCbk));
    (void)memset_s(g_connCbk, sizeof(g_connCbk), 0, sizeof(g_connCbk));
    (void)memset_s(g_connCbkLink, sizeof(g_connCbkLink), 0, sizeof(g_connCbkLink));
    (void)memset_s(g_dataPathCbk, sizeof(g_dataPathCbk), 0, sizeof(g_dataPathCbk));
    (void)memset_s(g_bitrateCbk, sizeof(g_bitrateCbk), 0, sizeof(g_bitrateCbk));
    (void)memset_s(g_sendMsgCbk, sizeof(g_sendMsgCbk), 0, sizeof(g_sendMsgCbk));
}

void TestSetAudioAutorateParam(uint8_t qosId, uint8_t startLevel)
{
    g_levelCnt = 8;
    uint8_t linkCnt = 2;
    QOSM_ICGMgrParam param = {};
    param.autorateParam.qosId = qosId;
    param.autorateParam.qosIndex = QOSM_QOSINDEX_AUDIO;
    param.autorateParam.supportedBitrate[0] = 192 / linkCnt;
    param.autorateParam.supportedBitrate[1] = 320 / linkCnt;
    param.autorateParam.supportedBitrate[2] = 640 / linkCnt;
    param.autorateParam.supportedBitrate[3] = 1500 / linkCnt;
    param.autorateParam.supportedBitrate[4] = 2300 / linkCnt;
    param.autorateParam.supportedBitrate[5] = 4600 / linkCnt;
    param.autorateParam.supportedBitrateCnt = 6;
    param.autorateParam.bitrate = 320;
    param.autorateParam.linkCnt = linkCnt;
    param.autorateParam.lcidCnt = 2;
    param.autorateParam.lcid[0] = g_lcid1;
    param.autorateParam.lcid[1] = g_lcid2;
    param.startParam.startLevel = startLevel;
    param.startParam.startBand = QOS_BAND_2D4;
    param.startParam.startDutyCycle = QOS_DUTY_CYCLE_100P;
    param.startParam.levelCnt = g_levelCnt;
    QOSM_ICGMgrSetAutorateParam(&param);
}

void TestSetCallParam(uint8_t qosId)
{
    g_levelCnt = 2;
    QOSM_ICGMgrParam param = {};
    param.autorateParam.qosId = qosId;
    param.autorateParam.qosIndex = QOSM_QOSINDEX_CALL;
    param.autorateParam.supportedBitrate[0] = 64;
    param.autorateParam.supportedBitrate[1] = 32;
    param.autorateParam.supportedBitrateCnt = 2;
    param.autorateParam.bitrate = 64;
    param.autorateParam.linkCnt = 2;
    param.autorateParam.lcidCnt = 2;
    param.autorateParam.lcid[0] = g_lcid1;
    param.autorateParam.lcid[1] = g_lcid2;
    param.startParam.startLevel = QOS_LEVEL_1;
    param.startParam.startBand = QOS_BAND_2D4;
    param.startParam.startDutyCycle = QOS_DUTY_CYCLE_100P;
    param.startParam.levelCnt = g_levelCnt;
    QOSM_ICGMgrSetAutorateParam(&param);
}

void TestSetOtherParam(uint8_t qosId, uint8_t qosIndex, uint8_t levelCnt, bool isAutorate)
{
    g_levelCnt = levelCnt;
    uint8_t linkCnt = 2;
    QOSM_ICGMgrParam param = {};
    param.autorateParam.qosId = qosId;
    param.autorateParam.qosIndex = qosIndex;
    param.autorateParam.supportedBitrate[0] = 96 / linkCnt;
    param.autorateParam.supportedBitrate[1] = 192 / linkCnt;
    param.autorateParam.supportedBitrate[2] = 320 / linkCnt;
    param.autorateParam.supportedBitrate[3] = 640 / linkCnt;
    param.autorateParam.supportedBitrate[4] = 960 / linkCnt;
    param.autorateParam.supportedBitrate[5] = 1500 / linkCnt;
    param.autorateParam.supportedBitrateCnt = 6;
    param.autorateParam.bitrate = 320;
    param.autorateParam.linkCnt = linkCnt;
    param.autorateParam.lcidCnt = 2;
    param.autorateParam.lcid[0] = g_lcid1;
    param.autorateParam.lcid[1] = g_lcid2;
    param.startParam.startLevel = QOS_LEVEL_3;
    param.startParam.startBand = QOS_BAND_2D4;
    param.startParam.startDutyCycle = QOS_DUTY_CYCLE_100P;
    param.startParam.levelCnt = g_levelCnt;
    if (isAutorate) {
        QOSM_ICGMgrSetAutorateParam(&param);
    } else {
        QOSM_ICGMgrSetParam(&param);
    }
}

void TestAddParamCbk(uint8_t qosId, int state, uint32_t errorCode, uint8_t channelCnt)
{
    for (uint8_t i = 0; i < g_levelCnt; i++) {
        CM_ICBConnection icbConn = {};
        icbConn.state = (CM_ICBConnectionState)state;
        icbConn.errorCode = errorCode;
        icbConn.id = qosId;
        icbConn.gHandle = g_gHandle;
        icbConn.channelCnt = channelCnt;
        CM_ICBChannel channel[2] = {};
        channel[0].lcid = g_lcid1;
        channel[0].connHandle = g_connHandle1;
        channel[1].lcid = g_lcid2;
        channel[1].connHandle = g_connHandle2;
        icbConn.channel = channel;
        g_icbCbk.connectionCbk(&icbConn);
    }
}

void TestLabelReportCbk(uint8_t qosId, uint16_t lcid, uint16_t connHandle, bool success, bool isCall)
{
    CM_ICBLabelReportParam labelReportParam = {};
    labelReportParam.errorCode = success ? CM_ICB_SUCCESS : CM_ICB_FAILED;
    labelReportParam.connHandle = connHandle;
    labelReportParam.lcid = lcid;
    labelReportParam.icgId = qosId;
    labelReportParam.labelCnt = 1;
    CM_ICBLabel label = {};
    if (isCall) {
        labelReportParam.icbInterval = 80;
        labelReportParam.bnG2T = 1;
        labelReportParam.bnT2G = 1;
        labelReportParam.ftG2T = 3;
        labelReportParam.ftT2G = 3;
        label.txPhy = 0;
        label.rxPhy = 0;
        label.txMcs = 0;
        label.rxMcs = 0;
        label.txFrame = 0;
        label.rxFrame = 0;
        label.maxSduG2T = 161;
        label.maxSduT2G = 161;
        label.maxPduG2T = 161;
        label.maxPduT2G = 161;
        label.nse = 3;
    } else {
        labelReportParam.icbInterval = 80;
        labelReportParam.bnG2T = 2;
        labelReportParam.bnT2G = 0;
        labelReportParam.ftG2T = 15;
        labelReportParam.ftT2G = 1;
        label.txPhy = 0;
        label.rxPhy = 0;
        label.txMcs = 6;
        label.rxMcs = 0;
        label.txFrame = 1;
        label.rxFrame = 0;
        label.maxSduG2T = 201;
        label.maxSduT2G = 0;
        label.maxPduG2T = 201;
        label.maxPduT2G = 0;
        label.nse = 5;
    }
    label.labelId = g_labelId++;
    labelReportParam.label = &label;
    g_icbCbk.labelReportCbk(&labelReportParam);
}

void TestEstablishedCbk(uint8_t qosId, uint16_t lcid, uint16_t connHandle, bool success)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = CM_ICB_STATE_IOB_CREATED;
    icbConn.errorCode = success ? CM_ICB_SUCCESS : CM_ICB_FAILED;
    icbConn.id = qosId;
    icbConn.gHandle = g_gHandle;
    icbConn.channelCnt = 1;
    channel.connHandle = connHandle;
    channel.lcid = lcid;
    icbConn.channel = &channel;
    g_icbCbk.connectionCbk(&icbConn);
}

void TestQualityCbkEx(uint16_t connHandle, uint8_t ackRate, int8_t rssi, uint16_t txFlushed, uint32_t diffMax,
    int icbType, uint8_t errPacketRate)
{
    CM_ICBQuality quality = {};
    quality.icbType = (CM_ICBType)icbType;
    quality.diffMax = diffMax;
    quality.connHandle = connHandle;
    quality.txFlushed = txFlushed;
    quality.rssi = rssi;
    quality.ackRate = ackRate;
    quality.errPacketRate = errPacketRate;
    g_icbCbk.qualityReportCbk(&quality);
}

void TestQualityCbk(uint16_t connHandle, uint8_t ackRate, int8_t rssi, uint16_t txFlushed, uint32_t diffMax)
{
    TestQualityCbkEx(connHandle, ackRate, rssi, txFlushed, diffMax, CM_IOB, 0);
}

void TestUpdateParamCbk(uint8_t qosId, uint16_t lcid, uint16_t connHandle, uint8_t labelId, uint32_t errorCode)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = CM_ICB_STATE_IMG_UPDATED;
    icbConn.errorCode = errorCode;
    icbConn.id = qosId;
    icbConn.gHandle = g_gHandle;
    icbConn.channelCnt = 1;
    channel.connHandle = connHandle;
    channel.lcid = lcid;
    channel.labelId = labelId;
    icbConn.channel = &channel;
    g_icbCbk.connectionCbk(&icbConn);
}

void TestDisconnectCbk(uint8_t qosId, uint16_t lcid, uint16_t connHandle, int disconnectType, uint32_t errorCode)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = CM_ICB_STATE_ICB_DELETED;
    icbConn.errorCode = errorCode;
    icbConn.disconnectType = (CM_LinkType)disconnectType;
    icbConn.id = qosId;
    icbConn.gHandle = g_gHandle;
    icbConn.channelCnt = 1;
    channel.connHandle = connHandle;
    channel.lcid = lcid;
    icbConn.channel = &channel;
    g_icbCbk.connectionCbk(&icbConn);
}

void TestRemoveParamCbk(uint8_t qosId, int state)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = (CM_ICBConnectionState)state;
    icbConn.errorCode = CM_ICB_SUCCESS;
    icbConn.id = qosId;
    icbConn.gHandle = g_gHandle;
    icbConn.channelCnt = 1;
    channel.connHandle = g_connHandle1;
    channel.lcid = g_lcid1;
    icbConn.channel = &channel;
    g_icbCbk.connectionCbk(&icbConn);
}

void TestDataPathCbk(uint8_t qosId, int state, uint8_t direction)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = (CM_ICBConnectionState)state;
    icbConn.errorCode = CM_ICB_SUCCESS;
    icbConn.id = qosId;
    icbConn.gHandle = g_gHandle;
    icbConn.channelCnt = 1;
    channel.connHandle = g_connHandle1;
    channel.lcid = g_lcid1;
    channel.direction = direction;
    icbConn.channel = &channel;
    g_icbCbk.connectionCbk(&icbConn);
}

void TestAddConnection(uint8_t qosId)
{
    QOSM_AutoRateConnParam connParam = {};
    QOSM_ConnParam link[2] = {};
    connParam.qosId = qosId;
    connParam.linkCnt = 2;
    link[0].connHandle = g_connHandle1;
    link[0].lcid = g_lcid1;
    link[1].connHandle = g_connHandle2;
    link[1].lcid = g_lcid2;
    connParam.link = link;
    QOSM_ICGMgrAddConnection(&connParam);
}

void TestDeleteConnection(uint8_t qosId, uint16_t connHandle)
{
    QOSM_AutoRateConnParam connParam = {};
    QOSM_ConnParam link = {};
    connParam.qosId = qosId;
    connParam.linkCnt = 1;
    link.connHandle = connHandle;
    connParam.link = &link;
    QOSM_ICGMgrDeleteConnection(&connParam);
}

void TestParamNotifyEarphone(const uint16_t *supportedBitrate, uint32_t cnt)
{
    QOSM_ICGMgrNotifyParam notifyParam = {};
    notifyParam.type = NOTIFY_TYPE_EARPHONE;
    for (uint32_t i = 0; i < cnt && i < QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT; i++) {
        notifyParam.supportedBitrate[i] = supportedBitrate[i];
    }
    notifyParam.supportedBitrateCnt = cnt;
    QOSM_ICGMgrParamNotify(&notifyParam);
}

void TestParamNotifyCoexist(uint16_t maxBitrate, uint8_t dutyCycle)
{
    QOSM_ICGMgrNotifyParam notifyParam = {};
    notifyParam.type = NOTIFY_TYPE_COEXIST;
    notifyParam.maxBitrate = maxBitrate;
    notifyParam.dutyCycle = dutyCycle;
    QOSM_ICGMgrParamNotify(&notifyParam);
}

void TestParamNotify5G(void)
{
    QOSM_ICGMgrNotifyParam notifyParam = {};
    notifyParam.type = NOTIFY_TYPE_5G;
    QOSM_ICGMgrParamNotify(&notifyParam);
}

void TestConnectIcg(uint8_t qosId)
{
    TestSetAudioAutorateParam(qosId, QOS_LEVEL_3);
    TestAddParamCbk(qosId, CM_ICB_STATE_IOG_CREATED, CM_ICB_SUCCESS, 2);
    TestAddConnection(qosId);
    TestLabelReportCbk(qosId, g_lcid1, g_connHandle1, true, false);
    TestLabelReportCbk(qosId, g_lcid2, g_connHandle2, true, false);
    TestEstablishedCbk(qosId, g_lcid1, g_connHandle1, true);
    TestEstablishedCbk(qosId, g_lcid2, g_connHandle2, true);
    QOSM_AutorateSetDspOn(true);
    g_bitrateCbkCnt = 0;
}

void TestConnectIcgAtLevel(uint8_t qosId, uint8_t startLevel)
{
    TestSetAudioAutorateParam(qosId, startLevel);
    TestAddParamCbk(qosId, CM_ICB_STATE_IOG_CREATED, CM_ICB_SUCCESS, 2);
    TestAddConnection(qosId);
    TestLabelReportCbk(qosId, g_lcid1, g_connHandle1, true, false);
    TestLabelReportCbk(qosId, g_lcid2, g_connHandle2, true, false);
    TestEstablishedCbk(qosId, g_lcid1, g_connHandle1, true);
    TestEstablishedCbk(qosId, g_lcid2, g_connHandle2, true);
    QOSM_AutorateSetDspOn(true);
    g_bitrateCbkCnt = 0;
}

void TestFireTimer(int timerId)
{
    if (timerId < 0 || timerId >= QOSM_TEST_TIMER_NUM) {
        return;
    }
    if (g_timerParam[timerId].callback != nullptr) {
        SDF_TimerParam param = g_timerParam[timerId];
        (void)memset_s(&g_timerParam[timerId], sizeof(SDF_TimerParam), 0, sizeof(SDF_TimerParam));
        param.callback(param.args);
    }
}

void TestRegisterCallback(void)
{
    QOSM_AutoRateCallback callback = {};
    callback.paramChangedCbk = TestParamChangedCbk;
    callback.connChangedCbk = TestConnChangedCbk;
    callback.dataPathChangedCbk = TestDataPathChangedCbk;
    callback.bitrateChangedCbk = TestBitrateChangedCbk;
    callback.callBitrateUpDownCbk = TestSendMsgCbk;
    (void)QOSM_ICGMgrRegisterCallback(&callback);
}

void TestUnregisterCallback(void)
{
    (void)QOSM_ICGMgrUnregisterCallback();
}
