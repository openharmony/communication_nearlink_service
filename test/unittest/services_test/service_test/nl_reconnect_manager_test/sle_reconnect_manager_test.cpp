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

#include "SleReconnectManager.h"
#include "SleConfig.h"
#include "CdsmService.h"
#include "SleInterfaceAdapterSub.h"
#include "SleInterfaceManager.h"

#include "log.h"
#include <thread>

namespace OHOS {
namespace Nearlink {
namespace TEST {

using namespace testing::ext;
constexpr int HOST_SERVER_TDD_DELAY_1000_MS = 1000;
constexpr int CDSM_CREATE_DELAY_500_MS = 500;
class SleReconnectManagerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;

private:
    void ProcPair(const std::vector<RawAddress> headset)
    {
        CdsmService *cdsmService = CdsmService::GetService();
        NL_CHECK_RETURN(cdsmService, "cdsmService is nullptr.");
        cdsmService->CdsmCreateGroup(headset[0], headset, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CDSM_CREATE_DELAY_500_MS));
        SleReconnectManager::GetInstance().OnDeviceStartPair(headset[0]);
    }

    void ProcUnPair(const std::vector<RawAddress> headset)
    {
        CdsmService *cdsmService = CdsmService::GetService();
        NL_CHECK_RETURN(cdsmService, "cdsmService is nullptr.");
        SleReconnectManager::GetInstance().OnDeviceUnpaired(headset[0]);
        cdsmService->CdsmDeleteGroup(headset[0]);
    }

    void ProcPair(const RawAddress mouse)
    {
        SleReconnectManager::GetInstance().OnDeviceStartPair(mouse);
    }

    void ProcUnPair(const RawAddress mouse)
    {
        SleReconnectManager::GetInstance().OnDeviceUnpaired(mouse);
    }

    std::vector<RawAddress> headset01 = {RawAddress("11:22:33:44:55:66"), RawAddress("66:55:44:33:22:11")}; // 耳机01
    std::vector<RawAddress> headset02 = {RawAddress("AA:BB:CC:DD:EE:FF"), RawAddress("FF:EE:DD:CC:BB:AA")}; // 耳机 02
    RawAddress mouse = RawAddress("12:34:56:78:9A:BC"); // 鼠标
};

void SleReconnectManagerTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase SleReconnectManagerTest");
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::HOST_SERVER_TDD_DELAY_1000_MS));
}
void SleReconnectManagerTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase SleReconnectManagerTest");
}
void SleReconnectManagerTest::SetUp()
{
    HILOGI("SetUp SleReconnectManagerTest");
}
void SleReconnectManagerTest::TearDown()
{
    HILOGI("TearDown SleReconnectManagerTest");
    SleConfig::GetInstance().SetReconnectDeviceAddressList("");
    SleConfig::GetInstance().Save();
    SleReconnectManager::GetInstance().pairedQueue_.clear();
    SleReconnectManager::GetInstance().windowStartIndex_ = 0;
    SleReconnectManager::GetInstance().windowEndIndex_ = 0;
    SleReconnectManager::GetInstance().inBgListDevices_.clear();
}

/**
 * @tc.name: SleReconnectManagerTest001
 * @tc.desc: 配对耳机01、耳机02、鼠标，验证队列状态和窗口索引
 * @tc.type: FUNC
 */
