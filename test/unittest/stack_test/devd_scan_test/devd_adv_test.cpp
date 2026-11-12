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

#include "devd_local.h"
#include "devd_tbl.h"
#include "devd_adv.h"
#include "devd_cbk.h"
#include "devd_scan.h"
#include "devd_scan_type.h"
#include "devd_scan_api.h"
#include "devd_scan_stm.h"
#include "devd_scan_manager.h"
#include "nlstk_scan_api.h"
#include "nlstk_devd_def.h"
#include "dli_def.h"
#include "dli_errno.h"

using namespace testing;
using namespace testing::ext;

static int g_advEvtCnt = 0;
static uint8_t g_advEvtEvent = 0;
static uint8_t g_advEvtResult = 0;
static uint8_t g_advEvtHandle = 0;

static void CountAdvCbk(NLSTK_DevdAdvCbkParam_S *param)
{
    g_advEvtCnt++;
    g_advEvtEvent = param->event;
    g_advEvtResult = param->result;
    g_advEvtHandle = param->advHandle;
}

static uint8_t CreateHandleOk(void)
{
    return DevdCreateAdvHandle(CountAdvCbk);
}

static DevdAdvNode_S *NodeOf(uint8_t handle)
{
    return DevdGetAdvNode(handle, DEVD_ADV_LIST);
}

// 直接构造临时广播数据挂在节点上（绕过保存入口的长度上限，用于分片矩阵布景）
static NLSTK_DevdAdvData_S *CraftAdvData(uint16_t advLen, uint16_t rspLen)
{
    NLSTK_DevdAdvData_S *data = (NLSTK_DevdAdvData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvData_S));
    if (data == NULL) {
        return NULL;
    }
    data->advDataLen = advLen;
    data->scanRspDataLen = rspLen;
    data->advData = (uint8_t *)SDF_MemZalloc(advLen);
    data->scanRspData = (uint8_t *)SDF_MemZalloc(rspLen);
    if (data->advData != NULL) {
        for (uint16_t i = 0; i < advLen; i++) {
            data->advData[i] = (uint8_t)(i & 0xFF);
        }
    }
    if (data->scanRspData != NULL && rspLen != 0) {
        (void)memset_s(data->scanRspData, rspLen, 0x5A, rspLen);
    }
    return data;
}

static uint8_t g_scanEvtCaptureCnt = 0;
static uint8_t g_scanEvtCaptureEvent = 0;
static uint8_t g_scanEvtCaptureResult = 0;
static void CaptureScanEventCbk(uint8_t event, uint8_t result)
{
    g_scanEvtCaptureCnt++;
    g_scanEvtCaptureEvent = event;
    g_scanEvtCaptureResult = result;
}

class DEVD_ADV_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<SdfMemMock> sdfMemMock;
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
        g_advEvtCnt = 0;
        g_advEvtEvent = 0;
        g_advEvtResult = 0;
        g_advEvtHandle = 0;
        g_scanEvtCaptureCnt = 0;
    }

    void TearDown() override
    {
        SCAN_EVENT_CBK = NULL;
        DevdScanDeInit();
        DevdLocalDeviceDeInit();
        TEST_StackScheduleDeInit();
        g_testDevdStartScanFail = 0;
        g_testDevdStopScanFail = 0;
        g_testDevdStartScanSilent = 0;
        g_testDevdStopScanSilent = 0;
        TEST_DevdDliStubReset();
    }
};

