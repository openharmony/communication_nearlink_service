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

/* dli 命令与事件分发用例：超时通知、内外回调注册注销、状态与完成事件处理、事件入口的长度分档 */
#include <gtest/gtest.h>
#include "securec.h"

#include "sdf_mem.h"
#include "sdf_buff.h"
#include "sdf_evc.h"
#include "sdf_thread.h"
#include "dli_full_memhook.h"

#include "dli.h"
#include "dli_cmd.h"
#include "dli_cmd_struct.h"
#include "dli_event.h"
#include "dli_event_struct.h"
#include "dli_errno.h"
#include "dli_opcode.h"
#include "dli_layer_stru.h"
#include "dli_layer_utils.h"
#include "dli_thread.h"
#include "dli_layer_callback.h"
#include "dli_sapi.h"
#include "dli_data_stub.h"

#define CE_TEST_SLEEP_MS 150
#define CE_TEST_EVT_FINAL 0x0003
#define CE_TEST_EVT_A 0x0022
#define CE_TEST_EVT_B 0x0031
#define CE_TEST_EVT_C 0x0032
#define CE_TEST_EVT_INNER_1 0x0041
#define CE_TEST_EVT_INNER_2 0x0042
#define CE_TEST_CMD_CANNED 0x0C06
#define CE_TEST_CMD_SCAN DLI_SET_SCAN_PARAMETERS
#define CE_TEST_SHORT_LEN 2

static volatile int g_ceErrInnerCount = 0;
static volatile uint16_t g_ceErrStatus = 0;
static volatile uint16_t g_ceErrCmd = 0;
static volatile int g_ceInnerACount = 0;
static volatile int g_ceInnerBCount = 0;
static volatile int g_ceNocpCount = 0;
static volatile uint16_t g_ceNocpHandle = 0;
static volatile uint8_t g_ceNocpNum = 0;
static volatile int g_ceExecCount = 0;
static volatile uint16_t g_ceExecStatus = 0;
static volatile uint32_t g_ceExecSize = 0;
static volatile uint16_t g_ceExecOpcode = 0;
static volatile bool g_cePostFail = false;
static volatile int g_ceFullInnerCount = 0;
static volatile bool g_ceFullInnerCtxNull = true;

extern "C" {
// 无头文件声明的内部导出函数，测试直接声明后调用
void TimeoutCallback(void *param);
}

static uint32_t CeSyncPost(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (g_cePostFail) {
        return 1;
    }
    if (cb != NULL) {
        cb(arg);
    }
    if (freeCb != NULL && arg != NULL) {
        freeCb(arg);
    }
    return 0;
}

static uint32_t CeSyncPostBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    (void)timeout;
    return CeSyncPost(cb, arg, freeCb);
}

static void CeRecvAcb(uint16_t lcid, SDF_Buff_S *buf)
{
    (void)lcid;
    (void)buf;
}

static void CeErrInner(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    (void)context;
    (void)len;
    (void)evtOpcode;
    DLI_CommandErrorStru *err = (DLI_CommandErrorStru *)arg;
    g_ceErrInnerCount++;
    g_ceErrStatus = err->status;
    g_ceErrCmd = err->cmd;
}

static void CeInnerA(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    (void)context;
    (void)arg;
    (void)len;
    (void)evtOpcode;
    g_ceInnerACount++;
}

static void CeInnerB(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    (void)context;
    (void)arg;
    (void)len;
    (void)evtOpcode;
    g_ceInnerBCount++;
}

static void CeNocp(uint16_t connHandle, uint8_t numCompletedPackets)
{
    g_ceNocpCount++;
    g_ceNocpHandle = connHandle;
    g_ceNocpNum = numCompletedPackets;
}

static void CeExecCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    g_ceExecCount++;
    g_ceExecStatus = status;
    g_ceExecSize = cmdRes->size;
    g_ceExecOpcode = cmdRes->cmdOpcode;
}

static void CeFullInnerEvt(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    (void)arg;
    (void)len;
    (void)evtOpcode;
    g_ceFullInnerCount++;
    g_ceFullInnerCtxNull = (context == NULL);
}

