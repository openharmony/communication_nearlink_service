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

#include "sdf_buff.h"
#include "cm_errno.h"
#include "cm_icb_init.h"
#include "cm_icb_api.h"
#include "cm_icb_inner_api.h"
#include "cm_api.h"
#include "cm_dli_adapter.h"
#include "cm_trans_channel_api.h"
#include "cp_worker.h"
#include "dli_callback.h"
#include "dli_event.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"

static void Test_CM_ICBConnectionStatusCbk(ICBConnectionType type, CM_TransChan_S *trans)
{
    return;
}

static void TEST_FreqBandListener(CM_FreqBandSwitchParam *param)
{
    return;
}

static void TEST_Connection(CM_ICBConnection *connection)
{
    return;
}

static void TEST_LabelReport(CM_ICBLabelReportParam *param)
{
    return;
}

static void TEST_Quality(CM_ICBQuality *quality)
{
    return;
}

static uint32_t TEST_DLI_EventCbkWithContext(
    uint16_t opcode, void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    DLI_ExecuteCmdCbk cbk = DLI_GetCbk(opcode);
    if (cbk == NULL) {
        return 1;
    }
    cbk(context, status, cmdRes);
    return 0;
}

static void TEST_DliRecvAcbHandler(uint16_t lcid, SDF_Buff_S *buf)
{
    (void)lcid;
    (void)buf;
}

static void TEST_DliInit(void)
{
    DLI_Callback dliCallback = {0};
    dliCallback.postOtherThread = CP_PostTask;
    dliCallback.postOtherBlockedThread = CP_PostTaskBlocked;
    dliCallback.dftReportKill = NULL;
    dliCallback.recvAcbHandler = TEST_DliRecvAcbHandler;
    DLI_SetCallback(&dliCallback);
}

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

class UT_CM_ICB_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    // NiceMock<DliMock> dliMock;
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
        CM_ICBCallback cb = {
            .connectionCbk = TEST_Connection,
            .labelReportCbk = TEST_LabelReport,
            .qualityReportCbk = TEST_Quality
        };
        TEST_DliInit();
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerAdd).WillRepeatedly(TEST_ScheduleTimerAddStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerDel).WillRepeatedly(TEST_ScheduleTimerDelStub);
        CM_DliAdapterInit();
        CM_ICBInit();
        CM_ICBEnable();
        CM_ICBRegisterCbk(&cb);
        CM_RegisterICBConnectionCbk(Test_CM_ICBConnectionStatusCbk);
    }

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
        CM_UnregisterICBConnectionCbk();
        CM_ICBUnregisterCbk();
        CM_ICBDisable();
        CM_ICBDeinit();
        CM_DliAdapterDeinit();
        TEST_StackScheduleDeInit();
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

TEST_F(UT_CM_ICB_TEST, CM_ICGSetParam)
{
    CM_ICGParam *icgParam = (CM_ICGParam *)SDF_MemZalloc(sizeof(CM_ICGParam));
    EXPECT_NE(icgParam, nullptr);
    CM_ICBParam *icbParam = (CM_ICBParam *)SDF_MemZalloc(sizeof(CM_ICBParam));
    EXPECT_NE(icbParam, nullptr);
    CM_ICBParam::CM_ICB ICB;
    ICB.maxSduG2T = 0x00;
    ICB.maxSduT2G = 0x00;
    ICB.rtnG2T = 0x00;
    ICB.rtnT2G = 0x00;
    icbParam->param = &ICB;
    icgParam->icbParam = icbParam;
    icgParam->icbCnt = 1;
    icgParam->paramCnt = 1;
    EXPECT_NE(CM_ICGSetParam(icgParam), CM_SUCCESS);
    SDF_MemFree(icgParam);
    SDF_MemFree(icbParam);
}

TEST_F(UT_CM_ICB_TEST, CM_ICGSetTestParam)
{
    CM_ICGTestParam *icgParam = (CM_ICGTestParam *)SDF_MemZalloc(sizeof(CM_ICGTestParam));
    EXPECT_NE(icgParam, nullptr);
    CM_ICBTestParam *icbParam = (CM_ICBTestParam *)SDF_MemZalloc(sizeof(CM_ICBTestParam));
    EXPECT_NE(icbParam, nullptr);
    icgParam->icbParam = icbParam;
    icgParam->icbCnt = 1;
    // ext wrapper func is null, return 0
    EXPECT_EQ(CM_ICGSetTestParam(icgParam, true), CM_SUCCESS);
    EXPECT_NE(CM_ICGSetTestParam(icgParam, false), CM_SUCCESS);
    SDF_MemFree(icgParam);
    SDF_MemFree(icbParam);
}

