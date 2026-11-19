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
 * 控制器服务收尾测试：连接间隔档位映射与非法档位拒绝、连接参数计算的超时
 * 饱和与重算（经共存管理器预置真实连接信息强断言）、共存模式入口守卫、
 * 共存间隔查询与待定间隔回写的各分支，以及共存使能/停用全生命周期
 * 从启动到收敛、保护定时器重建与终止态迁移。
 */

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

#include "SleCoexistManager.h"
#include "SleControllerService.h"
#include "SleInterfaceManager.h"
#include "nearlink_access_token_mock.h"
#include "nearlink_def.h"
#include "nearlink_sle_controller_def.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Nearlink;

namespace {
constexpr int DELAY_1000_MS = 1000;
constexpr const char *COEX_ADDR = "11:22:33:44:55:66";

/* 向共存管理器预置一条指定参数的连接信息，地址字节与真实地址转换一致 */
void InjectCoexConnection(const std::string &addr, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    CoexistConnInfo info;
    info.lcid = 0x0102;
    info.interval = interval;
    info.latency = latency;
    info.timeout = timeout;
    info.addr.type = 0;
    RawAddress(addr).ConvertToUint8(info.addr.addr, SLE_ADDR_LEN);
    SleCoexistManager::GetInstance()->UpdateConnectionInfo(info);
}
} // namespace

class SleControllerTailTest : public testing::Test {
public:
    SleControllerTailTest() {}
    ~SleControllerTailTest() {}

    static void SetUpTestCase(void)
    {
        NearlinkAccessTokenMock::SetNativeTokenInfo();
        SleInterfaceManager::GetInstance()->Start();
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_1000_MS));
    }

    static void TearDownTestCase(void) {}

    void SetUp(void)
    {
        controller_ = &SleControllerService::GetInstance();
        ResetCoexState();
    }

    void TearDown(void)
    {
        ResetCoexState();
        SleCoexistManager::GetInstance()->ClearAll();
    }

    /* 复位共存模式私有状态与保护定时器，单例不留跨用例状态 */
    void ResetCoexState(void)
    {
        if (controller_->sleHidCoexEnableDelayTimer_ != nullptr) {
            controller_->sleHidCoexEnableDelayTimer_->Stop();
            controller_->sleHidCoexEnableDelayTimer_ = nullptr;
        }
        if (controller_->sleHidCoexDisableDelayTimer_ != nullptr) {
            controller_->sleHidCoexDisableDelayTimer_->Stop();
            controller_->sleHidCoexDisableDelayTimer_ = nullptr;
        }
        controller_->sleHidCoexModeParam_ = {};
    }

    SleControllerService *controller_ = nullptr;
};

/* 连接间隔档位映射：档位值按表取回，非法档位直接拒绝 */
HWTEST_F(SleControllerTailTest, FetchIntervalMatrixTest_001, TestSize.Level1)
{
    uint16_t interval = 0;
    ASSERT_TRUE(controller_->FetchInterval(HIGH_SPEED_INTERVAL_4_5, interval));
    EXPECT_EQ(interval, 0x024);
    ASSERT_TRUE(controller_->FetchInterval(LOW_SPEED_INTERVAL_500, interval));
    EXPECT_EQ(interval, 0xFA0);
    EXPECT_FALSE(controller_->FetchInterval(-1, interval));
    EXPECT_FALSE(controller_->FetchInterval(0xFF, interval));
}

/* 连接参数计算：大延迟触发超时饱和并按饱和值重算，小延迟保持缓存超时不动 */
HWTEST_F(SleControllerTailTest, ConnectionParamsTimeoutArmsTest_001, TestSize.Level1)
{
    // 大延迟：换算超时溢出 16 位被饱和，缓存超时换算后小于饱和值，走重算分支
    InjectCoexConnection(COEX_ADDR, 0x320, 100, 100);
    uint16_t interval = 0xFA0;
    CM_ConnectUpdateParamReq_S updateParam = {};
    ASSERT_TRUE(controller_->GetConnectionParams(COEX_ADDR, interval, updateParam));
    EXPECT_EQ(updateParam.maxLatency, 100);
    EXPECT_EQ(updateParam.supervisionTimeout, (0xFFFF + 500) / 10);

    // 小延迟：换算超时远小于缓存超时的换算值，超时保持缓存值
    InjectCoexConnection(COEX_ADDR, 0x320, 0, 2000);
    ASSERT_TRUE(controller_->GetConnectionParams(COEX_ADDR, interval, updateParam));
    EXPECT_EQ(updateParam.maxLatency, 0);
    EXPECT_EQ(updateParam.supervisionTimeout, 2000);
}

/* 共存模式入口守卫：负值与越界模式号直接拒绝 */
HWTEST_F(SleControllerTailTest, SetSleCoexModeGuardsTest_001, TestSize.Level1)
{
    std::vector<std::string> devices = {COEX_ADDR};
    std::vector<ConnectionInterval> params = {LOW_SPEED_INTERVAL_500};
    EXPECT_FALSE(controller_->SetSleCoexMode(-1, devices, params));
    EXPECT_FALSE(controller_->SetSleCoexMode(SLE_COEX_MODE_BUTT, devices, params));
    EXPECT_FALSE(controller_->SetSleCoexMode(99, devices, params));
}

/* 共存间隔查询：未启动直接返回；启动后命中设备取当前值与上限的较大者，
 *   未命中设备保持出参不变 */
HWTEST_F(SleControllerTailTest, GetSleHidCoexIntervalArmsTest_001, TestSize.Level1)
{
    uint16_t out = 0;
    // 未启动：早退，出参不变
    controller_->GetSleHidCoexInterval(COEX_ADDR, 0x320, out);
    EXPECT_EQ(out, 0);

    controller_->sleHidCoexModeParam_.state = SleCoexModeStatus::STARTED;
    controller_->sleHidCoexModeParam_.deviceList.emplace_back(COEX_ADDR, 0x190, 0x64);

    // 当前值低于上限：返回上限
    controller_->GetSleHidCoexInterval(COEX_ADDR, 0x64, out);
    EXPECT_EQ(out, 0x190);
    // 当前值高于上限：保持当前值
    controller_->GetSleHidCoexInterval(COEX_ADDR, 0x320, out);
    EXPECT_EQ(out, 0x320);
    // 未命中设备：出参不变
    uint16_t miss = 0;
    controller_->GetSleHidCoexInterval("22:22:22:22:22:22", 0x64, miss);
    EXPECT_EQ(miss, 0);
}

/* 待定间隔回写：未启动早退；启动后命中设备写回，未命中设备不写 */
HWTEST_F(SleControllerTailTest, PendingIntervalArmsTest_001, TestSize.Level1)
{
    controller_->UpdateSleHidCoexModePendingInterval(COEX_ADDR, 0x320);
    EXPECT_TRUE(controller_->sleHidCoexModeParam_.deviceList.empty());

    controller_->sleHidCoexModeParam_.state = SleCoexModeStatus::STARTED;
    controller_->sleHidCoexModeParam_.deviceList.emplace_back(COEX_ADDR, 0x190, 0x64);
    controller_->UpdateSleHidCoexModePendingInterval(COEX_ADDR, 0x320);
    EXPECT_EQ(controller_->sleHidCoexModeParam_.deviceList[0].pendingInterval, 0x320);
    controller_->UpdateSleHidCoexModePendingInterval("22:22:22:22:22:22", 0x64);
    EXPECT_EQ(controller_->sleHidCoexModeParam_.deviceList[0].pendingInterval, 0x320);
}

/* 共存使能守卫：空设备表、参数表不匹配、非法档位、过渡态中置位均被拒绝 */
HWTEST_F(SleControllerTailTest, EnableCoexGuardsTest_001, TestSize.Level1)
{
    std::vector<std::string> devices = {COEX_ADDR};
    // 空设备表
    EXPECT_FALSE(controller_->SetSleCoexMode(SLE_HID_COEX_MODE_ENABLE, {}, {}));
    // 参数表与设备表长度不匹配
    EXPECT_FALSE(controller_->SetSleCoexMode(SLE_HID_COEX_MODE_ENABLE, devices, {}));
    // 非法档位
    EXPECT_FALSE(controller_->SetSleCoexMode(SLE_HID_COEX_MODE_ENABLE, devices, {static_cast<ConnectionInterval>(99)}));
    // 过渡态：置启动中后再使能被拒
    controller_->sleHidCoexModeParam_.state = SleCoexModeStatus::STARTING;
    EXPECT_FALSE(controller_->SetSleCoexMode(SLE_HID_COEX_MODE_ENABLE, devices, {LOW_SPEED_INTERVAL_500}));
    controller_->sleHidCoexModeParam_.state = SleCoexModeStatus::STOPPING;
    EXPECT_FALSE(controller_->SetSleCoexMode(SLE_HID_COEX_MODE_DISABLE, {}, {}));
}

/* 共存生命周期：无当前连接信息时使能立即收敛到已启动，重复使能幂等，
 *   停用收敛到已停并清空设备表 */
