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

#include <cstring>
#include <stdlib.h>
#include "securec.h"
#include "gtest/gtest.h"
#include "sdf_mem.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dli_mock.h"
#include "stack_dli_stub.h"

#include "cm_logic_link_api.h"
#include "dli_event_struct.h"
#include "dli_cmd.h"
#include "dli.h"

#include "hadm_init.h"
#include "hadm_api.h"
#include "hadm_parser_iq.h"
#include "hadm_link_manager.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;

static uint8_t g_localIqData[] = {
    0x00, 0x00, 0x00, 0x00, 0xC4, 0x80, 0x50, 0x8E, 0x12, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
    0xB4, 0x5D, 0x01, 0x00, 0xCF, 0x0C, 0x06, 0x40, 0x00, 0x01,
    0x60, 0x7F, 0xF7, 0x77, 0x00, 0xF4, 0xF7, 0x7F, 0x07, 0x80,
    0x00, 0x01, 0x40, 0x7F, 0xF7, 0x57, 0x00, 0x08, 0x30, 0x00,
    0x01, 0x80, 0x00, 0x06, 0xC0, 0x7F, 0xF8, 0xC7, 0x7F, 0x01,
    0x80, 0x00, 0x06, 0xA0, 0x7F, 0x08, 0x20, 0x00, 0xFD, 0x87,
    0x7F, 0xFB, 0x77, 0x00, 0x09, 0xF0, 0x7F, 0xF9, 0x97, 0x7F,
    0xFF, 0x77, 0x00, 0xFB, 0x37, 0x00, 0x02, 0x00, 0x00, 0x01,
    0xF0, 0x7F, 0xFD, 0xD7, 0x7F, 0x01, 0xC0, 0x7F, 0xFB, 0x27,
    0x00, 0x05, 0x30, 0x00, 0xFE, 0x87, 0x7F, 0xFB, 0x57, 0x00,
    0xF8, 0xF7, 0x7F, 0x04, 0x60, 0x00, 0x03, 0xA0, 0x7F, 0xFA,
    0x17, 0x00, 0xFB, 0xA7, 0x7F, 0xFE, 0x77, 0x00, 0x06, 0xD0,
    0x7F, 0xF9, 0xA7, 0x7F, 0xFE, 0x97, 0x00, 0xF8, 0x67, 0x00,
    0x09, 0x10, 0x00, 0xFB, 0x77, 0x7F, 0xFC, 0x77, 0x00, 0x07,
    0xF0, 0x7F, 0x06, 0x50, 0x00, 0x00, 0x80, 0x7F, 0xF9, 0x67,
    0x00, 0x09, 0x10, 0x00, 0x06, 0x50, 0x00, 0x01, 0xA0, 0x7F,
    0xFB, 0x27, 0x00, 0x05, 0x20, 0x00, 0xFF, 0xB7, 0x7F, 0xFC,
    0x37, 0x00, 0x04, 0x10, 0x00, 0x02, 0x30, 0x00, 0x01, 0xD0,
    0x7F, 0xFF, 0x17, 0x00, 0xFE, 0xE7, 0x7F, 0x00, 0x10, 0x00,
    0xFF, 0xF7, 0x7F, 0x01, 0x00, 0x00, 0xFD, 0xF7, 0x7F, 0x04,
    0x40, 0x00, 0xFC, 0x97, 0x7F, 0x00, 0x60, 0x7F, 0xFC, 0x77,
    0x00, 0x07, 0xC0, 0x7F, 0x08, 0x20, 0x00, 0xFA, 0xA7, 0x7F,
    0x00, 0x70, 0x00, 0x05, 0xA0, 0x7F, 0xF8, 0xF7, 0x7F, 0xFB,
    0xB7, 0x7F, 0xFE, 0x97, 0x7F, 0x03, 0xA0, 0x7F, 0xFA, 0x27,
    0x00, 0x04, 0x30, 0x00, 0x03, 0x50, 0x00, 0xFF, 0xA7, 0x7F,
    0xFE, 0x57, 0x00, 0x02, 0x00, 0x01
};

