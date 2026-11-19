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
 * 随机地址管理器测试：真实地址与随机地址的映射增删查、随机地址格式、
 * 配对保留/取消保留、周期清理的四种到期情形、真地址直通与随机替换、
 * 重启后从配置文件恢复保留设备。
 */

#include <gtest/gtest.h>
#include <chrono>
#include <string>

#include "nearlink_device_manager.h"
#include "SleConfig.h"

/* 这两个产品自由函数没有头文件声明，此处补声明以便直测 */
namespace OHOS {
namespace Nearlink {
extern void ToUpper(char *arr);
RawAddress GenerateRandomMacAddress();
} // namespace Nearlink
} // namespace OHOS

using namespace OHOS::Nearlink;

namespace {
constexpr const char *REAL_A = "11:11:11:11:11:11";
constexpr const char *REAL_B = "22:22:22:22:22:22";
constexpr const char *RANDOM_A = "AA:BB:CC:DD:EE:01";
constexpr const char *RANDOM_B = "AA:BB:CC:DD:EE:02";
/* 测试进程无对端 MAC 权限，真实地址判定按无权限路径走查表 */
constexpr uint32_t TOKEN_WITHOUT_PERMISSION = 0xdeadbeef;

/* 当前毫秒时间戳，与产品清理策略同一时基 */
int64_t NowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
} // namespace

class NearlinkDeviceManagerTest : public ::testing::Test {
protected:
    /* 配置单例首次使用前必须加载文档，否则所有写配置操作静默失败 */
    static void SetUpTestCase()
    {
        SleConfig::GetInstance().LoadConfigInfo();
    }

    static void TearDownTestCase() {}

    void SetUp() override
    {
        manager_ = NearlinkDeviceManager::GetInstance();
    }

    void TearDown() override
    {
        /* 清空内存映射并停掉清理定时器；配置文件中本用例写入的设备段落一并移除 */
        manager_->ClearDevicesInfo();
        if (manager_->nearlinkDeviceManagerTimer_ != nullptr) {
            manager_->nearlinkDeviceManagerTimer_->Stop();
        }
        (void)SleConfig::GetInstance().RemovePairedDevice(REAL_A);
        (void)SleConfig::GetInstance().RemovePairedDevice(REAL_B);
        (void)SleConfig::GetInstance().Save();
    }

    /* 直接向映射注入指定时间戳的记录，驱动清理策略的不同到期分支 */
    void InjectDeviceInfo(const std::string &real, const std::string &random, int64_t updateTime, bool isRetention)
    {
        NearlinkDeviceManager::NearlinkDeviceInfo info(real, random, updateTime, isRetention);
        manager_->nearlinkDevicesMap_.EnsureInsert(real, info);
    }

    NearlinkDeviceManager *manager_ = nullptr;
};

/* 大写化只作用于小写字母，数字、标点与高于小写区的字符原样保留 */
TEST(NearlinkDeviceManagerUtilTest, ToUpperConvertsLowercaseOnly)
{
    char text[] = "ab1z-M~";
    OHOS::Nearlink::ToUpper(text);
    EXPECT_STREQ(text, "AB1Z-M~");
}

/* 生成的随机地址：17 位、冒号落在固定位、全为十六进制字符、第二位为偶数值 */
TEST(NearlinkDeviceManagerUtilTest, GenerateRandomMacAddressFormat)
{
    RawAddress addr = OHOS::Nearlink::GenerateRandomMacAddress();
    const std::string text = addr.GetAddress();
    ASSERT_EQ(text.size(), 17U);
    for (size_t i = 0; i < text.size(); ++i) {
        if (i == 2 || i == 5 || i == 8 || i == 11 || i == 14) {
            EXPECT_EQ(text[i], ':');
            continue;
        }
        EXPECT_TRUE((text[i] >= '0' && text[i] <= '9') || (text[i] >= 'A' && text[i] <= 'F')) << "at " << i;
    }
    /* 静态随机地址第二字节高两位为 1，对应十六进制字符必为偶数 */
    const char second = text[1];
    const int value = (second >= 'A') ? (second - 'A' + 10) : (second - '0');
    EXPECT_EQ(value % 2, 0);
}