HWTEST_F(SleReconnectManagerTest, SleReconnectManagerTest001, TestSize.Level1)
{
    HILOGI("SleReconnectManagerTest001 start");
    // 设置bgConnMaxNum_为4并初始化
    SleReconnectManager::GetInstance().SetBgConnMaxNum(4);

    // 配对耳机01
    ProcPair(headset01);
    EXPECT_EQ(SleReconnectManager::GetInstance().pairedQueue_.size(), 1);
    EXPECT_EQ(SleReconnectManager::GetInstance().pairedQueue_.back().reportAddr_, "11:22:33:44:55:66");
    EXPECT_EQ(SleReconnectManager::GetInstance().windowStartIndex_, 0);
    EXPECT_EQ(SleReconnectManager::GetInstance().windowEndIndex_, 1);
    SleReconnectManager::GetInstance().OnDeviceConnected(headset01[0], true);
    SleReconnectManager::GetInstance().OnDeviceConnected(headset01[1], true);
    EXPECT_EQ(SleReconnectManager::GetInstance().GetWindowDeviceCount(), 2);

    // 配对耳机02
    ProcPair(headset02);
    EXPECT_EQ(SleReconnectManager::GetInstance().pairedQueue_.size(), 2);
    EXPECT_EQ(SleReconnectManager::GetInstance().pairedQueue_.back().reportAddr_, "AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(SleReconnectManager::GetInstance().windowStartIndex_, 0);
    EXPECT_EQ(SleReconnectManager::GetInstance().windowEndIndex_, 2);
    SleReconnectManager::GetInstance().OnDeviceConnected(headset02[0], true);
    SleReconnectManager::GetInstance().OnDeviceConnected(headset02[1], true);
    EXPECT_EQ(SleReconnectManager::GetInstance().GetWindowDeviceCount(), 4);

    // 配对鼠标
    ProcPair(mouse);
    EXPECT_EQ(SleReconnectManager::GetInstance().pairedQueue_.size(), 3);
    EXPECT_EQ(SleReconnectManager::GetInstance().pairedQueue_.back().reportAddr_, "12:34:56:78:9A:BC");
    EXPECT_EQ(SleReconnectManager::GetInstance().windowStartIndex_, 1);
    EXPECT_EQ(SleReconnectManager::GetInstance().windowEndIndex_, 3);
    SleReconnectManager::GetInstance().OnDeviceConnected(mouse, true);
    EXPECT_EQ(SleReconnectManager::GetInstance().GetWindowDeviceCount(), 3);

    HILOGI("SleReconnectManagerTest001 end");
}

/**
 * @tc.name: SleReconnectManagerTest002
 * @tc.desc: 配对耳机01、耳机02，手动断连耳机01，配对鼠标，手动连接耳机01，删除鼠标配对
 * @tc.type: FUNC
 */
HWTEST_F(SleReconnectManagerTest, SleReconnectManagerTest002, TestSize.Level1)
{
    HILOGI("SleReconnectManagerTest002 start");

    SleReconnectManager::GetInstance().SetBgConnMaxNum(4);

    // 配对耳机01
    ProcPair(headset01);
    EXPECT_EQ(SleReconnectManager::GetInstance().pairedQueue_.size(), 1);
    EXPECT_EQ(SleReconnectManager::GetInstance().pairedQueue_.back().reportAddr_, "11:22:33:44:55:66");
    EXPECT_EQ(SleReconnectManager::GetInstance().windowStartIndex_, 0);
    EXPECT_EQ(SleReconnectManager::GetInstance().windowEndIndex_, 1);
    SleReconnectManager::GetInstance().OnDeviceConnected(headset01[0], true);
    SleReconnectManager::GetInstance().OnDeviceConnected(headset01[1], true);
    EXPECT_EQ(SleReconnectManager::GetInstance().GetWindowDeviceCount(), 2);

    // 配对耳机02
    ProcPair(headset02);
    EXPECT_EQ(SleReconnectManager::GetInstance().pairedQueue_.size(), 2);
    EXPECT_EQ(SleReconnectManager::GetInstance().pairedQueue_.back().reportAddr_, "AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(SleReconnectManager::GetInstance().windowStartIndex_, 0);
    EXPECT_EQ(SleReconnectManager::GetInstance().windowEndIndex_, 2);
    SleReconnectManager::GetInstance().OnDeviceConnected(headset02[0], true);
    SleReconnectManager::GetInstance().OnDeviceConnected(headset02[1], true);
    EXPECT_EQ(SleReconnectManager::GetInstance().GetWindowDeviceCount(), 4);

    // 手动断连耳机01
    SleReconnectManager::GetInstance().OnDeviceDisConnected(headset01[0], true);
    EXPECT_EQ(SleReconnectManager::GetInstance().inBgListDevices_.size(), 1);

    // 配对鼠标
    ProcPair(mouse);
    EXPECT_EQ(SleReconnectManager::GetInstance().pairedQueue_.size(), 3);
    EXPECT_EQ(SleReconnectManager::GetInstance().pairedQueue_.back().reportAddr_, "12:34:56:78:9A:BC");
    EXPECT_EQ(SleReconnectManager::GetInstance().windowStartIndex_, 1);
    EXPECT_EQ(SleReconnectManager::GetInstance().windowEndIndex_, 3);
    SleReconnectManager::GetInstance().OnDeviceConnected(mouse, true);
    EXPECT_EQ(SleReconnectManager::GetInstance().GetWindowDeviceCount(), 3);

    // 手动连接耳机01
    SleReconnectManager::GetInstance().OnDeviceConnected(headset01[0], true);
    EXPECT_EQ(SleReconnectManager::GetInstance().pairedQueue_.back().reportAddr_, "11:22:33:44:55:66");
    EXPECT_EQ(SleReconnectManager::GetInstance().windowStartIndex_, 1);
    EXPECT_EQ(SleReconnectManager::GetInstance().windowEndIndex_, 3);

    // 删除鼠标配对
    ProcUnPair(mouse);
    EXPECT_EQ(SleReconnectManager::GetInstance().pairedQueue_.back().reportAddr_, "11:22:33:44:55:66");
    EXPECT_EQ(SleReconnectManager::GetInstance().windowStartIndex_, 0);
    EXPECT_EQ(SleReconnectManager::GetInstance().windowEndIndex_, 2);
    EXPECT_EQ(SleReconnectManager::GetInstance().GetWindowDeviceCount(), 4);

    HILOGI("SleReconnectManagerTest002 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS