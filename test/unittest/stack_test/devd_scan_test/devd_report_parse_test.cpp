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

#include "sdf_vector.h"
#include "devd_local.h"
#include "devd_scan_type.h"
#include "devd_scan_api.h"
#include "devd_report_parse.h"
#include "devd_cbk.h"
#include "nlstk_scan_api.h"
#include "nlstk_devd_def.h"
#include "dli_event_struct.h"

using namespace testing;
using namespace testing::ext;

#define TEST_EVT_SCANNABLE 0x02   // 可扫描非响应
#define TEST_EVT_SCAN_RSP 0x08    // 扫描响应
#define TEST_FRAME_1 0x00
#define TEST_FRAME_4 0x01

static const uint8_t g_devdAddr[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

static NLSTK_DevdAdvReportInfo_S *BuildReport(const uint8_t *addr, const uint8_t *data, uint16_t len,
    uint8_t eventType, uint8_t frameType, int8_t rssi)
{
    NLSTK_DevdAdvReportInfo_S *report =
        (NLSTK_DevdAdvReportInfo_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvReportInfo_S) + len);
    if (report != NULL) {
        (void)memcpy_s(report->addr, SLE_ADDR_LEN, addr, SLE_ADDR_LEN);
        report->rssi = rssi;
        report->dataLength = len;
        report->extendParams.primFrameType = frameType;
        report->extendParams.eventType = eventType;
        if (len != 0) {
            (void)memcpy_s(report->data, len, data, len);
        }
    }
    return report;
}

// 直接以芯片上报身份喂数据，返回解析结果（调用方负责释放）
static NLSTK_DevdAdvResult_S *Feed(const uint8_t *addr, const uint8_t *data, uint16_t len,
    uint8_t eventType, uint8_t frameType, int8_t rssi = 10)
{
    NLSTK_DevdAdvReportInfo_S *report = BuildReport(addr, data, len, eventType, frameType, rssi);
    if (report == NULL) {
        return NULL;
    }
    NLSTK_DevdAdvResult_S *result = DevdAdvDataReport(report);
    SDF_MemFree(report);
    return result;
}

static void FreeResult(NLSTK_DevdAdvResult_S *result)
{
    FreeAdvResult(result);
}

class DEVD_REPORT_TEST : public testing::Test {
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
    }

    void TearDown() override
    {
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

// 新格式广播数据全字段解析：各类型逐一识别，重复项去重，后写覆盖先写
TEST_F(DEVD_REPORT_TEST, NewFormatFieldMatrix)
{
    static const uint8_t data[] = {
        0x01, 0x01, 0x03,                                    // 发现级别
        0x03, 0x03, 0x5A, 0xFE, 0x77,                        // 16位服务数据 uuid 0xFE5A
        0x03, 0x03, 0x5A, 0xFE, 0x88,                        // 同uuid服务数据：去重
        0x05, 0x04, 0x34, 0x12, 0x78, 0x56,                  // 16位服务uuid列表 0x1234 0x5678
        0x05, 0x02, 0x34, 0x12,                              // 重复uuid：去重
        0x0A, 0x03, 'T', 'm', 'p',                           // 简称
        0x0B, 0x04, 'D', 'e', 'v', 'A',                      // 全称覆盖简称
        0x0C, 0x01, 0x5A,                                    // 发射功率
        0x10, 0x02, 0xA1, 0xB1,                              // mesh 信息
        0x10, 0x02, 0xC1, 0xD1,                              // mesh 后写覆盖
        0xFF, 0x03, 0x11, 0x22, 0x99,                        // 厂商数据 id 0x2211
        0xFF, 0x03, 0x11, 0x22, 0x88,                        // 同id厂商数据：去重
        0x77, 0x02, 0x01, 0x02,                              // 未知类型
        0x0B, 0x00,                                          // 零长度项
    };
    NLSTK_DevdAdvResult_S *result = Feed(g_devdAddr, data, sizeof(data), TEST_EVT_SCANNABLE, TEST_FRAME_4);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->discoveryLevel, 0x03);
    EXPECT_EQ(result->txPower, 0x5A);
    EXPECT_EQ(result->localName.len, 4); // 全称生效
    EXPECT_EQ(result->localName.data[0], 'D');
    EXPECT_EQ(result->meshInfo.len, 2);
    EXPECT_EQ(result->meshInfo.data[0], 0xC1); // 后写覆盖
    ASSERT_NE(result->serviceDataList, nullptr);
    EXPECT_EQ(result->serviceDataList->size, 1); // 同uuid去重
    ASSERT_NE(result->serviceUuids, nullptr);
    EXPECT_EQ(result->serviceUuids->size, 2); // 重复uuid去重
    ASSERT_NE(result->manufacturerDataList, nullptr);
    EXPECT_EQ(result->manufacturerDataList->size, 1); // 同id去重
    EXPECT_EQ(result->rssi, 10);
    EXPECT_EQ(result->advData.len, sizeof(data));
    FreeResult(result);
}

