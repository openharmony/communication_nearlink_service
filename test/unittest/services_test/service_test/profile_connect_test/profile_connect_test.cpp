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
    EXPECT_EQ(PROFILE_NAME_BAS, inst->GetNextConnectedProfile());

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

    // 直接驱动连接业务入口：仅音频流会触发组播安全配置
    manager_->ConnectProfile(PROFILE_NAME_ASC, RawAddress(kAddrL));
    EXPECT_EQ(1, calls_->sendImgSecu);
    manager_->ConnectProfile(PROFILE_NAME_HID_HOST, RawAddress(kAddrL));
    EXPECT_EQ(1, calls_->sendImgSecu);
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
    manager_->ProcessProfileConnected(PROFILE_NAME_DIS, RawAddress(kAddrL));
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING, watchInst->GetState());
    EXPECT_TRUE(calls_->notify.empty());

    // 对端是车钥匙：同样保留
    InsertInst(kAddrM, SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED);
    SetAppearance(kAddrM, DEVICE_CLASS_VEHICLE_LOCK);
    auto carKeyInst = GetInst(kAddrM);
    ASSERT_NE(nullptr, carKeyInst);
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

    // 旧档案状态不允许：拒绝重连
    InsertInst(kAddrU, SLE_ADAPTER_PROF_CONN_STATE_CONNECTED);
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
    manager_->SsapServicesDiscoveredTask(
        RawAddress(kAddrL), static_cast<int>(SsapStatus::SSAP_FAILURE) + 1);
    EXPECT_EQ(SLE_ADAPTER_PROF_CONN_STATE_UNUSED, silentInst->GetState());
    ASSERT_EQ(1U, calls_->notify.size());
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), NotifyNewState(0));
    EXPECT_EQ(static_cast<int>(SleConnectReason::CONNECT_FAIL_SERVICE_DISCOVERY), NotifyReason(0));

    InsertInst(kAddrM, SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING);
    manager_->SsapServicesDiscoveredTask(
        RawAddress(kAddrM), static_cast<int>(SsapStatus::SSAP_FAILURE) + 1);
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
    ASSERT_GE(calls_->notify.size(), 1U);
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTING), calls_->notify.back()[0]);
    EXPECT_EQ(static_cast<int>(SleConnectReason::CONNECT_FAIL_NO_AVAILABLE_SERVICE), calls_->notify.back()[2]);

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

}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS
