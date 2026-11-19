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

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_sdf_mem_mock.h"
#include "stack_sdf_mem_stub.h"
#include "devd_scan_test_ctrl.h"

#include "sdf_stm.h"
#include "devd_local.h"
#include "devd_scan_type.h"
#include "devd_scan_api.h"
#include "devd_scan_stm.h"
#include "devd_scan_manager.h"
#include "nlstk_scan_api.h"
#include "nlstk_devd_def.h"

using namespace testing;
using namespace testing::ext;

static int g_ssEvtCnt = 0;
static int g_ssEvtCode = 0;
static bool g_ssEvtIsStart = false;

static void StartStopEventCbk(NLSTK_Errcode_E code, bool isStart)
{
    g_ssEvtCnt++;
    g_ssEvtCode = code;
    g_ssEvtIsStart = isStart;
}

class DEVD_STM_TEST : public testing::Test {
public:
    NiceMock<OHOS::ScheduleMock> scheduleMock;
    NiceMock<OHOS::SdfMemMock> sdfMemMock;
protected:
    void SetUp() override
    {
        ON_CALL(sdfMemMock, SDF_MemZalloc).WillByDefault(TEST_SDF_MemZalloc);
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskQueueStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        TEST_DevdDliStubReset();
        g_testDevdStartScanFail = 0;
        g_testDevdStopScanFail = 0;
        g_testDevdStartScanSilent = 0;
        g_testDevdStopScanSilent = 0;
        DevdLocalDeviceInit();
        (void)DevdScanInit();
        NLSTK_DevdScanCbk_S scanCbk = {0};
        scanCbk.onStartOrStopEvent = StartStopEventCbk;
        (void)NLSTK_DevdRegScanModule(&g_stmModuleId, &scanCbk);
        g_ssEvtCnt = 0;
        g_ssEvtCode = 0;
        g_ssEvtIsStart = false;
    }

    void TearDown() override
    {
        DevdCollabUnRegFunc(); // 用例体注销可能被断言中断，兜底防协同钩子跨用例残留
        NLSTK_DevdDeregScanModule(g_stmModuleId);
        g_stmModuleId = DEVD_MAX_MODULE_ID;
        DevdScanDeInit();
        DevdLocalDeviceDeInit();
        TEST_StackScheduleDeInit();
        g_testDevdStartScanFail = 0;
        g_testDevdStopScanFail = 0;
        g_testDevdStartScanSilent = 0;
        g_testDevdStopScanSilent = 0;
        TEST_DevdDliStubReset();
    }

    static uint8_t g_stmModuleId;
public:
    static uint8_t GetStmModuleId() { return g_stmModuleId; }  // verify-fix: allow outside helper access
protected:
};
uint8_t DEVD_STM_TEST::g_stmModuleId = DEVD_MAX_MODULE_ID;  // verify-fix: restore out-of-line def


static uint32_t AllocScannerOk(void)
{
    uint32_t scannerId = DEVD_INVALID_SCANNER_ID;
    EXPECT_EQ(NLSTK_DevdAllocScannerId(DEVD_STM_TEST::GetStmModuleId(), &scannerId), NLSTK_ERRCODE_SUCCESS);
    return scannerId;
}

static NLSTK_Errcode_E StartScanMode(uint32_t scannerId, uint8_t frameType)
{
    NLSTK_DevdScanSetting_S setting = {0};
    setting.scanMode = SCAN_MODE_BALANCED;
    setting.frameType = frameType;
    return NLSTK_DevdStartScan(scannerId, &setting, NULL, 0);
}

static const char *CurState(void)
{
    return ((StateMachine *)DevdGetStateMachine())->GetCurrentStateName((StateMachine *)DevdGetStateMachine());
}

static bool InState(const char *name)
{
    return strcmp(CurState(), name) == 0;
}