static void CeSetSyncCallback()
{
    DLI_Callback cbk = {0};
    cbk.postOtherThread = CeSyncPost;
    cbk.postOtherBlockedThread = CeSyncPostBlocked;
    cbk.recvAcbHandler = CeRecvAcb;
    DLI_SetCallback(&cbk);
}

class UT_DLI_CE_LIGHT : public testing::Test {
protected:
    void SetUp()
    {
        g_cePostFail = false;
        g_ceErrInnerCount = 0;
        g_ceInnerACount = 0;
        g_ceInnerBCount = 0;
        g_ceNocpCount = 0;
        g_ceExecCount = 0;
        CeSetSyncCallback();
    }

    void TearDown()
    {
        g_cePostFail = false;
        DLI_DeInitEventCbkList();
        DLI_SetCallback(NULL);
    }
};

class UT_DLI_CE_FULL : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_EQ(0, SDF_EvcInit());
        EXPECT_EQ(0, SDF_ThreadInit(2));
    }

    static void TearDownTestCase()
    {
        SDF_ThreadDeinit();
        SDF_EvcDeinit();
    }

    void SetUp()
    {
        g_ceExecCount = 0;
        g_ceExecStatus = 0;
        g_ceExecSize = 0;
        g_ceExecOpcode = 0;
        g_ceFullInnerCount = 0;
        g_ceFullInnerCtxNull = true;
        CeSetSyncCallback();
        EXPECT_EQ(DLI_SUCCESS, DLI_Init());
        EXPECT_EQ(DLI_SUCCESS, DLI_Enable());
        EXPECT_NE(0, DLI_ThreadEvcHandleGet());
    }

    void TearDown()
    {
        DLI_Disable();
        DLI_DeInit();
        DLI_SetCallback(NULL);
    }
};

// 校验命令超时通知：完成事件超时与状态事件超时都会带上超时状态码和原命令号回调上层
TEST_F(UT_DLI_CE_LIGHT, TestCaseTimeoutCallback)
{
    static DLI_InnerCbkLineStru errTable[] = {{DLI_CMD_ERROR_EVT, CeErrInner}};
    ASSERT_EQ(DLI_SUCCESS, DLI_InnerEventCbkReg(errTable, 1));

    DLI_CmdStru info;
    (void)memset_s(&info, sizeof(info), 0, sizeof(info));
    info.cmd = DLI_CREATE_CONNECTION;
    DLI_CmdTxNode node;
    (void)memset_s(&node, sizeof(node), 0, sizeof(node));
    node.info = &info;

    info.event = DLI_CMD_COMPLETE_EVT;
    TimeoutCallback(&node);
    EXPECT_EQ(1, g_ceErrInnerCount);
    EXPECT_EQ(DLI_COMMAND_TIMEOUT, g_ceErrStatus);
    EXPECT_EQ(DLI_CREATE_CONNECTION, g_ceErrCmd);

    info.event = CE_TEST_EVT_FINAL;
    node.isRecvStatusEvt = false;
    TimeoutCallback(&node);
    EXPECT_EQ(2, g_ceErrInnerCount);

    node.isRecvStatusEvt = true;
    TimeoutCallback(&node);
    EXPECT_EQ(3, g_ceErrInnerCount);

    TimeoutCallback(NULL);
    EXPECT_EQ(3, g_ceErrInnerCount);
    DLI_InnerEventCbkUnReg(errTable, 1);
}

// 校验内部事件表注册注销的参数检查与失败清理：空表、零长度、内存不足、投递失败都返回错误
TEST_F(UT_DLI_CE_LIGHT, TestCaseInnerRegUnregParam)
{
    static DLI_InnerCbkLineStru table[] = {{CE_TEST_EVT_INNER_1, CeInnerA}};
    EXPECT_EQ(DLI_INVALID_PARAMETERS, DLI_InnerEventCbkReg(NULL, 1));
    EXPECT_EQ(DLI_INVALID_PARAMETERS, DLI_InnerEventCbkReg(table, 0));

    // 节点申请失败
    DhMemHooksInstall();
    g_dhFailAt = 1;
    EXPECT_EQ(DLI_STACK_MEM_ERRNO, DLI_InnerEventCbkReg(table, 1));
    DhMemHooksRestore();
    // 表内存申请失败
    DhMemHooksInstall();
    g_dhFailAt = 2;
    EXPECT_EQ(DLI_STACK_MEM_ERRNO, DLI_InnerEventCbkReg(table, 1));
    DhMemHooksRestore();

    g_cePostFail = true;
    EXPECT_EQ(DLI_STACK_MEM_ERRNO, DLI_InnerEventCbkReg(table, 1));
    g_cePostFail = false;

    EXPECT_EQ(DLI_SUCCESS, DLI_InnerEventCbkReg(table, 1));
    DLI_InnerEventCbkUnReg(NULL, 1);
    DLI_InnerEventCbkUnReg(table, 0);
    DLI_InnerEventCbkUnReg(table, 1);
}