TEST_F(UT_CM_ICB_TEST, CM_ICGRemoveParam)
{
    CM_ICGRemovedParam icgParam = {};
    icgParam.type = CM_IOB;
    icgParam.id = 0x00;
    EXPECT_NE(CM_ICGRemoveParam(&icgParam), CM_SUCCESS);
    DLI_ICGCbkParam context = { 0 };
    DLI_ExecuteCmdRetParam cmdRes = { 0 };
    DLI_SetIOGParamEvt *eventParameter1 =
        (DLI_SetIOGParamEvt *)SDF_MemAlloc(sizeof(DLI_SetIOGParamEvt) + sizeof(uint16_t));
    EXPECT_NE(eventParameter1, nullptr);
    eventParameter1->connHandle[0] = { 0 };
    cmdRes.eventParameter = eventParameter1;
    DLI_ExecuteCmdCbk cbk1 = DLI_GetCbk(DLI_CBK_SET_IOG_PARAM);
    eventParameter1->paramCnt = 1;
    cbk1(&context, 0x00, &cmdRes);

    EXPECT_NE(CM_ICGRemoveParam(&icgParam), CM_SUCCESS);
    SDF_MemFree(eventParameter1);
}

TEST_F(UT_CM_ICB_TEST, CM_ICGSetLabel)
{
    DLI_ICGCbkParam context = { 0 };
    DLI_ExecuteCmdRetParam cmdRes = { 0 };
    DLI_SetIOGParamEvt *eventParameter1 =
        (DLI_SetIOGParamEvt *)SDF_MemAlloc(sizeof(DLI_SetIOGParamEvt) + sizeof(uint16_t));
    EXPECT_NE(eventParameter1, nullptr);
    eventParameter1->connHandle[0] = { 0 };
    cmdRes.eventParameter = eventParameter1;
    DLI_ExecuteCmdCbk cbk1 = DLI_GetCbk(DLI_CBK_SET_IOG_PARAM);
    eventParameter1->paramCnt = 1;

    CM_ICGLabelParam icgLabel;
    icgLabel.type = CM_IOB;
    icgLabel.id = 0;
    CM_ICBChannel icg = { 0 };
    EXPECT_EQ(CM_ICGSetLabel(&icgLabel, true, true), CM_INVALID_PARAM_ERR);
    icgLabel.icbCnt = 1;
    icgLabel.icb = &icg;
    EXPECT_NE(CM_ICGSetLabel(&icgLabel, false, false), CM_SUCCESS);
    EXPECT_NE(CM_ICGSetLabel(&icgLabel, true, true), CM_SUCCESS);
    cbk1(&context, 0x00, &cmdRes);
    EXPECT_EQ(CM_ICGSetLabel(&icgLabel, true, false), CM_SUCCESS);
    SDF_MemFree(eventParameter1);
}

TEST_F(UT_CM_ICB_TEST, CM_ICBAddConnection)
{
    DLI_ICGCbkParam context = { 0 };
    DLI_ExecuteCmdRetParam cmdRes = { 0 };
    DLI_SetIOGParamEvt *eventParameter1 =
        (DLI_SetIOGParamEvt *)SDF_MemAlloc(sizeof(DLI_SetIOGParamEvt) + sizeof(uint16_t));
    EXPECT_NE(eventParameter1, nullptr);
    eventParameter1->connHandle[0] = { 0 };
    cmdRes.eventParameter = eventParameter1;
    DLI_ExecuteCmdCbk cbk1 = DLI_GetCbk(DLI_CBK_SET_IOG_PARAM);
    eventParameter1->paramCnt = 1;
    cbk1(&context, 0x00, &cmdRes);

    CM_ICBConnectionParam icgLabel = {};
    CM_ICBChannel channel = { 0 };
    icgLabel.id = 0x00;
    icgLabel.type = CM_IOB;
    icgLabel.channelCnt = 1;
    icgLabel.channel = &channel;
    // ext wrapper fun is null, return 0
    EXPECT_EQ(CM_ICBAddConnection(&icgLabel, true), CM_SUCCESS);
    EXPECT_NE(CM_ICBAddConnection(&icgLabel, false), CM_SUCCESS);
    SDF_MemFree(eventParameter1);
}

