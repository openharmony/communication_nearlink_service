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
#include "devd_scan_type.h"  // verify-fix: NLSTK_DevdAdvResult_S
#include "devd_report_parse.h"  // verify-fix: FreeAdvResult decl

#include "sdf_vector.h"
#include "devd_local.h"
#include "devd_scan_type.h"
#include "devd_scan_api.h"
#include "devd_scan_util.h"
#include "devd_scan_stm.h"
#include "devd_scan_filter.h"
#include "devd_scan_manager.h"
#include "nlstk_scan_api.h"
#include "nlstk_devd_def.h"
#include "nlstk_devd_scan_type.h"

using namespace testing;
using namespace testing::ext;

static const uint8_t g_filterAddr[6] = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26};
static const uint8_t g_baseUuidPrefix[14] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00};

static SDF_Traits g_plainTraits = {.dtor = SDF_MemFree};

// 构造解析结果：地址/名称/信号强度/mesh 长度可配，服务数据、uuid、厂商列表逐项添加
static NLSTK_DevdAdvResult_S *BuildResult(const uint8_t *addr, const uint8_t *name, uint8_t nameLen,
    int8_t rssi, uint8_t meshLen)
{
    NLSTK_DevdAdvResult_S *result = (NLSTK_DevdAdvResult_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvResult_S));
    if (result == NULL) {
        return NULL;
    }
    result->serviceDataList = SDF_CreateVector(g_plainTraits);
    result->serviceUuids = SDF_CreateVector(g_plainTraits);
    result->manufacturerDataList = SDF_CreateVector(g_plainTraits);
    if (result->serviceDataList == NULL || result->serviceUuids == NULL || result->manufacturerDataList == NULL) {
        FreeAdvResult(result);
        return NULL;
    }
    result->addr.type = 0;
    (void)memcpy_s(result->addr.addr, SLE_ADDR_LEN, addr, SLE_ADDR_LEN);
    result->rssi = rssi;
    result->frameType = 0;
    result->localName.len = nameLen;
    result->localName.data = const_cast<uint8_t *>(name);
    result->meshInfo.len = meshLen;
    return result;
}

static void AddServiceData(NLSTK_DevdAdvResult_S *result, const uint8_t *uuid, uint8_t uuidIs16, const uint8_t *data,
    uint8_t len)
{
    NLSTK_DevdAdvServiceData_S *entry =
        (NLSTK_DevdAdvServiceData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvServiceData_S) + len);
    ASSERT_NE(entry, nullptr);
    (void)memcpy_s(entry->uuid, SERVICE_UUID_LEN_128, g_baseUuidPrefix, sizeof(g_baseUuidPrefix));
    if (uuidIs16 != 0) {
        entry->uuid[SERVICE_UUID_LEN_128 - 1] = uuid[0]; // 小端：末字节存首字节
        entry->uuid[SERVICE_UUID_LEN_128 - 2] = uuid[1];
    } else {
        (void)memcpy_s(entry->uuid, SERVICE_UUID_LEN_128, uuid, SERVICE_UUID_LEN_128);
    }
    entry->len = len;
    (void)memcpy_s(entry->data, len, data, len);
    SDF_VectorEmplaceBack(result->serviceDataList, entry);
}

static void AddServiceUuid(NLSTK_DevdAdvResult_S *result, const uint8_t *uuid, uint8_t uuidIs16)
{
    NLSTK_DevdAdvServiceUuid_S *entry = (NLSTK_DevdAdvServiceUuid_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvServiceUuid_S));
    ASSERT_NE(entry, nullptr);
    (void)memcpy_s(entry->uuid, SERVICE_UUID_LEN_128, g_baseUuidPrefix, sizeof(g_baseUuidPrefix));
    if (uuidIs16 != 0) {
        entry->uuid[SERVICE_UUID_LEN_128 - 1] = uuid[0];
        entry->uuid[SERVICE_UUID_LEN_128 - 2] = uuid[1];
    } else {
        (void)memcpy_s(entry->uuid, SERVICE_UUID_LEN_128, uuid, SERVICE_UUID_LEN_128);
    }
    SDF_VectorEmplaceBack(result->serviceUuids, entry);
}

