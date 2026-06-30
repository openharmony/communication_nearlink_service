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

#include <gtest/gtest.h>
#include "securec.h"

#include "cm_def.h"
#include "cm_errno.h"
#include "cm_icb_api.h"
#include "cm_icb_mgr.h"
#include "qosm_audio_dfx.h"
#include "qosm_errno.h"
#include "qosm_icg_mgr.h"
#include "qosm_log.h"
#include "qosm_table_mgr.h"
#include "sdf_timer.h"
#include "nlstk_cfgdb_api.h"
#include "nlstk_public_define.h"
#include "sdf_worker.h"
#include "qosm_autorate_test_log.h"
#include "common_ext_func_wrapper.h"
#include "common_reg_ext_func.h"

#define AUDIO_QOS_LEVEL_CNT 8
#define OTHER_QOS_LEVEL_CNT 8
#define TIMER_NUM 6
#define DELAY_PARAM_TIMER_ID 0
#define DELAY_CONN_TIMER_ID 1
#define ADD_CONN_TIMER_ID 2
#define RETRY_CONN_TIMER_ID 3
#define UPGRADE_LABEL_TIMER_ID 4
#define DOWNGRADE_LABEL_TIMER_ID 5

#define DELAY_PARAM_TIMEOUT_MS 2000 // 2000ms，链路建立过程中set param
#define DELAY_CONN_TIMEOUT_MS 2000 // 2000ms，更新参数期间add connection
#define ADD_CONN_TIMEOUT_MS 4000 // 4000ms
#define RETRY_CONN_TIMEOUT_MS 100 // 100ms
#define UPGRADE_LABEL_TIMEOUT_MS 1000 // 升档1000ms
#define DOWNGRADE_LABEL_TIMEOUT_MS 5000 // 降档5000ms

