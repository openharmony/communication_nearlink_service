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
 * 被测对象与其依赖（CDSM 服务、配置、adapter）均为真实例，只注入合作集数据与配置预置。
 * 运行约束：仅在测试机执行——用例会发布 nearlink 公共事件并写 persist.nearlink 参数
 * （TearDown 恢复）；建议 nearlink 服务停用后运行，合成地址假定不与真实配对设备冲突。
 * 包含 ProfileServiceManager.cpp 仅为注册真实 CDSM 实例，该源文件已从共享构建源排除。
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "CdsmService.h"
#include "ProfileServiceManager.cpp"
#include "SleConfig.h"
#include "SleInterfaceAdapterSub.h"
#include "SleInterfaceManager.h"
#include "SleReconnectManager.h"
#include "SleServiceManager.h"
#include "ThreadUtil.h"
#include "log.h"
#include "parameters.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {

using namespace testing::ext;

// 参数键与产品内私有常量一致（定义在匿名命名空间，无法引用，只能镜像）
constexpr const char* RECONN_BT_PARAM_KEY = "persist.nearlink.reconn_device_bluetooth_address";

// 全部为合成地址，不与真实设备冲突
const std::string kHeadsetL = "A1:B2:C3:D4:E5:01";
const std::string kHeadsetR = "A1:B2:C3:D4:E5:02";
const std::string kMouseM = "B2:C3:D4:E5:F6:03";
const std::string kUnknownU = "CC:DD:EE:FF:00:99";
const std::string kCoopA = "D4:E5:F6:07:18:0A";
const std::string kCoopA2 = "D4:E5:F6:07:18:0B";
const std::string kSingleB = "E5:F6:07:18:29:0C";
const std::string kUserDiscC = "F6:07:18:29:3A:0D";
const std::string kBtAddr = "00:11:22:33:44:55";

class SleReconnectAdapterProbeTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;

private:
    void InjectCdsmGroup(uint32_t groupId, const std::string &reportAddr, const std::vector<std::string> &members);

    SleReconnectManager &Mgr()
    {
        return SleReconnectManager::GetInstance();
    }

    std::unique_ptr<CdsmService> cdsmService_;
    std::string savedReconnList_;
    std::string savedLastAscActive_;
    std::string savedLastAscConnected_;
    std::string savedBtParam_;
    std::vector<std::string> savedPairedList_;
    SleAutoConnectPolicy savedPolicy_;
    std::vector<std::string> testAddrs_ = {kHeadsetL, kHeadsetR, kMouseM, kCoopA, kCoopA2, kSingleB, kUserDiscC};
};

void SleReconnectAdapterProbeTest::SetUpTestCase()
{
    // 只创建 adapter 对象，不启动协议栈固件——测试进程没有固件运行环境
    SleServiceManager::GetInstance()->CreateAdapters();
    ASSERT_NE(nullptr, SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    // 加载真实配置文件，后续读写与保存都作用于它
    SleConfig::GetInstance().LoadConfigInfo();
}

void SleReconnectAdapterProbeTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase SleReconnectAdapterProbeTest");
}

void SleReconnectAdapterProbeTest::SetUp()
{
    HILOGI("SetUp SleReconnectAdapterProbeTest");
    // CDSM 调用在测试线程内同步完成
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_CDSM, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);

    // 快照本用例会触碰的进程态，TearDown 原样恢复
    savedReconnList_ = SleConfig::GetInstance().GetReconnectDeviceAddressList();
    savedLastAscActive_ = SleConfig::GetInstance().GetLastASCActiveDevice();
    savedLastAscConnected_ = SleConfig::GetInstance().GetLastASCConnectedDevice();
    savedBtParam_ = OHOS::system::GetParameter(RECONN_BT_PARAM_KEY, "");
    savedPairedList_ = SleConfig::GetInstance().GetPairedAddrList();
    savedPolicy_ = SleServiceManager::GetInstance()->autoConnPolicy_.load();

    SleConfig::GetInstance().RemoveLastASCActiveDevice();
    SleConfig::GetInstance().RemoveLastASCConnectedDevice();
    SleConfig::GetInstance().SetReconnectDeviceAddressList("");

    // 挂入真实 CDSM 实例，CdsmService::GetService() 取到的就是它
    cdsmService_ = std::make_unique<CdsmService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CDSM, cdsmService_.get());
    ASSERT_EQ(cdsmService_.get(), CdsmService::GetService());

    auto &manager = Mgr();
    manager.pairedQueue_.clear();
    manager.windowStartIndex_ = 0;
    manager.windowEndIndex_ = 0;
    manager.bgConnMaxNum_ = RECONNECT_ACCEPT_LIST_MAX_SIZE;
    manager.inBgListDevices_.clear();
}