static uint8_t g_remoteIqData[] = {
    0x00, 0x00, 0x00, 0x00, 0xC4, 0x80, 0x50, 0x8E, 0x12, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
    0xDF, 0xC0, 0x00, 0x00, 0xCB, 0x0C, 0xE8, 0x37, 0x7E, 0x21,
    0xB0, 0x7D, 0x34, 0xC0, 0x01, 0x0C, 0x10, 0x7C, 0x3C, 0x10,
    0x00, 0x05, 0x80, 0x03, 0xCB, 0xC7, 0x00, 0xF3, 0x47, 0x7D,
    0xD8, 0x77, 0x00, 0xFD, 0x87, 0x7D, 0x2A, 0x00, 0x00, 0xFD,
    0xD7, 0x02, 0xD2, 0xB7, 0x7F, 0xFA, 0xF7, 0x02, 0xD2, 0x57,
    0x7F, 0x0B, 0xE0, 0x7C, 0x33, 0x90, 0x00, 0xFB, 0x17, 0x03,
    0xD4, 0xE7, 0x7F, 0x03, 0x00, 0x02, 0xF1, 0x27, 0x00, 0x07,
    0x10, 0x00, 0xF6, 0x57, 0x01, 0x1C, 0xB0, 0x00, 0xEF, 0xD7,
    0x01, 0xDF, 0x97, 0x7E, 0x1D, 0xE0, 0x7D, 0x21, 0x00, 0x02,
    0x23, 0x40, 0x7E, 0x17, 0x80, 0x02, 0xD3, 0xE7, 0x00, 0xF9,
    0x47, 0x7D, 0xD0, 0x97, 0x7F, 0x0D, 0xE0, 0x7C, 0x2D, 0x60,
    0x01, 0xDE, 0x97, 0x02, 0xD5, 0x17, 0x7D, 0xCF, 0x57, 0x02,
    0xE2, 0xA7, 0x7C, 0x38, 0xA0, 0x7E, 0x0A, 0x70, 0x03, 0xC9,
    0xF7, 0x7F, 0xF3, 0x27, 0x03, 0xD6, 0x87, 0x7E, 0x25, 0x90,
    0x7D, 0x22, 0x90, 0x02, 0x22, 0x00, 0x7E, 0x0E, 0x20, 0x02,
    0xDB, 0x47, 0x00, 0x02, 0xD0, 0x7D, 0x1D, 0xA0, 0x00, 0xEE,
    0x27, 0x01, 0xF0, 0x87, 0x7E, 0xE9, 0xB7, 0x00, 0xFA, 0xC7,
    0x7E, 0x0D, 0x10, 0x00, 0x06, 0x80, 0x7F, 0x05, 0x70, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEA, 0xA7, 0x00, 0xEC,
    0x87, 0x7D, 0x33, 0x10, 0x7E, 0xDB, 0x77, 0x7C, 0x38, 0x90,
    0x7D, 0x1E, 0x90, 0x03, 0x3B, 0xA0, 0x7E, 0x0C, 0x70, 0x03,
    0xCB, 0x07, 0x00, 0x09, 0x30, 0x7D, 0x20, 0x70, 0x01, 0x22,
    0x70, 0x7E, 0xED, 0x87, 0x7D, 0xD9, 0x97, 0x00, 0x02, 0xD0,
    0x7D, 0x20, 0xD0, 0x00, 0x15, 0x00, 0x7E, 0x1E, 0x50, 0x01,
    0xEE, 0x67, 0x01, 0x02, 0x01, 0x00
};

void TriggerHadmUserStartEvent()
{
    HadmConnectionParam_S connectionParam = {0};
    HadmSoundingParam_S soundParam = {0};
    (void)HadmStartSounding(&g_addr, &connectionParam, &soundParam);
}

void TriggerHadmUserStopEvent()
{
    HadmStopSounding(&g_addr);
}

void TriggerCmConnectEvent()
{
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_HADM, &linkState);
}

void TriggerCmDisconnectEvent()
{
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_HADM, &linkState2);
}

void TriggerCmReportFeatureEvent()
{
    CM_LogicLinkRemoteFeatures_S feaures = {.lcid = g_lcid, .features = {0, 0, 0, 0, 0, 0, 0, 0, 4, 0}};
    TEST_CM_RemoteFeatureCbk(CM_MODULE_HADM, &feaures);
}

