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

#include "gtest/gtest.h"
#include "securec.h"
#include "sdf_mem.h"
#include "dli_event.h"
#include "dli_nbc_event.h"
#include "dli_secu_event.h"
#include "dli_hadm_event.h"
#include "dli_connect_event.h"
#include "dli_factory_event.h"
#include "dli_dev_discovery_event.h"

static volatile bool g_readRemoteMeasureCapsCbkCalled = false;
static void MockReadRemoteMeasureCapsCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    (void)status;
    (void)cmdRes;
    g_readRemoteMeasureCapsCbkCalled = true;
}

static void RegisterReadRemoteMeasureCapsCbk(void)
{
    DLI_CbkLineStru cbkTable[] = {
        { DLI_CBK_READ_REMOTE_MEASURE_CAPS, MockReadRemoteMeasureCapsCbk },
    };
    DLI_AddCbks(cbkTable, sizeof(cbkTable) / sizeof(DLI_CbkLineStru));
}

static void UnregisterReadRemoteMeasureCapsCbk(void)
{
    DLI_CbkLineStru cbkTable[] = {
        { DLI_CBK_READ_REMOTE_MEASURE_CAPS, NULL },
    };
    DLI_RemoveCbks(cbkTable, sizeof(cbkTable) / sizeof(DLI_CbkLineStru));
}

class UT_DLI_EVENT : public testing::Test {
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
        g_readRemoteMeasureCapsCbkCalled = false;
    }

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
        DLI_HadmEventSetIsSupportNewDisMeasure(NULL);
        UnregisterReadRemoteMeasureCapsCbk();
    }

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {}

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {}
};

TEST_F(UT_DLI_EVENT, DLI_SECU_EVENT)
{
    DLI_ManagerContext context = {0};
    DLI_EncryptChangeEvt arg = {0};
    EXPECT_NE(&context, nullptr);
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_EncryptParamReqCbk(&context, &arg, sizeof(DLI_EncryptParamReqEvt), 0x00);

    DLI_EncryptChangeCbk(&context, &arg, sizeof(DLI_EncryptChangeEvt), 0x00);

    DLI_ControllerDataEvt arg2 = {0};
    DLI_ControllerDataCbk(&context, &arg2, sizeof(DLI_ControllerDataEvt), 0x00);
}

TEST_F(UT_DLI_EVENT, DLI_NBC_EVENT)
{
    DLI_ManagerContext context = {0};
    DLI_EncryptChangeEvt arg = {0};
    EXPECT_NE(&context, nullptr);
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ChanInfoCbk(&context, &arg, sizeof(DLI_RssiEvt) + 1, 0x00);

    DLI_ChipResetNotifyCbk(&context, &arg, sizeof(DLI_ChipResetNotifyEvt), 0x00);
}

TEST_F(UT_DLI_EVENT, DLI_HADM_EVENT)
{
    DLI_ManagerContext context = {0};
    DLI_CsIqReportEvt arg = {0};
    EXPECT_NE(&context, nullptr);
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_CsIqReportCbk(&context, &arg, sizeof(DLI_CsIqReportEvt), 0x00);

    DLI_ReadRemoteCsCapsEvt arg2 = {0};
    DLI_ReadRemoteCsCapsCbk(&context, &arg2, sizeof(DLI_ReadRemoteCsCapsEvt), 0x00);

    DLI_MeasureStateChangeEvt arg3 = {0};
    DLI_MeasureStateChangeCbk(&context, &arg3, sizeof(DLI_MeasureStateChangeEvt), 0x00);
}

TEST_F(UT_DLI_EVENT, DLI_FACTORY_EVENT)
{
    DLI_ManagerContext context = {0};
    DLI_CsIqReportEvt arg = {0};
    EXPECT_NE(&context, nullptr);
    DLI_RfTxStartCbk(&context, &arg, sizeof(DLI_CsIqReportEvt), 0x00);
    DLI_RfResetCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsEvt), 0x00);
    DLI_RfTrxEndCbk(&context, &arg, sizeof(DLI_CsIqReportEvt), 0x00);
    DLI_RfRxStartCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsEvt), 0x00);
}

