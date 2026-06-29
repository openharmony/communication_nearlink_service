/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "stack_dli_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"

#include "nlstk_init_api.h"
#include "nlstk_devd_api.h"
#include "nlstk_devd.h"
#include "nlstk_log.h"
#include "devd_scan.h"
#include "devd_tbl.h"
#include "devd_local.h"
#include "devd_adv.h"
#include "dli.h"
#include "dli_errno.h"
#include "devd_cbk.h"

#define TEST_ADV_DATA_LEN 4
#define TEST_SCAN_RSP_DATA_LEN 4

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static uint16_t g_lcid = 0;

static SLE_Addr_S g_addr1 = {.type = PUBLIC_ADDRESS, .addr = {0x01, 0x01, 0x02, 0x02, 0x03, 0x03}};
static SLE_Addr_S g_addr2 = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};

class DEVD_API_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;
protected:
    void SetUp() override
    {
        // 步骤1：初始化桩函数
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskQueueStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerAdd).WillRepeatedly(TEST_ScheduleTimerAddStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerDel).WillRepeatedly(TEST_ScheduleTimerDelStub);

        TEST_DLI_Init();
        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);

        EXPECT_CALL(dtapMock, DTAP_RegisterDataRecvCb).WillRepeatedly(TEST_DTAP_RegisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_UnregisterDataRecvCb).WillRepeatedly(TEST_DTAP_UnregisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_DataSend).WillRepeatedly(TEST_DTAP_DataSend);

        // 步骤2：初始化DEVD模块
        DevdInit();
        DevdEnable();
    }

    void TearDown() override
    {
        // 步骤3：去使能DEVD模块
        DevdDeInit();

        // 步骤4：清理桩函数
        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
    }
};

static uint8_t testAdvData[TEST_ADV_DATA_LEN] = {0x05, 0x06, 0x07, 0x08};
static uint8_t testScanRspData[TEST_SCAN_RSP_DATA_LEN] = {0x08, 0x06, 0x07, 0x08};
static NLSTK_DevdAdvCbkParam_S g_devdAdvCbkParam = {0};
static uint8_t g_scanCbkMsg = DEVD_CBK_EVENT_MAX;
static uint8_t g_scanCbkStatus = DLI_UNSPECIFIED_ERROR;

// 模拟协议栈广播扫描动作执行结束后回调给上层的接口
static void AdvEventCbk(NLSTK_DevdAdvCbkParam_S *param)
{
    if (param == NULL) {
        NLSTK_LOG_INFO("[DEVD_ADV_Test] AdvEventCbk input param is null");
        return;
    }
    g_devdAdvCbkParam.advHandle = param->advHandle;
    g_devdAdvCbkParam.event = param->event;
    g_devdAdvCbkParam.result = param->result;
    NLSTK_LOG_INFO("adv event cbk: %d, handle: %d, status: %d\n", param->event, param->advHandle, param->result);
}

static void ResetData()
{
    g_devdAdvCbkParam.event = DEVD_CBK_EVENT_MAX;
    g_devdAdvCbkParam.result = DLI_UNSPECIFIED_ERROR;
    g_devdAdvCbkParam.advHandle = 0xFF;
    g_scanCbkMsg = DEVD_CBK_EVENT_MAX;
    g_scanCbkStatus = DLI_UNSPECIFIED_ERROR;
}

static void SetAdvParam(uint8_t handle, NLSTK_DevdSetAdvParams_S *advParams)
{
    advParams->accessMode = ADV_ACCESS_MODE_SLE;
    advParams->discoveryLevel = ADV_DISCOVERY_LEVEL_NORMAL;
    advParams->param.advHandle = handle;
    advParams->param.advMode = ADV_MODE_CONNECTABLE_SCANABLE;
    advParams->param.advGtRole = ADV_GT_ROLE_T_NO_NEGO;
    advParams->param.advIntervalMin = 1024;
    advParams->param.advIntervalMax = 2048;
    advParams->param.advChannelMap = ADV_CHANNEL_MAP_76;
    advParams->param.ownAddr = {.type = PUBLIC_ADDRESS, .addr = {0x01, 0x01, 0x01, 0x02, 0x02, 0x02}};
    advParams->param.peerAddr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x03, 0x04, 0x04, 0x04}};
    advParams->data.advDataLen = sizeof(testAdvData);
    advParams->data.advData = testAdvData;
    advParams->data.scanRspDataLen = sizeof(testScanRspData);
    advParams->data.scanRspData = testScanRspData;
}

