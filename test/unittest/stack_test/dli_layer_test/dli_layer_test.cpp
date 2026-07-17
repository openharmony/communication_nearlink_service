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

#include "dpfwk_errcode.h"
#include "dpfwk_log.h"
#include "sdf_evc.h"
#include "sdf_timer.h"
#include "sdf_event.h"
#include "sdf_worker.h"
#include "sdf_thread.h"
#include "dli_thread.h"
#include "dli_log.h"
#include "dli_opcode.h"
#include "dli_sapi.h"
#include "dli_layer.h"
#include "dli_errno.h"
#include "dli_layer_config.h"
#include "dli_layer_utils.h"
#include "dli_layer_callback.h"
#include "dli_cmd_struct.h"

#define DATA_LEN 10
#define DATA_NUM 10
#define DATA_LEN 10
#define DATA_MAX_LEN 11
#define DATA_FRAGMENTS_LEN 3
#define TEST_CMD_CNT1 1
#define DLI_TEST_SLEEP_TIME 100 /* 100ms */
#define TEST_DATA_CNT 1
static volatile int g_count = 0;

class UT_DLI_LAYER_TEST : public testing::Test {
  protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    void SetUp()
    {
        g_count = 0;
    }

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
    }

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {
        uint32_t ret = SDF_EvcInit();
        EXPECT_EQ(ret, 0);
        ret = SDF_ThreadInit(DATA_NUM);
        EXPECT_EQ(ret, 0);
        DLI_LOGI("UT_DLI_LAYER_TESTTEST SetUpTestCase end");
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
        SDF_ThreadDeinit();
        SDF_EvcDeinit();
    }
};

static void TestCaseAddWorkCallback(void *arg)
{
    uint8_t *count = (uint8_t*)arg;
    g_count = *count;
}

static void TEST_received(SlePacketType type, const SlePacket *packet)
{
    (void)type;
    (void)packet;
}

void TEST_acbLogStub(uint16_t lcid, SDF_Buff_S *buf)
{
    (void)lcid;
    (void)buf;
}

void ContextFreeMock(void *context)
{
    // 回调处理完成后释放context
    DLI_CHECK_RETURN(context, "context is null");
    DLI_ManagerContext *managerContext = (DLI_ManagerContext *)context;
    SDF_MemFree(managerContext->cbkContext);
    managerContext->cbkContext = NULL;
    SDF_MemFree(managerContext->innerCbkTable);
    managerContext->innerCbkTable = NULL;
    SDF_MemFree(managerContext);
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseThreadInit)
{
    EXPECT_NE(0, DLI_SapiInit(NULL));
    EXPECT_EQ(0, DLI_ThreadInit());
    EXPECT_NE(0, DLI_ThreadInit());
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseAddWork)
{
    uint8_t i = 1;
    EXPECT_NE(0, DLI_ThreadAddWork(TestCaseAddWorkCallback, (void*)&i, NULL));
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_ThreadAddWork(TestCaseAddWorkCallback, (void*)&i, NULL);
    EXPECT_EQ(0, g_count);
    DLI_ThreadPostEvent();
    EXPECT_NE(-1, DLI_ThreadEvcHandleGet());
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
    EXPECT_EQ(1, g_count);
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseAddAndPostWork)
{
    uint8_t i = 0;
    EXPECT_NE(0, DLI_PostTask(TestCaseAddWorkCallback, (void*)&i, NULL));
    EXPECT_EQ(0, DLI_ThreadInit());
    i = DATA_NUM;
    DLI_PostTask(TestCaseAddWorkCallback, (void*)&i, NULL);
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
    EXPECT_EQ(DATA_NUM, g_count);
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseSend)
{
    DLI_SapiInit(TEST_received);
    uint8_t data[DATA_LEN] = {0};
    data[0] = PACKET_TYPE_SLE_CMD;
    EXPECT_EQ(0, DLI_SapiSend(data, DATA_LEN));
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
    data[0] = PACKET_TYPE_SLE_ICB;
    EXPECT_EQ(0, DLI_SapiSend(data, DATA_LEN));
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
    data[0] = PACKET_TYPE_SLE_ACB;
    EXPECT_EQ(0, DLI_SapiSend(data, DATA_LEN));
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
    DLI_SapiDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseDataLenConfig)
{
    DLI_AllDataSet(0, 0, 0, 0);
    EXPECT_EQ(0, DLI_DataLenGet(ACB_DATA_TYPE));
    DLI_AllDataSet(DATA_LEN, DATA_NUM, DATA_LEN, DATA_NUM);
    EXPECT_EQ(DATA_LEN, DLI_DataLenGet(ACB_DATA_TYPE));
    EXPECT_EQ(0, DLI_DataLenGet(UNKNOWN_DATA_TYPE));
    EXPECT_EQ(DATA_NUM, DLI_DataNumGet(ACB_DATA_TYPE));
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseCmdConfig)
{
    DLI_CmdNumSet(DLI_DEFAULT_CMD_NUM);
    EXPECT_TRUE(DLI_IsCmdAllow());
    DLI_CmdNumSubOne();
    EXPECT_FALSE(DLI_IsCmdAllow());
    DLI_CmdNumSet(1);
    EXPECT_TRUE(DLI_IsCmdAllow());
    DLI_CmdNumSet(0);
    EXPECT_FALSE(DLI_IsCmdAllow());
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseLayerInit)
{
    EXPECT_EQ(0, DLI_LayerInit());
    EXPECT_NE(0, DLI_LayerInit());
    DLI_LayerDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseCmdSendFail)
{
    DLI_CmdStru cmd;
    cmd.parLen = 0;
    EXPECT_EQ(DLI_STACK_PARAMS_ERRNO, DLI_CmdSend(NULL));
    EXPECT_EQ(DLI_STACK_NOINIT_ERRNO, DLI_CmdSend(&cmd));
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseDataSendFail)
{
    DLI_DataStru data;
    SDF_Buff_S *buf = SDF_BuffNewWithReserve(DATA_LEN);
    EXPECT_NE(buf, nullptr);
    EXPECT_EQ(DLI_STACK_PARAMS_ERRNO, DLI_DataSend(NULL));
    data.buf = NULL;
    EXPECT_EQ(DLI_STACK_PARAMS_ERRNO, DLI_DataSend(&data));
    data.buf = buf;
    EXPECT_EQ(DLI_STACK_NOINIT_ERRNO, DLI_DataSend(&data));
    SDF_BuffFree(buf);
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseCmdSendSuccess)
{
    EXPECT_EQ(0, DLI_LayerInit());
    EXPECT_NE(0, DLI_LayerInit());
    EXPECT_EQ(0, DLI_LayerEnable());

    DLI_CmdStru *cmd = DLI_DefaultCmdStruCreate(1, 1, NULL, 0);
    EXPECT_NE(cmd, nullptr);
    cmd->needErase = true;
    EXPECT_EQ(0, DLI_CmdSend(cmd));

    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
    DLI_PostNextTask(DLI_CMD_TASK);
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);

    // dli 打印是：a101000000
    DLI_CmdNumSet(1);
    EXPECT_TRUE(DLI_IsCmdAllow());
    DLI_PostNextTask(DLI_CMD_TASK);
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseDataSendSuccess)
{
    SDF_Buff_S *buf = SDF_BuffNewWithReserve(DATA_LEN);
    EXPECT_NE(buf, nullptr);
    uint16_t len = SDF_BuffTailRoom(buf);
    uint8_t *info = SDF_BuffAppend(buf, DATA_LEN);
    (void)memcpy_s(info, len, "1234567890", DATA_LEN);
    DLI_DataStru *data = DLI_DefaultDataStruCreate(0, DLI_DATATYPE_ACB, 0, 0, buf);
    EXPECT_NE(data, nullptr);

    // 配置数据发送的长度及数目
    DLI_AllDataSet(DATA_LEN, DATA_NUM, DATA_LEN, DATA_NUM);
    EXPECT_EQ(DATA_LEN, DLI_DataLenGet(ACB_DATA_TYPE));
    EXPECT_EQ(0, DLI_DataSend(data));
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseDataSendLimitSuccess)
{
    SDF_Buff_S *buf = SDF_BuffNewWithReserve(DATA_LEN - 1);
    EXPECT_NE(buf, nullptr);

    uint16_t len = SDF_BuffTailRoom(buf);
    uint8_t *info = SDF_BuffAppend(buf, DATA_LEN - 1);
    (void)memcpy_s(info, len, "123456789", DATA_LEN - 1);
    DLI_DataStru *data = DLI_DefaultDataStruCreate(1, DLI_DATATYPE_ACB, 0, 0, buf);
    EXPECT_NE(data, nullptr);

    // 数据限流，只能发送1次
    DLI_AllDataSet(DATA_FRAGMENTS_LEN, 1, DATA_LEN, DATA_NUM);
    EXPECT_EQ(0, DLI_DataSend(data));
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
    // 分片数据没有全部发送，需要销毁不影响下一个用例
    DLI_LayerDisable();
    DLI_LayerDeinit();
}