TEST_F(UT_DLI_EVENT, DLI_EVENT)
{
    DLI_ManagerContext context = {0};
    uint8_t data[100] = {0};
    RecvEventHandler(DLI_CMD_COMPLETE_EVT, &context, data, 0);
    RecvEventHandler(DLI_CMD_COMPLETE_EVT, &context, data, 0x02);
    RecvEventHandler(DLI_CMD_COMPLETE_EVT, &context, data, sizeof(DLI_CommandComplete));
    EXPECT_NE(&context, nullptr);
    DLI_NumberOfCompletedPacketsEvt arg = {0};
    DLI_NumberOfCompletedPacketsCbk(&context, &arg, sizeof(DLI_NumberOfCompletedPacketsEvt), 0x00);

    DLI_CommandStatus arg2 = {0};
    arg2.status = 0x01;
    DLI_CommandStatusCbk(&context, &arg2, sizeof(DLI_CommandStatus), 0x00);

    DLI_CommandErrorStru arg3 = {0};
    DLI_CommandErrorCbk(&context, &arg3, sizeof(DLI_CommandErrorStru), 0x00);
}

TEST_F(UT_DLI_EVENT, DLI_DEV_DISCOVERY_EVENT)
{
    DLI_ManagerContext context = {0};
    DLI_AdvReportEvt *arg = (DLI_AdvReportEvt *)malloc(sizeof(DLI_AdvReportEvt) + sizeof(uint8_t));
    memset_s(arg, sizeof(DLI_AdvReportEvt), 0x00, sizeof(DLI_AdvReportEvt));
    arg->data[0] = {0};
    arg->dataLength = 1;
    DLI_AdvReportCbk(&context, arg, sizeof(DLI_AdvReportEvt), 0x00);
    DLI_AdvTerminatedCbk(&context, arg, sizeof(DLI_AdvertisingTerminatedEvt), 0x00);
    EXPECT_NE(&context, nullptr);
    if (arg != NULL) {
        SDF_MemFree(arg);
    }
}

TEST_F(UT_DLI_EVENT, DLI_CONNECT_CBK)
{
    DLI_ManagerContext context = {0};
    DLI_SetPhyEvt *arg = (DLI_SetPhyEvt *)malloc(sizeof(DLI_SetPhyEvt));
    memset_s(arg, sizeof(DLI_SetPhyEvt), 0x00, sizeof(DLI_SetPhyEvt));
    DLI_DataLengthChangeCbk(&context, arg, sizeof(DLI_AdvReportEvt), 0x00);
    DLI_SetPhyCbk(&context, arg, sizeof(DLI_SetPhyEvt), 0x00);
    EXPECT_NE(&context, nullptr);
    DLI_ReadRemoteRssiEvt *arg1 = (DLI_ReadRemoteRssiEvt *)malloc(sizeof(DLI_ReadRemoteRssiEvt));
    memset_s(arg1, sizeof(DLI_ReadRemoteRssiEvt), 0x00, sizeof(DLI_ReadRemoteRssiEvt));
    DLI_SetPhyCbk(&context, arg1, sizeof(DLI_ReadRemoteRssiEvt), 0x00);

    DLI_AcbLowLatencyEnableEvt *arg2 = (DLI_AcbLowLatencyEnableEvt *)malloc(sizeof(DLI_AcbLowLatencyEnableEvt));
    memset_s(arg2, sizeof(DLI_AcbLowLatencyEnableEvt), 0x00, sizeof(DLI_AcbLowLatencyEnableEvt));
    DLI_AcbLowLatencyEnableCbk(&context, arg2, sizeof(DLI_AcbLowLatencyEnableEvt), 0x00);

    DLI_RemoteConnParamReqEvt *arg3 = (DLI_RemoteConnParamReqEvt *)malloc(sizeof(DLI_RemoteConnParamReqEvt));
    memset_s(arg3, sizeof(DLI_RemoteConnParamReqEvt), 0x00, sizeof(DLI_RemoteConnParamReqEvt));
    DLI_RemoteConnParamReqCbk(&context, arg3, sizeof(DLI_RemoteConnParamReqEvt), 0x00);

    DLI_ConnectionCompleteEvt *arg4 = (DLI_ConnectionCompleteEvt *)malloc(sizeof(DLI_ConnectionCompleteEvt));
    memset_s(arg4, sizeof(DLI_ConnectionCompleteEvt), 0x00, sizeof(DLI_ConnectionCompleteEvt));
    DLI_ConnectionCbk(&context, arg4, sizeof(DLI_ConnectionCompleteEvt), 0x00);

    DLI_DisconnectEvt *arg5 = (DLI_DisconnectEvt *)malloc(sizeof(DLI_DisconnectEvt));
    memset_s(arg5, sizeof(DLI_DisconnectEvt), 0x00, sizeof(DLI_DisconnectEvt));
    DLI_DisconnectionCbk(&context, arg5, sizeof(DLI_DisconnectEvt), 0x00);

    SDF_MemFree(arg);
    SDF_MemFree(arg1);
    SDF_MemFree(arg2);
    SDF_MemFree(arg3);
    SDF_MemFree(arg4);
    SDF_MemFree(arg5);
}