static void SetAdvExtParam(NLSTK_DevdAdvExtParam_S *extParam)
{
    extParam->advFilterPolicy = ADV_FLT_ANY_SCAN_ANY_CONNECT;
    extParam->advSid = 0;
    extParam->advTxPower = ADV_TX_POWER_DEFAULT;
    extParam->phyParam.primAdvPhy = ADV_PHY_1M;
    extParam->phyParam.secondAdvFrame = ADV_FRAME_TYPE_GFSK;
    extParam->phyParam.secondAdvPhy = ADV_PHY_1M;
    extParam->scanParam.scanReqNotifEnable = 0;
    extParam->scanParam.scanReqRecvNumberMax = 1;
    extParam->scanParam.scanReqRxDurMax = 0;
}

static void SetAdvConnParam(NLSTK_DevdConnParam_S *connParam)
{
    connParam->intervalMax = CONN_INTV_MIN; // 6
    connParam->intervalMin = CONN_INTV_MIN; // 6
    connParam->maxCeLength = CONN_MAX_LATENCY;
    connParam->maxLatency = CONN_MAX_LATENCY;   // 499
    connParam->minCeLength = 0;
    // supervisionTimeout * 10 > (intervalMax >> 2) * (1 + maxLatency)
    connParam->supervisionTimeout = CONN_SUPERVISION_TIMEOUT_MAX;   // 3200
}

static void SetAdvData(uint8_t handle, NLSTK_DevdSetAdvData_S *advData)
{
    advData->advHandle = handle;
    advData->data.advDataLen = sizeof(testAdvData);
    advData->data.advData = testAdvData;
    advData->data.scanRspDataLen = sizeof(testScanRspData);
    advData->data.scanRspData = testScanRspData;
}

/**
 * @test DEVD_START_ADV_001
 * @brief 此测试用例用于验证DEVD模块开启广播的正确性。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 设置广播参数
 * 2. 设置广播数据
 * 3. 使能（开启）广播
 *
 * @pre
 * - SSAP模块已初始化。
 *
 * @post
 * - 预期结果：广播正常开启。
 */
TEST_F(DEVD_API_TEST, DEVD_START_ADV_001)
{
    uint8_t handle = NLSTK_DevdCreateAdvHandle(AdvEventCbk);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(handle != 0xFF);
    ResetData();

    NLSTK_DevdSetAdvParams_S advParams = {0};
    SetAdvParam(handle, &advParams);
    NLSTK_Errcode_E ret = NLSTK_DevdStartAdv(&advParams);
    TEST_RunQueueStubSchedule();

    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(g_devdAdvCbkParam.event, DEVD_CBK_EVENT_ENABLE_ADV);
    EXPECT_EQ(g_devdAdvCbkParam.result, DLI_SUCCESS);
    EXPECT_EQ(g_devdAdvCbkParam.advHandle, handle);

    ResetData();
    NLSTK_DevdSetAdvEnable_S setEnable = {.advHandle = handle, .enable = 0};
    ret = NLSTK_DevdEnableAdv(&setEnable);
    TEST_RunQueueStubSchedule();

    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(g_devdAdvCbkParam.event, DEVD_CBK_EVENT_DISABLE_ADV);
    EXPECT_EQ(g_devdAdvCbkParam.result, DLI_SUCCESS);
    EXPECT_EQ(g_devdAdvCbkParam.advHandle, handle);

    ret = NLSTK_DevdRemoveAdv(&handle);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
}