// 校验内部事件表按内容注销：注销注册过的表后对应事件失效，未注销的表继续生效
TEST_F(UT_DLI_CE_LIGHT, TestCaseInnerUnregByContent)
{
    static DLI_InnerCbkLineStru tableA[] = {{CE_TEST_EVT_INNER_1, CeInnerA}};
    static DLI_InnerCbkLineStru tableB[] = {{CE_TEST_EVT_INNER_2, CeInnerB}};
    ASSERT_EQ(DLI_SUCCESS, DLI_InnerEventCbkReg(tableA, 1));
    ASSERT_EQ(DLI_SUCCESS, DLI_InnerEventCbkReg(tableB, 1));
    uint8_t evtData[CE_TEST_SHORT_LEN] = {0};

    evtData[0] = CE_TEST_EVT_INNER_1 & 0xFF;
    evtData[1] = CE_TEST_EVT_INNER_1 >> 8;
    RecvEventHandler(CE_TEST_EVT_INNER_1, NULL, evtData, CE_TEST_SHORT_LEN);
    EXPECT_EQ(1, g_ceInnerACount);

    DLI_InnerEventCbkUnReg(tableA, 1);
    RecvEventHandler(CE_TEST_EVT_INNER_1, NULL, evtData, CE_TEST_SHORT_LEN);
    EXPECT_EQ(1, g_ceInnerACount);

    evtData[0] = CE_TEST_EVT_INNER_2 & 0xFF;
    evtData[1] = CE_TEST_EVT_INNER_2 >> 8;
    RecvEventHandler(CE_TEST_EVT_INNER_2, NULL, evtData, CE_TEST_SHORT_LEN);
    EXPECT_EQ(1, g_ceInnerBCount);
    DLI_InnerEventCbkUnReg(tableB, 1);
}

// 校验发送完成事件回调注册：非法模块号或空回调被拒绝，合法注册后能收到连接号与包数
TEST_F(UT_DLI_CE_LIGHT, TestCaseNocpRegUnreg)
{
    EXPECT_EQ(DLI_INVALID_PARAMETERS, DLI_RegNOCPEventCbk(DLI_REG_MODULE_MAX, CeNocp));
    EXPECT_EQ(DLI_INVALID_PARAMETERS, DLI_RegNOCPEventCbk((DLI_RegModuleType)-1, CeNocp));
    EXPECT_EQ(DLI_INVALID_PARAMETERS, DLI_RegNOCPEventCbk(DLI_REG_MODULE_DTAP, NULL));

    EXPECT_EQ(DLI_SUCCESS, DLI_RegNOCPEventCbk(DLI_REG_MODULE_DTAP, CeNocp));
    DLI_NumberOfCompletedPacketsEvt ev;
    ev.connHandle = 0x0102;
    ev.numCompletedPackets = 3;
    DLI_NumberOfCompletedPacketsCbk(NULL, NULL, sizeof(ev), 0);
    DLI_NumberOfCompletedPacketsCbk(NULL, &ev, sizeof(ev) - 1, 0);
    EXPECT_EQ(0, g_ceNocpCount);

    DLI_NumberOfCompletedPacketsCbk(NULL, &ev, sizeof(ev), 0);
    EXPECT_EQ(1, g_ceNocpCount);
    EXPECT_EQ(0x0102, g_ceNocpHandle);
    EXPECT_EQ(3, g_ceNocpNum);

    EXPECT_EQ(DLI_SUCCESS, DLI_UnregNOCPEventCbk(DLI_REG_MODULE_DTAP));
    DLI_NumberOfCompletedPacketsCbk(NULL, &ev, sizeof(ev), 0);
    EXPECT_EQ(1, g_ceNocpCount);
    EXPECT_EQ(DLI_INVALID_PARAMETERS, DLI_UnregNOCPEventCbk(DLI_REG_MODULE_MAX));
    EXPECT_EQ(DLI_INVALID_PARAMETERS, DLI_UnregNOCPEventCbk((DLI_RegModuleType)-1));
}