// 铅笔特征数据走旧格式解析：旧类型号识别 128 位 uuid 与名称，仅含类型字节的短项跳过
TEST_F(DEVD_REPORT_TEST, OldFormatPencilData)
{
    static const uint8_t uuid128[16] = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
        0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12};
    static const uint8_t name[] = {'H', 'U', 'A', 'W', 'E', 'I', ' ', 'P', 'e', 'n', 'c', 'i', 'l', '9', '8', '7', '6'};
    static const uint8_t filler[12] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B};
    static const uint8_t data[62] = {
        0x03, 0x19, 0xC7, 0x03, 0x0F, 0x16, 0xEE, 0xFD, // 铅笔特征1
        0x0F, 0x16, 0xEE, 0xFD,                         // 旧服务数据项头（值与特征字节拼成14字节）
    };
    // 旧格式按 长度-类型-值 排布，逐段拼装避免特征字节与后续项错位
    uint8_t payload[62] = {0};
    uint16_t pos = 0;
    (void)memcpy_s(payload + pos, 10, data, 10);
    pos = 10;
    (void)memcpy_s(payload + pos, 12, filler, 12); // 补齐上项的14字节值
    pos += 12;
    payload[pos++] = 0x01; // 长度1仅类型字节：短项跳过
    payload[pos++] = 0x0A;
    payload[pos++] = 18; // 名称项：长度18 = 类型1 + 名17
    payload[pos++] = 0x09;
    (void)memcpy_s(payload + pos, 17, name, 17);
    pos += 17;
    payload[pos++] = 17; // 128位uuid项：长度17 = 类型1 + uuid16
    payload[pos++] = 0x06;
    (void)memcpy_s(payload + pos, 16, uuid128, 16);
    pos += 16;
    ASSERT_EQ(pos, 62);

    NLSTK_DevdAdvResult_S *result = Feed(g_devdAddr, payload, pos, TEST_EVT_SCAN_RSP, TEST_FRAME_1);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isPencil); // 特征命中走旧格式
    EXPECT_EQ(result->localName.len, 17);
    EXPECT_EQ(result->localName.data[0], 'H');
    ASSERT_NE(result->serviceUuids, nullptr);
    ASSERT_EQ(result->serviceUuids->size, 1);
    NLSTK_DevdAdvServiceUuid_S *uuid = (NLSTK_DevdAdvServiceUuid_S *)SDF_VectorElementAt(result->serviceUuids, 0);
    EXPECT_EQ(memcmp(uuid->uuid, uuid128, 16), 0); // 128位uuid整串保留
    FreeAdvResult(result);
}

// dis 服务数据内嵌设备信息：名称被提前解析，空数据与非法长度不崩溃
TEST_F(DEVD_REPORT_TEST, DisServiceDataParse)
{
    static const uint8_t dataA[] = {
        0x03, 0x08, 0x06, 0x09,        // 16位服务数据 uuid 0x0906（dis）
        0x05, 0x03, 'P', 'e', 'n',     // 内嵌类型5无效，跳过3字节
        0x06, 0x03, 'D', 'i', 's',     // 内嵌名称
    };
    NLSTK_DevdAdvResult_S *result = Feed(g_devdAddr, dataA, sizeof(dataA), TEST_EVT_SCANNABLE, TEST_FRAME_4);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->localName.len, 3); // dis 名称先于常规名称解析
    EXPECT_EQ(result->localName.data[0], 'D');
    FreeResult(result);

    // dis 服务数据负载为空：仅告警不解析
    static const uint8_t dataB[] = {0x03, 0x02, 0x06, 0x09};
    result = Feed(g_devdAddr, dataB, sizeof(dataB), TEST_EVT_SCANNABLE, TEST_FRAME_4);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->localName.len, 0);
    FreeResult(result);

    // 内嵌长度越界：解析中止不越界读
    static const uint8_t dataC[] = {0x03, 0x06, 0x06, 0x09, 0x06, 0x7F, 0x41, 0x42};
    result = Feed(g_devdAddr, dataC, sizeof(dataC), TEST_EVT_SCANNABLE, TEST_FRAME_4);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->localName.len, 0);
    FreeResult(result);
}

// 帧一缓存：同地址新数据覆盖旧数据，超限逐出最旧缓存，未缓存地址的响应被丢弃
TEST_F(DEVD_REPORT_TEST, CacheReplaceAndEvict)
{
    static const uint8_t payload[] = {0x0B, 0x03, 'A', 'B', 'C'};
    uint8_t addr[6] = {0};
    (void)memcpy_s(addr, 6, g_devdAddr, 6);

    // 同地址两段广播：后段覆盖前段
    addr[5] = 0x01;
    EXPECT_EQ(Feed(addr, payload, sizeof(payload), TEST_EVT_SCANNABLE, TEST_FRAME_1), nullptr);
    EXPECT_EQ(Feed(addr, payload + 1, 3, TEST_EVT_SCANNABLE, TEST_FRAME_1), nullptr);
    NLSTK_DevdAdvResult_S *result = Feed(addr, payload, 2, TEST_EVT_SCAN_RSP, TEST_FRAME_1);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->advData.len, 2); // 命中同地址缓存
    FreeResult(result);

    // 8个地址填满缓存（上限7），最早地址被逐出：其响应无法配对
    for (uint8_t i = 0x10; i < 0x18; i++) {
        addr[5] = i;
        EXPECT_EQ(Feed(addr, payload, sizeof(payload), TEST_EVT_SCANNABLE, TEST_FRAME_1), nullptr);
    }
    addr[5] = 0x10;
    EXPECT_EQ(Feed(addr, payload, 2, TEST_EVT_SCAN_RSP, TEST_FRAME_1), nullptr); // 已被逐出
    addr[5] = 0x17;
    result = Feed(addr, payload, 2, TEST_EVT_SCAN_RSP, TEST_FRAME_1);
    ASSERT_NE(result, nullptr); // 最新的仍在
    FreeResult(result);
}

// 帧一两段式：广播段只入缓存，响应段拼接上报；空响应段沿用已缓存数据
TEST_F(DEVD_REPORT_TEST, Frame1TwoStageReport)
{
    static const uint8_t advPart[] = {0x0B, 0x03, 'A', 'B', 'C'};
    static const uint8_t rspPart[] = {0x0C, 0x01, 0x09};

    EXPECT_EQ(Feed(g_devdAddr, advPart, sizeof(advPart), TEST_EVT_SCANNABLE, TEST_FRAME_1), nullptr);
    NLSTK_DevdAdvResult_S *result = Feed(g_devdAddr, rspPart, sizeof(rspPart), TEST_EVT_SCAN_RSP, TEST_FRAME_1);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->advData.len, sizeof(advPart) + sizeof(rspPart)); // 拼接
    EXPECT_EQ(result->txPower, 0x09);
    FreeResult(result);

    // 响应段无缓存地址：直接丢弃
    uint8_t otherAddr[6] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};
    EXPECT_EQ(Feed(otherAddr, rspPart, sizeof(rspPart), TEST_EVT_SCAN_RSP, TEST_FRAME_1), nullptr);

    // 空响应段：沿用缓存直接上报
    EXPECT_EQ(Feed(g_devdAddr, advPart, sizeof(advPart), TEST_EVT_SCANNABLE, TEST_FRAME_1), nullptr);
    result = Feed(g_devdAddr, advPart, 0, TEST_EVT_SCAN_RSP, TEST_FRAME_1);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->advData.len, sizeof(advPart));
    FreeResult(result);

    // 空广播段（无缓存）：丢弃
    result = Feed(otherAddr, advPart, 0, TEST_EVT_SCANNABLE, TEST_FRAME_1);
    EXPECT_EQ(result, nullptr);
}