static void ScanEventCbk(uint8_t eventMsg, uint8_t result)
{
    g_scanCbkMsg = eventMsg;
    g_scanCbkStatus = result;
    NLSTK_LOG_INFO("[DEVD_API_TEST] scan event cbk msg: %d, result: %d\n", eventMsg, result);
}

static void ScanReportCbk(NLSTK_DevdAdvReportInfo_S *report)
{
    NLSTK_LOG_INFO("[DEVD_API_TEST] scan report cbk addr type: %d, rssi: %d\n", report->addrType, report->rssi);
}

static void SetScanData(NLSTK_DevdSleScanParams_S *scanData)
{
    scanData->localAddrType = PUBLIC_ADDRESS;
    scanData->scanFilterPolicy = SCAN_FLT_BASIC_NONE;
    scanData->frameType = SCAN_FRAME_TYPE_1;
    NLSTK_DevdSleScanParamsNoPhy_S *phy = (NLSTK_DevdSleScanParamsNoPhy_S *)scanData->params;
    phy->scanType = SCAN_TYPE_ACTIVE;
    phy->scanInterval = 100;
    phy->scanWindow = 100;
}

/**
 * @test DEVD_START_SCAN_001
 * @brief 此测试用例用于验证DEVD模块开启关闭扫描的正确性。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 设置扫描参数
 * 2. 使能（开启）扫描
 * 3. 使能（关闭）扫描
 *
 * @pre
 * - SSAP模块已初始化。
 *
 * @post
 * - 预期结果：
 * 1. 扫描正常开启
 * 2. 扫描正常关闭。
 */
TEST_F(DEVD_API_TEST, DEVD_START_SCAN_001)
{
    g_scanCbkMsg = DEVD_CBK_EVENT_MAX;
    g_scanCbkStatus = DLI_UNSPECIFIED_ERROR;
    NLSTK_DevdSleScanExterCbk_S scanCbk = {.scanCbk = ScanEventCbk, .reportCbk = ScanReportCbk};
    NLSTK_Errcode_E ret = NLSTK_DevdRegScanEventCbk(&scanCbk);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    NLSTK_DevdSleScanParams_S *scanData = (NLSTK_DevdSleScanParams_S *)SDF_MemAlloc(
        sizeof(NLSTK_DevdSleScanParams_S) + sizeof(NLSTK_DevdSleScanParamsNoPhy_S));
    SetScanData(scanData);
    ret = NLSTK_DevdSleStartScan(scanData);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(g_scanCbkMsg, DEVD_CBK_EVENT_ENABLE_SCAN);
    EXPECT_EQ(g_scanCbkStatus, DLI_SUCCESS);

    NLSTK_DevdSleScanEnable_S disable = {.scanEnable = 0, .scanFilterDuplicates = 0};
    ret = NLSTK_DevdSleEnableScan(&disable);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(g_scanCbkMsg, DEVD_CBK_EVENT_DISABLE_SCAN);
    EXPECT_EQ(g_scanCbkStatus, DLI_SUCCESS);

    SDF_MemFree(scanData);
}

