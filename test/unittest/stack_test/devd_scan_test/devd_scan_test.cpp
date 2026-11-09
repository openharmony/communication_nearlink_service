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

#include "sdf_string.h"
#include "nlstk_log.h"
#include "dli_def.h"
#include "dli_cmd.h"
#include "devd_scan_type.h"
#include "devd_scan_api.h"
#include "nlstk_scan_api.h"
#include "nlstk_devd_def.h"
#include "nlstk_devd_def_ext.h"
#include "nlstk_devd_api.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static uint8_t g_moduleId = DEVD_MAX_MODULE_ID;
static uint8_t g_frameType = 0;
static uint16_t g_window1 = 0;  // 第一组扫描窗口，可能帧一可能帧四
static uint16_t g_interval1 = UINT16_MAX;   // 第一组扫描间隔，可能帧一可能帧四
static uint16_t g_window2 = 0;  // 第二组扫描窗口，帧四
static uint16_t g_interval2 = UINT16_MAX;   // 第二组扫描间隔，帧四
static bool g_enable = false;

static uint8_t g_addr[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
static uint8_t g_mouseData1[] = {
    0x01, 0x01, 0x02, 0x05, 0x02, 0x0B, 0x06, 0x03,
    0x07, 0x09, 0x06, 0x07, 0x03, 0x02, 0x05, 0x00,
    0x03, 0x1B, 0xEE, 0xFD, 0x01, 0x01, 0x01, 0x04,
    0x04, 0x11, 0xF8, 0x12, 0x52, 0x58, 0x31, 0x42,
    0x15, 0xD0, 0xA5, 0x62, 0xF0, 0x0D, 0x01, 0xFF,
    0x00, 0x14, 0x02, 0x31, 0x38};
static uint8_t g_mouseData2[] = {
    0x0B, 0x14, 0x48, 0x55, 0x41, 0x57, 0x45, 0x49,
    0x20, 0x4D, 0x6F, 0x75, 0x73, 0x65, 0x20, 0x43,
    0x44, 0x32, 0x33, 0x2D, 0x52, 0x41};
static uint8_t g_podsData1[] = {
    0x01, 0x01, 0x01, 0x03, 0x14, 0x00, 0x06, 0x20,
    0x01, 0x01, 0x02, 0xCC, 0xC1, 0x5C, 0x91, 0xD0,
    0x14, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x03, 0x15, 0xEE, 0xFD, 0x01, 0x01, 0x01,
    0x03, 0x00, 0x01, 0x6D, 0x04, 0x0E, 0x13, 0x03,
    0x41, 0x15, 0x5C, 0xD8, 0x9E, 0x21, 0xD8, 0xA3,
    0x07, 0x04, 0x05, 0x06, 0x06, 0x06, 0xFF, 0x18,
    0x09, 0x00, 0x01, 0x01, 0x7C, 0x50, 0x22, 0x19,
    0x01, 0x00, 0x02, 0xFA, 0x5A, 0x22, 0x19, 0x01,
    0x00, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00
};
static uint8_t g_podsData2[] = {0x00};
static uint8_t g_pencilData1[] = {
    0x03, 0x19, 0xC7, 0x03, 0x0F, 0x16, 0xEE, 0xFD,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x12, 0x09, 0x48, 0x55,
    0x41, 0x57, 0x45, 0x49, 0x20, 0x4D, 0x2D, 0x50,
    0x65, 0x6E, 0x63, 0x69, 0x6C, 0x20, 0x33};

static uint8_t pencilName[] = {
    0x48,  // 'H'
    0x55,  // 'U'
    0x41,  // 'A'
    0x57,  // 'W'
    0x45,  // 'E'
    0x49,  // 'I'
    0x20,  // ' ' (空格)
    0x4D,  // 'M'
    0x2D,  // '-'
    0x50,  // 'P'
    0x65,  // 'e'
    0x6E,  // 'n'
    0x63,  // 'c'
    0x69,  // 'i'
    0x6C,  // 'l'
    0x20,  // ' ' (空格)
    0x33   // '3'
};

static uint8_t g_data[] = {0x00, 0x06, 0x20, 0x01, 0x01, 0x02, 0xCC, 0xC1, 0x5C, 0x91, 0xD0, 0x14, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t g_data2[] = {0xEE, 0xFD, 0x01, 0x01, 0x01, 0x04, 0x00, 0x00};
static uint8_t g_dataMask2[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00};

static NLSTK_DevdSleScanExterCbk_S g_cbk = {0};

NLSTK_Errcode_E NLSTK_DevdRegScanEventCbk(NLSTK_DevdSleScanExterCbk_S *scanEventCbk)
{
    NLSTK_LOG_INFO("enter DevdRegScanEventCbkStub");
    if (scanEventCbk) {
        g_cbk.reportCbk = scanEventCbk->reportCbk;
        g_cbk.scanCbk = scanEventCbk->scanCbk;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void DevdReportCbkStub(uint8_t *data, uint16_t dataLen, bool rsp)
{
    NLSTK_DevdAdvReportInfo_S *report = (NLSTK_DevdAdvReportInfo_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvReportInfo_S) + dataLen);
    if (report == NULL) {
        return;
    }
    report->dataLength = dataLen;
    (void)memcpy_s(report->addr, SLE_ADDR_LEN, g_addr, SLE_ADDR_LEN);
    report->rssi = 10;
    report->extendParams.primFrameType = 0;
    report->extendParams.eventType = rsp ? 0b1010 : 0b10;
    (void)memcpy_s(report->data, dataLen, data, dataLen);
    if (g_cbk.reportCbk) {
        g_cbk.reportCbk(report);
    }
    SDF_MemFree(report);
}

static void DevdSleStartScanInnerStub(void *arg)
{
    NLSTK_LOG_INFO("enter DevdSleStartScanInnerStub");
    NLSTK_DevdSleScanParams_S *sleScanParams = (NLSTK_DevdSleScanParams_S *)arg;
    g_frameType = sleScanParams->frameType;
    g_window1 = sleScanParams->params[0].scanWindow;
    g_interval1 = sleScanParams->params[0].scanInterval;
    uint8_t scanPhyCount = DLI_GetPhyCountByFrameType(sleScanParams->frameType);
    if (scanPhyCount == 2) {
        g_window2 = sleScanParams->params[1].scanWindow;
        g_interval2 = sleScanParams->params[1].scanInterval;
    }
    g_enable = true;
    if (g_cbk.scanCbk) {
        g_cbk.scanCbk(0x09, 0); // enable success
    }
}

NLSTK_Errcode_E NLSTK_DevdSleStartScan(NLSTK_DevdSleScanParams_S *sleScanParams)
{
    NLSTK_LOG_INFO("enter DevdSleStartScanStub");
    NLSTK_CHECK_RETURN(sleScanParams != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD] sleScanParams is null");
    uint8_t scanPhyCount = DLI_GetPhyCountByFrameType(sleScanParams->frameType);
    NLSTK_DevdSleScanParams_S *scanParams = (NLSTK_DevdSleScanParams_S *)SDF_MemZalloc(
        sizeof(NLSTK_DevdSleScanParams_S) + scanPhyCount * sizeof(NLSTK_DevdSleScanParamsNoPhy_S));
    NLSTK_CHECK_RETURN(scanParams != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD] scanParams is null");
    scanParams->localAddrType = sleScanParams->localAddrType;
    scanParams->scanFilterPolicy = sleScanParams->scanFilterPolicy;
    scanParams->frameType = sleScanParams->frameType;
    for (uint8_t idx = 0; idx < scanPhyCount; idx++) {
        scanParams->params[idx].scanType = sleScanParams->params[idx].scanType;
        scanParams->params[idx].scanInterval = sleScanParams->params[idx].scanInterval;
        scanParams->params[idx].scanWindow = sleScanParams->params[idx].scanWindow;
    }
    if ((scanParams->localAddrType >= ADDR_TYPE_END) ||
        (scanParams->scanFilterPolicy > SCAN_FLT_EXTEND) ||
        (scanParams->frameType > ((uint8_t)SCAN_FRAME_TYPE_1 | (uint8_t)SCAN_FRAME_TYPE_4))) {
        NLSTK_LOG_ERROR("[DEVD] set scan param check failed");
        SDF_MemFree(scanParams);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    for (uint8_t idx = 0; idx < scanPhyCount; idx++) {
        if ((scanParams->params[idx].scanType > SCAN_TYPE_ACTIVE) ||
            (scanParams->params[idx].scanInterval < SCAN_INTERVAL_MIN) ||
            (scanParams->params[idx].scanWindow < SCAN_WINDOW_MIN)) {
            NLSTK_LOG_ERROR("[DEVD] set scan param check failed");
            SDF_MemFree(scanParams);
            return NLSTK_ERRCODE_PARAM_ERR;
        }
    }
    if (TEST_SchedulePostTaskQueueStub(DevdSleStartScanInnerStub, scanParams, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[DEVD] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void DevdSleEnableScanInnerStub(void *arg)
{
    NLSTK_LOG_INFO("enter DevdSleEnableScanInnerStub");
    g_enable = false;
    if (g_cbk.scanCbk) {
        g_cbk.scanCbk(0x0A, 0); // disable success
    }
}

NLSTK_Errcode_E NLSTK_DevdSleEnableScan(NLSTK_DevdSleScanEnable_S *sleScanEnable)
{
    NLSTK_LOG_INFO("enter DevdSleEnableScanStub");
    TEST_SchedulePostTaskQueueStub(DevdSleEnableScanInnerStub, NULL, NULL);
    return NLSTK_ERRCODE_SUCCESS;
}

static void DevdScanReportCallbackStub(NLSTK_DevdAdvResult_S *result)
{
    for (size_t i = 0; i < result->scannerIds->size; i++) {
        uint32_t *scannerId = (uint32_t *)SDF_VectorElementAt(result->scannerIds, i);
        NLSTK_LOG_INFO("Test scannerId %u", *scannerId);
    }
    NLSTK_LOG_INFO("Test addr %s", SDF_GET_UINT8_STR(result->addr.addr, SLE_ADDR_LEN));
    NLSTK_LOG_INFO("Test %d %d %d %d", result->rssi, result->txPower, result->frameType, result->discoveryLevel);
    NLSTK_LOG_INFO("Test name %s", SDF_GET_UINT8_STR(result->localName.data, result->localName.len));
    for (size_t i = 0; i < result->serviceDataList->size; i++) {
        NLSTK_DevdAdvServiceData_S *data = (NLSTK_DevdAdvServiceData_S *)SDF_VectorElementAt(result->serviceDataList, i);
        NLSTK_LOG_INFO("Test serviceData %s, %s", SDF_GET_UINT8_STR(data->uuid, SERVICE_UUID_LEN_128),
            SDF_GET_UINT8_STR(data->data, data->len));
    }
    for (size_t i = 0; i < result->serviceUuids->size; i++) {
        NLSTK_DevdAdvServiceUuid_S *data = (NLSTK_DevdAdvServiceUuid_S *)SDF_VectorElementAt(result->serviceUuids, i);
        NLSTK_LOG_INFO("Test serviceUuids %s", SDF_GET_UINT8_STR(data->uuid, SERVICE_UUID_LEN_128));
    }
    for (size_t i = 0; i < result->manufacturerDataList->size; i++) {
        NLSTK_DevdAdvManufacturerData_S *data = (NLSTK_DevdAdvManufacturerData_S *)SDF_VectorElementAt(result->manufacturerDataList, i);
        NLSTK_LOG_INFO("Test manufacturerDataList %d, %s", data->manufacturerId, SDF_GET_UINT8_STR(data->data, data->len));
    }
}

class DEVD_SCAN_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    void SetUp() override
    {
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskQueueStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        (void)DevdScanInit();
        NLSTK_DevdScanCbk_S scanCbk = {0};
        scanCbk.onScanCallback = DevdScanReportCallbackStub;
        (void)NLSTK_DevdRegScanModule(&g_moduleId, &scanCbk);
    }

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    void TearDown() override
    {
        NLSTK_DevdDeregScanModule(g_moduleId);
        g_moduleId = DEVD_MAX_MODULE_ID;
        DevdScanDeInit();
        TEST_StackScheduleDeInit();
    }

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {}

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {}
};


TEST_F(DEVD_SCAN_TEST, AllocScannerTest001)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    uint32_t scannerId = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId, DEVD_INVALID_SCANNER_ID);
    NLSTK_DevdRemoveScannerId(scannerId);
}

TEST_F(DEVD_SCAN_TEST, StartScanTest001)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    uint32_t scannerId = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting = {0};
    setting.scanMode = SCAN_MODE_BALANCED;
    setting.frameType = DEVD_SCAN_FRAME_TYPE_4;
    ret = NLSTK_DevdStartScan(scannerId, &setting, NULL, 0);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_frameType, DEVD_SCAN_FRAME_TYPE_4);
    EXPECT_EQ(g_window1, 1024);
    EXPECT_EQ(g_interval1, 4096);
    EXPECT_EQ(g_enable, true);

    setting.scanMode = SCAN_MODE_FULL_SCAN;
    setting.frameType = DEVD_SCAN_FRAME_TYPE_1;
    ret = NLSTK_DevdStartScan(scannerId, &setting, NULL, 0);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_frameType, DEVD_SCAN_FRAME_TYPE_1 | DEVD_SCAN_FRAME_TYPE_4);
    EXPECT_EQ(g_window1, 2048);
    EXPECT_EQ(g_interval1, 4096);
    EXPECT_EQ(g_window2, 1024);
    EXPECT_EQ(g_interval2, 4096);
    EXPECT_EQ(g_enable, true);

    ret = NLSTK_DevdStopScan(scannerId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_enable, false);
}

TEST_F(DEVD_SCAN_TEST, StartScanTest002)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    uint32_t scannerId1 = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId1);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId1, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting1 = {0};
    setting1.scanMode = SCAN_MODE_LOW_POWER;
    setting1.frameType = DEVD_SCAN_FRAME_TYPE_1;
    ret = NLSTK_DevdStartScan(scannerId1, &setting1, NULL, 0);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_window1, 900);
    EXPECT_EQ(g_interval1, 9000);
    EXPECT_EQ(g_enable, true);

    uint32_t scannerId2 = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId2, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting2 = {0};
    setting2.scanMode = SCAN_MODE_BALANCED;
    setting2.frameType = DEVD_SCAN_FRAME_TYPE_1;
    ret = NLSTK_DevdStartScan(scannerId2, &setting2, NULL, 0);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_window1, 1024);
    EXPECT_EQ(g_interval1, 4096);
    EXPECT_EQ(g_enable, true);

    uint32_t scannerId3 = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId3);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId3, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting3 = {0};
    setting3.scanMode = SCAN_MODE_FULL_SCAN;
    setting3.frameType = DEVD_SCAN_FRAME_TYPE_1;
    ret = NLSTK_DevdStartScan(scannerId3, &setting3, NULL, 0);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_window1, 2048);
    EXPECT_EQ(g_interval1, 4096);
    EXPECT_EQ(g_enable, true);

    ret = NLSTK_DevdStopScan(scannerId3);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_window1, 1024);
    EXPECT_EQ(g_interval1, 4096);
    EXPECT_EQ(g_enable, true);

    ret = NLSTK_DevdStopScan(scannerId1);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_window1, 1024);
    EXPECT_EQ(g_interval1, 4096);
    EXPECT_EQ(g_enable, true);

    ret = NLSTK_DevdStopScan(scannerId2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_enable, false);
}