TEST_F(UT_CM_ICB_TEST, CM_ICBDelConnection)
{
    CM_ICBConnectionParam *icgLabel = (CM_ICBConnectionParam *)SDF_MemZalloc(sizeof(CM_ICBConnectionParam));
    EXPECT_NE(icgLabel, nullptr);
    CM_ICBChannel *channel = (CM_ICBChannel *)SDF_MemZalloc(sizeof(CM_ICBChannel));
    EXPECT_NE(channel, nullptr);
    icgLabel->id = 0x00;
    icgLabel->channelCnt = 1;
    icgLabel->channel = channel;
    icgLabel->type = CM_IOB;
    EXPECT_NE(CM_ICBDelConnection(icgLabel), CM_SUCCESS);
    SDF_MemFree(icgLabel);
    SDF_MemFree(channel);
}

TEST_F(UT_CM_ICB_TEST, CM_ICGUpdateParam)
{
    CM_ICGUpdatedParam *icgLabel = (CM_ICGUpdatedParam *)SDF_MemZalloc(sizeof(CM_ICGUpdatedParam));
    EXPECT_NE(icgLabel, nullptr);
    uint16_t connectHandle[1] = { 0 };
    memset_s(icgLabel, sizeof(CM_ICGUpdatedParam), 0x00, sizeof(CM_ICGUpdatedParam));
    icgLabel->id = 0x00;
    icgLabel->icbCnt = 1;
    icgLabel->connHandle = connectHandle;
    icgLabel->type = CM_IOB;
    EXPECT_NE(CM_ICGUpdateParam(icgLabel), CM_SUCCESS);
    SDF_MemFree(icgLabel);
}

TEST_F(UT_CM_ICB_TEST, CM_ICBSetupDataPath)
{
    CM_ICBDataPath *icgLabel = (CM_ICBDataPath *)SDF_MemZalloc(sizeof(CM_ICBDataPath));
    EXPECT_NE(icgLabel, nullptr);
    uint8_t codecConfigData[1] = { 0 };
    icgLabel->codecConfigData = codecConfigData;
    EXPECT_NE(CM_ICBSetupDataPath(icgLabel), CM_SUCCESS);
    SDF_MemFree(icgLabel);
}

TEST_F(UT_CM_ICB_TEST, CM_ICBRemoveDataPath)
{
    CM_ICBRemovedDataPath *dataPath = (CM_ICBRemovedDataPath *)SDF_MemZalloc(sizeof(CM_ICBRemovedDataPath));
    EXPECT_NE(dataPath, nullptr);
    EXPECT_NE(CM_ICBRemoveDataPath(dataPath), CM_SUCCESS);
    SDF_MemFree(dataPath);
}

TEST_F(UT_CM_ICB_TEST, CM_ListenFreqBandSwitchEvent)
{
    EXPECT_EQ(CM_ListenFreqBandSwitchEvent(TEST_FreqBandListener), CM_SUCCESS);
    EXPECT_EQ(CM_UnlistenFreqBandSwitchEvent(TEST_FreqBandListener), CM_SUCCESS);
}

TEST_F(UT_CM_ICB_TEST, CM_RegisterICBConnectionCbk)
{
    EXPECT_EQ(CM_RegisterICBConnectionCbk(Test_CM_ICBConnectionStatusCbk), CM_SUCCESS);
    EXPECT_EQ(CM_UnregisterICBConnectionCbk(), CM_SUCCESS);
}