void SleReconnectAdapterProbeTest::TearDown()
{
    HILOGI("TearDown SleReconnectAdapterProbeTest");
    SleConfig::GetInstance().SetReconnectDeviceAddressList(savedReconnList_);
    if (IsValidAddress(savedLastAscActive_)) {
        SleConfig::GetInstance().SetLastASCActiveDevice(savedLastAscActive_);
    } else {
        SleConfig::GetInstance().RemoveLastASCActiveDevice();
    }
    if (IsValidAddress(savedLastAscConnected_)) {
        SleConfig::GetInstance().SetLastASCConnectedDevice(savedLastAscConnected_);
    } else {
        SleConfig::GetInstance().RemoveLastASCConnectedDevice();
    }
    // 只清理本套件新建的合成地址子节，不动快照中已存在的
    for (const auto &addr : testAddrs_) {
        if (std::find(savedPairedList_.begin(), savedPairedList_.end(), addr) == savedPairedList_.end()) {
            SleConfig::GetInstance().RemovePairedDevice(addr);
            SleConfig::GetInstance().RemoveCdsmGroup(addr);
        }
    }
    SleConfig::GetInstance().Save();
    OHOS::system::SetParameter(RECONN_BT_PARAM_KEY, savedBtParam_);
    SleServiceManager::GetInstance()->autoConnPolicy_ = savedPolicy_;

    auto &manager = Mgr();
    manager.pairedQueue_.clear();
    manager.windowStartIndex_ = 0;
    manager.windowEndIndex_ = 0;
    manager.bgConnMaxNum_ = RECONNECT_ACCEPT_LIST_MAX_SIZE;
    manager.inBgListDevices_.clear();

    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CDSM, nullptr);
    cdsmService_.reset();
    ThreadUtil::GetInstance().InitThreadStateMap();
}

void SleReconnectAdapterProbeTest::InjectCdsmGroup(uint32_t groupId, const std::string &reportAddr,
    const std::vector<std::string> &members)
{
    // 直接写内存合作集表——vendor 建组不在本套件的验证范围内
    auto group = std::make_shared<CdsmInfo>(groupId, reportAddr);
    group->memberCnt_ = members.size();
    for (const auto &member : members) {
        group->CdsmAddMemberInfo(member, 0);
    }
    cdsmService_->cdsmList_.EnsureInsert(groupId, group);
}

/**
 * @tc.name: RemoveAutoConnAudioDevConf_001
 * @tc.desc: 移除音频设备时，最后 active 设备若属于该设备所在合作集则记录被清除，
 *           回连 BT 参数因失去回连目标被重置为无效地址
 * @tc.type: FUNC
 */
HWTEST_F(SleReconnectAdapterProbeTest, RemoveAutoConnAudioDevConf_001, TestSize.Level1)
{
    InjectCdsmGroup(101, kHeadsetL, {kHeadsetL, kHeadsetR});
    ASSERT_TRUE(SleConfig::GetInstance().SetLastASCActiveDevice(kHeadsetR));

    // kHeadsetR 的上报地址是 kHeadsetL，按该上报地址移除即命中
    Mgr().RemoveAutoConnAudioDevConf(RawAddress(kHeadsetL));

    EXPECT_EQ("", SleConfig::GetInstance().GetLastASCActiveDevice());
    EXPECT_EQ(INVALID_MAC_ADDRESS, OHOS::system::GetParameter(RECONN_BT_PARAM_KEY, ""));
}

/**
 * @tc.name: RemoveAutoConnAudioDevConf_002
 * @tc.desc: 移除音频设备时，最后 connected 设备记录以同样规则被清除
 * @tc.type: FUNC
 */
HWTEST_F(SleReconnectAdapterProbeTest, RemoveAutoConnAudioDevConf_002, TestSize.Level1)
{
    InjectCdsmGroup(102, kHeadsetL, {kHeadsetL, kHeadsetR});
    ASSERT_TRUE(SleConfig::GetInstance().SetLastASCConnectedDevice(kHeadsetR));

    Mgr().RemoveAutoConnAudioDevConf(RawAddress(kHeadsetL));

    EXPECT_EQ("", SleConfig::GetInstance().GetLastASCConnectedDevice());
    EXPECT_EQ(INVALID_MAC_ADDRESS, OHOS::system::GetParameter(RECONN_BT_PARAM_KEY, ""));
}

/**
 * @tc.name: RemoveAutoConnAudioDevConf_003
 * @tc.desc: 无记录或上报地址不匹配时配置保持不变；被移除地址在合作集内时，
 *           回连 BT 参数刷新为该合作集成员映射的 BT 地址
 * @tc.type: FUNC
 */
HWTEST_F(SleReconnectAdapterProbeTest, RemoveAutoConnAudioDevConf_003, TestSize.Level1)
{
    InjectCdsmGroup(103, kHeadsetL, {kHeadsetL, kHeadsetR});
    ASSERT_TRUE(SleConfig::GetInstance().SetBtAddrBySleAddr(kHeadsetR, kBtAddr));

    // 无 active/connected 记录：两个清除分支都不触发
    Mgr().RemoveAutoConnAudioDevConf(RawAddress(kHeadsetL));
    EXPECT_EQ("", SleConfig::GetInstance().GetLastASCActiveDevice());
    EXPECT_EQ("", SleConfig::GetInstance().GetLastASCConnectedDevice());

    // 上报地址与传入地址不一致：不清除记录
    ASSERT_TRUE(SleConfig::GetInstance().SetLastASCActiveDevice(kHeadsetR));
    Mgr().RemoveAutoConnAudioDevConf(RawAddress(kMouseM));
    EXPECT_EQ(kHeadsetR, SleConfig::GetInstance().GetLastASCActiveDevice());

    // 传入地址在合作集内：参数刷新，记录仍保留
    Mgr().RemoveAutoConnAudioDevConf(RawAddress(kHeadsetR));
    EXPECT_EQ(kBtAddr, OHOS::system::GetParameter(RECONN_BT_PARAM_KEY, ""));
    EXPECT_EQ(kHeadsetR, SleConfig::GetInstance().GetLastASCActiveDevice());
}

/**
 * @tc.name: IsNeedToIgnore_001
 * @tc.desc: 关闭“自动回连音频设备”策略后所有设备都被忽略；恢复默认策略后不再整体忽略
 * @tc.type: FUNC
 */
HWTEST_F(SleReconnectAdapterProbeTest, IsNeedToIgnore_001, TestSize.Level1)
{
    InjectCdsmGroup(104, kHeadsetL, {kHeadsetL, kHeadsetR});
    ASSERT_TRUE(SleConfig::GetInstance().SetLastASCActiveDevice(kHeadsetL));

    SleServiceManager::GetInstance()->autoConnPolicy_ =
        SleAutoConnectPolicy::AUTO_CONN_EXCEPT_AUDIO_DEVICES;
    EXPECT_TRUE(Mgr().IsNeedToIgnore(kHeadsetL));
    EXPECT_TRUE(Mgr().IsNeedToIgnore(kMouseM));

    SleServiceManager::GetInstance()->autoConnPolicy_ = SleAutoConnectPolicy::AUTO_CONN_GENERAL;
    EXPECT_FALSE(Mgr().IsNeedToIgnore(kHeadsetL));
}

/**
 * @tc.name: IsNeedToIgnore_002
 * @tc.desc: 只放行回连目标及其合作集成员，无关设备被忽略；无回连目标或 CDSM 服务缺失时不忽略任何设备
 * @tc.type: FUNC
 */
HWTEST_F(SleReconnectAdapterProbeTest, IsNeedToIgnore_002, TestSize.Level1)
{
    InjectCdsmGroup(105, kHeadsetL, {kHeadsetL, kHeadsetR});
    ASSERT_TRUE(SleConfig::GetInstance().SetLastASCActiveDevice(kHeadsetL));

    EXPECT_FALSE(Mgr().IsNeedToIgnore(kHeadsetL)); // 上报地址即回连目标
    EXPECT_FALSE(Mgr().IsNeedToIgnore(kHeadsetR)); // 合作集另一成员
    EXPECT_TRUE(Mgr().IsNeedToIgnore(kMouseM));    // 无关设备

    // 无回连目标
    ASSERT_TRUE(SleConfig::GetInstance().RemoveLastASCActiveDevice());
    EXPECT_FALSE(Mgr().IsNeedToIgnore(kMouseM));

    // 无 CDSM 服务
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CDSM, nullptr);
    EXPECT_FALSE(Mgr().IsNeedToIgnore(kMouseM));
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CDSM, cdsmService_.get());
}

/**
 * @tc.name: GetReconnDevice_001
 * @tc.desc: 回连目标优先取最后 active 设备，其次最后 connected 设备，都无效则返回空
 * @tc.type: FUNC
 */
HWTEST_F(SleReconnectAdapterProbeTest, GetReconnDevice_001, TestSize.Level1)
{
    ASSERT_TRUE(SleConfig::GetInstance().SetLastASCConnectedDevice(kHeadsetR));
    EXPECT_EQ(kHeadsetR, Mgr().GetReconnDevice());

    ASSERT_TRUE(SleConfig::GetInstance().SetLastASCActiveDevice(kHeadsetL));
    EXPECT_EQ(kHeadsetL, Mgr().GetReconnDevice()); // active 优先

    ASSERT_TRUE(SleConfig::GetInstance().RemoveLastASCActiveDevice());
    ASSERT_TRUE(SleConfig::GetInstance().RemoveLastASCConnectedDevice());
    EXPECT_EQ("", Mgr().GetReconnDevice());
}

/**
 * @tc.name: SetAutoConnectDevice_001
 * @tc.desc: 设备配对后按激活/被动两种姿态分别落 active/connected 记录，
 *           回连 BT 参数取当前回连目标映射的 BT 地址
 * @tc.type: FUNC
 */
HWTEST_F(SleReconnectAdapterProbeTest, SetAutoConnectDevice_001, TestSize.Level1)
{
    ASSERT_TRUE(SleConfig::GetInstance().SetBtAddrBySleAddr(kHeadsetL, kBtAddr));

    // 激活连接：落 active 记录，参数为映射的 BT 地址
    Mgr().SetAutoConnectDevice(RawAddress(kHeadsetL), true, static_cast<int>(SlePairState::SLE_PAIR_PAIRED), kBtAddr);
    EXPECT_EQ(kHeadsetL, SleConfig::GetInstance().GetLastASCActiveDevice());
    EXPECT_EQ(kBtAddr, OHOS::system::GetParameter(RECONN_BT_PARAM_KEY, ""));

    // 被动连接：落 connected 记录；参数仍由 active 目标决定
    Mgr().SetAutoConnectDevice(RawAddress(kHeadsetR), false, static_cast<int>(SlePairState::SLE_PAIR_PAIRED), "");
    EXPECT_EQ(kHeadsetR, SleConfig::GetInstance().GetLastASCConnectedDevice());
    EXPECT_EQ(kBtAddr, OHOS::system::GetParameter(RECONN_BT_PARAM_KEY, ""));
    EXPECT_EQ(kHeadsetL, SleConfig::GetInstance().GetLastASCActiveDevice());
}

/**
 * @tc.name: SetAutoConnectDevice_002
 * @tc.desc: 未配对状态的设备不产生任何记录或参数变更
 * @tc.type: FUNC
 */
HWTEST_F(SleReconnectAdapterProbeTest, SetAutoConnectDevice_002, TestSize.Level1)
{
    const std::string paramBefore = OHOS::system::GetParameter(RECONN_BT_PARAM_KEY, "");
    Mgr().SetAutoConnectDevice(RawAddress(kHeadsetL), true, static_cast<int>(SlePairState::SLE_PAIR_NONE), kBtAddr);
    EXPECT_EQ("", SleConfig::GetInstance().GetLastASCActiveDevice());
    EXPECT_EQ("", SleConfig::GetInstance().GetLastASCConnectedDevice());
    EXPECT_EQ(paramBefore, OHOS::system::GetParameter(RECONN_BT_PARAM_KEY, ""));
}

/**
 * @tc.name: OnDeviceConnected_001
 * @tc.desc: 未知设备不影响队列；断连时进入后台重连名单的设备在连接成功后清除名单并移到队尾；
 *           连接成功但不可用的设备被移出队列
 * @tc.type: FUNC
 */
HWTEST_F(SleReconnectAdapterProbeTest, OnDeviceConnected_001, TestSize.Level1)
{
    Mgr().OnDeviceStartPair(RawAddress(kMouseM));
    ASSERT_EQ(1U, Mgr().pairedQueue_.size());

    // 未知设备断连/连接：队列与名单不变
    Mgr().OnDeviceDisConnected(RawAddress(kUnknownU), true);
    EXPECT_TRUE(Mgr().inBgListDevices_.empty());
    Mgr().OnDeviceConnected(RawAddress(kUnknownU), true);
    EXPECT_EQ(1U, Mgr().pairedQueue_.size());

    // 需要后台重连的断连进入名单
    Mgr().OnDeviceDisConnected(RawAddress(kMouseM), true);
    EXPECT_EQ(1U, Mgr().inBgListDevices_.count(kMouseM));

    // 连接成功且可用：名单清除、移到队尾、队列落盘
    Mgr().OnDeviceConnected(RawAddress(kMouseM), true);
    EXPECT_TRUE(Mgr().inBgListDevices_.empty());
    ASSERT_EQ(1U, Mgr().pairedQueue_.size());
    EXPECT_EQ(kMouseM, Mgr().pairedQueue_.back().reportAddr_);
    EXPECT_EQ(kMouseM, SleConfig::GetInstance().GetReconnectDeviceAddressList());

    // 连接成功但不可用：移出队列
    Mgr().OnDeviceConnected(RawAddress(kMouseM), false);
    EXPECT_TRUE(Mgr().pairedQueue_.empty());
}

/**
 * @tc.name: OnDeviceStartPair_001
 * @tc.desc: 合作集设备以整组入队（上报地址代表整组、成员数为组员数），重复配对只调整位置；
 *           非合作设备以单成员入队；队列顺序按上报地址落盘
 * @tc.type: FUNC
 */
HWTEST_F(SleReconnectAdapterProbeTest, OnDeviceStartPair_001, TestSize.Level1)
{
    InjectCdsmGroup(106, kHeadsetL, {kHeadsetL, kHeadsetR});

    // 成员地址配对，整组入队
    Mgr().OnDeviceStartPair(RawAddress(kHeadsetR));
    ASSERT_EQ(1U, Mgr().pairedQueue_.size());
    EXPECT_EQ(kHeadsetL, Mgr().pairedQueue_.back().reportAddr_);
    EXPECT_EQ(2U, Mgr().pairedQueue_.back().cdsmMemberCount_);
    EXPECT_EQ(2U, Mgr().pairedQueue_.back().cdsmMembers_.size());

    // 同组上报地址再次配对：不重复入队
    Mgr().OnDeviceStartPair(RawAddress(kHeadsetL));
    EXPECT_EQ(1U, Mgr().pairedQueue_.size());

    // 非合作设备：单成员入队
    Mgr().OnDeviceStartPair(RawAddress(kMouseM));
    ASSERT_EQ(2U, Mgr().pairedQueue_.size());
    EXPECT_EQ(1U, Mgr().pairedQueue_.back().cdsmMemberCount_);
    EXPECT_EQ(kMouseM, Mgr().pairedQueue_.back().cdsmMembers_.front());

    EXPECT_EQ(kHeadsetL + "|" + kMouseM, SleConfig::GetInstance().GetReconnectDeviceAddressList());
}

/**
 * @tc.name: GetReconnectList_001
 * @tc.desc: 从配置加载回连列表：窗口容量不足时最老设备被挤出；用户主动断连的设备
 *           降到队头且移出窗口；配置为空时回退到配对设备表加载
 * @tc.type: FUNC
 */
HWTEST_F(SleReconnectAdapterProbeTest, GetReconnectList_001, TestSize.Level1)
{
    // 预置：kCoopA 为双成员合作集，kSingleB 单设备，kUserDiscC 标记用户断连，窗口容量 3
    ASSERT_TRUE(SleConfig::GetInstance().SetCdsmMemberList(kCoopA, {kCoopA, kCoopA2}));
    ASSERT_TRUE(SleConfig::GetInstance().SetUserDisconnectedFlag(kUserDiscC, true));
    ASSERT_TRUE(SleConfig::GetInstance().SetReconnectDeviceAddressList(kCoopA + "|" + kSingleB + "|" + kUserDiscC));
    SleServiceManager::GetInstance()->autoConnPolicy_ =
        SleAutoConnectPolicy::AUTO_CONN_EXCEPT_USER_DISCONNECTED_DEVICES;

    Mgr().SetBgConnMaxNum(3);
    std::vector<std::string> list = Mgr().GetReconnectList();

    // 窗口被 kCoopA(2)+kSingleB(1) 占满；kUserDiscC 挤出 kCoopA 后因用户断连降到队头且不回窗
    EXPECT_EQ((std::vector<std::string>{kCoopA, kCoopA2, kSingleB}), list);
    ASSERT_EQ(3U, Mgr().pairedQueue_.size());
    EXPECT_EQ(kUserDiscC, Mgr().pairedQueue_.front().reportAddr_);
    EXPECT_EQ(1U, Mgr().windowStartIndex_);
    EXPECT_EQ(3U, Mgr().windowEndIndex_);

    // 列表清空后回退到配对设备表加载；窗口索引跨调用保留，归零模拟进程重启后的首次加载
    Mgr().windowStartIndex_ = 0;
    Mgr().windowEndIndex_ = 0;
    ASSERT_TRUE(SleConfig::GetInstance().SetReconnectDeviceAddressList(""));
    std::vector<std::string> defaultList = Mgr().GetReconnectList();
    EXPECT_EQ((std::vector<std::string>{kUserDiscC}), defaultList);
}

/**
 * @tc.name: WindowEdges_001
 * @tc.desc: 窗口起点越过终点时设备计数为 0；移除窗口外设备时起点先回退、有余量时再回扩；
 *           窗口外设备连接成功后移到队尾；容量为 0 时窗口保持不越界
 * @tc.type: FUNC
 */
HWTEST_F(SleReconnectAdapterProbeTest, WindowEdges_001, TestSize.Level1)
{
    Mgr().OnDeviceStartPair(RawAddress(kMouseM));
    ASSERT_EQ(1U, Mgr().pairedQueue_.size());
    EXPECT_EQ(1U, Mgr().GetWindowDeviceCount());

    // 非法窗口：起点越过终点
    Mgr().windowStartIndex_ = Mgr().windowEndIndex_ + 1;
    EXPECT_EQ(0U, Mgr().GetWindowDeviceCount());
    Mgr().windowStartIndex_ = 0;

    // 窗口外设备被移除：起点先回退，余量充足时窗口回扩到满
    Mgr().OnDeviceStartPair(RawAddress(kHeadsetL));
    Mgr().OnDeviceStartPair(RawAddress(kCoopA));
    ASSERT_EQ(3U, Mgr().pairedQueue_.size()); // [kMouseM, kHeadsetL, kCoopA]
    Mgr().windowStartIndex_ = 2;              // 窗口只含 kCoopA
    Mgr().windowEndIndex_ = 3;
    Mgr().OnDeviceUnpaired(RawAddress(kMouseM));
    EXPECT_EQ(2U, Mgr().pairedQueue_.size());
    EXPECT_EQ(0U, Mgr().windowStartIndex_);
    EXPECT_EQ(2U, Mgr().windowEndIndex_);

    // 未知地址移除：无副作用
    Mgr().OnDeviceUnpaired(RawAddress(kUnknownU));
    EXPECT_EQ(2U, Mgr().pairedQueue_.size());

    // 窗口外设备连接成功：移到队尾，窗口起点回退
    Mgr().windowStartIndex_ = 1; // 窗口只含 kCoopA，kHeadsetL 在窗口外
    Mgr().windowEndIndex_ = 2;
    Mgr().OnDeviceConnected(RawAddress(kHeadsetL), true);
    ASSERT_FALSE(Mgr().pairedQueue_.empty());
    EXPECT_EQ(kHeadsetL, Mgr().pairedQueue_.back().reportAddr_);
    EXPECT_EQ(0U, Mgr().windowStartIndex_);
    EXPECT_EQ(2U, Mgr().windowEndIndex_);

    // 单设备 + 容量 0：窗口滑动走到越界保护即停，队列不变
    Mgr().pairedQueue_.clear();
    Mgr().windowStartIndex_ = 0;
    Mgr().windowEndIndex_ = 0;
    Mgr().OnDeviceStartPair(RawAddress(kMouseM));
    ASSERT_EQ(1U, Mgr().pairedQueue_.size());
    Mgr().SetBgConnMaxNum(0);
    EXPECT_EQ(1U, Mgr().pairedQueue_.size());
    EXPECT_EQ(0U, Mgr().windowStartIndex_);
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
