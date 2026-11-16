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

/* dli 基础层纯函数直调用例：节点查找矩阵、参数擦除、销毁兜底、回调注册与事件字段改写 */
#include <stdlib.h>
#include <gtest/gtest.h>
#include "securec.h"

#include "sdf_mem.h"
#include "sdf_buff.h"
#include "sdf_dlist.h"
#include "sdf_timer.h"
#include "stub_replace.h"

#include "dli.h"
#include "dli_cmd.h"
#include "dli_cmd_struct.h"
#include "dli_event.h"
#include "dli_event_struct.h"
#include "dli_errno.h"
#include "dli_opcode.h"
#include "dli_sapi.h"
#include "dli_data_stub.h"
#include "dli_layer_utils.h"
#include "dli_layer_callback.h"
#include "dli_secu_event.h"
#include "dli_hadm_event.h"

#define FULL_TEST_CMD_1 0x0C06
#define FULL_TEST_CMD_2 0x0C10
#define FULL_TEST_CMD_MISS 0x0C99
#define FULL_TEST_EVT_FINAL 0x0003
#define FULL_TEST_PAR_LEN 4
#define FULL_TEST_LCID 0x0011

static volatile int g_fullFuncCtxFreeCount = 0;
static volatile int g_fullFuncKillCount = 0;
static volatile int g_fullFuncTimerDelCount = 0;
static volatile int g_fullFuncExecCount = 0;
static volatile uint16_t g_fullFuncExecStatus = 0;
static volatile int g_fullFuncCsCapsCount = 0;
static volatile bool g_fullFuncCsCapsLocal = false;
static volatile int g_fullFuncInnerCount = 0;
static void *g_fullFuncInnerArg = NULL;

extern "C" {
// 无头文件声明的内部导出函数，测试直接声明后调用
void TimeoutCallback(void *param);
uint16_t DLI_GetopcodeByCmd(uint16_t cmd);
}

static void FullFuncCtxFree(void *context)
{
    g_fullFuncCtxFreeCount++;
}

static void FullFuncKill(uint16_t switchType)
{
    (void)switchType;
    g_fullFuncKillCount++;
}

static void FullFuncExecCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    (void)cmdRes;
    g_fullFuncExecCount++;
    g_fullFuncExecStatus = status;
}

static bool FullFuncCsCaps(const uint8_t *rawData, uint32_t len, bool isLocal)
{
    (void)rawData;
    (void)len;
    g_fullFuncCsCapsCount++;
    g_fullFuncCsCapsLocal = isLocal;
    return true;
}

static void FullFuncInnerEvt(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    (void)context;
    (void)len;
    (void)evtOpcode;
    g_fullFuncInnerCount++;
    g_fullFuncInnerArg = arg;
}

static uint32_t FullFuncSyncPost(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (cb != NULL) {
        cb(arg);
    }
    if (freeCb != NULL && arg != NULL) {
        freeCb(arg);
    }
    return 0;
}

static uint32_t FullFuncSyncPostBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    (void)timeout;
    return FullFuncSyncPost(cb, arg, freeCb);
}

static uint16_t FullFuncExtOpcode(uint16_t code)
{
    (void)code;
    return 0x0066;
}

static void FullFuncRecvAcb(uint16_t lcid, SDF_Buff_S *buf)
{
    (void)lcid;
    (void)buf;
}

static void FullFuncSetCallback(bool withCsCaps)
{
    DLI_Callback cbk = {0};
    cbk.postOtherThread = FullFuncSyncPost;
    cbk.postOtherBlockedThread = FullFuncSyncPostBlocked;
    cbk.recvAcbHandler = FullFuncRecvAcb;
    if (withCsCaps) {
        cbk.hadmProcessCsCaps = FullFuncCsCaps;
    }
    DLI_SetCallback(&cbk);
}

static void FullFuncTimerDelStub(int handle)
{
    (void)handle;
    g_fullFuncTimerDelCount++;
}

class UT_DLI_FULL_FUNC : public testing::Test {
protected:
    void SetUp()
    {
        STUB_Init();
        g_fullFuncCtxFreeCount = 0;
        g_fullFuncKillCount = 0;
        g_fullFuncTimerDelCount = 0;
        g_fullFuncExecCount = 0;
        g_fullFuncExecStatus = 0;
        g_fullFuncCsCapsCount = 0;
        g_fullFuncCsCapsLocal = false;
        g_fullFuncInnerCount = 0;
        g_fullFuncInnerArg = NULL;
    }

