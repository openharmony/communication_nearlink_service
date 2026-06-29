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
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"

#include "sdf_addr.h"
#include "sdf_mem.h"
#include "nlstk_log.h"

#include "cm_def.h"
#include "dtap_tcid.h"
#include "ssap_link.h"
#include "ssapc_cache.h"
#include "ssap_manager.h"
#include "ssaps_service.h"
#include "nlstk_ssap_app_link.h"

#include "hid_type.h"
#include "hid_utils.h"
#include "hid_ssap.h"
#include "hid_stm.h"
#include "hid_client.h"
#include "hid_client_init.h"
#include "hid_client_api.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
static uint16_t g_lcid = 1;

static uint8_t g_serviceUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0B};
static uint8_t g_reportMapUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x39};
static uint8_t g_workStateUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x3A};
static uint8_t g_reportIndexUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x3B};
static uint8_t g_inputReportUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x3C};
static uint8_t g_outputReportUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x3D};
static uint8_t g_featureReportUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x3E};

static uint8_t g_serviceUuidPen[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x06};
static uint8_t g_reportMapUuidPen[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x39, 0x10};
static uint8_t g_workStateUuidPen[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x10};
static uint8_t g_reportIndexUuidPen[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x10};
static uint8_t g_inputReportUuidPen[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x10};
static uint8_t g_outputReportUuidPen[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3D, 0x10};
static uint8_t g_featureReportUuidPen[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x10};

static void STUB_CacheHidService();

class IT_HID_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;

    static void STUB_ResetData();

    static void STUB_NotifyPropertyCbk(SLE_Addr_S *addr, HidPropertyType_E type, void *value);
    static void STUB_ReadPropertyCbk(SLE_Addr_S *addr, HidPropertyType_E type, void *value, NLSTK_Errcode_E ret);
    static void STUB_WritePropertyCbk(SLE_Addr_S *addr, HidPropertyType_E type, NLSTK_Errcode_E ret);
    static void STUB_ConnectStateChangeCbk(SLE_Addr_S *addr, HidConnectState_E state, HidConnectState_E preState,
                                           NLSTK_Errcode_E ret);

    static SLE_Addr_S g_recvAddr;
    static bool g_recvCbk;
    static HidConnectState_E g_state;
protected:
    void SetUp() override
    {
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskQueueStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerAdd).WillRepeatedly(TEST_ScheduleTimerAddStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerDel).WillRepeatedly(TEST_ScheduleTimerDelStub);

        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);

        EXPECT_CALL(dtapMock, DTAP_RegisterDataRecvCb).WillRepeatedly(TEST_DTAP_RegisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_UnregisterDataRecvCb).WillRepeatedly(TEST_DTAP_UnregisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_DataSend).WillRepeatedly(TEST_DTAP_DataSend);

        // 步骤2：初始化SSAP模块
        SSAP_LinkInit();
        SSAP_Init();

        // 步骤3：使能HID模块
        HidEnable();
        STUB_ResetData();
    }

    void TearDown() override
    {
        // 步骤4：去使能HID模块
        HidDisable();

        // 步骤5：去初始化SSAP模块
        SSAP_DeInit();
        SSAP_LinkDeInit();

        // 步骤6：清理桩函数
        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
    }

};

bool IT_HID_TEST::g_recvCbk = false;
HidConnectState_E IT_HID_TEST::g_state = HID_DISCONNECTED;

void IT_HID_TEST::STUB_ResetData()
{
    g_recvCbk = false;
    g_state = HID_DISCONNECTED;
}

void IT_HID_TEST::STUB_NotifyPropertyCbk(SLE_Addr_S *addr, HidPropertyType_E type, void *value)
{
    g_recvCbk = true;
}

void IT_HID_TEST::STUB_ReadPropertyCbk(SLE_Addr_S *addr, HidPropertyType_E type, void *value, NLSTK_Errcode_E ret)
{
    g_recvCbk = true;
}

void IT_HID_TEST::STUB_WritePropertyCbk(SLE_Addr_S *addr, HidPropertyType_E type, NLSTK_Errcode_E ret)
{
    g_recvCbk = true;
}