// 校验命令状态事件处理：空指针与短包直接返回，状态成功只更新并发数，状态失败转错误事件分发
TEST_F(UT_DLI_CE_LIGHT, TestCaseCommandStatusCbk)
{
    static DLI_InnerCbkLineStru errTable[] = {{DLI_CMD_ERROR_EVT, CeErrInner}};
    ASSERT_EQ(DLI_SUCCESS, DLI_InnerEventCbkReg(errTable, 1));

    DLI_CommandStatus cs;
    (void)memset_s(&cs, sizeof(cs), 0, sizeof(cs));
    cs.cmdOpcode = CE_TEST_CMD_CANNED;
    cs.numDliCommandPackets = 2;
    cs.status = DLI_SUCCESS;
    DLI_CommandStatusCbk(NULL, NULL, sizeof(cs), 0);
    DLI_CommandStatusCbk(NULL, &cs, CE_TEST_SHORT_LEN, 0);
    EXPECT_EQ(0, g_ceErrInnerCount);

    DLI_CommandStatusCbk(NULL, &cs, sizeof(cs), 0);
    EXPECT_EQ(0, g_ceErrInnerCount);

    cs.status = 9;
    DLI_CommandStatusCbk(NULL, &cs, sizeof(cs), 0);
    EXPECT_EQ(1, g_ceErrInnerCount);
    EXPECT_EQ(9, g_ceErrStatus);
    EXPECT_EQ(CE_TEST_CMD_CANNED, g_ceErrCmd);
    DLI_InnerEventCbkUnReg(errTable, 1);
}

// 校验事件入口的命令号解析：包长不足两字节时不解析命令号，完成事件与无效事件的命令号原样透传
TEST_F(UT_DLI_CE_LIGHT, TestCaseRecvEvtOpcodeDecode)
{
    static DLI_InnerCbkLineStru table[] = {
        {0, CeInnerA},
        {CE_TEST_CMD_SCAN, CeInnerB},
    };
    ASSERT_EQ(DLI_SUCCESS, DLI_InnerEventCbkReg(table, 2));
    uint8_t shortData[1] = {0};
    uint8_t fullData[CE_TEST_SHORT_LEN] = {0};
    fullData[0] = CE_TEST_CMD_SCAN & 0xFF;
    fullData[1] = CE_TEST_CMD_SCAN >> 8;

    int ctx = 0;
    RecvEventHandler(DLI_CMD_COMPLETE_EVT, &ctx, shortData, 1);
    EXPECT_EQ(1, g_ceInnerACount);
    RecvEventHandler(DLI_CMD_COMPLETE_EVT, &ctx, fullData, CE_TEST_SHORT_LEN);
    EXPECT_EQ(1, g_ceInnerACount);
    EXPECT_EQ(1, g_ceInnerBCount);

    RecvEventHandler(DLI_INVALID_EVT, &ctx, fullData, CE_TEST_SHORT_LEN);
    EXPECT_EQ(2, g_ceInnerBCount);

    RecvEventHandler(DLI_CMD_COMPLETE_EVT, NULL, fullData, CE_TEST_SHORT_LEN);
    RecvEventHandler(DLI_CMD_COMPLETE_EVT, &ctx, NULL, CE_TEST_SHORT_LEN);
    RecvEventHandler(DLI_CMD_COMPLETE_EVT, &ctx, fullData, 0);
    EXPECT_EQ(2, g_ceInnerBCount);
    DLI_InnerEventCbkUnReg(table, 2);
}