TEST_F(DEVD_SCAN_TEST, StartScanTest003)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    uint32_t scannerId1 = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId1);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId1, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting1 = {0};
    setting1.scanMode = SCAN_MODE_LOW_POWER;
    setting1.frameType = DEVD_SCAN_FRAME_TYPE_1;
    ret = NLSTK_DevdStartScan(scannerId1, &setting1, NULL, 0);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_frameType, DEVD_SCAN_FRAME_TYPE_1);
    EXPECT_EQ(g_window1, 900);
    EXPECT_EQ(g_interval1, 9000);
    EXPECT_EQ(g_enable, true);

    uint32_t scannerId2 = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId2, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting2 = {0};
    setting2.scanMode = SCAN_MODE_BALANCED;
    setting2.frameType = DEVD_SCAN_FRAME_TYPE_4;
    ret = NLSTK_DevdStartScan(scannerId2, &setting2, NULL, 0);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_frameType, DEVD_SCAN_FRAME_TYPE_1 | DEVD_SCAN_FRAME_TYPE_4);
    EXPECT_EQ(g_window1, 900);
    EXPECT_EQ(g_interval1, 9000);
    EXPECT_EQ(g_window2, 1024);
    EXPECT_EQ(g_interval2, 4096);
    EXPECT_EQ(g_enable, true);

    uint32_t scannerId3 = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId3);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId3, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting3 = {0};
    setting3.scanMode = SCAN_MODE_FULL_SCAN;
    setting3.frameType = DEVD_SCAN_FRAME_TYPE_4;
    ret = NLSTK_DevdStartScan(scannerId3, &setting3, NULL, 0);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_frameType, DEVD_SCAN_FRAME_TYPE_1 | DEVD_SCAN_FRAME_TYPE_4);
    EXPECT_EQ(g_window1, 900);
    EXPECT_EQ(g_interval1, 9000);
    EXPECT_EQ(g_window2, 2048);
    EXPECT_EQ(g_interval2, 4096);
    EXPECT_EQ(g_enable, true);

    ret = NLSTK_DevdStopScan(scannerId1);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_frameType, DEVD_SCAN_FRAME_TYPE_4);
    EXPECT_EQ(g_window1, 2048);
    EXPECT_EQ(g_interval1, 4096);
    EXPECT_EQ(g_enable, true);

    ret = NLSTK_DevdStopScan(scannerId3);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_frameType, DEVD_SCAN_FRAME_TYPE_4);
    EXPECT_EQ(g_window1, 1024);
    EXPECT_EQ(g_interval1, 4096);
    EXPECT_EQ(g_enable, true);

    ret = NLSTK_DevdStopScan(scannerId2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_enable, false);
}

