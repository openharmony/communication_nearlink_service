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

#include <cstdint>

#include "gtest/gtest.h"
#include "securec.h"
#include "cm_dli_adapter.h"
#include "cm_errno.h"
#include "collab_ext_func_wrapper.h"
#include "cp_worker.h"
#include "dli_callback.h"
#include "dli_layer_callback.h"
#include "dtap_channel.h"
#include "dtap_scheduler.h"
#include "dtap_errno.h"
#include "dli_layer_config.h"
#include "sdf_mem.h"
#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_dli_event_mock.h"
#include "stack_dli_event_stub.h"
#include "stack_dli_layer_mock.h"
#include "stack_dli_layer_stub.h"

static CM_DliCbk g_connectCbk = NULL;
static CM_DliCbk g_disconnectCbk = NULL;
static COLLAB_TransFuncExt g_collabTransFunc = {};
static DLI_DataNumChangecbk g_dliDataNumCbk = NULL;
static uint8_t g_connHandle1 = 1;
static uint8_t g_connHandle2 = 2;

extern "C" uint32_t CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE module, CM_DLI_ADAPTER_TYPE type, CM_DliCbk cbk)
{
    if (module != CM_DLI_ADAPTER_DTAP) {
        return CM_SUCCESS;
    }
    if (type == CM_DLI_ADAPTER_CONNECT) {
        g_connectCbk = cbk;
    } else if (type == CM_DLI_ADAPTER_DISCONNECT) {
        g_disconnectCbk = cbk;
    }
    return CM_SUCCESS;
}

extern "C" uint32_t CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE module, CM_DLI_ADAPTER_TYPE type)
{
    return CM_SUCCESS;
}

extern "C" uint32_t COLLAB_TransFuncRegister(const COLLAB_TransFuncExt *func)
{
    g_collabTransFunc.dliAcbNumChangeRegister = func->dliAcbNumChangeRegister;
    g_collabTransFunc.dliAcbNumGet = func->dliAcbNumGet;
    g_collabTransFunc.setApBufferNum = func->setApBufferNum;
    return 0;
}

extern "C" void DLI_DataNumChangeRegister(DLI_DataNumChangecbk cbk)
{
    g_dliDataNumCbk = cbk;
}

extern "C" void DLI_DataNumChange(DLI_DataType type, uint16_t dataNum)
{}

extern "C" uint32_t COLLAB_ContinueAssignTransBuffer(uint8_t apOccupiedBufferNum)
{
    return 0;
}

static void TEST_DtapConnectCbk(uint16_t connHandle)
{
    DLI_ConnectionCompleteEvt param = {0};
    param.status = 0;
    param.connHandle = connHandle;
    DLI_ExecuteCmdRetParam cmdRes = {};
    cmdRes.cmdOpcode = DLI_CREATE_CONNECTION;
    cmdRes.size = sizeof(param);
    cmdRes.eventParameter = &param;
    g_connectCbk(NULL, 0, &cmdRes);
}

static void TEST_DtapDisconnectCbk(uint16_t connHandle)
{
    DLI_DisconnectEvt param = {0};
    param.status = 0;
    param.connHandle = connHandle;
    DLI_ExecuteCmdRetParam cmdRes = {};
    cmdRes.cmdOpcode = DLI_DISCONNECT;
    cmdRes.size = sizeof(param);
    cmdRes.eventParameter = &param;
    g_disconnectCbk(NULL, 0, &cmdRes);
}

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

class UT_DTAP_SCHEDULER : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<DliEventMock> dliEventMock;
    NiceMock<DliLayerMock> dliLayerMock;