static void AddManufacturer(NLSTK_DevdAdvResult_S *result, uint16_t id, const uint8_t *data, uint8_t len)
{
    NLSTK_DevdAdvManufacturerData_S *entry =
        (NLSTK_DevdAdvManufacturerData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvManufacturerData_S) + len);
    ASSERT_NE(entry, nullptr);
    entry->manufacturerId = id;
    entry->len = len;
    (void)memcpy_s(entry->data, len, data, len);
    SDF_VectorEmplaceBack(result->manufacturerDataList, entry);
}

// 构造单过滤器向量：过滤器拷贝到堆上入向量，内存字段仍指向静态缓冲
static SDF_Vector_S *BuildFilterVector(const NLSTK_DevdScanFilter_S *filter)
{
    SDF_Vector_S *vector = SDF_CreateVector(g_plainTraits);
    if (vector == NULL) {
        return NULL;
    }
    NLSTK_DevdScanFilter_S *copy = (NLSTK_DevdScanFilter_S *)SDF_MemZalloc(sizeof(NLSTK_DevdScanFilter_S));
    if (copy == NULL) {
        SDF_DestroyVector(vector);
        return NULL;
    }
    (void)memcpy_s(copy, sizeof(NLSTK_DevdScanFilter_S), filter, sizeof(NLSTK_DevdScanFilter_S));
    if (!SDF_VectorEmplaceBack(vector, copy)) {
        SDF_MemFree(copy);
        SDF_DestroyVector(vector);
        return NULL;
    }
    return vector;
}

static void FreeFilterVector(SDF_Vector_S *vector)
{
    // 向量只持有过滤器本体，内存字段指向静态缓冲不释放
    SDF_DestroyVector(vector);
}

static void FreeResult(NLSTK_DevdAdvResult_S *result)
{
    // 名称与mesh指向静态缓冲，置空后按向量释放
    result->localName.data = NULL;
    result->meshInfo.data = NULL;
    FreeAdvResult(result);
}

class DEVD_FILTER_TEST : public testing::Test {
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
        uint8_t moduleId = DEVD_MAX_MODULE_ID;
        NLSTK_DevdScanCbk_S scanCbk = {0};
        (void)NLSTK_DevdRegScanModule(&moduleId, &scanCbk);
        g_filterModuleId = moduleId;
    }

    void TearDown() override
    {
        NLSTK_DevdDeregScanModule(g_filterModuleId);
        DevdScanDeInit();
        DevdLocalDeviceDeInit();
        TEST_StackScheduleDeInit();
        g_testDevdStartScanFail = 0;
        g_testDevdStopScanFail = 0;
        g_testDevdStartScanSilent = 0;
        g_testDevdStopScanSilent = 0;
        TEST_DevdDliStubReset();
    }

    static uint8_t g_filterModuleId;
};
uint8_t DEVD_FILTER_TEST::g_filterModuleId = DEVD_MAX_MODULE_ID;

