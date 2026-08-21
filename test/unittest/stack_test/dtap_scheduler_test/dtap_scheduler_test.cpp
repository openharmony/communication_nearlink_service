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
static uint8_t g_connHandle3 = 3;

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

// 可控时钟：替代真实 DP_GetMonoTimeMs，用于触发 10s 速率计算窗口
static uint64_t g_testMonoTimeMs = 0;
static void TEST_DtapSetMonoTimeMs(uint64_t t) { g_testMonoTimeMs = t; }

extern "C" uint64_t DP_GetMonoTimeMs(void) { return g_testMonoTimeMs; }
extern "C" uint64_t DP_GetRealTimeMs(void) { return g_testMonoTimeMs; }

// 取 lcid 的 buffer 节点并断言非空：避免后续直接解引用 quota/queuedPktCnt 等字段时空指针 UB。
// 节点不存在时当前用例会以 EXPECT_NE 失败终止，而非 crash。
static DTAP_LcidBufferNode *TEST_GetBufferNode(uint16_t lcid)
{
    DTAP_LcidBufferNode *node = DTAP_GetLcidBufferNode(lcid);
    EXPECT_NE(node, nullptr);
    return node;
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
    TEST_DtapConnectCbk(g_connHandle3);
}

static void TEST_DtapSchedulerDeinit(void)
{
    TEST_DtapDisconnectCbk(g_connHandle1);
    TEST_DtapDisconnectCbk(g_connHandle2);
    TEST_DtapDisconnectCbk(g_connHandle3);
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

    uint8_t acbNum = 8;
    DLI_AllDataSet(100, acbNum, 0, 0);
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
    SDF_Buff_S *buf3 = NULL;
    for (uint32_t i = 0; i < acbNum / 2; i++) {
        buf3 = SDF_BuffNewWithReserve(100);
        EXPECT_NE(buf3, nullptr);
        EXPECT_NE(SDF_BuffAppend(buf3, 10), nullptr);
        EXPECT_EQ(DTAP_DataSendWithPriority(channel2, buf3), DTAP_SUCCESS);
    }
    // different lcid
    DTAP_Channel_S *channel3 = TEST_DtapChannelCreate(g_connHandle2, DTAP_PRIORITY_CMD, 0);
    EXPECT_NE(channel3, NULL);
    SDF_Buff_S *buf4 = NULL;
    for (uint32_t i = 0; i < acbNum / 2 + 1; i++) {
        buf4 = SDF_BuffNewWithReserve(100);
        EXPECT_NE(buf4, nullptr);
        EXPECT_NE(SDF_BuffAppend(buf4, 10), nullptr);
        EXPECT_EQ(DTAP_DataSendWithPriority(channel3, buf4), DTAP_SUCCESS);
    }

    TEST_DtapDisconnectCbk(g_connHandle1);
    
    buf4 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf4, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf4, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(channel3, buf4), DTAP_SUCCESS);

    DLI_AllDataSet(100, acbNum / 2, 0, 0);
    EXPECT_EQ(TEST_NOCPEventDo(DLI_REG_MODULE_DTAP, channel3->lcid, 1), DTAP_SUCCESS);

    TEST_DtapConnectCbk(g_connHandle1);
    for (uint32_t i = 0; i < acbNum / 2; i++) {
        buf3 = SDF_BuffNewWithReserve(100);
        EXPECT_NE(buf3, nullptr);
        EXPECT_NE(SDF_BuffAppend(buf3, 10), nullptr);
        EXPECT_EQ(DTAP_DataSendWithPriority(channel2, buf3), DTAP_SUCCESS);
    }

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

TEST_F(UT_DTAP_SCHEDULER, DTAP_DataSendWithPriorityMaxFragmentNumTest)
{
    TEST_DtapSchedulerInit();
    DLI_AllDataSet(1, 8, 0, 0);
    DTAP_Channel_S *channel = TEST_DtapChannelCreate(g_connHandle1, DTAP_PRIORITY_HIGH, 0);
    EXPECT_NE(channel, NULL);
    SDF_Buff_S *buf = SDF_BuffNewWithReserve(DTAP_MAX_PAYLOAD_LEN);
    EXPECT_NE(buf, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf, DTAP_MAX_PAYLOAD_LEN), nullptr);
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

