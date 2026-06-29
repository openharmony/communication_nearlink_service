/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "qosm_audio_dfx.h"
#include <gtest/gtest.h>
#include <time.h>
#include <inttypes.h>
#include "qosm_log.h"
#include "sysdep.h"
#include "cp_worker.h"
#include "nearlink_dft_exception_c.h"
#include "qosm_uevent.h"
#include "sdf_mem.h"
#include "qosm_autorate_def.h"
#include "qosm_errno.h"
#include "qosm_table_mgr.h"
#include "securec.h"
#include "qosm_autorate_test_log.h"
#include "common_ext_func_wrapper.h"
#include "common_reg_ext_func.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DSP_ON1_STR "MAQAAAEE0g=="
#define DSP_ON2_STR "MAQAAAEE0g=="
#define DSP_OFF_STR "QB5wAAI4VgEFAmAYAAICAjwRAwI+FwQCbhMIAaQJAk8E"

/* {0x01 type|flag, 0x05 len, 0x00 seq|subSeq, 0x01 codecType, 0x02 codecAlgo, 0x78, 0x56, 0x34 errorCode} */
#define INVALID_CODEC_EXCEPTION "AQUAAQJ4VjQ="
/* {0x01 type|flag, 0x06 len, 0x00 seq|subSeq, 0x01 codecType, 0x02 codecAlgo, 0x78, 0x56, 0x34, 0x12 errorCode} */
#define CODEC_EXCEPTION "AQYAAQJ4VjQS"

/* {0x11 type|flag, 0x03 len, 0x00 seq|subSeq, 0xA5 , 0x00 connHandle, 0x77 choppyType} */
#define INVALID_CHOPPY_EVENT "EQMApQB3"
/* {0x11 type|flag, 0x04 len, 0x00 seq|subSeq, 0xA5 , 0x00 connHandle, 0x77 choppyType, 0x00 reserve} */
#define CHOPPY_EVENT "EQQApQB3AA=="

/* {0x61 type|flag, 0x05 len, 0x00 seq|subSeq, 0x34 , 0x12 lastSn, 0x78, 0x56 curSn, 0xBC lostCnt} */
#define INCALID_RX_LOST "YQUANBJ4Vrw="
/* {0x61 type|flag, 0x06 len, 0x00 seq|subSeq, 0x34 , 0x12 lastSn, 0x78, 0x56 curSn, 0xBC, 0x9A lostCnt} */
#define RX_LOST "YQYANBJ4Vrya"

/* {0x71 type|flag, 0x03 len, 0x00 seq|subSeq, 0x00, 0x00, 0x00} */
#define RX_NO_PKT "cQMAAAAA"

#define A5_ENTER_FLOW "UAMwAaUA"
#define A5_EXIT_FLOW "UAOgAKUA"

static bool g_isDspOn = false;
static bool g_enterFlowCtl = false;
static QOSM_DspDataProcessCb g_dspDataProc = nullptr;
static QOSM_DftAudioChoppyExcep g_choppyExcep = {};
static QOSM_DftAudioStats g_stats = {};
static QOSM_DftAudioCodecExcep g_codecExcep = {};

void QOSM_UeventInit(QOSM_DspDataProcessCb process)
{
    g_dspDataProc = process;
}

void QOSM_UeventDeinit(void)
{
    g_dspDataProc = nullptr;
}

void DftReportAudioChoppyExcep(const QOSM_DftAudioChoppyExcep *excep)
{
    (void)memcpy_s(&g_choppyExcep, sizeof(QOSM_DftAudioChoppyExcep), excep, sizeof(QOSM_DftAudioChoppyExcep));
}

void DftReportAudioStats(const QOSM_DftAudioStats *stats)
{
    (void)memcpy_s(&g_stats, sizeof(QOSM_DftAudioStats), stats, sizeof(QOSM_DftAudioStats));
}

void DftReportAudioCodecExcep(const QOSM_DftAudioCodecExcep *excep)
{
    (void)memcpy_s(&g_codecExcep, sizeof(QOSM_DftAudioCodecExcep), excep, sizeof(QOSM_DftAudioCodecExcep));
}

bool QOSM_ExecuteBitrateChangeDecision(uint16_t connHandle, uint16_t qosIndex, uint16_t reportedDirection,
    uint16_t reportedQosLevel)
{
    (void)connHandle;
    (void)qosIndex;
    (void)reportedDirection;
    (void)reportedQosLevel;
    return false;
}

void QOSM_PrintQualityReportParam(CM_ICBQuality *param)
{
    (void)param;
}

uint32_t CP_PostTaskBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    if (cb != nullptr) {
        cb(arg);
    }
    if (freeCb != nullptr) {
        freeCb(arg);
    }
    return 0;
}

uint32_t CP_TimerAdd(int *timerId, SDF_TimerParam *param)
{
    (void)timerId;
    (void)param;
    return 0;
}

void CP_TimerDel(int timerId)
{
    (void)timerId;
}

#ifdef __cplusplus
}
#endif

class UT_QOSM_AUDIO_DFX_TEST : public testing::Test {
protected:
    void SetUp()
    {
        QOSM_UnhookLog();
        g_isDspOn = false;
        g_enterFlowCtl = false;
        (void)memset_s(&g_choppyExcep, sizeof(QOSM_DftAudioChoppyExcep), 0, sizeof(QOSM_DftAudioChoppyExcep));
        (void)memset_s(&g_codecExcep, sizeof(QOSM_DftAudioCodecExcep), 0, sizeof(QOSM_DftAudioCodecExcep));
    }

    virtual void TearDown()
    {
    }

    static void SetUpTestCase()
    {
    }

    static void TearDownTestCase()
    {
    }
};

static void QOSM_AudioDfxDspStatusCbTest(bool isOn)
{
    g_isDspOn = isOn;
}