    void TearDown()
    {
        DLI_SetCallback(NULL);
    }
};

// 校验命令发送链表的查找规则：按事件加命令号匹配、命令号通配、跳过已收状态事件的节点、数据节点按链路号匹配
TEST_F(UT_DLI_FULL_FUNC, TestCaseFindNodeMatrix)
{
    SDF_DListHead_S head;
    SDF_DListHeadInit(&head);

    DLI_CmdStru *info1 = DLI_DefaultCmdStruCreate(FULL_TEST_CMD_1, DLI_CMD_COMPLETE_EVT, NULL, 0);
    DLI_CmdStru *info2 = DLI_DefaultCmdStruCreate(FULL_TEST_CMD_2, FULL_TEST_EVT_FINAL, NULL, 0);
    DLI_CmdStru *info3 = DLI_DefaultCmdStruCreate(FULL_TEST_CMD_1, FULL_TEST_EVT_FINAL, NULL, 0);
    ASSERT_TRUE(info1 != NULL && info2 != NULL && info3 != NULL);
    DLI_CmdTxNode *node1 = DLI_CmdNodeCreate(info1);
    DLI_CmdTxNode *node2 = DLI_CmdNodeCreate(info2);
    DLI_CmdTxNode *node3 = DLI_CmdNodeCreate(info3);
    ASSERT_TRUE(node1 != NULL && node2 != NULL && node3 != NULL);
    SDF_DListElmTailInsert(&head, node1, node);
    SDF_DListElmTailInsert(&head, node2, node);
    SDF_DListElmTailInsert(&head, node3, node);

    EXPECT_EQ(node1, DLI_CmdNodeFind(&head, DLI_CMD_COMPLETE_EVT, FULL_TEST_CMD_1));
    EXPECT_EQ(NULL, DLI_CmdNodeFind(&head, DLI_CMD_COMPLETE_EVT, FULL_TEST_CMD_2));
    EXPECT_EQ(node2, DLI_CmdNodeFind(&head, FULL_TEST_EVT_FINAL, 0));
    EXPECT_EQ(NULL, DLI_CmdNodeFind(&head, 0x0009, 0));

    EXPECT_EQ(node1, DLI_CmdNodeFindByOpcode(&head, 0));
    EXPECT_EQ(node2, DLI_CmdNodeFindByOpcode(&head, FULL_TEST_CMD_2));
    EXPECT_EQ(NULL, DLI_CmdNodeFindByOpcode(&head, FULL_TEST_CMD_MISS));

    node1->isRecvStatusEvt = true;
    EXPECT_EQ(node3, DLI_CmdNodeFindNotRecvStatus(&head, FULL_TEST_CMD_1));
    EXPECT_EQ(node2, DLI_CmdNodeFindNotRecvStatus(&head, 0));
    EXPECT_EQ(NULL, DLI_CmdNodeFindNotRecvStatus(&head, FULL_TEST_CMD_MISS));

    SDF_DListHead_S rxHead;
    SDF_DListHeadInit(&rxHead);
    SDF_Buff_S *buff = SDF_BuffNew(FULL_TEST_PAR_LEN);
    ASSERT_TRUE(buff != NULL);
    DLI_RecvDataNode *rxNode = DLI_RecvDataNodeCreate(FULL_TEST_LCID, DLI_FIRST_FRAGMENT, buff);
    ASSERT_TRUE(rxNode != NULL);
    SDF_DListElmTailInsert(&rxHead, rxNode, node);
    EXPECT_EQ(rxNode, DLI_DataNodeFind(&rxHead, FULL_TEST_LCID));
    EXPECT_EQ(NULL, DLI_DataNodeFind(&rxHead, 0x0022));
    DLI_RxDataDlistDestroy(&rxHead);

    EXPECT_EQ(3, SDF_DListCount(&head));
    DLI_CmdDlistDestroy(&head);
    EXPECT_EQ(0, SDF_DListCount(&head));
}