// 验证数据重组
TEST_F(UT_DLI_LAYER_TEST, TestCaseFragmentsDataRecvSuccess)
{
    SDF_Buff_S *buf = SDF_BuffNewWithReserve(DATA_MAX_LEN);
    EXPECT_NE(buf, nullptr);

    uint16_t len = SDF_BuffTailRoom(buf);
    uint8_t *info = SDF_BuffAppend(buf, DATA_MAX_LEN);
    (void)memcpy_s(info, len, "12345678900", DATA_MAX_LEN);
    DLI_DataStru *data = DLI_DefaultDataStruCreate(0, DLI_DATATYPE_ACB, 0, 0, buf);
    EXPECT_NE(data, nullptr);

    EXPECT_EQ(0, DLI_LayerInit());
    EXPECT_EQ(0, DLI_LayerEnable());
    // 配置数据发送的长度及数目
    DLI_AllDataSet(DATA_MAX_LEN, DATA_NUM, DATA_LEN, DATA_NUM);
    EXPECT_EQ(DATA_MAX_LEN, DLI_DataLenGet(ACB_DATA_TYPE));
    EXPECT_EQ(0, DLI_DataSend(data));
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
    DLI_LayerDisable();
    DLI_LayerDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, TestLayerUtil)
{
    SDF_DListHead_S dataRx;
    SDF_DListHeadInit(&dataRx);
    DLI_RecvDataNode *dataNode = DLI_RecvDataNodeCreate(0, 0, NULL);
    EXPECT_NE(dataNode, nullptr);
    SDF_DListElmTailInsert(&(dataRx), dataNode, node);
    EXPECT_NE(DLI_DataNodeFind(&dataRx, 0), nullptr);
    SDF_Buff_S *buf = SDF_BuffNew(100);
    EXPECT_NE(buf, nullptr);
    uint8_t *data = SDF_BuffAppend(buf, 100);
    EXPECT_NE(data, nullptr);
    // dataNode->buf null
    EXPECT_EQ(DLI_ReciveDataUpdate(dataNode, 0, DLI_MIDDLE_FRAGMENT, buf), false);
    SDF_Buff_S *buf1 = SDF_BuffNew(300);
    dataNode->buf = buf1;
    EXPECT_EQ(DLI_ReciveDataUpdate(dataNode, 0, DLI_MIDDLE_FRAGMENT, buf), true);
    SDF_BuffFree(buf1);
    buf1 = SDF_BuffNew(50);
    dataNode->buf = buf1;
    EXPECT_EQ(DLI_ReciveDataUpdate(dataNode, 0, DLI_MIDDLE_FRAGMENT, buf), true);
    DLI_DataStruDestroy(NULL);
    DLI_RecvDataNodeDestroy(dataNode);
    SDF_BuffFree(buf);
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseCmdSend)
{
    SDF_DListHead_S cmdRx;
    SDF_DListHeadInit(&cmdRx);

    DLI_CmdStru *cmd = DLI_DefaultCmdStruCreate(DLI_CREATE_CONNECTION, DLI_CONNECTION_COMPLETE_EVT, NULL, 0);
    EXPECT_NE(cmd, NULL);
    cmd->contextFree = ContextFreeMock;

    DLI_CmdTxNode *cmdNode = DLI_CmdNodeCreate(cmd);
    EXPECT_NE(cmdNode, NULL);

    SDF_DListElmTailInsert(&(cmdRx), cmdNode, node);
    EXPECT_NE(DLI_CmdNodeFindByOpcode(&cmdRx, 0), NULL);
    EXPECT_NE(DLI_CmdNodeFind(&cmdRx, DLI_CONNECTION_COMPLETE_EVT, DLI_CREATE_CONNECTION), NULL);

    DLI_CmdNodeDestroy((void *)cmdNode);
}