static NLSTK_DevdScanManager_S *Mgr(void)
{
    return DEVD_STM_M(DevdGetStateMachine(), scanManager);
}

static void InjectMsg(int what)
{
    STM_MFUNC(DevdGetStateMachine(), ProcessMessage, (::Message) {.what = what, .extData = NULL});
}

// 芯片启动扫描失败：所有已注册模块收到启动失败事件，状态机回到停止态
TEST_F(DEVD_STM_TEST, StartScanFailNotifiesAllModules)
{
    // 第二个模块不注册启停回调，验证通知循环按注册与回调两项过滤
    uint8_t plainId = DEVD_MAX_MODULE_ID;
    NLSTK_DevdScanCbk_S plainCbk = {0};
    ASSERT_EQ(NLSTK_DevdRegScanModule(&plainId, &plainCbk), NLSTK_ERRCODE_SUCCESS);

    uint32_t scannerId = AllocScannerOk();
    g_testDevdStartScanFail = 1;
    ASSERT_EQ(StartScanMode(scannerId, DEVD_SCAN_FRAME_TYPE_1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    EXPECT_EQ(g_ssEvtCnt, 1);
    EXPECT_EQ(g_ssEvtCode, NLSTK_ERRCODE_FAIL);
    EXPECT_TRUE(g_ssEvtIsStart);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPED)));
    NLSTK_DevdDeregScanModule(plainId);
}

// 启动扫描组包内存申请失败：与芯片失败同路，模块收到失败事件并回到停止态
TEST_F(DEVD_STM_TEST, StartScanAllocFailNotifies)
{
    // 先完成扫描器分配：分配参数与组包缓冲同为 8 字节（结构体按 1 字节对齐），注入须避开前者
    uint32_t scannerId = AllocScannerOk();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc(Eq(sizeof(NLSTK_DevdSleScanParams_S) + sizeof(NLSTK_DevdSleScanParamsNoPhy_S))))
        .WillOnce(TEST_SDF_MemZallocFail)
        .RetiresOnSaturation();

    ASSERT_EQ(StartScanMode(scannerId, DEVD_SCAN_FRAME_TYPE_1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    EXPECT_EQ(g_ssEvtCnt, 1);
    EXPECT_EQ(g_ssEvtCode, NLSTK_ERRCODE_FAIL);
    EXPECT_TRUE(g_ssEvtIsStart);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPED)));
}

// 启动态下收到启动失败/内部超时/未知事件：失败事件回报后回停止态，未知事件不改变状态
TEST_F(DEVD_STM_TEST, StartingDispatchEvents)
{
    uint32_t scannerId = AllocScannerOk();
    g_testDevdStartScanSilent = 1;
    ASSERT_EQ(StartScanMode(scannerId, DEVD_SCAN_FRAME_TYPE_1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTING)));

    // 芯片使能扫描回执携带失败：经事件回调链转为启动失败事件
    TEST_DevdInjectScanEvent(DEVD_CBK_EVENT_ENABLE_SCAN, 1);
    EXPECT_EQ(g_ssEvtCnt, 1);
    EXPECT_EQ(g_ssEvtCode, NLSTK_ERRCODE_FAIL);
    EXPECT_TRUE(g_ssEvtIsStart);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPED)));

    g_ssEvtCnt = 0;
    ASSERT_EQ(StartScanMode(scannerId, DEVD_SCAN_FRAME_TYPE_1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTING)));
    InjectMsg(DEVD_INTERNAL_TIMEOUT);
    EXPECT_EQ(g_ssEvtCnt, 1);
    EXPECT_EQ(g_ssEvtCode, NLSTK_ERRCODE_TIMEOUT);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPED)));

    // 未知事件在启动态被忽略
    g_ssEvtCnt = 0;
    ASSERT_EQ(StartScanMode(scannerId, DEVD_SCAN_FRAME_TYPE_1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTING)));
    InjectMsg(DEVD_POTIENTIAL_UPDATE + 100); // 不存在的事件号
    EXPECT_EQ(g_ssEvtCnt, 0);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTING)));
    g_testDevdStartScanSilent = 0;
}