TEST_F(UT_DLI_EVENT, DLI_FEATURE_CBK)
{
    DLI_ManagerContext context = {0};
    DLI_ReadRemoteFeatsEvt *arg0 = (DLI_ReadRemoteFeatsEvt *)malloc(sizeof(DLI_ReadRemoteFeatsEvt));
    memset_s(arg0, sizeof(DLI_ReadRemoteFeatsEvt), 0x00, sizeof(DLI_ReadRemoteFeatsEvt));
    DLI_ReadRemoteFeaturesCbk(&context, arg0, sizeof(DLI_ReadRemoteFeatsEvt), 0x00);

    DLI_SetAcbEvtParamEvt *arg1 = (DLI_SetAcbEvtParamEvt *)malloc(sizeof(DLI_SetAcbEvtParamEvt));
    memset_s(arg1, sizeof(DLI_SetAcbEvtParamEvt), 0x00, sizeof(DLI_SetAcbEvtParamEvt));
    DLI_SetAcbEvtParamCbk(&context, arg1, sizeof(DLI_SetAcbEvtParamEvt), 0x00);

    DLI_ConnectionUpdateCmpEvt *arg2 = (DLI_ConnectionUpdateCmpEvt *)malloc(sizeof(DLI_ConnectionUpdateCmpEvt));
    memset_s(arg2, sizeof(DLI_ConnectionUpdateCmpEvt), 0x00, sizeof(DLI_ConnectionUpdateCmpEvt));
    DLI_ConnectionUpdateCbk(&context, arg2, sizeof(DLI_ConnectionUpdateCmpEvt), 0x00);

    DLI_ReadRemoteVersionEvt *arg3 = (DLI_ReadRemoteVersionEvt *)malloc(sizeof(DLI_ReadRemoteVersionEvt));
    memset_s(arg3, sizeof(DLI_ReadRemoteVersionEvt), 0x00, sizeof(DLI_ReadRemoteVersionEvt));
    DLI_ReadRemoteVersionCbk(&context, arg3, sizeof(DLI_ReadRemoteVersionEvt), 0x00);
    EXPECT_NE(&context, nullptr);

    SDF_MemFree(arg0);
    SDF_MemFree(arg1);
    SDF_MemFree(arg2);
    SDF_MemFree(arg3);
}

TEST_F(UT_DLI_EVENT, DLI_IOB_CBK)
{
    DLI_ManagerContext context = {0};
    DLI_ICBConnectReqEvt *arg0 = (DLI_ICBConnectReqEvt *)malloc(sizeof(DLI_ICBConnectReqEvt));
    memset_s(arg0, sizeof(DLI_ICBConnectReqEvt), 0x00, sizeof(DLI_ICBConnectReqEvt));
    DLI_IOBConnectReqCbk(&context, arg0, sizeof(DLI_ICBConnectReqEvt), 0x00);

    DLI_ICBEstablishedEvt *arg1 = (DLI_ICBEstablishedEvt *)malloc(sizeof(DLI_ICBEstablishedEvt));
    memset_s(arg1, sizeof(DLI_ICBEstablishedEvt), 0x00, sizeof(DLI_ICBEstablishedEvt));
    DLI_IOBEstablishedCbk(&context, arg1, sizeof(DLI_ICBEstablishedEvt), 0x00);

    DLI_IOBQualityReportEvt *arg2 = (DLI_IOBQualityReportEvt *)malloc(sizeof(DLI_IOBQualityReportEvt));
    memset_s(arg2, sizeof(DLI_IOBQualityReportEvt), 0x00, sizeof(DLI_IOBQualityReportEvt));
    DLI_IOBReportParamCbk(&context, arg2, sizeof(DLI_IOBQualityReportEvt), 0x00);
    EXPECT_NE(&context, nullptr);

    SDF_MemFree(arg0);
    SDF_MemFree(arg1);
    SDF_MemFree(arg2);
}