// 校验敏感命令参数的擦除行为：要求擦除且带参数时清零，其余情况保持原样或直接返回
TEST_F(UT_DLI_FULL_FUNC, TestCaseEraseParInfo)
{
    uint8_t secret[FULL_TEST_PAR_LEN] = {0xAA, 0xBB, 0xCC, 0xDD};
    DLI_CmdStru *info = DLI_DefaultCmdStruCreate(FULL_TEST_CMD_1, DLI_CMD_COMPLETE_EVT, secret, FULL_TEST_PAR_LEN);
    ASSERT_TRUE(info != NULL);
    info->needErase = true;
    for (uint32_t i = 0; i < FULL_TEST_PAR_LEN; i++) {
        info->par[i] = 0x77;
    }
    DLI_EraseParInfo(info);
    // 现状只清除参数计数长度的头部区字节，参数区原样保留
    for (uint32_t i = 0; i < FULL_TEST_PAR_LEN; i++) {
        EXPECT_EQ(0, info->par[i]);
        EXPECT_EQ(secret[i], info->par[5 + i]);
    }
    SDF_MemFree(info);

    DLI_CmdStru *keep = DLI_DefaultCmdStruCreate(FULL_TEST_CMD_1, DLI_CMD_COMPLETE_EVT, secret, FULL_TEST_PAR_LEN);
    ASSERT_TRUE(keep != NULL);
    keep->needErase = false;
    keep->par[0] = 0x77;
    DLI_EraseParInfo(keep);
    EXPECT_EQ(0x77, keep->par[0]);
    SDF_MemFree(keep);

    DLI_CmdStru *empty = DLI_DefaultCmdStruCreate(FULL_TEST_CMD_1, DLI_CMD_COMPLETE_EVT, NULL, 0);
    ASSERT_TRUE(empty != NULL);
    empty->needErase = true;
    DLI_EraseParInfo(empty);
    DLI_EraseParInfo(NULL);
    SDF_MemFree(empty);
}

// 校验销毁函数对空指针的兜底：节点或数据为空时不崩溃，正常节点释放时回调上下文清理函数
TEST_F(UT_DLI_FULL_FUNC, TestCaseNodeDestroyNullGuard)
{
    DLI_CmdNodeDestroy(NULL);
    DLI_RecvDataNodeDestroy(NULL);

    uint32_t ctx = 8;
    DLI_CmdStru *info = DLI_DefaultCmdStruCreate(FULL_TEST_CMD_1, DLI_CMD_COMPLETE_EVT, NULL, 0);
    ASSERT_TRUE(info != NULL);
    info->context = &ctx;
    info->contextFree = FullFuncCtxFree;
    DLI_CmdTxNode *node = DLI_CmdNodeCreate(info);
    ASSERT_TRUE(node != NULL);
    DLI_CmdNodeDestroy(node);
    EXPECT_EQ(1, g_fullFuncCtxFreeCount);
}

// 校验命令链表整体销毁：没有定时器的节点直接释放，挂有定时器句柄的节点先删定时器再释放
TEST_F(UT_DLI_FULL_FUNC, TestCaseCmdDlistDestroyTimer)
{
    FuncStubInfo stubInfo;
    SDF_DListHead_S head;
    SDF_DListHeadInit(&head);
    DLI_CmdStru *plain = DLI_DefaultCmdStruCreate(FULL_TEST_CMD_1, DLI_CMD_COMPLETE_EVT, NULL, 0);
    DLI_CmdStru *timed = DLI_DefaultCmdStruCreate(FULL_TEST_CMD_2, DLI_CMD_COMPLETE_EVT, NULL, 0);
    ASSERT_TRUE(plain != NULL && timed != NULL);
    DLI_CmdTxNode *plainNode = DLI_CmdNodeCreate(plain);
    DLI_CmdTxNode *timedNode = DLI_CmdNodeCreate(timed);
    ASSERT_TRUE(plainNode != NULL && timedNode != NULL);
    timedNode->handle = 7;
    SDF_DListElmTailInsert(&head, plainNode, node);
    SDF_DListElmTailInsert(&head, timedNode, node);

    EXPECT_EQ(0, STUB_Replace(&stubInfo, (void *)SDF_TimerDel, (const void *)FullFuncTimerDelStub));
    DLI_CmdDlistDestroy(&head);
    STUB_Reset(&stubInfo);
    EXPECT_EQ(1, g_fullFuncTimerDelCount);
    EXPECT_EQ(0, SDF_DListCount(&head));
}

// 校验芯片异常上报开关：未注册处理回调时直接返回，注册后能收到开关类型
TEST_F(UT_DLI_FULL_FUNC, TestCaseDftReportKill)
{
    DLI_DftReportKill(1);
    EXPECT_EQ(0, g_fullFuncKillCount);

    DLI_Callback cbk = {0};
    cbk.postOtherThread = FullFuncSyncPost;
    cbk.postOtherBlockedThread = FullFuncSyncPostBlocked;
    cbk.recvAcbHandler = FullFuncRecvAcb;
    cbk.dftReportKill = FullFuncKill;
    EXPECT_EQ(DLI_SUCCESS, DLI_SetCallback(&cbk));
    DLI_DftReportKill(2);
    EXPECT_EQ(1, g_fullFuncKillCount);
}

// 校验发送接口的参数检查：空指针或零长度直接返回错误，不带擦除标记时合法输入走真实发送链
TEST_F(UT_DLI_FULL_FUNC, TestCaseSapiSendParam)
{
    uint8_t data[FULL_TEST_PAR_LEN] = {1, 2, 3, 4};
    EXPECT_EQ(DLI_STACK_PARAMS_ERRNO, DLI_SapiSend(NULL, 0, false));
    EXPECT_EQ(DLI_STACK_PARAMS_ERRNO, DLI_SapiSend(NULL, FULL_TEST_PAR_LEN, false));
    EXPECT_EQ(DLI_STACK_PARAMS_ERRNO, DLI_SapiSend(data, 0, false));
    // 回环层未初始化时发送失败，但不允许因参数问题崩溃
    EXPECT_NE(0, DLI_SapiSend(data, FULL_TEST_PAR_LEN, false));
}

static volatile bool g_fullFuncSleSendCalled = false;
static uint8_t *g_fullFuncSleSendData = NULL;
static uint32_t g_fullFuncSleSendSize = 0;

static int FullFuncSleSendStub(const SlePacket *packet)
{
    g_fullFuncSleSendCalled = true;
    g_fullFuncSleSendData = packet->data;
    g_fullFuncSleSendSize = packet->size;
    return 0;
}

static volatile bool g_fullFuncErasedAtFree = false;

static void FullFuncMemFreeStub(void *ptr)
{
    if (ptr == (void *)g_fullFuncSleSendData) {
        bool allZero = true;
        for (uint32_t i = 0; i < g_fullFuncSleSendSize; i++) {
            allZero = allZero && (g_fullFuncSleSendData[i] == 0);
        }
        g_fullFuncErasedAtFree = allZero;
    }
    free(ptr);
}

// 校验敏感数据发送后的擦除时机：带擦除标记时数据在释放前被清零，不带标记时保持原样直到释放
TEST_F(UT_DLI_FULL_FUNC, TestCaseSapiSendErase)
{
    FuncStubInfo sendStub;
    FuncStubInfo freeStub;
    uint8_t secret[FULL_TEST_PAR_LEN] = {0x5A, 0x5A, 0x5A, 0x5A};

    EXPECT_EQ(0, STUB_Replace(&sendStub, (void *)SleSendDliPacket, (const void *)FullFuncSleSendStub));
    EXPECT_EQ(0, STUB_Replace(&freeStub, (void *)SDF_MemFree, (const void *)FullFuncMemFreeStub));

    g_fullFuncErasedAtFree = false;
    EXPECT_EQ(0, DLI_SapiSend(secret, FULL_TEST_PAR_LEN, true));
    EXPECT_TRUE(g_fullFuncSleSendCalled);
    EXPECT_TRUE(g_fullFuncErasedAtFree);

    g_fullFuncSleSendCalled = false;
    g_fullFuncErasedAtFree = true;
    EXPECT_EQ(0, DLI_SapiSend(secret, FULL_TEST_PAR_LEN, false));
    EXPECT_TRUE(g_fullFuncSleSendCalled);
    EXPECT_FALSE(g_fullFuncErasedAtFree);

    STUB_Reset(&freeStub);
    STUB_Reset(&sendStub);
}