void TriggerCmUpdateConnParamEvent()
{
    CM_LogicLinkConnUpdateParam_S connUpdateCbkParam = {0};
    connUpdateCbkParam.lcid = g_lcid;
    memcpy_s(&connUpdateCbkParam.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_ConnUpdateCbk(CM_MODULE_HADM, &connUpdateCbkParam);
}

void TriggerDliReportRemoteCsEvent()
{
    DLI_ExecuteCmdRetParam remoteCsRetParam = {0};
    DLI_ReadRemoteCsCapsEvt caps = {.connHandle = g_lcid};
    remoteCsRetParam.eventParameter = &caps;
    TEST_DLI_EventCbk(DLI_CBK_READ_REMOTE_MEASURE_CAPS, 0, &remoteCsRetParam);
}

void TriggerDliReportSoundingConfigEvent()
{
    DLI_ExecuteCmdRetParam csConfigParam = {0};
    TEST_DLI_EventCbk(DLI_CBK_SET_MEASURE_CONFIG_PARAM, 0, &csConfigParam);
}

void TriggerDliReportSoundingResultEvent()
{
    DLI_ExecuteCmdRetParam csEnableParam = {0};
    TEST_DLI_EventCbk(DLI_CBK_SET_MEASURE_EN, 0, &csEnableParam);
}

void SetHadmSmToIdle()
{
    TriggerCmConnectEvent();
}

void SetHadmSmToReady()
{
    SetHadmSmToIdle();
    TriggerCmReportFeatureEvent();
    TriggerDliReportRemoteCsEvent();
}

void SetHadmSmToConnectionConfig()
{
    SetHadmSmToReady();
    TriggerHadmUserStartEvent();
}

void SetHadmSmToConfigSounding()
{
    SetHadmSmToConnectionConfig();
    TriggerCmUpdateConnParamEvent();
}

void SetHadmSmToEnableSounding()
{
    SetHadmSmToConfigSounding();
    TriggerDliReportSoundingConfigEvent();
}

void SetHadmSmToSounding()
{
    SetHadmSmToEnableSounding();
    TriggerDliReportSoundingResultEvent();
}

void SetHadmSmToDisableSounding()
{
    SetHadmSmToSounding();
    HadmStopSounding(&g_addr);
}

static void STUB_HadmSoundingIqCbk(SLE_Addr_S *addr, HadmSoundingIqData_S *args)
{
    return;
}

static NLSTK_Errcode_E userErrorCode = NLSTK_ERRCODE_MAX;

static void STUB_HadmSoundingControlResultCbk(SLE_Addr_S *addr, HadmUserOperate_E ctrlType, NLSTK_Errcode_E errorcode)
{
    userErrorCode = errorcode;
}

static void STUB_HadmSoundingStateReport(HadmSoundingStateInfo_S *state)
{
    return;
}

class HADM_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DliMock> dliMock;
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);

        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);

        TEST_DLI_Init();
        EXPECT_CALL(dliMock, DLI_CmdCbkReg).WillRepeatedly(TEST_DLI_CmdCbkReg);
        EXPECT_CALL(dliMock, DLI_CmdCbkUnReg).WillRepeatedly(TEST_DLI_CmdCbkUnReg);
        EXPECT_CALL(dliMock, DLI_SetMeasureParam).WillRepeatedly(TEST_DLI_SetMeasureParam);
        EXPECT_CALL(dliMock, DLI_SetMeasureEnable).WillRepeatedly(TEST_DLI_SetMeasureEnable);
        EXPECT_CALL(dliMock, DLI_ReadRemoteMeasureCaps).WillRepeatedly(TEST_DLI_ReadRemoteMeasureCaps);

        HadmInit();

        HadmSoundingCbk_S cbks = {0};
        cbks.controlResultCbk = STUB_HadmSoundingControlResultCbk;
        cbks.reportIqDataCbk = STUB_HadmSoundingIqCbk;
        cbks.soundingStateReportCbk = STUB_HadmSoundingStateReport;
        HadmRegCbk(&cbks);
    }
    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
        HadmDeInit();

        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
        TEST_DLI_DeInit();

        userErrorCode = NLSTK_ERRCODE_MAX;
    }

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {
    }
    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
    }
};