TEST_F(UT_DLI_EVENT, DLI_IOG_CBK)
{
    DLI_ManagerContext context = {0};
    DLI_ICGLabelReportEvt *arg0 = (DLI_ICGLabelReportEvt *)malloc(sizeof(DLI_ICGLabelReportEvt));
    memset_s(arg0, sizeof(DLI_ICGLabelReportEvt), 0x01, sizeof(DLI_ICGLabelReportEvt));
    DLI_IOGLabelReportCbk(&context, arg0, sizeof(DLI_ICGLabelReportEvt), 0x00);

    DLI_ICBParamUpdateEvt *arg1 = (DLI_ICBParamUpdateEvt *)malloc(sizeof(DLI_ICBParamUpdateEvt));
    memset_s(arg1, sizeof(DLI_ICBParamUpdateEvt), 0x00, sizeof(DLI_ICBParamUpdateEvt));
    DLI_IOGUpdateParamCbk(&context, arg1, sizeof(DLI_ICBParamUpdateEvt), 0x00);
    EXPECT_NE(&context, nullptr);

    SDF_MemFree(arg0);
    SDF_MemFree(arg1);
}

TEST_F(UT_DLI_EVENT, DLI_IMB_CBK)
{
    DLI_ManagerContext context = {0};
    DLI_ICBConnectReqEvt *arg0 = (DLI_ICBConnectReqEvt *)malloc(sizeof(DLI_ICBConnectReqEvt));
    memset_s(arg0, sizeof(DLI_ICBConnectReqEvt), 0x00, sizeof(DLI_ICBConnectReqEvt));
    DLI_IMBConnectReqCbk(&context, arg0, sizeof(DLI_ICBConnectReqEvt), 0x00);

    DLI_ICBEstablishedEvt *arg1 = (DLI_ICBEstablishedEvt *)malloc(sizeof(DLI_ICBEstablishedEvt));
    memset_s(arg1, sizeof(DLI_ICBEstablishedEvt), 0x00, sizeof(DLI_ICBEstablishedEvt));
    DLI_IMBEstablishedCbk(&context, arg1, sizeof(DLI_ICBEstablishedEvt), 0x00);

    DLI_IOBQualityReportEvt *arg2 = (DLI_IOBQualityReportEvt *)malloc(sizeof(DLI_IOBQualityReportEvt));
    memset_s(arg2, sizeof(DLI_IOBQualityReportEvt), 0x00, sizeof(DLI_IOBQualityReportEvt));
    DLI_IMBReportParamCbk(&context, arg2, sizeof(DLI_IOBQualityReportEvt), 0x00);
    EXPECT_NE(&context, nullptr);

    SDF_MemFree(arg0);
    SDF_MemFree(arg1);
    SDF_MemFree(arg2);
}

TEST_F(UT_DLI_EVENT, DLI_IMG_CBK)
{
    DLI_ManagerContext context = {0};
    DLI_ICGLabelReportEvt *arg0 = (DLI_ICGLabelReportEvt *)malloc(sizeof(DLI_ICGLabelReportEvt));
    memset_s(arg0, sizeof(DLI_ICGLabelReportEvt), 0x00, sizeof(DLI_ICGLabelReportEvt));
    DLI_IMGLabelReportCbk(&context, arg0, sizeof(DLI_ICGLabelReportEvt), 0x00);

    DLI_ICBParamUpdateEvt *arg1 = (DLI_ICBParamUpdateEvt *)malloc(sizeof(DLI_ICBParamUpdateEvt));
    memset_s(arg1, sizeof(DLI_ICBParamUpdateEvt), 0x00, sizeof(DLI_ICBParamUpdateEvt));
    DLI_IMGUpdateParamCbk(&context, arg1, sizeof(DLI_ICBParamUpdateEvt), 0x00);
    EXPECT_NE(&context, nullptr);

    SDF_MemFree(arg0);
    SDF_MemFree(arg1);
}

static bool MockIsSupportNewDisMeasure_ReturnTrue(void)
{
    return true;
}

