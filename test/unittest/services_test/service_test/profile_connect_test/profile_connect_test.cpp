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
 * 被测三个管理器均为真实例：SSAP 客户端用假件、其余 profile 服务（DIS/TWS/CDSM）为真实例，
 * 全部经业务注册表挂载；管理器内部状态经测试编译约定直接摆布，回调接出来计数。
 * 运行约束：仅在测试机执行——用例会发布 nearlink 公共事件并写 persist.nearlink /
 * persist.telephony 系统参数（TearDown 恢复）；建议 nearlink 服务停用后运行。
 * 本文件是全目标唯一包含三个被测源文件的编译单元，其余声明见 profile_connect_test_helper.h。
 */

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "profile_connect_test_helper.h"

#include "CdsmService.h"
#include "DisService.h"
#include "SleRemoteDeviceManager.h"
#include "TwsService.h"
#include "parameters.h"
#include "sle_service_data.h"

#include "ProfileServiceManager.cpp"
#include "SleServiceManager.cpp"
#include "SleProfileConnectManager.cpp"

namespace OHOS {
namespace Nearlink {
// 卸载替身的调用计数（定义见 mock_unload_sa.cpp）
extern std::atomic<int> g_unloadSaCallCount;

namespace TEST {

using namespace testing::ext;

static void SyncThreadsForTest()
{
    auto &util = ThreadUtil::GetInstance();
    util.threadStateMap_.EnsureInsert(THREAD_ID_SVC_MGR, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    util.threadStateMap_.EnsureInsert(THREAD_ID_ADAPTER, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    util.threadStateMap_.EnsureInsert(THREAD_ID_CDSM, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    util.threadStateMap_.EnsureInsert(THREAD_ID_DIS, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    util.threadStateMap_.EnsureInsert(THREAD_ID_DEVICE_ADAPTER, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
}

static void SetStartedProfile(const std::string &name, SleInterfaceProfile *profile)
{
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(SleTransport::ADAPTER_SLE, name, profile);
}

// ===========================================================================
// 套件一：SleProfileConnectManager（连接实例规则 + 连接编排）
// ===========================================================================
class SleProfileConnectTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        // 只加载配置文件，不创建 adapter——本套件不依赖固件栈
        SleConfig::GetInstance().LoadConfigInfo();
    }

    void SetUp() override
    {
        SyncThreadsForTest();
        calls_ = std::make_unique<FuncCalls>();
        manager_ = std::make_unique<SleProfileConnectManager>();
        manager_->Init({[this](const RawAddress &addr, const SleConnectionChangedParam &param) {
                            calls_->notify.push_back({param.connState, param.connPreState, param.connReason});
                            calls_->notifyAddrs.push_back(addr.GetAddress());
                        },
            [this](const RawAddress &addr, uint8_t reason) { calls_->disconnectAcb++; },
            [this](const RawAddress &addr) { calls_->sendImgSecu++; },
            [this](const RawAddress &addr) { calls_->allDiscAddrs.push_back(addr.GetAddress()); }});

        ssap_ = std::make_unique<FakeSsapClientService>();
        disService_ = std::make_unique<DisService>();
        twsService_ = std::make_unique<TwsService>();
        cdsmService_ = std::make_unique<CdsmService>();
        SetStartedProfile(PROFILE_NAME_SSAP_CLIENT, ssap_.get());
        SetStartedProfile(PROFILE_NAME_DIS, disService_.get());
        SetStartedProfile(PROFILE_NAME_TWS, twsService_.get());
        SetStartedProfile(PROFILE_NAME_CDSM, cdsmService_.get());
    }

    void TearDown() override
    {
        manager_->pimpl->SleProfileConnectInstSafeList_.Clear();
        manager_.reset();
        SetStartedProfile(PROFILE_NAME_SSAP_CLIENT, nullptr);
        SetStartedProfile(PROFILE_NAME_DIS, nullptr);
        SetStartedProfile(PROFILE_NAME_TWS, nullptr);
        SetStartedProfile(PROFILE_NAME_CDSM, nullptr);
        ssap_.reset();
        disService_.reset();
        twsService_.reset();
        cdsmService_.reset();
        for (const auto &addr : usedAddrs_) {
            SleConfig::GetInstance().SetPeerAppearance(addr, INVALID_APPEARANCE);
            SleConfig::GetInstance().RemovePairedDevice(addr);
            SleConfig::GetInstance().RemoveCdsmGroup(addr);
            SleRemoteDeviceManager::GetInstance()->RemovePeripheralDevice(addr);
        }
        SleConfig::GetInstance().Save();
        ThreadUtil::GetInstance().InitThreadStateMap();
    }

    std::shared_ptr<SleProfileConnectManager::SleProfileConnectInst> MakeInst(const std::string &addr,
        uint8_t state = SLE_ADAPTER_PROF_CONN_STATE_UNUSED)
    {
        auto inst = std::make_shared<SleProfileConnectManager::SleProfileConnectInst>(RawAddress(addr));
        if (state != SLE_ADAPTER_PROF_CONN_STATE_UNUSED) {
            inst->SetState(state);
            inst->StopConnTimer();
        }
        usedAddrs_.push_back(addr);
        return inst;
    }

    void InsertInst(const std::string &addr, uint8_t state)
    {
        manager_->pimpl->SleProfileConnectInstSafeList_.EnsureInsert(addr, MakeInst(addr, state));
    }

    std::shared_ptr<SleProfileConnectManager::SleProfileConnectInst> GetInst(const std::string &addr)
    {
        std::shared_ptr<SleProfileConnectManager::SleProfileConnectInst> inst;
        manager_->pimpl->SleProfileConnectInstSafeList_.GetValue(addr, inst);
        return inst;
    }

    bool HasInst(const std::string &addr)
    {
        return GetInst(addr) != nullptr;
    }

    void SetAppearance(const std::string &addr, int appearance)
    {
        SleConfig::GetInstance().SetPeerAppearance(addr, appearance);
        usedAddrs_.push_back(addr);
    }

    void InsertAudioDevice(const std::string &addr, bool isAudio, int connDirect)
    {
        auto device = std::make_shared<SlePeripheralDevice>();
        device->SetIsAudioDeviceFlag(isAudio);
        device->SetConnDirect(connDirect);
        SleRemoteDeviceManager::GetInstance()->AddPeripheralDevice(addr, device);
        usedAddrs_.push_back(addr);
    }

    int NotifyNewState(size_t i) const { return calls_->notify[i][0]; }
    int NotifyOldState(size_t i) const { return calls_->notify[i][1]; }
    int NotifyReason(size_t i) const { return calls_->notify[i][2]; }

    struct FuncCalls {
        std::vector<std::array<int, 3>> notify;
        std::vector<std::string> notifyAddrs;
        int disconnectAcb = 0;
        int sendImgSecu = 0;
        std::vector<std::string> allDiscAddrs;
    };

    std::unique_ptr<FuncCalls> calls_;
    std::unique_ptr<SleProfileConnectManager> manager_;
    std::unique_ptr<FakeSsapClientService> ssap_;
    std::unique_ptr<DisService> disService_;
    std::unique_ptr<TwsService> twsService_;
    std::unique_ptr<CdsmService> cdsmService_;
    std::vector<std::string> usedAddrs_;
};

/**
 * @tc.name: ProfileConnectInstNextProfileOrder_001
 * @tc.desc: 业务连接顺序按私有TWS、合作集、音量控制、普通业务、音频流、电池逐级推进，
 *           全部连完或无任何业务时不再给出下一个
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, ProfileConnectInstNextProfileOrder_001, TestSize.Level1)
{
    auto inst = MakeInst(kAddrL);
    inst->AddSupportProfile(PROFILE_NAME_ASC);
    inst->AddSupportProfile(PROFILE_NAME_BAS);
    inst->AddSupportProfile(PROFILE_NAME_CDSM);
    inst->AddSupportProfile(PROFILE_NAME_HID_HOST);
    inst->AddSupportProfile(PROFILE_NAME_TWS);
    inst->AddSupportProfile(PROFILE_NAME_VCP);

    EXPECT_EQ(PROFILE_NAME_TWS, inst->GetNextDisconnectedProfile());
    inst->UpdateProfileState(PROFILE_NAME_TWS, static_cast<int>(SleConnectState::CONNECTED));
    EXPECT_EQ(PROFILE_NAME_CDSM, inst->GetNextDisconnectedProfile());
    inst->UpdateProfileState(PROFILE_NAME_CDSM, static_cast<int>(SleConnectState::CONNECTED));
    EXPECT_EQ(PROFILE_NAME_VCP, inst->GetNextDisconnectedProfile());
    inst->UpdateProfileState(PROFILE_NAME_VCP, static_cast<int>(SleConnectState::CONNECTED));
    // 普通业务只剩 HID，音频流与电池垫底
    EXPECT_EQ(PROFILE_NAME_HID_HOST, inst->GetNextDisconnectedProfile());
    inst->UpdateProfileState(PROFILE_NAME_HID_HOST, static_cast<int>(SleConnectState::CONNECTED));
    EXPECT_EQ(PROFILE_NAME_ASC, inst->GetNextDisconnectedProfile());
    inst->UpdateProfileState(PROFILE_NAME_ASC, static_cast<int>(SleConnectState::CONNECTED));
    EXPECT_EQ(PROFILE_NAME_BAS, inst->GetNextDisconnectedProfile());
    inst->UpdateProfileState(PROFILE_NAME_BAS, static_cast<int>(SleConnectState::CONNECTED));

    EXPECT_EQ("", inst->GetNextDisconnectedProfile());
    // 全部已连时按业务名字典序取首个
    EXPECT_EQ(PROFILE_NAME_ASC, inst->GetNextConnectedProfile());

    auto empty = MakeInst(kAddrR);
    EXPECT_EQ("", empty->GetNextDisconnectedProfile());
    EXPECT_EQ("", empty->GetNextConnectedProfile());
}

/**
 * @tc.name: ProfileConnectInstStateNaming_002
 * @tc.desc: 每个连接实例状态都有对应的名字，未知状态返回空串；内部状态可换算成对上层的连接态
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, ProfileConnectInstStateNaming_002, TestSize.Level1)
{
    using Inst = SleProfileConnectManager::SleProfileConnectInst;
    const std::string stateNames[] = {
        "SLE_ADAPTER_PROF_CONN_STATE_UNUSED",
        "SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING",
        "SLE_ADAPTER_PROF_CONN_STATE_SSAP_CONNECTING",
        "SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING",
        "SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING",
        "SLE_ADAPTER_PROF_CONN_STATE_CONNECTED",
        "SLE_ADAPTER_PROF_CONN_STATE_DISCONNECTING",
    };
    for (size_t i = 0; i < sizeof(stateNames) / sizeof(stateNames[0]); i++) {
        EXPECT_EQ(stateNames[i], Inst::GetProfileConnectionStateName(static_cast<int>(i)));
    }
    EXPECT_EQ("", Inst::GetProfileConnectionStateName(99));

    auto &mgr = *manager_;
    const std::pair<int, SleConnectState> converts[] = {
        {SLE_ADAPTER_PROF_CONN_STATE_UNUSED, SleConnectState::DISCONNECTED},
        {SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING, SleConnectState::CONNECTING},
        {SLE_ADAPTER_PROF_CONN_STATE_SSAP_CONNECTING, SleConnectState::CONNECTING},
        {SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING, SleConnectState::CONNECTING},
        {SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING, SleConnectState::CONNECTING},
        {SLE_ADAPTER_PROF_CONN_STATE_CONNECTED, SleConnectState::CONNECTED},
        {SLE_ADAPTER_PROF_CONN_STATE_DISCONNECTING, SleConnectState::DISCONNECTING},
        {99, SleConnectState::DISCONNECTED},
    };
    for (auto &item : converts) {
        EXPECT_EQ(static_cast<int>(item.second), mgr.ConvertToProfileConnectionState(item.first));
    }
}

/**
 * @tc.name: ProfileConnectInstSetStateTimerPolicy_003
 * @tc.desc: 进入中间状态要起连接超时表，进入等待对端状态改起断连表，终态不起表；状态值如实落位
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, ProfileConnectInstSetStateTimerPolicy_003, TestSize.Level1)
{
    auto inst = MakeInst(kAddrL);
    for (uint8_t state = SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING;
         state <= SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING; state++) {
        inst->SetState(state);
        EXPECT_EQ(state, inst->GetState());
        inst->StopConnTimer();
    }
    inst->SetState(SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED);
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED, inst->GetState());
    inst->SetState(SLE_ADAPTER_PROF_CONN_STATE_CONNECTED);
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_CONNECTED, inst->GetState());
    inst->SetState(SLE_ADAPTER_PROF_CONN_STATE_UNUSED);
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_UNUSED, inst->GetState());
}

/**
 * @tc.name: ProfileConnStateChangedTaskDispatch_004
 * @tc.desc: 业务状态变化按连接/断连分派处理，未知设备与中间状态不产生任何动作
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, ProfileConnStateChangedTaskDispatch_004, TestSize.Level1)
{
    manager_->ProfileConnectionStateChangedTask(
        PROFILE_NAME_BAS, RawAddress(kAddrU), static_cast<int>(SleConnectState::CONNECTED), 0);
    manager_->ProfileConnectionStateChangedTask(
        PROFILE_NAME_BAS, RawAddress(kAddrU), static_cast<int>(SleConnectState::DISCONNECTED), 0);
    manager_->ProfileConnectionStateChangedTask(PROFILE_NAME_BAS, RawAddress(kAddrU), -1, 0);
    EXPECT_TRUE(calls_->notify.empty());
}

/**
 * @tc.name: ProcessProfileConnectedAllDone_005
 * @tc.desc: 最后一个业务连接完成后实例进入已连接态并上报成功；合作集业务触发落盘、
 *           音频与音量业务齐活时通知耳机服务
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, ProcessProfileConnectedAllDone_005, TestSize.Level1)
{
    InsertInst(kAddrL, SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING);
    auto inst = GetInst(kAddrL);
    ASSERT_NE(nullptr, inst);
    inst->AddSupportProfile(PROFILE_NAME_DIS);

    manager_->ProcessProfileConnected(PROFILE_NAME_DIS, RawAddress(kAddrL));

    ASSERT_EQ(1U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTED), NotifyNewState(0));
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTING), NotifyOldState(0));
    EXPECT_EQ(static_cast<int>(SleConnectReason::CONNECT_SUCCESS), NotifyReason(0));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_CONNECTED, inst->GetState());

    // 合作集与音频+音量路径不崩溃、状态仍收口到已连接
    InsertInst(kAddrM, SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING);
    auto cdsmInst = GetInst(kAddrM);
    ASSERT_NE(nullptr, cdsmInst);
    cdsmInst->AddSupportProfile(PROFILE_NAME_CDSM);
    manager_->ProcessProfileConnected(PROFILE_NAME_CDSM, RawAddress(kAddrM));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_CONNECTED, cdsmInst->GetState());

    InsertInst(kAddrR, SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING);
    auto audioInst = GetInst(kAddrR);
    ASSERT_NE(nullptr, audioInst);
    audioInst->AddSupportProfile(PROFILE_NAME_ASC);
    audioInst->AddSupportProfile(PROFILE_NAME_VCP);
    audioInst->SetProfileConnected(PROFILE_NAME_ASC);
    manager_->ProcessProfileConnected(PROFILE_NAME_VCP, RawAddress(kAddrR));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_CONNECTED, audioInst->GetState());
}

/**
 * @tc.name: ProcessProfileConnectedChainNext_006
 * @tc.desc: 还有业务未连时实例保持业务连接中并拉起下一个业务；连音频流前先发组播安全配置
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, ProcessProfileConnectedChainNext_006, TestSize.Level1)
{
    InsertInst(kAddrL, SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING);
    auto inst = GetInst(kAddrL);
    ASSERT_NE(nullptr, inst);
    inst->AddSupportProfile(PROFILE_NAME_HID_HOST);
    inst->AddSupportProfile(PROFILE_NAME_ASC);

    manager_->ProcessProfileConnected(PROFILE_NAME_HID_HOST, RawAddress(kAddrL));

    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING, inst->GetState());
    EXPECT_TRUE(calls_->notify.empty());
    // 链式拉起下一个业务是 ASC，已发过一次组播安全配置
    EXPECT_EQ(1, calls_->sendImgSecu);

    // 直接驱动连接业务入口：仅音频流会再次触发组播安全配置
    manager_->ConnectProfile(PROFILE_NAME_ASC, RawAddress(kAddrL));
    EXPECT_EQ(2, calls_->sendImgSecu);
    manager_->ConnectProfile(PROFILE_NAME_HID_HOST, RawAddress(kAddrL));
    EXPECT_EQ(2, calls_->sendImgSecu);
}

/**
 * @tc.name: ProcessProfileConnectedInnerScenarios_007
 * @tc.desc: 识别为不可用设备时的三种处置：本端或对端是手表/车钥匙则恢复连接，
 *           被连端等对端断连，连端主动断开DIS并上报
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, ProcessProfileConnectedInnerScenarios_007, TestSize.Level1)
{
    // 对端是手表：保留连接
    InsertInst(kAddrL, SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED);
    SetAppearance(kAddrL, DEVICE_SMART_WATCH);
    auto watchInst = GetInst(kAddrL);
    ASSERT_NE(nullptr, watchInst);
    watchInst->AddSupportProfile(PROFILE_NAME_ASC);
    manager_->ProcessProfileConnected(PROFILE_NAME_DIS, RawAddress(kAddrL));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING, watchInst->GetState());
    EXPECT_TRUE(calls_->notify.empty());

    // 对端是车钥匙：同样保留
    InsertInst(kAddrM, SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED);
    SetAppearance(kAddrM, DEVICE_CLASS_VEHICLE_LOCK);
    auto carKeyInst = GetInst(kAddrM);
    ASSERT_NE(nullptr, carKeyInst);
    carKeyInst->AddSupportProfile(PROFILE_NAME_ASC);
    manager_->ProcessProfileConnected(PROFILE_NAME_DIS, RawAddress(kAddrM));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING, carKeyInst->GetState());

    // 普通设备且本端是被连端：不主动断，只上报断连
    InsertInst(kAddrR, SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED);
    SetAppearance(kAddrR, 100);
    InsertAudioDevice(kAddrR, false, static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE));
    auto passiveInst = GetInst(kAddrR);
    ASSERT_NE(nullptr, passiveInst);
    manager_->ProcessProfileConnected(PROFILE_NAME_DIS, RawAddress(kAddrR));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_UNUSED, passiveInst->GetState());
    ASSERT_EQ(1U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), NotifyNewState(0));
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTED), NotifyOldState(0));
    EXPECT_EQ(static_cast<int>(SleConnectReason::CONNECT_REMOTE_DISCONNECT), NotifyReason(0));

    // 普通设备且本端是连端：上报连接成功后主动断开DIS
    calls_->notify.clear();
    InsertInst(kAddrU, SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED);
    SetAppearance(kAddrU, 100);
    InsertAudioDevice(kAddrU, false, static_cast<int>(SleConnDirect::SLE_CONNECTION_ACTIVE));
    manager_->ProcessProfileConnected(PROFILE_NAME_DIS, RawAddress(kAddrU));
    ASSERT_EQ(1U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTED), NotifyNewState(0));
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), NotifyOldState(0));
    EXPECT_EQ(static_cast<int>(SleConnectReason::CONNECT_SUCCESS), NotifyReason(0));
}

/**
 * @tc.name: ProcessProfileDisconnectedPaths_008
 * @tc.desc: 业务断连按实例状态分流：等待对端只上报、业务连接中走异常断连、静默盒装逐业务断、
 *           非静默且无存活业务时清档并通知全断
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, ProcessProfileDisconnectedPaths_008, TestSize.Level1)
{
    // 等待对端设备：只补一条无可用业务的上报
    InsertInst(kAddrL, SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED);
    manager_->ProcessProfileDisconnected(PROFILE_NAME_BAS, RawAddress(kAddrL));
    ASSERT_EQ(1U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), NotifyNewState(0));
    EXPECT_EQ(static_cast<int>(SleConnectReason::CONNECT_FAIL_NO_AVAILABLE_SERVICE), NotifyReason(0));

    // 业务连接中：走异常断连，回调断ACB并上报断连
    calls_->notify.clear();
    InsertInst(kAddrM, SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING);
    manager_->ProcessProfileDisconnected(PROFILE_NAME_BAS, RawAddress(kAddrM));
    EXPECT_EQ(1, calls_->disconnectAcb);
    ASSERT_EQ(1U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), NotifyNewState(0));
    EXPECT_EQ(static_cast<int>(SleConnectReason::CONNECT_LOCAL_DISCONNECT), NotifyReason(0));

    // 静默盒装：还有已连业务则改为断连中并断下一个
    calls_->notify.clear();
    calls_->disconnectAcb = 0;
    InsertInst(kAddrR, SLE_ADAPTER_PROF_CONN_STATE_CONNECTED);
    auto silentInst = GetInst(kAddrR);
    ASSERT_NE(nullptr, silentInst);
    silentInst->SetSilentPortFlag(true);
    silentInst->SetProfileConnected(PROFILE_NAME_ASC);
    manager_->ProcessProfileDisconnected(PROFILE_NAME_BAS, RawAddress(kAddrR));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_DISCONNECTING, silentInst->GetState());
    EXPECT_EQ(0, calls_->disconnectAcb);

    // 静默盒装且已无存活业务：直接收口并上报
    calls_->notify.clear();
    InsertInst(kAddrU, SLE_ADAPTER_PROF_CONN_STATE_CONNECTED);
    auto silentDone = GetInst(kAddrU);
    ASSERT_NE(nullptr, silentDone);
    silentDone->SetSilentPortFlag(true);
    manager_->ProcessProfileDisconnected(PROFILE_NAME_BAS, RawAddress(kAddrU));
    ASSERT_EQ(1U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), NotifyNewState(0));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_UNUSED, silentDone->GetState());

    // 非静默且无存活业务：清档、通知全断、上报
    calls_->notify.clear();
    InsertInst("D4:E5:F6:07:18:0A", SLE_ADAPTER_PROF_CONN_STATE_CONNECTED);
    const std::string lastAddr = "D4:E5:F6:07:18:0A";
    manager_->ProcessProfileDisconnected(PROFILE_NAME_BAS, RawAddress(lastAddr));
    ASSERT_EQ(1U, calls_->allDiscAddrs.size());
    EXPECT_EQ(lastAddr, calls_->allDiscAddrs[0]);
    // EXPECT_FALSE lastAddr
    ASSERT_EQ(1U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), NotifyNewState(0));
}

/**
 * @tc.name: SleConnectAllProfilePaths_009
 * @tc.desc: 全量连接按设备档案分流：新设备注册应用成功后进入SSAP连接中；
 *           注册或连接失败不建档；旧档案应用号无效或状态不允许时清档或拒绝
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, SleConnectAllProfilePaths_009, TestSize.Level1)
{
    // 新设备全通路径
    ssap_->registerAppId = 10;
    ssap_->connectRet = static_cast<int>(ReturnValue::RET_NO_ERROR);
    manager_->SleConnectAllProfile(RawAddress(kAddrL));
    auto inst = GetInst(kAddrL);
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_SSAP_CONNECTING, inst->GetState());
    EXPECT_EQ(10, inst->GetAppId());
    ASSERT_EQ(1U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTING), NotifyNewState(0));
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), NotifyOldState(0));

    // 应用注册失败：不建档不上报
    ssap_->registerAppId = -1;
    manager_->SleConnectAllProfile(RawAddress(kAddrR));
    EXPECT_FALSE(HasInst(kAddrR));
    EXPECT_EQ(1U, calls_->notify.size());

    // 应用连接失败：不建档
    ssap_->registerAppId = 11;
    ssap_->connectRet = static_cast<int>(ReturnValue::RET_BAD_STATUS);
    manager_->SleConnectAllProfile(RawAddress(kAddrR));
    EXPECT_FALSE(HasInst(kAddrR));

    // 旧档案应用号无效：清档
    ssap_->registerAppId = 12;
    ssap_->connectRet = static_cast<int>(ReturnValue::RET_NO_ERROR);
    auto badAppId = MakeInst(kAddrM, SLE_ADAPTER_PROF_CONN_STATE_UNUSED);
    badAppId->SetAppId(-1);
    manager_->pimpl->SleProfileConnectInstSafeList_.EnsureInsert(kAddrM, badAppId);
    manager_->SleConnectAllProfile(RawAddress(kAddrM));
    EXPECT_FALSE(HasInst(kAddrM));

    // 旧档案状态不允许：拒绝重连（应用号须有效才能走到状态判断）
    InsertInst(kAddrU, SLE_ADAPTER_PROF_CONN_STATE_CONNECTED);
    GetInst(kAddrU)->SetAppId(30);
    manager_->SleConnectAllProfile(RawAddress(kAddrU));
    auto busy = GetInst(kAddrU);
    ASSERT_NE(nullptr, busy);
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_CONNECTED, busy->GetState());

    // 旧档案正常重连：回到SSAP连接中
    const std::string retryAddr = "D4:E5:F6:07:18:0B";
    auto retry = std::make_shared<SleProfileConnectManager::SleProfileConnectInst>(RawAddress(retryAddr));
    manager_->pimpl->SleProfileConnectInstSafeList_.EnsureInsert(retryAddr, retry);
    retry->SetAppId(20);
    manager_->SleConnectAllProfile(RawAddress(retryAddr));
    auto retryInst = GetInst(retryAddr);
    ASSERT_NE(nullptr, retryInst);
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_SSAP_CONNECTING, retryInst->GetState());

    // 旧档案重连失败：清档
    ssap_->connectRet = static_cast<int>(ReturnValue::RET_BAD_STATUS);
    const std::string failAddr = "E5:F6:07:18:29:0C";
    manager_->pimpl->SleProfileConnectInstSafeList_.EnsureInsert(failAddr, MakeInst(failAddr));
    GetInst(failAddr)->SetAppId(21);
    manager_->SleConnectAllProfile(RawAddress(failAddr));
    EXPECT_FALSE(HasInst(failAddr));
}

/**
 * @tc.name: NotifyConnectAcbPaths_010
 * @tc.desc: ACB连接通知按设备档案分流：新设备建档进入ACB连接中（注册失败不建档），
 *           已有档案直接上报连接中
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, NotifyConnectAcbPaths_010, TestSize.Level1)
{
    ssap_->registerAppId = 30;
    manager_->NotifyConnectAcb(RawAddress(kAddrL));
    auto inst = GetInst(kAddrL);
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING, inst->GetState());
    EXPECT_EQ(30, inst->GetAppId());
    ASSERT_EQ(1U, calls_->notify.size());

    ssap_->registerAppId = -1;
    manager_->NotifyConnectAcb(RawAddress(kAddrR));
    EXPECT_FALSE(HasInst(kAddrR));
    EXPECT_EQ(1U, calls_->notify.size());

    manager_->NotifyConnectAcb(RawAddress(kAddrL));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING, inst->GetState());
    EXPECT_EQ(2U, calls_->notify.size());
}

/**
 * @tc.name: NotifyAcbDisconnectedPaths_011
 * @tc.desc: ACB断连通知：档案在且处于ACB连接中则收口为空闲，音频设备额外触发全断处理；
 *           无档案时直接上报断连
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, NotifyAcbDisconnectedPaths_011, TestSize.Level1)
{
    InsertInst(kAddrL, SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING);
    InsertAudioDevice(kAddrL, false, static_cast<int>(SleConnDirect::SLE_CONNECTION_ACTIVE));
    manager_->NotifyAcbDisconnected(RawAddress(kAddrL));
    auto inst = GetInst(kAddrL);
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_UNUSED, inst->GetState());
    ASSERT_EQ(1U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), NotifyNewState(0));
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTING), NotifyOldState(0));
    EXPECT_TRUE(calls_->allDiscAddrs.empty());

    // 音频设备：额外触发全断处理
    InsertInst(kAddrM, SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING);
    InsertAudioDevice(kAddrM, true, static_cast<int>(SleConnDirect::SLE_CONNECTION_ACTIVE));
    manager_->NotifyAcbDisconnected(RawAddress(kAddrM));
    ASSERT_EQ(1U, calls_->allDiscAddrs.size());
    EXPECT_EQ(kAddrM, calls_->allDiscAddrs[0]);

    // 无档案：仅上报
    calls_->notify.clear();
    manager_->NotifyAcbDisconnected(RawAddress(kAddrU));
    ASSERT_EQ(1U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), NotifyNewState(0));
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), NotifyOldState(0));
}

/**
 * @tc.name: SsapConnStateChangedPaths_012
 * @tc.desc: SSAP连接状态变化：车钥匙设备跳过服务发现直接判定连接成功；
 *           其他设备发起发现（发现失败回调断ACB，成功进入发现中）；
 *           断连且无存活业务时清档，等待对端期间例外
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, SsapConnStateChangedPaths_012, TestSize.Level1)
{
    // 车钥匙：跳过发现
    InsertInst(kAddrL, SLE_ADAPTER_PROF_CONN_STATE_SSAP_CONNECTING);
    SetAppearance(kAddrL, DEVICE_CLASS_VEHICLE_LOCK);
    auto carKey = GetInst(kAddrL);
    ASSERT_NE(nullptr, carKey);
    manager_->SsapConnectionStateChangedTask(RawAddress(kAddrL), static_cast<uint8_t>(SleConnectState::CONNECTED));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_CONNECTED, carKey->GetState());
    EXPECT_EQ(0, ssap_->discoverCount);
    ASSERT_EQ(1U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTED), NotifyNewState(0));

    // 普通设备：发起发现成功进入发现中
    calls_->notify.clear();
    InsertInst(kAddrM, SLE_ADAPTER_PROF_CONN_STATE_SSAP_CONNECTING);
    SetAppearance(kAddrM, 100);
    ssap_->discoverRet = static_cast<int>(ReturnValue::RET_NO_ERROR);
    manager_->SsapConnectionStateChangedTask(RawAddress(kAddrM), static_cast<uint8_t>(SleConnectState::CONNECTED));
    auto discovering = GetInst(kAddrM);
    ASSERT_NE(nullptr, discovering);
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING, discovering->GetState());
    EXPECT_EQ(1, ssap_->discoverCount);

    // 发现失败：回调断ACB，状态不变
    InsertInst(kAddrR, SLE_ADAPTER_PROF_CONN_STATE_SSAP_CONNECTING);
    SetAppearance(kAddrR, 100);
    ssap_->discoverRet = static_cast<int>(ReturnValue::RET_BAD_STATUS);
    manager_->SsapConnectionStateChangedTask(RawAddress(kAddrR), static_cast<uint8_t>(SleConnectState::CONNECTED));
    auto failed = GetInst(kAddrR);
    ASSERT_NE(nullptr, failed);
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_SSAP_CONNECTING, failed->GetState());
    EXPECT_EQ(1, calls_->disconnectAcb);

    // 断连且无存活业务：清档并通知全断
    InsertInst(kAddrU, SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING);
    manager_->SsapConnectionStateChangedTask(
        RawAddress(kAddrU), static_cast<uint8_t>(SleConnectState::DISCONNECTED));
    EXPECT_FALSE(HasInst(kAddrU));
    EXPECT_EQ(1U, calls_->allDiscAddrs.size());

    // 等待对端期间断连：只清档，不触发全断
    InsertInst("F6:07:18:29:3A:0D", SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED);
    const std::string waitAddr = "F6:07:18:29:3A:0D";
    manager_->SsapConnectionStateChangedTask(
        RawAddress(waitAddr), static_cast<uint8_t>(SleConnectState::DISCONNECTED));
    EXPECT_FALSE(HasInst(waitAddr));
    EXPECT_EQ(1U, calls_->allDiscAddrs.size());

    // 还有存活业务：保持档案
    InsertInst("D4:E5:F6:07:18:0A", SLE_ADAPTER_PROF_CONN_STATE_CONNECTED);
    const std::string keepAddr = "D4:E5:F6:07:18:0A";
    auto keepInst = GetInst(keepAddr);
    ASSERT_NE(nullptr, keepInst);
    keepInst->SetProfileConnected(PROFILE_NAME_ASC);
    manager_->SsapConnectionStateChangedTask(
        RawAddress(keepAddr), static_cast<uint8_t>(SleConnectState::DISCONNECTED));
    ASSERT_TRUE(HasInst(keepAddr));
}

/**
 * @tc.name: SsapDiscoveredFailurePaths_013
 * @tc.desc: 服务发现失败按静默盒装标志分流：静默中只收口上报，
 *           非静默走异常断连并停合作集邀请广播
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, SsapDiscoveredFailurePaths_013, TestSize.Level1)
{
    InsertInst(kAddrL, SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING);
    auto silentInst = GetInst(kAddrL);
    ASSERT_NE(nullptr, silentInst);
    silentInst->SetSilentPortFlag(true);
    manager_->SsapServicesDiscoveredTask(RawAddress(kAddrL), static_cast<int>(SsapStatus::SSAP_FAILURE));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_UNUSED, silentInst->GetState());
    ASSERT_EQ(1U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), NotifyNewState(0));
    EXPECT_EQ(static_cast<int>(SleConnectReason::CONNECT_FAIL_SERVICE_DISCOVERY), NotifyReason(0));

    InsertInst(kAddrM, SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING);
    manager_->SsapServicesDiscoveredTask(RawAddress(kAddrM), static_cast<int>(SsapStatus::SSAP_FAILURE));
    EXPECT_EQ(1, calls_->disconnectAcb);
    ASSERT_EQ(2U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), NotifyNewState(1));
}

/**
 * @tc.name: SsapDiscoveredSuccessPaths_014
 * @tc.desc: 服务发现结果处理：仅DIS可用时连DIS并等待对端；发现结果为空视为无可行业务走断连；
 *           普通可用业务就绪后进入业务连接中
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, SsapDiscoveredSuccessPaths_014, TestSize.Level1)
{
    // 仅DIS：等待对端
    InsertInst(kAddrL, SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING);
    auto disOnly = GetInst(kAddrL);
    ASSERT_NE(nullptr, disOnly);
    Service disSvc(1, 1, 1, Uuid::ConvertFrom16Bits(static_cast<uint16_t>(SleUuid::UUID_DEVICE_INFORMATION_SERVICE)));
    ssap_->services = {disSvc};
    manager_->SsapServicesDiscoveredTask(RawAddress(kAddrL), static_cast<int>(SsapStatus::SSAP_SUCCESS));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED, disOnly->GetState());

    // 发现结果为空：无可行业务，进入断连中并回调断ACB
    InsertInst(kAddrM, SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING);
    auto empty = GetInst(kAddrM);
    ASSERT_NE(nullptr, empty);
    ssap_->services.clear();
    manager_->SsapServicesDiscoveredTask(RawAddress(kAddrM), static_cast<int>(SsapStatus::SSAP_SUCCESS));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_DISCONNECTING, empty->GetState());
    EXPECT_EQ(1, calls_->disconnectAcb);
    ASSERT_GE(calls_->notify.size(), 2U);
    // 先报进入断连中，随后异常断连补报断连
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTING), calls_->notify[0][0]);
    EXPECT_EQ(static_cast<int>(SleConnectReason::CONNECT_FAIL_NO_AVAILABLE_SERVICE), calls_->notify[0][2]);
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), calls_->notify[1][0]);

    // HID可用：进入业务连接中
    InsertInst(kAddrR, SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING);
    auto hid = GetInst(kAddrR);
    ASSERT_NE(nullptr, hid);
    Service hidSvc(2, 2, 2, Uuid::ConvertFrom16Bits(static_cast<uint16_t>(SleUuid::SLE_STANDARD_SERVICE_HID_UUID)));
    ssap_->services = {hidSvc};
    SetAppearance(kAddrR, 100);
    manager_->SsapServicesDiscoveredTask(RawAddress(kAddrR), static_cast<int>(SsapStatus::SSAP_SUCCESS));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING, hid->GetState());
    EXPECT_EQ(1, hid->GetSupportedProfileNumInner());
}

/**
 * @tc.name: SsapDiscoveredTwsAndIcce_015
 * @tc.desc: 私有TWS服务只有耳机外观才算可行业务；ICCE服务只有车钥匙外观才算可用
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, SsapDiscoveredTwsAndIcce_015, TestSize.Level1)
{
    Service twsSvc(3, 3, 3, Uuid::ConvertFromString(SLE_UUID_TWS_PROFILE));

    InsertInst(kAddrL, SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING);
    auto earbud = GetInst(kAddrL);
    ASSERT_NE(nullptr, earbud);
    ssap_->services = {twsSvc};
    SetAppearance(kAddrL, DEVICE_IN_EAR_EARPHONE);
    manager_->SsapServicesDiscoveredTask(RawAddress(kAddrL), static_cast<int>(SsapStatus::SSAP_SUCCESS));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING, earbud->GetState());
    EXPECT_EQ(1, earbud->GetSupportedProfileNumInner());

    // 非耳机外观：TWS不可用，仅剩等待对端
    InsertInst(kAddrM, SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING);
    auto phone = GetInst(kAddrM);
    ASSERT_NE(nullptr, phone);
    ssap_->services = {twsSvc};
    SetAppearance(kAddrM, 100);
    manager_->SsapServicesDiscoveredTask(RawAddress(kAddrM), static_cast<int>(SsapStatus::SSAP_SUCCESS));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED, phone->GetState());

    if (OHOS::system::GetBoolParameter("const.nearlink.support.icce", false)) {
        Service icceSvc(4, 4, 4,
            Uuid::ConvertFrom16Bits(static_cast<uint16_t>(SleUuid::SLE_STANDARD_SERVICE_ICCE_UUID)));
        InsertInst(kAddrR, SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING);
        auto carKeyInst = GetInst(kAddrR);
        ssap_->services = {icceSvc};
        SetAppearance(kAddrR, DEVICE_CLASS_VEHICLE_LOCK);
        manager_->SsapServicesDiscoveredTask(RawAddress(kAddrR), static_cast<int>(SsapStatus::SSAP_SUCCESS));
        EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING, carKeyInst->GetState());
    }
}

/**
 * @tc.name: SsapDiscoveredIntersectionAndAudioFlag_016
 * @tc.desc: 服务交集按本机支持表展开成业务清单，音频流服务会把设备标记为音频设备；
 *           本机不支持的服务被忽略
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, SsapDiscoveredIntersectionAndAudioFlag_016, TestSize.Level1)
{
    InsertInst(kAddrL, SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING);
    auto audioDevice = std::make_shared<SlePeripheralDevice>();
    SleRemoteDeviceManager::GetInstance()->AddPeripheralDevice(kAddrL, audioDevice);
    usedAddrs_.push_back(kAddrL);
    auto inst = GetInst(kAddrL);
    ASSERT_NE(nullptr, inst);

    std::list<Service> services = {
        Service(1, 1, 1, Uuid::ConvertFrom16Bits(static_cast<uint16_t>(SleUuid::SLE_STANDARD_SERVICE_HID_UUID))),
        Service(2, 2, 2, Uuid::ConvertFrom16Bits(static_cast<uint16_t>(SleUuid::UUID_BATTERY_SERVICE))),
        Service(3, 3, 3, Uuid::ConvertFrom16Bits(static_cast<uint16_t>(SleUuid::SLE_STANDARD_ASC_MGMT_UUID))),
        Service(4, 4, 4, Uuid::ConvertFrom16Bits(static_cast<uint16_t>(SleUuid::SLE_STANDARD_SERVICE_CDSM_UUID))),
        Service(5, 5, 5, Uuid::ConvertFrom16Bits(static_cast<uint16_t>(SleUuid::UUID_PORT_PROFILE_SERVICE))),
        Service(6, 6, 6, Uuid::ConvertFrom16Bits(0xFFFF)),
    };
    ssap_->services = services;
    SetAppearance(kAddrL, 100);
    manager_->SsapServicesDiscoveredTask(RawAddress(kAddrL), static_cast<int>(SsapStatus::SSAP_SUCCESS));

    // 本机不认识的服务被忽略，其余进入业务清单
    EXPECT_EQ(5, inst->GetSupportedProfileNumInner());
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING, inst->GetState());
    EXPECT_TRUE(audioDevice->GetIsAudioDeviceFlag());
}

/**
 * @tc.name: ProfileConnTimeoutPaths_017
 * @tc.desc: 连接超时：等待对端期间只断SSAP客户端；其余情况走异常断连
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, ProfileConnTimeoutPaths_017, TestSize.Level1)
{
    const std::string waitAddr = kAddrL;
    auto waitInst = std::make_shared<SleProfileConnectManager::SleProfileConnectInst>(RawAddress(waitAddr));
    manager_->pimpl->SleProfileConnectInstSafeList_.EnsureInsert(waitAddr, waitInst);
    waitInst->SetAppId(50);
    waitInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED);
    waitInst->StopConnTimer();
    manager_->ProfileConnTimeoutTask(RawAddress(waitAddr));
    EXPECT_EQ(1, ssap_->disconnectCount);
    EXPECT_EQ(0, calls_->disconnectAcb);

    auto connecting = std::make_shared<SleProfileConnectManager::SleProfileConnectInst>(RawAddress(kAddrM));
    manager_->pimpl->SleProfileConnectInstSafeList_.EnsureInsert(kAddrM, connecting);
    connecting->SetState(SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING);
    connecting->StopConnTimer();
    manager_->ProfileConnTimeoutTask(RawAddress(kAddrM));
    EXPECT_EQ(1, calls_->disconnectAcb);
}

/**
 * @tc.name: ProfileConnectInfoQueryAndClear_018
 * @tc.desc: 连接档案的查询族按有无档案返回默认值；清理接口按档案有无分别收口，
 *           全量清理只通知非空闲档案
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, ProfileConnectInfoQueryAndClear_018, TestSize.Level1)
{
    const RawAddress unknown(kAddrU);
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_UNUSED, manager_->GetProfileConnectState(unknown));
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), manager_->GetProfileConnState(unknown));
    EXPECT_EQ(0, manager_->GetConnectedProfileNum(unknown));
    EXPECT_FALSE(manager_->IsAllProfileConnected(unknown));
    EXPECT_FALSE(manager_->GetSilentPortState(unknown));
    manager_->SetSilentPortState(unknown, true);
    manager_->StopTimerIfRePair(unknown);
    manager_->ClearProfileConnectInfo(unknown);
    EXPECT_TRUE(calls_->notify.empty());

    // 有档案：查询如实、静默口可写、清理会上报并移除
    InsertInst(kAddrL, SLE_ADAPTER_PROF_CONN_STATE_CONNECTED);
    auto inst = GetInst(kAddrL);
    ASSERT_NE(nullptr, inst);
    inst->AddSupportProfile(PROFILE_NAME_ASC);
    inst->SetProfileConnected(PROFILE_NAME_ASC);
    const RawAddress known(kAddrL);
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_CONNECTED, manager_->GetProfileConnectState(known));
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTED), manager_->GetProfileConnState(known));
    EXPECT_EQ(1, manager_->GetConnectedProfileNum(known));
    EXPECT_TRUE(manager_->IsAllProfileConnected(known));
    manager_->SetSilentPortState(known, true);
    EXPECT_TRUE(manager_->GetSilentPortState(known));
    manager_->StopTimerIfRePair(known);

    ssap_->registerAppId = 60;
    manager_->ClearProfileConnectInfo(known);
    EXPECT_FALSE(HasInst(kAddrL));
    EXPECT_EQ(1, ssap_->deregisterCount);
    ASSERT_EQ(1U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), NotifyNewState(0));

    // 无业务档案：不算全连
    InsertInst(kAddrM, SLE_ADAPTER_PROF_CONN_STATE_CONNECTED);
    auto noSupport = GetInst(kAddrM);
    ASSERT_NE(nullptr, noSupport);
    EXPECT_FALSE(manager_->IsAllProfileConnected(RawAddress(kAddrM)));
}

/**
 * @tc.name: ClearAllAndSilentPortDisconnect_019
 * @tc.desc: 全量清理只对非空闲档案上报断连；静默盒装断全量业务时还有已连业务则逐个断
 * @tc.type: FUNC
 */
HWTEST_F(SleProfileConnectTest, ClearAllAndSilentPortDisconnect_019, TestSize.Level1)
{
    InsertInst(kAddrL, SLE_ADAPTER_PROF_CONN_STATE_UNUSED);
    InsertInst(kAddrM, SLE_ADAPTER_PROF_CONN_STATE_CONNECTED);
    InsertInst(kAddrR, SLE_ADAPTER_PROF_CONN_STATE_CONNECTED);
    manager_->ClearAllProfileConnectInfo();
    EXPECT_EQ(2U, calls_->notify.size());
    EXPECT_FALSE(HasInst(kAddrM));

    // 静默盒装：无已连业务直接返回
    InsertInst(kAddrL, SLE_ADAPTER_PROF_CONN_STATE_CONNECTED);
    manager_->SleDisconnectAllProfileForSilentPort(RawAddress(kAddrL));
    auto idle = GetInst(kAddrL);
    ASSERT_NE(nullptr, idle);
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_CONNECTED, idle->GetState());

    // 有已连业务：进入断连中并标记静默口
    idle->SetProfileConnected(PROFILE_NAME_ASC);
    manager_->SleDisconnectAllProfileForSilentPort(RawAddress(kAddrL));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_DISCONNECTING, idle->GetState());
    EXPECT_TRUE(idle->GetSilentPortFlag());
}

// ===========================================================================
// 套件二：ProfileServiceManager（业务启停编排 + 查询）
// ===========================================================================
class ProfileServiceManagerTest : public testing::Test {
public:
    void SetUp() override
    {
        SyncThreadsForTest();
        ClearRegistry();
    }

    void TearDown() override
    {
        ClearRegistry();
        ThreadUtil::GetInstance().InitThreadStateMap();
    }

    FakeProfile *NewFake(const std::string &name, int state, uint8_t stateMask = 0)
    {
        auto fake = std::make_unique<FakeProfile>(stateMask);
        FakeProfile *raw = fake.get();
        fakes_.push_back(std::move(fake));
        Psm().pimpl->startedProfiles_.SetProfile(SleTransport::ADAPTER_SLE, name, raw);
        Psm().pimpl->profilesState_.SetProfile(SleTransport::ADAPTER_SLE, name, static_cast<ServiceStateID>(state));
        return raw;
    }

    static ProfileServiceManager &Psm()
    {
        return ProfileServiceManager::GetInstance();
    }

    static void ClearRegistry()
    {
        Psm().pimpl->startedProfiles_.Clear();
        Psm().pimpl->profilesState_.Clear();
    }

    std::vector<std::unique_ptr<FakeProfile>> fakes_;
};

/**
 * @tc.name: ProfileEnableAndAllEnabled_001
 * @tc.desc: 业务使能：无已启动业务时直接报完成；有业务处于关闭态则先逐个拉起再判断
 * @tc.type: FUNC
 */
HWTEST_F(ProfileServiceManagerTest, ProfileEnableAndAllEnabled_001, TestSize.Level1)
{
    // 空表：全使能成立，直接上报完成
    EXPECT_TRUE(Psm().IsAllEnabled(SleTransport::ADAPTER_SLE));
    EXPECT_TRUE(Psm().Enable(SleTransport::ADAPTER_SLE));

    auto hid = NewFake(PROFILE_NAME_HID_HOST, static_cast<int>(ServiceStateID::TURN_OFF));
    EXPECT_FALSE(Psm().IsAllEnabled(SleTransport::ADAPTER_SLE));
    EXPECT_TRUE(Psm().Enable(SleTransport::ADAPTER_SLE));
    EXPECT_EQ(1, hid->context_.enableCount);

    // 只有启动表没有状态表：既不算全使能也不算全停用
    ClearRegistry();
    fakes_.clear();
    auto orphan = std::make_unique<FakeProfile>();
    fakes_.push_back(std::move(orphan));
    FakeProfile *orphanRaw = static_cast<FakeProfile *>(fakes_.back().get());
    Psm().pimpl->startedProfiles_.SetProfile(SleTransport::ADAPTER_SLE, PROFILE_NAME_HID_HOST, orphanRaw);
    EXPECT_FALSE(Psm().IsAllEnabled(SleTransport::ADAPTER_SLE));
    EXPECT_FALSE(Psm().IsAllDisabled(SleTransport::ADAPTER_SLE));
}

/**
 * @tc.name: ProfileOnAllEnabledAndTurning_002
 * @tc.desc: 全使能完成把该通道全部业务置为开启；翻转判定按是否还有中间态业务给出
 * @tc.type: FUNC
 */
HWTEST_F(ProfileServiceManagerTest, ProfileOnAllEnabledAndTurning_002, TestSize.Level1)
{
    NewFake(PROFILE_NAME_HID_HOST, static_cast<int>(ServiceStateID::TURNING_ON));
    EXPECT_TRUE(Psm().IsProfilesTurning(SleTransport::ADAPTER_SLE));

    Psm().OnAllEnabled(SleTransport::ADAPTER_SLE);
    EXPECT_FALSE(Psm().IsProfilesTurning(SleTransport::ADAPTER_SLE));
    EXPECT_TRUE(Psm().IsAllEnabled(SleTransport::ADAPTER_SLE));

    // 空表：翻转判定为否，全使能完成不做动作
    ClearRegistry();
    fakes_.clear();
    EXPECT_FALSE(Psm().IsProfilesTurning(SleTransport::ADAPTER_SLE));
    Psm().OnAllEnabled(SleTransport::ADAPTER_SLE);
}

/**
 * @tc.name: ProfileEnableProfilesInherit_003
 * @tc.desc: 拉起业务继承另一通道状态：对端已开直接置开、对端关中先挂起、
 *           对端开中同步开中、对端未知则正常走开启流程
 * @tc.type: FUNC
 */
HWTEST_F(ProfileServiceManagerTest, ProfileEnableProfilesInherit_003, TestSize.Level1)
{
    const SleTransport other = SleTransport::ADAPTER_SLB;
    const SleTransport self = SleTransport::ADAPTER_SLE;

    auto onOther = NewFake(PROFILE_NAME_HID_HOST, static_cast<int>(ServiceStateID::TURN_OFF));
    Psm().pimpl->profilesState_.SetProfile(other, PROFILE_NAME_HID_HOST, ServiceStateID::TURN_ON);
    Psm().EnableProfiles(self);
    ServiceStateID state = ServiceStateID::TURN_OFF;
    ASSERT_TRUE(Psm().pimpl->profilesState_.Find(self, PROFILE_NAME_HID_HOST, state));
    EXPECT_EQ(ServiceStateID::TURN_ON, state);
    EXPECT_EQ(0, onOther->context_.enableCount);

    Psm().pimpl->profilesState_.SetProfile(other, PROFILE_NAME_HID_HOST, ServiceStateID::TURNING_OFF);
    Psm().EnableProfiles(self);
    ASSERT_TRUE(Psm().pimpl->profilesState_.Find(self, PROFILE_NAME_HID_HOST, state));
    EXPECT_EQ(ServiceStateID::WAIT_TURN_ON, state);

    Psm().pimpl->profilesState_.SetProfile(other, PROFILE_NAME_HID_HOST, ServiceStateID::TURNING_ON);
    Psm().EnableProfiles(self);
    ASSERT_TRUE(Psm().pimpl->profilesState_.Find(self, PROFILE_NAME_HID_HOST, state));
    EXPECT_EQ(ServiceStateID::TURNING_ON, state);

    // 对端无记录：正常开启
    ClearRegistry();
    fakes_.clear();
    auto fresh = NewFake(PROFILE_NAME_HID_HOST, static_cast<int>(ServiceStateID::TURN_OFF));
    Psm().EnableProfiles(self);
    EXPECT_EQ(1, fresh->context_.enableCount);
    ASSERT_TRUE(Psm().pimpl->profilesState_.Find(self, PROFILE_NAME_HID_HOST, state));
    EXPECT_EQ(ServiceStateID::TURNING_ON, state);

    // 已开启的业务不重复拉起
    Psm().EnableProfiles(self);
    EXPECT_EQ(1, fresh->context_.enableCount);
}

/**
 * @tc.name: ProfileEnableComplete_004
 * @tc.desc: 业务使能完成回调：双通道都在开启中则一起收口，未在开启中的通道不受影响，
 *           返回失败的业务回落关闭态
 * @tc.type: FUNC
 */
HWTEST_F(ProfileServiceManagerTest, ProfileEnableComplete_004, TestSize.Level1)
{
    auto hid = NewFake(PROFILE_NAME_HID_HOST, static_cast<int>(ServiceStateID::TURNING_ON));
    Psm().pimpl->profilesState_.SetProfile(
        SleTransport::ADAPTER_SLB, PROFILE_NAME_HID_HOST, ServiceStateID::TURNING_ON);

    Psm().OnEnable(PROFILE_NAME_HID_HOST, true);
    ServiceStateID state = ServiceStateID::TURN_OFF;
    ASSERT_TRUE(Psm().pimpl->profilesState_.Find(SleTransport::ADAPTER_SLE, PROFILE_NAME_HID_HOST, state));
    EXPECT_EQ(ServiceStateID::TURN_ON, state);
    ASSERT_TRUE(Psm().pimpl->profilesState_.Find(SleTransport::ADAPTER_SLB, PROFILE_NAME_HID_HOST, state));
    EXPECT_EQ(ServiceStateID::TURN_ON, state);

    // 未在开启中：忽略
    Psm().OnEnable(PROFILE_NAME_HID_HOST, false);
    ASSERT_TRUE(Psm().pimpl->profilesState_.Find(SleTransport::ADAPTER_SLE, PROFILE_NAME_HID_HOST, state));
    EXPECT_EQ(ServiceStateID::TURN_ON, state);

    // 失败回落
    auto mic = NewFake(PROFILE_NAME_MIC, static_cast<int>(ServiceStateID::TURNING_ON));
    Psm().OnEnable(PROFILE_NAME_MIC, false);
    ASSERT_TRUE(Psm().pimpl->profilesState_.Find(SleTransport::ADAPTER_SLE, PROFILE_NAME_MIC, state));
    EXPECT_EQ(ServiceStateID::TURN_OFF, state);
}

/**
 * @tc.name: ProfileDisableFlows_005
 * @tc.desc: 业务停用：无启动业务直接报完成；开启中的业务先转关中再停用上下文；
 *           对端通道仍在用的业务直接置关不调停用
 * @tc.type: FUNC
 */
HWTEST_F(ProfileServiceManagerTest, ProfileDisableFlows_005, TestSize.Level1)
{
    EXPECT_TRUE(Psm().IsAllDisabled(SleTransport::ADAPTER_SLE));
    EXPECT_TRUE(Psm().Disable(SleTransport::ADAPTER_SLE));

    auto hid = NewFake(PROFILE_NAME_HID_HOST, static_cast<int>(ServiceStateID::TURN_ON));
    EXPECT_FALSE(Psm().IsAllDisabled(SleTransport::ADAPTER_SLE));
    EXPECT_TRUE(Psm().Disable(SleTransport::ADAPTER_SLE));
    EXPECT_EQ(1, hid->context_.disableCount);

    // 对端仍在用：直接置关，不停用上下文
    ClearRegistry();
    fakes_.clear();
    auto shared = NewFake(PROFILE_NAME_HID_HOST, static_cast<int>(ServiceStateID::TURN_ON));
    Psm().pimpl->profilesState_.SetProfile(
        SleTransport::ADAPTER_SLB, PROFILE_NAME_HID_HOST, ServiceStateID::TURN_ON);
    Psm().DisableProfiles(SleTransport::ADAPTER_SLE);
    EXPECT_EQ(0, shared->context_.disableCount);
    ServiceStateID state = ServiceStateID::TURN_ON;
    ASSERT_TRUE(Psm().pimpl->profilesState_.Find(SleTransport::ADAPTER_SLE, PROFILE_NAME_HID_HOST, state));
    EXPECT_EQ(ServiceStateID::TURN_OFF, state);

    // 已关闭的业务不停用
    ClearRegistry();
    fakes_.clear();
    auto off = NewFake(PROFILE_NAME_HID_HOST, static_cast<int>(ServiceStateID::TURN_OFF));
    Psm().DisableProfiles(SleTransport::ADAPTER_SLE);
    EXPECT_EQ(0, off->context_.disableCount);
}

/**
 * @tc.name: ProfileDisableCompleteAndWaitEnable_006
 * @tc.desc: 业务停用完成回调把状态收口为关闭，另一通道挂起等待的业务随即重新拉起；
 *           全停后状态表可整体复位
 * @tc.type: FUNC
 */
HWTEST_F(ProfileServiceManagerTest, ProfileDisableCompleteAndWaitEnable_006, TestSize.Level1)
{
    auto hid = NewFake(PROFILE_NAME_HID_HOST, static_cast<int>(ServiceStateID::TURNING_OFF));
    Psm().pimpl->profilesState_.SetProfile(
        SleTransport::ADAPTER_SLB, PROFILE_NAME_HID_HOST, ServiceStateID::WAIT_TURN_ON);
    Psm().pimpl->startedProfiles_.SetProfile(SleTransport::ADAPTER_SLB, PROFILE_NAME_HID_HOST, hid);

    Psm().OnDisable(PROFILE_NAME_HID_HOST, true);
    ServiceStateID state = ServiceStateID::TURN_ON;
    ASSERT_TRUE(Psm().pimpl->profilesState_.Find(SleTransport::ADAPTER_SLE, PROFILE_NAME_HID_HOST, state));
    EXPECT_EQ(ServiceStateID::TURN_OFF, state);
    ASSERT_TRUE(Psm().pimpl->profilesState_.Find(SleTransport::ADAPTER_SLB, PROFILE_NAME_HID_HOST, state));
    EXPECT_EQ(ServiceStateID::TURNING_ON, state);
    EXPECT_EQ(1, hid->context_.enableCount);

    Psm().OnAllDisabled(SleTransport::ADAPTER_SLE);
    ASSERT_TRUE(Psm().pimpl->profilesState_.Find(SleTransport::ADAPTER_SLE, PROFILE_NAME_HID_HOST, state));
    EXPECT_EQ(ServiceStateID::TURN_OFF, state);

    // 空表：复位无动作
    ClearRegistry();
    fakes_.clear();
    Psm().OnAllDisabled(SleTransport::ADAPTER_SLE);
}

/**
 * @tc.name: ProfileQueryConnectState_007
 * @tc.desc: 业务连接态查询按状态掩码给出已连/连接中/断连中/断连；聚合查询排除SSAP两业务，
 *           任一业务已连即整体已连
 * @tc.type: FUNC
 */
HWTEST_F(ProfileServiceManagerTest, ProfileQueryConnectState_007, TestSize.Level1)
{
    uint32_t hidId = 0;
    for (auto &info : SupportProfilesInfo::GetSupportProfiles()) {
        if (info.mName == PROFILE_NAME_HID_HOST) {
            hidId = info.mId;
        }
    }
    ASSERT_NE(0U, hidId);

    EXPECT_EQ(SleConnectState::DISCONNECTED, Psm().GetProfileServiceConnectState(hidId));
    EXPECT_EQ(SleConnectState::DISCONNECTED, Psm().GetProfileServiceConnectState(0xFFF));
    EXPECT_EQ(SleConnectState::DISCONNECTED, Psm().GetProfileServicesConnectState());

    auto connected = NewFake(PROFILE_NAME_HID_HOST, static_cast<int>(ServiceStateID::TURN_ON),
        PROFILE_STATE_CONNECTED);
    EXPECT_EQ(SleConnectState::CONNECTED, Psm().GetProfileServiceConnectState(hidId));
    connected->stateMask_ = PROFILE_STATE_CONNECTING;
    EXPECT_EQ(SleConnectState::CONNECTING, Psm().GetProfileServiceConnectState(hidId));
    connected->stateMask_ = PROFILE_STATE_DISCONNECTING;
    EXPECT_EQ(SleConnectState::DISCONNECTING, Psm().GetProfileServiceConnectState(hidId));
    connected->stateMask_ = 0;
    EXPECT_EQ(SleConnectState::DISCONNECTED, Psm().GetProfileServiceConnectState(hidId));

    // 聚合：SSAP已连不参与聚合
    auto ssapClient = NewFake(PROFILE_NAME_SSAP_CLIENT, static_cast<int>(ServiceStateID::TURN_ON),
        PROFILE_STATE_CONNECTED);
    EXPECT_EQ(SleConnectState::DISCONNECTED, Psm().GetProfileServicesConnectState());
    ssapClient->stateMask_ = PROFILE_STATE_CONNECTING;
    connected->stateMask_ = PROFILE_STATE_CONNECTED;
    EXPECT_EQ(SleConnectState::CONNECTED, Psm().GetProfileServicesConnectState());
    connected->stateMask_ = PROFILE_STATE_CONNECTING;
    EXPECT_EQ(SleConnectState::CONNECTING, Psm().GetProfileServicesConnectState());
}

/**
 * @tc.name: ProfileQueryListAndUuids_008
 * @tc.desc: 业务ID清单只含已启动业务；本机支持服务号清单收集开启态业务的UUID并去重
 * @tc.type: FUNC
 */
HWTEST_F(ProfileServiceManagerTest, ProfileQueryListAndUuids_008, TestSize.Level1)
{
    NewFake(PROFILE_NAME_HID_HOST, static_cast<int>(ServiceStateID::TURN_ON));
    NewFake(PROFILE_NAME_MIC, static_cast<int>(ServiceStateID::TURN_ON));

    auto list = Psm().GetProfileServicesList();
    auto hasName = [&list](const std::string &name) {
        for (auto id : list) {
            if (SupportProfilesInfo::IdToName(id) == name) {
                return true;
            }
        }
        return false;
    };
    EXPECT_TRUE(hasName(PROFILE_NAME_HID_HOST));
    EXPECT_TRUE(hasName(PROFILE_NAME_MIC));
    EXPECT_FALSE(hasName(PROFILE_NAME_ASC));

    std::vector<std::string> uuids;
    Psm().GetProfileServicesSupportedUuids(uuids);
    std::string hidUuid;
    std::string micUuid;
    for (auto &info : SupportProfilesInfo::GetSupportProfiles()) {
        if (info.mName == PROFILE_NAME_HID_HOST) {
            hidUuid = info.mUuid;
        }
        if (info.mName == PROFILE_NAME_MIC) {
            micUuid = info.mUuid;
        }
    }
    EXPECT_NE(uuids.end(), std::find(uuids.begin(), uuids.end(), hidUuid));
    EXPECT_NE(uuids.end(), std::find(uuids.begin(), uuids.end(), micUuid));
    EXPECT_EQ(2U, uuids.size());

    // 关闭态业务与重复UUID不进清单
    NewFake(PROFILE_NAME_DIS, static_cast<int>(ServiceStateID::TURN_OFF));
    uuids.clear();
    Psm().GetProfileServicesSupportedUuids(uuids);
    EXPECT_EQ(2U, uuids.size());
}

/**
 * @tc.name: ProfileSystemSupportServices_009
 * @tc.desc: 本机支持服务集合按系统配置展开：基础业务恒在，音频业务随音频支持位，
 *           车钥匙业务随系统参数
 * @tc.type: FUNC
 */
HWTEST_F(ProfileServiceManagerTest, ProfileSystemSupportServices_009, TestSize.Level1)
{
    Psm().SetSystemSupportProfileServices();
    auto supported = Psm().GetSystemSupportProfileServices();
    auto has = [&supported](SleUuid uuid) { return supported.find(uuid) != supported.end(); };
    EXPECT_TRUE(has(SleUuid::SLE_STANDARD_SERVICE_HID_UUID));
    EXPECT_TRUE(has(SleUuid::UUID_BATTERY_SERVICE));
    EXPECT_TRUE(has(SleUuid::UUID_DEVICE_INFORMATION_SERVICE));
    EXPECT_TRUE(has(SleUuid::UUID_PORT_PROFILE_SERVICE));
    EXPECT_TRUE(has(SleUuid::SLE_STANDARD_ASC_MGMT_UUID));
    EXPECT_TRUE(has(SleUuid::SLE_STANDARD_SERVICE_CDSM_UUID));
    EXPECT_TRUE(has(SleUuid::SLE_STANDARD_SERVICE_VCP_UUID));
    EXPECT_TRUE(has(SleUuid::SLE_STANDARD_SERVICE_MIC_UUID));
    EXPECT_EQ(OHOS::system::GetBoolParameter("const.nearlink.support.icce", false),
        has(SleUuid::SLE_STANDARD_SERVICE_ICCE_UUID));
}

/**
 * @tc.name: ProfileStartWithoutAdapter_010
 * @tc.desc: 适配器未创建时启动业务管理器不创建任何业务，GetProfileService 查无返回空
 * @tc.type: FUNC
 */
HWTEST_F(ProfileServiceManagerTest, ProfileStartWithoutAdapter_010, TestSize.Level1)
{
    Psm().Start();
    SleInterfaceProfile *profile = nullptr;
    EXPECT_FALSE(Psm().pimpl->startedProfiles_.Find(PROFILE_NAME_HID_HOST, profile));
    EXPECT_EQ(nullptr, Psm().GetProfileService(PROFILE_NAME_HID_HOST));
}

// ===========================================================================
// 套件三：SleServiceManager（开关参数、卫星限制、状态通知）
// ===========================================================================
class SleServiceManagerTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        // 只创建 adapter 对象，不启动协议栈固件
        SleServiceManager::GetInstance()->CreateAdapters();
        SleConfig::GetInstance().LoadConfigInfo();
    }