/**
 * @test HADM_SUCCESS
 * @brief 此测试用例用于验证HADM的正常流程。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 初始化CM链路和HADM链路
 * 2. CM通知读取远端feature，HADM触发读取remote cs caps
 * 3. 接收remote cs caps回复
 * 4. HADM收到remote cs caps回复
 * 5. 用户开启测距 
 * 6. 用户调用HadmStartSounding，HADM下发config update
 * 7. 收到connection update回复，HADM下发sounding config
 * 8. 收到config sounding回复，HADM下发sounding enable
 * 9. 收到enable回复
 * 10. 接收measure state和iq report事件，验证HADM的处理。
 * 11. 用户停止测距：
 * 12. 收到disable回复
 * 13. 模拟链路断开，设置链路状态为断开。
 *
 * @pre
 * - HADM模块初始化
 *
 * @post
 * - 每一步操作后，状态机的状态应与预期一致。
 *
 */
TEST_F(HADM_TEST, HADM_SUCCESS)
{
    // 模拟创建CM链路，HADM链路创建
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_HADM, &linkState);

    uint8_t state = HADM_SOUNDING_STOP;
    NLSTK_Errcode_E ret = HadmGetSoundingState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HADM_SOUNDING_STOP);
    // 查询当前HADM状态机状态，为IDLE
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_IDLE);
    // 查询正在测距的数量，为0
    SLE_Addr_S soundingAddr = {0};
    uint32_t soundingNum = HadmGetSoundingAddrInfo(&soundingAddr, 1);
    EXPECT_EQ(soundingNum, 0);

    // CM通知读取远端feature，HADM触发读取remote cs caps
    CM_LogicLinkRemoteFeatures_S feaures = {.lcid = g_lcid, .features = {0, 0, 0, 0, 0, 0, 0, 0, 4, 0}};
    TEST_CM_RemoteFeatureCbk(CM_MODULE_HADM, &feaures);
    ret = HadmGetSoundingState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HADM_SOUNDING_STOP);
    smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_IDLE);

    // HADM收到remote cs caps回复，状态机切到SOUNDING_READY状态
    DLI_ExecuteCmdRetParam remoteCsRetParam = {0};
    DLI_ReadRemoteCsCapsEvt caps = {.connHandle = g_lcid};
    remoteCsRetParam.eventParameter = &caps;
    TEST_DLI_EventCbk(DLI_CBK_READ_REMOTE_MEASURE_CAPS, 0, &remoteCsRetParam);
    ret = HadmGetSoundingState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HADM_SOUNDING_STOP);
    smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_SOUNDING_READY);

    // 用户开启测距，HADM下发config update, 状态机切到CONFIG_CONNECTION
    HadmConnectionParam_S connectionParam = {0};
    HadmSoundingParam_S soundParam = {0};
    ret = HadmStartSounding(&g_addr, &connectionParam, &soundParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    ret = HadmGetSoundingState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HADM_SOUNDING_START);
    smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_CONFIG_CONNECTION);

    // 收到connection update回复，HADM下发sounding config，状态机切到CONFIG_SOUNDING
    CM_LogicLinkConnUpdateParam_S connUpdateCbkParam = {0};
    connUpdateCbkParam.lcid = g_lcid;
    memcpy_s(&connUpdateCbkParam.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_ConnUpdateCbk(CM_MODULE_HADM, &connUpdateCbkParam);
    ret = HadmGetSoundingState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HADM_SOUNDING_START);
    smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_CONFIG_SOUNDING);

    // 收到config sounding回复，HADM下发sounding enable，状态机切到ENABLE_SOUNDING
    DLI_ExecuteCmdRetParam csConfigParam = {0};
    TEST_DLI_EventCbk(DLI_CBK_SET_MEASURE_CONFIG_PARAM, 0, &csConfigParam);
    ret = HadmGetSoundingState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HADM_SOUNDING_START);
    smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_ENABLE_SOUNDING);

    // 收到enable回复，状态机切到SOUNDING
    DLI_ExecuteCmdRetParam csEnableParam = {0};
    TEST_DLI_EventCbk(DLI_CBK_SET_MEASURE_EN, 0, &csEnableParam);
    ret = HadmGetSoundingState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HADM_SOUNDING_START);
    smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_SOUNDING);
    soundingNum = HadmGetSoundingAddrInfo(&soundingAddr, 1);
    EXPECT_EQ(soundingNum, 1);

    DLI_ExecuteCmdRetParam measureStateParam = {0};
    DLI_MeasureStateChangeEvt measureState = {0};
    measureStateParam.eventParameter = &measureState;
    TEST_DLI_EventCbk(DLI_CBK_MEASURE_STATE, 0, &measureStateParam);

    DLI_ExecuteCmdRetParam iqParam = {0};
    iqParam.eventParameter = g_localIqData;
    iqParam.size = HADM_MEASURE_IQ_REPORT_DATA_LEN;
    TEST_DLI_EventCbk(DLI_CBK_MEASURE_IQ_REPORT, 0, &iqParam);

    DLI_ExecuteCmdRetParam iqParam2 = {0};
    iqParam2.eventParameter = g_remoteIqData;
    iqParam2.size = HADM_MEASURE_IQ_REPORT_DATA_LEN;
    TEST_DLI_EventCbk(DLI_CBK_MEASURE_IQ_REPORT, 0, &iqParam2);

    // 用户停止测距，HADM下发disable，状态机切到DISABLE_SOUNDING
    HadmStopSounding(&g_addr);
    ret = HadmGetSoundingState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HADM_SOUNDING_STOP);
    smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_DISABLE_SOUNDING);

    // 收到disable回复，状态机切到SOUNDING_READY
    DLI_ExecuteCmdRetParam csDisableParam = {0};
    TEST_DLI_EventCbk(DLI_CBK_SET_MEASURE_EN, 0, &csDisableParam);
    ret = HadmGetSoundingState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HADM_SOUNDING_STOP);
    smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_SOUNDING_READY);

    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_HADM, &linkState2);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_IDLE_001)
{
    SetHadmSmToIdle();

    TriggerHadmUserStartEvent();
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_IDLE);
    HadmUserOperate_E operate = HadmGetUserOperate(&g_addr);
    EXPECT_EQ(operate, HADM_USER_START_SOUNDING);
    uint8_t state = HADM_SOUNDING_STOP;
    NLSTK_Errcode_E ret = HadmGetSoundingState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HADM_SOUNDING_START);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_IDLE_002)
{
    SetHadmSmToIdle();

    TriggerHadmUserStopEvent();
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_IDLE);
    HadmUserOperate_E operate = HadmGetUserOperate(&g_addr);
    EXPECT_EQ(operate, HADM_USER_INVALID_OPERATE);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_IDLE_003)
{
    SetHadmSmToIdle();

    TriggerDliReportSoundingResultEvent();
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_IDLE);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_IDLE_004)
{
    SetHadmSmToIdle();

    TriggerCmDisconnectEvent();
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_INVALID);
    HadmUserOperate_E operate = HadmGetUserOperate(&g_addr);
    EXPECT_EQ(operate, HADM_USER_INVALID_OPERATE);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_IDLE_005)
{
    SetHadmSmToIdle();

    DLI_ExecuteCmdRetParam remoteCsRetParam = {0};
    TEST_DLI_EventCbk(DLI_CBK_READ_REMOTE_MEASURE_CAPS, 0, &remoteCsRetParam);

    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_IDLE);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_IDLE_006)
{
    SetHadmSmToIdle();

    DLI_ExecuteCmdRetParam remoteCsRetParam = {0};
    DLI_ReadRemoteCsCapsEvt caps = {.connHandle = g_lcid};
    remoteCsRetParam.eventParameter = &caps;
    TEST_DLI_EventCbk(DLI_CBK_READ_REMOTE_MEASURE_CAPS, 1, &remoteCsRetParam);

    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_IDLE);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_IDLE_007)
{
    SetHadmSmToIdle();

    DLI_ExecuteCmdRetParam csConfigParam = {0};
    TEST_DLI_EventCbk(DLI_CBK_SET_MEASURE_CONFIG_PARAM, 0, &csConfigParam);

    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_IDLE);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_IDLE_008)
{
    SetHadmSmToIdle();
    TriggerHadmUserStartEvent();
    TriggerCmDisconnectEvent();

    EXPECT_EQ(userErrorCode, NLSTK_HADM_ERRCODE_LINK_EXCEPTION);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_IDLE_009)
{
    SetHadmSmToIdle();
    TriggerHadmUserStopEvent();
    TriggerCmDisconnectEvent();

    EXPECT_EQ(userErrorCode, NLSTK_ERRCODE_SUCCESS);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_IDLE_010)
{
    SetHadmSmToIdle();
    TriggerHadmUserStartEvent();

    TriggerCmUpdateConnParamEvent();
    HadmUserOperate_E operate = HadmGetUserOperate(&g_addr);
    EXPECT_EQ(operate, HADM_USER_START_SOUNDING);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_IDLE_011)
{
    SetHadmSmToIdle();
    TriggerHadmUserStartEvent();
    TriggerCmReportFeatureEvent();
    TriggerDliReportRemoteCsEvent();

    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_CONFIG_CONNECTION);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_READY_001)
{
    SetHadmSmToReady();

    TriggerCmDisconnectEvent();
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_INVALID);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_READY_002)
{
    SetHadmSmToReady();

    TriggerCmReportFeatureEvent();
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_SOUNDING_READY);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_CONFIG_CONN_001)
{
    SetHadmSmToConnectionConfig();

    TriggerCmDisconnectEvent();
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_INVALID);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_CONFIG_CONN_002)
{
    SetHadmSmToConnectionConfig();

    TriggerHadmUserStartEvent();
    HadmUserOperate_E operate = HadmGetUserOperate(&g_addr);
    EXPECT_EQ(operate, HADM_USER_INVALID_OPERATE);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_CONFIG_CONN_003)
{
    SetHadmSmToConnectionConfig();

    TriggerHadmUserStopEvent();
    HadmUserOperate_E operate = HadmGetUserOperate(&g_addr);
    EXPECT_EQ(operate, HADM_USER_STOP_SOUNDING);
    uint8_t state = HADM_SOUNDING_STOP;
    NLSTK_Errcode_E ret = HadmGetSoundingState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HADM_SOUNDING_STOP);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_CONFIG_CONN_004)
{
    SetHadmSmToConnectionConfig();

    CM_LogicLinkConnUpdateParam_S connUpdateCbkParam = {0};
    connUpdateCbkParam.lcid = g_lcid;
    connUpdateCbkParam.result = 1;
    memcpy_s(&connUpdateCbkParam.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_ConnUpdateCbk(CM_MODULE_HADM, &connUpdateCbkParam);

    EXPECT_EQ(userErrorCode, NLSTK_HADM_ERRCODE_CONFIG_CM_FAIL);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_CONFIG_CONN_005)
{
    SetHadmSmToConnectionConfig();
    TriggerHadmUserStopEvent();

    TriggerCmDisconnectEvent();
    EXPECT_EQ(userErrorCode, NLSTK_ERRCODE_SUCCESS);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_CONFIG_SOUNDING_001)
{
    SetHadmSmToConfigSounding();

    DLI_ExecuteCmdRetParam csConfigParam = {0};
    TEST_DLI_EventCbk(DLI_CBK_SET_MEASURE_CONFIG_PARAM, 1, &csConfigParam);

    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_SOUNDING_READY);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_CONFIG_SOUNDING_002)
{
    SetHadmSmToConfigSounding();
    TriggerHadmUserStopEvent();

    TriggerDliReportSoundingConfigEvent();
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_SOUNDING_READY);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_ENABLE_001)
{
    SetHadmSmToEnableSounding();

    DLI_ExecuteCmdRetParam csEnableParam = {0};
    TEST_DLI_EventCbk(DLI_CBK_SET_MEASURE_EN, 1, &csEnableParam);

    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_SOUNDING_READY);
    EXPECT_EQ(userErrorCode, NLSTK_HADM_ERRCODE_LINK_EXCEPTION);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_ENABLE_002)
{
    SetHadmSmToEnableSounding();
    TriggerHadmUserStopEvent();

    DLI_ExecuteCmdRetParam csEnableParam = {0};
    TEST_DLI_EventCbk(DLI_CBK_SET_MEASURE_EN, 1, &csEnableParam);

    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_SOUNDING_READY);
    EXPECT_EQ(userErrorCode, NLSTK_ERRCODE_SUCCESS);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_ENABLE_003)
{
    SetHadmSmToEnableSounding();
    TriggerHadmUserStopEvent();

    TriggerDliReportSoundingResultEvent();

    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_DISABLE_SOUNDING);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_ENABLE_004)
{
    SetHadmSmToEnableSounding();

    TriggerHadmUserStopEvent();
    HadmUserOperate_E operate = HadmGetUserOperate(&g_addr);
    EXPECT_EQ(operate, HADM_USER_STOP_SOUNDING);
    uint8_t state = HADM_SOUNDING_STOP;
    NLSTK_Errcode_E ret = HadmGetSoundingState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HADM_SOUNDING_START);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_DISABLE_001)
{
    SetHadmSmToDisableSounding();

    TriggerCmDisconnectEvent();
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_INVALID);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_DISABLE_002)
{
    SetHadmSmToDisableSounding();
    TriggerHadmUserStartEvent();

    TriggerCmDisconnectEvent();
    EXPECT_EQ(userErrorCode, NLSTK_HADM_ERRCODE_LINK_EXCEPTION);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_DISABLE_003)
{
    SetHadmSmToDisableSounding();

    DLI_ExecuteCmdRetParam csEnableParam = {0};
    TEST_DLI_EventCbk(DLI_CBK_SET_MEASURE_EN, 1, &csEnableParam);

    EXPECT_EQ(userErrorCode, NLSTK_HADM_ERRCODE_LINK_EXCEPTION);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_SM_DISABLE_004)
{
    SetHadmSmToDisableSounding();
    TriggerHadmUserStartEvent();

    TriggerDliReportSoundingResultEvent();
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_CONFIG_CONNECTION);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_REPORT_IQ_01)
{
    SetHadmSmToIdle();

    DLI_ExecuteCmdRetParam iqParam = {0};
    iqParam.eventParameter = g_localIqData;
    iqParam.size = HADM_MEASURE_IQ_REPORT_DATA_LEN;
    TEST_DLI_EventCbk(DLI_CBK_MEASURE_IQ_REPORT, 0, &iqParam);
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_IDLE);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_REPORT_IQ_02)
{
    SetHadmSmToIdle();
    TriggerCmDisconnectEvent();

    DLI_ExecuteCmdRetParam iqParam = {0};
    iqParam.eventParameter = g_localIqData;
    iqParam.size = HADM_MEASURE_IQ_REPORT_DATA_LEN;
    TEST_DLI_EventCbk(DLI_CBK_MEASURE_IQ_REPORT, 0, &iqParam);
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_INVALID);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_NOT_SUPPORT_SOUNDING_001)
{
    SetHadmSmToIdle();

    // 不支持测距
    CM_LogicLinkRemoteFeatures_S feaures = {.lcid = g_lcid, .features = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    TEST_CM_RemoteFeatureCbk(CM_MODULE_HADM, &feaures);
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_IDLE);
    HadmConnectionParam_S connectionParam = {0};
    HadmSoundingParam_S soundParam = {0};
    NLSTK_Errcode_E ret = HadmStartSounding(&g_addr, &connectionParam, &soundParam);
    EXPECT_EQ(ret, NLSTK_HADM_ERRCODE_PEER_NOT_SUPPORT_SOUNDING);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_NOT_SUPPORT_SOUNDING_002)
{
    SetHadmSmToIdle();
    TriggerHadmUserStartEvent();

    // 不支持测距
    CM_LogicLinkRemoteFeatures_S feaures = {.lcid = g_lcid, .features = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    TEST_CM_RemoteFeatureCbk(CM_MODULE_HADM, &feaures);
    HadmSoundingState_E smState = HadmGetSoundingStateByAddr(&g_addr);
    EXPECT_EQ(smState, HADM_SOUNDING_STATE_IDLE);
    HadmUserOperate_E operate = HadmGetUserOperate(&g_addr);
    EXPECT_EQ(operate, HADM_USER_INVALID_OPERATE);
}

TEST_F(HADM_TEST, HADM_EXCEPTION_NOT_SUPPORT_SOUNDING_003)
{
    SetHadmSmToIdle();
    TriggerHadmUserStartEvent();
    TriggerCmReportFeatureEvent();

    DLI_ExecuteCmdRetParam remoteCsRetParam = {0};
    DLI_ReadRemoteCsCapsEvt caps = {.connHandle = g_lcid};
    remoteCsRetParam.eventParameter = &caps;
    TEST_DLI_EventCbk(DLI_CBK_READ_REMOTE_MEASURE_CAPS, 1, &remoteCsRetParam);

    EXPECT_EQ(userErrorCode, NLSTK_HADM_ERRCODE_PEER_NOT_SUPPORT_SOUNDING);
}

TEST_F(HADM_TEST, HADM_USER_STOP_001)
{
    TriggerHadmUserStopEvent();

    EXPECT_EQ(userErrorCode, NLSTK_ERRCODE_SUCCESS);
}