// 校验内部事件分发的空回调表项：表项函数为空时跳过该表项继续查找，不误伤后续注册
TEST_F(UT_DLI_CE_LIGHT, TestCaseInnerDispatchNullFunc)
{
    static DLI_InnerCbkLineStru table[] = {
        {CE_TEST_EVT_B, NULL},
        {CE_TEST_EVT_C, CeInnerB},
    };
    ASSERT_EQ(DLI_SUCCESS, DLI_InnerEventCbkReg(table, 2));
    uint8_t evtData[CE_TEST_SHORT_LEN] = {0};

    evtData[0] = CE_TEST_EVT_B & 0xFF;
    evtData[1] = CE_TEST_EVT_B >> 8;
    RecvEventHandler(CE_TEST_EVT_B, NULL, evtData, CE_TEST_SHORT_LEN);
    EXPECT_EQ(0, g_ceInnerBCount);

    evtData[0] = CE_TEST_EVT_C & 0xFF;
    evtData[1] = CE_TEST_EVT_C >> 8;
    RecvEventHandler(CE_TEST_EVT_C, NULL, evtData, CE_TEST_SHORT_LEN);
    EXPECT_EQ(1, g_ceInnerBCount);
    DLI_InnerEventCbkUnReg(table, 2);
}

// 校验链路未初始化时公共配置读取失败：读缓冲命令发不出去，配置接口返回读取失败的错误码
TEST_F(UT_DLI_CE_LIGHT, TestCaseReadCommConfigFail)
{
    EXPECT_EQ(DLI_STACK_READ_COMM_CONFIG_VAL_ERRNO, DLI_ReadCommConfigValue());
}

// 校验扫描参数命令的帧类型分档：空参数被拒绝，帧类型一与帧类型四分别按各自分档组包发送
TEST_F(UT_DLI_CE_LIGHT, TestCaseSetScanParamFrameType)
{
    EXPECT_EQ(DLI_STACK_PARAMS_ERRNO, DLI_SetScanParam(NULL));

    uint8_t raw[sizeof(DLI_ScanParam) + 2 * sizeof(DLI_ScanParamCoreNoPhy)] = {0};
    DLI_ScanParam *param = (DLI_ScanParam *)raw;
    param->frameFormatInd = SCAN_FRAME_TYPE_1;
    param->param[0].scanType = 1;
    param->param[0].scanInterval = 100;
    param->param[0].scanWindow = 50;
    EXPECT_NE(DLI_SUCCESS, DLI_SetScanParam(param));

    param->frameFormatInd = SCAN_FRAME_TYPE_4;
    EXPECT_NE(DLI_SUCCESS, DLI_SetScanParam(param));

    param->frameFormatInd = SCAN_FRAME_TYPE_1 | SCAN_FRAME_TYPE_4;
    param->param[1].scanType = 0;
    param->param[1].scanInterval = 200;
    param->param[1].scanWindow = 100;
    EXPECT_NE(DLI_SUCCESS, DLI_SetScanParam(param));
    EXPECT_EQ(0, DLI_GetPhyCountByFrameType(0));
    EXPECT_EQ(1, DLI_GetPhyCountByFrameType(SCAN_FRAME_TYPE_1));
    EXPECT_EQ(2, DLI_GetPhyCountByFrameType(SCAN_FRAME_TYPE_1 | SCAN_FRAME_TYPE_4));
}