static bool MockIsSupportNewDisMeasure_ReturnFalse(void)
{
    return false;
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_NullFuncFallback)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(NULL);
    RegisterReadRemoteMeasureCapsCbk();
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsEvt arg = {0};
    arg.status = 0x01;
    arg.connHandle = 0x02;
    DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsPrivEvt), 0x00);
    EXPECT_NE(&context, nullptr);
    EXPECT_FALSE(g_readRemoteMeasureCapsCbkCalled);
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_TrueFuncNewMeasure)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnTrue);
    RegisterReadRemoteMeasureCapsCbk();
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsPrivEvt arg = {0};
    arg.status = 0x00;
    arg.connHandle = 0x1234;
    arg.measureSignalCapabilitySupported = 1;
    DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsPrivEvt), 0x00);
    EXPECT_NE(&context, nullptr);
    EXPECT_TRUE(g_readRemoteMeasureCapsCbkCalled);
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_FalseFuncLegacyPath)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnFalse);
    RegisterReadRemoteMeasureCapsCbk();
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsEvt arg = {0};
    arg.status = 0x03;
    DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsPrivEvt), 0x00);
    EXPECT_NE(&context, nullptr);
    EXPECT_TRUE(g_readRemoteMeasureCapsCbkCalled);
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_NullFuncShortCircuit)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(NULL);
    RegisterReadRemoteMeasureCapsCbk();
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsEvt arg = {0};
    DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsEvt) - 1, 0x00);
    EXPECT_NE(&context, nullptr);
    EXPECT_FALSE(g_readRemoteMeasureCapsCbkCalled);
}

TEST_F(UT_DLI_EVENT, HadmEventSetIsSupportNewDisMeasure_RegisterAndClear)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnTrue);
    DLI_HadmEventSetIsSupportNewDisMeasure(NULL);
    RegisterReadRemoteMeasureCapsCbk();
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsEvt arg = {0};
    DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsPrivEvt), 0x00);
    EXPECT_NE(&context, nullptr);
    EXPECT_FALSE(g_readRemoteMeasureCapsCbkCalled);
}