TEST_F(DEVD_API_TEST, DEVD_START_ADV_FAIL_001)
{
    uint8_t handle = NLSTK_DevdCreateAdvHandle(AdvEventCbk);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(handle != 0xFF);
    g_devdAdvCbkParam.event = DEVD_CBK_EVENT_MAX;
    g_devdAdvCbkParam.result = DLI_UNSPECIFIED_ERROR;
    g_devdAdvCbkParam.advHandle = 0xFF;

    // 下面的NLSTK_DevdStartAdv接口只会走到同步函数，不会走到发布任务那一步，不需要运行异步任务
    NLSTK_DevdSetAdvParams_S advParams = {0};
    SetAdvParam(handle, &advParams);
    advParams.accessMode = (ADV_ACCESS_MODE_DEFAULT + 1);
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvParam(handle, &advParams);
    advParams.discoveryLevel = (ADV_DISCOVERY_LEVEL_SPECIAL + 1);
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvParam(handle, &advParams);
    advParams.param.advMode = (ADV_MODE_CONNECTABLE_SCANABLE + 1);
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvParam(handle, &advParams);
    advParams.param.advMode = (ADV_MODE_CONNECTABLE_DIRECTED);
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_SUCCESS);

    SetAdvParam(handle, &advParams);
    advParams.param.advGtRole = (ADV_GT_ROLE_G_NO_NEGO + 1);
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvParam(handle, &advParams);
    advParams.param.advIntervalMin = (ADV_INTERVAL_MIN - 1);
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvParam(handle, &advParams);
    advParams.param.advIntervalMin = (ADV_INTERVAL_MAX + 1);
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvParam(handle, &advParams);
    advParams.param.advIntervalMax = (ADV_INTERVAL_MIN - 1);
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvParam(handle, &advParams);
    advParams.param.advIntervalMax = (ADV_INTERVAL_MAX + 1);
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvParam(handle, &advParams);
    advParams.param.advChannelMap = (ADV_CHANNEL_MAP_DEFAULT + 1);
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvParam(handle, &advParams);
    advParams.param.ownAddr.type = (ADDR_TYPE_END);
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvParam(handle, &advParams);
    advParams.param.peerAddr.type = (ADDR_TYPE_END);
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    TEST_RunQueueStubSchedule();
}

TEST_F(DEVD_API_TEST, DEVD_START_ADV_FAIL_002)
{
    uint8_t handle = NLSTK_DevdCreateAdvHandle(AdvEventCbk);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(handle != 0xFF);
    g_devdAdvCbkParam.event = DEVD_CBK_EVENT_MAX;
    g_devdAdvCbkParam.result = DLI_UNSPECIFIED_ERROR;
    g_devdAdvCbkParam.advHandle = 0xFF;

    NLSTK_DevdSetAdvParams_S advParams = {0};
    SetAdvParam(handle, &advParams);
    advParams.param.advGtRole  = ADV_GT_ROLE_T_CAN_NEGO;
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvParam(handle, &advParams);
    advParams.data.advDataLen = 0;
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvParam(handle, &advParams);
    advParams.data.scanRspData = NULL;
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    TEST_RunQueueStubSchedule();
}

TEST_F(DEVD_API_TEST, DEVD_START_ADV_CONN_PARAM_SUCC)
{
    uint8_t handle = NLSTK_DevdCreateAdvHandle(AdvEventCbk);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(handle != 0xFF);
    g_devdAdvCbkParam.event = DEVD_CBK_EVENT_MAX;
    g_devdAdvCbkParam.result = DLI_UNSPECIFIED_ERROR;
    g_devdAdvCbkParam.advHandle = 0xFF;

    NLSTK_DevdSetAdvParams_S advParams = {0};
    SetAdvParam(handle, &advParams);
    NLSTK_DevdConnParam_S connParam = {0};
    SetAdvConnParam(&connParam);
    advParams.param.connParam = &connParam;
    advParams.param.advGtRole  = ADV_GT_ROLE_T_CAN_NEGO;
    NLSTK_Errcode_E ret = NLSTK_DevdStartAdv(&advParams);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    TEST_RunQueueStubSchedule();
}

