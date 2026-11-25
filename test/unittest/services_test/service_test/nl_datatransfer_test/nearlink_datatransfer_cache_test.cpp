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

#include <gtest/gtest.h>
#include "nearlink_sle_datatransfer_cache.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
const std::string ADDR_A = "11:22:33:44:55:66";
const std::string ADDR_B = "AA:BB:CC:DD:EE:FF";
constexpr uint8_t TCID_A = 0x1F;
constexpr uint8_t TCID_B = 0x2E;
}

class NearlinkDataTransferCacheTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkDataTransferCacheTest::SetUpTestCase()
{
    HILOGI("NearlinkDataTransferCacheTest SetUpTestCase");
}

void NearlinkDataTransferCacheTest::TearDownTestCase()
{
    HILOGI("NearlinkDataTransferCacheTest TearDownTestCase");
}

void NearlinkDataTransferCacheTest::SetUp()
{
    HILOGI("NearlinkDataTransferCacheTest SetUp");
}

void NearlinkDataTransferCacheTest::TearDown()
{
    HILOGI("NearlinkDataTransferCacheTest TearDown");
}

static AppConnectParamMapping MakeConnect(const std::string &addr, int32_t state, uint8_t tcid = 0)
{
    AppConnectParamMapping temp;
    temp.address = addr;
    temp.state = state;
    temp.tcid = tcid;
    temp.randomAddress = "R:" + addr;
    return temp;
}

/**
 * @tc.number: SetPortConnectsAndQuery001
 * @tc.name: Test SetPortConnects and query by address.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferCacheTest, SetPortConnectsAndQuery001, TestSize.Level1)
{
    // 同地址重复写入为覆盖语义，查询能取到最新状态；空地址被拒绝不入表
    auto cache = std::make_shared<SleDataTransferCache>();
    cache->SetPortConnects(MakeConnect(ADDR_A, static_cast<int32_t>(SleConnectState::CONNECTING)));
    cache->SetPortConnects(MakeConnect(ADDR_A, static_cast<int32_t>(SleConnectState::CONNECTED)));
    AppConnectParamMapping param;
    EXPECT_TRUE(cache->GetAppConnectParamByAddr(ADDR_A, param));
    EXPECT_EQ(param.state, static_cast<int32_t>(SleConnectState::CONNECTED));
    EXPECT_FALSE(cache->GetAppConnectParamByAddr(ADDR_B, param));
    cache->SetPortConnects(MakeConnect("", static_cast<int32_t>(SleConnectState::CONNECTED)));
    EXPECT_EQ(cache->GetPortConnects().size(), static_cast<size_t>(1));
}

/**
 * @tc.number: HasAppConnect001
 * @tc.name: Test HasAppConnect with different states.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferCacheTest, HasAppConnect001, TestSize.Level1)
{
    // 已连接或连接中任一存在即认为应用在连；全部断开才返回假
    auto cache = std::make_shared<SleDataTransferCache>();
    EXPECT_FALSE(cache->HasAppConnect());
    cache->SetPortConnects(MakeConnect(ADDR_A, static_cast<int32_t>(SleConnectState::DISCONNECTED)));
    EXPECT_FALSE(cache->HasAppConnect());
    cache->SetPortConnects(MakeConnect(ADDR_A, static_cast<int32_t>(SleConnectState::CONNECTING)));
    EXPECT_TRUE(cache->HasAppConnect());
    cache->SetPortConnects(MakeConnect(ADDR_A, static_cast<int32_t>(SleConnectState::CONNECTED)));
    EXPECT_TRUE(cache->HasAppConnect());
}

/**
 * @tc.number: HasRemoteAddressConnect001
 * @tc.name: Test HasRemoteAddressConnect with address and state combinations.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferCacheTest, HasRemoteAddressConnect001, TestSize.Level1)
{
    // 地址匹配且状态为已连接或连接中才算在连；地址不匹配或已断开都返回假
    auto cache = std::make_shared<SleDataTransferCache>();
    EXPECT_FALSE(cache->HasRemoteAddressConnect(ADDR_A));
    cache->SetPortConnects(MakeConnect(ADDR_A, static_cast<int32_t>(SleConnectState::DISCONNECTED)));
    EXPECT_FALSE(cache->HasRemoteAddressConnect(ADDR_A));
    EXPECT_FALSE(cache->HasRemoteAddressConnect(ADDR_B));
    cache->SetPortConnects(MakeConnect(ADDR_A, static_cast<int32_t>(SleConnectState::CONNECTING)));
    EXPECT_TRUE(cache->HasRemoteAddressConnect(ADDR_A));
    cache->SetPortConnects(MakeConnect(ADDR_A, static_cast<int32_t>(SleConnectState::CONNECTED)));
    EXPECT_TRUE(cache->HasRemoteAddressConnect(ADDR_A));
}

/**
 * @tc.number: IsAppConnect001
 * @tc.name: Test IsAppConnect returns the connected address.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferCacheTest, IsAppConnect001, TestSize.Level1)
{
    // 有在连记录时带出对端地址；无在连记录保持出参不变并返回假
    auto cache = std::make_shared<SleDataTransferCache>();
    std::string address = ADDR_B;
    EXPECT_FALSE(cache->IsAppConnect(address));
    EXPECT_EQ(address, ADDR_B);
    cache->SetPortConnects(MakeConnect(ADDR_A, static_cast<int32_t>(SleConnectState::CONNECTED)));
    EXPECT_TRUE(cache->IsAppConnect(address));
    EXPECT_EQ(address, ADDR_A);
}

/**
 * @tc.number: HasTargetAddr001
 * @tc.name: Test HasTargetAddr address matching.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferCacheTest, HasTargetAddr001, TestSize.Level1)
{
    // 只认地址本身，不看连接状态
    auto cache = std::make_shared<SleDataTransferCache>();
    EXPECT_FALSE(cache->HasTargetAddr(ADDR_A));
    cache->SetPortConnects(MakeConnect(ADDR_A, static_cast<int32_t>(SleConnectState::DISCONNECTED)));
    EXPECT_TRUE(cache->HasTargetAddr(ADDR_A));
    EXPECT_FALSE(cache->HasTargetAddr(ADDR_B));
}

/**
 * @tc.number: GetRandomAddrByAddr001
 * @tc.name: Test GetRandomAddrByAddr fallback behavior.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferCacheTest, GetRandomAddrByAddr001, TestSize.Level1)
{
    // 命中地址返回映射的随机地址，未命中原样返回入参地址
    auto cache = std::make_shared<SleDataTransferCache>();
    EXPECT_EQ(cache->GetRandomAddrByAddr(ADDR_A), ADDR_A);
    cache->SetPortConnects(MakeConnect(ADDR_A, static_cast<int32_t>(SleConnectState::CONNECTED)));
    EXPECT_EQ(cache->GetRandomAddrByAddr(ADDR_A), "R:" + ADDR_A);
    EXPECT_EQ(cache->GetRandomAddrByAddr(ADDR_B), ADDR_B);
}

/**
 * @tc.number: UpdateStateByAddr001
 * @tc.name: Test UpdateStateByAddr only touches matching address.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferCacheTest, UpdateStateByAddr001, TestSize.Level1)
{
    // 按地址改状态：命中改写，未命中的记录保持原状态
    auto cache = std::make_shared<SleDataTransferCache>();
    cache->SetPortConnects(MakeConnect(ADDR_A, static_cast<int32_t>(SleConnectState::CONNECTED)));
    cache->SetPortConnects(MakeConnect(ADDR_B, static_cast<int32_t>(SleConnectState::CONNECTED)));
    cache->UpdateStateByAddr(ADDR_A, static_cast<int32_t>(SleConnectState::DISCONNECTED));
    AppConnectParamMapping param;
    EXPECT_TRUE(cache->GetAppConnectParamByAddr(ADDR_A, param));
    EXPECT_EQ(param.state, static_cast<int32_t>(SleConnectState::DISCONNECTED));
    EXPECT_TRUE(cache->GetAppConnectParamByAddr(ADDR_B, param));
    EXPECT_EQ(param.state, static_cast<int32_t>(SleConnectState::CONNECTED));
}

/**
 * @tc.number: UpdateStateAndGetTcidByAddr001
 * @tc.name: Test UpdateStateAndGetTcidByAddr returns tcid of matching entry.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferCacheTest, UpdateStateAndGetTcidByAddr001, TestSize.Level1)
{
    // 命中返回该记录的通道号并更新状态；未命中返回零
    auto cache = std::make_shared<SleDataTransferCache>();
    EXPECT_EQ(cache->UpdateStateAndGetTcidByAddr(ADDR_A, static_cast<int32_t>(SleConnectState::DISCONNECTING)),
        static_cast<uint8_t>(0));
    cache->SetPortConnects(MakeConnect(ADDR_A, static_cast<int32_t>(SleConnectState::CONNECTED), TCID_A));
    uint8_t tcid = cache->UpdateStateAndGetTcidByAddr(ADDR_A, static_cast<int32_t>(SleConnectState::DISCONNECTING));
    EXPECT_EQ(tcid, TCID_A);
    AppConnectParamMapping param;
    EXPECT_TRUE(cache->GetAppConnectParamByAddr(ADDR_A, param));
    EXPECT_EQ(param.state, static_cast<int32_t>(SleConnectState::DISCONNECTING));
}

/**
 * @tc.number: UpdateTransferStateByAddrAndTcid001
 * @tc.name: Test UpdateTransferState requires address and tcid both matching.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferCacheTest, UpdateTransferStateByAddrAndTcid001, TestSize.Level1)
{
    // 地址与通道号都命中才迁移传输状态并保留前态；任一不匹配保持不变
    auto cache = std::make_shared<SleDataTransferCache>();
    cache->SetPortConnects(MakeConnect(ADDR_A, static_cast<int32_t>(SleConnectState::CONNECTED), TCID_A));
    cache->SetPortConnects(MakeConnect(ADDR_B, static_cast<int32_t>(SleConnectState::CONNECTED), TCID_B));
    cache->UpdateTransferState(ADDR_A, TCID_B, TransferState::TRANSFER_BUSY);
    cache->UpdateTransferState(ADDR_B, TCID_B, TransferState::TRANSFER_BUSY);
    AppConnectParamMapping param;
    EXPECT_TRUE(cache->GetAppConnectParamByAddr(ADDR_A, param));
    EXPECT_EQ(param.transState, static_cast<int32_t>(TransferState::TRANSFER_AVAILABLE));
    EXPECT_TRUE(cache->GetAppConnectParamByAddr(ADDR_B, param));
    EXPECT_EQ(param.transState, static_cast<int32_t>(TransferState::TRANSFER_BUSY));
    EXPECT_EQ(param.preTransState, static_cast<int32_t>(TransferState::TRANSFER_AVAILABLE));
}

/**
 * @tc.number: UuidTokenIdUidPidRoundtrip001
 * @tc.name: Test uuid/tokenId/uid/pid setters and getters.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferCacheTest, UuidTokenIdUidPidRoundtrip001, TestSize.Level1)
{
    // 缓存的业务标识读写一致，供端口查找链路使用
    auto cache = std::make_shared<SleDataTransferCache>();
    cache->SetUuid("060D");
    cache->SetTokenId(12345);
    cache->SetUid(200);
    cache->SetPid(300);
    EXPECT_EQ(cache->GetUuid(), "060D");
    EXPECT_EQ(cache->GetTokenId(), static_cast<uint64_t>(12345));
    EXPECT_EQ(cache->GetUid(), 200);
    EXPECT_EQ(cache->GetPid(), 300);
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