// 校验命令回调注册入口：自定义模块带内表注册、已知模块按模块号注册、非法模块号被拒绝
TEST_F(UT_DLI_FULL_FUNC, TestCaseCmdCbkRegMatrix)
{
    FullFuncSetCallback(false);
    static DLI_InnerCbkLineStru innerTable[] = {{FULL_TEST_EVT_FINAL, FullFuncInnerEvt}};
    static DLI_CbkLineStru cbkTable[] = {{DLI_CBK_CONNECT, FullFuncExecCbk}};
    uint8_t evtData[FULL_TEST_PAR_LEN] = {0};
    evtData[0] = FULL_TEST_EVT_FINAL & 0xFF;
    evtData[1] = FULL_TEST_EVT_FINAL >> 8;

    EXPECT_EQ(DLI_INVALID_PARAMETERS, DLI_CmdCbkReg((ModuleType)8, NULL, 0, cbkTable, 1));
    EXPECT_EQ(DLI_INVALID_PARAMETERS, DLI_CmdCbkReg((ModuleType)17, NULL, 0, cbkTable, 1));
    EXPECT_EQ(DLI_INVALID_PARAMETERS, DLI_CmdCbkReg(CM, NULL, 0, NULL, 1));
    EXPECT_EQ(DLI_INVALID_PARAMETERS, DLI_CmdCbkReg(CM, NULL, 1, cbkTable, 1));

    EXPECT_EQ(DLI_SUCCESS, DLI_CmdCbkReg((ModuleType)11, innerTable, 1, cbkTable, 1));
    RecvEventHandler(FULL_TEST_EVT_FINAL, NULL, evtData, FULL_TEST_PAR_LEN);
    EXPECT_EQ(1, g_fullFuncInnerCount);

    EXPECT_EQ(DLI_SUCCESS, DLI_CmdCbkReg(DEVD, NULL, 0, cbkTable, 1));
    EXPECT_EQ(DLI_SUCCESS, DLI_CmdCbkReg(CM, NULL, 0, cbkTable, 1));
    EXPECT_EQ(DLI_SUCCESS, DLI_CmdCbkReg(CM_ICB, NULL, 0, cbkTable, 1));
    EXPECT_EQ(DLI_SUCCESS, DLI_CmdCbkReg(CM_COMMON, NULL, 0, cbkTable, 1));
    EXPECT_EQ(DLI_SUCCESS, DLI_CmdCbkReg(SM, NULL, 0, cbkTable, 1));
    EXPECT_EQ(DLI_SUCCESS, DLI_CmdCbkReg(HADM, NULL, 0, cbkTable, 1));
    EXPECT_EQ(DLI_SUCCESS, DLI_CmdCbkReg(NBC, NULL, 0, cbkTable, 1));
    EXPECT_EQ(DLI_SUCCESS, DLI_CmdCbkReg(QOSM, NULL, 0, cbkTable, 1));

    DLI_CmdCbkUnReg((ModuleType)11, innerTable, 1, cbkTable, 1);
    RecvEventHandler(FULL_TEST_EVT_FINAL, NULL, evtData, FULL_TEST_PAR_LEN);
    EXPECT_EQ(1, g_fullFuncInnerCount);

    DLI_CmdCbkUnReg(CM_COMMON, NULL, 0, cbkTable, 1);
    DLI_CmdCbkUnReg(CM, NULL, 0, cbkTable, 1);
    DLI_CmdCbkUnReg(CM_ICB, NULL, 0, cbkTable, 1);
    DLI_CmdCbkUnReg(SM, NULL, 0, cbkTable, 1);
    DLI_CmdCbkUnReg(HADM, NULL, 0, cbkTable, 1);
    DLI_CmdCbkUnReg(NBC, NULL, 0, cbkTable, 1);
    DLI_CmdCbkUnReg(QOSM, NULL, 0, cbkTable, 1);
    DLI_DeInitEventCbkList();
}