    static std::string GetSwitchParam()
    {
        return OHOS::system::GetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "");
    }

    static void SetSatellite(bool on, const std::string &timeText)
    {
        OHOS::system::SetParameter(SATELLITE_SWITCH_SYSTEM_PARAMETER_NAME, on ? "true" : "false");
        OHOS::system::SetParameter(SATELLITE_START_SYSTEM_TIME_NAME, timeText);
    }

    void SetUp() override
    {
        SyncThreadsForTest();
        auto *mgr = SleServiceManager::GetInstance();
        savedSwitchParam_ = GetSwitchParam();
        savedSateSwitch_ = OHOS::system::GetParameter(SATELLITE_SWITCH_SYSTEM_PARAMETER_NAME, "");
        savedSateTime_ = OHOS::system::GetParameter(SATELLITE_START_SYSTEM_TIME_NAME, "");
        savedMpxx_ = OHOS::system::GetParameter(PARAM_NAME_FOR_STEP_MPXX_DIR_MODE, "");
        savedResetService_ = OHOS::system::GetParameter(RESET_SERVICE_SYSTEM_PARAMETER_NAME, "");
        SetSatellite(false, "");

        savedState_ = mgr->pimpl->state_.load();
        savedSysState_ = mgr->pimpl->sysState_;
        savedPolicy_ = mgr->autoConnPolicy_.load();
        savedRestoreNeeded_ = mgr->isRestoreNeeded_.load();
        savedDisableCaller_ = mgr->disableCallerInfo_;
        savedCollaboration_ = SleCollaborationManager::GetInstance().isCollaborationOn_.load();
        SleCollaborationManager::GetInstance().isCollaborationOn_ = false;
        mgr->pimpl->state_ = SleStateID::STATE_TURN_OFF;
        mgr->pimpl->sysState_ = SYS_STATE_STOPPED;

        ASSERT_TRUE(mgr->RegisterStateObserver(observer_));
        ASSERT_TRUE(mgr->RegisterSystemStateObserver(sysObserver_));
    }

