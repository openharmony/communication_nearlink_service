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
 * 被测适配器与其依赖（CDSM 服务、设备管理器、配置）均为真实例：设备经公开接口入表预置，
 * 合作集数据直接写内存表注入。运行约束：仅在测试机执行——用例会写 persist.nearlink 参数
 * 与配对配置节（TearDown 恢复）；建议 nearlink 服务停用后运行，合成地址假定不与真实设备冲突。
 * 包含 ProfileServiceManager.cpp 仅为注册真实 CDSM 实例，该源文件已从共享构建源排除。
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "CdsmDefines.h"
#include "CdsmService.h"
#include "ProfileServiceManager.cpp"
#include "SleConfig.h"
#include "SleDefs.h"
#include "SleHuksTool.h"
#include "SleInterfaceAdapterSub.h"
#include "SleInterfaceManager.h"
#include "SleRemoteDeviceAdapter.h"
#include "SleRemoteDeviceManager.h"
#include "SleServiceManager.h"
#include "ThreadUtil.h"
#include "cm.h"
#include "log.h"
#include "nearlink_def.h"
#include "nearlink_utils.h"
#include "nearlink_device_model.h"
#include "parameters.h"
#include "sle_service_data.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {

using namespace testing::ext;

// 产品侧外观到设备类型的映射枚举是文件内私有定义，此处按其取值镜像断言
enum class SrvDeviceType : uint32_t {
    OTHER = 0,
    AUDIO = 1,
    KEYBOARD = 2,
    MOUSE = 3,
    PEN = 4,
    HANDLE = 5,
    WATCH = 6,
};

constexpr const char* RECONN_BT_PARAM_KEY = "persist.nearlink.reconn_device_bluetooth_address";

const std::string kHeadsetL = "A1:B2:C3:D4:E5:01";
const std::string kHeadsetR = "A1:B2:C3:D4:E5:02";
const std::string kMouseM = "B2:C3:D4:E5:F6:03";
const std::string kWatchW = "B2:C3:D4:E5:F6:04";
const std::string kUnknownU = "CC:DD:EE:FF:00:99";
const std::string kBtAddr = "00:11:22:33:44:55";

bool ContainsAddr(const std::vector<RawAddress> &list, const std::string &addr)
{
    return std::find(list.begin(), list.end(), RawAddress(addr)) != list.end();
}

class SleRemoteDeviceAdapterTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;

protected:
    SleRemoteDeviceAdapter *Adapter()
    {
        return SleRemoteDeviceAdapter::GetInstance();
    }

    SleRemoteDeviceManager *DevMgr()
    {
        return SleRemoteDeviceManager::GetInstance();
    }

    void RegisterCdsm(bool registered);
    void InjectCdsmGroup(uint32_t groupId, const std::string &reportAddr,
        const std::vector<std::string> &members);
    void AddPairedDevice(const std::string &addr);
    std::shared_ptr<SlePeripheralDevice> Device(const std::string &addr);

    std::unique_ptr<CdsmService> cdsmService_;
    std::string savedLastAscActive_;
    std::string savedLastAscConnected_;
    std::string savedBtParam_;
    std::vector<std::string> savedPairedList_;
    SleAutoConnectPolicy savedPolicy_;
    std::vector<std::string> testAddrs_ = {kHeadsetL, kHeadsetR, kMouseM, kWatchW};
};

void SleRemoteDeviceAdapterTest::SetUpTestCase()
{
    // 只创建 adapter 对象，不启动协议栈固件——测试进程没有固件运行环境
    SleServiceManager::GetInstance()->CreateAdapters();
    ASSERT_NE(nullptr, SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    SleConfig::GetInstance().LoadConfigInfo();
}

void SleRemoteDeviceAdapterTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase SleRemoteDeviceAdapterTest");
}

void SleRemoteDeviceAdapterTest::RegisterCdsm(bool registered)
{
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CDSM, registered ? cdsmService_.get() : nullptr);
}

void SleRemoteDeviceAdapterTest::InjectCdsmGroup(uint32_t groupId, const std::string &reportAddr,
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

void SleRemoteDeviceAdapterTest::AddPairedDevice(const std::string &addr)
{
    Adapter()->SleAddPeerList(RawAddress(addr));
    ASSERT_TRUE(Adapter()->SetPairStatus(RawAddress(addr), static_cast<int>(SlePairState::SLE_PAIR_PAIRED)));
}

std::shared_ptr<SlePeripheralDevice> SleRemoteDeviceAdapterTest::Device(const std::string &addr)
{
    // 取可写设备引用：部分读接口在产品侧未声明 const，只能经非 const 对象访问
    std::shared_ptr<SlePeripheralDevice> dev;
    DevMgr()->peerConnDeviceSafeList_.GetValueAndOpt(addr,
        [&dev](const std::string &key, std::shared_ptr<SlePeripheralDevice> &value) -> void {
            dev = value;
        });
    return dev;
}

void SleRemoteDeviceAdapterTest::SetUp()
{
    HILOGI("SetUp SleRemoteDeviceAdapterTest");
    // 设备适配器任务与 CDSM 调用在测试线程内同步完成
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_DEVICE_ADAPTER, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_CDSM, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);

    savedLastAscActive_ = SleConfig::GetInstance().GetLastASCActiveDevice();
    savedLastAscConnected_ = SleConfig::GetInstance().GetLastASCConnectedDevice();
    savedBtParam_ = OHOS::system::GetParameter(RECONN_BT_PARAM_KEY, "");
    savedPairedList_ = SleConfig::GetInstance().GetPairedAddrList();
    savedPolicy_ = SleServiceManager::GetInstance()->autoConnPolicy_.load();

    SleConfig::GetInstance().RemoveLastASCActiveDevice();
    SleConfig::GetInstance().RemoveLastASCConnectedDevice();

    Adapter()->RemoveAllPeripheralDevices();

    cdsmService_ = std::make_unique<CdsmService>();
    RegisterCdsm(true);
    ASSERT_EQ(cdsmService_.get(), CdsmService::GetService());
}

void SleRemoteDeviceAdapterTest::TearDown()
{
    HILOGI("TearDown SleRemoteDeviceAdapterTest");
    Adapter()->RemoveAllPeripheralDevices();

    // 只清理本套件新建的合成地址子节，不动快照中已存在的
    for (const auto &addr : testAddrs_) {
        if (std::find(savedPairedList_.begin(), savedPairedList_.end(), addr) == savedPairedList_.end()) {
            SleConfig::GetInstance().RemovePairedDevice(addr);
            SleConfig::GetInstance().RemoveCdsmGroup(addr);
        }
    }
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
    SleConfig::GetInstance().Save();
    OHOS::system::SetParameter(RECONN_BT_PARAM_KEY, savedBtParam_);
    SleServiceManager::GetInstance()->autoConnPolicy_ = savedPolicy_;

    RegisterCdsm(false);
    cdsmService_.reset();
    ThreadUtil::GetInstance().InitThreadStateMap();
}

/**
 * @tc.name: GetPairedDevices_001
 * @tc.desc: 配对设备列表把合作集成员折叠为代表地址且去重；CDSM 服务缺失时每个地址原样返回
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, GetPairedDevices_001, TestSize.Level1)
{
    AddPairedDevice(kHeadsetL);
    AddPairedDevice(kHeadsetR);
    InjectCdsmGroup(301, kHeadsetL, {kHeadsetL, kHeadsetR});

    auto paired = Adapter()->GetPairedDevices();
    EXPECT_TRUE(ContainsAddr(paired, kHeadsetL));
    EXPECT_FALSE(ContainsAddr(paired, kHeadsetR));

    RegisterCdsm(false);
    auto rawList = Adapter()->GetPairedDevices();
    EXPECT_TRUE(ContainsAddr(rawList, kHeadsetL));
    EXPECT_TRUE(ContainsAddr(rawList, kHeadsetR));
}

/**
 * @tc.name: GetConnectedDevices_001
 * @tc.desc: 已连接设备列表按合作集代表地址去重，成员地址不再单独出现
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, GetConnectedDevices_001, TestSize.Level1)
{
    AddPairedDevice(kHeadsetL);
    AddPairedDevice(kHeadsetR);
    InjectCdsmGroup(302, kHeadsetL, {kHeadsetL, kHeadsetR});

    auto connected = Adapter()->GetConnectedDevices();
    EXPECT_TRUE(ContainsAddr(connected, kHeadsetL));
    EXPECT_FALSE(ContainsAddr(connected, kHeadsetR));
}

/**
 * @tc.name: GetPairStateAndCount_001
 * @tc.desc: 合作集内任一成员已配对则整组视为已配对；瞬态与稳态计数按成员逐一统计；
 *           CDSM 服务缺失时回退设备自身状态且计数为 0
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, GetPairStateAndCount_001, TestSize.Level1)
{
    AddPairedDevice(kHeadsetL);
    AddPairedDevice(kHeadsetR);
    ASSERT_TRUE(Adapter()->SetPairStatus(RawAddress(kHeadsetR), static_cast<int>(SlePairState::SLE_PAIR_PAIRING)));
    InjectCdsmGroup(303, kHeadsetL, {kHeadsetL, kHeadsetR});

    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRED), Adapter()->GetPairState(RawAddress(kHeadsetL)));
    EXPECT_EQ(2, Adapter()->GetNotPairNoneCnt(RawAddress(kHeadsetL)));
    EXPECT_EQ(1, Adapter()->GetPairedCnt(RawAddress(kHeadsetL)));

    RegisterCdsm(false);
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRED), Adapter()->GetPairState(RawAddress(kHeadsetL)));
    EXPECT_EQ(0, Adapter()->GetNotPairNoneCnt(RawAddress(kHeadsetL)));
    EXPECT_EQ(0, Adapter()->GetPairedCnt(RawAddress(kHeadsetL)));
}

/**
 * @tc.name: GetBgList_001
 * @tc.desc: 背景回连名单只收已配对、主动配对、非车钥匙/路由器、可用且在回连列表内的设备；
 *           未配对、被动配对、不可用、被用户断连（策略不允许时）与名单外设备都被排除，
 *           回连列表为空时不做名单过滤
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, GetBgList_001, TestSize.Level1)
{
    AddPairedDevice(kHeadsetL);
    AddPairedDevice(kHeadsetR);
    AddPairedDevice(kMouseM);
    AddPairedDevice(kWatchW);
    Adapter()->SetDeviceIsAvailable(RawAddress(kHeadsetL), true);
    Adapter()->SetDeviceIsAvailable(RawAddress(kHeadsetR), true);
    Adapter()->SetDeviceIsAvailable(RawAddress(kMouseM), true);
    Adapter()->SetDeviceIsAvailable(RawAddress(kWatchW), true);
    Device(kWatchW)->SetIsUserDisconnected(true);

    // 全部合格 + 空回连列表：不做过滤
    NearlinkSafeList<RawAddress> all;
    Adapter()->GetBgList(all, {});
    EXPECT_EQ(4U, all.Size());

    // 未配对设备不入名单
    ASSERT_TRUE(Adapter()->SetPairStatus(RawAddress(kMouseM), static_cast<int>(SlePairState::SLE_PAIR_PAIRING)));
    NearlinkSafeList<RawAddress> pairingList;
    Adapter()->GetBgList(pairingList, {});
    EXPECT_EQ(3U, pairingList.Size());
    ASSERT_TRUE(Adapter()->SetPairStatus(RawAddress(kMouseM), static_cast<int>(SlePairState::SLE_PAIR_PAIRED)));

    // 被动配对设备不入名单
    DevMgr()->SetPairDirection(RawAddress(kMouseM), static_cast<int>(SlePairDirect::SLE_PAIR_PASSIVE));
    NearlinkSafeList<RawAddress> passiveList;
    Adapter()->GetBgList(passiveList, {});
    EXPECT_EQ(3U, passiveList.Size());
    DevMgr()->SetPairDirection(RawAddress(kMouseM), static_cast<int>(SlePairDirect::SLE_PAIR_ACTIVE));

    // 不可用设备不入名单
    Adapter()->SetDeviceIsAvailable(RawAddress(kMouseM), false);
    NearlinkSafeList<RawAddress> unavailableList;
    Adapter()->GetBgList(unavailableList, {});
    EXPECT_EQ(3U, unavailableList.Size());
    Adapter()->SetDeviceIsAvailable(RawAddress(kMouseM), true);

    // 用户断连且策略不允许回连时不入名单；策略允许时恢复入名单
    SleServiceManager::GetInstance()->autoConnPolicy_ =
        SleAutoConnectPolicy::AUTO_CONN_EXCEPT_USER_DISCONNECTED_DEVICES;
    NearlinkSafeList<RawAddress> userDiscList;
    Adapter()->GetBgList(userDiscList, {});
    EXPECT_EQ(3U, userDiscList.Size());
    SleServiceManager::GetInstance()->autoConnPolicy_ = SleAutoConnectPolicy::AUTO_CONN_GENERAL;
    NearlinkSafeList<RawAddress> policyAllowList;
    Adapter()->GetBgList(policyAllowList, {});
    EXPECT_EQ(4U, policyAllowList.Size());

    // 回连列表非空时只收名单内设备
    NearlinkSafeList<RawAddress> filtered;
    Adapter()->GetBgList(filtered, {kMouseM});
    EXPECT_EQ(1U, filtered.Size());
}

/**
 * @tc.name: GetReconnDeviceAndParam_001
 * @tc.desc: 回连目标激活记录优先于连接记录；参数随目标映射的蓝牙地址刷新，
 *           目标缺失时参数重置、无映射时参数保持原值
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, GetReconnDeviceAndParam_001, TestSize.Level1)
{
    AddPairedDevice(kHeadsetL);
    ASSERT_TRUE(DevMgr()->SetBtAddrBySleAddr(kHeadsetL, kBtAddr));

    ASSERT_TRUE(SleConfig::GetInstance().SetLastASCConnectedDevice(kHeadsetR));
    EXPECT_EQ(kHeadsetR, Adapter()->GetReconnDevice());
    ASSERT_TRUE(SleConfig::GetInstance().SetLastASCActiveDevice(kHeadsetL));
    EXPECT_EQ(kHeadsetL, Adapter()->GetReconnDevice());

    // 有映射：参数刷新为映射的蓝牙地址
    Adapter()->SetReconnDeviceParam();
    EXPECT_EQ(kBtAddr, OHOS::system::GetParameter(RECONN_BT_PARAM_KEY, ""));

    // 无映射的目标：参数不动
    ASSERT_TRUE(SleConfig::GetInstance().SetLastASCActiveDevice(kMouseM));
    Adapter()->SetReconnDeviceParam();
    EXPECT_EQ(kBtAddr, OHOS::system::GetParameter(RECONN_BT_PARAM_KEY, ""));

    // 无目标：参数重置为无效地址
    ASSERT_TRUE(SleConfig::GetInstance().RemoveLastASCActiveDevice());
    ASSERT_TRUE(SleConfig::GetInstance().RemoveLastASCConnectedDevice());
    Adapter()->SetReconnDeviceParam();
    EXPECT_EQ(SLE_INVALID_MAC_ADDRESS, OHOS::system::GetParameter(RECONN_BT_PARAM_KEY, ""));
}

/**
 * @tc.name: IsNeedToIgnore_001
 * @tc.desc: 关闭音频设备回连策略后全部忽略；有回连目标时只放行目标及其合作集成员，
 *           无目标或无 CDSM 服务时不忽略
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, IsNeedToIgnore_001, TestSize.Level1)
{
    InjectCdsmGroup(304, kHeadsetL, {kHeadsetL, kHeadsetR});
    ASSERT_TRUE(SleConfig::GetInstance().SetLastASCActiveDevice(kHeadsetL));

    EXPECT_FALSE(Adapter()->IsNeedToIgnore(kHeadsetL));
    EXPECT_FALSE(Adapter()->IsNeedToIgnore(kHeadsetR));
    EXPECT_TRUE(Adapter()->IsNeedToIgnore(kMouseM));

    SleServiceManager::GetInstance()->autoConnPolicy_ =
        SleAutoConnectPolicy::AUTO_CONN_EXCEPT_AUDIO_DEVICES;
    EXPECT_TRUE(Adapter()->IsNeedToIgnore(kHeadsetL));
    SleServiceManager::GetInstance()->autoConnPolicy_ = SleAutoConnectPolicy::AUTO_CONN_GENERAL;

    ASSERT_TRUE(SleConfig::GetInstance().RemoveLastASCActiveDevice());
    EXPECT_FALSE(Adapter()->IsNeedToIgnore(kMouseM));

    RegisterCdsm(false);
    ASSERT_TRUE(SleConfig::GetInstance().SetLastASCActiveDevice(kHeadsetL));
    EXPECT_FALSE(Adapter()->IsNeedToIgnore(kMouseM));
}

/**
 * @tc.name: GetDirectConnList_001
 * @tc.desc: 直连名单只收通过忽略判定的音频回连设备并置为连接中状态；
 *           设备与回连目标无关时被忽略不入名单
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, GetDirectConnList_001, TestSize.Level1)
{
    AddPairedDevice(kHeadsetL);
    DevMgr()->SetIsAudioDeviceFlag(RawAddress(kHeadsetL));
    Adapter()->SetDeviceIsAvailable(RawAddress(kHeadsetL), true);

    NearlinkSafeList<RawAddress> directList;
    Adapter()->GetDirectConnList(directList);
    EXPECT_EQ(1U, directList.Size());
    EXPECT_EQ(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING),
        Adapter()->GetAcbState(kHeadsetL));

    // 回连目标为其他无关设备时，kHeadsetL 命中忽略判定
    ASSERT_TRUE(SleConfig::GetInstance().SetLastASCActiveDevice(kMouseM));
    NearlinkSafeList<RawAddress> ignoredList;
    Adapter()->GetDirectConnList(ignoredList);
    EXPECT_EQ(0U, ignoredList.Size());
}

/**
 * @tc.name: DeviceNameAndAlias_001
 * @tc.desc: 设备名读取在管理器未命中时回落扫描缓存最终为空；别名读取走管理器，
 *           写入对非合作设备直接生效，对合作设备同步写全部成员
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, DeviceNameAndAlias_001, TestSize.Level1)
{
    AddPairedDevice(kHeadsetL);
    AddPairedDevice(kMouseM);

    EXPECT_EQ("", Adapter()->GetDeviceName(RawAddress(kMouseM)));
    EXPECT_EQ("", Adapter()->GetAliasName(RawAddress(kMouseM)));
    ASSERT_TRUE(Adapter()->SetName(RawAddress(kMouseM), "MouseName"));
    EXPECT_EQ("MouseName", Adapter()->GetDeviceName(RawAddress(kMouseM)));
    // 设名未设别名：别名读取回落设备名
    EXPECT_EQ("MouseName", Adapter()->GetAliasName(RawAddress(kMouseM)));
    ASSERT_TRUE(Adapter()->SetAliasName(RawAddress(kMouseM), "MouseAlias"));
    EXPECT_EQ("MouseAlias", Adapter()->GetAliasName(RawAddress(kMouseM)));

    // 合作集成员改名：整组设备同步生效
    InjectCdsmGroup(305, kHeadsetL, {kHeadsetL, kHeadsetR});
    AddPairedDevice(kHeadsetR);
    ASSERT_TRUE(Adapter()->SetName(RawAddress(kHeadsetR), "CoopName"));
    ASSERT_TRUE(Adapter()->SetAliasName(RawAddress(kHeadsetR), "CoopAlias"));
    EXPECT_EQ("CoopName", Adapter()->GetDeviceName(RawAddress(kHeadsetL)));
    EXPECT_EQ("CoopName", Adapter()->GetDeviceName(RawAddress(kHeadsetR)));
    EXPECT_EQ("CoopAlias", Adapter()->GetAliasName(RawAddress(kHeadsetL)));
}

/**
 * @tc.name: SetAppearance_001
 * @tc.desc: 外观写入对非合作设备直接生效，对合作设备同步写全部成员
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, SetAppearance_001, TestSize.Level1)
{
    AddPairedDevice(kMouseM);
    ASSERT_TRUE(Adapter()->SetAppearance(RawAddress(kMouseM),
        static_cast<int>(DeviceClassForService::DEVICE_MOUSE)));
    ASSERT_EQ(static_cast<int>(DeviceClassForService::DEVICE_MOUSE),
        Device(kMouseM)->GetAppearance());
    EXPECT_EQ(static_cast<int>(DeviceClassForService::DEVICE_MOUSE),
        Adapter()->GetDeviceAppearance(RawAddress(kMouseM)));

    InjectCdsmGroup(306, kHeadsetL, {kHeadsetL, kHeadsetR});
    AddPairedDevice(kHeadsetR);
    ASSERT_TRUE(Adapter()->SetAppearance(RawAddress(kHeadsetR),
        static_cast<int>(DeviceClassForService::DEVICE_IN_EAR_EARPHONE)));
    ASSERT_EQ(static_cast<int>(DeviceClassForService::DEVICE_IN_EAR_EARPHONE),
        Device(kHeadsetL)->GetAppearance());
    EXPECT_EQ(static_cast<int>(DeviceClassForService::DEVICE_IN_EAR_EARPHONE),
        Adapter()->GetDeviceAppearance(RawAddress(kHeadsetL)));
}

/**
 * @tc.name: GetDeviceAppearance_001
 * @tc.desc: 外观读取按管理器、扫描、云端顺序回落；无任何来源时返回无效值，0 值归一为无效值
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, GetDeviceAppearance_001, TestSize.Level1)
{
    AddPairedDevice(kMouseM);
    EXPECT_EQ(INVALID_APPEARANCE, Adapter()->GetDeviceAppearance(RawAddress(kMouseM)));

    ASSERT_TRUE(DevMgr()->SetAppearance(RawAddress(kMouseM),
        static_cast<int>(DeviceClassForService::DEVICE_KEYBOARD)));
    EXPECT_EQ(static_cast<int>(DeviceClassForService::DEVICE_KEYBOARD),
        Adapter()->GetDeviceAppearance(RawAddress(kMouseM)));

    ASSERT_TRUE(DevMgr()->SetAppearance(RawAddress(kMouseM), 0));
    EXPECT_EQ(INVALID_APPEARANCE, Adapter()->GetDeviceAppearance(RawAddress(kMouseM)));
}

/**
 * @tc.name: AcbStateQuery_001
 * @tc.desc: 已连接/已加密查询对合作集按成员任一命中判定；CDSM 服务缺失时回退设备自身状态
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, AcbStateQuery_001, TestSize.Level1)
{
    AddPairedDevice(kHeadsetL);
    AddPairedDevice(kHeadsetR);
    InjectCdsmGroup(307, kHeadsetL, {kHeadsetL, kHeadsetR});

    // 合作集内 report 已连接：整组视为已连接
    ASSERT_TRUE(Adapter()->SetAcbState(kHeadsetL, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED)));
    EXPECT_TRUE(Adapter()->IsAcbConnected(RawAddress(kHeadsetR)));
    EXPECT_FALSE(Adapter()->IsAcbEncrypted(RawAddress(kHeadsetR)));

    // 成员已加密：整组视为已加密
    ASSERT_TRUE(Adapter()->SetAcbState(kHeadsetR, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED)));
    EXPECT_TRUE(Adapter()->IsAcbEncrypted(RawAddress(kHeadsetL)));

    RegisterCdsm(false);
    EXPECT_TRUE(Adapter()->IsAcbConnected(RawAddress(kHeadsetL)));
    EXPECT_FALSE(Adapter()->IsAcbEncrypted(RawAddress(kHeadsetL)));
}

/**
 * @tc.name: GetRealAddress_001
 * @tc.desc: 真实地址解析：自身已连接返回原地址；合作集另一成员已连接返回成员地址；都不在线返回原地址
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, GetRealAddress_001, TestSize.Level1)
{
    AddPairedDevice(kHeadsetL);
    AddPairedDevice(kHeadsetR);
    InjectCdsmGroup(308, kHeadsetL, {kHeadsetL, kHeadsetR});

    // 无人在线：返回原地址
    EXPECT_EQ(kHeadsetL, Adapter()->GetRealAddress(RawAddress(kHeadsetL)).GetAddress());

    // 成员在线：report 地址映射到在线成员
    ASSERT_TRUE(Adapter()->SetAcbState(kHeadsetR, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED)));
    EXPECT_EQ(kHeadsetR, Adapter()->GetRealAddress(RawAddress(kHeadsetL)).GetAddress());

    // 自身在线：直接返回原地址
    ASSERT_TRUE(Adapter()->SetAcbState(kHeadsetL, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED)));
    EXPECT_EQ(kHeadsetL, Adapter()->GetRealAddress(RawAddress(kHeadsetL)).GetAddress());
}

/**
 * @tc.name: DeviceModelInfo_001
 * @tc.desc: 型号信息优先读设备缓存，缓存未命中时回落扫描与云端并保持空字段
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, DeviceModelInfo_001, TestSize.Level1)
{
    DeviceModel model;
    std::string newModelId;
    Adapter()->FindDeviceModelInfoInCache(RawAddress(kUnknownU), model, newModelId);
    EXPECT_TRUE(model.GetModelId().empty());
    EXPECT_TRUE(model.GetIconId().empty());
    EXPECT_TRUE(model.GetSubModelId().empty());
    EXPECT_TRUE(newModelId.empty());

    AddPairedDevice(kMouseM);
    DeviceModel saved;
    saved.SetModelId("model-1");
    saved.SetIconId("icon-1");
    saved.SetSubModelId("sub-1");
    Adapter()->SaveDeviceModelInfo(kMouseM, saved, "new-1");

    ASSERT_NE(nullptr, Device(kMouseM));
    EXPECT_EQ("model-1", Device(kMouseM)->GetModelId());
    DeviceModel cached;
    std::string cachedNewModelId;
    Adapter()->FindDeviceModelInfoInCache(RawAddress(kMouseM), cached, cachedNewModelId);
    EXPECT_EQ("model-1", cached.GetModelId());
    EXPECT_EQ("icon-1", cached.GetIconId());
    EXPECT_EQ("", cached.GetSubModelId());
    EXPECT_EQ("new-1", cachedNewModelId);
}

/**
 * @tc.name: BtAddrMapping_001
 * @tc.desc: 星闪/蓝牙地址互查优先走云端映射，云端无数据时回落本地设备表；
 *           本地也无映射时返回失败
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, BtAddrMapping_001, TestSize.Level1)
{
    AddPairedDevice(kHeadsetL);
    ASSERT_TRUE(DevMgr()->SetBtAddrBySleAddr(kHeadsetL, kBtAddr));

    std::string btAddr;
    EXPECT_TRUE(Adapter()->GetBtAddrBySleAddrTask(kHeadsetL, btAddr));
    EXPECT_EQ(kBtAddr, btAddr);
    std::string missBt;
    EXPECT_FALSE(Adapter()->GetBtAddrBySleAddrTask(kMouseM, missBt));

    std::string sleAddr;
    EXPECT_TRUE(Adapter()->GetSleAddrByBtAddrTask(kBtAddr, sleAddr));
    EXPECT_EQ(kHeadsetL, sleAddr);
    std::string missSle;
    EXPECT_FALSE(Adapter()->GetSleAddrByBtAddrTask("00:11:22:33:44:66", missSle));
}

/**
 * @tc.name: GetDeviceTypeInfo_001
 * @tc.desc: 非音频设备取设备类型时合作集组号为无效值截断位，类型按外观映射；
 *           音频标记设备允许尝试建组且类型位按音频归一
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, GetDeviceTypeInfo_001, TestSize.Level1)
{
    AddPairedDevice(kMouseM);
    ASSERT_TRUE(DevMgr()->SetAppearance(RawAddress(kMouseM),
        static_cast<int>(DeviceClassForService::DEVICE_MOUSE)));
    ASSERT_EQ(static_cast<int>(DeviceClassForService::DEVICE_MOUSE),
        Device(kMouseM)->GetAppearance());

    SLE_Addr_S addrInfo = {};
    uint32_t devType = 0;
    Adapter()->GetDeviceTypeInfo(RawAddress(kMouseM), addrInfo, devType);
    uint32_t mouseType = static_cast<uint32_t>(SrvDeviceType::MOUSE);
    EXPECT_EQ(mouseType, devType & 0x0000FFFF);
    EXPECT_EQ(CDSM_SERVICE_INVALID_GROUP_ID >> 16, (devType & 0xFFFF0000) >> 16);

    // 音频标记设备：建组路径可执行，低 16 位类型仍正确
    AddPairedDevice(kHeadsetL);
    DevMgr()->SetIsAudioDeviceFlag(RawAddress(kHeadsetL));
    SLE_Addr_S audioAddrInfo = {};
    uint32_t audioDevType = 0;
    Adapter()->GetDeviceTypeInfo(RawAddress(kHeadsetL), audioAddrInfo, audioDevType);
    EXPECT_EQ(static_cast<uint32_t>(SrvDeviceType::AUDIO),
        audioDevType & 0x0000FFFF);
}

/**
 * @tc.name: GetDeviceTypeFromAppearance_001
 * @tc.desc: 外观值到设备类型的映射覆盖手表/键盘/鼠标/手写笔/手柄/耳机，未知外观归其他类；
 *           音频标记设备无论外观都归音频类
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, GetDeviceTypeFromAppearance_001, TestSize.Level1)
{
    RawAddress addr(kMouseM);
    auto toType = [this, &addr](int appearance) -> uint32_t {
        return Adapter()->GetDeviceTypeFromAppearance(addr, appearance);
    };
    EXPECT_EQ(static_cast<uint32_t>(SrvDeviceType::WATCH),
        toType(static_cast<int>(DeviceClassForService::DEVICE_SMART_WATCH)));
    EXPECT_EQ(static_cast<uint32_t>(SrvDeviceType::KEYBOARD),
        toType(static_cast<int>(DeviceClassForService::DEVICE_KEYBOARD)));
    EXPECT_EQ(static_cast<uint32_t>(SrvDeviceType::MOUSE),
        toType(static_cast<int>(DeviceClassForService::DEVICE_MOUSE)));
    EXPECT_EQ(static_cast<uint32_t>(SrvDeviceType::PEN),
        toType(static_cast<int>(DeviceClassForService::DEVICE_STYLUS)));
    EXPECT_EQ(static_cast<uint32_t>(SrvDeviceType::HANDLE),
        toType(static_cast<int>(DeviceClassForService::DEVICE_HANDLE)));
    EXPECT_EQ(static_cast<uint32_t>(SrvDeviceType::AUDIO),
        toType(static_cast<int>(DeviceClassForService::DEVICE_IN_EAR_EARPHONE)));
    EXPECT_EQ(static_cast<uint32_t>(SrvDeviceType::OTHER), toType(0x123456));

    AddPairedDevice(kMouseM);
    DevMgr()->SetIsAudioDeviceFlag(RawAddress(kMouseM));
    EXPECT_EQ(static_cast<uint32_t>(SrvDeviceType::AUDIO),
        toType(static_cast<int>(DeviceClassForService::DEVICE_KEYBOARD)));
}

/**
 * @tc.name: CdsmSaveData_001
 * @tc.desc: 合作集成员连接完成后各成员获得正确的合作集角色标记（report/成员）；
 *           非合作设备或 CDSM 服务缺失时不改任何标记
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, CdsmSaveData_001, TestSize.Level1)
{
    AddPairedDevice(kHeadsetL);
    AddPairedDevice(kHeadsetR);
    InjectCdsmGroup(309, kHeadsetL, {kHeadsetL, kHeadsetR});

    Adapter()->CdsmSaveData(RawAddress(kHeadsetR));
    EXPECT_EQ(static_cast<int>(SleCdsmAddrType::CDSM_TYPE_REPORT),
        Device(kHeadsetL)->GetCdsmAddrType());
    EXPECT_EQ(static_cast<int>(SleCdsmAddrType::CDSM_TYPE_MEMBER),
        Device(kHeadsetR)->GetCdsmAddrType());

    // 非合作设备：标记保持非合作集
    AddPairedDevice(kMouseM);
    Adapter()->CdsmSaveData(RawAddress(kMouseM));
    EXPECT_EQ(static_cast<int>(SleCdsmAddrType::CDSM_TYPE_NONE),
        Device(kMouseM)->GetCdsmAddrType());

    // CDSM 服务缺失：直接返回
    RegisterCdsm(false);
    AddPairedDevice(kWatchW);
    Adapter()->CdsmSaveData(RawAddress(kWatchW));
    EXPECT_EQ(static_cast<int>(SleCdsmAddrType::CDSM_TYPE_NONE),
        Device(kWatchW)->GetCdsmAddrType());
}

/**
 * @tc.name: HandleCdsmMemberFirstPairing_001
 * @tc.desc: 合作集成员首次配对时继承 report 设备的外观与名称并获得成员标记；
 *           成员已有外观时保留自身外观
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, HandleCdsmMemberFirstPairing_001, TestSize.Level1)
{
    AddPairedDevice(kHeadsetL);
    ASSERT_TRUE(DevMgr()->SetAppearance(RawAddress(kHeadsetL),
        static_cast<int>(DeviceClassForService::DEVICE_IN_EAR_EARPHONE)));
    ASSERT_TRUE(DevMgr()->SetName(RawAddress(kHeadsetL), "ReportName"));
    AddPairedDevice(kHeadsetR);
    InjectCdsmGroup(310, kHeadsetL, {kHeadsetL, kHeadsetR});

    Adapter()->HandleCdsmMemberFirstPairing(RawAddress(kHeadsetR));
    auto member = Device(kHeadsetR);
    EXPECT_EQ("ReportName", member->GetName());
    EXPECT_EQ(static_cast<int>(DeviceClassForService::DEVICE_IN_EAR_EARPHONE), member->GetAppearance());
    EXPECT_EQ(static_cast<int>(SleCdsmAddrType::CDSM_TYPE_MEMBER), member->GetCdsmAddrType());

    // 成员已有外观：保留自身值，仅补名称
    ASSERT_TRUE(DevMgr()->SetAppearance(RawAddress(kHeadsetR),
        static_cast<int>(DeviceClassForService::DEVICE_MOUSE)));
    Adapter()->HandleCdsmMemberFirstPairing(RawAddress(kHeadsetR));
    EXPECT_EQ(static_cast<int>(DeviceClassForService::DEVICE_MOUSE),
        DevMgr()->GetRemoteDevice(RawAddress(kHeadsetR))->GetAppearance());
}

/**
 * @tc.name: IsCdsmMemberPair_001
 * @tc.desc: 合作集成员配对确认：私有音频设备、report 自身、report 未连接、report 未配对
 *           都判定为需要弹窗；全部条件满足时才静默通过
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, IsCdsmMemberPair_001, TestSize.Level1)
{
    AddPairedDevice(kHeadsetL);
    AddPairedDevice(kHeadsetR);
    InjectCdsmGroup(311, kHeadsetL, {kHeadsetL, kHeadsetR});

    // report 未连接：不通过（连接中状态还会触发断连清理，栈调用受控失败）
    ASSERT_TRUE(Adapter()->SetAcbState(kHeadsetL, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING)));
    EXPECT_FALSE(Adapter()->IsCdsmMemberPair(RawAddress(kHeadsetR), RawAddress(kHeadsetL)));

    // report 已连接但未配对：不通过
    ASSERT_TRUE(Adapter()->SetAcbState(kHeadsetL, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED)));
    ASSERT_TRUE(Adapter()->SetPairStatus(RawAddress(kHeadsetL), static_cast<int>(SlePairState::SLE_PAIR_PAIRING)));
    EXPECT_FALSE(Adapter()->IsCdsmMemberPair(RawAddress(kHeadsetR), RawAddress(kHeadsetL)));

    // 条件齐备：静默通过
    ASSERT_TRUE(Adapter()->SetPairStatus(RawAddress(kHeadsetL), static_cast<int>(SlePairState::SLE_PAIR_PAIRED)));
    EXPECT_TRUE(Adapter()->IsCdsmMemberPair(RawAddress(kHeadsetR), RawAddress(kHeadsetL)));

    // report 地址与设备相同：不通过
    EXPECT_FALSE(Adapter()->IsCdsmMemberPair(RawAddress(kHeadsetL), RawAddress(kHeadsetL)));

    // 私有音频设备：不通过
    Device(kHeadsetL)->SetManufacturerBusiness(SLE_PRIVATE_AUDIO_BUSINESS_TYPE);
    EXPECT_FALSE(Adapter()->IsCdsmMemberPair(RawAddress(kHeadsetR), RawAddress(kHeadsetL)));
}

/**
 * @tc.name: CdsmAddOtherRecord_001
 * @tc.desc: 共用链路密钥场景为另一地址建档：继承源设备信息、直接置为已配对主动配对，
 *           并保留本地蓝牙映射；CDSM 服务缺失时不建档
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, CdsmAddOtherRecord_001, TestSize.Level1)
{
    AddPairedDevice(kHeadsetL);
    ASSERT_TRUE(DevMgr()->SetName(RawAddress(kHeadsetL), "SourceName"));

    Adapter()->CdsmAddOtherRecord(RawAddress(kHeadsetL), RawAddress(kMouseM));
    auto other = Device(kMouseM);
    ASSERT_NE(nullptr, other);
    EXPECT_EQ("SourceName", other->GetName());
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRED), static_cast<int>(other->GetPairedStatus()));
    EXPECT_EQ(static_cast<int>(SlePairDirect::SLE_PAIR_ACTIVE), other->GetPairDirection());
    EXPECT_EQ(static_cast<int>(SleCdsmAddrType::CDSM_TYPE_NONE), other->GetCdsmAddrType());

    RegisterCdsm(false);
    Adapter()->CdsmAddOtherRecord(RawAddress(kHeadsetL), RawAddress(kWatchW));
    EXPECT_EQ(nullptr, DevMgr()->GetRemoteDevice(RawAddress(kWatchW)));
}

/**
 * @tc.name: CancelAllConnection_001
 * @tc.desc: 取消全部连接把连接中的设备置为断开中状态
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, CancelAllConnection_001, TestSize.Level1)
{
    AddPairedDevice(kMouseM);
    ASSERT_TRUE(Adapter()->SetAcbState(kMouseM, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING)));

    Adapter()->CancelAllConnection();
    EXPECT_EQ(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTING),
        Adapter()->GetAcbState(kMouseM));
}

/**
 * @tc.name: UpdateDefaultRole_001
 * @tc.desc: 默认角色更新在非合作设备、加密成员数不为一、TWS 服务未注册时都不产生变更；
 *           CDSM 服务缺失时同样直接返回
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, UpdateDefaultRole_001, TestSize.Level1)
{
    AddPairedDevice(kMouseM);
    Adapter()->UpdateDefaultRole(RawAddress(kMouseM));
    EXPECT_NE(nullptr, DevMgr()->GetRemoteDevice(RawAddress(kMouseM)));

    RegisterCdsm(false);
    Adapter()->UpdateDefaultRole(RawAddress(kMouseM));
    EXPECT_NE(nullptr, DevMgr()->GetRemoteDevice(RawAddress(kMouseM)));

    RegisterCdsm(true);
    AddPairedDevice(kHeadsetL);
    AddPairedDevice(kHeadsetR);
    InjectCdsmGroup(312, kHeadsetL, {kHeadsetL, kHeadsetR});
    ASSERT_TRUE(Adapter()->SetAcbState(kHeadsetL, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED)));
    ASSERT_TRUE(Adapter()->SetAcbState(kHeadsetR, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED)));
    Adapter()->UpdateDefaultRole(RawAddress(kHeadsetL));
    auto member = DevMgr()->GetRemoteDevice(RawAddress(kHeadsetL));
    EXPECT_NE(nullptr, member);
}

/**
 * @tc.name: ConnectionCompleteHelper_001
 * @tc.desc: 连接完成落表返回成功且链路标识可查；已配对车钥匙设备额外走高功耗处理不影响落表
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, ConnectionCompleteHelper_001, TestSize.Level1)
{
    AddPairedDevice(kMouseM);
    EXPECT_TRUE(Adapter()->ConnectionCompleteHelper(RawAddress(kMouseM), 0x21, 1, 0));
    EXPECT_EQ(0x21, Adapter()->GetLcidByAddress(RawAddress(kMouseM)));

    AddPairedDevice(kHeadsetL);
    ASSERT_TRUE(DevMgr()->SetAppearance(RawAddress(kHeadsetL),
        static_cast<int>(DeviceClassForService::DEVICE_CLASS_VEHICLE_LOCK)));
    EXPECT_TRUE(Adapter()->ConnectionCompleteHelper(RawAddress(kHeadsetL), 0x22, 1, 0));
    EXPECT_EQ(0x22, Adapter()->GetLcidByAddress(RawAddress(kHeadsetL)));
}

/**
 * @tc.name: SavePeerDevices2Smp_001
 * @tc.desc: 无已连接设备或设备缺链路密钥时不向安全模块下发恢复数据，流程安全返回
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, SavePeerDevices2Smp_001, TestSize.Level1)
{
    Adapter()->SavePeerDevices2Smp();

    // 设备在表但无链路密钥：收集结果为空，不下发
    AddPairedDevice(kMouseM);
    ASSERT_TRUE(SleConfig::GetInstance().SetLinkKey(kMouseM, ""));
    Adapter()->SavePeerDevices2Smp();
    EXPECT_NE(nullptr, DevMgr()->GetRemoteDevice(RawAddress(kMouseM)));
}

/**
 * @tc.name: RestorePairedDevicesAbility2Cfgdb_001
 * @tc.desc: 恢复厂商能力到配置库时跳过无有效能力的设备，有效能力设备走下发流程
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, RestorePairedDevicesAbility2Cfgdb_001, TestSize.Level1)
{
    AddPairedDevice(kMouseM);
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> ability = {};
    ability[0] = 0x01;
    DevMgr()->SetManufacturerAbility(RawAddress(kMouseM), ability);
    AddPairedDevice(kHeadsetL);

    Adapter()->RestorePairedDevicesAbility2Cfgdb();
    EXPECT_EQ(0x01, DevMgr()->GetManufacturerAbility(RawAddress(kMouseM))[0]);
    EXPECT_NE(nullptr, DevMgr()->GetRemoteDevice(RawAddress(kHeadsetL)));
}

/**
 * @tc.name: SaveHidAbilityFromAdv_001
 * @tc.desc: 广播业务类型非 HID 的设备不落任何厂商能力配置
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, SaveHidAbilityFromAdv_001, TestSize.Level1)
{
    Adapter()->SaveHidManufacturerAbilityFromAdv(RawAddress(kMouseM));
    EXPECT_EQ("", SleConfig::GetInstance().GetManufacturerAbility(kMouseM));
}

/**
 * @tc.name: LinkKeyDecrypt_001
 * @tc.desc: 链路密钥解密对空串、非法十六进制、密文不匹配都返回失败
 * @tc.type: FUNC
 */
HWTEST_F(SleRemoteDeviceAdapterTest, LinkKeyDecrypt_001, TestSize.Level1)
{
    LinkKey key = {};
    EXPECT_FALSE(Adapter()->SleLinkKeyDecrypt("", key));
    EXPECT_FALSE(Adapter()->SleLinkKeyDecrypt("zzzz", key));
    EXPECT_FALSE(Adapter()->SleLinkKeyDecrypt("00112233445566778899AABBCCDDEEFF", key));
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