// 帧四一段式：空数据不上报，短于铅笔特征的数据按新格式解析且不误判铅笔
TEST_F(DEVD_REPORT_TEST, Frame4OneShotReport)
{
    static const uint8_t payload[] = {0x0B, 0x03, 'X', 'Y', 'Z'};
    EXPECT_EQ(Feed(g_devdAddr, payload, 0, TEST_EVT_SCANNABLE, TEST_FRAME_4), nullptr);

    // 短于铅笔特征：KMP 因数据过短直接不匹配
    static const uint8_t shortData[] = {0x03, 0x19, 0xC7};
    NLSTK_DevdAdvResult_S *result = Feed(g_devdAddr, shortData, sizeof(shortData), TEST_EVT_SCANNABLE, TEST_FRAME_4);
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->isPencil);
    FreeResult(result);

    result = Feed(g_devdAddr, payload, sizeof(payload), TEST_EVT_SCANNABLE, TEST_FRAME_4);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->localName.len, 3);
    FreeResult(result);
}

// 特征匹配表内存申请失败：特征匹配退化为不命中，按新格式解析不崩溃
TEST_F(DEVD_REPORT_TEST, LpsAllocFailFallsBackToNewFormat)
{
    static const uint8_t pencil2[] = {
        0x12, 0x09, 'H', 'U', 'A', 'W', 'E', 'I', ' ', 'M', '-', 'P', 'e', 'n', 'c', 'i', 'l', ' ', '3'};
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc(Eq(sizeof(pencil2) * sizeof(size_t))))
        .WillOnce(TEST_SDF_MemZallocFail)
        .RetiresOnSaturation();

    NLSTK_DevdAdvResult_S *result = Feed(g_devdAddr, pencil2, sizeof(pencil2), TEST_EVT_SCANNABLE, TEST_FRAME_4);
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->isPencil); // 匹配表申请失败视为不命中
    EXPECT_EQ(result->localName.len, 0); // 按新格式解析，类型0x12未知
    FreeResult(result);
}

// 解析结果向量的内存申请失败：上报整体失败返回空
TEST_F(DEVD_REPORT_TEST, CreateAdvResultAllocFail)
{
    static const uint8_t payload[] = {0x0B, 0x03, 'A', 'B', 'C'};
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc(Eq(sizeof(NLSTK_DevdAdvResult_S))))
        .WillOnce(TEST_SDF_MemZallocFail)
        .RetiresOnSaturation();
    EXPECT_EQ(Feed(g_devdAddr, payload, sizeof(payload), TEST_EVT_SCANNABLE, TEST_FRAME_4), nullptr);

    // 结果本体成功而首个向量本体失败：同样整体失败
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc(Eq(sizeof(SDF_Vector_S))))
        .WillOnce(TEST_SDF_MemZallocFail)
        .RetiresOnSaturation();
    EXPECT_EQ(Feed(g_devdAddr, payload, sizeof(payload), TEST_EVT_SCANNABLE, TEST_FRAME_4), nullptr);

    // 首向量成功，其元素缓冲失败：整体失败
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc(Eq(sizeof(void *) * 8)))
        .WillOnce(TEST_SDF_MemZallocFail)
        .RetiresOnSaturation();
    EXPECT_EQ(Feed(g_devdAddr, payload, sizeof(payload), TEST_EVT_SCANNABLE, TEST_FRAME_4), nullptr);
}

// 原始广播数据副本内存申请失败：解析结果不产出
TEST_F(DEVD_REPORT_TEST, AdvDataCopyAllocFail)
{
    static const uint8_t payload[] = {0x0B, 0x03, 'A', 'B', 'C', 0x00, 0x00, 0x00, 0x00, 0x00};
    const uint16_t len = sizeof(payload);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc(Eq(len)))
        .WillOnce(TEST_SDF_MemZalloc)     // 帧四缓存数据副本
        .WillOnce(TEST_SDF_MemZallocFail) // 结果内广播数据副本
        .RetiresOnSaturation();
    EXPECT_EQ(Feed(g_devdAddr, payload, len, TEST_EVT_SCANNABLE, TEST_FRAME_4), nullptr);
}

// 按地址清除缓存：已缓存地址被移除，未缓存地址无副作用
TEST_F(DEVD_REPORT_TEST, ClearAdvCacheByAddr)
{
    static const uint8_t payload[] = {0x0B, 0x03, 'A', 'B', 'C'};
    uint8_t addr[6] = {0};
    (void)memcpy_s(addr, 6, g_devdAddr, 6);
    EXPECT_EQ(Feed(addr, payload, sizeof(payload), TEST_EVT_SCANNABLE, TEST_FRAME_1), nullptr);

    uint8_t emptyAddr[6] = {0};
    ClearAdvDataCache((SLE_Addr_S *)emptyAddr); // 未缓存地址
    NLSTK_DevdAdvResult_S *result = Feed(addr, payload, 2, TEST_EVT_SCAN_RSP, TEST_FRAME_1);
    ASSERT_NE(result, nullptr); // 缓存未被误删
    FreeResult(result);

    EXPECT_EQ(Feed(addr, payload, sizeof(payload), TEST_EVT_SCANNABLE, TEST_FRAME_1), nullptr);
    ClearAdvDataCache((SLE_Addr_S *)addr);
    EXPECT_EQ(Feed(addr, payload, 2, TEST_EVT_SCAN_RSP, TEST_FRAME_1), nullptr); // 已被清除
}