// 广播参数下发按帧型推导次级帧格式与监督超时；帧四可协商时连接间隔取默认值
TEST_F(DEVD_ADV_TEST, AdvParamBuildByFrameType)
{
    uint8_t handle = CreateHandleOk();
    ASSERT_NE(handle, DEVD_INVALID_ADV_HANDLE);
    DevdAdvNode_S *node = NodeOf(handle);
    ASSERT_NE(node, nullptr);

    static uint8_t advOne[1] = {0x01};
    DevdSetAdvParams_S params = {0};
    params.param.handle = handle;
    params.param.basic.primaryFrameType = PRIM_ADV_FRAME_TYPE_1;
    params.param.basic.advMode = ADV_MODE_NONCONN_SCANABLE;
    params.data.advDataLen = 1;
    params.data.advData = advOne;
    DevdSetAdvParam(&params);
    EXPECT_EQ(g_dliStubAdvParamSecondFormat, SECOND_ADV_FRAME_TYPE_1); // 帧一推导次级帧一
    EXPECT_EQ(g_dliStubAdvParamTimeout, DEFAULT_FRAME_TYPE_1_ADV_TIMEOUT);

    // 帧四且角色可协商：连接间隔覆写为帧四默认
    node->status = DEVD_SLE_STATUS_IDLE;
    params.param.basic.primaryFrameType = PRIM_ADV_FRAME_TYPE_4_M_0;
    params.param.basic.gtRole = ADV_GT_ROLE_T_NO_NEGO + 1;
    params.param.connect.connMinInterval = 100;
    params.param.connect.connMaxInterval = 200;
    DevdSetAdvParam(&params);
    EXPECT_EQ(g_dliStubAdvParamSecondFormat, SECOND_ADV_FRAME_TYPE_4_M_0);
    EXPECT_EQ(g_dliStubAdvParamTimeout, DEFAULT_FRAME_TYPE_4_ADV_TIMEOUT);
    EXPECT_EQ(g_dliStubAdvParamConnMin, DEFAULT_FRAME_4_CONN_INTERVAL);
    EXPECT_EQ(g_dliStubAdvParamConnMax, DEFAULT_FRAME_4_CONN_INTERVAL);
}

// 设置广播参数失败回报：临时参数释放，无正式参数时节点删除、有正式参数时回空闲
TEST_F(DEVD_ADV_TEST, SetAdvParamCbFailCleanup)
{
    uint8_t handle = CreateHandleOk();
    ASSERT_NE(handle, DEVD_INVALID_ADV_HANDLE);
    DLI_AdvCbkContext ctx = {.advHandle = handle};

    DevdSetAdvParamCbk(&ctx, 0x11, NULL);
    EXPECT_EQ(g_advEvtCnt, 1);
    EXPECT_EQ(g_advEvtEvent, DEVD_CBK_EVENT_ENABLE_ADV);
    EXPECT_EQ(g_advEvtResult, 0x11);
    EXPECT_EQ(NodeOf(handle), nullptr); // 无正式参数：节点删除

    // 有正式参数：保留节点回到空闲
    handle = CreateHandleOk();
    DevdAdvNode_S *node = NodeOf(handle);
    ASSERT_NE(node, nullptr);
    node->param = (DevdAdvParam_S *)SDF_MemZalloc(sizeof(DevdAdvParam_S));
    DevdSetAdvParamCbk(&ctx, 0x11, NULL);
    EXPECT_NE(NodeOf(handle), nullptr);
    EXPECT_EQ(node->status, DEVD_SLE_STATUS_IDLE);
}

// 设置广播参数成功但无待发数据：节点回到空闲（数据入口的空数据防御）
TEST_F(DEVD_ADV_TEST, SetAdvParamCbWithoutDataGoesIdle)
{
    uint8_t handle = CreateHandleOk();
    DLI_AdvCbkContext ctx = {.advHandle = handle};
    DevdAdvNode_S *node = NodeOf(handle);
    ASSERT_NE(node, nullptr);
    node->tempParam = (DevdAdvParam_S *)SDF_MemZalloc(sizeof(DevdAdvParam_S));

    DevdSetAdvParamCbk(&ctx, DLI_SUCCESS, NULL);
    EXPECT_EQ(g_advEvtCnt, 0); // 数据阶段不向服务层回报
    EXPECT_EQ(node->status, DEVD_SLE_STATUS_IDLE); // 无临时数据直接回空闲
    EXPECT_EQ(g_dliStubSetAdvDataCnt, 0);
}