TEST_F(DEVD_SCAN_TEST, StartScanTest004)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    uint32_t scannerId1 = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId1);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId1, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting1 = {0};
    setting1.scanMode = SCAN_MODE_LOW_POWER;
    setting1.frameType = DEVD_SCAN_FRAME_TYPE_1;
    ret = NLSTK_DevdStartScan(scannerId1, &setting1, NULL, 0);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_window1, 900);
    EXPECT_EQ(g_interval1, 9000);
    EXPECT_EQ(g_enable, true);

    uint32_t scannerId2 = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId2, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting2 = {0};
    setting2.scanMode = SCAN_MODE_BALANCED;
    setting2.frameType = DEVD_SCAN_FRAME_TYPE_1;
    ret = NLSTK_DevdStartScan(scannerId2, &setting2, NULL, 0);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_window1, 1024);
    EXPECT_EQ(g_interval1, 4096);
    EXPECT_EQ(g_enable, true);

    ret = NLSTK_DevdStopAllScan();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_enable, false);
}

TEST_F(DEVD_SCAN_TEST, StartScanTest005)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    uint32_t scannerId1 = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId1);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId1, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting1 = {0};
    setting1.scanMode = SCAN_MODE_LOW_POWER;
    setting1.frameType = DEVD_SCAN_FRAME_TYPE_1;
    ret = NLSTK_DevdStartScan(scannerId1, &setting1, NULL, 0);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_window1, 900);
    EXPECT_EQ(g_interval1, 9000);
    EXPECT_EQ(g_enable, true);

    uint32_t scannerId2 = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId2, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting2 = {0};
    setting2.scanMode = SCAN_MODE_BALANCED;
    setting2.frameType = DEVD_SCAN_FRAME_TYPE_1;
    ret = NLSTK_DevdStartScan(scannerId2, &setting2, NULL, 0);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_window1, 1024);
    EXPECT_EQ(g_interval1, 4096);
    EXPECT_EQ(g_enable, true);

    NLSTK_DevdRemoveScannerId(scannerId2);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_window1, 900);
    EXPECT_EQ(g_interval1, 9000);
    EXPECT_EQ(g_enable, true);

    NLSTK_DevdRemoveScannerId(scannerId1);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_enable, false);
}