static void QOSM_AudioDfxFlowCtrlCbTest(uint16_t connHandle, bool enterFlowCtrl)
{
    g_enterFlowCtl = enterFlowCtrl;
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxStart)
{
    struct QOSM_AudioDfxInfo info = {};
    info.startBitrate = 320;
    info.startBand = QOS_BAND_2D4;
    info.param.sduInterval = 10000;
    info.param.ft = 15;
    info.param.bn = 2;
    info.dspStatusCb = QOSM_AudioDfxDspStatusCbTest;
    info.flowCtrlCb = QOSM_AudioDfxFlowCtrlCbTest;
    QOSM_AudioDfxStart(&info);
    EXPECT_NE(g_dspDataProc, nullptr);
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxUpdateConnected)
{
    struct QOSM_AudioDfxConn conn = {};
    conn.connHandle = 0xA5;
    conn.lcid = 3;
    conn.addr[0] = 0x01;
    conn.addr[1] = 0x02;
    conn.addr[2] = 0x03;
    conn.addr[3] = 0x04;
    conn.addr[4] = 0x05;
    conn.addr[5] = 0x06;
    QOSM_AudioDfxUpdateConn(&conn, true);

    struct QOSM_AudioDfxChoppyInfo info = {};
    info.connHandle = 0xA5;
    info.txFlushed = 1;
    info.ackRate = 1;
    info.rssi = -50;
    QOSM_AudioDfxNotifyChoppy(&info);
    EXPECT_EQ(g_choppyExcep.deviceAddr[0], conn.addr[0]);
    EXPECT_EQ(g_choppyExcep.deviceAddr[1], conn.addr[1]);
    EXPECT_EQ(g_choppyExcep.deviceAddr[2], conn.addr[2]);
    EXPECT_EQ(g_choppyExcep.deviceAddr[3], conn.addr[3]);
    EXPECT_EQ(g_choppyExcep.deviceAddr[4], conn.addr[4]);
    EXPECT_EQ(g_choppyExcep.deviceAddr[5], conn.addr[5]);
    EXPECT_EQ(g_choppyExcep.ackRate, 1);
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxUpdateBitrate)
{
    // bitrate not changed
    uint32_t bitrate = 320;
    QOSM_AudioDfxUpdateBitrate(bitrate);
    struct QOSM_AudioDfxChoppyInfo info = {};
    info.connHandle = 0xA5;
    QOSM_AudioDfxNotifyChoppy(&info);
    EXPECT_EQ(g_choppyExcep.bitrate, 320);

    // invalid bitrate
    bitrate = 0;
    QOSM_AudioDfxUpdateBitrate(bitrate);
    QOSM_AudioDfxNotifyChoppy(&info);
    EXPECT_EQ(g_choppyExcep.bitrate, 320);

    bitrate = 640;
    QOSM_AudioDfxUpdateBitrate(bitrate);
    QOSM_AudioDfxNotifyChoppy(&info);
    EXPECT_EQ(g_choppyExcep.bitrate, 640);
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxUpdateBand)
{
    // set illegal band
    uint32_t band = QOSM_AUDIO_DFX_BAND_MAX;
    QOSM_AudioDfxUpdateBand(band);
    struct QOSM_AudioDfxChoppyInfo info = {};
    info.connHandle = 0xA5;
    QOSM_AudioDfxNotifyChoppy(&info);
    EXPECT_EQ(g_choppyExcep.freqBand, 0);

    band = QOSM_AUDIO_DFX_BAND_5G_1;
    QOSM_AudioDfxUpdateBand(band);
    QOSM_AudioDfxNotifyChoppy(&info);
    EXPECT_EQ(g_choppyExcep.freqBand, QOSM_AUDIO_DFX_BAND_5G_1);

    // set QOSM_AUDIO_DFX_BAND_5G_1 repeatly
    QOSM_AudioDfxUpdateBand(band);
    QOSM_AudioDfxNotifyChoppy(&info);
    EXPECT_EQ(g_choppyExcep.freqBand, QOSM_AUDIO_DFX_BAND_5G_1);
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxUpdatePowerLevel)
{
    // pass illegal level
    uint16_t connHandle = 0xA5;
    uint8_t level = QOSM_POWER_LEVEL_MAX + 2;
    QOSM_AudioDfxUpdatePowerLevel(connHandle, level);
    struct QOSM_AudioDfxChoppyInfo info = {};
    info.connHandle = 0xA5;
    QOSM_AudioDfxNotifyChoppy(&info);
    // default level is QOSM_POWER_LEVEL_MAX + 1
    EXPECT_EQ(g_choppyExcep.powerInfo, QOSM_POWER_LEVEL_MAX + 1);

    // update level firstly
    level = QOSM_POWER_LEVEL_MAX;
    QOSM_AudioDfxUpdatePowerLevel(connHandle, level);
    QOSM_AudioDfxNotifyChoppy(&info);
    EXPECT_EQ(g_choppyExcep.powerInfo, QOSM_POWER_LEVEL_MAX);

    // update level secondly
    level = QOSM_POWER_LEVEL_MIN;
    QOSM_AudioDfxUpdatePowerLevel(connHandle, level);
    QOSM_AudioDfxNotifyChoppy(&info);
    EXPECT_EQ(g_choppyExcep.powerInfo, QOSM_POWER_LEVEL_MIN);
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxGetDspStatus)
{
    bool isDspOn = true;
    uint32_t ret = QOSM_AudioDfxGetDspStatus(&isDspOn);
    EXPECT_EQ(ret, QOSM_SUCCESS);
    EXPECT_EQ(isDspOn, false);

    g_dspDataProc(DSP_ON1_STR);
    g_dspDataProc(DSP_ON2_STR);
    sleep(3);
    ret = QOSM_AudioDfxGetDspStatus(&isDspOn);
    EXPECT_EQ(ret, QOSM_SUCCESS);
    EXPECT_EQ(isDspOn, true);
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxQualityReport)
{
    uint16_t txFlushed = 0;
    uint16_t ackrate = 0;
    QOSM_AudioDfxQualityReport(txFlushed, ackrate);

    g_dspDataProc(DSP_OFF_STR);
    txFlushed = 0;
    ackrate = 0;
    QOSM_AudioDfxQualityReport(txFlushed, ackrate);

    txFlushed = 1;
    ackrate = 0;
    QOSM_AudioDfxQualityReport(txFlushed, ackrate);
    EXPECT_EQ(g_isDspOn, true);
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxCodecException)
{
    // illegal data
    g_dspDataProc(INVALID_CODEC_EXCEPTION);
    EXPECT_EQ(g_codecExcep.codecType, 0);
    EXPECT_EQ(g_codecExcep.codecAlgo, 0);
    EXPECT_EQ(g_codecExcep.errorCode, 0);

    // {0x01 type|flag, 0x06 len, 0x00 seq|subSeq, 0x01 codecType, 0x02 codecAlgo, 0x78, 0x56, 0x34, 0x12 errorCode}
    g_dspDataProc(CODEC_EXCEPTION);
    EXPECT_EQ(g_codecExcep.codecType, 0x01);
    EXPECT_EQ(g_codecExcep.codecAlgo, 0x02);
    EXPECT_NE(g_codecExcep.errorCode, 0);
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxChoppyEvent)
{
    QOSM_HookLog();

    // illegal data
    g_dspDataProc(INVALID_CHOPPY_EVENT);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "invalid choppy len 3"), nullptr);

    // {0x11 type|flag, 0x04 len, 0x00 seq|subSeq, 0xA5 , 0x00 connHandle, 0x77 choppyType, 0x00 reserve}
    g_dspDataProc(CHOPPY_EVENT);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "conn handle 0x00a5 choppied, choppy type: 119, rsv: 0"), nullptr);
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxFlowControl)
{
    g_dspDataProc(A5_ENTER_FLOW);
    EXPECT_EQ(g_enterFlowCtl, true);
    g_dspDataProc(A5_EXIT_FLOW);
    EXPECT_EQ(g_enterFlowCtl, false);
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxRxLost)
{
    QOSM_HookLog();

    // illegal data
    g_dspDataProc(INCALID_RX_LOST);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "invalid len: 5"), nullptr);

    // {0x61 type|flag, 0x06 len, 0x00 seq|subSeq, 0x34 , 0x12 lastSn, 0x78, 0x56 curSn, 0xBC, 0x9A lostCnt}
    g_dspDataProc(RX_LOST);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "rx pkt lost, last sn: 4660, cur sn: 22136, lost cnt: 39612"), nullptr);
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxRxNoPkt)
{
    QOSM_HookLog();

    g_dspDataProc(RX_NO_PKT);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "no pkt recv in last 5s"), nullptr);
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxProcessDspData)
{
    QOSM_HookLog();

    // 0x11 type|flag
    g_dspDataProc("==");
    EXPECT_NE(strstr(QOSM_GetHookLog(), "invalid str len 2"), nullptr);

    // 0x11 type|flag, 0x00 len
    g_dspDataProc("EQA=");
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "base64 decode failed"), nullptr);

    // 0x11 type|flag, 0x10 len, 0x00 seq|subSeq
    g_dspDataProc("ERAA");
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "invalid len 16, output len: 3"), nullptr);
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxUpdateDisconnected)
{
    struct QOSM_AudioDfxConn conn = {};
    conn.connHandle = 0xA5;
    conn.lcid = 3;
    conn.addr[0] = 0x01;
    conn.addr[1] = 0x02;
    conn.addr[2] = 0x03;
    conn.addr[3] = 0x04;
    conn.addr[4] = 0x05;
    conn.addr[5] = 0x06;
    QOSM_AudioDfxUpdateConn(&conn, false);
    struct QOSM_AudioDfxChoppyInfo info = {};
    info.connHandle = 0xA5;
    QOSM_AudioDfxNotifyChoppy(&info);
    EXPECT_EQ(g_choppyExcep.deviceAddr[0], 0);
    EXPECT_EQ(g_choppyExcep.deviceAddr[1], 0);
    EXPECT_EQ(g_choppyExcep.deviceAddr[2], 0);
    EXPECT_EQ(g_choppyExcep.deviceAddr[3], 0);
    EXPECT_EQ(g_choppyExcep.deviceAddr[4], 0);
    EXPECT_EQ(g_choppyExcep.deviceAddr[5], 0);
    EXPECT_EQ(g_choppyExcep.ackRate, 0);
}

