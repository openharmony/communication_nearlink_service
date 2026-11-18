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
 * 被测对象 SleAdapter 与其依赖（合作集服务、配置、远程设备表、扫描缓存、云配对缓存）
 * 均为真实例，仅注入内存数据；SM 下行接口用成功桩（mock_nlstk_sm.cpp）。
 * 运行约束：仅在专用测试机执行——用例会发布 nearlink 公共事件并触碰配置文件（TearDown 恢复）；
 * 恢复出厂设置用例会清空整机配对配置并删除设备档案文件，超出夹具恢复能力；
 * 建议星闪服务停用后运行，合成地址假定不与真实配对设备冲突。
 * 测试编译单元包含 4 个产品源文件以访问内部注册表与状态（已从共享构建源排除）。
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "CdsmService.h"
#include "SleCloudPairService.h"
#include "SleConfig.h"
#include "SleInterfaceAdapterSub.h"
#include "SleInterfaceManager.h"
#include "SleProfileConnectManager.h"
#include "SleReconnectManager.h"
#include "SleServiceManager.h"
#include "ThreadUtil.h"
#include "interface_cloud_pair_service.h"
#include "log.h"
#include "nearlink_def.h"
#include "nearlink_access_token_mock.h"
#include "parameters.h"
#include "sle_service_data.h"

#include "ProfileServiceManager.cpp"
#include "SleServiceManager.cpp"
#include "ScanService.cpp"
#include "SleAdapter.cpp"

namespace OHOS {
namespace Nearlink {
namespace TEST {

using namespace testing::ext;

// 全部为合成地址，不与真实设备冲突（report/副耳/鼠标/车锁/云设备各取一组）
const std::string kHeadsetR = "A1:B2:C3:D4:E5:01";
const std::string kHeadsetL = "A1:B2:C3:D4:E5:02";
const std::string kNewEarL = "A1:B2:C3:D4:E5:03";
const std::string kMouseM = "B2:C3:D4:E5:F6:03";
const std::string kVehicle = "C3:D4:E5:F6:07:04";
const std::string kCloudR = "D4:E5:F6:07:18:05";
const std::string kCloudM = "D4:E5:F6:07:18:06";
const std::string kPlainA = "E5:F6:07:18:29:07";
const std::string kPlainB = "F6:07:18:29:3A:08";
const std::string kUnknownU = "CC:DD:EE:FF:00:99";
const std::string kBtAddr = "00:11:22:33:44:55";
constexpr uint32_t kTokenLen = 4;

// 捕获上报事件的观察者，断言全部落在真实上报内容上
class TestPeripheralCallback : public ISlePeripheralCallback {
public:
    struct PairEvent {
        RawAddress device;
        int preStatus;
        int status;
        int reason;
    };
    struct ConnEvent {
        RawAddress device;
        int state;
        int preState;
        int reason;
    };

    void OnPairStatusChanged(const RawAddress &device, int preStatus, int status, int reason) override
    {
        pairEvents_.push_back({device, preStatus, status, reason});
    }

    void OnCsdmPairStateChanged(const RawAddress &device, int state) override
    {
        csdmPairEvents_.push_back({device, state});
    }

    void OnAcbStateChanged(const RawAddress &device, int state, int reason) override
    {
        acbEvents_.push_back({device, state, reason});
    }

    void OnConnectionStateChanged(const RawAddress &device, int state, int preState, int reason) override
    {
        connEvents_.push_back({device, state, preState, reason});
    }

    void OnPairingRequest(const RawAddress &device, const std::string &passkey, int32_t type) override
    {
        pairingReqEvents_.push_back(device);
    }

    void Reset()
    {
        pairEvents_.clear();
        csdmPairEvents_.clear();
        acbEvents_.clear();
        connEvents_.clear();
        pairingReqEvents_.clear();
    }

    std::vector<PairEvent> pairEvents_;
    std::vector<std::pair<RawAddress, int>> csdmPairEvents_;
    std::vector<ConnEvent> acbEvents_;
    std::vector<ConnEvent> connEvents_;
    std::vector<RawAddress> pairingReqEvents_;
};

class SleAdapterServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;

protected:
    SleAdapter &Adapter()
    {
        return *adapter_;
    }

    CdsmService &Cdsm()
    {
        return *cdsmService_;
    }

    // 构造一台远程设备并注入设备表：配对状态/连接状态/朝向/外观等由入参决定
    void AddRemoteDevice(const std::string &addr, int pairState, int acbState,
        int connDirect = static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE), int appearance = 0,
        bool isAudio = false, int businessType = 0, int prePairState = -1, uint16_t lcid = INVALID_LCID);

    // 内存建合作集：report 上报地址 + 成员列表（成员状态默认断连）
    void InjectCdsmGroup(uint32_t groupId, const std::string &reportAddr, const std::vector<std::string> &members);

    // 预置扫描缓存：业务类型/协作地址/当前地址决定私有音频合作集的建组走向
    void InjectScanResult(const std::string &addr, int businessType, const std::string &collaborateAddr = "",
        const std::string &currentAddr = "");

    // 下云一台双耳云设备并置云配对状态（真实云缓存链路）
    void InjectCloudDevice(const std::string &reportAddr, const std::string &memberAddr, int cloudPairState);

    void SetSleSwitchState(SleStateID state);

    // 构造底层连接回调入参
    static CM_LogicLinkState_S MakeConnResult(const std::string &addr, uint8_t result, uint16_t lcid,
        uint8_t role, uint8_t reason = 0);

    static SleAdapter *adapter_;
    static TestPeripheralCallback callback_;

    std::unique_ptr<CdsmService> cdsmService_;
    std::vector<std::string> testAddrs_;
    std::vector<std::string> savedPairedList_;
    std::string savedReconnList_;
    SleStateID savedState_;
    SleAutoConnectPolicy savedPolicy_;
};

SleAdapter *SleAdapterServiceTest::adapter_ = nullptr;
TestPeripheralCallback SleAdapterServiceTest::callback_;

void SleAdapterServiceTest::SetUpTestCase()
{
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    // 只创建 adapter 对象，不启动协议栈固件——测试进程没有固件运行环境
    SleServiceManager::GetInstance()->CreateAdapters();
    adapter_ = static_cast<SleAdapter *>(SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    ASSERT_NE(nullptr, adapter_);
    SleConfig::GetInstance().LoadConfigInfo();
    adapter_->RegisterSlePeripheralCallback(callback_);
}

void SleAdapterServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase SleAdapterServiceTest");
}

void SleAdapterServiceTest::SetUp()
{
    HILOGI("SetUp SleAdapterServiceTest");
    // 被测链路的全部线程内联执行（adapter/设备表/cdsm/扫描/服务管理/回连等）
    for (int i = THREAD_ID_DIS; i < THREAD_ID_BUTT; ++i) {
        ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(i, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    }

    testAddrs_ = {kHeadsetR, kHeadsetL, kNewEarL, kMouseM, kVehicle, kCloudR, kCloudM, kPlainA, kPlainB, kUnknownU};
    savedPairedList_ = SleConfig::GetInstance().GetPairedAddrList();
    savedReconnList_ = SleConfig::GetInstance().GetReconnectDeviceAddressList();
    savedState_ = SleServiceManager::GetInstance()->GetState(SleTransport::ADAPTER_SLE);
    savedPolicy_ = SleServiceManager::GetInstance()->autoConnPolicy_.load();

    // 挂入真实 CDSM 实例，CdsmService::GetService() 与 GetProfileService 两条查询路径同时命中
    cdsmService_ = std::make_unique<CdsmService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CDSM, cdsmService_.get());
    ASSERT_EQ(cdsmService_.get(), CdsmService::GetService());

    // 清空本套件会触碰的进程态
    SleRemoteDeviceAdapter::GetInstance()->RemoveAllPeripheralDevices();
    // 扫描缓存按地址键控留存，跨用例残留会改变按序查找的命中项
    static_cast<ScanService &>(InterfaceScanService::GetInstance()).pimpl->sleScanResults_.Clear();
    SleReconnectManager::GetInstance().pairedQueue_.clear();
    SleReconnectManager::GetInstance().inBgListDevices_.clear();
    SleReconnectManager::GetInstance().bgConnMaxNum_ = RECONNECT_ACCEPT_LIST_MAX_SIZE;
    SleCoexistManager::GetInstance()->ClearAll();
    InterfaceCloudPairService::GetInstance().ClearCloudDeviceMap(false);
    auto &pimpl = *adapter_->pimpl;
    pimpl.credibleDevice_.Clear();
    pimpl.needReconnectDevices_.Clear();
    pimpl.needReportPairDevices_.Clear();
    pimpl.addrAndFrameTypeMap_.Clear();
    pimpl.acbCount_.store(0);
    pimpl.duration_ = 0;
    pimpl.btmEnableFlag_ = false;
    pimpl.sleDisconnReason_.store(static_cast<int>(SleConnectReason::CONNECT_NONE));
    pimpl.sleConnectableHandle_.store(
        static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE));
    if (pimpl.sleConnectableTimer_ != nullptr) {
        pimpl.sleConnectableTimer_->Stop();
        pimpl.sleConnectableTimer_ = nullptr;
    }
    if (pimpl.slePassivePairingDialogTimer_ != nullptr) {
        pimpl.slePassivePairingDialogTimer_->Stop();
        pimpl.slePassivePairingDialogTimer_ = nullptr;
    }
    pimpl.passivePairingSsapConnInst_ = nullptr;
    pimpl.passivePairingDialogFunc_ = nullptr;
    callback_.Reset();
}