protected:
    // SetUP 在每一个用例测试开始前执行一次
    virtual void SetUp()
    {
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);

        TEST_DliEventInit();
        EXPECT_CALL(dliEventMock, DLI_RegNOCPEventCbk).WillRepeatedly(TEST_DLI_RegNOCPEventCbk);
        EXPECT_CALL(dliEventMock, DLI_UnregNOCPEventCbk).WillRepeatedly(TEST_DLI_UnregNOCPEventCbk);

        EXPECT_CALL(dliLayerMock, DLI_GetDataFragmentNums).WillRepeatedly(TEST_DLI_GetDataFragmentNumsStub);
        EXPECT_CALL(dliLayerMock, DLI_GetFragmentMaxLen).WillRepeatedly(TEST_DLI_GetFragmentMaxLenStub);
        EXPECT_CALL(dliLayerMock, DLI_SplitData).WillRepeatedly(TEST_DLI_SplitDataStub);
        EXPECT_CALL(dliLayerMock, DLI_DataSend).WillRepeatedly(TEST_DLI_DataSendStub);
    }

    // TearDown 在每一个用例测试完成后执行一次
    virtual void TearDown()
    {
        TEST_StackScheduleDeInit();
        TEST_DliEventDeInit();
    }

    // SetUpTestCase 在所有用例测试开始前执行一次
    static void SetUpTestCase()
    {
    }

    // TearDownTestCase 在所有用例测试完成后执行一次
    static void TearDownTestCase()
    {
    }
};

TEST_F(UT_DTAP_SCHEDULER, DTAP_SchedulerInitTest)
{
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

static uint16_t g_dliAcbNum = 0;
static void TEST_DliAcbNumChangeCbk(uint16_t dataNum)
{
    g_dliAcbNum = dataNum;
}

static void TEST_DtapSchedulerInit(void)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    EXPECT_NE(g_collabTransFunc.dliAcbNumChangeRegister, NULL);
    EXPECT_NE(g_collabTransFunc.dliAcbNumGet, NULL);
    EXPECT_NE(g_collabTransFunc.setApBufferNum, NULL);

    g_collabTransFunc.dliAcbNumChangeRegister(TEST_DliAcbNumChangeCbk);

    uint16_t dataNum = 8;
    g_dliDataNumCbk(ACB_DATA_TYPE, dataNum);
    EXPECT_EQ(g_dliAcbNum, dataNum);

    g_collabTransFunc.setApBufferNum(dataNum);

    TEST_DtapConnectCbk(g_connHandle1);
    TEST_DtapConnectCbk(g_connHandle2);
}

static void TEST_DtapSchedulerDeinit(void)
{
    TEST_DtapDisconnectCbk(g_connHandle1);
    TEST_DtapDisconnectCbk(g_connHandle2);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_DataSendWithPriorityNotInitTest)
{
    EXPECT_EQ(DTAP_DataSendWithPriority(NULL, NULL), DTAP_TRANS_INIT_ERR);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_DataSendWithPriorityNullTest)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_DataSendWithPriority(NULL, NULL), DTAP_TRANS_INVALID_PARAM_ERR);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_DataSendWithPriorityInvalidPriTest)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    DTAP_Channel_S channel;
    (void)memset_s(&channel, sizeof(DTAP_Channel_S), 0, sizeof(DTAP_Channel_S));
    channel.priority = DTAP_PRIORITY_MAX;
    SDF_Buff_S buff = {0};
    EXPECT_EQ(DTAP_DataSendWithPriority(&channel, &buff), DTAP_TRANS_INVALID_MODULE_TYPE);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