// 停止态下收到停止失败/内部超时/未知事件：失败回已启动态，超时回停止态并触发重新评估
TEST_F(DEVD_STM_TEST, StoppingDispatchEvents)
{
    uint32_t scannerId = AllocScannerOk();
    ASSERT_EQ(StartScanMode(scannerId, DEVD_SCAN_FRAME_TYPE_1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTED)));
    g_ssEvtCnt = 0; // 清掉启动成功通知，只统计本用例注入的事件

    g_testDevdStopScanSilent = 1;
    ASSERT_EQ(NLSTK_DevdStopScan(scannerId), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPING)));

    // 未知事件在停止过渡态被忽略
    InjectMsg(DEVD_POTIENTIAL_UPDATE + 100); // 不存在的事件号
    EXPECT_EQ(g_ssEvtCnt, 0);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPING)));

    // 芯片禁用扫描回执携带失败：经事件回调链转为停止失败事件，回已启动态
    TEST_DevdInjectScanEvent(DEVD_CBK_EVENT_DISABLE_SCAN, 1);
    EXPECT_EQ(g_ssEvtCnt, 1);
    EXPECT_EQ(g_ssEvtCode, NLSTK_ERRCODE_FAIL);
    EXPECT_FALSE(g_ssEvtIsStart);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTED)));

    g_ssEvtCnt = 0;
    ASSERT_EQ(NLSTK_DevdStopScan(scannerId), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPING)));
    InjectMsg(DEVD_INTERNAL_TIMEOUT);
    EXPECT_EQ(g_ssEvtCnt, 1);
    EXPECT_EQ(g_ssEvtCode, NLSTK_ERRCODE_TIMEOUT);
    EXPECT_FALSE(g_ssEvtIsStart);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPED)));
    g_testDevdStopScanSilent = 0;
}

// 停止态收到参数更新：无扫描器或参数窗口为零不启动；帧一/帧四命中分别进入启动态
TEST_F(DEVD_STM_TEST, StoppedUpdateVariants)
{
    // 无任何扫描器：更新被忽略
    InjectMsg(DEVD_POTIENTIAL_UPDATE);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPED)));
    EXPECT_EQ(g_ssEvtCnt, 0);

    // 未知事件在停止态被忽略
    InjectMsg(DEVD_POTIENTIAL_UPDATE + 100); // 不存在的事件号
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPED)));
    EXPECT_EQ(g_ssEvtCnt, 0);

    // 扫描器存在但从未启动：窗口为零，不迁移
    uint32_t idleScanner = AllocScannerOk();
    InjectMsg(DEVD_POTIENTIAL_UPDATE);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPED)));

    // 帧一目标参数就绪：写入在途参数并迁移启动
    uint32_t frame1Scanner = AllocScannerOk();
    g_testDevdStartScanSilent = 1;
    ASSERT_EQ(StartScanMode(frame1Scanner, DEVD_SCAN_FRAME_TYPE_1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTING)));
    EXPECT_EQ(Mgr()->inflightScanSetting.window, SCAN_MODE_BALANCED_WINDOW);
    EXPECT_EQ(Mgr()->inflightFrame4ScanSetting.window, 0);
    (void)idleScanner;
    g_testDevdStartScanSilent = 0;
}