// 更新广播数据分片：超长数据依次按首片/中间片/尾片下发，发完回空闲并接管数据
TEST_F(DEVD_ADV_TEST, UpdateAdvDataFragmentSequence)
{
    uint8_t handle = CreateHandleOk();
    DLI_AdvCbkContext ctx = {.advHandle = handle};

    // 首片：单包上限内取满
    DevdAdvNode_S *node = NodeOf(handle);
    ASSERT_NE(node, nullptr);
    node->isUpdateData = true;
    node->status = DEVD_SLE_STATUS_SET_ADV_DATA;
    node->tempData = CraftAdvData(3000, 0);
    ASSERT_NE(node->tempData, nullptr);
    DevdSetAdvDataCbk(&ctx, DLI_SUCCESS, NULL);
    EXPECT_EQ(g_advEvtCnt, 0); // 未发完不通知
    EXPECT_EQ(g_dliStubAdvDataOp, ADV_OPERATION_FRAGMENT_FIRST);
    EXPECT_EQ(g_dliStubAdvDataLen, 2047);
    EXPECT_EQ(node->dataOp.dataOffset, 2047);

    // 中间片：剩余仍超单包上限
    node->tempData = CraftAdvData(5000, 0);
    node->dataOp.dataOffset = 2047;
    DevdSetAdvDataCbk(&ctx, DLI_SUCCESS, NULL);
    EXPECT_EQ(g_dliStubAdvDataOp, ADV_OPERATION_FRAGMENT_INTERMEDIATE);
    EXPECT_EQ(g_dliStubAdvDataLen, 2047);

    // 尾片：恰好补齐
    node->tempData = CraftAdvData(3000, 0);
    node->dataOp.dataOffset = 2047;
    DevdSetAdvDataCbk(&ctx, DLI_SUCCESS, NULL);
    EXPECT_EQ(g_dliStubAdvDataOp, ADV_OPERATION_FRAGMENT_LAST);
    EXPECT_EQ(g_dliStubAdvDataLen, 953);

    // 短数据一发即完整：回空闲、通知服务层、正式数据接管
    node->tempData = CraftAdvData(5, 0);
    node->dataOp.dataOffset = 0;
    DevdSetAdvDataCbk(&ctx, DLI_SUCCESS, NULL);
    EXPECT_EQ(g_dliStubAdvDataOp, ADV_OPERATION_FRAGMENT_COMPLETE);
    EXPECT_EQ(g_advEvtCnt, 1);
    EXPECT_EQ(g_advEvtEvent, DEVD_CBK_EVENT_SET_ADV_DATA);
    EXPECT_EQ(node->status, DEVD_SLE_STATUS_IDLE);
    ASSERT_NE(node->data, nullptr);
    EXPECT_EQ(node->data->advDataLen, 5);
    EXPECT_EQ(node->tempData, nullptr);
}

// 更新广播数据失败：临时数据释放；已有正式数据保留节点回空闲，无正式数据删除节点
TEST_F(DEVD_ADV_TEST, UpdateAdvDataFailCleanup)
{
    uint8_t handle = CreateHandleOk();
    DLI_AdvCbkContext ctx = {.advHandle = handle};

    // 未发完即失败：不通知、不下发、删除节点
    DevdAdvNode_S *node = NodeOf(handle);
    ASSERT_NE(node, nullptr);
    node->isUpdateData = true;
    node->status = DEVD_SLE_STATUS_SET_ADV_DATA;
    node->tempData = CraftAdvData(3000, 0);
    DevdSetAdvDataCbk(&ctx, 0x22, NULL);
    EXPECT_EQ(g_advEvtCnt, 0);
    EXPECT_EQ(g_dliStubSetAdvDataCnt, 0);
    EXPECT_EQ(NodeOf(handle), nullptr);

    // 已发完再失败：通知失败结果；有正式数据保留节点
    handle = CreateHandleOk();
    node = NodeOf(handle);
    ASSERT_NE(node, nullptr);
    node->isUpdateData = true;
    node->status = DEVD_SLE_STATUS_SET_ADV_DATA;
    node->tempData = CraftAdvData(10, 0);
    node->dataOp.dataOffset = 10;
    node->data = CraftAdvData(8, 0);
    DevdSetAdvDataCbk(&ctx, 0x22, NULL);
    EXPECT_EQ(g_advEvtCnt, 1);
    EXPECT_EQ(g_advEvtEvent, DEVD_CBK_EVENT_SET_ADV_DATA);
    EXPECT_EQ(g_advEvtResult, 0x22);
    EXPECT_NE(NodeOf(handle), nullptr);
    EXPECT_EQ(node->status, DEVD_SLE_STATUS_IDLE);
}