// 解析结果的组合释放：空指针直接返回，带各字段结果可安全释放
TEST_F(DEVD_REPORT_TEST, FreeAdvResultCombo)
{
    FreeAdvResult(NULL); // 空指针防御

    static const uint8_t payload[] = {0x0B, 0x03, 'A', 'B', 'C', 0x10, 0x02, 0x01, 0x02};
    NLSTK_DevdAdvResult_S *result = Feed(g_devdAddr, payload, sizeof(payload), TEST_EVT_SCANNABLE, TEST_FRAME_4);
    ASSERT_NE(result, nullptr);
    EXPECT_NE(result->localName.data, nullptr);
    EXPECT_NE(result->meshInfo.data, nullptr); // 本地名与mesh并存
    FreeResult(result); // 双字段组合释放不泄漏不崩溃
}

// 芯片上报事件直调：非法载荷被拒绝；合法载荷拆解事件位、按界内外调制参数填充并回调
static int g_reportCaptureCnt = 0;
static int8_t g_reportCaptureRssi = 0;
static uint8_t g_reportCaptureEvt = 0;
static uint8_t g_reportCaptureMod = 0;
static uint8_t g_reportCaptureDStatus = 0;
static void CaptureReportCbk(NLSTK_DevdAdvReportInfo_S *report)
{
    g_reportCaptureCnt++;
    g_reportCaptureRssi = report->rssi;
    g_reportCaptureEvt = report->extendParams.eventType;
    g_reportCaptureMod = report->extendParams.secondMod;
    g_reportCaptureDStatus = report->extendParams.dataStatus;
}

TEST_F(DEVD_REPORT_TEST, ScanReportCbkDirectMatrix)
{
    SCAN_EVENT_CBK = NULL;
    SCAN_REPORT_CBK = CaptureReportCbk;
    g_reportCaptureCnt = 0;

    // 空事件参数与尺寸不足：直接拒绝
    DevdSleScanReportCbk(NULL, 0, NULL);
    struct DLI_ExecuteCmdRetParam badRes = {0};
    DevdSleScanReportCbk(NULL, 0, &badRes);
    EXPECT_EQ(g_reportCaptureCnt, 0);

    // 合法事件：事件位拆解 + 调制参数界内查表
    DLI_AdvReportEvt evt = {0};
    evt.eventType = 0x2A; // bit1 可扫描 + bit3 扫描响应 + bit5:0 数据状态 0b01
    evt.rssi = (uint8_t)-60;
    evt.secondMcs = 1;    // 界内：查调制表
    evt.dataLength = 0;
    (void)memcpy_s(evt.addr, SLE_ADDR_LEN, g_devdAddr, SLE_ADDR_LEN);
    struct DLI_ExecuteCmdRetParam res = {0};
    res.eventParameter = &evt;
    res.size = sizeof(DLI_AdvReportEvt) + evt.dataLength;
    DevdSleScanReportCbk(NULL, 0, &res);
    EXPECT_EQ(g_reportCaptureCnt, 1);
    EXPECT_EQ(g_reportCaptureRssi, -60);
    EXPECT_EQ(g_reportCaptureEvt, 0x0A); // 低5位事件类型，高3位数据状态被拆走
    EXPECT_NE(g_reportCaptureMod, 0);           // 界内查表得到调制方式
    EXPECT_EQ(g_reportCaptureDStatus, 0x01);    // bit5:6 数据状态

    // 调制参数越界：不查表，保持默认
    evt.secondMcs = 0xFF;
    g_reportCaptureCnt = 0;
    DevdSleScanReportCbk(NULL, 0, &res);
    EXPECT_EQ(g_reportCaptureCnt, 1);
    EXPECT_EQ(g_reportCaptureMod, 0);
    SCAN_REPORT_CBK = NULL;
}