void IT_HID_TEST::STUB_ConnectStateChangeCbk(SLE_Addr_S *addr, HidConnectState_E state, HidConnectState_E preState,
                                             NLSTK_Errcode_E ret)
{
    g_recvCbk = true;
    g_state = state;
}

static void STUB_CacheHidService()
{
    SsapCacheServInfo_S serv = {0};
    serv.handle = 0x10;
    serv.endHandle = 0x18;
    serv.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serv.uuid, sizeof(NLSTK_SsapUuid_S), &g_serviceUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCacheServ(&g_addr, &serv);

    NLSTK_SsapPrty_S reportMap = {0};
    reportMap.handle = 0x11;
    memcpy_s(&reportMap.uuid, sizeof(NLSTK_SsapUuid_S), &g_reportMapUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &reportMap);

    NLSTK_SsapPrty_S workState = {0};
    workState.handle = 0x12;
    memcpy_s(&workState.uuid, sizeof(NLSTK_SsapUuid_S), &g_workStateUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &workState);

    NLSTK_SsapPrty_S reportIndex1 = {0};
    reportIndex1.handle = 0x13;
    memcpy_s(&reportIndex1.uuid, sizeof(NLSTK_SsapUuid_S), &g_reportIndexUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &reportIndex1);

    NLSTK_SsapPrty_S reportIndex2 = {0};
    reportIndex2.handle = 0x14;
    memcpy_s(&reportIndex2.uuid, sizeof(NLSTK_SsapUuid_S), &g_reportIndexUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &reportIndex2);

    NLSTK_SsapPrty_S reportIndex3 = {0};
    reportIndex3.handle = 0x15;
    memcpy_s(&reportIndex3.uuid, sizeof(NLSTK_SsapUuid_S), &g_reportIndexUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &reportIndex3);

    NLSTK_SsapPrty_S inputReport = {0};
    inputReport.handle = 0x16;
    memcpy_s(&inputReport.uuid, sizeof(NLSTK_SsapUuid_S), &g_inputReportUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &inputReport);

    NLSTK_SsapPrty_S outputReport = {0};
    outputReport.handle = 0x17;
    memcpy_s(&outputReport.uuid, sizeof(NLSTK_SsapUuid_S), &g_outputReportUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &outputReport);

    NLSTK_SsapPrty_S featureReport = {0};
    featureReport.handle = 0x18;
    memcpy_s(&featureReport.uuid, sizeof(NLSTK_SsapUuid_S), &g_featureReportUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &featureReport);

    SsapcCacheServMemberDiscFinish(&g_addr, FIND_STRUCTURE_TYPE_PROPERTY, 0x18);
    SsapcCacheServDiscFinish(&g_addr);
}

static void STUB_CacheHidServicePen()
{
    SsapCacheServInfo_S serv = {0};
    serv.handle = 0x10;
    serv.endHandle = 0x18;
    serv.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serv.uuid, sizeof(NLSTK_SsapUuid_S), &g_serviceUuidPen, sizeof(NLSTK_SsapUuid_S));
    SsapcCacheServ(&g_addr, &serv);

    NLSTK_SsapPrty_S reportMap = {0};
    reportMap.handle = 0x11;
    memcpy_s(&reportMap.uuid, sizeof(NLSTK_SsapUuid_S), &g_reportMapUuidPen, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &reportMap);

    NLSTK_SsapPrty_S workState = {0};
    workState.handle = 0x12;
    memcpy_s(&workState.uuid, sizeof(NLSTK_SsapUuid_S), &g_workStateUuidPen, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &workState);

    NLSTK_SsapPrty_S reportIndex1 = {0};
    reportIndex1.handle = 0x13;
    memcpy_s(&reportIndex1.uuid, sizeof(NLSTK_SsapUuid_S), &g_reportIndexUuidPen, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &reportIndex1);

    NLSTK_SsapPrty_S reportIndex2 = {0};
    reportIndex2.handle = 0x14;
    memcpy_s(&reportIndex2.uuid, sizeof(NLSTK_SsapUuid_S), &g_reportIndexUuidPen, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &reportIndex2);

    NLSTK_SsapPrty_S reportIndex3 = {0};
    reportIndex3.handle = 0x15;
    memcpy_s(&reportIndex3.uuid, sizeof(NLSTK_SsapUuid_S), &g_reportIndexUuidPen, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &reportIndex3);

    NLSTK_SsapPrty_S inputReport = {0};
    inputReport.handle = 0x16;
    memcpy_s(&inputReport.uuid, sizeof(NLSTK_SsapUuid_S), &g_inputReportUuidPen, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &inputReport);

    NLSTK_SsapPrty_S outputReport = {0};
    outputReport.handle = 0x17;
    memcpy_s(&outputReport.uuid, sizeof(NLSTK_SsapUuid_S), &g_outputReportUuidPen, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &outputReport);

    NLSTK_SsapPrty_S featureReport = {0};
    featureReport.handle = 0x18;
    memcpy_s(&featureReport.uuid, sizeof(NLSTK_SsapUuid_S), &g_featureReportUuidPen, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &featureReport);

    SsapcCacheServMemberDiscFinish(&g_addr, FIND_STRUCTURE_TYPE_PROPERTY, 0x18);
    SsapcCacheServDiscFinish(&g_addr);
}