// 同一扫描器先后配置帧一与帧四：经停止重启后两侧参数同时生效
TEST_F(DEVD_STM_TEST, StoppedUpdateDualFrame)
{
    uint32_t scannerId = AllocScannerOk();
    ASSERT_EQ(StartScanMode(scannerId, DEVD_SCAN_FRAME_TYPE_1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTED)));

    ASSERT_EQ(StartScanMode(scannerId, DEVD_SCAN_FRAME_TYPE_4), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule(); // 停止→重启级联全部排空
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTED)));

    EXPECT_EQ(Mgr()->currentScanSetting.window, SCAN_MODE_BALANCED_WINDOW);
    EXPECT_EQ(Mgr()->currentFrame4ScanSetting.window, SCAN_MODE_BALANCED_WINDOW);
    EXPECT_EQ(Mgr()->inflightScanSetting.window, SCAN_MODE_BALANCED_WINDOW);
    EXPECT_EQ(Mgr()->inflightFrame4ScanSetting.window, SCAN_MODE_BALANCED_WINDOW);
}

// 已启动态参数更新：参数一致且无协同诉求时保持；协同要求续停时迁移停止；参数变化时清在途并迁移
TEST_F(DEVD_STM_TEST, StartedUpdateVariants)
{
    uint32_t scannerId = AllocScannerOk();
    ASSERT_EQ(StartScanMode(scannerId, DEVD_SCAN_FRAME_TYPE_1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTED)));

    // 参数一致：不迁移
    InjectMsg(DEVD_POTIENTIAL_UPDATE);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTED)));

    // 协同判定无需续停（返回假）：与未注册等价，不迁移
    NLSTK_DevdCollabScanFunc_S collab = {0};
    collab.isNeedTryStopScanCollab = [](State *state) { (void)state; return false; };
    ASSERT_EQ(DevdCollabRegFunc(&collab), NLSTK_ERRCODE_SUCCESS);
    InjectMsg(DEVD_POTIENTIAL_UPDATE);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTED)));

    // 协同判定要求续停（返回真）：即使参数一致也迁移停止态（注册为值拷贝，改钩子须重新注册）
    collab.isNeedTryStopScanCollab = [](State *state) { (void)state; return true; };
    ASSERT_EQ(DevdCollabRegFunc(&collab), NLSTK_ERRCODE_SUCCESS);
    g_testDevdStopScanSilent = 1;
    InjectMsg(DEVD_POTIENTIAL_UPDATE);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPING)));
    DevdCollabUnRegFunc();

    // 未知芯片事件在任何态都被扫描事件回调忽略
    TEST_DevdInjectScanEvent(0x77, 0);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPING)));

    // 补齐停止成功事件：回到停止态后既有参数重新驱动到已启动态
    g_testDevdStopScanSilent = 0;
    InjectMsg(DEVD_STOP_OK);
    TEST_RunQueueStubSchedule(); // 排空重启扫描级联任务
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTED)));

    // 参数变化：清空在途参数并迁移停止
    NLSTK_DevdScanSetting_S setting = {0};
    setting.scanMode = SCAN_MODE_FULL_SCAN;
    setting.frameType = DEVD_SCAN_FRAME_TYPE_1;
    g_testDevdStopScanSilent = 1;
    ASSERT_EQ(NLSTK_DevdStartScan(scannerId, &setting, NULL, 0), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPING)));
    EXPECT_EQ(Mgr()->inflightScanSetting.interval, UINT16_MAX); // 在途参数被复位
    g_testDevdStopScanSilent = 0;
}

// 已启动态无扫描器：更新触发停止流程，模块收到停止成功事件
TEST_F(DEVD_STM_TEST, StartedNoScannerStops)
{
    uint32_t scannerId = AllocScannerOk();
    ASSERT_EQ(StartScanMode(scannerId, DEVD_SCAN_FRAME_TYPE_1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTED)));
    g_ssEvtCnt = 0; // 清掉启动成功通知，只统计停止流程的通知

    NLSTK_DevdRemoveScannerId(scannerId);
    TEST_RunQueueStubSchedule();
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPED)));
    EXPECT_EQ(g_ssEvtCnt, 1);
    EXPECT_EQ(g_ssEvtCode, NLSTK_ERRCODE_SUCCESS);
    EXPECT_FALSE(g_ssEvtIsStart);
}

// 状态名接口：自定义区间内设置生效可查，区间外与空指针被拒绝，越界查询返回空串
TEST_F(DEVD_STM_TEST, StateNameApiBoundaries)
{
    EXPECT_EQ(DevdSetStmStateName(NLSTK_DEVD_STATE_USER_DEF_START, "MyUserState"), NLSTK_ERRCODE_SUCCESS);
    EXPECT_STREQ(DevdGetStmStateName(NLSTK_DEVD_STATE_USER_DEF_START), "MyUserState");

    EXPECT_EQ(DevdSetStmStateName(NLSTK_DEVD_STATE_USER_DEF_START - 1, "x"), NLSTK_ERRCODE_PARAM_ERR);
    EXPECT_EQ(DevdSetStmStateName(NLSTK_DEVD_STATE_USER_DEF_END, "x"), NLSTK_ERRCODE_PARAM_ERR);
    EXPECT_EQ(DevdSetStmStateName(NLSTK_DEVD_STATE_USER_DEF_START, NULL), NLSTK_ERRCODE_PARAM_ERR);
    EXPECT_STREQ(DevdGetStmStateName(NLSTK_DEVD_STATE_USER_DEF_END), "");
    EXPECT_STREQ(DevdGetStmStateName(0xFF), "");

    // 用户区初始为空指针且该接口拒空，无复位入口，用哨兵空串写回防止悬垂
    EXPECT_EQ(DevdSetStmStateName(NLSTK_DEVD_STATE_USER_DEF_START, ""), NLSTK_ERRCODE_SUCCESS);
}

// 协同状态只注册一半时构造失败，成对注册后协同状态并入状态机
TEST_F(DEVD_STM_TEST, CollabStatePairing)
{
    NLSTK_DevdCollabScanFunc_S collab = {0};
    collab.createStartCollabState = [](StateMachine *stm) { return StateCtor(stm, "collabStartState"); };
    ASSERT_EQ(DevdCollabRegFunc(&collab), NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(DevdStateMachineCtor(), nullptr); // 缺停止侧协同状态
    DevdCollabUnRegFunc();

    collab = {0};
    collab.createStopCollabState = [](StateMachine *stm) { return StateCtor(stm, "collabStopState"); };
    ASSERT_EQ(DevdCollabRegFunc(&collab), NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(DevdStateMachineCtor(), nullptr); // 缺启动侧协同状态
    DevdCollabUnRegFunc();

    collab = {0};
    collab.createStartCollabState = [](StateMachine *stm) { return StateCtor(stm, "collabStartState"); };
    collab.createStopCollabState = [](StateMachine *stm) { return StateCtor(stm, "collabStopState"); };
    ASSERT_EQ(DevdCollabRegFunc(&collab), NLSTK_ERRCODE_SUCCESS);
    NLSTK_DevdStateMachine_S *stm = DevdStateMachineCtor();
    EXPECT_NE(stm, nullptr);
    DevdStateMachineDtor(stm);
    DevdCollabUnRegFunc();
}

// 状态机构造的内存申请失败：管理器或状态机本体申请失败都返回空
TEST_F(DEVD_STM_TEST, CtorAllocFailPaths)
{
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc(Eq(sizeof(NLSTK_DevdScanManager_S))))
        .WillOnce(TEST_SDF_MemZallocFail)
        .RetiresOnSaturation();
    EXPECT_EQ(DevdStateMachineCtor(), nullptr);

    EXPECT_CALL(sdfMemMock, SDF_MemZalloc(Eq(sizeof(NLSTK_DevdStateMachine_S))))
        .WillOnce(TEST_SDF_MemZallocFail)
        .RetiresOnSaturation();
    EXPECT_EQ(DevdStateMachineCtor(), nullptr);
}

// 协同反初始化钩子在状态机析构时被调用
TEST_F(DEVD_STM_TEST, DtorInvokesCollabDeinit)
{
    static int deinitCnt = 0;
    deinitCnt = 0;
    NLSTK_DevdCollabScanFunc_S collab = {0};
    collab.collabScanStmDeInit = [](void) { deinitCnt++; };
    ASSERT_EQ(DevdCollabRegFunc(&collab), NLSTK_ERRCODE_SUCCESS);

    NLSTK_DevdStateMachine_S *stm = DevdStateMachineCtor();
    ASSERT_NE(stm, nullptr);
    DevdStateMachineDtor(stm);
    EXPECT_EQ(deinitCnt, 1);
    DevdCollabUnRegFunc();
}

// 进入停止态与已启动态时协同结果通知分别携带停止/未停止标志
TEST_F(DEVD_STM_TEST, CollabNotifyOnStateEntry)
{
    static int notifyCnt = 0;
    static bool lastStopped = false;
    notifyCnt = 0;
    NLSTK_DevdCollabScanFunc_S collab = {0};
    collab.notifyScanCollabResult = [](State *state, bool stopped) { (void)state; notifyCnt++; lastStopped = stopped; };
    ASSERT_EQ(DevdCollabRegFunc(&collab), NLSTK_ERRCODE_SUCCESS);

    uint32_t scannerId = AllocScannerOk();
    ASSERT_EQ(StartScanMode(scannerId, DEVD_SCAN_FRAME_TYPE_1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTED)));
    EXPECT_GT(notifyCnt, 0);
    EXPECT_FALSE(lastStopped);

    ASSERT_EQ(NLSTK_DevdStopScan(scannerId), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPED)));
    EXPECT_TRUE(lastStopped);
    DevdCollabUnRegFunc();
}