TEST_F(UT_DTAP_SCHEDULER, DTAP_CalcLcidSendRatePrintTest)
{
    TEST_DtapSchedulerInit();
    DLI_AllDataSet(600, 8, 0, 0);
    EXPECT_EQ(g_collabTransFunc.dliAcbNumGet(), 8);

    DTAP_Channel_S *channel = TEST_DtapChannelCreate(g_connHandle1, DTAP_PRIORITY_CMD, 0);
    EXPECT_NE(channel, nullptr);

    // 先建立高配额: g_apBufferNum=0 时入队一个大包, 再 setApBufferNum(25) 触发 recalc
    // 单 LCID 有 queuedPktCnt, quota = 1 + bonusPool = 1 + (25-3) = 23
    g_collabTransFunc.setApBufferNum(0);
    SDF_Buff_S *bigBuf = SDF_BuffNewWithReserve(20000);
    EXPECT_NE(bigBuf, nullptr);
    EXPECT_NE(SDF_BuffAppend(bigBuf, 12000), nullptr); // 12000B → 20 fragments
    EXPECT_EQ(DTAP_DataSendWithPriority(channel, bigBuf), DTAP_SUCCESS); // 入队, 不发送
    g_collabTransFunc.setApBufferNum(25); // 触发 recalc, quota1=23
    // 断言重算结果: 单 LCID 独占全部 bonusPool(=25-3=22), quota=1+22=23
    // 入队是 1 个完整大包(分片发生在调度时), queuedPktCnt=1
    DTAP_LcidBufferNode *node = TEST_GetBufferNode(g_connHandle1);
    EXPECT_EQ(node->quota, 23);
    EXPECT_EQ(node->queuedPktCnt, 1);

    // t=1000: 调度大包, 20 fragments 全发(quota=23>=20, g_apBufferNum=25>=20)
    // 每片 dataLen=600(数据)+5(DLI_HEADER)=605, windowBytes 累加 20*605=12100
    // 首次 DTAP_CalcLcidSendRate: lastRateCalcTime=0→1000 初始化; 后续 elapsed=0 不结算
    TEST_DtapSetMonoTimeMs(1000);
    EXPECT_EQ(TEST_NOCPEventDo(DLI_REG_MODULE_DTAP, g_connHandle1, 0), DTAP_SUCCESS); // 触发 SchedulerRun
    // 断言速率窗口状态: 窗口内累计字节数=12100, 起始时间=1000, 全部已发未确认
    EXPECT_EQ(node->windowBytes, 12100);
    EXPECT_EQ(node->lastRateCalcTime, 1000);
    EXPECT_EQ(node->sendNotAckPktCnt, 20);

    // send-complete 释放全部 20 个包: 速率窗口不因 ACK 重置
    EXPECT_EQ(TEST_NOCPEventDo(DLI_REG_MODULE_DTAP, g_connHandle1, 20), DTAP_SUCCESS);
    EXPECT_EQ(node->windowBytes, 12100);
    EXPECT_EQ(node->lastRateCalcTime, 1000);
    EXPECT_EQ(node->sendNotAckPktCnt, 0);

    // t=12000: 发一个小包(10B+5HEADER=15B), elapsed=11000>=10000 → 结算并重置窗口
    // windowBytes 结算前=12100+15=12115, 结算后清零, lastRateCalcTime=12000
    TEST_DtapSetMonoTimeMs(12000);
    SDF_Buff_S *buf2 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf2, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf2, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(channel, buf2), DTAP_SUCCESS);
    // 断言窗口已结算重置: windowBytes=0, lastRateCalcTime 更新到 12000
    EXPECT_EQ(node->windowBytes, 0);
    EXPECT_EQ(node->lastRateCalcTime, 12000);

    // send-complete 释放缓冲区
    EXPECT_EQ(TEST_NOCPEventDo(DLI_REG_MODULE_DTAP, g_connHandle1, 1), DTAP_SUCCESS);

    // t=23000: elapsed=11000>=10000, 但 windowBytes=10(刚重置)+15=15, rateKB≈0.0014<=1.0 不打印
    TEST_DtapSetMonoTimeMs(23000);
    SDF_Buff_S *buf3 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf3, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf3, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(channel, buf3), DTAP_SUCCESS);
    // 断言窗口已结算重置(速率低于阈值也走重置路径)
    EXPECT_EQ(node->windowBytes, 0);
    EXPECT_EQ(node->lastRateCalcTime, 23000);

    TEST_DtapSchedulerDeinit();
    SDF_MemFree(channel);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_AllocLcidQuotaLargestRemainderTest)
{
    TEST_DtapSchedulerInit();
    // g_apBufferNum=0 阻止实际发送, 使 queuedPktCnt 保留入队值
    DLI_AllDataSet(100, 0, 0, 0);
    g_collabTransFunc.setApBufferNum(0);
    EXPECT_EQ(g_collabTransFunc.dliAcbNumGet(), 0);

    // 3 个 LCID 各入队不等量包: lcid1=2, lcid2=1, lcid3=1 → totalQueuedPktCnt=4
    DTAP_Channel_S *ch1 = TEST_DtapChannelCreate(g_connHandle1, DTAP_PRIORITY_CMD, 0);
    DTAP_Channel_S *ch2 = TEST_DtapChannelCreate(g_connHandle2, DTAP_PRIORITY_CMD, 0);
    DTAP_Channel_S *ch3 = TEST_DtapChannelCreate(g_connHandle3, DTAP_PRIORITY_CMD, 0);
    EXPECT_NE(ch1, nullptr);
    EXPECT_NE(ch2, nullptr);
    EXPECT_NE(ch3, nullptr);

    for (int i = 0; i < 2; i++) {
        SDF_Buff_S *buf = SDF_BuffNewWithReserve(100);
        EXPECT_NE(buf, nullptr);
        EXPECT_NE(SDF_BuffAppend(buf, 10), nullptr);
        EXPECT_EQ(DTAP_DataSendWithPriority(ch1, buf), DTAP_SUCCESS);
    }
    SDF_Buff_S *b2 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(b2, nullptr);
    EXPECT_NE(SDF_BuffAppend(b2, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(ch2, b2), DTAP_SUCCESS);
    SDF_Buff_S *b3 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(b3, nullptr);
    EXPECT_NE(SDF_BuffAppend(b3, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(ch3, b3), DTAP_SUCCESS);

    // setApBufferNum(8) 触发 recalc: bonusPool=8-3=5, 截断 bonus=[5*2/4,5*1/4,5*1/4]=[2,1,1], Σ=4, rem=1
    // 最大余数法补齐1次: frac=[5*2%4,5*1%4,5*1%4]=[2,1,1] → 给 lcid1, 最终 quota=[1+2+1,1+1,1+1]=[4,2,2], Σquota=8
    DLI_AllDataSet(100, 8, 0, 0);
    g_collabTransFunc.setApBufferNum(8);
    EXPECT_EQ(g_collabTransFunc.dliAcbNumGet(), 8);

    // 断言重算结果: 配额守恒 Σquota=g_apBufferNum, 且按最大余数法分配到具体值
    DTAP_LcidBufferNode *n1 = TEST_GetBufferNode(g_connHandle1);
    DTAP_LcidBufferNode *n2 = TEST_GetBufferNode(g_connHandle2);
    DTAP_LcidBufferNode *n3 = TEST_GetBufferNode(g_connHandle3);
    EXPECT_EQ(n1->quota, 4);
    EXPECT_EQ(n2->quota, 2);
    EXPECT_EQ(n3->quota, 2);
    EXPECT_EQ((uint32_t)n1->quota + n2->quota + n3->quota, 8);
    // 冻结发送场景下 queuedPktCnt 守恒, 应等于入队值
    EXPECT_EQ(n1->queuedPktCnt, 2);
    EXPECT_EQ(n2->queuedPktCnt, 1);
    EXPECT_EQ(n3->queuedPktCnt, 1);

    TEST_DtapSchedulerDeinit();
    SDF_MemFree(ch1);
    SDF_MemFree(ch2);
    SDF_MemFree(ch3);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_AllocLcidQuotaBonusPoolZeroTest)
{
    TEST_DtapSchedulerInit();
    // g_apBufferNum=0 阻止发送, 入队 2 个包保留 queuedPktCnt
    DLI_AllDataSet(100, 0, 0, 0);
    g_collabTransFunc.setApBufferNum(0);
    EXPECT_EQ(g_collabTransFunc.dliAcbNumGet(), 0);

    DTAP_Channel_S *ch1 = TEST_DtapChannelCreate(g_connHandle1, DTAP_PRIORITY_CMD, 0);
    EXPECT_NE(ch1, nullptr);
    for (int i = 0; i < 2; i++) {
        SDF_Buff_S *buf = SDF_BuffNewWithReserve(100);
        EXPECT_NE(buf, nullptr);
        EXPECT_NE(SDF_BuffAppend(buf, 10), nullptr);
        EXPECT_EQ(DTAP_DataSendWithPriority(ch1, buf), DTAP_SUCCESS);
    }

    // setApBufferNum(1): lcidNums=3, 1 <= 3*1 → bonusPool=0; totalQueuedPktCnt=2 != 0 → 进 AllocLcidQuota 走 bonusPool=0 分支
    DLI_AllDataSet(100, 1, 0, 0);
    g_collabTransFunc.setApBufferNum(1);
    EXPECT_EQ(g_collabTransFunc.dliAcbNumGet(), 1);

    // bonusPool=0 时全部 LCID 仅保底配额 1, queuedPktCnt 不变
    DTAP_LcidBufferNode *n1 = TEST_GetBufferNode(g_connHandle1);
    EXPECT_EQ(TEST_GetBufferNode(g_connHandle2)->quota, 1);
    EXPECT_EQ(TEST_GetBufferNode(g_connHandle3)->quota, 1);
    EXPECT_EQ(n1->quota, 1);
    EXPECT_EQ(n1->queuedPktCnt, 2);

    TEST_DtapSchedulerDeinit();
    SDF_MemFree(ch1);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_RecalcLcidQuotaZeroQueueTest)
{
    TEST_DtapSchedulerInit();
    // 3 个 LCID 均未发包, queuedPktCnt==0
    DLI_AllDataSet(100, 8, 0, 0);
    // 首次 setApBufferNum(8) 触发 recalc: totalQueuedPktCnt==0 走 else, quota=1, lastQuota 0→1 (line 244 true 打印)
    g_collabTransFunc.setApBufferNum(8);
    EXPECT_EQ(g_collabTransFunc.dliAcbNumGet(), 8);
    // 零队列分支: 全部 LCID 配额复位为保底值 1
    DTAP_LcidBufferNode *n1 = TEST_GetBufferNode(g_connHandle1);
    DTAP_LcidBufferNode *n2 = TEST_GetBufferNode(g_connHandle2);
    DTAP_LcidBufferNode *n3 = TEST_GetBufferNode(g_connHandle3);
    EXPECT_EQ(n1->quota, 1);
    EXPECT_EQ(n2->quota, 1);
    EXPECT_EQ(n3->quota, 1);

    // 再次 setApBufferNum(8) 触发 recalc: quota 仍 1 == lastQuota, 走 line 244 false 日志抑制
    g_collabTransFunc.setApBufferNum(8);
    // 二次重算后配额保持不变
    EXPECT_EQ(n1->quota, 1);
    EXPECT_EQ(n2->quota, 1);
    EXPECT_EQ(n3->quota, 1);

    TEST_DtapSchedulerDeinit();
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_DataSendWithPriorityQuotaExhaustedRecalcTest)
{
    TEST_DtapSchedulerInit();
    // g_apBufferNum=4, lcidNums=3 → bonusPool=1; 初始全 queued=0 走零队列分支 → quota 全部复位为 1
    DLI_AllDataSet(100, 4, 0, 0);
    g_collabTransFunc.setApBufferNum(4);
    EXPECT_EQ(g_collabTransFunc.dliAcbNumGet(), 4);
    DTAP_LcidBufferNode *n1 = TEST_GetBufferNode(g_connHandle1);
    DTAP_LcidBufferNode *n2 = TEST_GetBufferNode(g_connHandle2);
    EXPECT_EQ(n1->quota, 1);
    EXPECT_EQ(n2->quota, 1);

    DTAP_Channel_S *ch1 = TEST_DtapChannelCreate(g_connHandle1, DTAP_PRIORITY_CMD, 0);
    DTAP_Channel_S *ch2 = TEST_DtapChannelCreate(g_connHandle2, DTAP_PRIORITY_CMD, 0);
    EXPECT_NE(ch1, nullptr);
    EXPECT_NE(ch2, nullptr);

    // ch2 入队 1 包: sendNotAck2=0 < quota2=1 → 不重算; SchedulerRun 发 1 包, sendNotAck2=1=quota2 停
    SDF_Buff_S *b2 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(b2, nullptr);
    EXPECT_NE(SDF_BuffAppend(b2, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(ch2, b2), DTAP_SUCCESS);
    EXPECT_EQ(n1->quota, 1);
    EXPECT_EQ(n2->quota, 1);
    EXPECT_EQ(n2->sendNotAckPktCnt, 1);

    // ch2 再入队 1 包: sendNotAck2=1 >= quota2=1 且 g=1<4 → 触发重算, bonusPool=1 全给 ch2 → quota2=2
    // SchedulerRun: sendNotAck2=1 < quota2=2 → 再发 1 包, sendNotAck2=2=quota2 停
    SDF_Buff_S *b2b = SDF_BuffNewWithReserve(100);
    EXPECT_NE(b2b, nullptr);
    EXPECT_NE(SDF_BuffAppend(b2b, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(ch2, b2b), DTAP_SUCCESS);
    EXPECT_EQ(n2->quota, 2);
    EXPECT_EQ(n2->sendNotAckPktCnt, 2);

    // ch2 第 3 包: sendNotAck2=2 >= quota2=2 且 g=2<4 → 触发重算, ch2 仍唯一排队 → quota2 维持 2
    // SchedulerRun: sendNotAck2=2 < quota2=2 false → 不发
    SDF_Buff_S *b2c = SDF_BuffNewWithReserve(100);
    EXPECT_NE(b2c, nullptr);
    EXPECT_NE(SDF_BuffAppend(b2c, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(ch2, b2c), DTAP_SUCCESS);
    EXPECT_EQ(n2->quota, 2);
    EXPECT_EQ(n2->sendNotAckPktCnt, 2);
    // 配额耗尽且全局未满, b2c 仍排队待发
    EXPECT_EQ(n2->queuedPktCnt, 1);

    TEST_DtapSchedulerDeinit();
    SDF_MemFree(ch1);
    SDF_MemFree(ch2);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_ScheduleLcidFragmentPartialSaveTest)
{
    TEST_DtapSchedulerInit();
    // g_apBufferNum=1 仅允许 1 个未确认包, 使大包分片发送中途 CanSend 变 false
    DLI_AllDataSet(100, 1, 0, 0);
    g_collabTransFunc.setApBufferNum(1);
    EXPECT_EQ(g_collabTransFunc.dliAcbNumGet(), 1);

    DTAP_Channel_S *channel = TEST_DtapChannelCreate(g_connHandle1, DTAP_PRIORITY_CMD, 0);
    EXPECT_NE(channel, nullptr);

    // 大包 2000B, 按 600B 切分 fragmentCnt=4; 首次发送 sendCnt=1 后 g_sendNotAckPktCnt=1>=g_apBufferNum=1 break
    // → sendCnt=1 < 4 走 DTAP_SaveFragmentData 保存剩余 3 fragment (line 751 true, 758 queuedPktCnt+=3)
    TEST_DtapSetMonoTimeMs(0);
    SDF_Buff_S *bigBuf = SDF_BuffNewWithReserve(20000);
    EXPECT_NE(bigBuf, nullptr);
    EXPECT_NE(SDF_BuffAppend(bigBuf, 2000), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(channel, bigBuf), DTAP_SUCCESS);

    // send-complete 释放 1 包, g_sendNotAckPktCnt=0, 触发 SchedulerRun 调度 splited 包走 line 711-724
    TEST_DtapSetMonoTimeMs(1000);
    EXPECT_EQ(TEST_NOCPEventDo(DLI_REG_MODULE_DTAP, g_connHandle1, 1), DTAP_SUCCESS);

    // 继续释放并发送剩余 splited fragment, 每次发 1 个
    TEST_DtapSetMonoTimeMs(2000);
    EXPECT_EQ(TEST_NOCPEventDo(DLI_REG_MODULE_DTAP, g_connHandle1, 1), DTAP_SUCCESS);
    TEST_DtapSetMonoTimeMs(3000);
    EXPECT_EQ(TEST_NOCPEventDo(DLI_REG_MODULE_DTAP, g_connHandle1, 1), DTAP_SUCCESS);

    TEST_DtapSchedulerDeinit();
    SDF_MemFree(channel);
}