#ifdef __cplusplus
extern "C" {
#endif

struct __attribute__((packed)) QOSM_PowerLevelInfo {
    uint16_t connHandle;
    uint8_t level;
};

static CM_ICBCallback g_icbCbk = {};
static struct QOSM_AudioDfxInfo g_audioDfxInfo = {};
static SDF_TimerParam g_timerParam[TIMER_NUM] = {};
static CM_FreqBandListener g_freqBandListener = {};
static uint32_t g_freqBand = 0;
static NLSTK_CfgdbConfigListenerFunc g_powerLevelListner = {};
static uint8_t g_powerLevel = 0;

static uint8_t g_levelCnt = 0;
static uint8_t g_labelId = 0;
static uint32_t g_registerCbkRet = CM_SUCCESS;
static uint32_t g_listenFreqBandRet = CM_SUCCESS;
static uint32_t g_setParamRet = CM_SUCCESS;
static uint32_t g_setTestParamRet = CM_SUCCESS;
static uint32_t g_setLabelRet = CM_SUCCESS;
static uint32_t g_cmSetDataPathRet = CM_SUCCESS;
static uint32_t g_cmRemoveDataPathRet = CM_SUCCESS;
static uint32_t g_removeParamRet = CM_SUCCESS;
static uint32_t g_deleteConnectionRet = CM_SUCCESS;
static uint32_t g_cpTimerRet = 0;
static uint8_t g_isDelaySetParam = false;

static CM_ICGLabelParam g_labelParam = {};
static CM_ICBChannel g_labelIcb[2] = {};
static CM_ICBConnectionParam g_createParam = {};
static CM_ICBChannel g_createChannel[2] = {};
static CM_ICGUpdatedParam g_updateParam = {};
static uint16_t g_updateChannel[2] = {};
static CM_ICBConnectionParam g_disconnectParam = {};
static CM_ICBChannel g_disconnectChannel[2] = {};
static CM_ICGParam g_settedParam = {};
static CM_ICGTestParam g_settedTestParam = {};
static CM_ICGRemovedParam g_removedParam = {};

static QOSM_ParamCb g_paramCbk[2] = {}; // 0: set param, 1: remove param
static QOSM_ConnParamCb g_connParam[2] = {}; // 0: add connection, 1: delete connection
static QOSM_ConnParam g_connParamLink[4] = {}; // 0、1: add connection, 2、3: delete connection
static QOSM_DataPathParamCb g_dataPath[2] = {}; // 0: setup data path, 1: remove data path
static QOSM_BitrateParamCb g_bitrateParam[2] = {}; // 0: downgrade, 1: upgrade

static uint8_t g_qosId = 1;
static uint16_t g_lcid1 = 3;
static uint16_t g_lcid2 = 4;
static uint16_t g_gHandle = 0xA2;
static uint16_t g_connHandle1 = 0xA3;
static uint16_t g_connHandle2 = 0xA4;
static uint8_t g_levelUpAckrateOverCnt = 8;

uint32_t CM_ICBRegisterCbk(const CM_ICBCallback *cb)
{
    if (g_registerCbkRet != CM_SUCCESS) {
        return g_registerCbkRet;
    }
    if (memcpy_s(&g_icbCbk, sizeof(CM_ICBCallback), cb, sizeof(CM_ICBCallback)) != EOK) {
        return CM_FAIL;
    }
    return g_registerCbkRet;
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
    if (memcpy_s(&g_freqBandListener, sizeof(CM_FreqBandListener), &listener, sizeof(CM_FreqBandListener)) != EOK) {
        return CM_FAIL;
    }
    return g_listenFreqBandRet;
}

uint32_t CM_ICBMgrUnlistenFreqBandSwitchEvent(CM_FreqBandListener listener)
{
    if (g_freqBandListener == listener) {
        (void)memset_s(&g_freqBandListener, sizeof(CM_FreqBandListener), 0, sizeof(CM_FreqBandListener));
    }
    return CM_SUCCESS;
}

uint32_t CM_ICGSetParam(CM_ICGParam *icgParam)
{
    if (g_setParamRet != CM_SUCCESS) {
        return g_setParamRet;
    }
    if (memcpy_s(&g_settedParam, sizeof(g_settedParam), icgParam, sizeof(g_settedParam)) != EOK) {
        return CM_FAIL;
    }
    return g_setParamRet;
}

uint32_t CM_ICGSetTestParam(CM_ICGTestParam *icgParam, bool supportAutorate)
{
    if (g_setTestParamRet != CM_SUCCESS) {
        return g_setTestParamRet;
    }
    if (memcpy_s(&g_settedTestParam, sizeof(g_settedTestParam), icgParam, sizeof(g_settedTestParam)) != EOK) {
        return CM_FAIL;
    }
    return g_setTestParamRet;
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
    for (uint8_t i = 0; i < icgLabel->icbCnt; i++) {
        if (memcpy_s(&g_labelIcb[i], sizeof(CM_ICBChannel), &icgLabel->icb[i], sizeof(CM_ICBChannel)) != EOK) {
            return CM_FAIL;
        }
    }
    return g_setLabelRet;
}

uint32_t CM_ICBAddConnection(CM_ICBConnectionParam *connParam, bool supportAutorate)
{
    if (connParam->channelCnt == 0) {
        return CM_FAIL;
    }
    if (memcpy_s(&g_createParam, sizeof(CM_ICBConnectionParam), connParam, sizeof(CM_ICBConnectionParam)) != EOK) {
        return CM_FAIL;
    }
    for (uint8_t i = 0; i < connParam->channelCnt; i++) {
        if (memcpy_s(&g_createChannel[i], sizeof(CM_ICBChannel), &connParam->channel[i],
            sizeof(CM_ICBChannel)) != EOK) {
            return CM_FAIL;
        }
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
    for (uint8_t i = 0; i < icgParam->icbCnt; i++) {
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
    if (memcpy_s(&g_disconnectParam, sizeof(CM_ICBConnectionParam), connParam, sizeof(CM_ICBConnectionParam)) != EOK) {
        return CM_FAIL;
    }
    for (uint8_t i = 0; i < connParam->channelCnt; i++) {
        if (memcpy_s(&g_disconnectChannel[i], sizeof(CM_ICBChannel), &connParam->channel[i],
            sizeof(CM_ICBChannel)) != EOK) {
            return CM_FAIL;
        }
    }
    return g_deleteConnectionRet;
}

uint32_t CM_ICGRemoveParam(CM_ICGRemovedParam *icgParam)
{
    if (g_removeParamRet != CM_SUCCESS) {
        return g_removeParamRet;
    }
    if (memcpy_s(&g_removedParam, sizeof(CM_ICGRemovedParam), icgParam, sizeof(CM_ICGRemovedParam)) != EOK) {
        return CM_FAIL;
    }
    return g_removeParamRet;
}

uint32_t CP_TimerAdd(int *handle, SDF_TimerParam *param)
{
    if (g_cpTimerRet != 0) {
        return g_cpTimerRet;
    }
    int timerId = -1;
    if (param->expires == DELAY_PARAM_TIMEOUT_MS) {
        if (g_isDelaySetParam) {
            timerId = DELAY_PARAM_TIMER_ID;
        } else {
            timerId = DELAY_CONN_TIMER_ID;
        }
    } else if (param->expires == ADD_CONN_TIMEOUT_MS) {
        timerId = ADD_CONN_TIMER_ID;
    } else if (param->expires == RETRY_CONN_TIMEOUT_MS) {
        timerId = RETRY_CONN_TIMER_ID;
    } else if (param->expires == UPGRADE_LABEL_TIMEOUT_MS) {
        // "HOST通知BTC升PHY等待1000ms超时"与"BTC升PHY成功后延迟1000ms通知DSP"都是1000ms，共用timerId
        timerId = UPGRADE_LABEL_TIMER_ID;
    } else if (param->expires == DOWNGRADE_LABEL_TIMEOUT_MS) {
        timerId = DOWNGRADE_LABEL_TIMER_ID;
    } else {
        timerId = -1;
    }
    if (timerId == -1) {
        return 1;
    }
    if (memcpy_s(&g_timerParam[timerId], sizeof(SDF_TimerParam), param, sizeof(SDF_TimerParam)) != EOK) {
        return 1;
    }
    *handle = timerId;
    return g_cpTimerRet;
}

void CP_TimerDel(int handle)
{
    if (handle < DELAY_PARAM_TIMER_ID || handle >= TIMER_NUM) {
        return;
    }
    (void)memset_s(&g_timerParam[handle], sizeof(SDF_TimerParam), 0, sizeof(SDF_TimerParam));
}

uint32_t CM_GetLogicLinkByLcid(uint16_t lcid, CM_LogicLink_S *logicLink)
{
    return CM_SUCCESS;
}

NLSTK_Errcode_E NLSTK_CfgdbRegisterConfigListener(NLSTK_CfgdbConfigListenerFunc func, NLSTK_CfgdbModule_E module,
    NLSTK_CfgdbConfig_E config)
{
    if (module == NLSTK_CFGDB_MODULE_QOSM && config == NLSTK_CFGDB_CONFIG_POWER_LEVEL) {
        if (memcpy_s(&g_powerLevelListner, sizeof(NLSTK_CfgdbConfigListenerFunc), &func,
            sizeof(NLSTK_CfgdbConfigListenerFunc)) != EOK) {
            return NLSTK_ERRCODE_FAIL;
        }
    }
    return NLSTK_ERRCODE_SUCCESS;
}

bool CM_InnerGetRemotePrivateFeature(uint16_t lcid, uint16_t featureBit)
{
    return true;
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

uint32_t SDF_EvcInstanceCreate(int *handle, const char *name)
{
    return 0;
}

uint32_t SDF_EvcListenEvent(int handle, SDF_EvcEvent *event)
{
    return 0;
}

void SDF_EvcCancelEvent(int eventHandle)
{
}

void SDF_EvcInstanceClose(int handle)
{
}

void QOSM_AudioDfxStart(struct QOSM_AudioDfxInfo *info)
{
    (void)memcpy_s(&g_audioDfxInfo, sizeof(struct QOSM_AudioDfxInfo), info, sizeof(struct QOSM_AudioDfxInfo));
}

void QOSM_AudioDfxUpdateBand(uint32_t band)
{
    g_freqBand = band;
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
}

void QOSM_AudioDfxGetDspStatusInner(void *arg)
{
}

#ifdef __cplusplus
}
#endif

static void QOSM_ParamChangedCbk(const QOSM_ParamCb *param)
{
    QOSM_LOGI("QOSM_ParamChangedCbk qosId:%u, state:%u", param->qosId, param->state);
    for (uint8_t i = 0; i < param->linkCnt; i++) {
        QOSM_LOGI("QOSM_ParamChangedCbk connHandle:%u", param->connHandle[i]);
    }
    if (param->state == QOSM_PARAM_SETTED) {
        (void)memcpy_s(&g_paramCbk[0], sizeof(QOSM_ParamCb), param, sizeof(QOSM_ParamCb));
    } else if (param->state == QOSM_PARAM_REMOVED) {
        (void)memcpy_s(&g_paramCbk[1], sizeof(QOSM_ParamCb), param, sizeof(QOSM_ParamCb));
    }
}

static void QOSM_ConnectionChangedCbk(const QOSM_ConnParamCb *param)
{
    QOSM_LOGI("QOSM_ConnectionChangedCbk qosId:%u, state:%u", param->qosId, param->state);
    for (uint8_t i = 0; i < param->linkCnt; i++) {
        QOSM_LOGI("QOSM_ConnectionChangedCbk lcid:%u, connHandle:%u", param->link[i].lcid, param->link[i].connHandle);
    }
    if (param->state == QOSM_CONNECTION_ADDED) {
        (void)memcpy_s(&g_connParam[0], sizeof(QOSM_ConnParamCb), param, sizeof(QOSM_ConnParamCb));
        for (uint8_t i = 0; i < param->linkCnt; i++) {
            g_connParamLink[i].lcid = param->link[i].lcid;
            g_connParamLink[i].connHandle = param->link[i].connHandle;
        }
    } else if (param->state == QOSM_CONNECTION_DELETED) {
        (void)memcpy_s(&g_connParam[1], sizeof(QOSM_ConnParamCb), param, sizeof(QOSM_ConnParamCb));
        for (uint8_t i = 0; i < param->linkCnt; i++) {
            g_connParamLink[i + 2].lcid = param->link[i].lcid;
            g_connParamLink[i + 2].connHandle = param->link[i].connHandle;
        }
    }
}

static void QOSM_DataPathChangedCbk(const QOSM_DataPathParamCb *param)
{
    QOSM_LOGI("QOSM_DataPathChangedCbk qosId:%u, state:%u, connHandle:%u, direction:%u", param->qosId, param->state,
        param->connHandle, param->direction);
    if (param->state == QOSM_DATAPATH_ADDED) {
        (void)memcpy_s(&g_dataPath[0], sizeof(QOSM_DataPathParamCb), param, sizeof(QOSM_DataPathParamCb));
    } else if (param->state == QOSM_DATAPATH_DELETED) {
        (void)memcpy_s(&g_dataPath[1], sizeof(QOSM_DataPathParamCb), param, sizeof(QOSM_DataPathParamCb));
    }
}

static void QOSM_BitrateChangedCbk(const QOSM_BitrateParamCb *param, uint8_t paramCnt)
{
    if (param->labelId == NEARLINK_INVALID_LABEL) {
        QOSM_LOGI("bitrate not changed, just set icg info");
        return;
    }
    QOSM_LOGI("QOSM_BitrateChangedCbk qosId:%u, direction:%u, labelId:%u, downwardBitrate:%u",
        param->qosId, param->direction, param->labelId, param->downwardBitrate);
    uint16_t direction = param->direction;
    if (direction == LEVEL_DOWN) {
        (void)memcpy_s(&g_bitrateParam[0], sizeof(QOSM_BitrateParamCb), param, sizeof(QOSM_BitrateParamCb));
    } else if (direction == LEVEL_UP) {
        (void)memcpy_s(&g_bitrateParam[1], sizeof(QOSM_BitrateParamCb), param, sizeof(QOSM_BitrateParamCb));
    }
}

static void QOSM_FrequencyBandChangedCbk(const QOSM_FrequencyBandParamCb *param)
{
}

static void QOSM_HighPowerModeChangedCbk(const QOSM_HighPowerModeParamCb *param)
{
}

class UT_QOSM_ICG_MGR_TEST : public testing::Test {
protected:
    void SetUp()
    {
        g_registerCbkRet = CM_SUCCESS;
        g_listenFreqBandRet = CM_SUCCESS;
        g_setParamRet = CM_SUCCESS;
        g_setTestParamRet = CM_SUCCESS;
        g_setLabelRet = CM_SUCCESS;
        g_cmSetDataPathRet = CM_SUCCESS;
        g_cmRemoveDataPathRet = CM_SUCCESS;
        g_removeParamRet = CM_SUCCESS;
        g_deleteConnectionRet = CM_SUCCESS;
        g_cpTimerRet = 0;
        g_isDelaySetParam = false;

        (void)memset_s(&g_labelParam, sizeof(g_labelParam), 0, sizeof(g_labelParam));
        (void)memset_s(&g_labelIcb, sizeof(CM_ICBChannel), 0, sizeof(CM_ICBChannel));

        (void)memset_s(&g_createParam, sizeof(g_createParam), 0, sizeof(g_createParam));
        (void)memset_s(&g_createChannel, sizeof(g_createChannel), 0, sizeof(g_createChannel));

        (void)memset_s(&g_updateParam, sizeof(g_updateParam), 0, sizeof(g_updateParam));
        (void)memset_s(&g_updateChannel, sizeof(g_updateChannel), 0, sizeof(g_updateChannel));

        (void)memset_s(&g_disconnectParam, sizeof(g_disconnectParam), 0, sizeof(g_disconnectParam));
        (void)memset_s(&g_disconnectChannel, sizeof(g_disconnectChannel), 0, sizeof(g_disconnectChannel));

        (void)memset_s(&g_settedParam, sizeof(g_settedParam), 0, sizeof(g_settedParam));
        (void)memset_s(&g_settedTestParam, sizeof(g_settedTestParam), 0, sizeof(g_settedTestParam));
        (void)memset_s(&g_removedParam, sizeof(g_removedParam), 0, sizeof(g_removedParam));

        (void)memset_s(&g_dataPath, sizeof(g_dataPath), 0, sizeof(g_dataPath));
        (void)memset_s(&g_bitrateParam, sizeof(g_bitrateParam), 0, sizeof(g_bitrateParam));
        (void)memset_s(&g_timerParam, sizeof(g_timerParam), 0, sizeof(g_timerParam));

        (void)memset_s(&g_connParam, sizeof(g_connParam), 0, sizeof(g_connParam));
        (void)memset_s(&g_connParamLink, sizeof(g_connParamLink), 0, sizeof(g_connParamLink));
    }

    virtual void TearDown()
    {
    }

    static void SetUpTestCase()
    {
        QOSM_ICGMgrEnable();

        QOSM_AutoRateCallback callback = {};
        callback.paramChangedCbk = QOSM_ParamChangedCbk;
        callback.connChangedCbk = QOSM_ConnectionChangedCbk;
        callback.dataPathChangedCbk = QOSM_DataPathChangedCbk;
        callback.bitrateChangedCbk = QOSM_BitrateChangedCbk;
        callback.frequencyBandChangedCbk = QOSM_FrequencyBandChangedCbk;
        callback.highPowerModeChangedCbk = QOSM_HighPowerModeChangedCbk;
        uint32_t ret = QOSM_ICGMgrRegisterCallback(&callback);
        EXPECT_EQ(ret, QOSM_SUCCESS);
        EXPECT_NE(g_icbCbk.connectionCbk, nullptr);
    }

    static void TearDownTestCase()
    {
        uint32_t ret = QOSM_ICGMgrUnregisterCallback();
        EXPECT_EQ(ret, QOSM_SUCCESS);
        EXPECT_EQ(g_icbCbk.connectionCbk, nullptr);
        QOSM_ICGMgrDisable();
    }
};

static void QOSM_ICGMgrSetAudioTestParam(uint8_t qosId, uint8_t levelCnt)
{
    g_levelCnt = levelCnt;
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
    param.startParam.startLevel = QOS_LEVEL_3;
    param.startParam.startBand = QOS_BAND_2D4;
    param.startParam.startDutyCycle = QOS_DUTY_CYCLE_100P;
    param.startParam.levelCnt = g_levelCnt;
    QOSM_ICGMgrSetTestParam(&param);
}

static void QOSM_ICGMgrSetOtherParam(uint8_t qosId, QOSM_QosIndex qosIndex, uint8_t levelCnt, bool isTest)
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
    if (isTest) {
        QOSM_ICGMgrSetTestParam(&param);
    } else {
        QOSM_ICGMgrSetParam(&param);
    }
}

static void ICB_AddParamCbk(uint8_t qosId, CM_ICBConnectionState state)
{
    for (uint8_t i = 0; i < g_levelCnt; i++) {
        CM_ICBConnection icbConn = {};
        icbConn.state = state;
        icbConn.errorCode = CM_ICB_SUCCESS;
        icbConn.id = qosId;
        icbConn.gHandle = g_gHandle;
        icbConn.channelCnt = 2;
        CM_ICBChannel channel[2] = {};
        channel[0].lcid = g_lcid1;
        channel[0].connHandle = g_connHandle1;
        channel[1].lcid = g_lcid2;
        channel[1].connHandle = g_connHandle2;
        icbConn.channel = channel;
        g_icbCbk.connectionCbk(&icbConn);
    }
}

static void ICB_LabelReportCbk(uint8_t qosId, uint16_t lcid, uint16_t connHandle, bool setLabelSuccess)
{
    CM_ICBLabelReportParam labelReportParam = {};
    labelReportParam.errorCode = setLabelSuccess ? CM_ICB_SUCCESS : CM_ICB_FAILED;
    labelReportParam.connHandle = connHandle;
    labelReportParam.lcid = lcid;
    labelReportParam.icgId = qosId;
    labelReportParam.labelCnt = 1;
    // {
    //     QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_3 /* qosLevel */, QOS_LEVEL_4 /* upQosLevel */,
    //     QOS_LEVEL_1 /* downQosLevel */, 320 /* downwardBitrate */, 0 /* upwardBitrate */,
    //     10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
    //     300 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 15 /* ftG2T */, 1 /* ftT2G */,
    //     57 /* rtnG2T */, 1 /* rtnT2G */, QOS_MAKE_SDU_LEN(200) /* maxSduG2T */, 0 /* maxSduT2G */,
    //     QOS_MAKE_SDU_LEN(200) /* maxPduG2T */, 0 /* maxPduT2G */,
    //     5 /* nse */, 2 /* bnG2T */, 0 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
    //     6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    // }
    labelReportParam.icbInterval = 80;
    labelReportParam.bnG2T = 2;
    labelReportParam.bnT2G = 0;
    labelReportParam.ftG2T = 15;
    labelReportParam.ftT2G = 1;
    CM_ICBLabel label = {};
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
    label.labelId = g_labelId++;
    labelReportParam.label = &label;
    g_icbCbk.labelReportCbk(&labelReportParam);
}

static void ICB_EstablishedCbk(uint8_t qosId, uint16_t lcid, uint16_t connHandle, bool createSuccess)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = CM_ICB_STATE_IOB_CREATED;
    icbConn.errorCode = createSuccess ? CM_ICB_SUCCESS : CM_ICB_FAILED;
    icbConn.isIMG = false;
    icbConn.gHandle = 0XA2;
    icbConn.id = qosId;
    icbConn.channelCnt = 1;
    channel.connHandle = connHandle;
    channel.lcid = lcid;
    icbConn.channel = &channel;
    g_icbCbk.connectionCbk(&icbConn);
}

static void ICB_DataPathAddedCbk(uint8_t qosId, uint16_t lcid, uint16_t connHandle, uint8_t direction)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = CM_ICB_STATE_ICB_DATA_PATH_SETUP;
    icbConn.errorCode = CM_ICB_SUCCESS;
    icbConn.isIMG = false;
    icbConn.gHandle = g_gHandle;
    icbConn.id = qosId;
    icbConn.channelCnt = 1;
    channel.connHandle = connHandle;
    channel.lcid = lcid;
    channel.direction = direction;
    icbConn.channel = &channel;
    g_icbCbk.connectionCbk(&icbConn);
}