/* 已登记设备能取回随机地址；未登记设备返回不存在 */
TEST_F(NearlinkDeviceManagerTest, GetDeviceRandomAddrHitAndMiss)
{
    manager_->AddDeviceInfo(RawAddress(REAL_A), RawAddress(RANDOM_A), true);
    RawAddress random;
    EXPECT_EQ(manager_->GetDeviceRandomAddr(RawAddress(REAL_A), random), RET_SUCCESS);
    EXPECT_EQ(random.GetAddress(), RANDOM_A);

    EXPECT_EQ(manager_->GetDeviceRandomAddr(RawAddress(REAL_B), random), RET_NO_EXIST);
}

/* 按随机地址反查真实地址：三个重载均命中；查不到时按输入地址原样返回 */
TEST_F(NearlinkDeviceManagerTest, GetDeviceRealAddrByRandomAddress)
{
    manager_->AddDeviceInfo(RawAddress(REAL_A), RawAddress(RANDOM_A), true);

    RawAddress real;
    manager_->GetDeviceRealAddr(RawAddress(RANDOM_A), real);
    EXPECT_EQ(real.GetAddress(), REAL_A);

    std::string realStr;
    manager_->GetDeviceRealAddr(RANDOM_A, realStr, TOKEN_WITHOUT_PERMISSION);
    EXPECT_EQ(realStr, REAL_A);
    manager_->GetDeviceRealAddr(RANDOM_A, realStr);
    EXPECT_EQ(realStr, REAL_A);

    /* 输入不是已登记的随机地址：无权限时视为真实地址直接透传 */
    manager_->GetDeviceRealAddr(RawAddress(REAL_B), real);
    EXPECT_EQ(real.GetAddress(), REAL_B);
    manager_->GetDeviceRealAddr(REAL_B, realStr, TOKEN_WITHOUT_PERMISSION);
    EXPECT_EQ(realStr, REAL_B);
    manager_->GetDeviceRealAddr(REAL_B, realStr);
    EXPECT_EQ(realStr, REAL_B);
}