TEST_F(DEVD_SCAN_TEST, ScanReportParse001)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    uint32_t scannerId = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting = {0};
    setting.scanMode = SCAN_MODE_BALANCED;
    setting.frameType = DEVD_SCAN_FRAME_TYPE_1;
    ret = NLSTK_DevdStartScan(scannerId, &setting, NULL, 0);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    TEST_RunQueueStubSchedule();

    DevdReportCbkStub(g_mouseData1, sizeof(g_mouseData1), false);
    DevdReportCbkStub(g_mouseData2, sizeof(g_mouseData2), true);

    DevdReportCbkStub(g_podsData1, sizeof(g_podsData1), false);
    DevdReportCbkStub(g_podsData2, sizeof(g_podsData2), true);

    DevdReportCbkStub(g_pencilData1, sizeof(g_pencilData1), true);
}

// 命中手写笔的name
TEST_F(DEVD_SCAN_TEST, ScanFilter001)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    uint32_t scannerId = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting = {0};
    setting.scanMode = SCAN_MODE_BALANCED;
    setting.frameType = DEVD_SCAN_FRAME_TYPE_1;
    NLSTK_DevdScanFilter_S filter = {0};
    filter.hasAddr = true;
    (void)memcpy_s(filter.addr.addr, SLE_ADDR_LEN, g_addr, SLE_ADDR_LEN);
    filter.hasRssiThreshold = true;
    filter.rssiThreshold = 5;
    filter.name.len = sizeof(pencilName);
    filter.name.data = pencilName;
    ret = NLSTK_DevdStartScan(scannerId, &setting, &filter, 1);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    TEST_RunQueueStubSchedule();

    DevdReportCbkStub(g_mouseData1, sizeof(g_mouseData1), false);
    DevdReportCbkStub(g_mouseData2, sizeof(g_mouseData2), true);

    DevdReportCbkStub(g_podsData1, sizeof(g_podsData1), false);
    DevdReportCbkStub(g_podsData2, sizeof(g_podsData2), true);

    DevdReportCbkStub(g_pencilData1, sizeof(g_pencilData1), true);
}

// 命中耳机的serviceData
TEST_F(DEVD_SCAN_TEST, ScanFilter002)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    uint32_t scannerId = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting = {0};
    setting.scanMode = SCAN_MODE_BALANCED;
    setting.frameType = DEVD_SCAN_FRAME_TYPE_1;
    NLSTK_DevdScanFilter_S filter = {0};
    filter.hasAddr = true;
    (void)memcpy_s(filter.addr.addr, SLE_ADDR_LEN, g_addr, SLE_ADDR_LEN);
    filter.hasRssiThreshold = true;
    filter.rssiThreshold = 5;
    filter.serviceData.len = sizeof(g_data);
    filter.serviceData.data = g_data;
    ret = NLSTK_DevdStartScan(scannerId, &setting, &filter, 1);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    TEST_RunQueueStubSchedule();

    DevdReportCbkStub(g_mouseData1, sizeof(g_mouseData1), false);
    DevdReportCbkStub(g_mouseData2, sizeof(g_mouseData2), true);

    DevdReportCbkStub(g_podsData1, sizeof(g_podsData1), false);
    DevdReportCbkStub(g_podsData2, sizeof(g_podsData2), true);

    DevdReportCbkStub(g_pencilData1, sizeof(g_pencilData1), true);
}

// 命中鼠标掩码后的serviceData
TEST_F(DEVD_SCAN_TEST, ScanFilter003)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    uint32_t scannerId = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting = {0};
    setting.scanMode = SCAN_MODE_BALANCED;
    setting.frameType = DEVD_SCAN_FRAME_TYPE_1;
    NLSTK_DevdScanFilter_S filter = {0};
    filter.hasAddr = true;
    (void)memcpy_s(filter.addr.addr, SLE_ADDR_LEN, g_addr, SLE_ADDR_LEN);
    filter.hasRssiThreshold = true;
    filter.rssiThreshold = 5;
    filter.serviceData.len = sizeof(g_data2);
    filter.serviceData.data = g_data2;
    filter.serviceDataMask.len = sizeof(g_dataMask2);
    filter.serviceDataMask.data = g_dataMask2;
    ret = NLSTK_DevdStartScan(scannerId, &setting, &filter, 1);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    TEST_RunQueueStubSchedule();

    DevdReportCbkStub(g_mouseData1, sizeof(g_mouseData1), false);
    DevdReportCbkStub(g_mouseData2, sizeof(g_mouseData2), true);

    DevdReportCbkStub(g_podsData1, sizeof(g_podsData1), false);
    DevdReportCbkStub(g_podsData2, sizeof(g_podsData2), true);

    DevdReportCbkStub(g_pencilData1, sizeof(g_pencilData1), true);
}