static DTAP_Channel_S *TEST_DtapChannelCreate(uint16_t connHandle, DTAP_ChannelPriority priority, uint8_t srcTcid)
{
    DTAP_Channel_S *channel = (DTAP_Channel_S *)SDF_MemZalloc(sizeof(DTAP_Channel_S));
    if (channel == NULL) {
        return NULL;
    }
    SDF_DListHeadInit(&channel->pktList);
    SDF_DListEntryInit(&channel->schedEntry);
    SDF_DListEntryInit(&channel->entry);
    channel->priority = priority;
    channel->lcid = connHandle;
    channel->srcTcid = srcTcid;
    return channel;
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_DataSendWithPrioritySuccessTest)
{
    TEST_DtapSchedulerInit();

    DTAP_Channel_S *channel = TEST_DtapChannelCreate(g_connHandle1, DTAP_PRIORITY_CMD, 0);
    EXPECT_NE(channel, NULL);

    DLI_AllDataSet(100, 8, 0, 0);
    SDF_Buff_S *buf1 = SDF_BuffNewWithReserve(20000);
    EXPECT_NE(buf1, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf1, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(channel, buf1), DTAP_SUCCESS);
    // same lcid and tcid
    SDF_Buff_S *buf2 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf2, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf2, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(channel, buf2), DTAP_SUCCESS);
    // same lcid and different tcid
    DTAP_Channel_S *channel2 = TEST_DtapChannelCreate(g_connHandle1, DTAP_PRIORITY_CMD, 1);
    EXPECT_NE(channel2, NULL);
    SDF_Buff_S *buf3 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf3, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf3, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(channel2, buf3), DTAP_SUCCESS);
    // different lcid
    DTAP_Channel_S *channel3 = TEST_DtapChannelCreate(g_connHandle2, DTAP_PRIORITY_CMD, 0);
    EXPECT_NE(channel3, NULL);
    SDF_Buff_S *buf4 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf4, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf4, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(channel3, buf4), DTAP_SUCCESS);

    DLI_AllDataSet(100, 4, 0, 0);
    EXPECT_EQ(TEST_NOCPEventDo(DLI_REG_MODULE_DTAP, 0, 1), DTAP_SUCCESS);
    TEST_DtapSchedulerDeinit();

    SDF_MemFree(channel);
    SDF_MemFree(channel2);
    SDF_MemFree(channel3);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_DataSendWithPrioritySingleNumTest)
{
    TEST_DtapSchedulerInit();
    DLI_AllDataSet(1, 1, 0, 0);
    DTAP_Channel_S *channel = TEST_DtapChannelCreate(g_connHandle1, DTAP_PRIORITY_HIGH, 0);
    EXPECT_NE(channel, NULL);
    SDF_Buff_S *buf = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf, 100), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(channel, buf), DTAP_SUCCESS);
    TEST_DtapSchedulerDeinit();

    SDF_MemFree(channel);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_DataSendWithPrioritySendFailedTest)
{
    TEST_DtapSchedulerInit();
    DTAP_Channel_S channel;
    (void)memset_s(&channel, sizeof(DTAP_Channel_S), 0, sizeof(DTAP_Channel_S));
    SDF_DListHeadInit(&channel.pktList);
    SDF_DListEntryInit(&channel.schedEntry);
    SDF_DListEntryInit(&channel.entry);
    channel.priority = DTAP_PRIORITY_CMD;
    channel.lcid = 0xffff;

    DLI_AllDataSet(100, 1, 0, 0);
    SDF_Buff_S *buf = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(&channel, buf), DTAP_SUCCESS);

    TEST_DtapSchedulerDeinit();
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_ChannelDownTest)
{
    TEST_DtapSchedulerInit();
    DLI_AllDataSet(100, 0, 0, 0);
    DTAP_Channel_S *channel = TEST_DtapChannelCreate(g_connHandle1, DTAP_PRIORITY_CMD, 0);
    EXPECT_NE(channel, NULL);
    SDF_Buff_S *buf = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf, 100), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(channel, buf), DTAP_SUCCESS);

    DTAP_Channel_S *channel2 = TEST_DtapChannelCreate(g_connHandle1, DTAP_PRIORITY_CMD, 1);
    EXPECT_NE(channel2, NULL);
    SDF_Buff_S *buf2 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf2, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf2, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(channel2, buf2), DTAP_SUCCESS);
    DTAP_ChannelDown(g_connHandle2, 0);
    DTAP_ChannelDown(g_connHandle1, 2);
    DTAP_ChannelDown(g_connHandle1, 0);
    DTAP_ChannelDown(g_connHandle1, 1);
    TEST_DtapSchedulerDeinit();

    SDF_MemFree(channel);
    SDF_MemFree(channel2);
}
