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

class UT_DLI_EVENT : public testing::Test {
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {}

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {}

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