TEST_F(UT_CM_ICB_TEST, CM_ICGRemoveParamCbk)
{
    DLI_ICGCbkParam param;
    DLI_ExecuteCmdRetParam retParam;
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_REMOVE_ICG_PARAM, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_REMOVE_ICG_PARAM, (void *)&param, 0, &retParam), 0);

    DLI_RemoveICGParamEvt evt = {.id = 1};
    retParam.eventParameter = (void *)&evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_REMOVE_ICG_PARAM, (void *)&param, 0, &retParam), 0);
    evt.id = 0;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_REMOVE_ICG_PARAM, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_IOGSetParamCbk)
{
    uint8_t tmp[100];
    DLI_ICGCbkParam param;
    DLI_ExecuteCmdRetParam retParam;
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SET_IOG_PARAM, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SET_IOG_PARAM, (void *)&param, 0, &retParam), 0);

    DLI_SetIOGParamEvt *evt = (DLI_SetIOGParamEvt *)tmp;
    retParam.eventParameter = (void *)evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SET_IOG_PARAM, (void *)&param, 0, &retParam), 0);
    evt->paramCnt = 1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SET_IOG_PARAM, (void *)&param, 0, &retParam), 0);

    DLI_RemoveICGParamEvt evt1 = {.id = 0};
    retParam.eventParameter = (void *)&evt1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_REMOVE_ICG_PARAM, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_IOGLabelReportCbk)
{
    uint8_t tmp[100];
    DLI_ICGLabelReportEvt *reportEvt = (DLI_ICGLabelReportEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    DLI_SetIOGParamEvt *evt = (DLI_SetIOGParamEvt *)tmp;
    retParam.eventParameter = (void *)evt;
    evt->paramCnt = 1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SET_IOG_PARAM, (void *)&param, 0, &retParam), 0);

    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = reportEvt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IOG_LABEL_REPORT, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IOG_LABEL_REPORT, (void *)&param, 0, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IOG_LABEL_REPORT, (void *)&param, 0, &retParam), 0);
    reportEvt->labelCnt = 1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IOG_LABEL_REPORT, (void *)&param, 0, &retParam), 0);
    reportEvt->connHandle = 1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IOG_LABEL_REPORT, (void *)&param, 0, &retParam), 0);

    DLI_RemoveICGParamEvt evt1 = {.id = 0};
    retParam.eventParameter = (void *)&evt1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_REMOVE_ICG_PARAM, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_IOBEstablishedCbk)
{
    uint8_t tmp[100];
    DLI_ICBEstablishedEvt *evt = (DLI_ICBEstablishedEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    DLI_SetIOGParamEvt *setEvt = (DLI_SetIOGParamEvt *)tmp;
    retParam.eventParameter = (void *)setEvt;
    setEvt->paramCnt = 1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SET_IOG_PARAM, (void *)&param, 0, &retParam), 0);

    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IOB_ESTABLISHED, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IOB_ESTABLISHED, (void *)&param, 0, &retParam), 0);

    DLI_RemoveICGParamEvt evt1 = {.id = 0};
    retParam.eventParameter = (void *)&evt1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_REMOVE_ICG_PARAM, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_IOBConnectReqCbk)
{
    uint8_t tmp[100];
    DLI_ICBConnectReqEvt *evt = (DLI_ICBConnectReqEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IOB_CONNECT_REQ, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IOB_CONNECT_REQ, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_IOBQualityReportCbk)
{
    uint8_t tmp[100];
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    DLI_IOBQualityReportEvt *evt = (DLI_IOBQualityReportEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IOB_REPORT_PARAM, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_IOGParamUpdateCbk)
{
    uint8_t tmp[100];
    DLI_ICBParamUpdateEvt *evt = (DLI_ICBParamUpdateEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    DLI_SetIOGParamEvt *setEvt = (DLI_SetIOGParamEvt *)tmp;
    retParam.eventParameter = (void *)setEvt;
    setEvt->paramCnt = 1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SET_IOG_PARAM, (void *)&param, 0, &retParam), 0);

    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IOG_PARAM_UPDATE, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IOG_PARAM_UPDATE, (void *)&param, 0, &retParam), 0);
    evt->connHandle = 1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IOG_PARAM_UPDATE, (void *)&param, 0, &retParam), 0);
    DLI_RemoveICGParamEvt evt1 = {.id = 0};
    retParam.eventParameter = (void *)&evt1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_REMOVE_ICG_PARAM, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_IOBRejectReqCbk)
{
    uint8_t tmp[100];
    DLI_ICBRejectReqEvt *evt = (DLI_ICBRejectReqEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_REJECT_IOB_REQ, (void *)&param, 0, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_REJECT_IOB_REQ, (void *)&param, 1, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_IMGSetParamCbk)
{
    uint8_t tmp[100];
    DLI_SetIMGParamEvt *evt = (DLI_SetIMGParamEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SET_IMG_PARAM, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SET_IMG_PARAM, NULL, 0, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SET_IMG_PARAM, (void *)&param, 0, &retParam), 0);
    evt->paramCnt = 1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SET_IMG_PARAM, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_IMGLabelReportCbk)
{
    uint8_t tmp[100];
    DLI_ICGLabelReportEvt *evt = (DLI_ICGLabelReportEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));

    DLI_SetIMGParamEvt *evt1 = (DLI_SetIMGParamEvt *)tmp;
    retParam.eventParameter = evt1;
    evt1->paramCnt = 1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SET_IMG_PARAM, (void *)&param, 0, &retParam), 0);
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMG_LABEL_REPORT, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMG_LABEL_REPORT, NULL, 0, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMG_LABEL_REPORT, (void *)&param, 0, &retParam), 0);
    evt->labelCnt = 1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMG_LABEL_REPORT, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_IMBEstablishedCbk)
{
    uint8_t tmp[100];
    DLI_ICBEstablishedEvt *evt = (DLI_ICBEstablishedEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));

    DLI_SetIMGParamEvt *evt1 = (DLI_SetIMGParamEvt *)tmp;
    retParam.eventParameter = evt1;
    evt1->paramCnt = 1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SET_IMG_PARAM, (void *)&param, 0, &retParam), 0);
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMB_ESTABLISHED, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMB_ESTABLISHED, NULL, 0, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMB_ESTABLISHED, (void *)&param, 0, &retParam), 0);
    evt->connHandle = 1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMB_ESTABLISHED, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_IMBConnectReqCbk)
{
    uint8_t tmp[100];
    DLI_ICBConnectReqEvt *evt = (DLI_ICBConnectReqEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));

    DLI_SetIMGParamEvt *evt1 = (DLI_SetIMGParamEvt *)tmp;
    retParam.eventParameter = evt1;
    evt1->paramCnt = 1;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SET_IMG_PARAM, (void *)&param, 0, &retParam), 0);
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMB_CONNECT_REQ, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMB_CONNECT_REQ, NULL, 0, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMB_CONNECT_REQ, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_IMBQualityReportCbk)
{
    uint8_t tmp[100];
    DLI_IOBQualityReportEvt *evt = (DLI_IOBQualityReportEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMB_REPORT_PARAM, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMB_REPORT_PARAM, NULL, 0, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMB_REPORT_PARAM, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_IMGParamUpdateCbk)
{
    uint8_t tmp[100];
    DLI_ICBParamUpdateEvt *evt = (DLI_ICBParamUpdateEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMG_PARAM_UPDATE, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_IMG_PARAM_UPDATE, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_IMBRejectReqCbk)
{
    uint8_t tmp[100];
    DLI_ICBRejectReqEvt *evt = (DLI_ICBRejectReqEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_REJECT_IMB_REQ, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_REJECT_IMB_REQ, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_ICBSetDataPathCbk)
{
    uint8_t tmp[100];
    DLI_ICBRejectReqEvt *evt = (DLI_ICBRejectReqEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_ICB_SETUP_DATA_PATH, NULL, 0, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_ICB_SETUP_DATA_PATH, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_ICB_SETUP_DATA_PATH, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_ICBRemoveDataPathCbk)
{
    uint8_t tmp[100];
    DLI_ICBDataPathEvt *evt = (DLI_ICBDataPathEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_ICB_REMOVE_DATA_PATH, NULL, 0, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_ICB_REMOVE_DATA_PATH, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_ICB_REMOVE_DATA_PATH, (void *)&param, 0, &retParam), 0);
}

TEST_F(UT_CM_ICB_TEST, CM_FreqBandSwitchCbk)
{
    uint8_t tmp[100];
    DLI_FreqBandSwitchEvt *evt = (DLI_FreqBandSwitchEvt *)tmp;
    DLI_ExecuteCmdRetParam retParam;
    DLI_ICGCbkParam param;
    (void)memset_s(&param, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    (void)memset_s(&retParam, sizeof(DLI_ExecuteCmdRetParam), 0, sizeof(DLI_ExecuteCmdRetParam));
    (void)memset_s(tmp, sizeof(tmp), 0, sizeof(tmp));
    retParam.eventParameter = (void *)evt;
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SWITCH_FREQ_BAND, NULL, 0, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SWITCH_FREQ_BAND, (void *)&param, 1, &retParam), 0);
    EXPECT_EQ(TEST_DLI_EventCbkWithContext(DLI_CBK_SWITCH_FREQ_BAND, (void *)&param, 0, &retParam), 0);
}