TEST_F(UT_QOSM_AUDIO_DFX_TEST, TestCaseAudioDfxStop)
{
    // 使用python生成：data = bytes([0x20, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02])
    // print(base64.b64encode(data).decode())
    // type=2, len=5, bitrateChangeResult=0, setLabelResult=1
    g_dspDataProc("IAUAAAAAAAE=");
    // type=2, len=5, bitrateChangeResult=0, setLabelResult=2
    g_dspDataProc("IAUAAAAAAAI=");
    // type=2, len=5, bitrateChangeResult=0, setLabelResult=3
    g_dspDataProc("IAUAAAAAAAM=");
    // type=2, len=5, bitrateChangeResult=0, setLabelResult=4
    g_dspDataProc("IAUAAAAAAAQ=");
    // type=2, len=5, bitrateChangeResult=0, setLabelResult=5
    g_dspDataProc("IAUAAAAAAAU=");
    // type=2, len=5, bitrateChangeResult=0, setLabelResult=6
    g_dspDataProc("IAUAAAAAAAY=");
    // type=2, len=5, bitrateChangeResult=0, setLabelResult=5
    g_dspDataProc("IAUAAAAAAAU=");

    QOSM_HookLog();
    QOSM_AudioDfxStop();
    EXPECT_EQ(g_dspDataProc, nullptr);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "5:2"), nullptr);
}