    void TearDown() override
    {
        auto *mgr = SleServiceManager::GetInstance();
        mgr->DeregisterStateObserver(observer_);
        mgr->DeregisterSystemStateObserver(sysObserver_);
        mgr->pimpl->state_ = savedState_;
        mgr->pimpl->sysState_ = savedSysState_;
        mgr->autoConnPolicy_ = savedPolicy_;
        mgr->isRestoreNeeded_ = savedRestoreNeeded_;
        mgr->disableCallerInfo_ = savedDisableCaller_;
        SleCollaborationManager::GetInstance().isCollaborationOn_ = savedCollaboration_;
        UnloadSa::GetInstance().StopUnloadNearlinkSaTimer();
        mgr->switchDependency_ = nullptr;
        mgr->pimpl->resetPromise_ = std::promise<void>();
        mgr->pimpl->stopPromise_ = std::promise<void>();
        NearlinkVerificationManager::GetInstance().ClearStrategies();
        SetSatellite(!savedSateSwitch_.empty() && savedSateSwitch_ == "true", savedSateTime_);
        OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, savedSwitchParam_);
        if (!savedMpxx_.empty()) {
            OHOS::system::SetParameter(PARAM_NAME_FOR_STEP_MPXX_DIR_MODE, savedMpxx_);
        }
        if (!savedResetService_.empty()) {
            OHOS::system::SetParameter(RESET_SERVICE_SYSTEM_PARAMETER_NAME, savedResetService_);
        }
        ThreadUtil::GetInstance().InitThreadStateMap();
    }