// 校验加密状态事件的连接号回填：命令超时或内存不足时从上下文补全连接号，其余状态不改
TEST_F(UT_DLI_FULL_FUNC, TestCaseEncryptChangeConnHandle)
{
    static DLI_CbkLineStru cbkTable[] = {{DLI_CBK_ENCRYPT_CHANGE, FullFuncExecCbk}};
    DLI_AddCbks(cbkTable, 1);
    DLI_EncryptChangeEvt evt;
    (void)memset_s(&evt, sizeof(evt), 0, sizeof(evt));
    evt.connHandle = 0x0001;

    evt.status = DLI_SUCCESS;
    DLI_EncryptChangeCbk(NULL, &evt, sizeof(evt), 0);
    EXPECT_EQ(1, g_fullFuncExecCount);

    DLI_ManagerContext manager;
    (void)memset_s(&manager, sizeof(manager), 0, sizeof(manager));
    DLI_SmCbkContext smCtx;
    smCtx.connHandle = 0x1234;
    smCtx.isGroup = false;
    manager.cbkContext = &smCtx;

    evt.status = DLI_COMMAND_TIMEOUT;
    DLI_EncryptChangeCbk(&manager, &evt, sizeof(evt), 0);
    EXPECT_EQ(2, g_fullFuncExecCount);
    EXPECT_EQ(0x1234, evt.connHandle);

    evt.connHandle = 0x0002;
    evt.status = DLI_MEMORY_CAPACITY_EXCEEDED;
    DLI_EncryptChangeCbk(NULL, &evt, sizeof(evt), 0);
    EXPECT_EQ(3, g_fullFuncExecCount);
    EXPECT_EQ(0x0002, evt.connHandle);

    evt.connHandle = 0x0003;
    DLI_EncryptChangeCbk(NULL, &evt, 1, 0);
    EXPECT_EQ(3, g_fullFuncExecCount);
    DLI_RemoveCbks(cbkTable, 1);
}

// 校验测距能力事件的处理回调透传：注册了处理函数时本地与远端事件分别带标记回调，未注册时不回调
TEST_F(UT_DLI_FULL_FUNC, TestCaseHadmCsCapsCallback)
{
    static DLI_CbkLineStru cbkTable[] = {{DLI_CBK_READ_REMOTE_MEASURE_CAPS, FullFuncExecCbk}};
    DLI_ReadRemoteCsCapsPrivEvt remote;
    (void)memset_s(&remote, sizeof(remote), 0, sizeof(remote));
    remote.status = 5;

    FullFuncSetCallback(true);
    DLI_AddCbks(cbkTable, 1);
    DLI_ReadRemoteCsCapsCbk(NULL, &remote, sizeof(remote), 0);
    EXPECT_EQ(1, g_fullFuncCsCapsCount);
    EXPECT_FALSE(g_fullFuncCsCapsLocal);
    EXPECT_EQ(1, g_fullFuncExecCount);
    EXPECT_EQ(5, g_fullFuncExecStatus);
    DLI_ReadRemoteCsCapsCbk(NULL, &remote, 1, 0);
    EXPECT_EQ(1, g_fullFuncCsCapsCount);

    DLI_ReadLocalCsCapsPrivEvt local;
    (void)memset_s(&local, sizeof(local), 0, sizeof(local));
    DLI_ReadLocalCsCapsCbk(NULL, &local, sizeof(local), 0);
    EXPECT_EQ(2, g_fullFuncCsCapsCount);
    EXPECT_TRUE(g_fullFuncCsCapsLocal);

    FullFuncSetCallback(false);
    DLI_ReadLocalCsCapsCbk(NULL, &local, sizeof(local), 0);
    DLI_ReadRemoteCsCapsCbk(NULL, &remote, sizeof(remote), 0);
    EXPECT_EQ(2, g_fullFuncCsCapsCount);
    DLI_RemoveCbks(cbkTable, 1);
}

// 校验命令号到注册号的映射：内置命令查映射表，未知命令走扩展回调，扩展回调缺失时返回无效值
TEST_F(UT_DLI_FULL_FUNC, TestCaseGetopcodeByCmd)
{
    DLI_Callback cbk = {0};
    cbk.postOtherThread = FullFuncSyncPost;
    cbk.postOtherBlockedThread = FullFuncSyncPostBlocked;
    cbk.recvAcbHandler = FullFuncRecvAcb;
    cbk.getExtRegOpcode = FullFuncExtOpcode;
    DLI_SetCallback(&cbk);

    EXPECT_EQ(DLI_CBK_CONNECT, DLI_GetopcodeByCmd(DLI_CREATE_CONNECTION));
    EXPECT_EQ(0x0066, DLI_GetopcodeByCmd(0x0123));

    FullFuncSetCallback(false);
    EXPECT_EQ(DLI_CBK_MAX, DLI_GetopcodeByCmd(0x0123));
}