// 设置广播参数后带扫描响应数据：先发广播数据再发扫描响应，全部完成后自动使能广播
TEST_F(DEVD_ADV_TEST, SetAdvChainWithScanRspAutoEnable)
{
    static uint8_t advBody[3] = {0x01, 0x02, 0x03};
    static uint8_t rspBody[4] = {0x0A, 0x0B, 0x0C, 0x0D};
    uint8_t handle = CreateHandleOk();
    ASSERT_NE(handle, DEVD_INVALID_ADV_HANDLE);

    DevdSetAdvParams_S params = {0};
    params.param.handle = handle;
    params.param.basic.primaryFrameType = PRIM_ADV_FRAME_TYPE_1;
    params.param.basic.advMode = ADV_MODE_NONCONN_SCANABLE;
    params.data.advDataLen = sizeof(advBody);
    params.data.advData = advBody;
    params.data.scanRspDataLen = sizeof(rspBody);
    params.data.scanRspData = rspBody;

    g_dliStubSetAdvParamReply = 1;
    g_dliStubSetAdvParamStatus = DLI_SUCCESS;
    g_dliStubSetAdvDataReply = 1;
    g_dliStubSetAdvDataStatus = DLI_SUCCESS;
    g_dliStubSetScanRspReply = 1;
    g_dliStubSetScanRspStatus = DLI_SUCCESS;
    g_dliStubEnableAdvReply = 1;
    g_dliStubEnableAdvStatus = DLI_SUCCESS;
    DevdSetAdvParam(&params);
    TEST_RunQueueStubSchedule(); // 参数→数据→响应→使能 全链回放

    EXPECT_EQ(g_dliStubAdvDataOp, ADV_OPERATION_FRAGMENT_COMPLETE);
    EXPECT_EQ(g_dliStubAdvDataLen, sizeof(advBody));
    EXPECT_EQ(g_dliStubScanRspOp, ADV_OPERATION_FRAGMENT_COMPLETE); // 响应数据独立下发
    EXPECT_EQ(g_dliStubEnableAdvEnable, 1); // 数据齐后自动使能
    EXPECT_EQ(g_advEvtCnt, 1);
    EXPECT_EQ(g_advEvtEvent, DEVD_CBK_EVENT_ENABLE_ADV);
    DevdAdvNode_S *node = NodeOf(handle);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->advStatus, DEVD_CBK_EVENT_ENABLE_ADV); // 成功回报写入使能事件标记
    EXPECT_EQ(node->data->scanRspDataLen, sizeof(rspBody));
}

// 使能/禁用广播链：下发参数被捕获，成功回报更新广播状态；下发失败不残留中间状态
TEST_F(DEVD_ADV_TEST, EnableDisableAdvChain)
{
    uint8_t handle = CreateHandleOk();
    DevdAdvNode_S *node = NodeOf(handle);
    ASSERT_NE(node, nullptr);
    NLSTK_DevdSetAdvEnable_S enable = {0};
    enable.advHandle = handle;

    // 下发入口失败：节点状态回空闲且不回报
    g_dliStubEnableAdvFail = 1;
    enable.enable = ADV_ENABLED;
    DevdEnableAdv(&enable);
    EXPECT_EQ(g_advEvtCnt, 0);
    EXPECT_EQ(node->status, DEVD_SLE_STATUS_IDLE);

    // 使能成功
    g_dliStubEnableAdvFail = 0;
    g_dliStubEnableAdvReply = 1;
    g_dliStubEnableAdvStatus = DLI_SUCCESS;
    DevdEnableAdv(&enable);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_advEvtCnt, 1);
    EXPECT_EQ(g_advEvtEvent, DEVD_CBK_EVENT_ENABLE_ADV);
    EXPECT_EQ(node->advStatus, DEVD_CBK_EVENT_ENABLE_ADV); // 回报写入使能事件标记
    EXPECT_EQ(g_dliStubEnableAdvHandle, handle);

    // 禁用成功
    enable.enable = ADV_DISABLED;
    DevdEnableAdv(&enable);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_advEvtCnt, 2);
    EXPECT_EQ(g_advEvtEvent, DEVD_CBK_EVENT_DISABLE_ADV);
    EXPECT_EQ(node->advStatus, DEVD_CBK_EVENT_DISABLE_ADV);
}

