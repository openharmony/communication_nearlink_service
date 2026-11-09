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

/*
 * 共存连接信息管理测试：按设备地址查询连接参数的命中与未命中、
 * 连接移除、多连接判定与整体清空。
 */

#include <gtest/gtest.h>
#include <cstring>
#include <vector>

#include "SleCoexistManager.h"
#include "securec.h"

using namespace OHOS::Nearlink;

namespace {
/* 构造一条指定地址与参数的连接信息 */
CoexistConnInfo MakeConnInfo(uint16_t lcid, uint16_t interval, uint16_t latency, uint16_t timeout, uint8_t addrByte)
{
    CoexistConnInfo info;
    info.lcid = lcid;
    info.interval = interval;
    info.latency = latency;
    info.timeout = timeout;
    info.addr.type = 0;
    (void)memset_s(info.addr.addr, SLE_ADDR_LEN, addrByte, SLE_ADDR_LEN);
    return info;
}
} // namespace

class SleCoexistManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        manager_ = SleCoexistManager::GetInstance();
    }

    void TearDown() override
    {
        manager_->ClearAll();
    }

    SleCoexistManager *manager_ = nullptr;
};

/* 按地址查询连接参数：命中返回真实参数，未命中返回失败 */
TEST_F(SleCoexistManagerTest, GetConnectionParamByAddr)
{
    manager_->UpdateConnectionInfo(MakeConnInfo(0x0102, 40, 6, 100, 0x11));

    const SLE_Addr_S query = MakeConnInfo(0, 0, 0, 0, 0x11).addr;
    const SLE_Addr_S other = MakeConnInfo(0, 0, 0, 0, 0x22).addr;

    uint16_t timeout = 0;
    uint16_t maxLatency = 0;
    ASSERT_TRUE(manager_->GetConnectionParam(query, timeout, maxLatency));
    EXPECT_EQ(timeout, 100);
    EXPECT_EQ(maxLatency, 6);
    EXPECT_FALSE(manager_->GetConnectionParam(other, timeout, maxLatency));
}

/* 带调度间隔的三参重载：命中返回超时/时延/间隔三项 */
TEST_F(SleCoexistManagerTest, GetConnectionParamWithInterval)
{
    manager_->UpdateConnectionInfo(MakeConnInfo(0x0103, 24, 4, 80, 0x33));

    const SLE_Addr_S query = MakeConnInfo(0, 0, 0, 0, 0x33).addr;
    uint16_t timeout = 0;
    uint16_t maxLatency = 0;
    uint16_t interval = 0;
    ASSERT_TRUE(manager_->GetConnectionParam(query, timeout, maxLatency, interval));
    EXPECT_EQ(timeout, 80);
    EXPECT_EQ(maxLatency, 4);
    EXPECT_EQ(interval, 24);

    const SLE_Addr_S absent = MakeConnInfo(0, 0, 0, 0, 0x44).addr;
    EXPECT_FALSE(manager_->GetConnectionParam(absent, timeout, maxLatency, interval));
}

/* 连接移除后查不到，多连接判定随条数变化，清空后回到初始态 */
TEST_F(SleCoexistManagerTest, RemoveAndMultipleConnections)
{
    EXPECT_FALSE(manager_->HasMultipleConnections());

    manager_->UpdateConnectionInfo(MakeConnInfo(0x0102, 40, 6, 100, 0x11));
    EXPECT_FALSE(manager_->HasMultipleConnections());

    manager_->UpdateConnectionInfo(MakeConnInfo(0x0104, 32, 2, 60, 0x22));
    EXPECT_TRUE(manager_->HasMultipleConnections());

    manager_->OnConnectionRemoved(0x0102);
    EXPECT_FALSE(manager_->HasMultipleConnections());

    const SLE_Addr_S removed = MakeConnInfo(0, 0, 0, 0, 0x11).addr;
    uint16_t timeout = 0;
    uint16_t maxLatency = 0;
    EXPECT_FALSE(manager_->GetConnectionParam(removed, timeout, maxLatency));

    manager_->ClearAll();
    const SLE_Addr_S remain = MakeConnInfo(0, 0, 0, 0, 0x22).addr;
    EXPECT_FALSE(manager_->GetConnectionParam(remain, timeout, maxLatency));
    EXPECT_FALSE(manager_->HasMultipleConnections());
}

/* 遍历回调能拿到全部登记的连接 */
TEST_F(SleCoexistManagerTest, IterateConnInfoVisitsAll)
{
    manager_->UpdateConnectionInfo(MakeConnInfo(0x0102, 40, 6, 100, 0x11));
    manager_->UpdateConnectionInfo(MakeConnInfo(0x0104, 32, 2, 60, 0x22));

    std::vector<uint16_t> lcids;
    manager_->IterateConnInfo([&lcids](const CoexistConnInfo &info) { lcids.push_back(info.lcid); });
    ASSERT_EQ(lcids.size(), 2U);
}