HWTEST_F(SleControllerTailTest, CoexLifecycleWithoutConnectionTest_001, TestSize.Level1)
{
    std::vector<std::string> devices = {COEX_ADDR};
    // 未预置连接信息：当前间隔为零，无保护时间，立即完成全部设备迁移
    ASSERT_TRUE(controller_->SetSleCoexMode(SLE_HID_COEX_MODE_ENABLE, devices, {LOW_SPEED_INTERVAL_500}));
    EXPECT_EQ(controller_->sleHidCoexModeParam_.state, SleCoexModeStatus::STARTED);
    ASSERT_EQ(controller_->sleHidCoexModeParam_.deviceList.size(), 1U);

    // 已启动后重复使能：设备已存在不重建，状态保持
    ASSERT_TRUE(controller_->SetSleCoexMode(SLE_HID_COEX_MODE_ENABLE, devices, {LOW_SPEED_INTERVAL_500}));
    EXPECT_EQ(controller_->sleHidCoexModeParam_.state, SleCoexModeStatus::STARTED);
    ASSERT_EQ(controller_->sleHidCoexModeParam_.deviceList.size(), 1U);

    // 停用：立即收敛到已停并清空设备表
    ASSERT_TRUE(controller_->SetSleCoexMode(SLE_HID_COEX_MODE_DISABLE, {}, {}));
    EXPECT_EQ(controller_->sleHidCoexModeParam_.state, SleCoexModeStatus::STOPPED);
    EXPECT_TRUE(controller_->sleHidCoexModeParam_.deviceList.empty());
}

/* 共存定时器路径：存在当前连接信息时更新带保护时间，使能定时器建立后重建会先停旧定时器，
 *   到期后状态收敛为已启动 */
HWTEST_F(SleControllerTailTest, CoexEnableTimerPathTest_001, TestSize.Level1)
{
    // 预置短间隔当前连接：使能需要把间隔提到共存上限，产生保护时间走定时器路径
    InjectCoexConnection(COEX_ADDR, 0x024, 0, 1000);
    std::vector<std::string> devices = {COEX_ADDR};
    ASSERT_TRUE(controller_->SetSleCoexMode(SLE_HID_COEX_MODE_ENABLE, devices, {LOW_SPEED_INTERVAL_500}));
    EXPECT_EQ(controller_->sleHidCoexModeParam_.state, SleCoexModeStatus::STARTING);
    EXPECT_NE(controller_->sleHidCoexEnableDelayTimer_, nullptr);

    // 直接再排一次保护更新：旧定时器被停掉重建
    std::vector<SleHidCoexDevice> list = controller_->sleHidCoexModeParam_.deviceList;
    controller_->StartSleHidCoexUpdateTimer(list, 1, SleCoexModeStatus::STARTING, 50);
    EXPECT_NE(controller_->sleHidCoexEnableDelayTimer_, nullptr);

    // 保护时间到期后完成迁移收敛
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    EXPECT_EQ(controller_->sleHidCoexModeParam_.state, SleCoexModeStatus::STARTED);
}

/* 终止下标与非法状态：递归到末尾完成状态迁移，非法状态入参安全返回 */
HWTEST_F(SleControllerTailTest, ForEachTerminationAndGuardTest_001, TestSize.Level1)
{
    // 空设备表 + 启动中状态：立即走终止迁移
    controller_->sleHidCoexModeParam_.state = SleCoexModeStatus::STARTING;
    controller_->UpdateSleHidCoexIntervalForEach({}, 0, SleCoexModeStatus::STARTING);
    EXPECT_EQ(controller_->sleHidCoexModeParam_.state, SleCoexModeStatus::STARTED);

    // 停止中状态：终止迁移清设备表并置已停
    controller_->sleHidCoexModeParam_.state = SleCoexModeStatus::STOPPING;
    controller_->sleHidCoexModeParam_.deviceList.emplace_back(COEX_ADDR, 0x190, 0x64);
    controller_->UpdateSleHidCoexIntervalForEach(controller_->sleHidCoexModeParam_.deviceList, 1,
        SleCoexModeStatus::STOPPING);
    EXPECT_EQ(controller_->sleHidCoexModeParam_.state, SleCoexModeStatus::STOPPED);
    EXPECT_TRUE(controller_->sleHidCoexModeParam_.deviceList.empty());

    // 非法状态入参：安全返回不改状态
    controller_->UpdateSleHidCoexIntervalForEach({}, 0, SleCoexModeStatus::STARTED);
    EXPECT_EQ(controller_->sleHidCoexModeParam_.state, SleCoexModeStatus::STOPPED);
}