TEST_F(DEVD_API_TEST, DEVD_START_ADV_FAIL_003)
{
    uint8_t handle = NLSTK_DevdCreateAdvHandle(AdvEventCbk);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(handle != 0xFF);
    g_devdAdvCbkParam.event = DEVD_CBK_EVENT_MAX;
    g_devdAdvCbkParam.result = DLI_UNSPECIFIED_ERROR;
    g_devdAdvCbkParam.advHandle = 0xFF;

    // 下面的NLSTK_DevdStartAdv接口只会走到同步函数，不会走到发布任务那一步，不需要运行异步任务
    NLSTK_DevdSetAdvParams_S advParams = {0};
    SetAdvParam(handle, &advParams);
    NLSTK_DevdConnParam_S connParam = {0};
    SetAdvConnParam(&connParam);
    connParam.supervisionTimeout = CONN_SUPERVISION_TIMEOUT_MIN - 1;
    // 9 * 10 <= (0x3E80 >> 2)(1 + 0x1F3)
    advParams.param.connParam = &connParam;
    advParams.param.advGtRole  = ADV_GT_ROLE_T_CAN_NEGO;
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvConnParam(&connParam);
    connParam.supervisionTimeout = CONN_SUPERVISION_TIMEOUT_MAX + 1;
    advParams.param.connParam = &connParam;
    advParams.param.advGtRole  = ADV_GT_ROLE_T_CAN_NEGO;
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvConnParam(&connParam);
    connParam.intervalMin = CONN_INTV_MIN - 1;
    advParams.param.advGtRole  = ADV_GT_ROLE_T_CAN_NEGO;
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvConnParam(&connParam);
    connParam.intervalMin = CONN_INTV_MAX + 1;
    advParams.param.advGtRole  = ADV_GT_ROLE_T_CAN_NEGO;
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvConnParam(&connParam);
    connParam.intervalMax = CONN_INTV_MIN - 1;
    advParams.param.advGtRole  = ADV_GT_ROLE_T_CAN_NEGO;
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvConnParam(&connParam);
    connParam.intervalMax = CONN_INTV_MAX + 1;
    advParams.param.advGtRole  = ADV_GT_ROLE_T_CAN_NEGO;
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    SetAdvConnParam(&connParam);
    connParam.maxLatency = CONN_MAX_LATENCY + 1;
    advParams.param.advGtRole  = ADV_GT_ROLE_T_CAN_NEGO;
    EXPECT_EQ(NLSTK_DevdStartAdv(&advParams), NLSTK_ERRCODE_PARAM_ERR);

    TEST_RunQueueStubSchedule();
}

TEST_F(DEVD_API_TEST, DEVD_START_SCAN_FAIL_001)
{
    g_scanCbkMsg = DEVD_CBK_EVENT_MAX;
    g_scanCbkStatus = DLI_UNSPECIFIED_ERROR;
    NLSTK_DevdSleScanExterCbk_S scanCbk = {.scanCbk = ScanEventCbk, .reportCbk = ScanReportCbk};
    NLSTK_Errcode_E ret = NLSTK_DevdRegScanEventCbk(&scanCbk);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    NLSTK_DevdSleScanParams_S *scanData = (NLSTK_DevdSleScanParams_S *)SDF_MemAlloc(
        sizeof(NLSTK_DevdSleScanParams_S) + sizeof(NLSTK_DevdSleScanParamsNoPhy_S));
    SetScanData(scanData);
    scanData->localAddrType = ADDR_TYPE_END + 1;
    ret = NLSTK_DevdSleStartScan(scanData);
    EXPECT_EQ(ret, NLSTK_ERRCODE_PARAM_ERR);

    SetScanData(scanData);
    scanData->scanFilterPolicy = SCAN_FLT_EXTEND + 1;
    ret = NLSTK_DevdSleStartScan(scanData);
    EXPECT_EQ(ret, NLSTK_ERRCODE_PARAM_ERR);

    SetScanData(scanData);
    scanData->frameType  = ((uint8_t)SCAN_FRAME_TYPE_1 | (uint8_t)SCAN_FRAME_TYPE_4) + 1;
    ret = NLSTK_DevdSleStartScan(scanData);
    EXPECT_EQ(ret, NLSTK_ERRCODE_PARAM_ERR);

    SetScanData(scanData);
    ret = NLSTK_DevdSleStartScan(scanData);
    EXPECT_EQ(ret, NLSTK_ERRCODE_PARAM_ERR);

    TEST_RunQueueStubSchedule();
    SDF_MemFree(scanData);
}