static void ICB_QualityCbk(uint16_t connHandle, uint8_t ackRate, int8_t rssi, uint16_t txFlushed, uint32_t diffMax)
{
    CM_ICBQuality quality = {};
    quality.diffMax = diffMax;
    quality.connHandle = connHandle;
    quality.txFlushed = txFlushed;
    quality.rssi = rssi;
    quality.ackRate = ackRate;
    g_icbCbk.qualityReportCbk(&quality);
}

static void ICB_UpdateParamCbk(uint8_t qosId, uint16_t lcid, uint16_t connHandle, uint8_t labelId)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = CM_ICB_STATE_IMG_UPDATED;
    icbConn.errorCode = CM_ICB_SUCCESS;
    icbConn.isIMG = false;
    icbConn.gHandle = g_gHandle;
    icbConn.id = qosId;
    icbConn.channelCnt = 1;
    channel.connHandle = connHandle;
    channel.lcid = lcid;
    channel.labelId = labelId;
    icbConn.channel = &channel;
    g_icbCbk.connectionCbk(&icbConn);
}

static void ICB_DataPathDeletedCbk(uint8_t qosId, uint16_t lcid, uint16_t connHandle, uint8_t direction)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = CM_ICB_STATE_ICB_DATA_PATH_REMOVED;
    icbConn.errorCode = CM_ICB_SUCCESS;
    icbConn.isIMG = false;
    icbConn.gHandle = g_gHandle;
    icbConn.id = qosId;
    icbConn.channelCnt = 1;
    channel.connHandle = connHandle;
    channel.lcid = lcid;
    channel.direction = direction;
    icbConn.channel = &channel;
    g_icbCbk.connectionCbk(&icbConn);
}

static void ICB_DisconnectCbk(uint8_t qosId, uint16_t lcid, uint16_t connHandle, CM_LinkType disconnectType)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = CM_ICB_STATE_ICB_DELETED;
    icbConn.errorCode = CM_ICB_SUCCESS;
    icbConn.disconnectType = disconnectType;
    icbConn.isIMG = false;
    icbConn.gHandle = g_gHandle;
    icbConn.id = qosId;
    icbConn.channelCnt = 1;
    channel.connHandle = connHandle;
    channel.lcid = lcid;
    icbConn.channel = &channel;
    g_icbCbk.connectionCbk(&icbConn);
}

static void ICB_RemoveParamCbk(uint8_t qosId, CM_ICBConnectionState state)
{
    CM_ICBConnection icbConn = {};
    icbConn.state = state;
    icbConn.errorCode = CM_ICB_SUCCESS;
    icbConn.id = qosId;
    icbConn.gHandle = g_gHandle;
    icbConn.channelCnt = 2;
    CM_ICBChannel channel[2] = {};
    channel[0].lcid = g_lcid1;
    channel[0].connHandle = g_connHandle1;
    channel[1].lcid = g_lcid2;
    channel[1].connHandle = g_connHandle2;
    icbConn.channel = channel;
    g_icbCbk.connectionCbk(&icbConn);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseRegisterCbkFail)
{
    QOSM_AutoRateCallback callback = {};
    callback.paramChangedCbk = QOSM_ParamChangedCbk;
    callback.connChangedCbk = QOSM_ConnectionChangedCbk;
    callback.dataPathChangedCbk = QOSM_DataPathChangedCbk;
    callback.bitrateChangedCbk = QOSM_BitrateChangedCbk;
    callback.frequencyBandChangedCbk = QOSM_FrequencyBandChangedCbk;
    callback.highPowerModeChangedCbk = QOSM_HighPowerModeChangedCbk;

    // CM_ICBMgrListenFreqBandSwitchEvent failed
    g_listenFreqBandRet = CM_FAIL;
    uint32_t ret = QOSM_ICGMgrRegisterCallback(&callback);
    EXPECT_EQ(ret, QOSM_REGISTER_CBK_ERR);

    // CM_ICBRegisterCbk failed
    g_listenFreqBandRet = CM_SUCCESS;
    g_registerCbkRet = CM_FAIL;
    ret = QOSM_ICGMgrRegisterCallback(&callback);
    EXPECT_EQ(ret, QOSM_REGISTER_CBK_ERR);

    g_registerCbkRet = CM_SUCCESS;
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseSetParamAndRemoveParam)
{
    // QOSM_CreateQosICGInfo failed
    QOSM_HookLog();
    QOSM_ICGMgrSetOtherParam(g_qosId, QOSM_QOSINDEX_MAX, OTHER_QOS_LEVEL_CNT, false);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "qos param is null"), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "QOSM_CreateQosICGInfo failed"), nullptr);
    QOSM_UnhookLog();

    // set param return fail
    g_setParamRet = CM_FAIL;
    QOSM_ICGMgrSetOtherParam(g_qosId, QOSM_QOSINDEX_OTHERS, OTHER_QOS_LEVEL_CNT, false);
    EXPECT_EQ(g_paramCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[0].state, QOSM_PARAM_SETTED);
    EXPECT_NE(g_paramCbk[0].result, 0);

    // set param success
    g_setParamRet = CM_SUCCESS;
    QOSM_ICGMgrSetOtherParam(g_qosId, QOSM_QOSINDEX_OTHERS, OTHER_QOS_LEVEL_CNT, false);
    EXPECT_EQ(g_settedParam.id, g_qosId);
    ICB_AddParamCbk(g_qosId, CM_ICB_STATE_IOG_CREATED);
    EXPECT_EQ(g_paramCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[0].state, QOSM_PARAM_SETTED);
    EXPECT_EQ(g_paramCbk[0].result, 0);

    // set same param repeatly: remove param and then set param, remove param will no callback to caller
    QOSM_ICGMgrSetOtherParam(g_qosId, QOSM_QOSINDEX_OTHERS, OTHER_QOS_LEVEL_CNT, false);
    EXPECT_EQ(g_removedParam.id, g_qosId);
    ICB_RemoveParamCbk(g_qosId, CM_ICB_STATE_IOG_REMOVED);
    EXPECT_EQ(g_settedParam.id, g_qosId);
    ICB_AddParamCbk(g_qosId, CM_ICB_STATE_IOG_CREATED);
    EXPECT_EQ(g_paramCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[0].state, QOSM_PARAM_SETTED);
    EXPECT_EQ(g_paramCbk[0].result, 0);

    QOSM_ICGMgrRemoveParam((void *)(uintptr_t)g_qosId);
    EXPECT_EQ(g_removedParam.id, g_qosId);
    ICB_RemoveParamCbk(g_qosId, CM_ICB_STATE_IOG_REMOVED);
    EXPECT_EQ(g_paramCbk[1].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[1].state, QOSM_PARAM_REMOVED);
    EXPECT_EQ(g_paramCbk[1].result, 0);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseSetTestParam)
{
    // QOSM_CreateQosICGInfo failed
    QOSM_HookLog();
    QOSM_ICGMgrSetOtherParam(g_qosId, QOSM_QOSINDEX_MAX, OTHER_QOS_LEVEL_CNT, true);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "qos param is null"), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "QOSM_CreateQosICGInfo failed"), nullptr);
    QOSM_UnhookLog();

    // set param return fail
    g_setTestParamRet = CM_FAIL;
    QOSM_ICGMgrSetOtherParam(g_qosId, QOSM_QOSINDEX_OTHERS, OTHER_QOS_LEVEL_CNT, true);
    EXPECT_EQ(g_paramCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[0].state, QOSM_PARAM_SETTED);
    EXPECT_NE(g_paramCbk[0].result, 0);

    // set param success
    g_setTestParamRet = CM_SUCCESS;
    QOSM_ICGMgrSetOtherParam(g_qosId, QOSM_QOSINDEX_OTHERS, OTHER_QOS_LEVEL_CNT, true);
    EXPECT_EQ(g_settedTestParam.id, g_qosId);
    ICB_AddParamCbk(g_qosId, CM_ICB_STATE_IOG_CREATED);
    EXPECT_EQ(g_paramCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[0].state, QOSM_PARAM_SETTED);
    EXPECT_EQ(g_paramCbk[0].result, 0);

    // set same param repeatly: remove param and then set param, remove param will no callback to caller
    QOSM_ICGMgrSetOtherParam(g_qosId, QOSM_QOSINDEX_OTHERS, OTHER_QOS_LEVEL_CNT, true);
    EXPECT_EQ(g_removedParam.id, g_qosId);
    ICB_RemoveParamCbk(g_qosId, CM_ICB_STATE_IOG_REMOVED);
    EXPECT_EQ(g_paramCbk[1].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[1].state, QOSM_PARAM_REMOVED);
    EXPECT_EQ(g_paramCbk[1].result, 0);
    EXPECT_EQ(g_settedTestParam.id, g_qosId);
    ICB_AddParamCbk(g_qosId, CM_ICB_STATE_IOG_CREATED);
    EXPECT_EQ(g_paramCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[0].state, QOSM_PARAM_SETTED);
    EXPECT_EQ(g_paramCbk[0].result, 0);
}

