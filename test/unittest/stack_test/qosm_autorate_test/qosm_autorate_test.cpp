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

#include "cp_worker.h"
#include "dli_cmd.h"
#include "dli_errno.h"
#include "qosm.h"
#include "qosm_autorate.h"
#include "qosm_autorate_def.h"
#include "qosm_errno.h"
#include "qosm_icg_mgr.h"
#include "qosm_table_mgr.h"
#include "qosm_trans_channel.h"
#include "sdf_addr.h"
#include "common_ext_func_wrapper.h"
#include "common_reg_ext_func.h"

using namespace testing;
using namespace testing::ext;

uint32_t g_stubRet = QOSM_SUCCESS;
uint32_t g_registerRet = QOSM_SUCCESS;
uint32_t g_unregisterRet = QOSM_SUCCESS;
uint32_t g_cpPostRet = CP_OK;
uint32_t g_dliSetPowerModeRet = DLI_SUCCESS;
uint8_t g_dutyCycle = QOS_DUTY_CYCLE_100P;
bool g_is5G = false;
CM_ICBType g_icbType = CM_IOB;

QOSM_StartParam g_startParam = {
    QOS_LEVEL_2 /* startLevel */, QOS_BAND_2D4 /* startBand */, QOS_DUTY_CYCLE_100P /* startDutyCycle */,
    1 /* levelCnt */
};

QOSM_LinkParam g_linkParam = {
    QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_7 /* qosLevel */, QOS_LEVEL_7 /* upQosLevel */,
    QOS_LEVEL_4 /* downQosLevel */, 4600 /* downwardBitrate */, 0 /* upwardBitrate */,
    10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
    300 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 15 /* ftG2T */, 1 /* ftT2G */,
    43 /* rtnG2T */, 1 /* rtnT2G */, 2877 /* maxSduG2T */, 0 /* maxSduT2G */,
    2877 /* maxPduG2T */, 0 /* maxPduT2G */,
    3 /* nse */, 2 /* bnG2T */, 0 /* bnT2G */, 2 /* phyG2T */, 0 /* phyT2G */,
    16 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
};

#ifdef __cplusplus
extern "C" {
#endif

uint32_t CP_PostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (g_cpPostRet != CP_OK) {
        return g_cpPostRet;
    }
    if (cb != nullptr) {
        cb(arg);
    }
    if (freeCb != nullptr) {
        freeCb(arg);
    }
    return 0;
}

uint32_t QOSM_TransChannelInit(void)
{
    return 0;
}

void QOSM_TransChannelDeInit(void)
{
}

void QOSM_ICGMgrEnable(void)
{
}

void QOSM_ICGMgrDisable(void)
{
}

uint32_t QOSM_ICGMgrRegisterCallback(const QOSM_AutoRateCallback *callback)
{
    return g_registerRet;
}

uint32_t QOSM_ICGMgrUnregisterCallback(void)
{
    return g_unregisterRet;
}

void QOSM_ICGMgrSetParam(void *param)
{
}

void QOSM_ICGMgrSetTestParam(void *param)
{
}

void QOSM_ICGMgrRemoveParam(void *id)
{
}

void QOSM_ICGMgrAddConnection(void *param)
{
}

void QOSM_ICGMgrDeleteConnection(void *param)
{
}

void QOSM_ICGMgrAddDataPath(void *param)
{
}

void QOSM_ICGMgrDeleteDataPath(void *param)
{
}

void QOSM_ICGMgrParamNotify(void *param)
{
}

uint8_t QOSM_GetDutyCycle(void)
{
    return g_dutyCycle;
}

bool QOSM_Is5GFreqBand(void)
{
    return g_is5G;
}

QOSM_StartParam *QOSM_GetStartParamByIndex(QOSM_QosIndex qosIndex, uint16_t bitrate, uint8_t linkCnt)
{
    return &g_startParam;
}

QOSM_LinkParam *QOSM_GetQosParamByIndex(QOSM_QosIndex qosIndex, uint8_t qosLevelIndex)
{
    return &g_linkParam;
}

CM_ICBType QOSM_GetICBTypeByIndex(QOSM_QosIndex qosIndex)
{
    return g_icbType;
}

uint32_t QOSM_GetICBG2TParamByIndex(QOSM_QosIndex qosIndex, QOSM_ICBParam *param)
{
    return g_stubRet;
}

uint32_t QOSM_GetICBT2GParamByIndex(QOSM_QosIndex qosIndex, QOSM_ICBParam *param)
{
    return g_stubRet;
}