TEST_F(DEVD_API_TEST, DEVD_START_ADV_WITH_EXT_PARAM)
{
    // 设置ext_param成功
    uint8_t handle = NLSTK_DevdCreateAdvHandle(AdvEventCbk);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(handle != 0xFF);
    ResetData();

    NLSTK_DevdSetAdvParams_S advParams = {0};
    NLSTK_DevdAdvExtParam_S extParams = {0};
    advParams.param.extParam = &extParams;
    SetAdvParam(handle, &advParams);
    SetAdvExtParam(&extParams);
    NLSTK_Errcode_E ret = NLSTK_DevdStartAdv(&advParams);
    TEST_RunQueueStubSchedule();

    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(g_devdAdvCbkParam.event, DEVD_CBK_EVENT_ENABLE_ADV);
    EXPECT_EQ(g_devdAdvCbkParam.result, DLI_SUCCESS);
    EXPECT_EQ(g_devdAdvCbkParam.advHandle, handle);
}

TEST_F(DEVD_API_TEST, DEVD_START_ADV_EXT_PARAM_FAIL_001)
{
    // 设置ext_param失败
    uint8_t handle = NLSTK_DevdCreateAdvHandle(AdvEventCbk);
    TEST_RunQueueStubSchedule();
    ASSERT_TRUE(handle != 0xFF);
    ResetData();

    NLSTK_Errcode_E ret = NLSTK_ERRCODE_MAX;
    NLSTK_DevdSetAdvParams_S advParams = {0};
    NLSTK_DevdAdvExtParam_S extParams = {0};
    advParams.param.extParam = &extParams;
    SetAdvParam(handle, &advParams);
    SetAdvExtParam(&extParams);
    extParams.advFilterPolicy = ADV_FLT_ALLOW_SCAN_ALLOW_CONNECT + 1;
    ret = NLSTK_DevdStartAdv(&advParams);
    EXPECT_EQ(ret, NLSTK_ERRCODE_PARAM_ERR);    // 预期走不到SchedulePostTask，所以不需要运行任务

    SetAdvExtParam(&extParams);
    extParams.advTxPower = ADV_TX_POWER_MIN - 1;
    ret = NLSTK_DevdStartAdv(&advParams);
    EXPECT_EQ(ret, NLSTK_ERRCODE_PARAM_ERR);

    SetAdvExtParam(&extParams);
    extParams.advTxPower = ADV_TX_POWER_MAX + 1;
    ret = NLSTK_DevdStartAdv(&advParams);
    EXPECT_EQ(ret, NLSTK_ERRCODE_PARAM_ERR);

    SetAdvExtParam(&extParams);
    extParams.phyParam.primAdvPhy = ADV_PHY_4M + 1;
    ret = NLSTK_DevdStartAdv(&advParams);
    EXPECT_EQ(ret, NLSTK_ERRCODE_PARAM_ERR);

    SetAdvExtParam(&extParams);
    extParams.phyParam.secondAdvFrame = ADV_FRAME_TYPE_SHORT_HEADER + 1;
    ret = NLSTK_DevdStartAdv(&advParams);
    EXPECT_EQ(ret, NLSTK_ERRCODE_PARAM_ERR);

    SetAdvExtParam(&extParams);
    extParams.phyParam.secondAdvPhy = ADV_PHY_4M + 1;
    ret = NLSTK_DevdStartAdv(&advParams);
    EXPECT_EQ(ret, NLSTK_ERRCODE_PARAM_ERR);

    SetAdvExtParam(&extParams);
    extParams.phyParam.secondAdvMcs = ADV_MCS_MAX + 1;
    ret = NLSTK_DevdStartAdv(&advParams);
    EXPECT_EQ(ret, NLSTK_ERRCODE_PARAM_ERR);

    SetAdvExtParam(&extParams);
    extParams.phyParam.secondAdvPilot = ADV_PILOT_RATIO_NO + 1;
    ret = NLSTK_DevdStartAdv(&advParams);
    EXPECT_EQ(ret, NLSTK_ERRCODE_PARAM_ERR);

    SetAdvExtParam(&extParams);
    extParams.scanParam.scanReqNotifEnable = SCAN_REQ_REPORT + 1;
    ret = NLSTK_DevdStartAdv(&advParams);
    EXPECT_EQ(ret, NLSTK_ERRCODE_PARAM_ERR);

    TEST_RunQueueStubSchedule();    // 最后运行一遍防止残留任务导致资源泄漏
}