void SleAdapterServiceTest::TearDown()
{
    HILOGI("TearDown SleAdapterServiceTest");
    // 只清理本套件新建的合成地址子节，不动快照中已存在的
    for (const auto &addr : testAddrs_) {
        if (std::find(savedPairedList_.begin(), savedPairedList_.end(), addr) == savedPairedList_.end()) {
            SleConfig::GetInstance().RemovePairedDevice(addr);
            SleConfig::GetInstance().RemoveCdsmGroup(addr);
        }
    }
    SleConfig::GetInstance().SetReconnectDeviceAddressList(savedReconnList_);
    SleConfig::GetInstance().Save();
    InterfaceCloudPairService::GetInstance().ClearCloudDeviceMap(false);
    SleRemoteDeviceAdapter::GetInstance()->RemoveAllPeripheralDevices();
    static_cast<ScanService &>(InterfaceScanService::GetInstance()).pimpl->sleScanResults_.Clear();

    auto &pimpl = *adapter_->pimpl;
    pimpl.credibleDevice_.Clear();
    pimpl.needReconnectDevices_.Clear();
    pimpl.needReportPairDevices_.Clear();
    pimpl.addrAndFrameTypeMap_.Clear();
    pimpl.acbCount_.store(0);
    pimpl.sleDisconnReason_.store(static_cast<int>(SleConnectReason::CONNECT_NONE));
    if (pimpl.sleConnectableTimer_ != nullptr) {
        pimpl.sleConnectableTimer_->Stop();
        pimpl.sleConnectableTimer_ = nullptr;
    }
    if (pimpl.slePassivePairingDialogTimer_ != nullptr) {
        pimpl.slePassivePairingDialogTimer_->Stop();
        pimpl.slePassivePairingDialogTimer_ = nullptr;
    }
    pimpl.passivePairingSsapConnInst_ = nullptr;
    pimpl.passivePairingDialogFunc_ = nullptr;
    SleServiceManager::GetInstance()->autoConnPolicy_ = savedPolicy_;
    SleServiceManager::GetInstance()->pimpl->state_.store(savedState_);
    SleReconnectManager::GetInstance().bgConnMaxNum_ = RECONNECT_ACCEPT_LIST_MAX_SIZE;

    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CDSM, nullptr);
    cdsmService_.reset();
    SleCoexistManager::GetInstance()->ClearAll();
    SleReconnectManager::GetInstance().pairedQueue_.clear();
    SleReconnectManager::GetInstance().inBgListDevices_.clear();
    ThreadUtil::GetInstance().InitThreadStateMap();
    callback_.Reset();
}

void SleAdapterServiceTest::AddRemoteDevice(const std::string &addr, int pairState, int acbState,
    int connDirect, int appearance, bool isAudio, int businessType, int prePairState, uint16_t lcid)
{
    auto device = std::make_shared<SlePeripheralDevice>();
    device->SetAddress(RawAddress(addr));
    device->SetPairedStatus(pairState);
    device->SetPrePairedStatus(prePairState < 0 ? pairState : prePairState);
    device->SetAcbConnectState(acbState);
    device->SetConnDirect(connDirect);
    device->SetAppearance(appearance);
    device->SetIsAudioDeviceFlag(isAudio);
    device->SetManufacturerBusiness(businessType);
    device->SetLcid(lcid);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(addr, device);
}

void SleAdapterServiceTest::InjectCdsmGroup(uint32_t groupId, const std::string &reportAddr,
    const std::vector<std::string> &members)
{
    auto group = std::make_shared<CdsmInfo>(groupId, reportAddr);
    group->memberCnt_ = members.size();
    for (const auto &member : members) {
        group->CdsmAddMemberInfo(member, static_cast<uint8_t>(SleConnectState::DISCONNECTED));
    }
    Cdsm().cdsmList_.EnsureInsert(groupId, group);
}

void SleAdapterServiceTest::InjectScanResult(const std::string &addr, int businessType,
    const std::string &collaborateAddr, const std::string &currentAddr)
{
    auto &scan = static_cast<ScanService &>(InterfaceScanService::GetInstance());
    SlePeripheralDevice peripheral;
    peripheral.SetAddress(RawAddress(addr));
    peripheral.SetManufacturerBusiness(businessType);
    if (!collaborateAddr.empty()) {
        peripheral.SetCollaborateAddress(RawAddress(collaborateAddr));
    }
    peripheral.SetCurrentRawAddress(RawAddress(currentAddr.empty() ? addr : currentAddr));
    SleScanResultImpl result;
    result.SetPeripheralDevice(peripheral);
    scan.pimpl->sleScanResults_.EnsureInsert(addr, result);
}

void SleAdapterServiceTest::InjectCloudDevice(const std::string &reportAddr, const std::string &memberAddr,
    int cloudPairState)
{
    // 直插云缓存表：真实下云链路会按合作集一致性做游离态分类，
    // 分类走向与本套件的被测分支无关，内存注入保证云态确定可控
    auto downDevice = std::make_shared<DownCloudPairDevice>();
    downDevice->SetBtAddr(kBtAddr);
    downDevice->SetDeviceName("cloudEar");
    downDevice->SetToken(std::vector<uint8_t>(kTokenLen, 0x01));
    downDevice->SetReportAddr(reportAddr);
    downDevice->SetMembersAddr({reportAddr, memberAddr});
    auto &cloud = static_cast<SleCloudPairService &>(InterfaceCloudPairService::GetInstance());
    cloud.cloudDevicesMap_.EnsureInsert(reportAddr, downDevice);
    downDevice->SetCloudPairState(cloudPairState);
}

void SleAdapterServiceTest::SetSleSwitchState(SleStateID state)
{
    SleServiceManager::GetInstance()->pimpl->state_.store(state);
}

CM_LogicLinkState_S SleAdapterServiceTest::MakeConnResult(const std::string &addr, uint8_t result, uint16_t lcid,
    uint8_t role, uint8_t reason)
{
    CM_LogicLinkState_S connResult = {};
    RawAddress(addr).ConvertToUint8(connResult.addr.addr);
    connResult.addr.type = 0;
    connResult.result = result;
    connResult.lcid = lcid;
    connResult.role = role;
    connResult.discReason = reason;
    return connResult;
}

// ==================== 合作集生命周期 ====================

/**
 * @tc.name: SleAdapter_CdsmGroup_001
 * @tc.desc: 私有音频设备首次配对时按扫描到的双耳信息走建组流程，设备进入配对中
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_CdsmGroup_001, TestSize.Level1)
{
    InjectScanResult(kHeadsetR, SLE_PRIVATE_AUDIO_BUSINESS_TYPE, kHeadsetL);
    InjectCloudDevice(kCloudR, kCloudM, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID);

    EXPECT_TRUE(Adapter().StartPair(RawAddress(kHeadsetR)));

    // 云设备地址与本次配对地址无关，不拦截建组流程；设备进入配对中
    // 建组出口依赖协议栈建组接口，测试进程无栈不落内存表，组内容不作断言
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        SleRemoteDeviceAdapter::GetInstance()->GetPairStatus(RawAddress(kHeadsetR)));
}

/**
 * @tc.name: SleAdapter_CdsmGroup_002
 * @tc.desc: 副耳换新后重新配对：旧副耳按丢失处理转入取消配对，旧合作集整组删除
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_CdsmGroup_002, TestSize.Level1)
{
    // 既有组：report=右耳，成员含左耳；本次配对右耳时扫描到的协作地址是一副新左耳
    InjectCdsmGroup(201, kHeadsetR, {kHeadsetR, kHeadsetL});
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    InjectScanResult(kHeadsetR, SLE_PRIVATE_AUDIO_BUSINESS_TYPE, kNewEarL);

    EXPECT_TRUE(Adapter().StartPair(RawAddress(kHeadsetR)));

    // 旧左耳按丢失处理转为取消配对中，右耳进入重新配对；旧组已整组删除，
    // 新组出口依赖协议栈建组接口（测试进程无栈不落内存表），组表保持为空
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_CANCELING),
        SleRemoteDeviceAdapter::GetInstance()->GetPairStatus(RawAddress(kHeadsetL)));
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        SleRemoteDeviceAdapter::GetInstance()->GetPairStatus(RawAddress(kHeadsetR)));
    EXPECT_EQ(0, Cdsm().cdsmList_.Size());
}

/**
 * @tc.name: SleAdapter_CdsmGroup_003
 * @tc.desc: 双耳互换后重新配对：旧 report 地址失效，配对状态归零并上报取消，上报地址替换为新 report
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_CdsmGroup_003, TestSize.Level1)
{
    // 旧组：report=右耳；本次从左耳发起配对，扫描侧协作地址指向右耳（双耳互换）
    InjectCdsmGroup(202, kHeadsetR, {kHeadsetR, kHeadsetL});
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    InjectScanResult(kHeadsetL, SLE_PRIVATE_AUDIO_BUSINESS_TYPE, kHeadsetR);

    EXPECT_TRUE(Adapter().StartPair(RawAddress(kHeadsetL)));

    // 旧 report（右耳）收到取消配对上报，两只耳机的设备记录都被移除
    ASSERT_FALSE(callback_.pairEvents_.empty());
    EXPECT_EQ(RawAddress(kHeadsetR), callback_.pairEvents_.front().device);
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_CANCELING), callback_.pairEvents_.front().preStatus);
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_NONE), callback_.pairEvents_.front().status);
    EXPECT_EQ(nullptr, SleRemoteDeviceAdapter::GetInstance()->GetRemoteDevice(RawAddress(kHeadsetR)));
    // 右耳设备记录被移除后，左耳按新 report 重新发起配对
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        SleRemoteDeviceAdapter::GetInstance()->GetPairStatus(RawAddress(kHeadsetL)));
    // report 地址替换后按新 report（左耳）入回连队列
    ASSERT_FALSE(SleReconnectManager::GetInstance().pairedQueue_.empty());
    EXPECT_EQ(kHeadsetL, SleReconnectManager::GetInstance().pairedQueue_.back().reportAddr_);
}

/**
 * @tc.name: SleAdapter_CdsmGroup_004
 * @tc.desc: 公版耳机无合作集信息时配对直接走普通流程，不创建也不清理任何合作集
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_CdsmGroup_004, TestSize.Level1)
{
    InjectScanResult(kMouseM, 0);
    EXPECT_TRUE(Adapter().StartPair(RawAddress(kMouseM)));
    EXPECT_EQ(0U, Cdsm().cdsmList_.Size());
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        SleRemoteDeviceAdapter::GetInstance()->GetPairStatus(RawAddress(kMouseM)));
}

/**
 * @tc.name: SleAdapter_CdsmGroup_005
 * @tc.desc: 云设备处于建_pair态时配对不建合作集，状态直接按已配对上报
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_CdsmGroup_005, TestSize.Level1)
{
    InjectCloudDevice(kCloudR, kCloudM, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR);

    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRED), Adapter().GetPairState(RawAddress(kCloudR)));
    EXPECT_EQ(0U, Cdsm().cdsmList_.Size());
}

/**
 * @tc.name: SleAdapter_CdsmGroup_006
 * @tc.desc: 确认配对走回连流程时保留设备缓存：状态先置未配对再重新置为配对中并等待上报
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_CdsmGroup_006, TestSize.Level1)
{
    // 旧组 report=右耳，从左耳确认配对（音频设备走回连建组路径）；GetRealAddress 不做
    // 成员映射（对端未连接时返回原址），扫描项以左耳自身为当前地址
    InjectCdsmGroup(203, kHeadsetR, {kHeadsetR, kHeadsetL});
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED),
        static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE), 0, true);
    InjectScanResult(kHeadsetL, SLE_PRIVATE_AUDIO_BUSINESS_TYPE, kHeadsetR, kHeadsetL);

    // 设备未连接 ACB，确认最终失败，但建组副作用已发生
    EXPECT_FALSE(Adapter().SetPairingConfirmation(RawAddress(kHeadsetL)));

    EXPECT_EQ(1U, Adapter().pimpl->needReportPairDevices_.Size());
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        SleRemoteDeviceAdapter::GetInstance()->GetPairStatus(RawAddress(kHeadsetL)));
    // 设备缓存保留（回连流程不删缓存）
    EXPECT_NE(nullptr, SleRemoteDeviceAdapter::GetInstance()->GetRemoteDevice(RawAddress(kHeadsetL)));
}

/**
 * @tc.name: SleAdapter_CdsmGroup_007
 * @tc.desc: 全员空闲（无瞬态连接、无人持有配对密钥）时清理合作集；仍有成员持有密钥时保留
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_CdsmGroup_007, TestSize.Level1)
{
    InjectCdsmGroup(204, kHeadsetR, {kHeadsetR, kHeadsetL});
    Adapter().ClearAllCdsmData(RawAddress(kHeadsetR));
    EXPECT_EQ(0U, Cdsm().cdsmList_.Size());

    // 成员仍有 linkKey：不清理
    InjectCdsmGroup(205, kHeadsetR, {kHeadsetR, kHeadsetL});
    ASSERT_TRUE(SleConfig::GetInstance().SetLinkKey(kHeadsetL, "AABBCCDDEEFF"));
    Adapter().ClearAllCdsmData(RawAddress(kHeadsetR));
    EXPECT_EQ(1U, Cdsm().cdsmList_.Size());

    // 成员处于连接瞬态：不清理
    ASSERT_TRUE(SleConfig::GetInstance().RemovePairedDevice(kHeadsetL));
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING));
    Adapter().ClearAllCdsmData(RawAddress(kHeadsetR));
    EXPECT_EQ(1U, Cdsm().cdsmList_.Size());
}

/**
 * @tc.name: SleAdapter_CdsmGroup_008
 * @tc.desc: 密钥丢失时：非音频设备仅置为配对中不处理合作集；音频 report 设备触发对端副耳断连
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_CdsmGroup_008, TestSize.Level1)
{
    // 非音频：状态置 PAIRING 后直接返回，组不动
    AddRemoteDevice(kMouseM, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    Adapter().EncryptionKeyMissingComplete(RawAddress(kMouseM));
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        SleRemoteDeviceAdapter::GetInstance()->GetPairStatus(RawAddress(kMouseM)));
    EXPECT_EQ(0U, Cdsm().cdsmList_.Size());

    // 音频 report 设备：副耳被取消配对记录并断连
    InjectCdsmGroup(206, kHeadsetR, {kHeadsetR, kHeadsetL});
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED), 
        static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE), 0, true);
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    Adapter().pimpl->needReconnectDevices_.Emplace(kHeadsetL);
    Adapter().EncryptionKeyMissingComplete(RawAddress(kHeadsetR));
    EXPECT_EQ(0U, Adapter().pimpl->needReconnectDevices_.Count(kHeadsetL));
}

/**
 * @tc.name: SleAdapter_CdsmGroup_009
 * @tc.desc: 密钥丢失重建：扫描侧副耳与合作集副耳一致时判定非丢失场景，原组原记录保持不变
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_CdsmGroup_009, TestSize.Level1)
{
    InjectCdsmGroup(207, kHeadsetR, {kHeadsetR, kHeadsetL});
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED),
        static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE), 0, true);
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    // 扫描项以右耳为当前地址、左耳（合作集副耳）为广播 report，命中"副耳一致"等值臂
    InjectScanResult(kHeadsetL, SLE_PRIVATE_AUDIO_BUSINESS_TYPE, kHeadsetR, kHeadsetR);

    Adapter().UpdateKeyMissingCdsmGroup(RawAddress(kHeadsetR));

    // 扫描 report 即合作集副耳：非丢失场景，不删组、不动配对记录
    EXPECT_EQ(1U, Cdsm().cdsmList_.Size());
    EXPECT_NE(nullptr, SleRemoteDeviceAdapter::GetInstance()->GetRemoteDevice(RawAddress(kHeadsetL)));
}

// ==================== 配对状态与取消配对 ====================

/**
 * @tc.name: SleAdapter_Pair_001
 * @tc.desc: 已配对设备取消配对：状态迁移到取消中，合作集成员被一并取消，密钥从配置移除
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Pair_001, TestSize.Level1)
{
    InjectCdsmGroup(301, kHeadsetR, {kHeadsetR, kHeadsetL});
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));

    EXPECT_TRUE(Adapter().CancelPairing(RawAddress(kHeadsetR)));

    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_CANCELING),
        SleRemoteDeviceAdapter::GetInstance()->GetPairStatus(RawAddress(kHeadsetR)));
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_CANCELING),
        SleRemoteDeviceAdapter::GetInstance()->GetPairStatus(RawAddress(kHeadsetL)));
}

/**
 * @tc.name: SleAdapter_Pair_002
 * @tc.desc: 未配对或已在取消中的设备再次取消配对直接失败，状态不变
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Pair_002, TestSize.Level1)
{
    AddRemoteDevice(kMouseM, static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    EXPECT_FALSE(Adapter().CancelPairing(RawAddress(kMouseM)));
    EXPECT_EQ(nullptr, SleRemoteDeviceAdapter::GetInstance()->GetRemoteDevice(RawAddress(kUnknownU)));
    EXPECT_FALSE(Adapter().CancelPairing(RawAddress(kUnknownU)));
}

/**
 * @tc.name: SleAdapter_Pair_003
 * @tc.desc: 云设备取消配对走云端链路：本地成员配对记录被清理并上报已取消
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Pair_003, TestSize.Level1)
{
    InjectCloudDevice(kCloudR, kCloudM, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR);
    InjectCdsmGroup(302, kCloudR, {kCloudR, kCloudM});
    AddRemoteDevice(kCloudR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    Adapter().pimpl->credibleDevice_.Insert(kCloudR);

    EXPECT_TRUE(Adapter().CancelPairing(RawAddress(kCloudR)));

    EXPECT_EQ(nullptr, SleRemoteDeviceAdapter::GetInstance()->GetRemoteDevice(RawAddress(kCloudR)));
    EXPECT_FALSE(Adapter().pimpl->credibleDevice_.Find(
        [](const std::string &addr) { return addr == kCloudR; }));
    ASSERT_FALSE(callback_.pairEvents_.empty());
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_NONE), callback_.pairEvents_.back().status);
}

/**
 * @tc.name: SleAdapter_Pair_004
 * @tc.desc: 配对成功：配对中状态迁移为已配对，连接朝向按上次姿态保存
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Pair_004, TestSize.Level1)
{
    AddRemoteDevice(kMouseM, static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED),
        static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE));

    Adapter().PairComplete(RawAddress(kMouseM), SM_PAIR_OK);

    auto remote = SleRemoteDeviceAdapter::GetInstance()->GetRemoteDevice(RawAddress(kMouseM));
    ASSERT_NE(nullptr, remote);
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRED), remote->GetPairedStatus());
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRING), remote->GetPrePairedStatus());
    ASSERT_FALSE(callback_.pairEvents_.empty());
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRED), callback_.pairEvents_.back().status);
}

/**
 * @tc.name: SleAdapter_Pair_005
 * @tc.desc: 配对失败：配对中状态回退为未配对并按远端取消上报
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Pair_005, TestSize.Level1)
{
    AddRemoteDevice(kMouseM, static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));

    Adapter().PairComplete(RawAddress(kMouseM), SM_PAIR_ERROR);

    auto remote = SleRemoteDeviceAdapter::GetInstance()->GetRemoteDevice(RawAddress(kMouseM));
    ASSERT_NE(nullptr, remote);
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_NONE), remote->GetPairedStatus());
    ASSERT_FALSE(callback_.pairEvents_.empty());
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_NONE), callback_.pairEvents_.back().status);
}

/**
 * @tc.name: SleAdapter_Pair_006
 * @tc.desc: 首次被配对（远端发起）时设备直接进入配对中并上报
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Pair_006, TestSize.Level1)
{
    Adapter().PairStartChanged(RawAddress(kPlainA));
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        SleRemoteDeviceAdapter::GetInstance()->GetPairStatus(RawAddress(kPlainA)));
    ASSERT_FALSE(callback_.pairEvents_.empty());
}

/**
 * @tc.name: SleAdapter_Pair_007
 * @tc.desc: 可信设备再次收到配对请求时不拉弹窗，直接回调框架并移出可信列表
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Pair_007, TestSize.Level1)
{
    Adapter().pimpl->credibleDevice_.Insert(kHeadsetR);
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    std::string passKey = "1234";

    Adapter().PairingRequest(RawAddress(kHeadsetR), passKey, 0);

    ASSERT_EQ(1U, callback_.pairingReqEvents_.size());
    EXPECT_EQ(RawAddress(kHeadsetR), callback_.pairingReqEvents_.front());
    EXPECT_FALSE(Adapter().pimpl->credibleDevice_.Find(
        [](const std::string &addr) { return addr == kHeadsetR; }));
}

/**
 * @tc.name: SleAdapter_Pair_008
 * @tc.desc: 普通设备配对请求进入回连队列；车锁类外观设备不参与后台回连
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Pair_008, TestSize.Level1)
{
    AddRemoteDevice(kMouseM, static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    AddRemoteDevice(kVehicle, static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED), 
        static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE),
        static_cast<int>(DeviceClassForService::DEVICE_CLASS_VEHICLE_LOCK));
    std::string passKey = "1234";

    Adapter().PairingRequest(RawAddress(kMouseM), passKey, 0);
    ASSERT_EQ(1U, SleReconnectManager::GetInstance().pairedQueue_.size());
    EXPECT_EQ(kMouseM, SleReconnectManager::GetInstance().pairedQueue_.back().reportAddr_);

    Adapter().PairingRequest(RawAddress(kVehicle), passKey, 0);
    EXPECT_EQ(1U, SleReconnectManager::GetInstance().pairedQueue_.size());
}

/**
 * @tc.name: SleAdapter_Pair_009
 * @tc.desc: 被动配对拉弹窗时先异步读对端名字：定时器挂起，名字到达后弹窗函数被执行
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Pair_009, TestSize.Level1)
{
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED),
        static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE));

    std::string passKey = "1234";
    Adapter().PairingRequest(RawAddress(kHeadsetL), passKey, 0);
    // 被动朝向：转异步读名，定时器在挂
    EXPECT_NE(nullptr, Adapter().pimpl->slePassivePairingDialogTimer_);
    EXPECT_EQ(nullptr, Adapter().pimpl->passivePairingSsapConnInst_);

    Adapter().PassivePairingDialog();
    EXPECT_EQ(nullptr, Adapter().pimpl->slePassivePairingDialogTimer_);
}

/**
 * @tc.name: SleAdapter_Pair_010
 * @tc.desc: 加密成功：链路置为已加密并上报；加密失败：设备转入回连名单等待重试
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Pair_010, TestSize.Level1)
{
    AddRemoteDevice(kMouseM, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED),
        static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE));

    Adapter().EncryptionComplete(RawAddress(kMouseM), SM_PAIR_OK);
    EXPECT_EQ(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED),
        SleRemoteDeviceAdapter::GetInstance()->GetAcbState(kMouseM));
    ASSERT_FALSE(callback_.acbEvents_.empty());
    EXPECT_EQ(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED), callback_.acbEvents_.back().state);

    Adapter().EncryptionComplete(RawAddress(kMouseM), SM_PAIR_ERROR);
    EXPECT_EQ(1U, Adapter().pimpl->needReconnectDevices_.Count(kMouseM));
}

/**
 * @tc.name: SleAdapter_Pair_011
 * @tc.desc: 已连接设备再次发起配对走加密链路成功；连接中的设备直接拒绝重复配对
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Pair_011, TestSize.Level1)
{
    AddRemoteDevice(kPlainA, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    EXPECT_TRUE(Adapter().StartPair(RawAddress(kPlainA)));
    EXPECT_EQ(static_cast<int>(SleConnDirect::SLE_CONNECTION_ACTIVE),
        SleRemoteDeviceAdapter::GetInstance()->GetConnDirect(RawAddress(kPlainA)));

    AddRemoteDevice(kPlainB, static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING));
    EXPECT_TRUE(Adapter().StartPair(RawAddress(kPlainB)));
    // 连接中的设备不重复发起
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        SleRemoteDeviceAdapter::GetInstance()->GetPairStatus(RawAddress(kPlainB)));
}

/**
 * @tc.name: SleAdapter_Pair_012
 * @tc.desc: 可信配对成功后设备地址进入可信列表；取消配对完成后本地密钥与设备记录被清空
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Pair_012, TestSize.Level1)
{
    EXPECT_TRUE(Adapter().StartCrediblePair(RawAddress(kPlainA)));
    EXPECT_FALSE(Adapter().pimpl->credibleDevice_.IsEmpty());

    ASSERT_TRUE(SleConfig::GetInstance().SetLinkKey(kPlainB, "AABBCCDDEEFF"));
    AddRemoteDevice(kPlainB, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    Adapter().CancelPairComplete(RawAddress(kPlainB), NLSTK_ERRCODE_SUCCESS,
        static_cast<int>(PairingStateChangeReason::PAIRING_FAILURE));
    EXPECT_EQ(nullptr, SleRemoteDeviceAdapter::GetInstance()->GetRemoteDevice(RawAddress(kPlainB)));
}

// ==================== 连接与断连 ====================

/**
 * @tc.name: SleAdapter_Conn_001
 * @tc.desc: 底层连接完成（既有设备）：配对设备直接补安全链路，链路计数刷新并通知已连接
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Conn_001, TestSize.Level1)
{
    SetSleSwitchState(SleStateID::STATE_TURN_ON);
    AddRemoteDevice(kMouseM, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));

    auto connResult = MakeConnResult(kMouseM, CM_LINK_STATE_CONNECTED, 9, SLE_T_NODE);
    Adapter().ConnectionStateTask(connResult);

    EXPECT_EQ(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED),
        SleRemoteDeviceAdapter::GetInstance()->GetAcbState(kMouseM));
    EXPECT_EQ(1U, Adapter().GetAcbCount());
    ASSERT_FALSE(callback_.acbEvents_.empty());
    EXPECT_EQ(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED), callback_.acbEvents_.back().state);
}

/**
 * @tc.name: SleAdapter_Conn_002
 * @tc.desc: 底层连接完成（陌生设备）：自动创建设备记录，公版合作集成员首次配对自动发起配对
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Conn_002, TestSize.Level1)
{
    SetSleSwitchState(SleStateID::STATE_TURN_ON);
    InjectCdsmGroup(401, kHeadsetR, {kHeadsetR, kHeadsetL});
    // 新连接的是合作集成员左耳（非私有音频）
    auto connResult = MakeConnResult(kHeadsetL, CM_LINK_STATE_CONNECTED, 10, 0);
    Adapter().ConnectionStateTask(connResult);

    auto remote = SleRemoteDeviceAdapter::GetInstance()->GetRemoteDevice(RawAddress(kHeadsetL));
    ASSERT_NE(nullptr, remote);
    EXPECT_EQ(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED), remote->GetAcbConnectState());
    // 合作集成员首连自动配对
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRING), remote->GetPairedStatus());
    EXPECT_EQ(1U, Adapter().GetAcbCount());
}

/**
 * @tc.name: SleAdapter_Conn_003
 * @tc.desc: 配对状态为未配对但本地仍留有密钥的设备连接后自动重新发起配对
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Conn_003, TestSize.Level1)
{
    SetSleSwitchState(SleStateID::STATE_TURN_ON);
    ASSERT_TRUE(SleConfig::GetInstance().SetLinkKey(kPlainA, "AABBCCDDEEFF"));
    AddRemoteDevice(kPlainA, static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));

    auto connResult = MakeConnResult(kPlainA, CM_LINK_STATE_CONNECTED, 11, 0);
    Adapter().ConnectionStateTask(connResult);

    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        SleRemoteDeviceAdapter::GetInstance()->GetPairStatus(RawAddress(kPlainA)));
}

/**
 * @tc.name: SleAdapter_Conn_004
 * @tc.desc: 断链原因映射：本地主动断/远端断/超时分别记录对应断连原因，超时同时给出配对失败原因
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Conn_004, TestSize.Level1)
{
    EXPECT_EQ(static_cast<int>(PairingStateChangeReason::PAIRING_FAILURE),
        Adapter().HandleDisconnAndUnpairedReason(NLSTK_SLE_CONNECTION_TERMINATED_BY_LOCAL_HOST));
    EXPECT_EQ(static_cast<int>(SleConnectReason::CONNECT_LOCAL_DISCONNECT),
        Adapter().pimpl->sleDisconnReason_.load());

    Adapter().HandleDisconnAndUnpairedReason(NLSTK_SLE_REMOTE_USER_TERMINATED_CONNECTION);
    EXPECT_EQ(static_cast<int>(SleConnectReason::CONNECT_REMOTE_DISCONNECT),
        Adapter().pimpl->sleDisconnReason_.load());

    EXPECT_EQ(static_cast<int>(PairingStateChangeReason::PAIRING_ACB_CONNECTION_FAILED),
        Adapter().HandleDisconnAndUnpairedReason(NLSTK_SLE_CONNECTION_TIMEOUT));
    EXPECT_EQ(static_cast<int>(SleConnectReason::CONNECT_FAIL_ACB_CONNECTION),
        Adapter().pimpl->sleDisconnReason_.load());
}

/**
 * @tc.name: SleAdapter_Conn_005
 * @tc.desc: 断连完成：用户主动断连写入标记；满足回连条件的断连进入后台回连名单
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Conn_005, TestSize.Level1)
{
    SetSleSwitchState(SleStateID::STATE_TURN_ON);
    // 用户主动断：DISCONNECTING + 已配对 + 本地原因
    AddRemoteDevice(kPlainA, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTING), 
        static_cast<int>(SleConnDirect::SLE_CONNECTION_ACTIVE));

    auto disconnResult = MakeConnResult(kPlainA, CM_LINK_STATE_DISCONNECTED, 11, 0,
        static_cast<uint8_t>(NLSTK_SLE_CONNECTION_TERMINATED_BY_LOCAL_HOST));
    Adapter().ConnectionStateTask(disconnResult);

    EXPECT_TRUE(SleConfig::GetInstance().GetUserDisconnectedFlag(kPlainA));
    EXPECT_EQ(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED),
        SleRemoteDeviceAdapter::GetInstance()->GetAcbState(kPlainA));
    ASSERT_FALSE(callback_.acbEvents_.empty());
    EXPECT_EQ(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED), callback_.acbEvents_.back().state);
}

/**
 * @tc.name: SleAdapter_Conn_006
 * @tc.desc: 加密态已配对设备异常断连（超时）：进入后台回连名单；配对中断则按前置状态回退
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Conn_006, TestSize.Level1)
{
    SetSleSwitchState(SleStateID::STATE_TURN_ON);
    AddRemoteDevice(kMouseM, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));
    // 后台回连名单只收已入回连队列的设备，先按配对入队
    SleReconnectManager::GetInstance().OnDeviceStartPair(RawAddress(kMouseM));

    auto disconnResult = MakeConnResult(kMouseM, CM_LINK_STATE_DISCONNECTED, 12, 0,
        static_cast<uint8_t>(NLSTK_SLE_CONNECTION_TIMEOUT));
    Adapter().ConnectionStateTask(disconnResult);
    EXPECT_EQ(1U, SleReconnectManager::GetInstance().inBgListDevices_.count(kMouseM));

    // 配对中断且此前已配对：状态回退为已配对
    AddRemoteDevice(kPlainA, static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED), 
        static_cast<int>(SleConnDirect::SLE_CONNECTION_ACTIVE), 0, false, 0,
        static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    auto disconnPlain = MakeConnResult(kPlainA, CM_LINK_STATE_DISCONNECTED, 13, 0,
        static_cast<uint8_t>(NLSTK_SLE_CONNECTION_TIMEOUT));
    Adapter().ConnectionStateTask(disconnPlain);
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        SleRemoteDeviceAdapter::GetInstance()->GetPairStatus(RawAddress(kPlainA)));
}

/**
 * @tc.name: SleAdapter_Conn_007
 * @tc.desc: 配对中且无前置配对的设备断连按取消配对收尾：设备记录与密钥一并清除
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Conn_007, TestSize.Level1)
{
    SetSleSwitchState(SleStateID::STATE_TURN_ON);
    ASSERT_TRUE(SleConfig::GetInstance().SetLinkKey(kPlainB, "AABBCCDDEEFF"));
    AddRemoteDevice(kPlainB, static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED), 
        static_cast<int>(SleConnDirect::SLE_CONNECTION_ACTIVE), 0, false, 0,
        static_cast<int>(SlePairState::SLE_PAIR_NONE));

    auto disconnResult = MakeConnResult(kPlainB, CM_LINK_STATE_DISCONNECTED, 14, 0,
        static_cast<uint8_t>(NLSTK_SLE_CONNECTION_TIMEOUT));
    Adapter().ConnectionStateTask(disconnResult);

    EXPECT_EQ(nullptr, SleRemoteDeviceAdapter::GetInstance()->GetRemoteDevice(RawAddress(kPlainB)));
}

/**
 * @tc.name: SleAdapter_Conn_008
 * @tc.desc: 回连判定：未配对或星闪未开时不回连；加密态、音频设备、断连原因命中回连原因集时回连
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Conn_008, TestSize.Level1)
{
    SetSleSwitchState(SleStateID::STATE_TURN_ON);
    // 已配对 + 音频设备：回连
    AddRemoteDevice(kMouseM, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED),
        static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE), 0, true);
    EXPECT_TRUE(Adapter().NeedBgConn(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED),
        static_cast<int>(SlePairState::SLE_PAIR_PAIRED), RawAddress(kMouseM), 0));
    // 加密态：回连
    EXPECT_TRUE(Adapter().NeedBgConn(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED),
        static_cast<int>(SlePairState::SLE_PAIR_PAIRED), RawAddress(kMouseM), 0));
    // 命中回连原因集：回连
    EXPECT_TRUE(Adapter().NeedBgConn(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED),
        static_cast<int>(SlePairState::SLE_PAIR_PAIRED), RawAddress(kMouseM),
        NLSTK_SLE_CONNECTION_TIMEOUT));
    // 未配对：不回连
    EXPECT_FALSE(Adapter().NeedBgConn(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED),
        static_cast<int>(SlePairState::SLE_PAIR_NONE), RawAddress(kMouseM), 0));
    // 星闪未开：不回连
    SetSleSwitchState(SleStateID::STATE_TURN_OFF);
    EXPECT_FALSE(Adapter().NeedBgConn(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED),
        static_cast<int>(SlePairState::SLE_PAIR_PAIRED), RawAddress(kMouseM), 0));
}

/**
 * @tc.name: SleAdapter_Conn_009
 * @tc.desc: 断开全部 profile：合作集成员逐一断开；非合作集设备按自身连接状态断开
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Conn_009, TestSize.Level1)
{
    SetSleSwitchState(SleStateID::STATE_TURN_ON);
    InjectCdsmGroup(402, kHeadsetR, {kHeadsetR, kHeadsetL});
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    Adapter().pimpl->needReconnectDevices_.Emplace(kHeadsetR);
    Adapter().pimpl->needReconnectDevices_.Emplace(kHeadsetL);

    // 断开下发经协议栈（测试进程无栈必失败），断言落在下发前的真实状态迁移
    EXPECT_FALSE(Adapter().DisconnectAllProfile(RawAddress(kHeadsetR)));
    EXPECT_EQ(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTING),
        SleRemoteDeviceAdapter::GetInstance()->GetAcbState(kHeadsetR));
    EXPECT_EQ(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTING),
        SleRemoteDeviceAdapter::GetInstance()->GetAcbState(kHeadsetL));

    // 断开动作清理回连重试名单
    EXPECT_EQ(0U, Adapter().pimpl->needReconnectDevices_.Count(kHeadsetR));
    EXPECT_EQ(0U, Adapter().pimpl->needReconnectDevices_.Count(kHeadsetL));
}

/**
 * @tc.name: SleAdapter_Conn_010
 * @tc.desc: 连接全部 profile：断连态的已配对设备发起 ACB 连接；合作集整组一起连
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Conn_010, TestSize.Level1)
{
    SetSleSwitchState(SleStateID::STATE_TURN_ON);
    AddRemoteDevice(kMouseM, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));

    EXPECT_TRUE(Adapter().ConnectAllProfile(RawAddress(kMouseM)));
    // 发起连接即置连接中（入直连白名单副作用），不等待底层结果
    EXPECT_EQ(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING),
        SleRemoteDeviceAdapter::GetInstance()->GetAcbState(kMouseM));

    // 合作集整组连接：另一个成员也被发起连接
    InjectCdsmGroup(403, kHeadsetR, {kHeadsetR, kHeadsetL});
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    EXPECT_TRUE(Adapter().ConnectAllProfile(RawAddress(kHeadsetR)));
    EXPECT_NE(nullptr, SleRemoteDeviceAdapter::GetInstance()->GetRemoteDevice(RawAddress(kHeadsetL)));
}

/**
 * @tc.name: SleAdapter_Conn_011
 * @tc.desc: 私有音频设备加密后连接全部 profile 时补记副耳配对关系并上报副耳配对状态
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Conn_011, TestSize.Level1)
{
    InjectCdsmGroup(404, kHeadsetR, {kHeadsetR, kHeadsetL});
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED),
        static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE), 0, true,
        SLE_PRIVATE_AUDIO_BUSINESS_TYPE);
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));

    Adapter().AddDevicePairRecord(RawAddress(kHeadsetR), SLE_PRIVATE_AUDIO_BUSINESS_TYPE);

    // 副耳（另一成员）经历配对中→已配对两次上报
    ASSERT_GE(callback_.csdmPairEvents_.size(), 2U);
    EXPECT_EQ(RawAddress(kHeadsetL), callback_.csdmPairEvents_[0].first);
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRING), callback_.csdmPairEvents_[0].second);
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRED), callback_.csdmPairEvents_[1].second);
}

/**
 * @tc.name: SleAdapter_Conn_012
 * @tc.desc: 全部 profile 断开后消费缓存的断连上下文（无上下文直接跳过），随后清空缓存
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Conn_012, TestSize.Level1)
{
    SetSleSwitchState(SleStateID::STATE_TURN_ON);
    // 无缓存上下文：直接返回
    Adapter().OnAllProfileDisconnected(RawAddress(kMouseM));

    // 有缓存上下文且满足回连：后台回连经协议栈通道下发（无栈必败、不落内存表），
    // 可断言的副作用是消费后清空缓存上下文
    AddRemoteDevice(kMouseM, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));
    Adapter().SetAcbDisConnReasonTask(kMouseM, NLSTK_SLE_CONNECTION_TIMEOUT);
    Adapter().OnAllProfileDisconnected(RawAddress(kMouseM));
    EXPECT_EQ(0, Adapter().GetAcbDisConnReasonTask(kMouseM));
}

/**
 * @tc.name: SleAdapter_Conn_013
 * @tc.desc: 加密态设备正常连接全部 profile：不产生连接状态回退上报，链路状态保持加密；
 *           静默口拒绝路径依赖 profile 连接实例（后续套件覆盖）
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Conn_013, TestSize.Level1)
{
    SetSleSwitchState(SleStateID::STATE_TURN_ON);
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));

    EXPECT_TRUE(Adapter().ConnectAllProfile(RawAddress(kHeadsetR)));
    // 正常路径不发状态回退（回退上报是静默口拒绝路径专属）
    EXPECT_TRUE(callback_.connEvents_.empty());
    EXPECT_EQ(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED),
        SleRemoteDeviceAdapter::GetInstance()->GetAcbState(kHeadsetR));
}

// ==================== 状态上报口径 ====================

/**
 * @tc.name: SleAdapter_Report_001
 * @tc.desc: 合作集成员连接状态按整组口径上报：上报地址映射为 report，重复状态被抑制
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Report_001, TestSize.Level1)
{
    SetSleSwitchState(SleStateID::STATE_TURN_ON);
    InjectCdsmGroup(501, kHeadsetR, {kHeadsetR, kHeadsetL});
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));

    SleConnectionChangedParam param(static_cast<int>(SleConnectState::CONNECTING),
        static_cast<int>(SleConnectState::DISCONNECTED));
    Adapter().NotifyConnectionStateChanged(RawAddress(kHeadsetR), param);

    ASSERT_EQ(1U, callback_.connEvents_.size());
    EXPECT_EQ(RawAddress(kHeadsetR), callback_.connEvents_[0].device);
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTING), callback_.connEvents_[0].state);

    // 同状态重复上报被抑制
    Adapter().NotifyConnectionStateChanged(RawAddress(kHeadsetR), param);
    EXPECT_EQ(1U, callback_.connEvents_.size());
}

/**
 * @tc.name: SleAdapter_Report_002
 * @tc.desc: 非合作集设备状态未变化时不上报；状态变化时按设备地址原样上报
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Report_002, TestSize.Level1)
{
    AddRemoteDevice(kMouseM, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));

    SleConnectionChangedParam same(static_cast<int>(SleConnectState::DISCONNECTED),
        static_cast<int>(SleConnectState::DISCONNECTED));
    Adapter().NotifyConnectionStateChanged(RawAddress(kMouseM), same);
    EXPECT_TRUE(callback_.connEvents_.empty());

    SleConnectionChangedParam changed(static_cast<int>(SleConnectState::CONNECTED),
        static_cast<int>(SleConnectState::DISCONNECTED));
    Adapter().NotifyConnectionStateChanged(RawAddress(kMouseM), changed);
    ASSERT_EQ(1U, callback_.connEvents_.size());
    EXPECT_EQ(RawAddress(kMouseM), callback_.connEvents_[0].device);
}

/**
 * @tc.name: SleAdapter_Report_003
 * @tc.desc: 配对状态按整组口径上报：全员未配对时上报取消结果，配对中→未配对同时删除合作集
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Report_003, TestSize.Level1)
{
    InjectCdsmGroup(502, kHeadsetR, {kHeadsetR, kHeadsetL});
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));

    Adapter().NotifyPairStatusChanged(RawAddress(kHeadsetR), static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(PairingStateChangeReason::PAIRING_REMOTE_CANCELED));

    ASSERT_EQ(1U, callback_.pairEvents_.size());
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_NONE), callback_.pairEvents_[0].status);
    // 配对中→未配对：合作集被删除
    EXPECT_EQ(0U, Cdsm().cdsmList_.Size());
}

/**
 * @tc.name: SleAdapter_Report_004
 * @tc.desc: 合作集配对状态仅 server 侧上报（csdm 回调），非合作集设备不触发 csdm 上报
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Report_004, TestSize.Level1)
{
    InjectCdsmGroup(503, kHeadsetR, {kHeadsetR, kHeadsetL});
    Adapter().NotifyCdsmPairStatusChanged(RawAddress(kHeadsetR), static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    ASSERT_EQ(1U, callback_.csdmPairEvents_.size());

    Adapter().NotifyCdsmPairStatusChanged(RawAddress(kMouseM), static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    EXPECT_EQ(1U, callback_.csdmPairEvents_.size());
}

/**
 * @tc.name: SleAdapter_Report_005
 * @tc.desc: 连接态上报判定：连接中/已断开整组空闲时上报，已连接但稳态计数不足/断开中仍有瞬态时抑制
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Report_005, TestSize.Level1)
{
    InjectCdsmGroup(504, kHeadsetR, {kHeadsetR, kHeadsetL});
    RawAddress report(kHeadsetR);
    CdsmService *cdsmService = CdsmService::GetService();

    // 连接中：稳态已连接数为 0 → 上报
    EXPECT_TRUE(Adapter().IsProfileStateReport(RawAddress(kHeadsetR), report, cdsmService,
        static_cast<int>(SleConnectState::CONNECTING)));
    // 已连接：稳态已连接数为 0 → 不上报
    EXPECT_FALSE(Adapter().IsProfileStateReport(RawAddress(kHeadsetR), report, cdsmService,
        static_cast<int>(SleConnectState::CONNECTED)));
    // 已断开：无瞬态连接 → 上报
    EXPECT_TRUE(Adapter().IsProfileStateReport(RawAddress(kHeadsetR), report, cdsmService,
        static_cast<int>(SleConnectState::DISCONNECTED)));
    // 断开中：无瞬态连接 → 不上报
    EXPECT_FALSE(Adapter().IsProfileStateReport(RawAddress(kHeadsetR), report, cdsmService,
        static_cast<int>(SleConnectState::DISCONNECTING)));
    // 未知状态：不上报
    EXPECT_FALSE(Adapter().IsProfileStateReport(RawAddress(kHeadsetR), report, cdsmService, -1));

    // 一名成员处于连接瞬态：断开中满足唯一瞬态 → 上报
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING));
    EXPECT_TRUE(Adapter().IsProfileStateReport(RawAddress(kHeadsetR), report, cdsmService,
        static_cast<int>(SleConnectState::DISCONNECTING)));
    EXPECT_EQ(RawAddress(kHeadsetR), report);
}

/**
 * @tc.name: SleAdapter_Report_006
 * @tc.desc: 配对态上报判定：已配对按稳态计数、配对中/未配对按瞬态计数决定是否上报
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Report_006, TestSize.Level1)
{
    InjectCdsmGroup(505, kHeadsetR, {kHeadsetR, kHeadsetL});
    RawAddress report(kHeadsetR);
    CdsmService *cdsmService = CdsmService::GetService();

    // 全员未配对：未配对上报成立
    EXPECT_TRUE(Adapter().IsPairStateReport(RawAddress(kHeadsetR), report, cdsmService,
        static_cast<int>(SlePairState::SLE_PAIR_NONE)));
    // 配对中：稳态已配对数为 0 → 上报
    EXPECT_TRUE(Adapter().IsPairStateReport(RawAddress(kHeadsetR), report, cdsmService,
        static_cast<int>(SlePairState::SLE_PAIR_PAIRING)));
    // 一员已配对：已配对上报成立
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    EXPECT_TRUE(Adapter().IsPairStateReport(RawAddress(kHeadsetR), report, cdsmService,
        static_cast<int>(SlePairState::SLE_PAIR_PAIRED)));
    EXPECT_EQ(RawAddress(kHeadsetR), report);
    // 取消中但瞬态计数不止一个 → 不上报
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_CANCELING),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    EXPECT_FALSE(Adapter().IsPairStateReport(RawAddress(kHeadsetR), report, cdsmService,
        static_cast<int>(SlePairState::SLE_PAIR_CANCELING)));
}

/**
 * @tc.name: SleAdapter_Report_007
 * @tc.desc: 连接/瞬态计数：合作集缺失或服务未注册时计数为 0；成员 ACB 瞬态计入未断连计数
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Report_007, TestSize.Level1)
{
    EXPECT_EQ(0, Adapter().GetUnDisconnectedCnt(RawAddress(kUnknownU)));
    EXPECT_EQ(0, Adapter().GetConnectedCnt(RawAddress(kUnknownU)));

    InjectCdsmGroup(506, kHeadsetR, {kHeadsetR, kHeadsetL});
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    EXPECT_EQ(1, Adapter().GetUnDisconnectedCnt(RawAddress(kHeadsetR)));
}

/**
 * @tc.name: SleAdapter_Report_008
 * @tc.desc: 云设备连接中时整体 profile 连接态上报为连接中；双成员合作集按合并表输出
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Report_008, TestSize.Level1)
{
    InjectCloudDevice(kCloudR, kCloudM, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR);
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTING),
        Adapter().GetProfileConnStateTask(RawAddress(kCloudR)));

    // 双成员默认都是已断开：合并结果为已断开
    InjectCdsmGroup(507, kHeadsetR, {kHeadsetR, kHeadsetL});
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED),
        Adapter().GetProfileConnStateTask(RawAddress(kHeadsetR)));
    // 单设备（非合作集）直接返回该设备的 profile 连接态
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED),
        Adapter().GetProfileConnStateTask(RawAddress(kMouseM)));
}

// ==================== 设备信息与杂项 ====================

/**
 * @tc.name: SleAdapter_Device_001
 * @tc.desc: 从配置装载已配对设备档案：有密钥判定已配对、无密钥判定未配对，业务类型/音频标记/朝向齐备
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_001, TestSize.Level1)
{
    ASSERT_TRUE(SleConfig::GetInstance().SetLinkKey(kHeadsetR, "AABBCCDDEEFF"));
    ASSERT_TRUE(SleConfig::GetInstance().SetSleBusiness(kHeadsetR, SLE_PRIVATE_AUDIO_BUSINESS_TYPE));
    ASSERT_TRUE(SleConfig::GetInstance().SetIsAudioDeviceFlag(kHeadsetR, true));
    ASSERT_TRUE(SleConfig::GetInstance().SetPairDirect(kHeadsetR, 1));
    ASSERT_TRUE(SleConfig::GetInstance().SetPeerName(kHeadsetR, "headset"));

    auto paired = std::make_shared<SlePeripheralDevice>();
    Adapter().SetSlePeripheralDeviceBasicInfo(kHeadsetR, paired);
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRED), paired->GetPairedStatus());
    EXPECT_EQ(SLE_PRIVATE_AUDIO_BUSINESS_TYPE, paired->GetManufacturerBusiness());
    EXPECT_TRUE(paired->GetIsAudioDeviceFlag());
    EXPECT_EQ("headset", paired->GetName());

    auto unpaired = std::make_shared<SlePeripheralDevice>();
    Adapter().SetSlePeripheralDeviceBasicInfo(kHeadsetL, unpaired);
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_NONE), unpaired->GetPairedStatus());
}

/**
 * @tc.name: SleAdapter_Device_002
 * @tc.desc: 型号五件套按配置选择性装载：未配置的字段保持缺省
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_002, TestSize.Level1)
{
    ASSERT_TRUE(SleConfig::GetInstance().SetDeviceModelId(kHeadsetR, "123"));
    ASSERT_TRUE(SleConfig::GetInstance().SetDeviceIconId(kHeadsetR, "ABCD"));

    auto device = std::make_shared<SlePeripheralDevice>();
    Adapter().SetSlePeripheralDeviceModelInfo(kHeadsetR, device);
    EXPECT_EQ("123", device->GetModelId());
    EXPECT_EQ("ABCD", device->GetIconId());
    EXPECT_TRUE(device->GetNewModelId().empty());
}

/**
 * @tc.name: SleAdapter_Device_003
 * @tc.desc: 配对设备表装载时跳过无效地址与云端 NONE 态设备，只装有效档案
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_003, TestSize.Level1)
{
    ASSERT_TRUE(SleConfig::GetInstance().SetLinkKey(kHeadsetR, "AABBCCDDEEFF"));
    // 云端 NONE 态设备不下发本地档案
    InjectCloudDevice(kCloudR, kCloudM, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
    ASSERT_TRUE(SleConfig::GetInstance().Save());

    Adapter().ReadPeerDeviceInfoFromConf({"00:00:00:00:00:00", "", kCloudR, kHeadsetR});

    EXPECT_NE(nullptr, SleRemoteDeviceAdapter::GetInstance()->GetRemoteDevice(RawAddress(kHeadsetR)));
    EXPECT_EQ(nullptr, SleRemoteDeviceAdapter::GetInstance()->GetRemoteDevice(RawAddress(kCloudR)));
}

/**
 * @tc.name: SleAdapter_Device_004
 * @tc.desc: 型号信息格式化：空值补默认占位（FFFFFF/FF/FFFF），超长截断、不足补零
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_004, TestSize.Level1)
{
    DeviceModel model;
    std::string newModelId = "999";
    Adapter().FormatDeviceModelInfo(model, newModelId);
    EXPECT_EQ("999", model.GetModelId());
    EXPECT_EQ("FF", model.GetSubModelId());
    EXPECT_EQ("FFFF", model.GetIconId());
    EXPECT_EQ("FF", model.GetDevType());

    DeviceModel full;
    full.SetModelId("ABCDEFGH");
    full.SetSubModelId("12");
    full.SetIconId("ABCDEF");
    full.SetDevType("XYZW");
    std::string emptyModelId;
    Adapter().FormatDeviceModelInfo(full, emptyModelId);
    EXPECT_EQ("ABCDEF", full.GetModelId());
    EXPECT_EQ("12", full.GetSubModelId());
    EXPECT_EQ("ABCD", full.GetIconId());
    EXPECT_EQ("XY", full.GetDevType());

    // 全空：modelId 落默认占位
    DeviceModel blank;
    Adapter().FormatDeviceModelInfo(blank, emptyModelId);
    EXPECT_EQ("FFFFFF", blank.GetModelId());
}

/**
 * @tc.name: SleAdapter_Device_005
 * @tc.desc: 更新设备型号信息时同步写进合作集每个成员的配置；非合作集设备只写自身
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_005, TestSize.Level1)
{
    DeviceModel model;
    model.SetModelId("777");

    // 型号落盘只对设备表中已注册的设备生效，先预建设备
    AddRemoteDevice(kMouseM, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    Adapter().UpdateDeviceModelInfo(kMouseM, model, "");
    EXPECT_EQ("777", SleConfig::GetInstance().GetDeviceModelId(kMouseM));

    InjectCdsmGroup(601, kHeadsetR, {kHeadsetR, kHeadsetL});
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    AddRemoteDevice(kHeadsetL, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    Adapter().UpdateDeviceModelInfo(kHeadsetR, model, "");
    EXPECT_EQ("777", SleConfig::GetInstance().GetDeviceModelId(kHeadsetR));
    EXPECT_EQ("777", SleConfig::GetInstance().GetDeviceModelId(kHeadsetL));
}

/**
 * @tc.name: SleAdapter_Device_006
 * @tc.desc: 查询合作集成员列表只返回连接态成员；服务未注册时查询失败
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_006, TestSize.Level1)
{
    auto group = std::make_shared<CdsmInfo>(602, kHeadsetR);
    group->memberCnt_ = 2;
    group->CdsmAddMemberInfo(kHeadsetR, static_cast<uint8_t>(SleConnectState::CONNECTED));
    group->CdsmAddMemberInfo(kHeadsetL, static_cast<uint8_t>(SleConnectState::DISCONNECTED));
    Cdsm().cdsmList_.EnsureInsert(602, group);

    std::vector<RawAddress> members;
    EXPECT_TRUE(Adapter().WrapperCdsmGetAllMemberInfo(RawAddress(kHeadsetR), members));
    ASSERT_EQ(1U, members.size());
    EXPECT_EQ(RawAddress(kHeadsetR), members.front());

    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CDSM, nullptr);
    EXPECT_FALSE(Adapter().WrapperCdsmGetAllMemberInfo(RawAddress(kHeadsetR), members));
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CDSM, cdsmService_.get());
}

/**
 * @tc.name: SleAdapter_Device_007
 * @tc.desc: 帧类型记录支持设置/查询/删除；扫描连接且帧四的判定只在两者同时成立时为真
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_007, TestSize.Level1)
{
    uint8_t frameType = 0;
    Adapter().SetConnFrameType(kMouseM, static_cast<uint8_t>(SleConnFrameType::SLE_CONN_FRAME_TYPE_4));
    EXPECT_TRUE(Adapter().GetConnFrameType(kMouseM, frameType));
    EXPECT_EQ(static_cast<uint8_t>(SleConnFrameType::SLE_CONN_FRAME_TYPE_4), frameType);
    EXPECT_TRUE(Adapter().IsScanConnTypeAndFrameType4(RawAddress(kMouseM), CM_CONN_COMPLETE_SCAN));
    EXPECT_FALSE(Adapter().IsScanConnTypeAndFrameType4(RawAddress(kMouseM), CM_CONN_COMPLETE_ADV));

    Adapter().DelConnFrameType(kMouseM);
    EXPECT_FALSE(Adapter().GetConnFrameType(kMouseM, frameType));
    EXPECT_FALSE(Adapter().IsScanConnTypeAndFrameType4(RawAddress(kMouseM), CM_CONN_COMPLETE_SCAN));
}

/**
 * @tc.name: SleAdapter_Device_008
 * @tc.desc: 可连接模式：限时模式挂定时器、不限时模式不挂；句柄空闲时不重复启动广播流程
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_008, TestSize.Level1)
{
    // 已有可连接广播句柄：只刷新定时器
    Adapter().pimpl->sleConnectableHandle_.store(5);
    Adapter().SetSleConnectionModeTask(
        static_cast<int>(SLEConnectionMode::CONNECTION_MODE_CONNECTABLE), 0);
    EXPECT_EQ(nullptr, Adapter().pimpl->sleConnectableTimer_);

    Adapter().SetSleConnectionModeTask(
        static_cast<int>(SLEConnectionMode::CONNECTION_MODE_CONNECTABLE), 3);
    EXPECT_NE(nullptr, Adapter().pimpl->sleConnectableTimer_);

    Adapter().SetSleConnectionModeTask(
        static_cast<int>(SLEConnectionMode::CONNECTION_MODE_UNCONNECTABLE), 3);
    EXPECT_EQ(nullptr, Adapter().pimpl->sleConnectableTimer_);
    // 句柄由停止广播的异步结果回调复位（见 Device_009），同步调用后仍保持原值
    EXPECT_EQ(5, Adapter().pimpl->sleConnectableHandle_.load());

    // 非法模式值被拒绝
    Adapter().SetSleConnectionModeTask(99, 3);
}

/**
 * @tc.name: SleAdapter_Device_009
 * @tc.desc: 可连接广播结果回调按句柄匹配复位句柄，不匹配的回调被忽略
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_009, TestSize.Level1)
{
    Adapter().pimpl->sleConnectableHandle_.store(5);
    Adapter().pimpl->sleConnectableAdvertiserCallback_->OnStopResultEvent(
        static_cast<int>(ADV_RESULT_SUCCESS), 6);
    EXPECT_EQ(5, Adapter().pimpl->sleConnectableHandle_.load());

    Adapter().pimpl->sleConnectableAdvertiserCallback_->OnStopResultEvent(
        static_cast<int>(ADV_RESULT_SUCCESS), 5);
    EXPECT_EQ(static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
        Adapter().pimpl->sleConnectableHandle_.load());
}

/**
 * @tc.name: SleAdapter_Device_010
 * @tc.desc: 帧四能力以系统配置为准，测距能力以芯片上报的本地能力位为准，其余特性返回不支持
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_010, TestSize.Level1)
{
    EXPECT_FALSE(Adapter().IsFeatureSupported(0));
    // 帧四：当前环境未开帧四开关
    bool frame4 = Adapter().IsFeatureSupported(static_cast<int32_t>(SleFeatureSupported::SLE_RADIO_FRAME_TYPE_4));
    EXPECT_FALSE(frame4);
    // 测距：本地能力位未上报
    EXPECT_FALSE(Adapter().IsFeatureSupported(static_cast<int32_t>(SleFeatureSupported::SLE_RANGING)));
}

/**
 * @tc.name: SleAdapter_Device_011
 * @tc.desc: 读远端特性完成：帧四设备跳过物理层设置，帧一设备按音频与否选择帧型
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_011, TestSize.Level1)
{
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED), 
        static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE), 0, true);
    Adapter().SetConnFrameType(kHeadsetR, static_cast<uint8_t>(SleConnFrameType::SLE_CONN_FRAME_TYPE_4));

    CM_ReadRemoteFeatureVersionRsp_S param = {};
    RawAddress(kHeadsetR).ConvertToUint8(param.addr.addr);
    // 帧四：直接返回，不改帧型记录
    Adapter().ReadFeatureVersionTask(param);
    uint8_t frameType = 0;
    ASSERT_TRUE(Adapter().GetConnFrameType(kHeadsetR, frameType));
    EXPECT_EQ(static_cast<uint8_t>(SleConnFrameType::SLE_CONN_FRAME_TYPE_4), frameType);

    // 帧一：按设备音频标记重置为帧一并下发物理层设置（裸进程下发失败不影响状态）
    Adapter().SetConnFrameType(kHeadsetR, static_cast<uint8_t>(SleConnFrameType::SLE_CONN_FRAME_TYPE_1));
    Adapter().ReadFeatureVersionTask(param);
}

/**
 * @tc.name: SleAdapter_Device_012
 * @tc.desc: 接受过滤列表大小读取成功时刷新后台回连容量上限，失败时保持不变
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_012, TestSize.Level1)
{
    CM_ReadAcceptFilterListSize_S param = {};
    param.status = 1;
    param.listSize = 9;
    Adapter().ReadAcceptFilterListSizeCallbackTask(param);
    EXPECT_NE(9U, SleReconnectManager::GetInstance().bgConnMaxNum_);

    param.status = 0;
    Adapter().ReadAcceptFilterListSizeCallbackTask(param);
    EXPECT_EQ(9U, SleReconnectManager::GetInstance().bgConnMaxNum_);
}

/**
 * @tc.name: SleAdapter_Device_013
 * @tc.desc: HID 共存事件按类型分流：查询间隔走查询链路，参数更新走待定链路
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_013, TestSize.Level1)
{
    CM_HidCoexModeRsp_S param = {};
    RawAddress(kMouseM).ConvertToUint8(param.addr.addr);

    param.eventType = CM_SLE_CBK_EVENT_GET_HID_COEX_INTERVAL;
    param.incomingInterval = 20;
    param.coexInterval = 40;
    Adapter().HidCoexModeCallback(&param);

    param.eventType = CM_SLE_CBK_EVENT_HID_COEX_MODE_PARAM_UPDATE;
    Adapter().HidCoexModeCallback(&param);

    // 非法事件类型
    param.eventType = static_cast<CM_HidCoexModeEventType_E>(0xFF);
    Adapter().HidCoexModeCallback(&param);
}

/**
 * @tc.name: SleAdapter_Device_014
 * @tc.desc: 已知链路句柄能反查设备地址；未知句柄返回无效地址并尝试同步链路转换
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_014, TestSize.Level1)
{
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED), 
        static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE), 0, false, 0, -1, 15);

    EXPECT_EQ(kHeadsetR, SleAdapter::GetAddressByConnHandle(15));
    EXPECT_NE(kHeadsetR, SleAdapter::GetAddressByConnHandle(16));
}

/**
 * @tc.name: SleAdapter_Device_015
 * @tc.desc: 恢复出厂设置清空全部设备记录与配对配置；重复执行仍成功返回
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_015, TestSize.Level1)
{
    ASSERT_TRUE(SleConfig::GetInstance().SetLinkKey(kHeadsetR, "AABBCCDDEEFF"));
    ASSERT_TRUE(SleConfig::GetInstance().Save());
    AddRemoteDevice(kHeadsetR, static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));

    EXPECT_TRUE(Adapter().FactoryReset());
    EXPECT_TRUE(SleConfig::GetInstance().GetPairedAddrList().empty());
    EXPECT_EQ(nullptr, SleRemoteDeviceAdapter::GetInstance()->GetRemoteDevice(RawAddress(kHeadsetR)));

    EXPECT_TRUE(Adapter().FactoryReset());
}

/**
 * @tc.name: SleAdapter_Device_016
 * @tc.desc: 直连名单只在允许自动回连用户断连设备的策略下发送；策略关闭时直接跳过
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_016, TestSize.Level1)
{
    SleServiceManager::GetInstance()->autoConnPolicy_ =
        SleAutoConnectPolicy::AUTO_CONN_EXCEPT_USER_DISCONNECTED_DEVICES;
    Adapter().SendDirectConnList();

    SleServiceManager::GetInstance()->autoConnPolicy_ = SleAutoConnectPolicy::AUTO_CONN_GENERAL;
    Adapter().SendDirectConnList();
}

/**
 * @tc.name: SleAdapter_Device_017
 * @tc.desc: 星闪开启状态下整机连接数变化触发公共事件上报路径；关闭状态下不上报
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_017, TestSize.Level1)
{
    SetSleSwitchState(SleStateID::STATE_TURN_OFF);
    // 仅验证两条分支都不崩（公共事件发布为尽力而为，不作为断言依据）
    PublishDeviceConnectionState(0, CM_LINK_STATE_DISCONNECTED);
    PublishDeviceConnectionState(1, CM_LINK_STATE_CONNECTED);

    SetSleSwitchState(SleStateID::STATE_TURN_ON);
    PublishDeviceConnectionState(0, CM_LINK_STATE_DISCONNECTED);
    PublishDeviceConnectionState(1, CM_LINK_STATE_CONNECTED);
    PublishDeviceConnectionState(2, CM_LINK_STATE_CONNECTED);
}

/**
 * @tc.name: SleAdapter_Device_018
 * @tc.desc: 云设备档案查询：云缓存下设备按已配对口径返回，普通设备按设备表实际状态返回
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterServiceTest, SleAdapter_Device_018, TestSize.Level1)
{
    InjectCloudDevice(kCloudR, kCloudM, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRED), Adapter().GetPairState(RawAddress(kCloudR)));

    AddRemoteDevice(kMouseM, static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_NONE), Adapter().GetPairState(RawAddress(kMouseM)));
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