static int32_t g_rangingTrueBranchData = 0;
static int32_t g_rangingFalseBranchData = 0;

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_TrueBranchFieldExtraction)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnTrue);
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsPrivEvt arg = {0};
    arg.status = 0x00;
    arg.connHandle = 0x5678;
    arg.measureSignalCapabilitySupported = 1;
    arg.multiAntennasSupported = 2;
    arg.type1MinTimeIp1 = 0x0A;
    DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsPrivEvt), 0x00);
    EXPECT_NE(&context, nullptr);
    DLI_HadmEventSetIsSupportNewDisMeasure(NULL);
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_FalseBranchMemcpyPath)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnFalse);
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsEvt arg = {0};
    arg.status = 0x02;
    arg.connHandle = 0xABCD;
    DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsEvt), 0x00);
    EXPECT_NE(&context, nullptr);
    DLI_HadmEventSetIsSupportNewDisMeasure(NULL);
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_DynamicSwitchTrueToFalse)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnTrue);
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsPrivEvt argTrue = {0};
    argTrue.status = 0x00;
    argTrue.connHandle = 0x1111;
    argTrue.measureSignalCapabilitySupported = 1;
    DLI_ReadRemoteCsCapsCbk(&context, &argTrue, sizeof(DLI_ReadRemoteCsCapsPrivEvt), 0x00);
    EXPECT_NE(&context, nullptr);

    DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnFalse);
    DLI_ReadRemoteCsCapsEvt argFalse = {0};
    argFalse.status = 0x03;
    argFalse.connHandle = 0x2222;
    DLI_ReadRemoteCsCapsCbk(&context, &argFalse, sizeof(DLI_ReadRemoteCsCapsEvt), 0x00);
    EXPECT_NE(&context, nullptr);
    DLI_HadmEventSetIsSupportNewDisMeasure(NULL);
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_DynamicSwitchFalseToTrue)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnFalse);
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsEvt argFalse = {0};
    argFalse.status = 0x04;
    DLI_ReadRemoteCsCapsCbk(&context, &argFalse, sizeof(DLI_ReadRemoteCsCapsEvt), 0x00);
    EXPECT_NE(&context, nullptr);

    DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnTrue);
    DLI_ReadRemoteCsCapsPrivEvt argTrue = {0};
    argTrue.status = 0x00;
    argTrue.connHandle = 0x3333;
    argTrue.measureSignalCapabilitySupported = 1;
    DLI_ReadRemoteCsCapsCbk(&context, &argTrue, sizeof(DLI_ReadRemoteCsCapsPrivEvt), 0x00);
    EXPECT_NE(&context, nullptr);
    DLI_HadmEventSetIsSupportNewDisMeasure(NULL);
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_DeinitPathNullFuncFallback)
{
    RegisterReadRemoteMeasureCapsCbk();
    DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnTrue);
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsPrivEvt arg = {0};
    arg.status = 0x00;
    arg.connHandle = 0x4444;
    g_readRemoteMeasureCapsCbkCalled = false;
    DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsPrivEvt), 0x00);
    EXPECT_NE(&context, nullptr);
    EXPECT_TRUE(g_readRemoteMeasureCapsCbkCalled);

    DLI_HadmEventSetIsSupportNewDisMeasure(NULL);
    DLI_ReadRemoteCsCapsEvt argFallback = {0};
    argFallback.status = 0x05;
    g_readRemoteMeasureCapsCbkCalled = false;
    DLI_ReadRemoteCsCapsCbk(&context, &argFallback, sizeof(DLI_ReadRemoteCsCapsEvt), 0x00);
    EXPECT_NE(&context, nullptr);
    EXPECT_FALSE(g_readRemoteMeasureCapsCbkCalled);
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_RepeatedRegistrationConsistent)
{
    for (int i = 0; i < 5; i++) {
        DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnTrue);
        DLI_ManagerContext context = {0};
        DLI_SmCbkContext cbkcontext = {0};
        context.cbkContext = &cbkcontext;
        DLI_ReadRemoteCsCapsPrivEvt arg = {0};
        arg.status = 0x00;
        arg.connHandle = 0x5555;
        DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsPrivEvt), 0x00);
        EXPECT_NE(&context, nullptr);
    }
    DLI_HadmEventSetIsSupportNewDisMeasure(NULL);
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_TrueFuncLenTooSmallEarlyReturn)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnTrue);
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsPrivEvt arg = {0};
    arg.status = 0x00;
    arg.connHandle = 0x6666;
    DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsPrivEvt) - 1, 0x00);
    EXPECT_NE(&context, nullptr);
    DLI_HadmEventSetIsSupportNewDisMeasure(NULL);
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_FalseFuncLenTooSmallEarlyReturn)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnFalse);
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsEvt arg = {0};
    arg.status = 0x07;
    DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsEvt) - 1, 0x00);
    EXPECT_NE(&context, nullptr);
    DLI_HadmEventSetIsSupportNewDisMeasure(NULL);
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_NullFuncEarlyReturnNoCallback)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(NULL);
    RegisterReadRemoteMeasureCapsCbk();
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsPrivEvt arg = {0};
    arg.status = 0x00;
    arg.connHandle = 0x7777;
    DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsPrivEvt), 0x00);
    EXPECT_FALSE(g_readRemoteMeasureCapsCbkCalled);
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_TrueFuncCallbackInvoked)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnTrue);
    RegisterReadRemoteMeasureCapsCbk();
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsPrivEvt arg = {0};
    arg.status = 0x00;
    arg.connHandle = 0x8888;
    DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsPrivEvt), 0x00);
    EXPECT_TRUE(g_readRemoteMeasureCapsCbkCalled);
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_FalseFuncCallbackInvoked)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(MockIsSupportNewDisMeasure_ReturnFalse);
    RegisterReadRemoteMeasureCapsCbk();
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsEvt arg = {0};
    arg.status = 0x09;
    DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsEvt), 0x00);
    EXPECT_TRUE(g_readRemoteMeasureCapsCbkCalled);
}

TEST_F(UT_DLI_EVENT, ReadRemoteCsCaps_NullFuncEarlyReturnBeforeLenCheck)
{
    DLI_HadmEventSetIsSupportNewDisMeasure(NULL);
    RegisterReadRemoteMeasureCapsCbk();
    DLI_ManagerContext context = {0};
    DLI_SmCbkContext cbkcontext = {0};
    context.cbkContext = &cbkcontext;
    DLI_ReadRemoteCsCapsEvt arg = {0};
    arg.status = 0x0A;
    DLI_ReadRemoteCsCapsCbk(&context, &arg, sizeof(DLI_ReadRemoteCsCapsEvt), 0x00);
    EXPECT_FALSE(g_readRemoteMeasureCapsCbkCalled);
}
