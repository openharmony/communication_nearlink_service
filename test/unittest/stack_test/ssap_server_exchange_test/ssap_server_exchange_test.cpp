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

#include "sdf_addr.h"
#include "sdf_mem.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"


#include "cm_def.h"
#include "dtap_tcid.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssaps_server_app.h"
#include "ssaps_server_api.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

// 静态变量初始化
static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;
static int32_t g_appId = -1;
static uint16_t g_mtu = 1024;

// {SSAP_EXCHANGE_INFO_REQ(1), 保留 版本号 是否有MTU(1)，客户端MTU(2)，版本号(2)}
static uint8_t reqPkt1[] = {0x02, 0x03, 0x00, 0x02, 0x01, 0x01};
// {SSAP_EXCHANGE_INFO_RSP(1), 保留 版本号 是否有MTU(1)，服务端MTU(2)，版本号(2)}
static uint8_t rspPkt1[] = {0x03, 0x03, 0x00, 0x02, 0x01, 0x01};

// {SSAP_EXCHANGE_INFO_REQ(1), 保留 版本号 是否有MTU(1)，客户端MTU(过小)，版本号(2)}
static uint8_t reqPkt2[] = {0x02, 0x03, 0x80, 0x00, 0x01, 0x01};
// {SSAP_EXCHANGE_INFO_RSP(1), 保留 版本号 是否有MTU(1)，服务端MTU(2)，版本号(2)}
static uint8_t rspPkt2[] = {0x03, 0x03, 0xFB, 0x00, 0x01, 0x01};

// {SSAP_EXCHANGE_INFO_REQ(1), 保留 版本号 是否有MTU(1)，客户端MTU(过大)，版本号(2)}
static uint8_t reqPkt3[] = {0x02, 0x03, 0xFF, 0xFF, 0x01, 0x01};
// {SSAP_EXCHANGE_INFO_RSP(1), 保留 版本号 是否有MTU(1)，服务端MTU(2)，版本号(2)}
static uint8_t rspPkt3[] = {0x03, 0x03, 0x00, 0x04, 0x01, 0x01};

// 客户端支持的MTU为512（0x0200），小于服务端的700
#define TEST_SERVER_MTU 512
#define TEST_DEFAULT_MTU 251
#define TEST_MAX_MTU 1024

class IT_SSAP_SERVER_EXCHANGE : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;
protected:
    void SetUp() override
    {
        // 步骤1：初始化桩函数
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);

        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);

        EXPECT_CALL(dtapMock, DTAP_RegisterDataRecvCb).WillRepeatedly(TEST_DTAP_RegisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_UnregisterDataRecvCb).WillRepeatedly(TEST_DTAP_UnregisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_DataSend).WillRepeatedly(TEST_DTAP_DataSend);

        // 步骤2：初始化SSAP模块
        SSAP_LinkInit();
        SSAP_Init();
    }

    void TearDown() override
    {
        // 步骤5：去初始化SSAP模块
        NLSTK_SsapServerDeregisterApplication(g_appId);
        SSAP_DeInit();
        SSAP_LinkDeInit();
    }
};

// 模拟用户设置Mtu的时候传入的回调函数，用于接收服务端Mtu变化的结果
static void Stub_SsapServerMtuChangedCb(int32_t appId, SLE_Addr_S *addr, uint16_t mtu)
{
    g_appId = appId;
    g_mtu = mtu;
}

/**
 * @test EXCHANGEINFO_RSP_001
 * @brief 此测试用例用于验证服务端接收exchange报文与解析报文的正确性。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_SsapServerRegApp注册应用，获取appId。
 * 2. 模拟服务端接收到exchange报文，并解析报文，进行MTU协商，构造响应报文并发送。
 * 3. 校验发送报文是否符合预期。
 *
 * @pre
 * - SSAP服务端已初始化。
 *
 * @post
 * - 预期结果：
 * 1. 成功注册应用，获取到有效的appId。
 * 2. 服务端成功处理报文，并将MTU协商结果发送
 * 3. 发送的报文符合预期。
 */
TEST_F(IT_SSAP_SERVER_EXCHANGE, EXCHANGEINFO_RSP_001)
{
    // 步骤1：注册AppId
    int32_t appId = -1;
    NLSTK_SsapAppServerCb_S cb = {0};
    cb.onMtuChanged = Stub_SsapServerMtuChangedCb;

    NLSTK_Errcode_E ret = NLSTK_SsapServerRegApp(&cb, &appId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    // 先调用NLSTK_SsapServerSetMtu预设置服务端mtu为700，大于此时的客户端req的mtu512
    ret = NLSTK_SsapServerSetMtu(700);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    // 检查server端mtu数值，之后会根据该值创建link
    EXPECT_EQ(SSAP_GetServerMtu(), 700);

    // 创建逻辑链路
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 步骤2：模拟服务端接收到exchange报文
    DTAP_Data_Info_S dtapInfo = {.lcid = g_lcid};
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt1));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt1));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt1), reqPkt1, sizeof(reqPkt1));
    TEST_DTAP_RecData(TCID_SLE_SMTC, &dtapInfo, tmp);

    // 步骤3：校验发送报文是否符合预期
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rspPkt1, sizeof(rspPkt1)));

    // 校验回调上来的结果
    EXPECT_EQ(g_mtu, TEST_SERVER_MTU);

    SDF_BuffFree(tmp);
}

TEST_F(IT_SSAP_SERVER_EXCHANGE, EXCHANGEINFO_RSP_MIN)
{
    // 步骤1：注册AppId
    int32_t appId = -1;
    NLSTK_SsapAppServerCb_S cb = {0};
    cb.onMtuChanged = Stub_SsapServerMtuChangedCb;

    NLSTK_Errcode_E ret = NLSTK_SsapServerRegApp(&cb, &appId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    // 先调用NLSTK_SsapServerSetMtu预设置服务端mtu为700，大于此时的客户端req的mtu200
    ret = NLSTK_SsapServerSetMtu(700);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    // 检查server端mtu数值，之后会根据该值创建link
    EXPECT_EQ(SSAP_GetServerMtu(), 700);

    // 创建逻辑链路
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 步骤2：模拟服务端接收到exchange报文
    DTAP_Data_Info_S dtapInfo = {.lcid = g_lcid};
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt2));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt2));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt2), reqPkt2, sizeof(reqPkt2));
    TEST_DTAP_RecData(TCID_SLE_SMTC, &dtapInfo, tmp);

    // 步骤3：校验发送报文是否符合预期
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rspPkt2, sizeof(rspPkt2)));

    // 校验回调上来的结果
    EXPECT_EQ(g_mtu, TEST_DEFAULT_MTU);

    SDF_BuffFree(tmp);
}

TEST_F(IT_SSAP_SERVER_EXCHANGE, EXCHANGEINFO_RSP_MAX)
{
    // 步骤1：注册AppId
    int32_t appId = -1;
    NLSTK_SsapAppServerCb_S cb = {0};
    cb.onMtuChanged = Stub_SsapServerMtuChangedCb;

    NLSTK_Errcode_E ret = NLSTK_SsapServerRegApp(&cb, &appId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    // 先调用NLSTK_SsapServerSetMtu预设置服务端mtu为1024
    ret = NLSTK_SsapServerSetMtu(1024);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    // 检查server端mtu数值，之后会根据该值创建link
    EXPECT_EQ(SSAP_GetServerMtu(), 1024);

    // 创建逻辑链路
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 步骤2：模拟服务端接收到exchange报文
    DTAP_Data_Info_S dtapInfo = {.lcid = g_lcid};
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt3));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt3));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt3), reqPkt3, sizeof(reqPkt3));
    TEST_DTAP_RecData(TCID_SLE_SMTC, &dtapInfo, tmp);

    // 步骤3：校验发送报文是否符合预期
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rspPkt3, sizeof(rspPkt3)));

    // 校验回调上来的结果
    EXPECT_EQ(g_mtu, TEST_MAX_MTU);

    SDF_BuffFree(tmp);
}