// 校验完成事件的完整处理链：先收状态事件再收完成事件后，命令节点被清理且上层收到状态与净荷
TEST_F(UT_DLI_CE_FULL, TestCaseCommandCompleteChain)
{
    DLI_CmdParams params;
    (void)memset_s(&params, sizeof(params), 0, sizeof(params));
    params.cmd = CE_TEST_CMD_CANNED;
    params.event = DLI_CMD_COMPLETE_EVT;
    params.cbk = CeExecCbk;
    ASSERT_EQ(DLI_SUCCESS, DLI_SetCmd(&params));
    SDF_ThreadSleep(CE_TEST_SLEEP_MS);

    uint8_t statusPkt[8] = {0};
    DLI_ENCODE2BYTE(statusPkt, DLI_CMD_STATUS_EVT);
    DLI_ENCODE2BYTE(statusPkt + 4, CE_TEST_CMD_CANNED);
    SlePacket statusPacket = {statusPkt, sizeof(statusPkt)};
    statusPkt[6] = 2;
    statusPkt[7] = DLI_SUCCESS;
    SleSendToDliStub(PACKET_TYPE_SLE_EVENT, &statusPacket);
    SDF_ThreadSleep(CE_TEST_SLEEP_MS);

    uint8_t completePkt[10] = {0};
    DLI_ENCODE2BYTE(completePkt, DLI_CMD_COMPLETE_EVT);
    DLI_ENCODE2BYTE(completePkt + 4, CE_TEST_CMD_CANNED);
    completePkt[6] = 1;
    completePkt[7] = DLI_SUCCESS;
    completePkt[8] = 0x5A;
    completePkt[9] = 0xA5;
    SlePacket completePacket = {completePkt, sizeof(completePkt)};
    SleSendToDliStub(PACKET_TYPE_SLE_EVENT, &completePacket);
    SDF_ThreadSleep(CE_TEST_SLEEP_MS);

    EXPECT_EQ(1, g_ceExecCount);
    EXPECT_EQ(DLI_SUCCESS, g_ceExecStatus);
    EXPECT_EQ(CE_TEST_CMD_CANNED, g_ceExecOpcode);
    EXPECT_EQ(2, g_ceExecSize);
}

// 校验完成事件回调的容错：上下文回调为空不崩溃，失败状态不带净荷，短包不带净荷，正常包带净荷
TEST_F(UT_DLI_CE_FULL, TestCaseCompleteEvtContextTolerance)
{
    DLI_CmdParams params;
    (void)memset_s(&params, sizeof(params), 0, sizeof(params));
    params.cmd = CE_TEST_CMD_SCAN;
    params.event = DLI_CMD_COMPLETE_EVT;
    params.cbk = CeExecCbk;
    ASSERT_EQ(DLI_SUCCESS, DLI_SetCmd(&params));
    SDF_ThreadSleep(CE_TEST_SLEEP_MS);

    DLI_ManagerContext nullCbkCtx;
    (void)memset_s(&nullCbkCtx, sizeof(nullCbkCtx), 0, sizeof(nullCbkCtx));
    nullCbkCtx.cbk = NULL;
    uint8_t evtData[6] = {0};
    DLI_ENCODE2BYTE(evtData, CE_TEST_CMD_SCAN);
    evtData[2] = 1;
    evtData[3] = DLI_SUCCESS;
    RecvEventHandler(DLI_CMD_COMPLETE_EVT, &nullCbkCtx, evtData, 4);
    EXPECT_EQ(0, g_ceExecCount);

    evtData[3] = 7;
    RecvEventHandler(DLI_CMD_COMPLETE_EVT, &nullCbkCtx, evtData, 4);
    EXPECT_EQ(0, g_ceExecCount);

    DLI_ManagerContext execCtx;
    (void)memset_s(&execCtx, sizeof(execCtx), 0, sizeof(execCtx));
    execCtx.cbk = CeExecCbk;
    evtData[3] = 7;
    RecvEventHandler(DLI_CMD_COMPLETE_EVT, &execCtx, evtData, 4);
    EXPECT_EQ(1, g_ceExecCount);
    EXPECT_EQ(7, g_ceExecStatus);
    EXPECT_EQ(0, g_ceExecSize);

    evtData[4] = 0x33;
    evtData[5] = 0x44;
    evtData[3] = DLI_SUCCESS;
    RecvEventHandler(DLI_CMD_COMPLETE_EVT, &execCtx, evtData, 6);
    EXPECT_EQ(2, g_ceExecCount);
    EXPECT_EQ(DLI_SUCCESS, g_ceExecStatus);
    EXPECT_EQ(2, g_ceExecSize);

    uint8_t shortEvt[4] = {0};
    DLI_ENCODE2BYTE(shortEvt, CE_TEST_CMD_SCAN);
    RecvEventHandler(DLI_CMD_COMPLETE_EVT, &execCtx, shortEvt, 3);
    EXPECT_EQ(3, g_ceExecCount);
    EXPECT_EQ(0, g_ceExecSize);
}