static void QOSM_AddConnection(uint8_t qosId)
{
    QOSM_AutoRateConnParam param = {};
    param.qosId = qosId;
    param.linkCnt = 2;
    QOSM_ConnParam link[2] = {};
    link[0].lcid = g_lcid1;
    link[0].connHandle = g_connHandle1;
    link[1].lcid = g_lcid2;
    link[1].connHandle = g_connHandle2;
    param.link = link;
    QOSM_ICGMgrAddConnection(&param);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseSetLabelFail)
{
    // icgInfo is NULL
    QOSM_HookLog();
    QOSM_AddConnection(100);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "icgInfo is NULL"), nullptr);

    // contain invalid conn handle
    QOSM_AutoRateConnParam param = {};
    param.qosId = g_qosId;
    param.linkCnt = 2;
    QOSM_ConnParam link[2] = {};
    link[0].lcid = g_lcid1;
    link[0].connHandle = g_lcid1;
    link[1].lcid = g_lcid2;
    link[1].connHandle = g_lcid2;
    param.link = link;
    QOSM_ICGMgrAddConnection(&param);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "contain invalid conn handle"), nullptr);

    // CM_ICGSetLabel failed
    g_setLabelRet = CM_FAIL;
    QOSM_AddConnection(g_qosId);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "CM_ICGSetLabel failed"), nullptr);
    QOSM_UnhookLog();

    // add connection timeout, will remove param
    g_setLabelRet = CM_SUCCESS;
    QOSM_AddConnection(g_qosId);
    EXPECT_NE(g_timerParam[ADD_CONN_TIMER_ID].callback, nullptr);
    g_timerParam[ADD_CONN_TIMER_ID].callback(g_timerParam[ADD_CONN_TIMER_ID].args);
    EXPECT_EQ(g_connParam[0].qosId, g_qosId);
    EXPECT_EQ(g_connParam[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_NE(g_connParam[0].result, 0);
    EXPECT_EQ(g_connParam[0].linkCnt, 2);
    EXPECT_EQ(g_connParamLink[0].lcid, 0);
    EXPECT_EQ(g_connParamLink[0].connHandle, g_connHandle1);
    EXPECT_EQ(g_connParamLink[1].lcid, 0);
    EXPECT_EQ(g_connParamLink[1].connHandle, g_connHandle2);

    EXPECT_EQ(g_removedParam.id, g_qosId);
    ICB_RemoveParamCbk(g_qosId, CM_ICB_STATE_IOG_REMOVED);
    EXPECT_EQ(g_paramCbk[1].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[1].state, QOSM_PARAM_REMOVED);
    EXPECT_EQ(g_paramCbk[1].result, 0);

    QOSM_ICGMgrSetOtherParam(g_qosId, QOSM_QOSINDEX_OTHERS, OTHER_QOS_LEVEL_CNT, true);
    EXPECT_EQ(g_settedTestParam.id, g_qosId);
    ICB_AddParamCbk(g_qosId, CM_ICB_STATE_IOG_CREATED);
    EXPECT_EQ(g_paramCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[0].state, QOSM_PARAM_SETTED);
    EXPECT_EQ(g_paramCbk[0].result, 0);

    // add connection: set label first
    QOSM_AddConnection(g_qosId);
    EXPECT_EQ(g_labelParam.id, g_qosId);
    EXPECT_EQ(g_labelParam.icbCnt, 2);
    EXPECT_EQ(g_labelIcb[0].lcid, g_lcid1);
    EXPECT_EQ(g_labelIcb[0].connHandle, g_connHandle1);
    EXPECT_EQ(g_labelIcb[1].lcid, g_lcid2);
    EXPECT_EQ(g_labelIcb[1].connHandle, g_connHandle2);
    ICB_LabelReportCbk(g_qosId, g_lcid1, g_connHandle1, false); // will add timer to retry set g_connHandle1 label
    ICB_LabelReportCbk(g_qosId, g_lcid2, g_connHandle2, false);

    // retry set label g_connHandle1, still fail
    EXPECT_NE(g_timerParam[RETRY_CONN_TIMER_ID].callback, nullptr);
    g_timerParam[RETRY_CONN_TIMER_ID].callback(g_timerParam[RETRY_CONN_TIMER_ID].args);
    EXPECT_EQ(g_labelParam.id, g_qosId);
    EXPECT_EQ(g_labelParam.icbCnt, 1);
    EXPECT_EQ(g_labelIcb[0].lcid, g_lcid1);
    EXPECT_EQ(g_labelIcb[0].connHandle, g_connHandle1);
    ICB_LabelReportCbk(g_qosId, g_lcid1, g_connHandle1, false); // will add timer to retry set g_connHandle2 label

    // retry set label g_connHandle2, still fail
    EXPECT_NE(g_timerParam[RETRY_CONN_TIMER_ID].callback, nullptr);
    g_timerParam[RETRY_CONN_TIMER_ID].callback(g_timerParam[RETRY_CONN_TIMER_ID].args);
    EXPECT_EQ(g_labelParam.id, g_qosId);
    EXPECT_EQ(g_labelParam.icbCnt, 1);
    EXPECT_EQ(g_labelIcb[0].lcid, g_lcid2);
    EXPECT_EQ(g_labelIcb[0].connHandle, g_connHandle2);
    ICB_LabelReportCbk(g_qosId, g_lcid2, g_connHandle2, false);

    EXPECT_EQ(g_connParam[0].qosId, g_qosId);
    EXPECT_EQ(g_connParam[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_NE(g_connParam[0].result, 0);
    EXPECT_EQ(g_connParam[0].linkCnt, 2);
    EXPECT_EQ(g_connParamLink[0].lcid, 0);
    EXPECT_EQ(g_connParamLink[0].connHandle, g_connHandle1);
    EXPECT_EQ(g_connParamLink[1].lcid, 0);
    EXPECT_EQ(g_connParamLink[1].connHandle, g_connHandle2);

    // all disconnect, will remove param
    EXPECT_EQ(g_removedParam.id, g_qosId);
    ICB_RemoveParamCbk(g_qosId, CM_ICB_STATE_IOG_REMOVED);
    EXPECT_EQ(g_paramCbk[1].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[1].state, QOSM_PARAM_REMOVED);
    EXPECT_EQ(g_paramCbk[1].result, 0);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseSetLabelOneFailAnotherSuccess)
{
    QOSM_ICGMgrSetOtherParam(g_qosId, QOSM_QOSINDEX_OTHERS, OTHER_QOS_LEVEL_CNT, true);
    EXPECT_EQ(g_settedTestParam.id, g_qosId);
    ICB_AddParamCbk(g_qosId, CM_ICB_STATE_IOG_CREATED);
    EXPECT_EQ(g_paramCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[0].state, QOSM_PARAM_SETTED);
    EXPECT_EQ(g_paramCbk[0].result, 0);

    // add connection: g_connHandle1 set label success, and g_lcid2 disconnect
    QOSM_AddConnection(g_qosId);
    EXPECT_EQ(g_labelParam.id, g_qosId);
    EXPECT_EQ(g_labelParam.icbCnt, 2);
    EXPECT_EQ(g_labelIcb[0].lcid, g_lcid1);
    EXPECT_EQ(g_labelIcb[0].connHandle, g_connHandle1);
    EXPECT_EQ(g_labelIcb[1].lcid, g_lcid2);
    EXPECT_EQ(g_labelIcb[1].connHandle, g_connHandle2);
    ICB_LabelReportCbk(g_qosId, g_lcid1, g_connHandle1, true);
    
    // 收到链路2的异步链路断开事件，此时链路2上报建链失败
    ICB_DisconnectCbk(g_qosId, g_lcid2, g_connHandle2, CM_LINK_ACB);
    EXPECT_EQ(g_connParam[0].qosId, g_qosId);
    EXPECT_EQ(g_connParam[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_NE(g_connParam[0].result, 0);
    EXPECT_EQ(g_connParam[0].linkCnt, 1);
    EXPECT_EQ(g_connParamLink[0].connHandle, g_connHandle2);

    // 然后继续建立链路1的ICB连接
    EXPECT_EQ(g_createParam.id, g_qosId);
    EXPECT_EQ(g_createParam.channelCnt, 1);
    EXPECT_EQ(g_createChannel[0].lcid, g_lcid1);
    EXPECT_EQ(g_createChannel[0].connHandle, g_connHandle1);
    ICB_EstablishedCbk(g_qosId, g_lcid1, g_connHandle1, true);
    EXPECT_EQ(g_connParam[0].qosId, g_qosId);
    EXPECT_EQ(g_connParam[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_EQ(g_connParam[0].result, 0);
    EXPECT_EQ(g_connParam[0].linkCnt, 1);
    EXPECT_EQ(g_connParamLink[0].lcid, g_lcid1);
    EXPECT_EQ(g_connParamLink[0].connHandle, g_connHandle1);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseAddConnectionSuccess)
{
    QOSM_AutoRateConnParam param = {};
    param.qosId = g_qosId;
    param.linkCnt = 1;
    QOSM_ConnParam link[1] = {};
    link[0].lcid = g_lcid2;
    link[0].connHandle = g_connHandle2;
    param.link = link;
    QOSM_ICGMgrAddConnection(&param);

    EXPECT_EQ(g_labelParam.id, g_qosId);
    EXPECT_EQ(g_labelParam.icbCnt, 1);
    EXPECT_EQ(g_labelIcb[0].lcid, g_lcid2);
    EXPECT_EQ(g_labelIcb[0].connHandle, g_connHandle2);
    ICB_LabelReportCbk(g_qosId, g_lcid2, g_connHandle2, true);
    EXPECT_EQ(g_createParam.id, g_qosId);
    EXPECT_EQ(g_createParam.channelCnt, 1);
    EXPECT_EQ(g_createChannel[0].lcid, g_lcid2);
    EXPECT_EQ(g_createChannel[0].connHandle, g_connHandle2);
    ICB_EstablishedCbk(g_qosId, g_lcid2, g_connHandle2, true);
    EXPECT_EQ(g_connParam[0].qosId, g_qosId);
    EXPECT_EQ(g_connParam[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_EQ(g_connParam[0].result, 0);
    EXPECT_EQ(g_connParam[0].linkCnt, 1);
    EXPECT_EQ(g_connParamLink[0].lcid, g_lcid2);
    EXPECT_EQ(g_connParamLink[0].connHandle, g_connHandle2);

    // add connection repeatly, will callback success directly
    QOSM_AddConnection(g_qosId);
    EXPECT_EQ(g_connParam[0].qosId, g_qosId);
    EXPECT_EQ(g_connParam[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_EQ(g_connParam[0].result, 0);
    EXPECT_EQ(g_connParam[0].linkCnt, 2);
    EXPECT_EQ(g_connParamLink[0].lcid, g_lcid1);
    EXPECT_EQ(g_connParamLink[0].connHandle, g_connHandle1);
    EXPECT_EQ(g_connParamLink[1].lcid, g_lcid2);
    EXPECT_EQ(g_connParamLink[1].connHandle, g_connHandle2);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseQualityReportInvalid)
{
    // ignore quality event when tx flushed and ack rate are 0
    QOSM_HookLog();
    ICB_QualityCbk(g_connHandle1, 0, -50, 0, 0);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "ignore quality event when tx flushed and ack rate are 0"), nullptr);

    // ignore quality event when cbk is null or autorate disable
    QOSM_AutoRateCallback callback = {};
    callback.paramChangedCbk = QOSM_ParamChangedCbk;
    callback.connChangedCbk = QOSM_ConnectionChangedCbk;
    callback.dataPathChangedCbk = QOSM_DataPathChangedCbk;
    callback.bitrateChangedCbk = nullptr;
    callback.frequencyBandChangedCbk = QOSM_FrequencyBandChangedCbk;
    callback.highPowerModeChangedCbk = QOSM_HighPowerModeChangedCbk;
    uint32_t ret = QOSM_ICGMgrRegisterCallback(&callback);
    EXPECT_EQ(ret, QOSM_SUCCESS);
    EXPECT_NE(g_icbCbk.connectionCbk, nullptr);

    ICB_QualityCbk(g_connHandle1, 100, -50, 0, 0);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "ignore quality event when cbk is null or autorate disable"), nullptr);
    QOSM_UnhookLog();

    // restore callback
    callback.bitrateChangedCbk = QOSM_BitrateChangedCbk;
    ret = QOSM_ICGMgrRegisterCallback(&callback);
    EXPECT_EQ(ret, QOSM_SUCCESS);
    EXPECT_NE(g_icbCbk.connectionCbk, nullptr);
}

// 1、降档到192，然后升档到320，此时还没有收到升档成功事件
// 2、链路A3被耳机断开，然后再建立链路A3（延迟任务）
// 3、收到升档成功事件，然后链路A4被耳机断开
// 预期：链路A4被断开时，应建立链路A3（执行延迟任务）
TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseDelayConnWhileLevelUping)
{
    EXPECT_NE(g_audioDfxInfo.dspStatusCb, nullptr);
    g_audioDfxInfo.dspStatusCb(true);

    // 当前处于100%占空比320kbps，降档到192kbps
    ICB_QualityCbk(g_connHandle1, 100, -50, 1, 0);
    ICB_QualityCbk(g_connHandle2, 10, -50, 0, 0);
    EXPECT_EQ(g_bitrateParam[0].qosId, g_qosId);
    EXPECT_EQ(g_bitrateParam[0].direction, LEVEL_DOWN);
    EXPECT_EQ(g_bitrateParam[0].downwardBitrate, 96);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_bitrateParam[0].labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_bitrateParam[0].labelId);

    // 升档到320kbps
    for (uint8_t i = 0; i < g_levelUpAckrateOverCnt; i++) {
        ICB_QualityCbk(g_connHandle1, 90, -50, 0, 0);
        ICB_QualityCbk(g_connHandle2, 90, -50, 0, 0);
    }
    EXPECT_EQ(g_updateParam.id, g_qosId);
    EXPECT_EQ(g_updateParam.icbCnt, 2);
    EXPECT_EQ(g_updateChannel[0], g_connHandle1);
    EXPECT_EQ(g_updateChannel[1], g_connHandle2);

    // 收到链路1被断开事件
    ICB_DisconnectCbk(g_qosId, g_lcid1, g_connHandle1, CM_LINK_ICB);

    // 建立链路1，创建延迟建链任务
    QOSM_AutoRateConnParam param = {};
    param.qosId = g_qosId;
    param.linkCnt = 1;
    QOSM_ConnParam link[1] = {};
    link[0].lcid = g_lcid1;
    link[0].connHandle = g_connHandle1;
    param.link = link;
    QOSM_ICGMgrAddConnection(&param);

    // 收到升档成功事件
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_updateParam.labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_updateParam.labelId);

    // 收到链路2被断开事件，此时会执行链路1的延迟建链任务
    ICB_DisconnectCbk(g_qosId, g_lcid2, g_connHandle2, CM_LINK_ICB);
    EXPECT_EQ(g_labelParam.id, g_qosId);
    EXPECT_EQ(g_labelParam.icbCnt, 1);
    EXPECT_EQ(g_labelIcb[0].lcid, g_lcid1);
    EXPECT_EQ(g_labelIcb[0].connHandle, g_connHandle1);
    ICB_LabelReportCbk(g_qosId, g_lcid1, g_connHandle1, true);
    EXPECT_EQ(g_createParam.id, g_qosId);
    EXPECT_EQ(g_createParam.channelCnt, 1);
    EXPECT_EQ(g_createChannel[0].lcid, g_lcid1);
    EXPECT_EQ(g_createChannel[0].connHandle, g_connHandle1);
    ICB_EstablishedCbk(g_qosId, g_lcid1, g_connHandle1, true);
    EXPECT_EQ(g_connParam[0].qosId, g_qosId);
    EXPECT_EQ(g_connParam[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_EQ(g_connParam[0].result, 0);
    EXPECT_EQ(g_connParam[0].linkCnt, 1);
    EXPECT_EQ(g_connParamLink[0].lcid, g_lcid1);
    EXPECT_EQ(g_connParamLink[0].connHandle, g_connHandle1);
    // 升档成功以后，等待2个周期上报，才回调通知调用方码率升档
    EXPECT_NE(g_timerParam[UPGRADE_LABEL_TIMER_ID].callback, nullptr);
    g_timerParam[UPGRADE_LABEL_TIMER_ID].callback(g_timerParam[UPGRADE_LABEL_TIMER_ID].args);
    EXPECT_EQ(g_bitrateParam[1].qosId, g_qosId);
    EXPECT_EQ(g_bitrateParam[1].direction, LEVEL_UP);
    EXPECT_EQ(g_bitrateParam[1].downwardBitrate, 160);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);

    // 建立链路2
    link[0].lcid = g_lcid2;
    link[0].connHandle = g_connHandle2;
    QOSM_ICGMgrAddConnection(&param);
    EXPECT_EQ(g_labelParam.id, g_qosId);
    EXPECT_EQ(g_labelParam.icbCnt, 1);
    EXPECT_EQ(g_labelIcb[0].lcid, g_lcid2);
    EXPECT_EQ(g_labelIcb[0].connHandle, g_connHandle2);
    ICB_LabelReportCbk(g_qosId, g_lcid2, g_connHandle2, true);
    EXPECT_EQ(g_createParam.id, g_qosId);
    EXPECT_EQ(g_createParam.channelCnt, 1);
    EXPECT_EQ(g_createChannel[0].lcid, g_lcid2);
    EXPECT_EQ(g_createChannel[0].connHandle, g_connHandle2);
    ICB_EstablishedCbk(g_qosId, g_lcid2, g_connHandle2, true);
    EXPECT_EQ(g_connParam[0].qosId, g_qosId);
    EXPECT_EQ(g_connParam[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_EQ(g_connParam[0].result, 0);
    EXPECT_EQ(g_connParam[0].linkCnt, 1);
    EXPECT_EQ(g_connParamLink[0].lcid, g_lcid2);
    EXPECT_EQ(g_connParamLink[0].connHandle, g_connHandle2);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseFreqBandChanged)
{
    // freq band change from 2.4G to 5.1G
    EXPECT_NE(g_freqBandListener, nullptr);
    CM_FreqBandSwitchParam param = {};
    param.status = 0;
    param.connHandle = g_connHandle1;
    param.oldFreqBand = 0;
    param.newFreqBand = 1;
    g_freqBandListener(&param);
    EXPECT_EQ(g_freqBand, 1);

    // freq band change from 5.1G to 2.4G
    param.oldFreqBand = 1;
    param.newFreqBand = 0;
    g_freqBandListener(&param);
    EXPECT_EQ(g_freqBand, 0);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCasePowerLevelChanged)
{
    uint8_t level = 8;
    EXPECT_NE(g_powerLevelListner, nullptr);
    struct QOSM_PowerLevelInfo info = {};
    info.level = level;
    NLSTK_CfgdbChanInfo_S chanInfo;
    chanInfo.data = (void *)&info;
    chanInfo.len = sizeof(info);
    g_powerLevelListner(&chanInfo);
    EXPECT_EQ(g_powerLevel, level);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseEstablishNotExistQosId)
{
    uint8_t qosId = 2;
    ICB_EstablishedCbk(qosId, g_lcid1, g_connHandle1, true);
    EXPECT_EQ(g_connParam[0].qosId, qosId);
    EXPECT_EQ(g_connParam[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_NE(g_connParam[0].result, 0);
    EXPECT_EQ(g_connParam[0].linkCnt, 1);
    EXPECT_EQ(g_connParamLink[0].lcid, 0);
    EXPECT_EQ(g_connParamLink[0].connHandle, g_connHandle1);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseSetParamAfterConnected)
{
    g_isDelaySetParam = true;

    // set same param, CP_TimerAdd failed, errno 1, will callback fail
    QOSM_HookLog();
    g_cpTimerRet = 1;
    QOSM_ICGMgrSetOtherParam(g_qosId, QOSM_QOSINDEX_OTHERS, OTHER_QOS_LEVEL_CNT, true);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "CP_TimerAdd failed, errno 1"), nullptr);
    QOSM_UnhookLog();
    EXPECT_EQ(g_paramCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[0].state, QOSM_PARAM_SETTED);
    EXPECT_NE(g_paramCbk[0].result, 0);

    // set same param, will disconnect and remove param and then delay set param, no disconnect event
    g_cpTimerRet = 0;
    QOSM_ICGMgrSetOtherParam(g_qosId, QOSM_QOSINDEX_OTHERS, OTHER_QOS_LEVEL_CNT, true);
    EXPECT_EQ(g_disconnectParam.id, g_qosId);
    EXPECT_EQ(g_disconnectParam.channelCnt, 1);
    EXPECT_EQ(g_disconnectChannel[0].connHandle, g_connHandle2);
    EXPECT_NE(g_timerParam[DELAY_PARAM_TIMER_ID].callback, nullptr);

    // not receive disconnect event, trigger delay set param timeout,
    // will remove param directly whether connHandle is connected or not and then set param
    g_timerParam[DELAY_PARAM_TIMER_ID].callback(g_timerParam[DELAY_PARAM_TIMER_ID].args);
    EXPECT_EQ(g_removedParam.id, g_qosId);
    ICB_RemoveParamCbk(g_qosId, CM_ICB_STATE_IOG_REMOVED);
    EXPECT_EQ(g_paramCbk[1].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[1].state, QOSM_PARAM_REMOVED);
    EXPECT_EQ(g_paramCbk[1].result, 0);
    EXPECT_EQ(g_settedTestParam.id, g_qosId);
    ICB_AddParamCbk(g_qosId, CM_ICB_STATE_IOG_CREATED);
    EXPECT_EQ(g_paramCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[0].state, QOSM_PARAM_SETTED);
    EXPECT_EQ(g_paramCbk[0].result, 0);

    // set different param, will disconnect and remove param and then delay set param
    QOSM_ICGMgrSetAudioTestParam(g_qosId, AUDIO_QOS_LEVEL_CNT);
    EXPECT_EQ(g_disconnectParam.id, g_qosId);
    EXPECT_EQ(g_disconnectParam.channelCnt, 1);
    EXPECT_EQ(g_disconnectChannel[0].connHandle, g_connHandle2);
    EXPECT_NE(g_timerParam[DELAY_PARAM_TIMER_ID].callback, nullptr);
    ICB_DisconnectCbk(g_qosId, g_lcid2, g_connHandle2, CM_LINK_ICB);
    EXPECT_EQ(g_connParam[1].qosId, g_qosId);
    EXPECT_EQ(g_connParam[1].state, QOSM_CONNECTION_DELETED);
    EXPECT_EQ(g_connParam[1].result, 0);
    EXPECT_EQ(g_connParam[1].linkCnt, 1);
    EXPECT_EQ(g_connParamLink[2].lcid, g_lcid2);
    EXPECT_EQ(g_connParamLink[2].connHandle, g_connHandle2);

    EXPECT_EQ(g_removedParam.id, g_qosId);
    ICB_RemoveParamCbk(g_qosId, CM_ICB_STATE_IOG_REMOVED);
    EXPECT_EQ(g_paramCbk[1].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[1].state, QOSM_PARAM_REMOVED);
    EXPECT_EQ(g_paramCbk[1].result, 0);
    EXPECT_EQ(g_settedTestParam.id, g_qosId);
    ICB_AddParamCbk(g_qosId, CM_ICB_STATE_IOG_CREATED);
    EXPECT_EQ(g_paramCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[0].state, QOSM_PARAM_SETTED);
    EXPECT_EQ(g_paramCbk[0].result, 0);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseAddDataPath)
{
    QOSM_AddConnection(g_qosId);
    EXPECT_EQ(g_labelParam.id, g_qosId);
    EXPECT_EQ(g_labelParam.icbCnt, 2);
    EXPECT_EQ(g_labelIcb[0].lcid, g_lcid1);
    EXPECT_EQ(g_labelIcb[0].connHandle, g_connHandle1);
    EXPECT_EQ(g_labelIcb[1].lcid, g_lcid2);
    EXPECT_EQ(g_labelIcb[1].connHandle, g_connHandle2);
    ICB_LabelReportCbk(g_qosId, g_lcid1, g_connHandle1, true);
    ICB_LabelReportCbk(g_qosId, g_lcid2, g_connHandle2, true);
    EXPECT_EQ(g_createParam.id, g_qosId);
    EXPECT_EQ(g_createParam.channelCnt, 2);
    EXPECT_EQ(g_createChannel[0].lcid, g_lcid1);
    EXPECT_EQ(g_createChannel[0].connHandle, g_connHandle1);
    EXPECT_EQ(g_createChannel[1].lcid, g_lcid2);
    EXPECT_EQ(g_createChannel[1].connHandle, g_connHandle2);
    ICB_EstablishedCbk(g_qosId, g_lcid1, g_connHandle1, true);
    ICB_EstablishedCbk(g_qosId, g_lcid2, g_connHandle2, true);
    EXPECT_EQ(g_connParam[0].qosId, g_qosId);
    EXPECT_EQ(g_connParam[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_EQ(g_connParam[0].result, 0);
    EXPECT_EQ(g_connParam[0].linkCnt, 2);
    EXPECT_EQ(g_connParamLink[0].lcid, g_lcid1);
    EXPECT_EQ(g_connParamLink[0].connHandle, g_connHandle1);
    EXPECT_EQ(g_connParamLink[1].lcid, g_lcid2);
    EXPECT_EQ(g_connParamLink[1].connHandle, g_connHandle2);

    QOSM_HookLog();
    uint8_t direction = 1;
    QOSM_AutoRateDataPath dataPath = {};
    dataPath.qosId = g_qosId;
    dataPath.connHandle = g_connHandle1;
    dataPath.direction = direction;
    dataPath.pathId = QOSM_PATHID_DSP;
    // codec config data is null
    dataPath.codecConfigLen = 1;
    QOSM_ICGMgrAddDataPath(&dataPath);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "codec config data is null"), nullptr);
    // icgInfo is NULL
    dataPath.codecConfigLen = 0;
    dataPath.qosId = 100;
    QOSM_ICGMgrAddDataPath(&dataPath);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "icgInfo is NULL"), nullptr);
    // CM_ICBSetupDataPath failed
    dataPath.qosId = g_qosId;
    g_cmSetDataPathRet = CM_FAIL;
    QOSM_ICGMgrAddDataPath(&dataPath);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "CM_ICBSetupDataPath failed"), nullptr);
    QOSM_UnhookLog();

    g_cmSetDataPathRet = CM_SUCCESS;
    QOSM_ICGMgrAddDataPath(&dataPath);
    dataPath.connHandle = g_connHandle2;
    QOSM_ICGMgrAddDataPath(&dataPath);
    ICB_DataPathAddedCbk(g_qosId, g_lcid1, g_connHandle1, direction);
    EXPECT_EQ(g_dataPath[0].qosId, g_qosId);
    EXPECT_EQ(g_dataPath[0].state, QOSM_DATAPATH_ADDED);
    EXPECT_EQ(g_dataPath[0].result, 0);
    EXPECT_EQ(g_dataPath[0].connHandle, g_connHandle1);
    EXPECT_EQ(g_dataPath[0].direction, direction);
    ICB_DataPathAddedCbk(g_qosId, g_lcid2, g_connHandle2, direction);
    EXPECT_EQ(g_dataPath[0].qosId, g_qosId);
    EXPECT_EQ(g_dataPath[0].state, QOSM_DATAPATH_ADDED);
    EXPECT_EQ(g_dataPath[0].result, 0);
    EXPECT_EQ(g_dataPath[0].connHandle, g_connHandle2);
    EXPECT_EQ(g_dataPath[0].direction, direction);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseReceiveLevelDownEventUnexpectly)
{
    EXPECT_NE(g_audioDfxInfo.dspStatusCb, nullptr);
    g_audioDfxInfo.dspStatusCb(true);

    // 当前处于100%占空比320kbps，意外收到降档到192kbps的事件通知，此时会重新升档回320kbps
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, 6);
    // 重新升档回320kbps
    EXPECT_EQ(g_updateParam.id, g_qosId);
    EXPECT_EQ(g_updateParam.icbCnt, 2);
    EXPECT_EQ(g_updateChannel[0], g_connHandle1);
    EXPECT_EQ(g_updateChannel[1], g_connHandle2);
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_updateParam.labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_updateParam.labelId);
    // 升档成功以后，等待2个周期上报，码率没有变化，所以调用方不会收到回调
    EXPECT_NE(g_timerParam[UPGRADE_LABEL_TIMER_ID].callback, nullptr);
    g_timerParam[UPGRADE_LABEL_TIMER_ID].callback(g_timerParam[UPGRADE_LABEL_TIMER_ID].args);
    EXPECT_EQ(g_bitrateParam[1].qosId, 0);
    EXPECT_EQ(g_bitrateParam[1].direction, 0);
    EXPECT_EQ(g_bitrateParam[1].downwardBitrate, 0);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseNotifyCoexistChanged)
{
    EXPECT_NE(g_audioDfxInfo.dspStatusCb, nullptr);
    g_audioDfxInfo.dspStatusCb(true);

    // 当前处于100%占空比320kbps，共存模块通知最高码率为640，占空比为50%，触发占空比切换
    QOSM_ICGMgrNotifyParam notifyParam = {};
    notifyParam.type = NOTIFY_TYPE_COEXIST;
    notifyParam.maxBitrate = 640;
    notifyParam.dutyCycle = QOS_DUTY_CYCLE_50P;
    // CP_TimerAdd for qos id 1, errno 1
    QOSM_HookLog();
    g_cpTimerRet = 1;
    QOSM_ICGMgrParamNotify(&notifyParam);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "CP_TimerAdd for qos id 1, errno 1"), nullptr);
    EXPECT_EQ(g_bitrateParam[0].qosId, g_qosId);
    EXPECT_EQ(g_bitrateParam[0].direction, LEVEL_DOWN);
    EXPECT_EQ(g_bitrateParam[0].downwardBitrate, 160);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_50P);
    QOSM_UnhookLog();

    // add connection while updating will delay add connection
    g_cpTimerRet = 0;
    QOSM_AddConnection(g_qosId);
    EXPECT_NE(g_timerParam[DELAY_CONN_TIMER_ID].callback, nullptr);
    // and because already connected will callback success directly
    g_timerParam[DELAY_CONN_TIMER_ID].callback(g_timerParam[DELAY_CONN_TIMER_ID].args);
    EXPECT_EQ(g_connParam[0].qosId, g_qosId);
    EXPECT_EQ(g_connParam[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_EQ(g_connParam[0].result, 0);
    EXPECT_EQ(g_connParam[0].linkCnt, 2);
    EXPECT_EQ(g_connParamLink[0].lcid, g_lcid1);
    EXPECT_EQ(g_connParamLink[0].connHandle, g_connHandle1);
    EXPECT_EQ(g_connParamLink[1].lcid, g_lcid2);
    EXPECT_EQ(g_connParamLink[1].connHandle, g_connHandle2);

    // add connection while updating will delay add connection
    QOSM_AddConnection(g_qosId);
    EXPECT_NE(g_timerParam[DELAY_CONN_TIMER_ID].callback, nullptr);
    // add connection while updating repeatly will notify failed
    QOSM_HookLog();
    QOSM_AddConnection(g_qosId);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "already has delay connection task"), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "delay connection task failed"), nullptr);
    QOSM_UnhookLog();
    // and delete connection at this time will destroy the delay add connection task
    QOSM_AutoRateConnParam connParam = {};
    connParam.qosId = g_qosId;
    connParam.linkCnt = 2;
    QOSM_ConnParam link[2] = {};
    link[0].lcid = g_lcid1;
    link[0].connHandle = g_connHandle1;
    link[1].lcid = g_lcid2;
    link[1].connHandle = g_connHandle2;
    connParam.link = link;
    QOSM_ICGMgrDeleteConnection(&connParam);
    EXPECT_EQ(g_timerParam[DELAY_CONN_TIMER_ID].callback, nullptr);

    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_bitrateParam[0].labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_bitrateParam[0].labelId);

    // 当前处于50%占空比320kbps，共存模块通知最高码率为4600，占空比为100%，触发占空比切换
    notifyParam.maxBitrate = 4600;
    notifyParam.dutyCycle = QOS_DUTY_CYCLE_100P;
    QOSM_ICGMgrParamNotify(&notifyParam);
    EXPECT_EQ(g_bitrateParam[0].qosId, g_qosId);
    EXPECT_EQ(g_bitrateParam[0].direction, LEVEL_DOWN);
    EXPECT_EQ(g_bitrateParam[0].downwardBitrate, 160);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_bitrateParam[0].labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_bitrateParam[0].labelId);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseLevelUpTo640kbpsSuccess)
{
    EXPECT_NE(g_audioDfxInfo.dspStatusCb, nullptr);
    g_audioDfxInfo.dspStatusCb(true);

    // 当前处于100%占空比320kbps，升档到640kbps
    for (uint8_t i = 0; i < g_levelUpAckrateOverCnt; i++) {
        ICB_QualityCbk(g_connHandle1, 90, -50, 0, 0);
        ICB_QualityCbk(g_connHandle2, 90, -50, 0, 0);
    }
    EXPECT_EQ(g_updateParam.id, g_qosId);
    EXPECT_EQ(g_updateParam.icbCnt, 2);
    EXPECT_EQ(g_updateChannel[0], g_connHandle1);
    EXPECT_EQ(g_updateChannel[1], g_connHandle2);

    // 升档超时
    EXPECT_NE(g_timerParam[UPGRADE_LABEL_TIMER_ID].callback, nullptr);
    g_timerParam[UPGRADE_LABEL_TIMER_ID].callback(g_timerParam[UPGRADE_LABEL_TIMER_ID].args);
    EXPECT_EQ(g_bitrateParam[1].qosId, 0);
    EXPECT_EQ(g_bitrateParam[1].direction, 0);
    EXPECT_EQ(g_bitrateParam[1].downwardBitrate, 0);

    // 重新触发升档
    for (uint8_t i = 0; i < g_levelUpAckrateOverCnt; i++) {
        ICB_QualityCbk(g_connHandle1, 90, -50, 0, 0);
        ICB_QualityCbk(g_connHandle2, 90, -50, 0, 0);
    }
    EXPECT_EQ(g_updateParam.id, g_qosId);
    EXPECT_EQ(g_updateParam.icbCnt, 2);
    EXPECT_EQ(g_updateChannel[0], g_connHandle1);
    EXPECT_EQ(g_updateChannel[1], g_connHandle2);
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_updateParam.labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_updateParam.labelId);
    // 升档成功以后，等待2个周期上报，才回调通知调用方码率升档
    EXPECT_NE(g_timerParam[UPGRADE_LABEL_TIMER_ID].callback, nullptr);
    g_timerParam[UPGRADE_LABEL_TIMER_ID].callback(g_timerParam[UPGRADE_LABEL_TIMER_ID].args);
    EXPECT_EQ(g_bitrateParam[1].qosId, g_qosId);
    EXPECT_EQ(g_bitrateParam[1].direction, LEVEL_UP);
    EXPECT_EQ(g_bitrateParam[1].downwardBitrate, 320);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseLevelUpTo1500kbpsFail)
{
    EXPECT_NE(g_audioDfxInfo.dspStatusCb, nullptr);
    g_audioDfxInfo.dspStatusCb(true);

    // 当前处于100%占空比640kbps，升档到1500kbps
    for (uint8_t i = 0; i < g_levelUpAckrateOverCnt; i++) {
        ICB_QualityCbk(g_connHandle1, 90, -50, 0, 0);
        ICB_QualityCbk(g_connHandle2, 90, -50, 0, 0);
    }
    EXPECT_EQ(g_updateParam.id, g_qosId);
    EXPECT_EQ(g_updateParam.icbCnt, 2);
    EXPECT_EQ(g_updateChannel[0], g_connHandle1);
    EXPECT_EQ(g_updateChannel[1], g_connHandle2);
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_updateParam.labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_updateParam.labelId);
    // 升档成功以后，等待2个周期上报，但是触发降档，从1500kbps降档回到640kbps，无需通知调用方
    ICB_QualityCbk(g_connHandle1, 10, -50, 0, 0);
    ICB_QualityCbk(g_connHandle2, 10, -50, 0, 0);
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_updateParam.labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_updateParam.labelId);
    EXPECT_EQ(g_bitrateParam[0].qosId, 0);
    EXPECT_EQ(g_bitrateParam[0].direction, 0);
    EXPECT_EQ(g_bitrateParam[0].downwardBitrate, 0);
    EXPECT_EQ(g_bitrateParam[1].qosId, 0);
    EXPECT_EQ(g_bitrateParam[1].direction, 0);
    EXPECT_EQ(g_bitrateParam[1].downwardBitrate, 0);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseLevelUpTo2300kbpsFail)
{
    EXPECT_NE(g_audioDfxInfo.dspStatusCb, nullptr);
    g_audioDfxInfo.dspStatusCb(true);

    // 当前处于100%占空比640kbps，升档到1500kbps
    for (uint8_t i = 0; i < g_levelUpAckrateOverCnt; i++) {
        ICB_QualityCbk(g_connHandle1, 90, -50, 0, 0);
        ICB_QualityCbk(g_connHandle2, 90, -50, 0, 0);
    }
    EXPECT_EQ(g_updateParam.id, g_qosId);
    EXPECT_EQ(g_updateParam.icbCnt, 2);
    EXPECT_EQ(g_updateChannel[0], g_connHandle1);
    EXPECT_EQ(g_updateChannel[1], g_connHandle2);
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_updateParam.labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_updateParam.labelId);
    // 升档成功以后，等待2个周期上报，才回调通知调用方码率升档
    EXPECT_NE(g_timerParam[UPGRADE_LABEL_TIMER_ID].callback, nullptr);
    g_timerParam[UPGRADE_LABEL_TIMER_ID].callback(g_timerParam[UPGRADE_LABEL_TIMER_ID].args);
    EXPECT_EQ(g_bitrateParam[1].qosId, g_qosId);
    EXPECT_EQ(g_bitrateParam[1].direction, LEVEL_UP);
    EXPECT_EQ(g_bitrateParam[1].downwardBitrate, 750);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);

    // 当前处于100%占空比1500kbps，升档到2300kbps
    for (uint8_t i = 0; i < g_levelUpAckrateOverCnt; i++) {
        ICB_QualityCbk(g_connHandle1, 90, -50, 0, 0);
        ICB_QualityCbk(g_connHandle2, 90, -50, 0, 0);
    }
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_updateParam.labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_updateParam.labelId);
    // 升档成功以后，等待2个周期上报，但是触发降档，从2300降档到640，需通知调用方
    ICB_QualityCbk(g_connHandle1, 10, -50, 0, 0);
    ICB_QualityCbk(g_connHandle2, 10, -50, 0, 0);
    EXPECT_EQ(g_bitrateParam[0].qosId, g_qosId);
    EXPECT_EQ(g_bitrateParam[0].direction, LEVEL_DOWN);
    EXPECT_EQ(g_bitrateParam[0].downwardBitrate, 320);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_bitrateParam[0].labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_bitrateParam[0].labelId);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseLevelUpTo4600kbpsSuccessAndFlowControll)
{
    EXPECT_NE(g_audioDfxInfo.dspStatusCb, nullptr);
    g_audioDfxInfo.dspStatusCb(true);

    // 当前处于100%占空比640kbps，升档到1500kbps
    for (uint8_t i = 0; i < g_levelUpAckrateOverCnt; i++) {
        ICB_QualityCbk(g_connHandle1, 90, -50, 0, 0);
        ICB_QualityCbk(g_connHandle2, 90, -50, 0, 0);
    }
    EXPECT_EQ(g_updateParam.id, g_qosId);
    EXPECT_EQ(g_updateParam.icbCnt, 2);
    EXPECT_EQ(g_updateChannel[0], g_connHandle1);
    EXPECT_EQ(g_updateChannel[1], g_connHandle2);
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_updateParam.labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_updateParam.labelId);
    // 升档成功以后，等待2个周期上报，才回调通知调用方码率升档
    EXPECT_NE(g_timerParam[UPGRADE_LABEL_TIMER_ID].callback, nullptr);
    g_timerParam[UPGRADE_LABEL_TIMER_ID].callback(g_timerParam[UPGRADE_LABEL_TIMER_ID].args);
    EXPECT_EQ(g_bitrateParam[1].qosId, g_qosId);
    EXPECT_EQ(g_bitrateParam[1].direction, LEVEL_UP);
    EXPECT_EQ(g_bitrateParam[1].downwardBitrate, 750);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);

    // 当前处于100%占空比1500kbps，升档到2300kbps
    for (uint8_t i = 0; i < g_levelUpAckrateOverCnt; i++) {
        ICB_QualityCbk(g_connHandle1, 90, -50, 0, 0);
        ICB_QualityCbk(g_connHandle2, 90, -50, 0, 0);
    }
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_updateParam.labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_updateParam.labelId);
    // 升档成功以后，等待2个周期上报，才回调通知调用方码率升档
    EXPECT_NE(g_timerParam[UPGRADE_LABEL_TIMER_ID].callback, nullptr);
    g_timerParam[UPGRADE_LABEL_TIMER_ID].callback(g_timerParam[UPGRADE_LABEL_TIMER_ID].args);
    EXPECT_EQ(g_bitrateParam[1].qosId, g_qosId);
    EXPECT_EQ(g_bitrateParam[1].direction, LEVEL_UP);
    EXPECT_EQ(g_bitrateParam[1].downwardBitrate, 1150);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);

    // 当前处于100%占空比2300kbps，升档到4600kbps
    for (uint8_t i = 0; i < g_levelUpAckrateOverCnt; i++) {
        ICB_QualityCbk(g_connHandle1, 90, -50, 0, 0);
        ICB_QualityCbk(g_connHandle2, 90, -50, 0, 0);
    }
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_updateParam.labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_updateParam.labelId);
    // 升档成功以后，等待2个周期上报，才回调通知调用方码率升档
    EXPECT_NE(g_timerParam[UPGRADE_LABEL_TIMER_ID].callback, nullptr);
    g_timerParam[UPGRADE_LABEL_TIMER_ID].callback(g_timerParam[UPGRADE_LABEL_TIMER_ID].args);
    EXPECT_EQ(g_bitrateParam[1].qosId, g_qosId);
    EXPECT_EQ(g_bitrateParam[1].direction, LEVEL_UP);
    EXPECT_EQ(g_bitrateParam[1].downwardBitrate, 2300);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);

    // 4300kbps触发流控，降档到640kbps
    EXPECT_NE(g_audioDfxInfo.flowCtrlCb, nullptr);
    g_audioDfxInfo.flowCtrlCb(g_connHandle1, true);
    g_audioDfxInfo.flowCtrlCb(g_connHandle2, true);
    EXPECT_EQ(g_bitrateParam[0].qosId, g_qosId);
    EXPECT_EQ(g_bitrateParam[0].direction, LEVEL_DOWN);
    EXPECT_EQ(g_bitrateParam[0].downwardBitrate, 320);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_bitrateParam[0].labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_bitrateParam[0].labelId);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseLevelDownTo320kbpsTimeout)
{
    // 当前处于100%占空比640kbps，降档到320kbps
    ICB_QualityCbk(g_connHandle1, 10, -50, 0, 0);
    ICB_QualityCbk(g_connHandle2, 10, -50, 0, 0);
    EXPECT_EQ(g_bitrateParam[0].qosId, g_qosId);
    EXPECT_EQ(g_bitrateParam[0].direction, LEVEL_DOWN);
    EXPECT_EQ(g_bitrateParam[0].downwardBitrate, 160);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);
    
    // 降档超时，升档回640kbps
    EXPECT_NE(g_timerParam[DOWNGRADE_LABEL_TIMER_ID].callback, nullptr);
    g_timerParam[DOWNGRADE_LABEL_TIMER_ID].callback(g_timerParam[DOWNGRADE_LABEL_TIMER_ID].args);
    EXPECT_EQ(g_updateParam.id, g_qosId);
    EXPECT_EQ(g_updateParam.icbCnt, 2);
    EXPECT_EQ(g_updateChannel[0], g_connHandle1);
    EXPECT_EQ(g_updateChannel[1], g_connHandle2);
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_updateParam.labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_updateParam.labelId);
    // 升档成功以后，等待2个周期上报，码率没有变化，所以调用方不会收到回调
    EXPECT_NE(g_timerParam[UPGRADE_LABEL_TIMER_ID].callback, nullptr);
    g_timerParam[UPGRADE_LABEL_TIMER_ID].callback(g_timerParam[UPGRADE_LABEL_TIMER_ID].args);
    EXPECT_EQ(g_bitrateParam[1].qosId, 0);
    EXPECT_EQ(g_bitrateParam[1].direction, 0);
    EXPECT_EQ(g_bitrateParam[1].downwardBitrate, 0);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseNotifyEarphoneFeedback)
{
    // dsp关闭，当前处于100%占空比640kbps，耳机反馈仅192kbps、320kbps可用，缓存降码率到320kbps的任务
    EXPECT_NE(g_audioDfxInfo.dspStatusCb, nullptr);
    g_audioDfxInfo.dspStatusCb(false); // not support autorate while dsp is off
    QOSM_ICGMgrNotifyParam notifyParam = {};
    notifyParam.type = NOTIFY_TYPE_EARPHONE;
    notifyParam.supportedBitrateCnt = 2;
    notifyParam.supportedBitrate[0] = 96;
    notifyParam.supportedBitrate[1] = 160;
    QOSM_ICGMgrParamNotify(&notifyParam);

    // dsp打开以后，检查到有缓存的降码率任务，执行降码率到320kbps
    EXPECT_NE(g_audioDfxInfo.dspStatusCb, nullptr);
    g_audioDfxInfo.dspStatusCb(true); // check autorate while dsp is on
    EXPECT_EQ(g_bitrateParam[0].qosId, g_qosId);
    EXPECT_EQ(g_bitrateParam[0].direction, LEVEL_DOWN);
    EXPECT_EQ(g_bitrateParam[0].downwardBitrate, 160);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_bitrateParam[0].labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_bitrateParam[0].labelId);

    // 当前处于100%占空比320kbps，耳机反馈仅192kbps可用，触发降档到192kbps
    notifyParam.supportedBitrateCnt = 1;
    notifyParam.supportedBitrate[0] = 96;
    QOSM_ICGMgrParamNotify(&notifyParam);
    EXPECT_EQ(g_bitrateParam[0].qosId, g_qosId);
    EXPECT_EQ(g_bitrateParam[0].direction, LEVEL_DOWN);
    EXPECT_EQ(g_bitrateParam[0].downwardBitrate, 96);
    EXPECT_EQ(QOSM_GetDutyCycle(), QOS_DUTY_CYCLE_100P);
    ICB_UpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, g_bitrateParam[0].labelId);
    ICB_UpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, g_bitrateParam[0].labelId);

    // 当前处于100%占空比192kbps，耳机反馈仅96kbps可用，由于setTestParam时96kbps不可用，所以为非法bitrate
    QOSM_HookLog();
    notifyParam.supportedBitrateCnt = 1;
    notifyParam.supportedBitrate[0] = 48;
    QOSM_ICGMgrParamNotify(&notifyParam);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "bitrate is invalid"), nullptr);
    QOSM_UnhookLog();
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseDeleteDataPath)
{
    uint8_t direction = 1;
    QOSM_AutoRateDeletedDataPath deletedDataPath = {};
    deletedDataPath.qosId = g_qosId;
    deletedDataPath.connHandle = g_connHandle1;
    deletedDataPath.direction = direction;
    // icgInfo is NULL
    QOSM_HookLog();
    deletedDataPath.qosId = 100;
    QOSM_ICGMgrDeleteDataPath(&deletedDataPath);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "icgInfo is NULL"), nullptr);

    // CM_ICBRemoveDataPath failed
    deletedDataPath.qosId = g_qosId;
    g_cmRemoveDataPathRet = CM_FAIL;
    QOSM_ICGMgrDeleteDataPath(&deletedDataPath);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "CM_ICBRemoveDataPath failed"), nullptr);
    QOSM_UnhookLog();

    g_cmRemoveDataPathRet = CM_SUCCESS;
    QOSM_ICGMgrDeleteDataPath(&deletedDataPath);
    ICB_DataPathDeletedCbk(g_qosId, g_lcid1, g_connHandle1, direction);
    EXPECT_EQ(g_dataPath[1].qosId, g_qosId);
    EXPECT_EQ(g_dataPath[1].state, QOSM_DATAPATH_DELETED);
    EXPECT_EQ(g_dataPath[1].result, 0);
    EXPECT_EQ(g_dataPath[1].connHandle, g_connHandle1);
    EXPECT_EQ(g_dataPath[1].direction, direction);
    ICB_DataPathDeletedCbk(g_qosId, g_lcid2, g_connHandle2, direction);
    EXPECT_EQ(g_dataPath[1].qosId, g_qosId);
    EXPECT_EQ(g_dataPath[1].state, QOSM_DATAPATH_DELETED);
    EXPECT_EQ(g_dataPath[1].result, 0);
    EXPECT_EQ(g_dataPath[1].connHandle, g_connHandle2);
    EXPECT_EQ(g_dataPath[1].direction, direction);
}