// 移除广播链：成功回报后节点删除，失败回报节点保留
TEST_F(DEVD_ADV_TEST, RemoveAdvChain)
{
    uint8_t handle = CreateHandleOk();
    uint8_t handleArg = handle;

    g_dliStubRemoveAdvReply = 1;
    g_dliStubRemoveAdvStatus = 0x33;
    DevdRemoveAdv(&handleArg);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_advEvtCnt, 1);
    EXPECT_EQ(g_advEvtEvent, DEVD_CBK_EVENT_REMOVE_ADV);
    EXPECT_NE(NodeOf(handle), nullptr); // 失败保留

    g_dliStubRemoveAdvStatus = DLI_SUCCESS;
    DevdRemoveAdv(&handleArg);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(NodeOf(handle), nullptr); // 成功删除
}

// 底层终止广播事件：非法载荷拒绝；有效载荷回报事件，成功时复位广播状态
TEST_F(DEVD_ADV_TEST, AdvTerminatedCbkMatrix)
{
    uint8_t handle = CreateHandleOk();
    DevdAdvNode_S *node = NodeOf(handle);
    ASSERT_NE(node, nullptr);

    DevdAdvTerminatedCbk(NULL, DLI_SUCCESS, NULL); // 空结果
    struct DLI_ExecuteCmdRetParam res = {0};
    DevdAdvTerminatedCbk(NULL, DLI_SUCCESS, &res); // 空事件参数
    EXPECT_EQ(g_advEvtCnt, 0);

    DevdAdvTerminatedEvent_S evt = {0};
    evt.advHandle = handle;
    evt.connHandle = 0x0B;
    res.eventParameter = &evt;
    res.size = sizeof(evt) - 1; // 尺寸不足
    DevdAdvTerminatedCbk(NULL, DLI_SUCCESS, &res);
    EXPECT_EQ(g_advEvtCnt, 0);

    res.size = sizeof(evt);
    evt.status = 0x44;
    node->advStatus = ADV_ENABLED;
    DevdAdvTerminatedCbk(NULL, DLI_SUCCESS, &res); // 事件内状态失败：只回报不复位
    EXPECT_EQ(g_advEvtCnt, 1);
    EXPECT_EQ(g_advEvtEvent, DEVD_CBK_EVENT_TERMINATED_ADV);
    EXPECT_EQ(node->advStatus, ADV_ENABLED);

    evt.status = DLI_SUCCESS;
    DevdAdvTerminatedCbk(NULL, DLI_SUCCESS, &res);
    EXPECT_EQ(g_advEvtCnt, 2);
    EXPECT_EQ(node->advStatus, ADV_DISABLED); // 成功：广播复位
    EXPECT_EQ(node->status, DEVD_SLE_STATUS_IDLE);
}

// 广播句柄分配：默认单节点上限，占用后再建失败，移除后可重建
TEST_F(DEVD_ADV_TEST, CreateAdvHandleSlot)
{
    uint8_t handle1 = CreateHandleOk();
    EXPECT_NE(handle1, DEVD_INVALID_ADV_HANDLE);
    EXPECT_EQ(CreateHandleOk(), DEVD_INVALID_ADV_HANDLE); // 上限已满

    DevdRemoveAdvNode(handle1, DEVD_ADV_LIST);
    uint8_t handle2 = CreateHandleOk();
    EXPECT_NE(handle2, DEVD_INVALID_ADV_HANDLE); // 释放后可重建
}

// 广播表释放与查询入口的组合行为
TEST_F(DEVD_ADV_TEST, TblFreeAndQueryCombo)
{
    DevdFreeSetAdvData(NULL); // 空指针防御
    DevdFreeSetAdvParams(NULL);

    uint8_t handle = CreateHandleOk();
    EXPECT_NE(NodeOf(handle), nullptr); // 命中
    EXPECT_EQ(NodeOf(handle + 1), nullptr); // 未命中

    DevdAdvNode_S *node = NodeOf(handle);
    DevdAdvParam_S param = {0};
    EXPECT_EQ(DevdSaveAdvParamToTbl(node, &param), NLSTK_OK);
    DevdAdvParam_S *first = node->tempParam;
    EXPECT_EQ(DevdSaveAdvParamToTbl(node, &param), NLSTK_OK); // 重复保存不泄漏
    EXPECT_NE(node->tempParam, nullptr);
    EXPECT_NE(node->tempParam, first);

    NLSTK_DevdSetAdvData_S setData = {0};
    static uint8_t body[2] = {0x01, 0x02};
    setData.advHandle = handle;
    setData.data.advDataLen = sizeof(body);
    setData.data.advData = body;
    DevdFreeSetAdvData(&setData); // 带数据释放
}