// 地址/名称/信号强度/mesh 四类基础规则的开关与命中组合
TEST_F(DEVD_FILTER_TEST, BasicRuleMatrix)
{
    static const uint8_t name[] = {'D', 'e', 'v', 'A'};
    static const uint8_t otherName[] = {'D', 'e', 'v', 'B'};
    static const uint8_t otherAddr[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    struct Case {
        bool hasAddr;
        bool addrMatch;
        uint8_t filterNameLen;
        uint8_t resultNameLen;
        bool nameEqual;
        bool hasRssi;
        int8_t rssi;
        int8_t threshold;
        bool meshReport;
        uint8_t meshLen;
        bool expect;
    } cases[] = {
        {false, false, 0, 0, true, false, 0, 0, false, 0, true},   // 全不开：通配通过
        {true, true, 0, 0, true, false, 0, 0, false, 0, true},     // 地址命中
        {true, false, 0, 0, true, false, 0, 0, false, 0, false},   // 地址不命中
        {false, false, 4, 4, true, false, 0, 0, false, 0, true},   // 名称等长且相同
        {false, false, 4, 4, false, false, 0, 0, false, 0, false}, // 名称不同
        {false, false, 4, 0, false, false, 0, 0, false, 0, false}, // 结果无名称
        {false, false, 0, 0, true, true, -50, -60, false, 0, true},  // 信号达标
        {false, false, 0, 0, true, true, -70, -60, false, 0, false}, // 信号不足
        {false, false, 0, 0, true, false, 0, 0, true, 2, true},      // mesh 有内容
        {false, false, 0, 0, true, false, 0, 0, true, 0, false},     // mesh 无内容
        {true, false, 4, 4, false, true, -70, -60, true, 0, false},  // 多规则之一失败即失败
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        NLSTK_DevdAdvResult_S *result = BuildResult(cases[i].addrMatch ? g_filterAddr : otherAddr, name,
            cases[i].resultNameLen, cases[i].rssi, cases[i].meshLen);
        ASSERT_NE(result, nullptr);
        result->localName.data = cases[i].nameEqual ? const_cast<uint8_t *>(name) :
            const_cast<uint8_t *>(otherName);
        NLSTK_DevdScanFilter_S filter = {0};
        filter.hasAddr = cases[i].hasAddr;
        (void)memcpy_s(filter.addr.addr, SLE_ADDR_LEN, g_filterAddr, SLE_ADDR_LEN);
        filter.name.len = cases[i].filterNameLen;
        filter.name.data = const_cast<uint8_t *>(name);
        filter.hasRssiThreshold = cases[i].hasRssi;
        filter.rssiThreshold = cases[i].threshold;
        filter.meshInfoReport = cases[i].meshReport;
        filter.isNoFilter = false;
        filter.advFrameType = 0;
        SDF_Vector_S *vector = BuildFilterVector(&filter);
        ASSERT_NE(vector, nullptr);
        bool match = DevdIsMatchFilters(result, vector);
        EXPECT_EQ(match, cases[i].expect) << "case " << i;
        FreeFilterVector(vector);
        FreeResult(result);
    }
}

// 服务uuid规则：不启用通配、空列表拒绝、精确/掩码命中与失配、多元素逐一比较
TEST_F(DEVD_FILTER_TEST, ServiceUuidMatrix)
{
    static const uint8_t uuid16[2] = {0x34, 0x12};
    static const uint8_t uuidOther[2] = {0x78, 0x56};
    static const uint8_t uuid128[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    NLSTK_DevdAdvResult_S *result = BuildResult(g_filterAddr, NULL, 0, 0, 0);
    ASSERT_NE(result, nullptr);

    // 不启用：通过
    NLSTK_DevdScanFilter_S filter = {0};
    SDF_Vector_S *vector = BuildFilterVector(&filter);
    EXPECT_TRUE(DevdIsMatchFilters(result, vector));
    FreeFilterVector(vector);

    // 启用但结果无uuid：拒绝（过滤uuid按产品存储序：末字节存首字节）
    filter.hasServiceUuid = true;
    (void)memcpy_s(filter.serviceUuid, SERVICE_UUID_LEN_128, g_baseUuidPrefix, sizeof(g_baseUuidPrefix));
    filter.serviceUuid[SERVICE_UUID_LEN_128 - 1] = uuid16[0];
    filter.serviceUuid[SERVICE_UUID_LEN_128 - 2] = uuid16[1];
    vector = BuildFilterVector(&filter);
    EXPECT_FALSE(DevdIsMatchFilters(result, vector));
    FreeFilterVector(vector);

    // 多元素：第二个命中
    AddServiceUuid(result, uuidOther, 1);
    AddServiceUuid(result, uuid16, 1);
    vector = BuildFilterVector(&filter);
    EXPECT_TRUE(DevdIsMatchFilters(result, vector));
    FreeFilterVector(vector);

    // 掩码命中：仅比较末字节，另一字节故意不同
    (void)memset_s(filter.serviceUuidMask, SERVICE_UUID_LEN_128, 0, SERVICE_UUID_LEN_128);
    filter.hasServiceUuidMask = true;
    filter.serviceUuidMask[SERVICE_UUID_LEN_128 - 1] = 0xFF;
    filter.serviceUuid[SERVICE_UUID_LEN_128 - 2] = 0xAA;
    vector = BuildFilterVector(&filter);
    EXPECT_TRUE(DevdIsMatchFilters(result, vector));
    FreeFilterVector(vector);

    // 掩码失配：两比较字节均参与且其一不同
    filter.serviceUuidMask[SERVICE_UUID_LEN_128 - 2] = 0xFF;
    vector = BuildFilterVector(&filter);
    EXPECT_FALSE(DevdIsMatchFilters(result, vector));
    FreeFilterVector(vector);

    // 128位uuid整串比较
    filter.hasServiceUuidMask = false;
    (void)memcpy_s(filter.serviceUuid, SERVICE_UUID_LEN_128, uuid128, SERVICE_UUID_LEN_128);
    AddServiceUuid(result, uuid128, 0);
    vector = BuildFilterVector(&filter);
    EXPECT_TRUE(DevdIsMatchFilters(result, vector));
    FreeFilterVector(vector);
    FreeResult(result);
}

// 服务数据规则：16位uuid重建为uuid+数据前缀后比较，掩码按位与
TEST_F(DEVD_FILTER_TEST, ServiceDataMatrix)
{
    static const uint8_t dataBody[] = {0xAA, 0xBB, 0xCC};
    static const uint8_t uuid16[2] = {0x5A, 0xFE};
    static const uint8_t uuid128[16] = {0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8,
        0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0};
    NLSTK_DevdAdvResult_S *result = BuildResult(g_filterAddr, NULL, 0, 0, 0);
    ASSERT_NE(result, nullptr);
    AddServiceData(result, uuid16, 1, dataBody, sizeof(dataBody));
    AddServiceData(result, uuid128, 0, dataBody, sizeof(dataBody));

    // 16位：过滤内容 = uuid两字节 + 数据
    NLSTK_DevdScanFilter_S filter = {0};
    static const uint8_t full16[5] = {0x5A, 0xFE, 0xAA, 0xBB, 0xCC};
    filter.serviceData.len = sizeof(full16);
    filter.serviceData.data = const_cast<uint8_t *>(full16);
    SDF_Vector_S *vector = BuildFilterVector(&filter);
    EXPECT_TRUE(DevdIsMatchFilters(result, vector)); // 命中16位项
    FreeFilterVector(vector);

    // 128位：过滤内容 = uuid整串 + 数据
    static uint8_t full128[SERVICE_UUID_LEN_128 + sizeof(dataBody)];
    (void)memcpy_s(full128, SERVICE_UUID_LEN_128, uuid128, SERVICE_UUID_LEN_128);
    (void)memcpy_s(full128 + SERVICE_UUID_LEN_128, sizeof(dataBody), dataBody, sizeof(dataBody));
    filter.serviceData.len = sizeof(full128);
    filter.serviceData.data = full128;
    vector = BuildFilterVector(&filter);
    EXPECT_TRUE(DevdIsMatchFilters(result, vector)); // 命中128位项
    FreeFilterVector(vector);

    // 长于结果数据：拒绝
    static const uint8_t longer[6] = {0x5A, 0xFE, 0xAA, 0xBB, 0xCC, 0xDD};
    filter.serviceData.len = sizeof(longer);
    filter.serviceData.data = const_cast<uint8_t *>(longer);
    vector = BuildFilterVector(&filter);
    EXPECT_FALSE(DevdIsMatchFilters(result, vector));
    FreeFilterVector(vector);

    // 掩码：只比较前两字节
    static const uint8_t mask5[5] = {0xFF, 0xFF, 0x00, 0x00, 0x00};
    static const uint8_t fuzzy[5] = {0x5A, 0xFE, 0x99, 0x88, 0x77};
    filter.serviceData.len = sizeof(fuzzy);
    filter.serviceData.data = const_cast<uint8_t *>(fuzzy);
    filter.serviceDataMask.len = sizeof(mask5);
    filter.serviceDataMask.data = const_cast<uint8_t *>(mask5);
    vector = BuildFilterVector(&filter);
    EXPECT_TRUE(DevdIsMatchFilters(result, vector));
    FreeFilterVector(vector);

    // 结果列表为空：拒绝
    NLSTK_DevdAdvResult_S *emptyResult = BuildResult(g_filterAddr, NULL, 0, 0, 0);
    filter.serviceData.len = sizeof(full16);
    filter.serviceData.data = const_cast<uint8_t *>(full16);
    filter.serviceDataMask.len = 0;
    filter.serviceDataMask.data = NULL;
    vector = BuildFilterVector(&filter);
    EXPECT_FALSE(DevdIsMatchFilters(emptyResult, vector));
    FreeFilterVector(vector);
    FreeResult(emptyResult);
    FreeResult(result);
}

// 厂商数据规则：id 为零通配、id 不匹配跳过、载荷与掩码比较
TEST_F(DEVD_FILTER_TEST, ManufacturerMatrix)
{
    static const uint8_t body[] = {0x11, 0x22};
    NLSTK_DevdAdvResult_S *result = BuildResult(g_filterAddr, NULL, 0, 0, 0);
    ASSERT_NE(result, nullptr);
    AddManufacturer(result, 0x0102, body, sizeof(body));
    AddManufacturer(result, 0x0304, NULL, 0);

    NLSTK_DevdScanFilter_S filter = {0};
    SDF_Vector_S *vector = BuildFilterVector(&filter);
    EXPECT_TRUE(DevdIsMatchFilters(result, vector)); // id为零通配
    FreeFilterVector(vector);

    filter.manufacturerId = 0x0304;
    vector = BuildFilterVector(&filter);
    EXPECT_TRUE(DevdIsMatchFilters(result, vector)); // id命中且无载荷
    FreeFilterVector(vector);

    filter.manufacturerId = 0x9999;
    vector = BuildFilterVector(&filter);
    EXPECT_FALSE(DevdIsMatchFilters(result, vector)); // 无id命中
    FreeFilterVector(vector);

    filter.manufacturerId = 0x0102;
    filter.manufacturerData.len = sizeof(body);
    filter.manufacturerData.data = const_cast<uint8_t *>(body);
    vector = BuildFilterVector(&filter);
    EXPECT_TRUE(DevdIsMatchFilters(result, vector)); // 载荷相同
    FreeFilterVector(vector);

    static const uint8_t wrongBody[] = {0x11, 0x33};
    filter.manufacturerData.data = const_cast<uint8_t *>(wrongBody);
    vector = BuildFilterVector(&filter);
    EXPECT_FALSE(DevdIsMatchFilters(result, vector)); // 载荷不同
    FreeFilterVector(vector);

    static const uint8_t mask2[2] = {0xFF, 0x00};
    filter.manufacturerData.data = const_cast<uint8_t *>(wrongBody);
    filter.manufacturerDataMask.len = sizeof(mask2);
    filter.manufacturerDataMask.data = const_cast<uint8_t *>(mask2);
    vector = BuildFilterVector(&filter);
    EXPECT_TRUE(DevdIsMatchFilters(result, vector)); // 掩码下命中
    FreeFilterVector(vector);

    NLSTK_DevdAdvResult_S *emptyResult = BuildResult(g_filterAddr, NULL, 0, 0, 0);
    filter.manufacturerId = 0x0102;
    filter.manufacturerData.len = 0;
    filter.manufacturerData.data = NULL;
    filter.manufacturerDataMask.len = 0;
    filter.manufacturerDataMask.data = NULL;
    vector = BuildFilterVector(&filter);
    EXPECT_FALSE(DevdIsMatchFilters(emptyResult, vector)); // 空列表
    FreeFilterVector(vector);
    FreeResult(emptyResult);
    FreeResult(result);
}

// 整体匹配：空入参拒绝、通配过滤器直通、帧型不符拒绝、多过滤器任一命中
TEST_F(DEVD_FILTER_TEST, WholeMatchMatrix)
{
    NLSTK_DevdAdvResult_S *result = BuildResult(g_filterAddr, NULL, 0, 0, 0);
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(DevdIsMatchFilters(NULL, NULL));
    EXPECT_FALSE(DevdIsMatchFilters(result, NULL));

    NLSTK_DevdScanFilter_S filters[2] = {{0}, {0}};
    filters[0].advFrameType = 0;
    filters[0].name.len = 1; // 不可能命中（结果无名称）
    filters[0].name.data = const_cast<uint8_t *>(g_baseUuidPrefix);
    filters[1].isNoFilter = true; // 通配直通

    SDF_Vector_S *vector = SDF_CreateVector(g_plainTraits);
    ASSERT_NE(vector, nullptr);
    SDF_VectorEmplaceBack(vector, &filters[0]);
    SDF_VectorEmplaceBack(vector, &filters[1]);
    result->frameType = 0;
    EXPECT_TRUE(DevdIsMatchFilters(result, vector)); // 第二个通配命中
    result->frameType = 1;                           // 帧型与第一个不符
    filters[1].isNoFilter = false;
    filters[1].advFrameType = 0;
    EXPECT_FALSE(DevdIsMatchFilters(result, vector)); // 帧型不符拒绝
    vector->traits.dtor = NULL; // 过滤器在栈上，只清向量
    SDF_DestroyVector(vector);
    FreeResult(result);
}

// 释放与克隆入口的空指针防御
TEST_F(DEVD_FILTER_TEST, FreeAndCloneNullGuards)
{
    DevdFreeDevdScanner(NULL);
    DevdFreeStartScanParam(NULL);
    EXPECT_EQ(DevdCloneFiltersVector(NULL), nullptr);
}

// 过滤器深拷贝：五类内存字段全部/部分设置均可入表，字段拷贝失败则该轮启动参数不生效
TEST_F(DEVD_FILTER_TEST, FilterDeepCopyAndFailPaths)
{
    static uint8_t fieldBuf[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint32_t scannerId = DEVD_INVALID_SCANNER_ID;
    ASSERT_EQ(NLSTK_DevdAllocScannerId(g_filterModuleId, &scannerId), NLSTK_ERRCODE_SUCCESS);

    // 五字段全设：拷贝入表成功，参数生效
    NLSTK_DevdScanFilter_S filter = {0};
    filter.advFrameType = 0;
    filter.name.len = 1;
    filter.name.data = fieldBuf;
    filter.serviceData.len = 2;
    filter.serviceData.data = fieldBuf;
    filter.serviceDataMask.len = 3;
    filter.serviceDataMask.data = fieldBuf;
    filter.manufacturerData.len = 4;
    filter.manufacturerData.data = fieldBuf;
    filter.manufacturerDataMask.len = 5;
    filter.manufacturerDataMask.data = fieldBuf;
    NLSTK_DevdScanSetting_S setting = {0};
    setting.scanMode = SCAN_MODE_BALANCED;
    setting.frameType = DEVD_SCAN_FRAME_TYPE_1;
    ASSERT_EQ(NLSTK_DevdStartScan(scannerId, &setting, &filter, 1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(
        DEVD_STM_M(DevdGetStateMachine(), scanManager)->scanners, 0);
    ASSERT_NE(scanner->filters, nullptr);
    EXPECT_EQ(scanner->filters->size, 1); // 深拷贝入表
    EXPECT_EQ(scanner->scanSetting.window, SCAN_MODE_BALANCED_WINDOW); // 参数已生效
    ASSERT_EQ(NLSTK_DevdStopScan(scannerId), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 部分字段（仅名称）：拷贝成功
    NLSTK_DevdScanFilter_S nameOnly = {0};
    nameOnly.advFrameType = 0;
    nameOnly.name.len = 2;
    nameOnly.name.data = fieldBuf;
    ASSERT_EQ(NLSTK_DevdStartScan(scannerId, &setting, &nameOnly, 1), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ASSERT_EQ(NLSTK_DevdStopScan(scannerId), NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 各字段拷贝失败：参数不入表，扫描参数不生效（窗口保持复位值）
    for (uint16_t failLen = 1; failLen <= 5; failLen++) {
        EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
        EXPECT_CALL(sdfMemMock, SDF_MemZalloc(Eq(failLen)))
            .WillOnce(TEST_SDF_MemZallocFail)
            .RetiresOnSaturation();
        filter.name.len = (failLen == 1) ? 1 : 0;
        filter.name.data = (failLen == 1) ? fieldBuf : NULL;
        filter.serviceData.len = (failLen == 2) ? 2 : 0;
        filter.serviceData.data = (failLen == 2) ? fieldBuf : NULL;
        filter.serviceDataMask.len = (failLen == 3) ? 3 : 0;
        filter.serviceDataMask.data = (failLen == 3) ? fieldBuf : NULL;
        filter.manufacturerData.len = (failLen == 4) ? 4 : 0;
        filter.manufacturerData.data = (failLen == 4) ? fieldBuf : NULL;
        filter.manufacturerDataMask.len = (failLen == 5) ? 5 : 0;
        filter.manufacturerDataMask.data = (failLen == 5) ? fieldBuf : NULL;
        ASSERT_EQ(NLSTK_DevdStartScan(scannerId, &setting, &filter, 1), NLSTK_ERRCODE_SUCCESS);
        TEST_RunQueueStubSchedule();
        scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(
            DEVD_STM_M(DevdGetStateMachine(), scanManager)->scanners, 0);
        EXPECT_EQ(scanner->scanSetting.window, 0) << "failLen " << failLen; // 拷贝失败参数未生效
    }
    NLSTK_DevdRemoveScannerId(scannerId);
}

// 多扫描器参数择优：占空比大者优先，同占空比间隔小者优先（帧一/帧四两侧同规）
TEST_F(DEVD_FILTER_TEST, FetchTargetSettingsMatrix)
{
    struct Case {
        uint16_t win[3];
        uint16_t itv[3];
        size_t expectIdx;
    } cases[] = {
        {{100, 200, 300}, {400, 400, 400}, 2},  // 占空比递增选最后
        {{200, 100, 300}, {400, 400, 400}, 2},  // 最大占空比
        {{100, 100, 200}, {400, 200, 400}, 1},  // 同占空比取间隔小者
        {{100, 100, 100}, {400, 300, 200}, 2},  // 占空比全同选间隔最小
    };
    for (size_t side = 0; side < 2; side++) {
        NLSTK_DevdStateMachine_S *stm = DevdGetStateMachine();
        SDF_Vector_S *scanners = DEVD_STM_M(stm, scanManager)->scanners;
        for (size_t c = 0; c < sizeof(cases) / sizeof(cases[0]); c++) {
            // 每轮同步清空扫描器表重建布景
            while (scanners->size > 0) {
                NLSTK_DevdScanner_S *s = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(scanners, 0);
                DevdDestroyScanner(s->scannerId);
            }
            for (size_t i = 0; i < 3; i++) {
                uint32_t id = DEVD_INVALID_SCANNER_ID;
                ASSERT_EQ(NLSTK_DevdAllocScannerId(g_filterModuleId, &id), NLSTK_ERRCODE_SUCCESS);
                NLSTK_DevdScanner_S *s = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(scanners, i);
                NLSTK_DevdScanSettingInner_S *inner = side == 0 ? &s->scanSetting : &s->frame4ScanSetting;
                inner->window = cases[c].win[i];
                inner->interval = cases[c].itv[i];
            }
            NLSTK_DevdScanSettingInner_S *target =
                side == 0 ? FetchFrame1TargetScanSettings(scanners) : FetchFrame4TargetScanSettings(scanners);
            NLSTK_DevdScanner_S *expect = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(scanners, cases[c].expectIdx);
            EXPECT_EQ(target, side == 0 ? &expect->scanSetting : &expect->frame4ScanSetting)
                << "side " << side << " case " << c;
            while (scanners->size > 0) {
                NLSTK_DevdScanner_S *s = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(scanners, 0);
                DevdDestroyScanner(s->scannerId);
            }
        }
    }
}
