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

NLSTK_Errcode_E NLSTK_DevdSleSetScanFilter(NLSTK_DevdSleScanFilter_S *sleScanFilter)
{
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