// 扫描参数下发失败回报：临时参数释放、模块收到使能扫描失败
TEST_F(DEVD_ADV_TEST, ScanParamFailCbkFreesTmp)
{
    SCAN_EVENT_CBK = CaptureScanEventCbk;
    NLSTK_DevdSleScanParams_S *scanParams = (NLSTK_DevdSleScanParams_S *)SDF_MemZalloc(
        sizeof(NLSTK_DevdSleScanParams_S) + sizeof(NLSTK_DevdSleScanParamsNoPhy_S));
    ASSERT_NE(scanParams, nullptr);
    scanParams->frameType = DEVD_SCAN_FRAME_TYPE_1;
    scanParams->params[0].scanWindow = 1024;
    scanParams->params[0].scanInterval = 4096;

    g_dliStubSetScanParamReply = 1;
    g_dliStubSetScanParamStatus = 0x31;
    DevdSleSetScanParam(scanParams);
    EXPECT_NE((void *)DEVD_SCAN_MGR.tmpScanParams, nullptr); // 已暂存待确认
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(DEVD_SCAN_MGR.tmpScanParams, nullptr); // 失败后释放
    EXPECT_EQ(DEVD_SCAN_MGR.status, DEVD_SLE_STATUS_IDLE);
    EXPECT_EQ(g_scanEvtCaptureCnt, 1);
    EXPECT_EQ(g_scanEvtCaptureEvent, DEVD_CBK_EVENT_ENABLE_SCAN);
    EXPECT_EQ(g_scanEvtCaptureResult, 0x31);
    SDF_MemFree(scanParams);
}

// 扫描使能回报按暂存方向给出事件：使能成功/禁用成功/失败路径
TEST_F(DEVD_ADV_TEST, ScanEnableCbkDirectionMatrix)
{
    SCAN_EVENT_CBK = CaptureScanEventCbk;

    // 由关到开且成功：使能事件
    DEVD_SCAN_MGR.status = DEVD_SLE_STATUS_SET_SCAN_DISABLE2ENABLE;
    DevdSleEnableScanCbk(NULL, DLI_SUCCESS, NULL);
    EXPECT_EQ(g_scanEvtCaptureEvent, DEVD_CBK_EVENT_ENABLE_SCAN);
    EXPECT_EQ(DEVD_SCAN_MGR.status, DEVD_SLE_STATUS_IDLE);

    // 由开到关且成功：禁用事件
    g_scanEvtCaptureCnt = 0;
    DEVD_SCAN_MGR.status = DEVD_SLE_STATUS_SET_SCAN_ENABLE2DISABLE;
    DevdSleEnableScanCbk(NULL, DLI_SUCCESS, NULL);
    EXPECT_EQ(g_scanEvtCaptureEvent, DEVD_CBK_EVENT_DISABLE_SCAN);

    // 方向未知且成功：按禁用回报
    g_scanEvtCaptureCnt = 0;
    DEVD_SCAN_MGR.status = DEVD_SLE_STATUS_IDLE;
    DevdSleEnableScanCbk(NULL, DLI_SUCCESS, NULL);
    EXPECT_EQ(g_scanEvtCaptureEvent, DEVD_CBK_EVENT_DISABLE_SCAN);

    // 失败：仍回报禁用事件携带失败状态
    g_scanEvtCaptureCnt = 0;
    DEVD_SCAN_MGR.status = DEVD_SLE_STATUS_SET_SCAN_DISABLE2ENABLE;
    DevdSleEnableScanCbk(NULL, 0x55, NULL);
    EXPECT_EQ(g_scanEvtCaptureEvent, DEVD_CBK_EVENT_DISABLE_SCAN);
    EXPECT_EQ(g_scanEvtCaptureResult, 0x55);
    EXPECT_EQ(DEVD_SCAN_MGR.status, DEVD_SLE_STATUS_IDLE);
}