static volatile bool g_dataNumChangeCalled = false;
static DLI_DataType g_receivedDataType = ACB_DATA_TYPE;
static uint16_t g_receivedDataNum = 0;

static void TestDataNumChangeCallback(DLI_DataType type, uint16_t dataNum)
{
    g_dataNumChangeCalled = true;
    g_receivedDataType = type;
    g_receivedDataNum = dataNum;
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseDLI_DataNumChangeRegister)
{
    g_dataNumChangeCalled = false;
    DLI_DataNumChangeRegister(TestDataNumChangeCallback);
    DLI_DataNumChangeRegister(NULL);
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseDLI_DataNumChange_WithCallback)
{
    DLI_DataNumChangeRegister(TestDataNumChangeCallback);
    g_dataNumChangeCalled = false;
    DLI_DataNumChange(ACB_DATA_TYPE, 10);
    EXPECT_TRUE(g_dataNumChangeCalled);
    EXPECT_EQ(g_receivedDataType, ACB_DATA_TYPE);
    EXPECT_EQ(g_receivedDataNum, 10);
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseDLI_DataNumChange_DifferentDataTypes)
{
    DLI_DataNumChangeRegister(TestDataNumChangeCallback);
    g_dataNumChangeCalled = false;
    DLI_DataNumChange(ICB_DATA_TYPE, 5);
    EXPECT_TRUE(g_dataNumChangeCalled);
    EXPECT_EQ(g_receivedDataType, ICB_DATA_TYPE);
    EXPECT_EQ(g_receivedDataNum, 5);

    g_dataNumChangeCalled = false;
    DLI_DataNumChange(UNKNOWN_DATA_TYPE, 3);
    EXPECT_TRUE(g_dataNumChangeCalled);
    EXPECT_EQ(g_receivedDataType, UNKNOWN_DATA_TYPE);
    EXPECT_EQ(g_receivedDataNum, 3);
}

TEST_F(UT_DLI_LAYER_TEST, TestCaseDLI_DataNumChange_WithoutCallback)
{
    DLI_DataNumChangeRegister(NULL);
    g_dataNumChangeCalled = false;
    DLI_DataNumChange(ACB_DATA_TYPE, 10);
    EXPECT_FALSE(g_dataNumChangeCalled);
}

static volatile bool g_postOtherThreadCalled = false;
static uint32_t MockPostOtherThread(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    g_postOtherThreadCalled = true;
    if (cb != NULL) {
        cb(arg);
    }
    if (freeCb != NULL && arg != NULL) {
        freeCb(arg);
    }
    return 0;
}