// 扫描管理器重复初始化：旧状态机被析构，模块表重建后可正常注册分配
TEST_F(DEVD_STM_TEST, DoubleInitRebuilds)
{
    ASSERT_EQ(DevdScanInit(), NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(DevdGetStateMachine(), nullptr);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPED)));

    // 二次初始化后模块表清零，重新注册方可分配
    NLSTK_DevdScanCbk_S scanCbk = {0};
    scanCbk.onStartOrStopEvent = StartStopEventCbk;
    ASSERT_EQ(NLSTK_DevdRegScanModule(&g_stmModuleId, &scanCbk), NLSTK_ERRCODE_SUCCESS);
    uint32_t scannerId = AllocScannerOk();
    EXPECT_NE(scannerId, DEVD_INVALID_SCANNER_ID);
    NLSTK_DevdRemoveScannerId(scannerId);
}

// 协同侧提供扫描器上限时按协同上限分配，注销后恢复默认上限
TEST_F(DEVD_STM_TEST, CollabScannerMaxNum)
{
    NLSTK_DevdCollabScanFunc_S collab = {0};
    collab.collabGetScannerMaxNum = +[](void) -> uint32_t { return 1; };  // verify-fix: unary + forces fn ptr
    ASSERT_EQ(DevdCollabRegFunc(&collab), NLSTK_ERRCODE_SUCCESS);

    uint32_t scannerId1 = DEVD_INVALID_SCANNER_ID;
    ASSERT_EQ(NLSTK_DevdAllocScannerId(g_stmModuleId, &scannerId1), NLSTK_ERRCODE_SUCCESS);
    uint32_t scannerId2 = DEVD_INVALID_SCANNER_ID;
    EXPECT_EQ(NLSTK_DevdAllocScannerId(g_stmModuleId, &scannerId2), NLSTK_ERRCODE_TASK_FAIL);
    NLSTK_DevdRemoveScannerId(scannerId1);
    DevdCollabUnRegFunc();

    uint32_t scannerId3 = DEVD_INVALID_SCANNER_ID;
    EXPECT_EQ(NLSTK_DevdAllocScannerId(g_stmModuleId, &scannerId3), NLSTK_ERRCODE_SUCCESS);
    NLSTK_DevdRemoveScannerId(scannerId3);
}

// 全停时若已在停止态：直接向所有挂了回调的模块回报停止成功，不再下发停止命令
TEST_F(DEVD_STM_TEST, StopAllInStoppedNotifies)
{
    uint8_t plainId = DEVD_MAX_MODULE_ID;
    NLSTK_DevdScanCbk_S plainCbk = {0};
    ASSERT_EQ(NLSTK_DevdRegScanModule(&plainId, &plainCbk), NLSTK_ERRCODE_SUCCESS);

    ASSERT_EQ(NLSTK_DevdStopAllScan(), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_ssEvtCnt, 1); // 只回报挂了回调的模块
    EXPECT_EQ(g_ssEvtCode, NLSTK_ERRCODE_SUCCESS);
    EXPECT_FALSE(g_ssEvtIsStart);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPED)));
    NLSTK_DevdDeregScanModule(plainId);

    // 已启动态下全停不立即回报，走停止流程
    g_ssEvtCnt = 0;
    uint32_t scannerId = AllocScannerOk();
    ASSERT_EQ(StartScanMode(scannerId, DEVD_SCAN_FRAME_TYPE_1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STARTED)));
    g_ssEvtCnt = 0; // 清掉启动成功通知，隔离"停止态立即回报"分支的计数
    g_testDevdStopScanSilent = 1;
    ASSERT_EQ(NLSTK_DevdStopAllScan(), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_ssEvtCnt, 0);
    EXPECT_TRUE(InState(DevdGetStmStateName(DEVD_STATE_STOPPING)));
    g_testDevdStopScanSilent = 0;
}

// 模块表槽位复用：注销中间模块后新注册占用其槽位
TEST_F(DEVD_STM_TEST, ModuleHoleReuse)
{
    uint8_t idA = DEVD_MAX_MODULE_ID;
    uint8_t idB = DEVD_MAX_MODULE_ID;
    NLSTK_DevdScanCbk_S cbk = {0};
    cbk.onStartOrStopEvent = StartStopEventCbk;
    ASSERT_EQ(NLSTK_DevdRegScanModule(&idA, &cbk), NLSTK_ERRCODE_SUCCESS);
    ASSERT_EQ(NLSTK_DevdRegScanModule(&idB, &cbk), NLSTK_ERRCODE_SUCCESS);
    NLSTK_DevdDeregScanModule(idA);

    uint8_t idC = DEVD_MAX_MODULE_ID;
    ASSERT_EQ(NLSTK_DevdRegScanModule(&idC, &cbk), NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(idC, idA); // 空洞被复用
    NLSTK_DevdDeregScanModule(idC);
    NLSTK_DevdDeregScanModule(idB);
}

// 注册与分配接口在任务队列超时时向调用方回传超时错误
TEST_F(DEVD_STM_TEST, ApiTaskTimeoutRollback)
{
    EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked)
        .WillOnce(Return(NLSTK_ERRCODE_TASK_TIMEOUT))
        .RetiresOnSaturation();
    uint8_t moduleId = DEVD_MAX_MODULE_ID;
    NLSTK_DevdScanCbk_S cbk = {0};
    cbk.onStartOrStopEvent = StartStopEventCbk;
    EXPECT_EQ(NLSTK_DevdRegScanModule(&moduleId, &cbk), NLSTK_ERRCODE_TASK_TIMEOUT);

    EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked)
        .WillOnce(Return(NLSTK_ERRCODE_TASK_TIMEOUT))
        .RetiresOnSaturation();
    uint32_t scannerId = DEVD_INVALID_SCANNER_ID;
    EXPECT_EQ(NLSTK_DevdAllocScannerId(g_stmModuleId, &scannerId), NLSTK_ERRCODE_TASK_TIMEOUT);
    TEST_RunQueueStubSchedule(); // 排空超时路径补投的清理任务
}