TEST_F(UT_QOSM_ICG_MGR_TEST, TestCaseDeleteConnection)
{
    QOSM_AutoRateConnParam connParam = {};
    connParam.qosId = g_qosId;
    connParam.linkCnt = 2;
    QOSM_ConnParam link[2] = {};
    link[0].lcid = g_lcid1;
    link[0].connHandle = g_connHandle1;
    link[1].lcid = g_lcid2;
    link[1].connHandle = g_connHandle2;
    connParam.link = link;

    // icgInfo is NULL
    QOSM_HookLog();
    connParam.qosId = 100;
    QOSM_ICGMgrDeleteConnection(&connParam);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "icgInfo is NULL"), nullptr);

    // CM_ICBDelConnection failed
    connParam.qosId = g_qosId;
    g_deleteConnectionRet = CM_FAIL;
    QOSM_ICGMgrDeleteConnection(&connParam);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "CM_ICBDelConnection failed"), nullptr);

    // icgInfo is NULL
    QOSM_HookLog();
    QOSM_ICGMgrRemoveParam((void *)(uintptr_t)100);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "icgInfo is NULL"), nullptr);

    // CM_ICGRemoveParam failed
    g_removeParamRet = CM_FAIL;
    QOSM_ICGMgrRemoveParam((void *)(uintptr_t)g_qosId);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "QOSM_ICGMgrRemoveParamInner failed"), nullptr);
    QOSM_UnhookLog();

    g_removeParamRet = CM_SUCCESS;
    g_deleteConnectionRet = CM_SUCCESS;
    QOSM_ICGMgrDeleteConnection(&connParam);
    ICB_DisconnectCbk(g_qosId, g_lcid1, g_connHandle1, CM_LINK_ICB);
    EXPECT_EQ(g_connParam[1].qosId, g_qosId);
    EXPECT_EQ(g_connParam[1].state, QOSM_CONNECTION_DELETED);
    EXPECT_EQ(g_connParam[1].result, 0);
    EXPECT_EQ(g_connParam[1].linkCnt, 1);
    EXPECT_EQ(g_connParamLink[2].lcid, g_lcid1);
    EXPECT_EQ(g_connParamLink[2].connHandle, g_connHandle1);
    ICB_DisconnectCbk(g_qosId, g_lcid2, g_connHandle2, CM_LINK_ICB);
    EXPECT_EQ(g_connParam[1].qosId, g_qosId);
    EXPECT_EQ(g_connParam[1].state, QOSM_CONNECTION_DELETED);
    EXPECT_EQ(g_connParam[1].result, 0);
    EXPECT_EQ(g_connParam[1].linkCnt, 1);
    EXPECT_EQ(g_connParamLink[2].lcid, g_lcid2);
    EXPECT_EQ(g_connParamLink[2].connHandle, g_connHandle2);

    // all disconnect, will remove param, but we still call remove param
    QOSM_ICGMgrRemoveParam((void *)(uintptr_t)g_qosId);
    EXPECT_EQ(g_removedParam.id, g_qosId);
    ICB_RemoveParamCbk(g_qosId, CM_ICB_STATE_IOG_REMOVED);
    EXPECT_EQ(g_paramCbk[1].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[1].state, QOSM_PARAM_REMOVED);
    EXPECT_EQ(g_paramCbk[1].result, 0);

    // delete connection which not exist will callback fail
    QOSM_ICGMgrDeleteConnection(&connParam);
    EXPECT_EQ(g_connParam[1].qosId, g_qosId);
    EXPECT_EQ(g_connParam[1].state, QOSM_CONNECTION_DELETED);
    EXPECT_NE(g_connParam[1].result, 0);
    EXPECT_EQ(g_connParam[1].linkCnt, 2);
    EXPECT_EQ(g_connParamLink[2].lcid, g_lcid1);
    EXPECT_EQ(g_connParamLink[2].connHandle, g_connHandle1);
    EXPECT_EQ(g_connParamLink[3].lcid, g_lcid2);
    EXPECT_EQ(g_connParamLink[3].connHandle, g_connHandle2);
}