static volatile bool g_postOtherBlockedThreadCalled = false;
static uint32_t MockPostOtherBlockedThread(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    g_postOtherBlockedThreadCalled = true;
    if (cb != NULL) {
        cb(arg);
    }
    if (freeCb != NULL && arg != NULL) {
        freeCb(arg);
    }
    return 0;
}

static void MockPostWorkCb(void *arg)
{
    g_count = 1;
}

static void MockRecvAcbHandler(uint16_t lcid, SDF_Buff_S *buf)
{
    (void)lcid;
    (void)buf;
}

TEST_F(UT_DLI_LAYER_TEST, DLI_ThreadSetCallback_NullPointer)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_ThreadSetCallback(NULL);
    EXPECT_EQ(DLI_STACK_POST_BLOCK_ERROR, DLI_PostOtherThread(MockPostWorkCb, NULL, NULL));
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, DLI_PostOtherThread_WithCallback)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_Callback cbk = {0};
    cbk.postOtherThread = MockPostOtherThread;
    cbk.postOtherBlockedThread = MockPostOtherBlockedThread;
    cbk.recvAcbHandler = MockRecvAcbHandler;
    EXPECT_EQ(0, DLI_SetCallback(&cbk));
    g_postOtherThreadCalled = false;
    g_count = 0;
    EXPECT_EQ(0, DLI_PostOtherThread(MockPostWorkCb, NULL, NULL));
    EXPECT_TRUE(g_postOtherThreadCalled);
    DLI_SetCallback(NULL);
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, DLI_PostOtherBlockedThread_NullPointer)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_ThreadSetCallback(NULL);
    EXPECT_EQ(DLI_STACK_POST_BLOCK_ERROR, DLI_PostOtherBlockedThread(MockPostWorkCb, NULL, NULL, 100));
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, DLI_PostOtherBlockedThread_WithCallback)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_Callback cbk = {0};
    cbk.postOtherThread = MockPostOtherThread;
    cbk.postOtherBlockedThread = MockPostOtherBlockedThread;
    cbk.recvAcbHandler = MockRecvAcbHandler;
    EXPECT_EQ(0, DLI_SetCallback(&cbk));
    g_postOtherBlockedThreadCalled = false;
    EXPECT_EQ(0, DLI_PostOtherBlockedThread(MockPostWorkCb, NULL, NULL, 1000));
    EXPECT_TRUE(g_postOtherBlockedThreadCalled);
    DLI_SetCallback(NULL);
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, DLI_SetCallback_NullClearsThreadCallback)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_Callback cbk = {0};
    cbk.postOtherThread = MockPostOtherThread;
    cbk.postOtherBlockedThread = MockPostOtherBlockedThread;
    cbk.recvAcbHandler = MockRecvAcbHandler;
    EXPECT_EQ(0, DLI_SetCallback(&cbk));
    g_postOtherThreadCalled = false;
    EXPECT_EQ(0, DLI_PostOtherThread(MockPostWorkCb, NULL, NULL));
    EXPECT_TRUE(g_postOtherThreadCalled);
    DLI_SetCallback(NULL);
    g_postOtherThreadCalled = false;
    EXPECT_EQ(DLI_STACK_POST_BLOCK_ERROR, DLI_PostOtherThread(MockPostWorkCb, NULL, NULL));
    EXPECT_FALSE(g_postOtherThreadCalled);
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, DLI_SetCallback_InvalidParamNotInjected)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_ThreadSetCallback(NULL);
    DLI_Callback invalidCbk = {0};
    invalidCbk.postOtherThread = NULL;
    invalidCbk.postOtherBlockedThread = MockPostOtherBlockedThread;
    invalidCbk.recvAcbHandler = MockRecvAcbHandler;
    EXPECT_EQ(DLI_STACK_PARAMS_ERRNO, DLI_SetCallback(&invalidCbk));
    EXPECT_EQ(DLI_STACK_POST_BLOCK_ERROR, DLI_PostOtherThread(MockPostWorkCb, NULL, NULL));
    DLI_ThreadDeinit();
}