/* 配对成功保留随机地址（写配置并留存），取消配对仅解除保留不删记录 */
TEST_F(NearlinkDeviceManagerTest, UpdateRandomAddressMapRetainAndUnretain)
{
    manager_->UpdateRandomAddressMap(RawAddress(REAL_A), static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    RawAddress random;
    ASSERT_EQ(manager_->GetDeviceRandomAddr(RawAddress(REAL_A), random), RET_SUCCESS);
    EXPECT_FALSE(random.GetAddress().empty());

    /* 已保留的记录即使超时也不被清理 */
    InjectDeviceInfo(REAL_B, RANDOM_B, NowMs() - TIME_MINUTES_10 - 1, true);
    manager_->UpdateRandomAddressMap(RawAddress(REAL_B), static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    manager_->ScheduleCleanDeviceInfo();
    EXPECT_EQ(manager_->GetDeviceRandomAddr(RawAddress(REAL_B), random), RET_SUCCESS);

    /* 取消配对：解除保留，记录仍在 */
    manager_->UpdateRandomAddressMap(RawAddress(REAL_B), static_cast<int>(SlePairState::SLE_PAIR_NONE));
    EXPECT_EQ(manager_->GetDeviceRandomAddr(RawAddress(REAL_B), random), RET_SUCCESS);
}

/* 其他配对状态既不保留也不解除，未登记设备的映射不变 */
TEST_F(NearlinkDeviceManagerTest, UpdateRandomAddressMapIgnoresOtherStates)
{
    manager_->UpdateRandomAddressMap(RawAddress(REAL_A), static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
    RawAddress random;
    EXPECT_EQ(manager_->GetDeviceRandomAddr(RawAddress(REAL_A), random), RET_NO_EXIST);
}

/* 周期清理：过期未保留的删除，未过期与保留的留下，时间戳超前只记日志不删 */
TEST_F(NearlinkDeviceManagerTest, ScheduleCleanRemovesOnlyExpiredNonRetention)
{
    const int64_t now = NowMs();
    InjectDeviceInfo("33:33:33:33:33:33", "AA:BB:CC:DD:EE:03", now - TIME_MINUTES_10 - 1, false);
    InjectDeviceInfo("44:44:44:44:44:44", "AA:BB:CC:DD:EE:04", now, false);
    InjectDeviceInfo("55:55:55:55:55:55", "AA:BB:CC:DD:EE:05", now - TIME_MINUTES_10 - 1, true);
    InjectDeviceInfo("66:66:66:66:66:66", "AA:BB:CC:DD:EE:06", now + 1000, false);

    manager_->ScheduleCleanDeviceInfo();

    RawAddress random;
    EXPECT_EQ(manager_->GetDeviceRandomAddr(RawAddress("33:33:33:33:33:33"), random), RET_NO_EXIST);
    EXPECT_EQ(manager_->GetDeviceRandomAddr(RawAddress("44:44:44:44:44:44"), random), RET_SUCCESS);
    EXPECT_EQ(manager_->GetDeviceRandomAddr(RawAddress("55:55:55:55:55:55"), random), RET_SUCCESS);
    EXPECT_EQ(manager_->GetDeviceRandomAddr(RawAddress("66:66:66:66:66:66"), random), RET_SUCCESS);
}

/* 有权限用真实地址：直通返回；无权限且未登记：生成随机地址并登记保存 */
TEST_F(NearlinkDeviceManagerTest, ConvertToRandomAddressTwoBranches)
{
    RawAddress out;
    manager_->ConvertToRandomAddress(true, RawAddress(REAL_A), out, true);
    EXPECT_EQ(out.GetAddress(), REAL_A);

    manager_->ConvertToRandomAddress(false, RawAddress(REAL_B), out, false);
    EXPECT_NE(out.GetAddress(), REAL_B);
    EXPECT_EQ(out.GetAddress().size(), 17U);
    /* 新生成的随机地址已登记，再次转换取回同一随机地址 */
    RawAddress again;
    manager_->ConvertToRandomAddress(false, RawAddress(REAL_B), again, false);
    EXPECT_EQ(again.GetAddress(), out.GetAddress());
}

/* 无权限三参版本与显式 false 走同一替换逻辑 */
TEST_F(NearlinkDeviceManagerTest, ConvertToRandomAddressWithoutFlag)
{
    manager_->AddDeviceInfo(RawAddress(REAL_A), RawAddress(RANDOM_A), true);
    RawAddress out;
    manager_->ConvertToRandomAddress(RawAddress(REAL_A), out, true);
    EXPECT_EQ(out.GetAddress(), RANDOM_A);
}

/* 重启恢复：配置中的配对设备回到映射；缺随机地址的记录补生成并回写配置 */
TEST_F(NearlinkDeviceManagerTest, RecoverRetainedDeviceInfoFromConfig)
{
    manager_->UpdateRandomAddressMap(RawAddress(REAL_A), static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    /* 只写设备名不写随机地址，构造迁移中间态的配对记录；落盘后恢复流程才能读到 */
    ASSERT_TRUE(SleConfig::GetInstance().SetPeerName(REAL_B, "legacy"));
    ASSERT_TRUE(SleConfig::GetInstance().Save());

    manager_->ClearDevicesInfo();
    manager_->RecoverRetainedDeviceInfo();

    RawAddress random;
    ASSERT_EQ(manager_->GetDeviceRandomAddr(RawAddress(REAL_A), random), RET_SUCCESS);
    EXPECT_EQ(random.GetAddress().size(), 17U);
    ASSERT_EQ(manager_->GetDeviceRandomAddr(RawAddress(REAL_B), random), RET_SUCCESS);
    EXPECT_EQ(random.GetAddress().size(), 17U);
}
