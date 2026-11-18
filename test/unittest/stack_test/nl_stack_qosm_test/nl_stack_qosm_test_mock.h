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

#ifndef NL_STACK_QOSM_TEST_MOCK_H
#define NL_STACK_QOSM_TEST_MOCK_H

#include <stdint.h>
#include <stdbool.h>
#include "cm_def.h"
#include "cm_icb_api.h"
#include "cm_icb_mgr.h"
#include "qosm_autorate.h"
#include "qosm_icg_types.h"
#include "sdf_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QOSM_TEST_TIMER_NUM 6
#define QOSM_TEST_DELAY_PARAM_TIMER 0
#define QOSM_TEST_DELAY_CONN_TIMER 1
#define QOSM_TEST_ADD_CONN_TIMER 2
#define QOSM_TEST_RETRY_CONN_TIMER 3
#define QOSM_TEST_UPGRADE_LABEL_TIMER 4
#define QOSM_TEST_DOWNGRADE_LABEL_TIMER 5

#define QOSM_TEST_BITRATE_HISTORY_CNT 8

extern CM_ICBCallback g_icbCbk;
extern CM_FreqBandListener g_freqBandListener;
extern uint8_t g_levelCnt;
extern uint8_t g_labelId;
extern uint8_t g_qosId;
extern uint16_t g_lcid1;
extern uint16_t g_lcid2;
extern uint16_t g_connHandle1;
extern uint16_t g_connHandle2;

extern uint32_t g_registerCbkRet;
extern uint32_t g_listenFreqBandRet;
extern uint32_t g_setParamRet;
extern uint32_t g_setAutorateParamRet;
extern uint32_t g_setLabelRet;
extern uint32_t g_addConnectionRet;
extern uint32_t g_cmSetDataPathRet;
extern uint32_t g_cmRemoveDataPathRet;
extern uint32_t g_removeParamRet;
extern uint32_t g_deleteConnectionRet;
extern uint32_t g_cpTimerRet;

extern SDF_TimerParam g_timerParam[QOSM_TEST_TIMER_NUM];
extern CM_ICGLabelParam g_labelParam;
extern CM_ICBChannel g_labelIcb[2];
extern CM_ICBConnectionParam g_createParam;
extern CM_ICBChannel g_createChannel[2];
extern CM_ICGUpdatedParam g_updateParam;
extern uint16_t g_updateChannel[2];
extern CM_ICBConnectionParam g_disconnectParam;
extern CM_ICBChannel g_disconnectChannel[2];
extern CM_ICGParam g_settedParam;
extern CM_ICGAutorateParam g_settedAutorateParam;
extern CM_ICGRemovedParam g_removedParam;

extern QOSM_ParamCb g_paramCbk[2];
extern QOSM_ConnParamCb g_connCbk[2];
extern QOSM_ConnParam g_connCbkLink[4];
extern QOSM_DataPathParamCb g_dataPathCbk[2];
extern QOSM_BitrateParamCb g_bitrateCbk[QOSM_TEST_BITRATE_HISTORY_CNT];
extern uint8_t g_bitrateCbkCnt;
extern QOSM_AutoRateSendMsgCb g_sendMsgCbk[QOSM_TEST_BITRATE_HISTORY_CNT];
extern uint8_t g_sendMsgCbkCnt;
extern struct QOSM_AudioDfxInfo g_audioDfxInfo;
extern uint32_t g_dfxBand;
extern uint8_t g_powerLevel;
extern uint32_t g_dfxUpdateConnCnt;
extern bool g_useDelayParamTimer;

void TestResetMock(void);
void TestRegisterCallback(void);
void TestUnregisterCallback(void);
void TestSetAudioAutorateParam(uint8_t qosId, uint8_t startLevel);
void TestSetCallParam(uint8_t qosId);
void TestSetOtherParam(uint8_t qosId, uint8_t qosIndex, uint8_t levelCnt, bool isAutorate);
void TestAddParamCbk(uint8_t qosId, int state, uint32_t errorCode, uint8_t channelCnt);
void TestLabelReportCbk(uint8_t qosId, uint16_t lcid, uint16_t connHandle, bool success, bool isCall);
void TestEstablishedCbk(uint8_t qosId, uint16_t lcid, uint16_t connHandle, bool success);
void TestQualityCbkEx(uint16_t connHandle, uint8_t ackRate, int8_t rssi, uint16_t txFlushed, uint32_t diffMax,
    int icbType, uint8_t errPacketRate);
void TestQualityCbk(uint16_t connHandle, uint8_t ackRate, int8_t rssi, uint16_t txFlushed, uint32_t diffMax);
void TestUpdateParamCbk(uint8_t qosId, uint16_t lcid, uint16_t connHandle, uint8_t labelId, uint32_t errorCode);
void TestDisconnectCbk(uint8_t qosId, uint16_t lcid, uint16_t connHandle, int disconnectType, uint32_t errorCode);
void TestRemoveParamCbk(uint8_t qosId, int state);
void TestDataPathCbk(uint8_t qosId, int state, uint8_t direction);
void TestAddConnection(uint8_t qosId);
void TestDeleteConnection(uint8_t qosId, uint16_t connHandle);
void TestParamNotifyEarphone(const uint16_t *supportedBitrate, uint32_t cnt);
void TestParamNotifyCoexist(uint16_t maxBitrate, uint8_t dutyCycle);
void TestParamNotify5G(void);
void TestConnectIcg(uint8_t qosId);
void TestFireTimer(int timerId);

#ifdef __cplusplus
}
#endif
#endif