static volatile int32_t g_regOrderWorkResult = 0;
static void MockRegOrderWorkCb(void *arg)
{
    g_regOrderWorkResult = 42;
}

static uint32_t MockRegOrderPostOtherThread(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (cb != NULL) {
        cb(arg);
    }
    return 0;
}

static uint32_t MockRegOrderPostOtherBlockedThread(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    if (cb != NULL) {
        cb(arg);
    }
    return 0;
}

static volatile int32_t g_deinitWorkResult = 0;
static void MockDeinitWorkCb(void *arg)
{
    g_deinitWorkResult = 99;
}

TEST_F(UT_DLI_LAYER_TEST, DLI_InitOrder_SetCallbackBeforePost)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_ThreadSetCallback(NULL);
    DLI_Callback cbk = {0};
    cbk.postOtherThread = MockRegOrderPostOtherThread;
    cbk.postOtherBlockedThread = MockRegOrderPostOtherBlockedThread;
    cbk.recvAcbHandler = MockRecvAcbHandler;
    EXPECT_EQ(0, DLI_SetCallback(&cbk));
    g_regOrderWorkResult = 0;
    EXPECT_EQ(0, DLI_PostOtherThread(MockRegOrderWorkCb, NULL, NULL));
    EXPECT_EQ(42, g_regOrderWorkResult);
    DLI_SetCallback(NULL);
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, DLI_InitOrder_PostBeforeSetCallbackFails)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_ThreadSetCallback(NULL);
    g_regOrderWorkResult = 0;
    EXPECT_EQ(DLI_STACK_POST_BLOCK_ERROR, DLI_PostOtherThread(MockRegOrderWorkCb, NULL, NULL));
    EXPECT_EQ(0, g_regOrderWorkResult);
    EXPECT_EQ(DLI_STACK_POST_BLOCK_ERROR, DLI_PostOtherBlockedThread(MockRegOrderWorkCb, NULL, NULL, 100));
    EXPECT_EQ(0, g_regOrderWorkResult);
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, DLI_PostOtherBlockedThread_FunctionalWithCallback)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_Callback cbk = {0};
    cbk.postOtherThread = MockPostOtherThread;
    cbk.postOtherBlockedThread = MockPostOtherBlockedThread;
    cbk.recvAcbHandler = MockRecvAcbHandler;
    EXPECT_EQ(0, DLI_SetCallback(&cbk));
    g_postOtherBlockedThreadCalled = false;
    g_count = 0;
    EXPECT_EQ(0, DLI_PostOtherBlockedThread(MockPostWorkCb, NULL, NULL, 5000));
    EXPECT_TRUE(g_postOtherBlockedThreadCalled);
    EXPECT_EQ(1, g_count);
    DLI_SetCallback(NULL);
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, DLI_DeinitOrder_CallbackClearedBeforePost)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_Callback cbk = {0};
    cbk.postOtherThread = MockPostOtherThread;
    cbk.postOtherBlockedThread = MockPostOtherBlockedThread;
    cbk.recvAcbHandler = MockRecvAcbHandler;
    EXPECT_EQ(0, DLI_SetCallback(&cbk));
    g_postOtherThreadCalled = false;
    EXPECT_EQ(0, DLI_PostOtherThread(MockPostWorkCb, NULL, NULL));
    EXPECT_TRUE(g_postOtherThreadCalled);
    DLI_SetCallback(NULL);
    g_deinitWorkResult = 0;
    EXPECT_EQ(DLI_STACK_POST_BLOCK_ERROR, DLI_PostOtherThread(MockDeinitWorkCb, NULL, NULL));
    EXPECT_EQ(0, g_deinitWorkResult);
    EXPECT_EQ(DLI_STACK_POST_BLOCK_ERROR, DLI_PostOtherBlockedThread(MockDeinitWorkCb, NULL, NULL, 100));
    EXPECT_EQ(0, g_deinitWorkResult);
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, DLI_DeinitOrder_ThreadDeinitAfterCallbackClear)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_Callback cbk = {0};
    cbk.postOtherThread = MockPostOtherThread;
    cbk.postOtherBlockedThread = MockPostOtherBlockedThread;
    cbk.recvAcbHandler = MockRecvAcbHandler;
    EXPECT_EQ(0, DLI_SetCallback(&cbk));
    DLI_SetCallback(NULL);
    DLI_ThreadDeinit();
    EXPECT_EQ(DLI_STACK_POST_BLOCK_ERROR, DLI_PostOtherThread(MockDeinitWorkCb, NULL, NULL));
    EXPECT_EQ(DLI_STACK_POST_BLOCK_ERROR, DLI_PostOtherBlockedThread(MockDeinitWorkCb, NULL, NULL, 100));
}

static volatile int32_t g_repeatedPostCount = 0;
static uint32_t MockRepeatedPostOtherThread(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    g_repeatedPostCount++;
    if (cb != NULL) {
        cb(arg);
    }
    return 0;
}

static void MockRepeatedWorkCb(void *arg)
{
    (void)arg;
}

TEST_F(UT_DLI_LAYER_TEST, DLI_PostOtherThread_RepeatedCallsConsistent)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_Callback cbk = {0};
    cbk.postOtherThread = MockRepeatedPostOtherThread;
    cbk.postOtherBlockedThread = MockPostOtherBlockedThread;
    cbk.recvAcbHandler = MockRecvAcbHandler;
    EXPECT_EQ(0, DLI_SetCallback(&cbk));
    g_repeatedPostCount = 0;
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(0, DLI_PostOtherThread(MockRepeatedWorkCb, NULL, NULL));
    }
    EXPECT_EQ(10, g_repeatedPostCount);
    DLI_SetCallback(NULL);
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, DLI_PostOtherThread_ReRegisterCallback)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_Callback cbk1 = {0};
    cbk1.postOtherThread = MockPostOtherThread;
    cbk1.postOtherBlockedThread = MockPostOtherBlockedThread;
    cbk1.recvAcbHandler = MockRecvAcbHandler;
    EXPECT_EQ(0, DLI_SetCallback(&cbk1));
    g_postOtherThreadCalled = false;
    EXPECT_EQ(0, DLI_PostOtherThread(MockPostWorkCb, NULL, NULL));
    EXPECT_TRUE(g_postOtherThreadCalled);

    DLI_Callback cbk2 = {0};
    cbk2.postOtherThread = MockRepeatedPostOtherThread;
    cbk2.postOtherBlockedThread = MockPostOtherBlockedThread;
    cbk2.recvAcbHandler = MockRecvAcbHandler;
    EXPECT_EQ(0, DLI_SetCallback(&cbk2));
    g_repeatedPostCount = 0;
    EXPECT_EQ(0, DLI_PostOtherThread(MockRepeatedWorkCb, NULL, NULL));
    EXPECT_EQ(1, g_repeatedPostCount);
    g_postOtherThreadCalled = false;
    EXPECT_FALSE(g_postOtherThreadCalled);

    DLI_SetCallback(NULL);
    DLI_ThreadDeinit();
}

static volatile bool g_freeArgCalled = false;
static void MockFreeArg(void *arg)
{
    g_freeArgCalled = true;
    (void)arg;
}

TEST_F(UT_DLI_LAYER_TEST, DLI_PostOtherThread_NullCallbackFreesArg)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_ThreadSetCallback(NULL);
    uint32_t testArg = 0xDEAD;
    g_freeArgCalled = false;
    EXPECT_EQ(DLI_STACK_POST_BLOCK_ERROR, DLI_PostOtherThread(MockPostWorkCb, &testArg, MockFreeArg));
    EXPECT_TRUE(g_freeArgCalled);
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, DLI_PostOtherBlockedThread_NullCallbackFreesArg)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_ThreadSetCallback(NULL);
    uint32_t testArg = 0xBEEF;
    g_freeArgCalled = false;
    EXPECT_EQ(DLI_STACK_POST_BLOCK_ERROR, DLI_PostOtherBlockedThread(MockPostWorkCb, &testArg, MockFreeArg, 100));
    EXPECT_TRUE(g_freeArgCalled);
    DLI_ThreadDeinit();
}

static volatile int32_t g_mockPostReturnVal = 0;
static uint32_t MockPostOtherThreadReturnErr(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    (void)cb;
    (void)arg;
    (void)freeCb;
    return g_mockPostReturnVal;
}

TEST_F(UT_DLI_LAYER_TEST, DLI_PostOtherThread_ReturnValuePropagated)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_Callback cbk = {0};
    cbk.postOtherThread = MockPostOtherThreadReturnErr;
    cbk.postOtherBlockedThread = MockPostOtherBlockedThread;
    cbk.recvAcbHandler = MockRecvAcbHandler;
    EXPECT_EQ(0, DLI_SetCallback(&cbk));
    g_mockPostReturnVal = 0x1234;
    EXPECT_EQ(0x1234, DLI_PostOtherThread(MockPostWorkCb, NULL, NULL));
    g_mockPostReturnVal = 0;
    EXPECT_EQ(0, DLI_PostOtherThread(MockPostWorkCb, NULL, NULL));
    DLI_SetCallback(NULL);
    DLI_ThreadDeinit();
}

static volatile int32_t g_mockBlockedTimeout = 0;
static uint32_t MockPostOtherBlockedThreadRecordTimeout(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    g_mockBlockedTimeout = timeout;
    if (cb != NULL) {
        cb(arg);
    }
    return 0;
}

TEST_F(UT_DLI_LAYER_TEST, DLI_PostOtherBlockedThread_TimeoutForwarded)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_Callback cbk = {0};
    cbk.postOtherThread = MockPostOtherThread;
    cbk.postOtherBlockedThread = MockPostOtherBlockedThreadRecordTimeout;
    cbk.recvAcbHandler = MockRecvAcbHandler;
    EXPECT_EQ(0, DLI_SetCallback(&cbk));
    g_mockBlockedTimeout = 0;
    EXPECT_EQ(0, DLI_PostOtherBlockedThread(MockPostWorkCb, NULL, NULL, 3000));
    EXPECT_EQ(3000, g_mockBlockedTimeout);
    DLI_SetCallback(NULL);
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, DLI_SetCallback_PostOtherBlockedThreadNullRejected)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_ThreadSetCallback(NULL);
    DLI_Callback invalidCbk = {0};
    invalidCbk.postOtherThread = MockPostOtherThread;
    invalidCbk.postOtherBlockedThread = NULL;
    invalidCbk.recvAcbHandler = MockRecvAcbHandler;
    EXPECT_EQ(DLI_STACK_PARAMS_ERRNO, DLI_SetCallback(&invalidCbk));
    EXPECT_EQ(DLI_STACK_POST_BLOCK_ERROR, DLI_PostOtherBlockedThread(MockPostWorkCb, NULL, NULL, 100));
    DLI_ThreadDeinit();
}

TEST_F(UT_DLI_LAYER_TEST, DLI_SetCallback_RecvAcbHandlerNullRejected)
{
    EXPECT_EQ(0, DLI_ThreadInit());
    DLI_ThreadSetCallback(NULL);
    DLI_Callback invalidCbk = {0};
    invalidCbk.postOtherThread = MockPostOtherThread;
    invalidCbk.postOtherBlockedThread = MockPostOtherBlockedThread;
    invalidCbk.recvAcbHandler = NULL;
    EXPECT_EQ(DLI_STACK_PARAMS_ERRNO, DLI_SetCallback(&invalidCbk));
    EXPECT_EQ(DLI_STACK_POST_BLOCK_ERROR, DLI_PostOtherThread(MockPostWorkCb, NULL, NULL));
    DLI_ThreadDeinit();
}