// 测试多路过滤能力,预期全部上报
TEST_F(DEVD_SCAN_TEST, ScanFilter004)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    uint32_t scannerId1 = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId1);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId1, DEVD_INVALID_SCANNER_ID);

    NLSTK_DevdScanSetting_S setting = {0};
    setting.scanMode = SCAN_MODE_BALANCED;
    setting.frameType = DEVD_SCAN_FRAME_TYPE_1;
    NLSTK_DevdScanFilter_S *filter = (NLSTK_DevdScanFilter_S *)SDF_MemZalloc(3 * sizeof(NLSTK_DevdScanFilter_S));
    EXPECT_NE(filter, nullptr);
    filter[0].name.len = sizeof(pencilName);
    filter[0].name.data = pencilName;
    filter[1].serviceData.len = sizeof(g_data);
    filter[1].serviceData.data = g_data;
    filter[2].serviceData.len = sizeof(g_data2);
    filter[2].serviceData.data = g_data2;
    filter[2].serviceDataMask.len = sizeof(g_dataMask2);
    filter[2].serviceDataMask.data = g_dataMask2;
    ret = NLSTK_DevdStartScan(scannerId1, &setting, filter, 3);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    uint32_t scannerId2 = DEVD_INVALID_SCANNER_ID;
    ret = NLSTK_DevdAllocScannerId(g_moduleId, &scannerId2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId2, DEVD_INVALID_SCANNER_ID);
    ret = NLSTK_DevdStartScan(scannerId2, &setting, NULL, 0);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    TEST_RunQueueStubSchedule();

    DevdReportCbkStub(g_mouseData1, sizeof(g_mouseData1), false);
    DevdReportCbkStub(g_mouseData2, sizeof(g_mouseData2), true);

    DevdReportCbkStub(g_podsData1, sizeof(g_podsData1), false);
    DevdReportCbkStub(g_podsData2, sizeof(g_podsData2), true);

    DevdReportCbkStub(g_pencilData1, sizeof(g_pencilData1), true);

    SDF_MemFree(filter);
}

// ===================== 扫描时长与扫描模式矩阵用例 =====================
#include "devd_scan_manager.h"
#include "devd_scan_type.h"

static uint32_t AllocScannerOk(void)
{
    uint32_t scannerId = DEVD_INVALID_SCANNER_ID;
    EXPECT_EQ(NLSTK_DevdAllocScannerId(g_moduleId, &scannerId), NLSTK_ERRCODE_SUCCESS);
    EXPECT_NE(scannerId, DEVD_INVALID_SCANNER_ID);
    return scannerId;
}

static NLSTK_Errcode_E StartScanWithDuration(uint32_t scannerId, uint8_t frameType, int32_t duration)
{
    NLSTK_DevdScanSetting_S setting = {0};
    setting.scanMode = SCAN_MODE_BALANCED;
    setting.frameType = frameType;
    setting.duration = duration;
    return NLSTK_DevdStartScan(scannerId, &setting, NULL, 0);
}

static NLSTK_Errcode_E StartScanWithMode(uint32_t scannerId, uint8_t frameType, int32_t scanMode,
    int64_t reportDelayMillis)
{
    NLSTK_DevdScanSetting_S setting = {0};
    setting.scanMode = scanMode;
    setting.frameType = frameType;
    setting.duration = 3600;
    setting.reportDelayMillis = reportDelayMillis;
    return NLSTK_DevdStartScan(scannerId, &setting, NULL, 0);
}

static NLSTK_DevdScanner_S *FindScannerById(uint32_t scannerId)
{
    NLSTK_DevdStateMachine_S *scanStm = DevdGetStateMachine();
    if (scanStm == NULL) {
        return NULL;
    }
    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(scanStm, scanManager);
    if (manager == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < manager->scanners->size; i++) {
        NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(manager->scanners, i);
        if (scanner->scannerId == scannerId) {
            return scanner;
        }
    }
    return NULL;
}

// 直改内部 expireTime，构造确定性的秒/纳秒到期序列
static void SetScannerExpireTime(uint32_t scannerId, bool frame4, time_t tvSec, long tvNsec)
{
    NLSTK_DevdScanner_S *scanner = FindScannerById(scannerId);
    ASSERT_NE(scanner, nullptr);
    NLSTK_DevdScanSettingInner_S *inner = frame4 ? &scanner->frame4ScanSetting : &scanner->scanSetting;
    inner->expireTime.tv_sec = tvSec;
    inner->expireTime.tv_nsec = tvNsec;
}

// 清空定时器 mock 期望并恢复队列 stub：TearDown 的 DeInit 还会触发 ScheduleTimerDel，
// 不得进入捕获本用例局部变量的 lambda
static void ClearTimerMockExpectations(NiceMock<ScheduleMock> &mock)
{
    ::testing::Mock::VerifyAndClearExpectations(&mock);
    EXPECT_CALL(mock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskQueueStub);
    EXPECT_CALL(mock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
}

// 帧一/帧四各自配置 duration 后，仅各自的到期参数置位
TEST_F(DEVD_SCAN_TEST, ScanDurationTest001)
{
    uint32_t scannerId1 = AllocScannerOk();
    uint32_t scannerId2 = AllocScannerOk();
    EXPECT_EQ(StartScanWithDuration(scannerId1, DEVD_SCAN_FRAME_TYPE_1, 10), NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(StartScanWithDuration(scannerId2, DEVD_SCAN_FRAME_TYPE_4, 20), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    NLSTK_DevdScanner_S *scanner1 = FindScannerById(scannerId1);
    ASSERT_NE(scanner1, nullptr);
    EXPECT_TRUE(scanner1->scanSetting.hasDuration);
    EXPECT_FALSE(scanner1->frame4ScanSetting.hasDuration);
    NLSTK_DevdScanner_S *scanner2 = FindScannerById(scannerId2);
    ASSERT_NE(scanner2, nullptr);
    EXPECT_FALSE(scanner2->scanSetting.hasDuration);
    EXPECT_TRUE(scanner2->frame4ScanSetting.hasDuration);
}

// 多个帧一扫描并存时，到期定时器指向最早到期时间：首个直接入选、秒级更早者替换、
// 同秒比纳秒、更晚者保留
TEST_F(DEVD_SCAN_TEST, ScanDurationTest002)
{
    uint32_t scannerId[6] = {0}; // 前 5 个为布景 scanner，第 6 个仅作移除触发
    for (size_t i = 0; i < sizeof(scannerId) / sizeof(scannerId[0]); i++) {
        scannerId[i] = AllocScannerOk();
    }
    for (size_t i = 0; i < sizeof(scannerId) / sizeof(scannerId[0]) - 1; i++) {
        EXPECT_EQ(StartScanWithDuration(scannerId[i], DEVD_SCAN_FRAME_TYPE_1, 3600), NLSTK_ERRCODE_SUCCESS);
    }
    TEST_RunQueueStubSchedule();

    time_t base = time(NULL) + 3600;
    SetScannerExpireTime(scannerId[0], false, base, 900);   // 首个 duration 直接入选
    SetScannerExpireTime(scannerId[1], false, base, 100);   // 同秒纳秒更小，替换
    SetScannerExpireTime(scannerId[2], false, base, 500);   // 同秒纳秒更大，保留
    SetScannerExpireTime(scannerId[3], false, base - 2, 0); // 秒级更早，替换为最终最早项
    SetScannerExpireTime(scannerId[4], false, base - 1, 0); // 秒级更晚，保留

    int addCount = 0;
    int fakeHandle = 0;
    time_t armedMs = 0;
    int *durationTimerPtr = &DEVD_STM_M(DevdGetStateMachine(), scanManager)->durationTimer;
    EXPECT_CALL(scheduleMock, ScheduleTimerAdd).WillRepeatedly(Invoke(
        [&](int *handle, SDF_TimerParam *param) -> uint32_t {
            *handle = ++fakeHandle; // 模拟真实定时器框架分配句柄
            if (handle == durationTimerPtr) { // 排除扫描 STM 内部定时器（CP_TimerAdd 同走此 mock）
                addCount++;
                armedMs = param->expires;
            }
            return 0; // SDF_OK
        }));

    EXPECT_EQ(NLSTK_DevdStopScan(scannerId[5]), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    EXPECT_EQ(addCount, 1); // 移除参数路径仅重布防一次
    // 到期定时器指向最早到期项 scannerId[3]（约 3598 秒，留 ±0.5 秒执行抖动余量）
    EXPECT_GT(armedMs, (time_t)3597500);
    EXPECT_LT(armedMs, (time_t)3598500);
    ClearTimerMockExpectations(scheduleMock);
}

// 帧四侧同 ScanDurationTest002：到期定时器指向最早到期时间
TEST_F(DEVD_SCAN_TEST, ScanDurationTest003)
{
    uint32_t scannerId[6] = {0};
    for (size_t i = 0; i < sizeof(scannerId) / sizeof(scannerId[0]); i++) {
        scannerId[i] = AllocScannerOk();
    }
    for (size_t i = 0; i < sizeof(scannerId) / sizeof(scannerId[0]) - 1; i++) {
        EXPECT_EQ(StartScanWithDuration(scannerId[i], DEVD_SCAN_FRAME_TYPE_4, 3600), NLSTK_ERRCODE_SUCCESS);
    }
    TEST_RunQueueStubSchedule();

    time_t base = time(NULL) + 3600;
    SetScannerExpireTime(scannerId[0], true, base, 900);
    SetScannerExpireTime(scannerId[1], true, base, 100);
    SetScannerExpireTime(scannerId[2], true, base, 500);
    SetScannerExpireTime(scannerId[3], true, base - 2, 0);
    SetScannerExpireTime(scannerId[4], true, base - 1, 0);

    int addCount = 0;
    int fakeHandle = 0;
    time_t armedMs = 0;
    int *durationTimerPtr = &DEVD_STM_M(DevdGetStateMachine(), scanManager)->durationTimer;
    EXPECT_CALL(scheduleMock, ScheduleTimerAdd).WillRepeatedly(Invoke(
        [&](int *handle, SDF_TimerParam *param) -> uint32_t {
            *handle = ++fakeHandle;
            if (handle == durationTimerPtr) {
                addCount++;
                armedMs = param->expires;
            }
            return 0;
        }));

    EXPECT_EQ(NLSTK_DevdStopScan(scannerId[5]), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    EXPECT_EQ(addCount, 1);
    EXPECT_GT(armedMs, (time_t)3597500);
    EXPECT_LT(armedMs, (time_t)3598500);
    ClearTimerMockExpectations(scheduleMock);
}

// 同一扫描先后配置帧一/帧四时长：两类到期参数并存，各自按设置登记
TEST_F(DEVD_SCAN_TEST, ScanDurationTest004)
{
    uint32_t scannerId1 = AllocScannerOk();
    EXPECT_EQ(StartScanWithDuration(scannerId1, DEVD_SCAN_FRAME_TYPE_1, 10), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(StartScanWithDuration(scannerId1, DEVD_SCAN_FRAME_TYPE_4, 5), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    NLSTK_DevdScanner_S *scanner1 = FindScannerById(scannerId1);
    ASSERT_NE(scanner1, nullptr);
    EXPECT_TRUE(scanner1->scanSetting.hasDuration);
    EXPECT_TRUE(scanner1->frame4ScanSetting.hasDuration);
}

// 无任何扫描配置时长时，停止扫描/停止全部扫描不布防到期定时器
TEST_F(DEVD_SCAN_TEST, ScanDurationTest005)
{
    uint32_t scannerId1 = AllocScannerOk();
    EXPECT_EQ(StartScanWithDuration(scannerId1, DEVD_SCAN_FRAME_TYPE_1, 0), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule(); // duration==0 时启动路径不布防到期定时器
    EXPECT_EQ(NLSTK_DevdStopScan(scannerId1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    uint32_t scannerId2 = AllocScannerOk();
    EXPECT_EQ(StartScanWithDuration(scannerId2, DEVD_SCAN_FRAME_TYPE_4, 0), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(NLSTK_DevdStopAllScan(), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    NLSTK_DevdScanner_S *scanner2 = FindScannerById(scannerId2);
    ASSERT_NE(scanner2, nullptr);
    EXPECT_FALSE(scanner2->scanSetting.hasDuration);
    EXPECT_FALSE(scanner2->frame4ScanSetting.hasDuration);
}

// 存在已过期的时长参数时同步触发到期处理：过期项被清理，未到期项保留并重布防
TEST_F(DEVD_SCAN_TEST, ScanDurationTest006)
{
    uint32_t scannerId1 = AllocScannerOk();
    uint32_t scannerId2 = AllocScannerOk();
    uint32_t scannerId3 = AllocScannerOk();
    EXPECT_EQ(StartScanWithDuration(scannerId1, DEVD_SCAN_FRAME_TYPE_1, 10), NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(StartScanWithDuration(scannerId2, DEVD_SCAN_FRAME_TYPE_1, 30), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    SetScannerExpireTime(scannerId1, false, time(NULL) - 2, 0); // 改为已过期

    EXPECT_EQ(NLSTK_DevdStopScan(scannerId3), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    NLSTK_DevdScanner_S *scanner1 = FindScannerById(scannerId1);
    ASSERT_NE(scanner1, nullptr);
    EXPECT_FALSE(scanner1->scanSetting.hasDuration); // 过期参数已被清理
    NLSTK_DevdScanner_S *scanner2 = FindScannerById(scannerId2);
    ASSERT_NE(scanner2, nullptr);
    EXPECT_TRUE(scanner2->scanSetting.hasDuration); // 未到期参数保留并重布防
}

// 定时器到期路径：按剩余最近到期时间重布防，且不误删新布防的定时器句柄
TEST_F(DEVD_SCAN_TEST, ScanDurationTest007)
{
    int addCount = 0;
    int delCount = 0;
    int fakeHandle = 0;
    int durHandle = NLSTK_DEVD_TIMER_NO_USED_HANDLE;
    SDF_TimerCallback timerCbk = NULL;
    void *timerArg = NULL;
    int *durationTimerPtr = &DEVD_STM_M(DevdGetStateMachine(), scanManager)->durationTimer;
    EXPECT_CALL(scheduleMock, ScheduleTimerAdd).WillRepeatedly(Invoke(
        [&](int *handle, SDF_TimerParam *param) -> uint32_t {
            *handle = ++fakeHandle;
            if (handle == durationTimerPtr) { // 仅统计 duration 定时器，排除扫描 STM 内部定时器
                durHandle = fakeHandle;
                timerCbk = param->callback;
                timerArg = param->args;
                addCount++;
            }
            return 0; // SDF_OK
        }));
    EXPECT_CALL(scheduleMock, ScheduleTimerDel).WillRepeatedly(Invoke(
        [&](int handle) {
            if (handle == durHandle && durHandle != NLSTK_DEVD_TIMER_NO_USED_HANDLE) {
                delCount++;
                durHandle = NLSTK_DEVD_TIMER_NO_USED_HANDLE;
            }
        }));

    uint32_t scannerId1 = AllocScannerOk();
    uint32_t scannerId2 = AllocScannerOk();
    EXPECT_EQ(StartScanWithDuration(scannerId1, DEVD_SCAN_FRAME_TYPE_1, 10), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule(); // 布防 #1
    EXPECT_EQ(StartScanWithDuration(scannerId2, DEVD_SCAN_FRAME_TYPE_1, 20), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule(); // 删 #1 后布防 #2（更早的到期时间不变）
    EXPECT_EQ(addCount, 2);
    EXPECT_EQ(delCount, 1);
    ASSERT_NE(timerCbk, nullptr);

    timerCbk(timerArg); // 模拟定时器到期，走真实到期处理路径
    TEST_RunQueueStubSchedule(); // 排空回调内 ProcessMessage 级联任务
    EXPECT_EQ(addCount, 3); // 依剩余最近到期时间重布防
    EXPECT_EQ(delCount, 1); // 到期回调路径不误删新句柄

    NLSTK_DevdScanner_S *scanner1 = FindScannerById(scannerId1);
    ASSERT_NE(scanner1, nullptr);
    EXPECT_TRUE(scanner1->scanSetting.hasDuration); // 未到期参数不被清理
    // 清定时器期望：TearDown 的 DeInit 还会触发 ScheduleTimerDel，不得进入悬垂 lambda
    ClearTimerMockExpectations(scheduleMock);
}

// 全部合法扫描模式 × 直接上报档：间隔与窗口按各模式默认档配置
TEST_F(DEVD_SCAN_TEST, ScanModeMatrixTest)
{
    struct ModeExpect {
        int32_t mode;
        uint16_t window;
        uint16_t interval;
    };
    const ModeExpect expects[] = {
        { SCAN_MODE_LOW_POWER, SCAN_MODE_LOW_POWER_WINDOW, SCAN_MODE_LOW_POWER_INTERVAL },
        { SCAN_MODE_BALANCED, SCAN_MODE_BALANCED_WINDOW, SCAN_MODE_BALANCED_INTERVAL },
        { SCAN_MODE_LOW_LATENCY, SCAN_MODE_LOW_LATENCY_WINDOW, SCAN_MODE_LOW_LATENCY_INTERVAL },
        { SCAN_MODE_FULL_SCAN, SCAN_MODE_FULL_SCAN_WINDOW, SCAN_MODE_FULL_SCAN_INTERVAL },
        { SCAN_MODE_OP_P2_60_3000, SCAN_MODE_OP_P2_60_3000_WINDOW, SCAN_MODE_OP_P2_60_3000_INTERVAL },
        { SCAN_MODE_OP_P10_30_300, SCAN_MODE_OP_P10_30_300_WINDOW, SCAN_MODE_OP_P10_30_300_INTERVAL },
        { SCAN_MODE_OP_P10_60_600, SCAN_MODE_OP_P10_60_600_WINDOW, SCAN_MODE_OP_P10_60_600_INTERVAL },
        { SCAN_MODE_OP_P25_60_240, SCAN_MODE_OP_P25_60_240_WINDOW, SCAN_MODE_OP_P25_60_240_INTERVAL },
        { SCAN_MODE_OP_P50_100_200, SCAN_MODE_OP_P50_100_200_WINDOW, SCAN_MODE_OP_P50_100_200_INTERVAL },
        { SCAN_MODE_OP_P50_240_480, SCAN_MODE_OP_P50_240_480_WINDOW, SCAN_MODE_OP_P50_240_480_INTERVAL },
        { SCAN_MODE_OP_P50_480_960, SCAN_MODE_OP_P50_480_960_WINDOW, SCAN_MODE_OP_P50_480_960_INTERVAL },
        { SCAN_MODE_OP_P100_240_240, SCAN_MODE_OP_P100_240_240_WINDOW, SCAN_MODE_OP_P100_240_240_INTERVAL },
        { SCAN_MODE_OP_P100_1000_1000, SCAN_MODE_OP_P100_1000_1000_WINDOW, SCAN_MODE_OP_P100_1000_1000_INTERVAL },
    };
    for (size_t i = 0; i < sizeof(expects) / sizeof(expects[0]); i++) {
        uint32_t scannerId = AllocScannerOk();
        EXPECT_EQ(StartScanWithMode(scannerId, DEVD_SCAN_FRAME_TYPE_1, expects[i].mode, 0), NLSTK_ERRCODE_SUCCESS);
        TEST_RunQueueStubSchedule();
        NLSTK_DevdScanner_S *scanner = FindScannerById(scannerId);
        ASSERT_NE(scanner, nullptr) << "mode " << expects[i].mode;
        EXPECT_EQ(scanner->scanSetting.window, expects[i].window) << "mode " << expects[i].mode;
        EXPECT_EQ(scanner->scanSetting.interval, expects[i].interval) << "mode " << expects[i].mode;
    }
}

// 批量上报档（reportDelay > 0）按批量档间隔配置；帧四侧同样生效；
// MONITOR 模式不进入模式配置；非法模式不写入任何配置
TEST_F(DEVD_SCAN_TEST, ScanModeBatchAndFrame4Test)
{
    uint32_t scannerId1 = AllocScannerOk();
    EXPECT_EQ(StartScanWithMode(scannerId1, DEVD_SCAN_FRAME_TYPE_1, SCAN_MODE_LOW_POWER, 500),
        NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    NLSTK_DevdScanner_S *scanner1 = FindScannerById(scannerId1);
    ASSERT_NE(scanner1, nullptr);
    EXPECT_EQ(scanner1->scanSetting.window, SCAN_MODE_BATCH_LOW_POWER_WINDOW);
    EXPECT_EQ(scanner1->scanSetting.interval, SCAN_MODE_BATCH_LOW_POWER_INTERVAL);

    uint32_t scannerId2 = AllocScannerOk();
    EXPECT_EQ(StartScanWithMode(scannerId2, DEVD_SCAN_FRAME_TYPE_4, SCAN_MODE_BALANCED, 0), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    NLSTK_DevdScanner_S *scanner2 = FindScannerById(scannerId2);
    ASSERT_NE(scanner2, nullptr);
    EXPECT_EQ(scanner2->frame4ScanSetting.window, SCAN_MODE_BALANCED_WINDOW);
    EXPECT_EQ(scanner2->frame4ScanSetting.interval, SCAN_MODE_BALANCED_INTERVAL);

    // MONITOR 模式视为停止该扫描：监听由软件过滤承担，不写入模式配置
    uint32_t scannerId3 = AllocScannerOk();
    EXPECT_EQ(StartScanWithMode(scannerId3, DEVD_SCAN_FRAME_TYPE_1, SCAN_MODE_MONITOR, 0), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    NLSTK_DevdScanner_S *scanner3 = FindScannerById(scannerId3);
    ASSERT_NE(scanner3, nullptr);
    EXPECT_EQ(scanner3->scanSetting.interval, UINT16_MAX); // 复位默认值，未写入模式配置

    // 非法模式走默认分支：不写入任何配置
    uint32_t scannerId4 = AllocScannerOk();
    EXPECT_EQ(StartScanWithMode(scannerId4, DEVD_SCAN_FRAME_TYPE_1, SCAN_MODE_INVALID, 0), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    NLSTK_DevdScanner_S *scanner4 = FindScannerById(scannerId4);
    ASSERT_NE(scanner4, nullptr);
    EXPECT_EQ(scanner4->scanSetting.interval, UINT16_MAX);
}

// 帧四时长过期后被清理：已过期项复位、未到期项保留、帧一参数不受牵连，
// 同秒时按纳秒比较两侧均生效
TEST_F(DEVD_SCAN_TEST, Frame4ExpireTest)
{
    uint32_t scannerId1 = AllocScannerOk();
    uint32_t scannerId2 = AllocScannerOk();
    uint32_t scannerId3 = AllocScannerOk();
    EXPECT_EQ(StartScanWithDuration(scannerId1, DEVD_SCAN_FRAME_TYPE_4, 10), NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(StartScanWithDuration(scannerId2, DEVD_SCAN_FRAME_TYPE_4, 30), NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(StartScanWithDuration(scannerId3, DEVD_SCAN_FRAME_TYPE_1, 30), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    struct timespec now = {0};
    ASSERT_EQ(clock_gettime(CLOCK_REALTIME, &now), 0);
    // scannerId1 置为严格过期；scannerId2 同秒但纳秒更大（未到期保留）
    SetScannerExpireTime(scannerId1, true, now.tv_sec - 1, 0);
    SetScannerExpireTime(scannerId2, true, now.tv_sec, now.tv_nsec + 500000000);

    EXPECT_EQ(NLSTK_DevdStopScan(scannerId2), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    NLSTK_DevdScanner_S *scanner1 = FindScannerById(scannerId1);
    ASSERT_NE(scanner1, nullptr);
    EXPECT_FALSE(scanner1->frame4ScanSetting.hasDuration); // 过期清理
    EXPECT_EQ(scanner1->frame4ScanSetting.interval, UINT16_MAX); // 复位默认间隔
    NLSTK_DevdScanner_S *scanner2 = FindScannerById(scannerId2);
    ASSERT_NE(scanner2, nullptr);
    EXPECT_TRUE(scanner2->frame4ScanSetting.hasDuration); // 未到期保留
    NLSTK_DevdScanner_S *scanner3 = FindScannerById(scannerId3);
    ASSERT_NE(scanner3, nullptr);
    EXPECT_TRUE(scanner3->scanSetting.hasDuration); // 帧一参数不受帧四清理牵连
}

// 扫描模块表：多次注册取不同槽位、注销后槽位可复用、满表后注册失败
TEST_F(DEVD_SCAN_TEST, ScanModuleSlotTest)
{
    uint8_t moduleId[DEVD_MAX_MODULE_ID] = {0};
    for (uint8_t i = 0; i + 1 < DEVD_MAX_MODULE_ID; i++) { // SetUp 已占一个槽
        NLSTK_DevdScanCbk_S scanCbk = {0};
        scanCbk.onScanCallback = DevdScanReportCallbackStub;
        EXPECT_EQ(NLSTK_DevdRegScanModule(&moduleId[i], &scanCbk), NLSTK_ERRCODE_SUCCESS);
        EXPECT_LT(moduleId[i], DEVD_MAX_MODULE_ID);
    }
    // 模块表已满：再注册返回无效模块号
    uint8_t extraId = DEVD_MAX_MODULE_ID;
    NLSTK_DevdScanCbk_S scanCbk = {0};
    scanCbk.onScanCallback = DevdScanReportCallbackStub;
    EXPECT_NE(NLSTK_DevdRegScanModule(&extraId, &scanCbk), NLSTK_ERRCODE_SUCCESS);
    // 注销后槽位可复用
    NLSTK_DevdDeregScanModule(moduleId[0]);
    uint8_t reusedId = DEVD_MAX_MODULE_ID;
    EXPECT_EQ(NLSTK_DevdRegScanModule(&reusedId, &scanCbk), NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(reusedId, moduleId[0]);
    for (uint8_t i = 1; i + 1 < DEVD_MAX_MODULE_ID; i++) {
        NLSTK_DevdDeregScanModule(moduleId[i]);
    }
}