/**
 * @test HID_TEST_001
 * @brief 此测试用例用于验证HID Profile连接流程，对端设备UUID后两字节为大端序。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用HidRegClientCbk注册回调；
 * 2. 调用HidConnect开启连接；
 * 3. 模拟链路建立成功；
 * 4. 模拟收到连接流程中的SSAP响应报文；
 * 5. 校验是否收到连接状态的回调已连接；
 * 6. 调用NLSTK_McpVolumeDisconnect断连；
 * 7. 模拟链路断连销毁；
 * 8. 校验是否收到连接状态的回调已断连；
 *
 * @pre
 * - SSAP模块已初始化。
 * - HID模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_HID_TEST, HID_TEST_001)
{
    // 注册回调
    HidClientCallBack_S clientCallbacks = {0};
    clientCallbacks.notifyPropertyCbk = &STUB_NotifyPropertyCbk;
    clientCallbacks.readPropertyCbk = &STUB_ReadPropertyCbk;
    clientCallbacks.writePropertyCbk = &STUB_WritePropertyCbk;
    clientCallbacks.connectStateChangeCbk = &STUB_ConnectStateChangeCbk;

    NLSTK_Errcode_E ret = HidRegClientCbk(&clientCallbacks);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheHidService();

    // 调用连接
    ret = HidConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_CONNECTING);
    STUB_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 模拟收到连接流程的SSAP报文
    uint8_t desc[] = {0x09, 0x03, 0x01};
    uint8_t index1[] = {0x09, 0x03, 0x01, 0x01, 0x16, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t index2[] = {0x09, 0x03, 0x01, 0x02, 0x17, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t index3[] = {0x09, 0x03, 0x01, 0x03, 0x18, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, desc, sizeof(desc));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, index1, sizeof(index1));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, index2, sizeof(index2));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, index3, sizeof(index3));
    TEST_RunQueueStubSchedule();
    uint8_t input[] = {0x09, 0x03, 0x01, 0x02, 0x03, 0x04};
    uint8_t output[] = {0x09, 0x03, 0x04, 0x03, 0x02, 0x01, 0x00};
    uint8_t feature[] = {0x09, 0x03, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, input, sizeof(input));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, output, sizeof(output));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, feature, sizeof(feature));
    TEST_RunQueueStubSchedule();
    uint8_t cpcd[] = {0x0E, 0x03, 0x16, 0x00, 0x02, 0x01, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, cpcd, sizeof(cpcd));
    TEST_RunQueueStubSchedule();

    // 预期收到回调已连接
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_CONNECTED);
    STUB_ResetData();

    HidInformation_S *info = nullptr;
    HidFreeFunc freeFunc = nullptr;
    ret = HidGetInformation(&g_addr, &info, &freeFunc);
    EXPECT_EQ(info->desc.type, 1);
    EXPECT_EQ(info->reportInfo->reportIdAndType.reportId, 1);
    EXPECT_EQ(info->reportInfo->reportIdAndType.reportType, 1);
    EXPECT_EQ(info->reportInfo->reportInfoValue.len, 4);
    uint8_t value[] = {0x01, 0x02, 0x03, 0x04};
    EXPECT_EQ(memcmp(info->reportInfo->reportInfoValue.data, value, 4), 0);
    EXPECT_NE(freeFunc, nullptr);
    freeFunc(info);

    uint8_t state = HID_DISCONNECTED;
    ret = HidGetConnectState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HID_CONNECTED);

    ret = HidReadProperty(&g_addr, HID_TYPE_AND_FORMAT_DESC, nullptr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, desc, sizeof(desc));
    EXPECT_EQ(g_recvCbk, true);
    STUB_ResetData();

    ret = HidReadProperty(&g_addr, HID_WORK_STATUS_INDICATION, nullptr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    uint8_t workstatus[] = {0x09, 0x03, 0x01};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, workstatus, sizeof(desc));
    EXPECT_EQ(g_recvCbk, true);
    STUB_ResetData();

    HidReportIdAndType_S reportIdAndType = { .reportId = 0x01, .reportType = 0x01 };
    ret = HidReadProperty(&g_addr, HID_INPUT_REPORT_INFO, &reportIdAndType);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, input, sizeof(input));
    EXPECT_EQ(g_recvCbk, true);
    STUB_ResetData();

    HidReportInfo_S reportInfo = { .reportIdAndType = { .reportId = 0x01, .reportType = 0x02},
                                   .reportInfoValue = { .len = 4, .data = value } };
    ret = HidWriteProperty(&g_addr, HID_OUTPUT_REPORT_INFO, &reportInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_recvCbk, true);
    STUB_ResetData();

    uint8_t workStatus = 0x01;
    ret = HidWriteProperty(&g_addr, HID_WORK_STATUS_INDICATION, &workStatus);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_recvCbk, true);
    STUB_ResetData();

    SLE_Addr_S *addrs = nullptr;
    size_t num = 0;
    HidFreeFunc func = nullptr;
    ret = HidGetConnectedDevice(&addrs, &num, &func);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(num, 1);
    EXPECT_EQ(memcmp(addrs, &g_addr, sizeof(SLE_Addr_S)), 0);
    EXPECT_NE(func, nullptr);
    func(addrs);

    uint8_t ntf[] = {0x0F, 0x03, 0x16, 0x00, 0x04, 0x00, 0x04, 0x03, 0x02, 0x01};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, ntf, sizeof(ntf));
    EXPECT_EQ(g_recvCbk, true);
    STUB_ResetData();

    ret = HidDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // // 预期收到回调断连中
    // EXPECT_EQ(g_recvCbk, true);
    // EXPECT_EQ(g_state, HID_DISCONNECTING);
    // STUB_ResetData();

    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();
}

/**
 * @test HID_TEST_002
 * @brief 此测试用例用于验证HID Profile连接流程，对端设备UUID后两字节为小端序。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用HidRegClientCbk注册回调；
 * 2. 调用HidConnect开启连接；
 * 3. 模拟链路建立成功；
 * 4. 模拟收到连接流程中的SSAP响应报文；
 * 5. 校验是否收到连接状态的回调已连接；
 * 6. 调用NLSTK_McpVolumeDisconnect断连；
 * 7. 模拟链路断连销毁；
 * 8. 校验是否收到连接状态的回调已断连；
 *
 * @pre
 * - SSAP模块已初始化。
 * - HID模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_HID_TEST, HID_TEST_002)
{
    // 注册回调
    HidClientCallBack_S clientCallbacks = {0};
    clientCallbacks.notifyPropertyCbk = &STUB_NotifyPropertyCbk;
    clientCallbacks.readPropertyCbk = &STUB_ReadPropertyCbk;
    clientCallbacks.writePropertyCbk = &STUB_WritePropertyCbk;
    clientCallbacks.connectStateChangeCbk = &STUB_ConnectStateChangeCbk;

    NLSTK_Errcode_E ret = HidRegClientCbk(&clientCallbacks);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    uint8_t state = HID_CONNECTED;
    ret = HidGetConnectState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HID_DISCONNECTED);

    SLE_Addr_S *addrs = nullptr;
    size_t num = 0;
    HidFreeFunc func = nullptr;
    ret = HidGetConnectedDevice(&addrs, &num, &func);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(num, 0);

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheHidServicePen();

    // 调用连接
    ret = HidConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_CONNECTING);
    STUB_ResetData();

    ret = HidGetConnectState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HID_CONNECTING);

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 模拟收到连接流程的SSAP报文
    uint8_t desc[] = {0x09, 0x03, 0x01};
    uint8_t index1[] = {0x09, 0x03, 0x01, 0x01, 0x16, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t index2[] = {0x09, 0x03, 0x01, 0x02, 0x17, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t index3[] = {0x09, 0x03, 0x01, 0x03, 0x18, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, desc, sizeof(desc));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, index1, sizeof(index1));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, index2, sizeof(index2));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, index3, sizeof(index3));
    TEST_RunQueueStubSchedule();
    uint8_t input[] = {0x09, 0x03, 0x01, 0x02, 0x03, 0x04};
    uint8_t output[] = {0x09, 0x03, 0x04, 0x03, 0x02, 0x01, 0x00};
    uint8_t feature[] = {0x09, 0x03, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, input, sizeof(input));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, output, sizeof(output));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, feature, sizeof(feature));
    TEST_RunQueueStubSchedule();
    uint8_t cpcd[] = {0x0E, 0x03, 0x16, 0x00, 0x02, 0x01, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, cpcd, sizeof(cpcd));
    TEST_RunQueueStubSchedule();

    // 预期收到回调已连接
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_CONNECTED);
    STUB_ResetData();

    HidInformation_S *info = nullptr;
    HidFreeFunc freeFunc = nullptr;
    ret = HidGetInformation(&g_addr, &info, &freeFunc);
    EXPECT_EQ(info->desc.type, 1);
    EXPECT_EQ(info->reportInfo->reportIdAndType.reportId, 1);
    EXPECT_EQ(info->reportInfo->reportIdAndType.reportType, 1);
    EXPECT_EQ(info->reportInfo->reportInfoValue.len, 4);
    uint8_t value[] = {0x01, 0x02, 0x03, 0x04};
    EXPECT_EQ(memcmp(info->reportInfo->reportInfoValue.data, value, 4), 0);
    EXPECT_NE(freeFunc, nullptr);
    freeFunc(info);

    ret = HidGetConnectState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HID_CONNECTED);

    ret = HidReadProperty(&g_addr, HID_TYPE_AND_FORMAT_DESC, nullptr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, desc, sizeof(desc));
    EXPECT_EQ(g_recvCbk, true);
    STUB_ResetData();

    ret = HidReadProperty(&g_addr, HID_WORK_STATUS_INDICATION, nullptr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    uint8_t workstatus[] = {0x09, 0x03, 0x01};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, workstatus, sizeof(desc));
    EXPECT_EQ(g_recvCbk, true);
    STUB_ResetData();

    HidReportIdAndType_S reportIdAndType = { .reportId = 0x01, .reportType = 0x01 };
    ret = HidReadProperty(&g_addr, HID_INPUT_REPORT_INFO, &reportIdAndType);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, input, sizeof(input));
    EXPECT_EQ(g_recvCbk, true);
    STUB_ResetData();

    HidReportInfo_S reportInfo = { .reportIdAndType = { .reportId = 0x01, .reportType = 0x02},
                                   .reportInfoValue = { .len = 4, .data = value } };
    ret = HidWriteProperty(&g_addr, HID_OUTPUT_REPORT_INFO, &reportInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_recvCbk, true);
    STUB_ResetData();

    uint8_t workStatus = 0x01;
    ret = HidWriteProperty(&g_addr, HID_WORK_STATUS_INDICATION, &workStatus);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_recvCbk, true);
    STUB_ResetData();

    ret = HidGetConnectedDevice(&addrs, &num, &func);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(num, 1);
    EXPECT_EQ(memcmp(addrs, &g_addr, sizeof(SLE_Addr_S)), 0);
    EXPECT_NE(func, nullptr);
    func(addrs);

    uint8_t ntf[] = {0x0F, 0x03, 0x16, 0x00, 0x04, 0x00, 0x04, 0x03, 0x02, 0x01};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, ntf, sizeof(ntf));
    EXPECT_EQ(g_recvCbk, true);
    STUB_ResetData();

    ret = HidDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // // 预期收到回调断连中
    // EXPECT_EQ(g_recvCbk, true);
    // EXPECT_EQ(g_state, HID_DISCONNECTING);
    // STUB_ResetData();

    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();
}

TEST_F(IT_HID_TEST, HID_TEST_STM_FAIL_001)
{
    // 注册回调
    HidClientCallBack_S clientCallbacks = {0};
    clientCallbacks.notifyPropertyCbk = &STUB_NotifyPropertyCbk;
    clientCallbacks.readPropertyCbk = &STUB_ReadPropertyCbk;
    clientCallbacks.writePropertyCbk = &STUB_WritePropertyCbk;
    clientCallbacks.connectStateChangeCbk = &STUB_ConnectStateChangeCbk;

    NLSTK_Errcode_E ret = HidRegClientCbk(&clientCallbacks);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    STUB_ResetData();

    HidDevice_S dev = {0};
    (void)memcpy_s(&dev.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    dev.appId = SSAP_APP_INVALID_ID;
    HidStmParam_S msg = {0};

    // HID_STATE_UNINIT状态下的各种事件
    dev.state = HID_STATE_UNINIT;
    msg.what = HID_ON_USER_DISCONNECT;
    HidStateMachineCall(&dev, msg);

    // HID_STATE_INIT状态下的各种事件
    dev.state = HID_STATE_INIT;
    msg.what = HID_ON_REGISTER_APP;
    int32_t appId = SSAP_APP_INVALID_ID;
    msg.extData = &appId;
    HidStateMachineCall(&dev, msg);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();

    // HID_STATE_CREATE_LINK状态下的各种事件
    dev.state = HID_STATE_CREATE_LINK;
    msg.what = HID_ON_STATE_CHANGED;
    uint8_t state = SSAP_CONNECT_STATE_DISCONNECTED;
    msg.extData = &state;
    HidStateMachineCall(&dev, msg);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();

    // HID_STATE_GET_SERVICE状态下的各种事件
    dev.state = HID_STATE_GET_SERVICE;
    dev.desc.desc = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    msg.what = HID_ON_GET_SERVICE;
    msg.extData = NULL;
    HidStateMachineCall(&dev, msg);

    dev.state = HID_STATE_GET_SERVICE;
    msg.what = HID_ON_GET_SERVICE;
    HidGetServiceMsg_S getServiceMsg = {0};
    NLSTK_SsapServ_S service = {0};
    getServiceMsg.service = &service;
    getServiceMsg.serviceNum = 0;
    msg.extData = &getServiceMsg;
    HidStateMachineCall(&dev, msg);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();

    dev.state = HID_STATE_GET_SERVICE;
    msg.what = HID_ON_GET_SERVICE;
    getServiceMsg.service = NULL;
    getServiceMsg.serviceNum = 0;
    msg.extData = &getServiceMsg;
    HidStateMachineCall(&dev, msg);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();

    dev.state = HID_STATE_GET_SERVICE;
    msg.what = HID_ON_GET_SERVICE;
    getServiceMsg.service = &service;
    getServiceMsg.serviceNum = HID_MAX_SERVICE_NUM + 1; // 0xff + 1
    msg.extData = &getServiceMsg;
    HidStateMachineCall(&dev, msg);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();

    dev.state = HID_STATE_GET_SERVICE;
    msg.what = HID_ON_GET_SERVICE;
    getServiceMsg.service = &service;   // srvcUuid != HID_SERVICE_UUID && srvcUuid != HID_SERVICE_UUID_PEN
    getServiceMsg.serviceNum = 1;
    msg.extData = &getServiceMsg;
    HidStateMachineCall(&dev, msg);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();

    dev.state = HID_STATE_GET_SERVICE;
    msg.what = HID_ON_GET_SERVICE;
    getServiceMsg.service = &service;
    service.propertyNum = HID_MAX_SERVICE_NUM + 1;  // service[i].propertyNum > HID_MAX_SERVICE_NUM
    (void)memcpy_s(&service.uuid, sizeof(NLSTK_SsapUuid_S), &g_serviceUuid, sizeof(NLSTK_SsapUuid_S));
    getServiceMsg.serviceNum = 1;
    msg.extData = &getServiceMsg;
    HidStateMachineCall(&dev, msg);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();

    dev.state = HID_STATE_GET_SERVICE;
    msg.what = HID_ON_STATE_CHANGED;
    state = SSAP_CONNECT_STATE_DISCONNECTED;
    msg.extData = &state;
    HidStateMachineCall(&dev, msg);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();

    dev.state = HID_STATE_GET_SERVICE;
    msg.what = HID_ON_STATE_CHANGED;
    state = SSAP_CONNECT_STATE_IDLE;
    msg.extData = &state;
    HidStateMachineCall(&dev, msg);
    EXPECT_EQ(g_recvCbk, false);
    STUB_ResetData();

    dev.state = HID_STATE_GET_SERVICE;
    msg.what = HID_ON_USER_DISCONNECT;
    HidStateMachineCall(&dev, msg);

    dev.state = HID_STATE_GET_SERVICE;
    msg.what = HID_ON_USER_GET_INFO;
    HidStateMachineCall(&dev, msg);

    // HID_STATE_READ_PROPERTY状态下的各种事件
    dev.state = HID_STATE_READ_PROPERTY;
    msg.what = HID_ON_STATE_CHANGED;
    HidStateMachineCall(&dev, msg);
    state = SSAP_CONNECT_STATE_DISCONNECTED;
    msg.extData = &state;
    HidStateMachineCall(&dev, msg);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();

    dev.state = HID_STATE_READ_PROPERTY;
    msg.what = HID_ON_USER_DISCONNECT;
    HidStateMachineCall(&dev, msg);

    dev.state = HID_STATE_READ_PROPERTY;
    msg.what = HID_ON_USER_GET_INFO;
    HidStateMachineCall(&dev, msg); // 进入break

    dev.state = HID_STATE_READ_PROPERTY;
    msg.what = HID_ON_READ_PROPERTY;
    HidReadPropertyMsg_S readMsg = {0};
    msg.extData = NULL; // readMsg == NULL
    HidStateMachineCall(&dev, msg);

    dev.state = HID_STATE_READ_PROPERTY;
    msg.what = HID_ON_READ_PROPERTY;
    readMsg.ret = NLSTK_ERRCODE_FAIL;
    msg.extData = &readMsg; // readMsg->ret != NLSTK_ERRCODE_SUCCESS
    HidStateMachineCall(&dev, msg);

    dev.state = HID_STATE_READ_PROPERTY;
    msg.what = HID_ON_READ_PROPERTY;
    NLSTK_SsapClientReadPropertyInfo_S property = {0};
    readMsg.property = &property;
    readMsg.ret = NLSTK_ERRCODE_SUCCESS;
    uint8_t data = 0;
    property.value.len = 1;
    property.value.data = &data;
    (void)memcpy_s(&property.uuid, sizeof(NLSTK_SsapUuid_S), &g_reportIndexUuid, sizeof(NLSTK_SsapUuid_S));
    property.handle = dev.service.handle + 1;   // readMsg->property->handle != dev->service.descHandle
    msg.extData = &readMsg;
    HidStateMachineCall(&dev, msg); // readMsg->property->value.len < HID_REPORT_INDEX_INFO_LEN
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();

    dev.state = HID_STATE_READ_PROPERTY;
    msg.what = HID_ON_READ_PROPERTY;
    readMsg.property = &property;
    readMsg.ret = NLSTK_ERRCODE_SUCCESS;
    property.value.len = 1;
    property.value.data = &data;
    // 进入HidOnReadReportInfoInReadPropertyState
    (void)memcpy_s(&property.uuid, sizeof(NLSTK_SsapUuid_S), &g_workStateUuid, sizeof(NLSTK_SsapUuid_S));
    property.handle = dev.service.handle + 1;   // readMsg->property->handle != dev->service.descHandle
    msg.extData = &readMsg;
    HidStateMachineCall(&dev, msg); // report handle not found
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();

    // HID_STATE_SET_NOTIFICATION状态下的各种事件
    dev.state = HID_STATE_SET_NOTIFICATION;
    msg.what = HID_ON_STATE_CHANGED;
    HidStateMachineCall(&dev, msg);
    state = SSAP_CONNECT_STATE_DISCONNECTED;
    msg.extData = &state;
    HidStateMachineCall(&dev, msg);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();

    dev.state = HID_STATE_SET_NOTIFICATION;
    msg.what = HID_ON_USER_DISCONNECT;
    HidStateMachineCall(&dev, msg);

    dev.state = HID_STATE_SET_NOTIFICATION;
    msg.what = HID_ON_USER_GET_INFO;
    HidStateMachineCall(&dev, msg);

    dev.state = HID_STATE_SET_NOTIFICATION;
    msg.what = HID_ON_SET_NOTIFICATION;
    msg.extData = NULL;
    HidStateMachineCall(&dev, msg); // setNtfMsg == NULL
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, HID_DISCONNECTED);
    STUB_ResetData();

    dev.state = HID_STATE_SET_NOTIFICATION;
    msg.what = HID_ON_SET_NOTIFICATION;
    dev.lastSetNtfHandle = 0;
    HidSetPropertyNtfMsg_S setNtfMsg = {.enable = true, .ret = NLSTK_ERRCODE_SUCCESS, .handle = 0x1};
    msg.extData = &setNtfMsg;
    HidStateMachineCall(&dev, msg); // setNtfMsg->handle != dev->lastSetNtfHandle
    EXPECT_EQ(g_recvCbk, false);
    STUB_ResetData();

    // HID_STATE_CONNECTED状态下的各种事件
    dev.state = HID_STATE_CONNECTED;
    msg.what = HID_ON_USER_DISCONNECT;
    HidStateMachineCall(&dev, msg);

    dev.state = HID_STATE_CONNECTED;
    msg.what = HID_ON_USER_READ;
    HidReadParam_S readParam = {};
    msg.extData = &readParam;
    readParam.type = HID_REPORT_INDEX_INFO;
    HidStateMachineCall(&dev, msg);

    dev.state = HID_STATE_CONNECTED;
    msg.what = HID_ON_USER_WRITE;
    HidWriteParam_S writeParam = {};
    msg.extData = &writeParam;
    writeParam.type = HID_TYPE_AND_FORMAT_DESC;
    HidStateMachineCall(&dev, msg);

    dev.state = HID_STATE_CONNECTED;
    msg.what = HID_ON_READ_PROPERTY;
    msg.extData = &readMsg;
    readMsg.property = &property;
    readMsg.ret = NLSTK_ERRCODE_FAIL;
    HidStateMachineCall(&dev, msg); // HidCheckPropertyParam fail
    EXPECT_EQ(g_recvCbk, true);
    STUB_ResetData();

    dev.state = HID_STATE_CONNECTED;
    msg.what = HID_ON_FIND_SERVICE;
    HidStateMachineCall(&dev, msg);

    dev.state = HID_STATE_CONNECTED;
    msg.what = HID_ON_STATE_CHANGED;
    state = SSAP_CONNECT_STATE_IDLE;
    msg.extData = &state;
    HidStateMachineCall(&dev, msg);
    EXPECT_EQ(g_recvCbk, false);
    STUB_ResetData();

    if (dev.desc.desc != NULL) {
        SDF_MemFree(dev.desc.desc);
        dev.desc.desc = NULL;
    }
    TEST_RunQueueStubSchedule();
}