    FakeAdapterStateObserver observer_;
    FakeSystemStateObserver sysObserver_;
    std::string savedSwitchParam_;
    std::string savedSateSwitch_;
    std::string savedSateTime_;
    std::string savedMpxx_;
    std::string savedResetService_;
    SleStateID savedState_ = SleStateID::STATE_TURN_OFF;
    std::string savedSysState_;
    SleAutoConnectPolicy savedPolicy_ = SleAutoConnectPolicy::AUTO_CONN_GENERAL;
    bool savedRestoreNeeded_ = false;
    SwitchCallerInfo savedDisableCaller_;
    bool savedCollaboration_ = false;
};

/**
 * @tc.name: SwitchStateStable_001
 * @tc.desc: 状态稳定性判定：开/关/半开三个稳态为真，过渡态与越界值为假
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SwitchStateStable_001, TestSize.Level1)
{
    EXPECT_TRUE(SleServiceManager::IsStateStable(SleStateID::STATE_TURN_ON));
    EXPECT_TRUE(SleServiceManager::IsStateStable(SleStateID::STATE_TURN_OFF));
    EXPECT_TRUE(SleServiceManager::IsStateStable(SleStateID::STATE_TURN_HALF));
    EXPECT_FALSE(SleServiceManager::IsStateStable(SleStateID::STATE_TURNING_ON));
    EXPECT_FALSE(SleServiceManager::IsStateStable(SleStateID::STATE_TURNING_OFF));
    EXPECT_FALSE(SleServiceManager::IsStateStable(static_cast<SleStateID>(-1)));
}

/**
 * @tc.name: SwitchParameterParseAndUpdate_002
 * @tc.desc: 开关系统参数解析：1为开、2为半开、其余为关、长度异常回退关；
 *           参数写入按目标态落位，恢复挂起期间关态不覆盖参数
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SwitchParameterParseAndUpdate_002, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "1");
    EXPECT_EQ(SleStateID::STATE_TURN_ON, mgr->GetNearlinkSwitchStateFromSystemParameter());
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "2");
    EXPECT_EQ(SleStateID::STATE_TURN_HALF, mgr->GetNearlinkSwitchStateFromSystemParameter());
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "0");
    EXPECT_EQ(SleStateID::STATE_TURN_OFF, mgr->GetNearlinkSwitchStateFromSystemParameter());
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "bad");
    EXPECT_EQ(SleStateID::STATE_TURN_OFF, mgr->GetNearlinkSwitchStateFromSystemParameter());

    mgr->UpdateNearlinkSwitchSystemParameter(SleTransport::ADAPTER_SLE, SleStateID::STATE_TURN_ON);
    EXPECT_EQ("1", GetSwitchParam());
    mgr->isRestoreNeeded_ = true;
    mgr->UpdateNearlinkSwitchSystemParameter(SleTransport::ADAPTER_SLE, SleStateID::STATE_TURN_OFF);
    EXPECT_EQ("1", GetSwitchParam());
    mgr->isRestoreNeeded_ = false;
    mgr->UpdateNearlinkSwitchSystemParameter(SleTransport::ADAPTER_SLE, SleStateID::STATE_TURN_OFF);
    EXPECT_EQ("0", GetSwitchParam());
    mgr->UpdateNearlinkSwitchSystemParameter(SleTransport::ADAPTER_SLE, SleStateID::STATE_TURN_HALF);
    EXPECT_EQ("2", GetSwitchParam());
    // 目标态与参数一致时不改写；过渡态直接拒绝
    mgr->UpdateNearlinkSwitchSystemParameter(SleTransport::ADAPTER_SLE, SleStateID::STATE_TURN_HALF);
    EXPECT_EQ("2", GetSwitchParam());
}

/**
 * @tc.name: SatelliteRestriction_003
 * @tc.desc: 卫星模式判定：开关串非法或关闭即为不受限；开关开启时按时间窗判定，
 *           过期、未到点、非数字串都不受限，当前时刻在窗内才受限
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SatelliteRestriction_003, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();

    // 开关关闭或非法：不受限
    SetSatellite(false, "0");
    EXPECT_FALSE(mgr->IsSleSwitchRestricted());
    SetSatellite(true, "");
    EXPECT_FALSE(mgr->IsSleSwitchRestricted());
    SetSatellite(true, "abc");
    EXPECT_FALSE(mgr->IsSleSwitchRestricted());

    // 时间窗过期与未到点
    SetSatellite(true, "0");
    EXPECT_FALSE(mgr->IsSleSwitchRestricted());
    SetSatellite(true, std::to_string(GetBootTimeNanoSec() + 1000000000000LL));
    EXPECT_FALSE(mgr->IsSleSwitchRestricted());

    // 当前时刻在窗内：受限
    SetSatellite(true, std::to_string(GetBootTimeNanoSec()));
    EXPECT_TRUE(mgr->IsSleSwitchRestricted());
}

/**
 * @tc.name: SwitchEnableNeeded_004
 * @tc.desc: 允许开星闪判定：受限或特定恢复调用方时不允许，普通调用方允许
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SwitchEnableNeeded_004, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();
    SetSatellite(false, "");
    EXPECT_TRUE(mgr->IsEnableNeeded(0, 0));

    auto strategy = std::make_shared<SwitchControlStubStrategy>(true);
    NearlinkVerificationManager::GetInstance().RegisterStrategy(strategy);
    EXPECT_FALSE(mgr->IsEnableNeeded(0, 0));

    NearlinkVerificationManager::GetInstance().ClearStrategies();
    SetSatellite(true, std::to_string(GetBootTimeNanoSec()));
    EXPECT_FALSE(mgr->IsEnableNeeded(0, 0));
}

/**
 * @tc.name: SwitchEnableInnerStates_005
 * @tc.desc: 开星闪任务按当前态分流：关/半开发起开流程并落参数，已开报无效操作，
 *           过渡态直接成功；受限时拒绝
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SwitchEnableInnerStates_005, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();
    const SwitchCallerInfo caller(0, 0);
    SetSatellite(false, "");
    mgr->pimpl->sysState_ = SYS_STATE_STARTED;
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "0");

    mgr->pimpl->state_ = SleStateID::STATE_TURN_OFF;
    EXPECT_EQ(NL_NO_ERROR,
        mgr->EnableInner(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED, caller,
            SleAutoConnectPolicy::AUTO_CONN_GENERAL));
    EXPECT_EQ("1", GetSwitchParam());

    mgr->pimpl->state_ = SleStateID::STATE_TURN_ON;
    EXPECT_EQ(NL_ERR_INVALID_SWITCH_OPERATION,
        mgr->EnableInner(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED, caller,
            SleAutoConnectPolicy::AUTO_CONN_GENERAL));

    mgr->pimpl->state_ = SleStateID::STATE_TURNING_ON;
    EXPECT_EQ(NL_NO_ERROR,
        mgr->EnableInner(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED, caller,
            SleAutoConnectPolicy::AUTO_CONN_GENERAL));

    SetSatellite(true, std::to_string(GetBootTimeNanoSec()));
    mgr->pimpl->state_ = SleStateID::STATE_TURN_OFF;
    EXPECT_EQ(NL_ERR_INVALID_SWITCH_OPERATION,
        mgr->EnableInner(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED, caller,
            SleAutoConnectPolicy::AUTO_CONN_GENERAL));
}

/**
 * @tc.name: SwitchDisableRouting_006
 * @tc.desc: 关星闪入口：已关或正在关时拒绝；协同关闭走全关任务落参数0，
 *           协同开启走半关任务落参数2
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SwitchDisableRouting_006, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();
    mgr->pimpl->sysState_ = SYS_STATE_STARTED;
    SetSatellite(false, "");

    for (auto state : {SleStateID::STATE_TURN_OFF, SleStateID::STATE_TURNING_OFF,
             SleStateID::STATE_TURNING_HALF_TO_OFF}) {
        mgr->pimpl->state_ = state;
        EXPECT_EQ(NL_ERR_INVALID_SWITCH_OPERATION, mgr->Disable(SleTransport::ADAPTER_SLE,
            SleEventType::SVC_TRIGGERED, SwitchCallerInfo(0, 0)));
    }

    // 协同关闭（默认态）：全关
    mgr->pimpl->state_ = SleStateID::STATE_TURN_ON;
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "1");
    EXPECT_EQ(NL_NO_ERROR, mgr->DisableTask(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED,
        SwitchCallerInfo(0, 0)));
    EXPECT_EQ("0", GetSwitchParam());

    // 协同开启：半关
    SleCollaborationManager::GetInstance().isCollaborationOn_ = true;
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "1");
    EXPECT_EQ(NL_NO_ERROR, mgr->DisableTask(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED,
        SwitchCallerInfo(0, 0)));
    EXPECT_EQ("2", GetSwitchParam());
}

/**
 * @tc.name: SwitchToOffTaskStates_007
 * @tc.desc: 全关任务：一般调用方落参数0，特定恢复调用方挂起恢复标志不落参数；
 *           已关报无效操作，过渡态直接成功
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SwitchToOffTaskStates_007, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();
    const SwitchCallerInfo caller(0, 0);
    SetSatellite(false, "");

    mgr->pimpl->state_ = SleStateID::STATE_TURN_ON;
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "1");
    EXPECT_EQ(NL_NO_ERROR,
        mgr->SwitchToOffTask(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED, caller));
    EXPECT_EQ("0", GetSwitchParam());

    // 特定调用方：保持参数，挂恢复标志
    auto strategy = std::make_shared<SwitchControlStubStrategy>(true);
    NearlinkVerificationManager::GetInstance().RegisterStrategy(strategy);
    mgr->pimpl->state_ = SleStateID::STATE_TURN_ON;
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "1");
    EXPECT_EQ(NL_NO_ERROR,
        mgr->SwitchToOffTask(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED, caller));
    EXPECT_EQ("1", GetSwitchParam());
    EXPECT_TRUE(mgr->isRestoreNeeded_.load());

    NearlinkVerificationManager::GetInstance().ClearStrategies();
    mgr->pimpl->state_ = SleStateID::STATE_TURN_OFF;
    EXPECT_EQ(NL_ERR_INVALID_SWITCH_OPERATION,
        mgr->SwitchToOffTask(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED, caller));

    mgr->pimpl->state_ = SleStateID::STATE_TURNING_OFF;
    EXPECT_EQ(NL_NO_ERROR,
        mgr->SwitchToOffTask(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED, caller));
}

/**
 * @tc.name: SwitchToHalfTaskStates_008
 * @tc.desc: 半关任务按状态分流：关态受限拒绝；开态先转关；关态正常转半开；
 *           已半开报无效操作，过渡态直接成功
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SwitchToHalfTaskStates_008, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();
    const SwitchCallerInfo caller(0, 0);
    mgr->pimpl->sysState_ = SYS_STATE_STARTED;
    SetSatellite(false, "");

    mgr->pimpl->state_ = SleStateID::STATE_TURN_OFF;
    SetSatellite(true, std::to_string(GetBootTimeNanoSec()));
    EXPECT_EQ(NL_ERR_INVALID_SWITCH_OPERATION,
        mgr->SwitchToHalfTask(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED, caller));
    SetSatellite(false, "");

    mgr->pimpl->state_ = SleStateID::STATE_TURN_ON;
    EXPECT_EQ(NL_NO_ERROR,
        mgr->SwitchToHalfTask(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED, caller));
    EXPECT_EQ("2", GetSwitchParam());
    EXPECT_EQ(SleStateID::STATE_TURN_OFF, mgr->pimpl->stateMachine_->GetNextTargetState());

    mgr->pimpl->state_ = SleStateID::STATE_TURN_OFF;
    EXPECT_EQ(NL_NO_ERROR,
        mgr->SwitchToHalfTask(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED, caller));

    mgr->pimpl->state_ = SleStateID::STATE_TURN_HALF;
    EXPECT_EQ(NL_ERR_INVALID_SWITCH_OPERATION,
        mgr->SwitchToHalfTask(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED, caller));

    mgr->pimpl->state_ = SleStateID::STATE_TURNING_ON;
    EXPECT_EQ(NL_NO_ERROR,
        mgr->SwitchToHalfTask(SleTransport::ADAPTER_SLE, SleEventType::SVC_TRIGGERED, caller));
}

/**
 * @tc.name: SwitchRestoreFromParameter_009
 * @tc.desc: 按系统参数恢复：参数为开则重新走开流程，参数为半开且协同开启转半关、
 *           协同关闭改落参数0，其余不恢复
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SwitchRestoreFromParameter_009, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();
    mgr->pimpl->sysState_ = SYS_STATE_STARTED;
    SetSatellite(false, "");

    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "1");
    mgr->pimpl->state_ = SleStateID::STATE_TURN_OFF;
    EXPECT_EQ(NL_NO_ERROR, mgr->RestoreSleFromSystemParameter(
        SleTransport::ADAPTER_SLE, SleEventType::RESTORE_TRIGGERED));

    // 半开 + 协同开启
    SleCollaborationManager::GetInstance().isCollaborationOn_ = true;
    mgr->pimpl->state_ = SleStateID::STATE_TURN_OFF;
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "2");
    EXPECT_EQ(NL_NO_ERROR, mgr->RestoreSleFromSystemParameter(
        SleTransport::ADAPTER_SLE, SleEventType::RESTORE_TRIGGERED));
    EXPECT_EQ("2", GetSwitchParam());

    // 半开 + 协同关闭：参数改落0
    SleCollaborationManager::GetInstance().isCollaborationOn_ = false;
    EXPECT_EQ(NL_ERR_INVALID_SWITCH_OPERATION, mgr->RestoreSleFromSystemParameter(
        SleTransport::ADAPTER_SLE, SleEventType::RESTORE_TRIGGERED));
    EXPECT_EQ("0", GetSwitchParam());

    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "0");
    EXPECT_EQ(NL_ERR_INVALID_SWITCH_OPERATION, mgr->RestoreSleFromSystemParameter(
        SleTransport::ADAPTER_SLE, SleEventType::RESTORE_TRIGGERED));
}

/**
 * @tc.name: SwitchRestoreSwitchStatus_010
 * @tc.desc: 开机恢复：参数为开或半开走参数恢复；参数为关但协同开启则转半关；
 *           无需恢复时挂卸载定时器不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SwitchRestoreSwitchStatus_010, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();
    mgr->pimpl->sysState_ = SYS_STATE_STARTED;
    SetSatellite(false, "");

    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "1");
    mgr->pimpl->state_ = SleStateID::STATE_TURN_OFF;
    mgr->RestoreSwitchStatus();

    // 参数为关 + 协同开启：经延迟任务转半关
    SleCollaborationManager::GetInstance().isCollaborationOn_ = true;
    mgr->pimpl->state_ = SleStateID::STATE_TURN_OFF;
    mgr->pimpl->stateMachine_->SetNextTargetState(SleStateID::STATE_TURN_OFF);
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "0");
    mgr->RestoreSwitchStatus();
    EXPECT_EQ("2", GetSwitchParam());

    // 参数为关 + 协同关闭：不恢复，参数保持为关
    SleCollaborationManager::GetInstance().isCollaborationOn_ = false;
    mgr->pimpl->stateMachine_->SetNextTargetState(SleStateID::STATE_TURN_OFF);
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "0");
    mgr->RestoreSwitchStatus();
    EXPECT_EQ("0", GetSwitchParam());
}

/**
 * @tc.name: SwitchCollaborationAndNeedHalf_011
 * @tc.desc: 协同恢复入口只在关态发起半关；是否需要为协同转半开的判定按协同开关给出
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SwitchCollaborationAndNeedHalf_011, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();
    mgr->pimpl->sysState_ = SYS_STATE_STARTED;
    SetSatellite(false, "");

    mgr->pimpl->state_ = SleStateID::STATE_TURN_OFF;
    mgr->RestoreSleFromCollaboration();
    EXPECT_EQ("2", GetSwitchParam());

    mgr->pimpl->state_ = SleStateID::STATE_TURN_ON;
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "0");
    mgr->RestoreSleFromCollaboration();
    EXPECT_EQ("0", GetSwitchParam());

    EXPECT_FALSE(mgr->IsNearlinkNeedTurnHalfFromCollaboration());
    SleCollaborationManager::GetInstance().isCollaborationOn_ = true;
    EXPECT_TRUE(mgr->IsNearlinkNeedTurnHalfFromCollaboration());
}

/**
 * @tc.name: SwitchDriverLoad_012
 * @tc.desc: 驱动加载回调只在键值都匹配时兑现；等待驱动加载在参数已就绪时立即返回
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SwitchDriverLoad_012, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();
    mgr->DriverLoadStatusChange(PARAM_NAME_FOR_STEP_MPXX_DIR_MODE, "fail");
    mgr->DriverLoadStatusChange("other.key", "success");

    OHOS::system::SetParameter(PARAM_NAME_FOR_STEP_MPXX_DIR_MODE, DRIVER_LOAD_SUCCESS);
    mgr->WaitDriverLoadCompleted();

    mgr->DriverLoadStatusChange(PARAM_NAME_FOR_STEP_MPXX_DIR_MODE, "success");
}

/**
 * @tc.name: SwitchCausationString_013
 * @tc.desc: 开关事件原因串：接口触发的记调用方名，其余记系统原因前缀
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SwitchCausationString_013, TestSize.Level1)
{
    EXPECT_EQ("satellite_service", GetDftSwitchEventCausationString(SleEventType::INTERFACE_TRIGGERED,
        "satellite_service"));
    std::string other = GetDftSwitchEventCausationString(SleEventType::SVC_TRIGGERED, "satellite_service");
    EXPECT_NE(std::string::npos, other.find(GetReasonString(static_cast<int>(SleEventType::SVC_TRIGGERED))));
}

/**
 * @tc.name: SysStateChangeAndExit_014
 * @tc.desc: 系统状态变化：工厂复位中不上报，状态不变不通知，正常变化按开/关通知观察者；
 *           退出态回调按状态兑现对应承诺
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SysStateChangeAndExit_014, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();

    mgr->OnSysStateChange(SYS_STATE_FRESETTING);
    EXPECT_EQ(SYS_STATE_FRESETTING, mgr->GetSysState());
    EXPECT_TRUE(sysObserver_.changes.empty());

    mgr->OnSysStateChange(SYS_STATE_STARTED);
    ASSERT_EQ(1U, sysObserver_.changes.size());
    EXPECT_EQ(SleSystemState::ON, sysObserver_.changes[0]);

    mgr->OnSysStateChange(SYS_STATE_STARTED);
    EXPECT_EQ(1U, sysObserver_.changes.size());

    mgr->OnSysStateChange(SYS_STATE_STOPPING);
    EXPECT_EQ(1U, sysObserver_.changes.size());
    EXPECT_EQ(SYS_STATE_STOPPING, mgr->GetSysState());

    mgr->OnSysStateExit(SYS_STATE_FRESETTING);
    mgr->OnSysStateExit(SYS_STATE_STOPPING);
    mgr->OnSysStateExit(SYS_STATE_STARTED);
}

/**
 * @tc.name: AdapterStateChangeTaskAndNotify_015
 * @tc.desc: 适配器状态变化：状态不变不通知，正常变化通知观察者并驱动系统状态机；
 *           复位期间不通知观察者，开态迁移也可达
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, AdapterStateChangeTaskAndNotify_015, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();
    mgr->pimpl->sysState_ = SYS_STATE_STARTED;
    SetSatellite(false, "");
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "0");

    mgr->pimpl->state_ = SleStateID::STATE_TURN_ON;
    mgr->OnAdapterStateChange(SleTransport::ADAPTER_SLE, SleStateID::STATE_TURN_ON);
    EXPECT_TRUE(observer_.stateChanges.empty());

    mgr->OnAdapterStateChange(SleTransport::ADAPTER_SLE, SleStateID::STATE_TURN_OFF);
    ASSERT_EQ(1U, observer_.stateChanges.size());
    EXPECT_EQ(SleStateID::STATE_TURN_OFF, observer_.stateChanges[0]);
    EXPECT_EQ(SleStateID::STATE_TURN_OFF, mgr->GetState(SleTransport::ADAPTER_SLE));

    // 复位期间：观察者不通知
    mgr->pimpl->sysState_ = SYS_STATE_RESETTING;
    mgr->OnAdapterStateChange(SleTransport::ADAPTER_SLE, SleStateID::STATE_TURN_ON);
    EXPECT_EQ(1U, observer_.stateChanges.size());
    EXPECT_EQ(SleStateID::STATE_TURN_ON, mgr->GetState(SleTransport::ADAPTER_SLE));

    // 开态迁移可达（连接计数由适配器连接流程维护，此处为无连接姿态）
    mgr->pimpl->sysState_ = SYS_STATE_STARTED;
    mgr->pimpl->state_ = SleStateID::STATE_TURN_OFF;
    mgr->OnAdapterStateChange(SleTransport::ADAPTER_SLE, SleStateID::STATE_TURN_ON);
    EXPECT_EQ(2U, observer_.stateChanges.size());
}

/**
 * @tc.name: SysCheckAndStopService_016
 * @tc.desc: 进入关态后的收尾：参数为关或恢复挂起时走卸载，参数为半开关半关回执，
 *           参数为开（开关失败）时写复位参数触发服务重启
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SysCheckAndStopService_016, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();
    mgr->pimpl->sysState_ = SYS_STATE_STARTED;
    SetSatellite(false, "");
    // 卸载计数为进程级累积，按用例内增量断言
    const int unloadBase = g_unloadSaCallCount.load();

    // 参数为关 + 有调用方：回执关并走卸载
    mgr->disableCallerInfo_ = SwitchCallerInfo(5, 100);
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "0");
    mgr->CheckAndStopService();
    ASSERT_EQ(1U, observer_.disableResponses.size());
    EXPECT_FALSE(observer_.disableResponses[0].first);
    EXPECT_EQ(unloadBase + 1, g_unloadSaCallCount.load());

    // 参数为半开：回执半关再卸载（回执为一次性语义，需重设调用方）
    mgr->disableCallerInfo_ = SwitchCallerInfo(6, 100);
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "2");
    mgr->CheckAndStopService();
    EXPECT_EQ(unloadBase + 2, g_unloadSaCallCount.load());
    ASSERT_EQ(2U, observer_.disableResponses.size());
    EXPECT_TRUE(observer_.disableResponses[1].first);

    // 参数为开：写复位参数，不走卸载
    OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, "1");
    mgr->CheckAndStopService();
    EXPECT_EQ("0", OHOS::system::GetParameter(RESET_SERVICE_SYSTEM_PARAMETER_NAME, ""));
    EXPECT_EQ(unloadBase + 2, g_unloadSaCallCount.load());

    // 恢复挂起：走卸载分支
    mgr->isRestoreNeeded_ = true;
    mgr->CheckAndStopService();
    EXPECT_EQ(unloadBase + 3, g_unloadSaCallCount.load());
}

/**
 * @tc.name: SysSendDisableResponse_017
 * @tc.desc: 关星闪回执：无有效调用方不回执，有调用方按半关/关回执且只回一次
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SysSendDisableResponse_017, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();
    mgr->disableCallerInfo_ = SwitchCallerInfo();
    mgr->SendDisableResponse(true);
    EXPECT_TRUE(observer_.disableResponses.empty());

    mgr->disableCallerInfo_ = SwitchCallerInfo(7, 200);
    mgr->SendDisableResponse(true);
    ASSERT_EQ(1U, observer_.disableResponses.size());
    EXPECT_TRUE(observer_.disableResponses[0].first);
    EXPECT_EQ(200, observer_.disableResponses[0].second.callerUid);

    mgr->SendDisableResponse(false);
    EXPECT_EQ(1U, observer_.disableResponses.size());
}

/**
 * @tc.name: SysAssortedEdges_018
 * @tc.desc: 杂项边界：非启动态工厂复位返回失败，配置重载后可清理存储，
 *           驱动复位回调注销按注册状态分流，关停接口在已停/停中不动作，
 *           开关依赖对象创建后可释放
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceManagerTest, SysAssortedEdges_018, TestSize.Level1)
{
    auto *mgr = SleServiceManager::GetInstance();

    EXPECT_FALSE(mgr->FactoryReset());

    mgr->DeregisterDliResetCallback();
    mgr->pimpl->dliFailureCallbacks.dliFailure = SleServiceManager::DliFailedReset;
    mgr->DeregisterDliResetCallback();
    EXPECT_EQ(nullptr, mgr->pimpl->dliFailureCallbacks.dliFailure);

    mgr->pimpl->sysState_ = SYS_STATE_STOPPED;
    mgr->Stop();
    mgr->pimpl->sysState_ = SYS_STATE_STOPPING;
    mgr->Stop();

    mgr->WaitForAllSwitchDependency();
}

}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS
