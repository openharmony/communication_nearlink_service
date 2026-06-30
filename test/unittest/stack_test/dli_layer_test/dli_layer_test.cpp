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
static volatile int g_sendDataCnt = 0;
static volatile int g_sendCmdCnt = 0;
static volatile int g_sendEventCnt = 0;

class UT_DLI_LAYER_TEST : public testing::Test {
  protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    void SetUp()
    {
        g_count = 0;
        g_sendDataCnt = 0;
        g_sendCmdCnt = 0;
        g_sendEventCnt = 0;
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

void TEST_writeLogStub(uint16_t type, const uint8_t *data, uint32_t len, int result)
{
    switch (type) {
        case DLI_DATATYPE_ACB:
            g_sendDataCnt++;
            break;
        case DLI_DATATYPE_CMD:
            g_sendCmdCnt++;
            break;
        case DLI_DATATYPE_EVENT:
            g_sendEventCnt++;
            break;
        default:
            break;
    }
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
    DLI_SetWriteFileCallback(TEST_writeLogStub);
    EXPECT_EQ(0, DLI_LayerInit());
    EXPECT_NE(0, DLI_LayerInit());
    EXPECT_EQ(0, DLI_LayerEnable());

    DLI_CmdStru *cmd = DLI_DefaultCmdStruCreate(1, 1, NULL, 0);
    EXPECT_NE(cmd, nullptr);
    EXPECT_EQ(0, DLI_CmdSend(cmd));

    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
    EXPECT_EQ(1, g_sendCmdCnt);
    DLI_PostNextTask(DLI_CMD_TASK);
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
    EXPECT_EQ(1, g_sendCmdCnt);

    // dli 打印是：a101000000
    DLI_CmdNumSet(1);
    EXPECT_TRUE(DLI_IsCmdAllow());
    DLI_PostNextTask(DLI_CMD_TASK);
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
    EXPECT_EQ(TEST_CMD_CNT1, g_sendCmdCnt);
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

    DLI_SetWriteFileCallback(TEST_writeLogStub);
    // 配置数据发送的长度及数目
    DLI_AllDataSet(DATA_LEN, DATA_NUM, DATA_LEN, DATA_NUM);
    EXPECT_EQ(DATA_LEN, DLI_DataLenGet(ACB_DATA_TYPE));
    EXPECT_EQ(0, DLI_DataSend(data));
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
    EXPECT_EQ(1, g_sendDataCnt);
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
    DLI_SetWriteFileCallback(TEST_writeLogStub);
    g_sendDataCnt = 0;
    EXPECT_EQ(0, DLI_DataSend(data));
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
    EXPECT_EQ(1, g_sendDataCnt);
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

    g_sendDataCnt = 0;
    EXPECT_EQ(0, DLI_LayerInit());
    EXPECT_EQ(0, DLI_LayerEnable());
    DLI_SetWriteFileCallback(TEST_writeLogStub);
    // 配置数据发送的长度及数目
    DLI_AllDataSet(DATA_MAX_LEN, DATA_NUM, DATA_LEN, DATA_NUM);
    EXPECT_EQ(DATA_MAX_LEN, DLI_DataLenGet(ACB_DATA_TYPE));
    EXPECT_EQ(0, DLI_DataSend(data));
    SDF_ThreadSleep(DLI_TEST_SLEEP_TIME);
    // DLI_DataSend不分片，所以发送一次，接收1次数据。dataCnt总计1次
    EXPECT_EQ(TEST_DATA_CNT, g_sendDataCnt);
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