uint32_t SDF_EvcListenEvent(int handle, SDF_EvcEvent *event)
{
    if (event->callback != nullptr) {
        event->callback(event->eventHandle, event->args);
    }
    return 0;
}

#ifdef __cplusplus
}
#endif

static void TestParamChangedCallback(const QOSM_ParamCb *param){};

static void TestConnectionChangedCallback(const QOSM_ConnParamCb *param){};

static void TestDataPathChangedCallback(const QOSM_DataPathParamCb *param){};

static void TestBitrateChangedCallback(const QOSM_BitrateParamCb *param, uint8_t paramCnt){};

static void TestCallBitrateUpDownCbk(const QOSM_AutoRateSendMsgCb* param){};

class UT_QOSM_AUTORATE_TEST : public testing::Test {
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
        g_registerRet = QOSM_SUCCESS;
        g_unregisterRet = QOSM_SUCCESS;
        g_cpPostRet = CP_OK;
        g_dliSetPowerModeRet = DLI_SUCCESS;
    }
    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
    }

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {
        QOSM_AutoRateCallback cbk = {0};
        cbk.bitrateChangedCbk = TestBitrateChangedCallback;
        cbk.connChangedCbk = TestConnectionChangedCallback;
        cbk.dataPathChangedCbk = TestDataPathChangedCallback;
        cbk.paramChangedCbk = TestParamChangedCallback;
        cbk.callBitrateUpDownCbk = TestCallBitrateUpDownCbk;
        EXPECT_EQ(QOSM_AutoRateRegisterCallback(&cbk), QOSM_SUCCESS);
        EXPECT_EQ(QOSM_Init(), QOSM_SUCCESS);
        EXPECT_EQ(QOSM_Enable(), QOSM_SUCCESS);
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
        EXPECT_EQ(QOSM_AutoRateUnregisterCallback(), QOSM_SUCCESS);
        QOSM_Disable();
        QOSM_DeInit();
    }
};

TEST_F(UT_QOSM_AUTORATE_TEST, QOSM_AutoRateRegisterFailAndUnregisterFail)
{
    g_registerRet = QOSM_FAIL;
    QOSM_AutoRateCallback cbk = {0};
    cbk.bitrateChangedCbk = TestBitrateChangedCallback;
    cbk.connChangedCbk = TestConnectionChangedCallback;
    cbk.dataPathChangedCbk = TestDataPathChangedCallback;
    cbk.paramChangedCbk = TestParamChangedCallback;
    cbk.callBitrateUpDownCbk = TestCallBitrateUpDownCbk;
    EXPECT_EQ(QOSM_AutoRateRegisterCallback(&cbk), g_registerRet);

    g_unregisterRet = QOSM_FAIL;
    EXPECT_EQ(QOSM_AutoRateUnregisterCallback(), g_unregisterRet);
}

TEST_F(UT_QOSM_AUTORATE_TEST, QOSM_AutoRateSetParam)
{
    QOSM_AutoRateParam param = {0};
    param.qosId = 0;
    param.linkCnt = 1;
    param.supportedBitrateCnt = 1;
    param.qosIndex = QOSM_QOSINDEX_AUDIO;

    g_cpPostRet = 1;
    EXPECT_EQ(QOSM_AutoRateSetParam(&param), QOSM_POST_TASK_ERR);

    g_cpPostRet = CP_OK;
    EXPECT_EQ(QOSM_AutoRateSetParam(&param), QOSM_SUCCESS);
}

TEST_F(UT_QOSM_AUTORATE_TEST, QOSM_AutoRateSetTestParam)
{
    QOSM_AutoRateParam param = {0};
    param.qosId = 0;
    param.linkCnt = 1;
    param.supportedBitrateCnt = 1;
    param.qosIndex = QOSM_QOSINDEX_AUDIO;

    g_cpPostRet = 1;
    EXPECT_EQ(QOSM_AutoRateSetTestParam(&param), QOSM_POST_TASK_ERR);

    g_cpPostRet = CP_OK;
    EXPECT_EQ(QOSM_AutoRateSetTestParam(&param), QOSM_SUCCESS);
}

TEST_F(UT_QOSM_AUTORATE_TEST, QOSM_AutoRateRemoveParam)
{
    QOSM_AutoRateParam param = {0};
    param.linkCnt = 1;
    param.supportedBitrateCnt = 1;
    param.qosIndex = QOSM_QOSINDEX_AUDIO;

    g_cpPostRet = 1;
    EXPECT_EQ(QOSM_AutoRateRemoveParam(0), QOSM_POST_TASK_ERR);

    g_cpPostRet = CP_OK;
    EXPECT_EQ(QOSM_AutoRateRemoveParam(0), QOSM_SUCCESS);
}

TEST_F(UT_QOSM_AUTORATE_TEST, QOSM_AutoRateAddConnection)
{
    QOSM_AutoRateConnParam param = {0};
    QOSM_ConnParam connParam = {0};
    param.link = &connParam;
    param.linkCnt = 1;

    g_cpPostRet = 1;
    EXPECT_EQ(QOSM_AutoRateAddConnection(&param), QOSM_POST_TASK_ERR);

    g_cpPostRet = CP_OK;
    EXPECT_EQ(QOSM_AutoRateAddConnection(&param), QOSM_SUCCESS);
}

TEST_F(UT_QOSM_AUTORATE_TEST, QOSM_AutoRateDeleteConnection)
{
    QOSM_AutoRateConnParam param = {0};
    QOSM_ConnParam connParam = {0};
    param.link = &connParam;
    param.linkCnt = 1;

    g_cpPostRet = 1;
    EXPECT_EQ(QOSM_AutoRateDeleteConnection(&param), QOSM_POST_TASK_ERR);

    g_cpPostRet = CP_OK;
    EXPECT_EQ(QOSM_AutoRateDeleteConnection(&param), QOSM_SUCCESS);
}

TEST_F(UT_QOSM_AUTORATE_TEST, QOSM_AutoRateAddDataPath)
{
    QOSM_AutoRateDataPath path = {0};
    uint8_t codecConfigData = 0;
    path.codecConfigData = &codecConfigData;
    path.codecConfigLen = 1;

    g_cpPostRet = 1;
    EXPECT_EQ(QOSM_AutoRateAddDataPath(&path), QOSM_POST_TASK_ERR);

    g_cpPostRet = CP_OK;
    EXPECT_EQ(QOSM_AutoRateAddDataPath(&path), QOSM_SUCCESS);
}

TEST_F(UT_QOSM_AUTORATE_TEST, QOSM_AutoRateDeleteDataPath)
{
    QOSM_AutoRateDeletedDataPath path = {0};

    g_cpPostRet = 1;
    EXPECT_EQ(QOSM_AutoRateDeleteDataPath(&path), QOSM_POST_TASK_ERR);

    g_cpPostRet = CP_OK;
    EXPECT_EQ(QOSM_AutoRateDeleteDataPath(&path), QOSM_SUCCESS);
}

TEST_F(UT_QOSM_AUTORATE_TEST, QOSM_AutoRateSetEarphoneFeedback)
{
    QOSM_AutoRateEarphoneFeedbackParam param = {};
    param.supportedBitrateCnt = 1;

    g_cpPostRet = 1;
    EXPECT_EQ(QOSM_AutoRateSetEarphoneFeedback(&param), QOSM_POST_TASK_ERR);

    g_cpPostRet = CP_OK;
    EXPECT_EQ(QOSM_AutoRateSetEarphoneFeedback(&param), QOSM_SUCCESS);
}

TEST_F(UT_QOSM_AUTORATE_TEST, QOSM_AutoRateSetCoexistSuggestion)
{
    QOSM_AutoRateCoexistSuggestionParam param = {};
    param.maxBitrate = QOSM_MIN_BITRATE;
    param.dutyCycle = QOS_DUTY_CYCLE_100P;

    g_cpPostRet = 1;
    EXPECT_EQ(QOSM_AutoRateSetCoexistSuggestion(&param), QOSM_POST_TASK_ERR);

    g_cpPostRet = CP_OK;
    EXPECT_EQ(QOSM_AutoRateSetCoexistSuggestion(&param), QOSM_SUCCESS);
}

TEST_F(UT_QOSM_AUTORATE_TEST, QOSM_AutoRateGetICGG2TParam)
{
    QOSM_ICBParam param = {0};
    EXPECT_EQ(QOSM_AutoRateGetICGG2TParam(QOSM_QOSINDEX_AUDIO, &param), QOSM_SUCCESS);
}

TEST_F(UT_QOSM_AUTORATE_TEST, QOSM_AutoRateGetICGT2GParam)
{
    QOSM_ICBParam param = {0};
    EXPECT_EQ(QOSM_AutoRateGetICGT2GParam(QOSM_QOSINDEX_AUDIO, &param), QOSM_SUCCESS);
}
