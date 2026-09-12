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
#include "ssap_link_state.h"
#include "ssap_link.h"
#include "nlstk_ssap_app_server.h"
#include "sdf_addr.h"
#include "sdf_mem.h"
#include "ssap_manager.h"

#include "cpfwk_log.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static SLE_Addr_S g_replayAddr1 = {.type = PUBLIC_ADDRESS, .addr = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}};
static SLE_Addr_S g_replayAddr2 = {.type = PUBLIC_ADDRESS, .addr = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};
static SLE_Addr_S g_replayAddr3 = {.type = PUBLIC_ADDRESS, .addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};

static int32_t g_connStateCbCount = 0;
static int32_t g_mtuCbCount = 0;
static uint16_t g_lastMtu = 0;
static SLE_Addr_S g_lastConnAddr;

static void ResetReplayCounters(void)
{
    g_connStateCbCount = 0;
    g_mtuCbCount = 0;
    g_lastMtu = 0;
    (void)memset_s(&g_lastConnAddr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
}

static void ReplayConnStateCb(int32_t appId, const SLE_Addr_S *addr, uint8_t state,
    NLSTK_Errcode_E ret, int32_t reason)
{
    (void)appId;
    (void)ret;
    (void)reason;
    if (state == SSAP_CONNECT_STATE_CONNECTED) {
        g_connStateCbCount++;
        (void)memcpy_s(&g_lastConnAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    }
}

// 与 NLSTK_SsapServerMtuChanged typedef 一致（addr 无 const，nlstk_ssap_app_server.h:206）
static void ReplayMtuCb(int32_t appId, SLE_Addr_S *addr, uint16_t mtu)
{
    (void)appId;
    (void)addr;
    g_mtuCbCount++;
    g_lastMtu = mtu;
}

// SendCb 桩：创建 link 实体需要非 NULL 的发送回调（ssap_link.c 入参校验），测试不触发实际发送
static void TestSendCb(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    (void)link;
    (void)buff;
    (void)opcode;
}

static void FillReplayCb(NLSTK_SsapAppServerCb_S *cb)
{
    (void)memset_s(cb, sizeof(NLSTK_SsapAppServerCb_S), 0, sizeof(NLSTK_SsapAppServerCb_S));
    cb->onConnectionStateChanged = &ReplayConnStateCb;
    cb->onMtuChanged = &ReplayMtuCb;
}

class UT_SSAP_LINK_STATE_REPLAY : public testing::Test {
protected:
    void SetUp() override
    {
        ResetReplayCounters();
        // 实体表统一初始化为空表（重放内部会走 SSAP_FindSsapLinkByAddr 遍历，未初始化的表不安全）
        SSAP_LinkInit();
        // 清空 g_linkState 状态表并复位 cleanup 标志，保证用例间状态隔离
        // （注意：SsapRemoveAllLink 只向 CM 发断链请求且置位 g_ssapServerCleanUp，不清状态表，不能用于清理）
        SsapLinkStateDeinit();
    }

    void TearDown() override
    {
        // 清理 Replay002 创建的 SSAP link 实体表与状态表，避免泄漏到后续用例
        SSAP_LinkDeInit();
        SsapLinkStateDeinit();
    }
};

/**
 * @tc.name: SsapLinkStateReplay001
 * @tc.desc: replay reports CONNECTED links only, disconnected ones are excluded
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAP_LINK_STATE_REPLAY, SsapLinkStateReplay001, TestSize.Level1)
{
    // 预置：两条 CONNECTED、一条先连接后断开（条目应被释放）
    SsapLinkHandleRecordLinkStateFromCm(&g_replayAddr1, SSAP_CONNECT_STATE_CONNECTED);
    SsapLinkHandleRecordLinkStateFromCm(&g_replayAddr2, SSAP_CONNECT_STATE_CONNECTED);
    SsapLinkHandleRecordLinkStateFromCm(&g_replayAddr3, SSAP_CONNECT_STATE_CONNECTED);
    SsapLinkHandleRecordLinkStateFromCm(&g_replayAddr3, SSAP_CONNECT_STATE_DISCONNECTED);

    NLSTK_SsapAppServerCb_S cb;
    FillReplayCb(&cb);
    SsapLinkStateReplayToServerApp(0, &cb);

    // 恰好 2 次 CONNECTED，断开的 addr3 不回调；无 link 实体时 MTU 不补发
    EXPECT_EQ(2, g_connStateCbCount);
    EXPECT_EQ(0, g_mtuCbCount);
}

/**
 * @tc.name: SsapLinkStateReplay002
 * @tc.desc: replay also notifies the negotiated MTU when link entity exists
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAP_LINK_STATE_REPLAY, SsapLinkStateReplay002, TestSize.Level1)
{
    SsapLinkHandleRecordLinkStateFromCm(&g_replayAddr1, SSAP_CONNECT_STATE_CONNECTED);
    // 创建 link 实体（mtu 初始化为 SSAP_GetServerMtu()，与 ssap_link.c:75 一致；实体表已在 SetUp 初始化）
    SSAP_Link_S *link = SSAP_CreateSsapLink(&g_replayAddr1, 2, &TestSendCb);
    ASSERT_TRUE(link != NULL);

    NLSTK_SsapAppServerCb_S cb;
    FillReplayCb(&cb);
    SsapLinkStateReplayToServerApp(0, &cb);

    EXPECT_EQ(1, g_connStateCbCount);
    EXPECT_EQ(1, g_mtuCbCount);
    EXPECT_EQ(link->mtu, g_lastMtu);
}

/**
 * @tc.name: SsapLinkStateReplay003
 * @tc.desc: replay with null cb or null onConnectionStateChanged returns safely
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAP_LINK_STATE_REPLAY, SsapLinkStateReplay003, TestSize.Level1)
{
    SsapLinkHandleRecordLinkStateFromCm(&g_replayAddr1, SSAP_CONNECT_STATE_CONNECTED);

    // NULL cb 不崩溃
    SsapLinkStateReplayToServerApp(0, NULL);
    EXPECT_EQ(0, g_connStateCbCount);

    // onConnectionStateChanged 为 NULL 时直接返回
    NLSTK_SsapAppServerCb_S cb;
    (void)memset_s(&cb, sizeof(NLSTK_SsapAppServerCb_S), 0, sizeof(NLSTK_SsapAppServerCb_S));
    SsapLinkStateReplayToServerApp(0, &cb);
    EXPECT_EQ(0, g_connStateCbCount);
}
