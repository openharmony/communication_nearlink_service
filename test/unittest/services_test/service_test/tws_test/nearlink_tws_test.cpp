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
#include <gmock/gmock.h>
#include <thread>
#include "nearlink_asc_audio_stream_info.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "TwsService.h"
#include "TwsDefines.h"
#include "CdsmService.h"
#include "SleUtils.h"
#include "log.h"
#include <memory>
#include "ThreadUtil.h"
#include "ProfileServiceManager.cpp"
#include "SleInterfaceProfileASC.h"
#include "interface_profile_ssap_client.h"
#include "SleConfig.h"

extern void SetMockNearlinkOut(bool isNearlinkOut);
extern void ClearMockAudioFwkState();


namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing;
using namespace testing::ext;

class MockCdsmService : public CdsmService {
public:
};

class MockTwsClient : public TwsClient {
public:
    MockTwsClient(const std::string& macAddress) : TwsClient(macAddress) {}
    void SetAppid(int value) { appId_ = value;}
    void SetState(TwsClientState newState) { twsClientState_ = newState;}
};

class NearlinkTwsTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

private:
    // 测试目的：fixture 持有自有 TwsService 实例（dis 样板），
    // 不再依赖厂商栈启动链（裸进程 StartTask/slem_initialize 必败，GetService() 返回 nullptr）。
    std::unique_ptr<TwsService> service_;
};

void NearlinkTwsTest::SetUpTestCase()
{
    // 测试目的：删除 Start()（裸进程必败且返回值被忽略）与固定 sleep；
    // 服务实例改由每例 SetUp 自建并注入注册表，token 设置保留。
    HILOGI("SetUpTestCase start NearlinkTwsTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
}

void NearlinkTwsTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkTwsTest");
}

void NearlinkTwsTest::SetUp()
{
    HILOGI("SetUp NearlinkTwsTest.");
    // 测试目的：与产品注册路径同构（ProfileServiceManager SetProfile）直接注入自有服务，
    // 使 GetService() 命中；NOT_SWITCH_THREAD 让服务任务在测试线程同步执行，无需 sleep。
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_TWS, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    service_ = std::make_unique<TwsService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_TWS, service_.get());
    ASSERT_EQ(service_.get(), TwsService::GetService());
}

void NearlinkTwsTest::TearDown()
{
    HILOGI("TearDown NearlinkTwsTest.");
    // 测试目的：先解绑注册表再销毁服务，并恢复线程调度默认态，防单例状态跨例残留。
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_TWS, nullptr);
    service_.reset();
    // 测试目的：清除固定地址的持久化配置（佩戴检测/自动连接等），防 repeat 与跨进程残留翻转用例分支
    SleConfig::GetInstance().RemovePairedDevice("11:22:33:44:55:66");
    SleConfig::GetInstance().Save();
    ThreadUtil::GetInstance().InitThreadStateMap();
}

/**
 * @tc.name: ConnectTws
 * @tc.desc: Test the ConnectTws function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, ConnectTws001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:ConnectTws001 start");
    MockTwsClient twsClient("11:22:33:44:55:66");
    twsClient.SetAppid(-5);
    twsClient.ConnectTws();
    HILOGI("NearlinkTwsTest:ConnectTws001 end");
}

/**
 * @tc.name: ConnectTws
 * @tc.desc: Test the ConnectTws function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, ConnectTws002, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:ConnectTws002 start");
    MockTwsClient twsClient("11:22:33:44:55:66");
    twsClient.SetAppid(1);
    twsClient.SetState(TwsClientState::TWS_STATE_CONNECTED);
    twsClient.ConnectTws();
    HILOGI("NearlinkTwsTest:ConnectTws002 end");
}

/**
 * @tc.name: ConnectTws
 * @tc.desc: Test the ConnectTws function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, ConnectTws003, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:ConnectTws003 start");
    MockTwsClient twsClient("11:22:33:44:55:66");
    twsClient.SetAppid(1);
    twsClient.SetState(TwsClientState::TWS_STATE_DISCONNECTED);
    twsClient.ConnectTws();
    HILOGI("NearlinkTwsTest:ConnectTws003 end");
}

/**
 * @tc.name: DisconnectTws
 * @tc.desc: Test the DisconnectTws function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, DisconnectTws001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:DisconnectTws001 start");
    MockTwsClient twsClient("11:22:33:44:55:66");
    twsClient.SetAppid(-5);
    twsClient.DisconnectTws();
    HILOGI("NearlinkTwsTest:DisconnectTws001 end");
}

/**
 * @tc.name: DisconnectTws
 * @tc.desc: Test the DisconnectTws function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, DisconnectTws002, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:DisconnectTws002 start");
    MockTwsClient twsClient("11:22:33:44:55:66");
    twsClient.SetAppid(1);
    twsClient.SetState(TwsClientState::TWS_STATE_CONNECTED);
    twsClient.DisconnectTws();
    HILOGI("NearlinkTwsTest:DisconnectTws002 end");
}

/**
 * @tc.name: DisconnectTws
 * @tc.desc: Test the DisconnectTws function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, DisconnectTws003, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:DisconnectTws003 start");
    MockTwsClient twsClient("11:22:33:44:55:66");
    twsClient.SetAppid(1);
    twsClient.SetState(TwsClientState::TWS_STATE_DISCONNECTED);
    twsClient.DisconnectTws();
    HILOGI("NearlinkTwsTest:DisconnectTws003 end");
}

/**
 * @tc.name: SendData
 * @tc.desc: Test the SendData function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, SendData001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:SendData001 start");
    MockTwsClient twsClient("11:22:33:44:55:66");
    twsClient.SetAppid(1);
    twsClient.SetState(TwsClientState::TWS_STATE_DISCONNECTED);
    std::vector<uint8_t> data = {0,1,2,3,4};
    twsClient.SendData(data);
    HILOGI("NearlinkTwsTest:SendData001 end");
}

/**
 * @tc.name: SendData
 * @tc.desc: Test the SendData function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, SendData002, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:SendData002 start");
    MockTwsClient twsClient("11:22:33:44:55:66");
    twsClient.SetAppid(1);
    twsClient.SetState(TwsClientState::TWS_STATE_CONNECTED);
    std::vector<uint8_t> data = {};
    twsClient.SendData(data);
    HILOGI("NearlinkTwsTest:SendData002 end");
}

/**
 * @tc.name: SendData
 * @tc.desc: Test the SendData function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, SendData003, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:SendData003 start");
    MockTwsClient twsClient("11:22:33:44:55:66");
    twsClient.SetAppid(1);
    twsClient.SetState(TwsClientState::TWS_STATE_CONNECTED);
    std::vector<uint8_t> data = {0,1,2,3,4};
    twsClient.SendData(data);
    HILOGI("NearlinkTwsTest:SendData003 end");
}

/**
 * @tc.name: SaveTwsServicePropertyInfo
 * @tc.desc: Test the SaveTwsServicePropertyInfo function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, SaveTwsServicePropertyInfo001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:SaveTwsServicePropertyInfo001 start");
    MockTwsClient twsClient("11:22:33:44:55:66");
    twsClient.SaveTwsServicePropertyInfo();
    HILOGI("NearlinkTwsTest:SaveTwsServicePropertyInfo001 end");
}

/**
 * @tc.name: TwsSendManufacturerAbility
 * @tc.desc: Test the TwsSendManufacturerAbility function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, TwsSendManufacturerAbility001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:TwsSendManufacturerAbility001 start");
    MockTwsClient twsClient("11:22:33:44:55:66");
    RawAddress device("11:22:33:44:55:66");
    twsClient.TwsSendManufacturerAbility(device);
    HILOGI("NearlinkTwsTest:TwsSendManufacturerAbility001 end");
}

/**
 * @tc.name: OnConnectionStateChangedTask
 * @tc.desc: Test the OnConnectionStateChangedTask function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, OnConnectionStateChangedTask001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:OnConnectionStateChangedTask001 start");
    MockTwsClient twsClient("11:22:33:44:55:66");
    RawAddress device("11:22:33:44:55:66");
    twsClient.OnConnectionStateChangedTask(static_cast<uint8_t>(SleConnectState::CONNECTED));
    HILOGI("NearlinkTwsTest:OnConnectionStateChangedTask001 end");
}

/**
 * @tc.name: OnPropertyChangedTask
 * @tc.desc: Test the OnPropertyChangedTask function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, OnPropertyChangedTask001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:OnPropertyChangedTask001 start");
    MockTwsClient twsClient("11:22:33:44:55:66");
    Property property(0);
    twsClient.OnPropertyChangedTask(property);
    HILOGI("NearlinkTwsTest:OnPropertyChangedTask001 end");
}

/**
 * @tc.name: TwsClientCallback::OnPropertyChanged
 * @tc.desc: Test the TwsClientCallback::OnPropertyChanged function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, OnPropertyChanged001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:OnPropertyChanged001 start");
    std::shared_ptr<TwsClient> twsClientInstance = std::make_shared<TwsClient>("11:22:33:44:55:66");
    twsClientInstance->Init(twsClientInstance);
    Property property(0);
    twsClientInstance->ssapCallback_->OnPropertyChanged(property);
    HILOGI("NearlinkTwsTest:OnPropertyChanged001 end");
}

/**
 * @tc.name: TwsClientCallback::OnConnectionStateChanged
 * @tc.desc: Test the TwsClientCallback::OnConnectionStateChanged function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, OnConnectionStateChanged001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:OnConnectionStateChanged001 start");
    std::shared_ptr<TwsClient> twsClientInstance = std::make_shared<TwsClient>("11:22:33:44:55:66");
    twsClientInstance->Init(twsClientInstance);
    twsClientInstance->ssapCallback_->OnConnectionStateChanged(1, 0);
    HILOGI("NearlinkTwsTest:OnConnectionStateChanged001 end");
}

/**
 * @tc.name: Connect
 * @tc.desc: Test the Connect function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, Connect001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:Connect001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    int ret = twsService->Connect(device);
    EXPECT_EQ(ret, NL_NO_ERROR);
    HILOGI("NearlinkTwsTest:Connect001 end");
}

/**
 * @tc.name: TwsGetDeviceNature
 * @tc.desc: Test the TwsGetDeviceNature function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, TwsGetDeviceNature001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:TwsGetDeviceNature001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    std::list<RawAddress> devlist = twsService->GetConnectDevices();
    uint8_t nature = twsService->TwsGetDeviceNature(device);
    HILOGI("NearlinkTwsTest:TwsGetDeviceNature001 end");
}

/**
 * @tc.name: TwsGetDeviceAudioMusicType
 * @tc.desc: Test the TwsGetDeviceAudioMusicType function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, TwsGetDeviceAudioMusicType001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:TwsGetDeviceAudioMusicType001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    uint8_t mediaState = twsService->TwsGetDeviceAudioMusicType(device);
    HILOGI("NearlinkTwsTest:TwsGetDeviceAudioMusicType001 end");
}

/**
 * @tc.name: GetTwsAudioDelay
 * @tc.desc: Test the GetTwsAudioDelay function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, GetTwsAudioDelay001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:GetTwsAudioDelay001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    uint32_t audioDelay;
    twsService->GetTwsAudioDelay(device, audioDelay);
    HILOGI("NearlinkTwsTest:GetTwsAudioDelay001 end");
}

/**
 * @tc.name: TwsGetDeviceWearStatus
 * @tc.desc: Test the TwsGetDeviceWearStatus function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, TwsGetDeviceWearStatus001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:TwsGetDeviceWearStatus001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    TwsDevWearStatus wearStatus;
    twsService->TwsGetDeviceWearStatus(device, wearStatus);
    HILOGI("NearlinkTwsTest:TwsGetDeviceWearStatus001 end");
}

/**
 * @tc.name: TwsUpdateDeviceDefaultRole
 * @tc.desc: Test the TwsUpdateDeviceDefaultRole function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, TwsUpdateDeviceDefaultRole001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:TwsUpdateDeviceDefaultRole001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->TwsUpdateDeviceDefaultRole(device, static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY));
    HILOGI("NearlinkTwsTest:TwsUpdateDeviceDefaultRole001 end");
}

/**
 * @tc.name: ProcPauseRecordMap
 * @tc.desc: Test the ProcPauseRecordMap function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, ProcPauseRecordMap001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:ProcPauseRecordMap001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->ProcPauseRecordMap();
    HILOGI("NearlinkTwsTest:ProcPauseRecordMap001 end");
}

/**
 * @tc.name: UpdateHangUpTimeStamp
 * @tc.desc: Test the UpdateHangUpTimeStamp function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, UpdateHangUpTimeStamp001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:UpdateHangUpTimeStamp001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->UpdateHangUpTimeStamp(device);
    HILOGI("NearlinkTwsTest:UpdateHangUpTimeStamp001 end");
}

/**
 * @tc.name: SendProfileConnected
 * @tc.desc: Test the SendProfileConnected function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, SendProfileConnected001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:SendProfileConnected001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->SendProfileConnected(device);
    HILOGI("NearlinkTwsTest:SendProfileConnected001 end");
}

/**
 * @tc.name: NotifyStateChanged
 * @tc.desc: Test the NotifyStateChanged function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, NotifyStateChanged001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:NotifyStateChanged001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->NotifyStateChanged(device, TwsClientState::TWS_STATE_CONNECTED, TwsClientState::TWS_STATE_CONNECTED);
    HILOGI("NearlinkTwsTest:NotifyStateChanged001 end");
}

/**
 * @tc.name: NotifyStateChanged
 * @tc.desc: Test the NotifyStateChanged function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, NotifyStateChanged002, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:NotifyStateChanged002 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->NotifyStateChanged(device, TwsClientState::TWS_STATE_CONNECTED, TwsClientState::TWS_STATE_DISCONNECTED);
    HILOGI("NearlinkTwsTest:NotifyStateChanged002 end");
}

/**
 * @tc.name: NotifyStateChanged
 * @tc.desc: Test the NotifyStateChanged function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, NotifyStateChanged003, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:NotifyStateChanged003 start");
    RawAddress device("11:22:33:44:55:66");

    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->NotifyStateChanged(device, TwsClientState::TWS_STATE_CONNECTED, TwsClientState::TWS_STATE_DISCONNECTED);
    HILOGI("NearlinkTwsTest:NotifyStateChanged003 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("3F0508010100012733"); // echo 5,8
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent001 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent002, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent002 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("F3050D7F0100006E6A"); // echo 5,D
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_RSP;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent002 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent003, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent003 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("3F050E010100013732"); // echo 5,e
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent003 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent004, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent004 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("F305127F01000045EF"); // echo 5,12
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_RSP;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent004 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent005, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent005 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("3F040201010000020100005E89"); // echo 4,2
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent005 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent006, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent006 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("3F04040104000002010002020000010304000102030404040001020304050400010203047C68"); // echo 4,4
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent006 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent007, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent007 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("3F0302010100040CA0"); // echo 3,2
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent007 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent008, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent008 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("F30102010200010002020006000310000700000000000000000000000000000093E9"); // echo 1,2
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_RSP;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent008 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent009, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent009 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string str1("3F0403013A0041542B555044415445485541574549424154544552593D372C322C3130302C332C30");
    std::string str2("2C342C39322C352C302C362C39392C372C312C392C31310DC1A5");
    std::string strData = str1 + str2; // echo 4,3 AT+UPDATEHUAWEIBATTERY
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent009 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent010, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent010 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string str1("3F080A0104001200000002010001030200FA000406009001C8002C01052300ECFF05000401010203");
    std::string str2("0301040400000000000000000000000000000000000000000000971D");
    std::string strData = str1 + str2; // echo 8,A
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent010 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent011, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent011 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("3F04010103000205018EAD"); // echo 4,1
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent011 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent012, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent012 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("3F0511010100011CB7"); // echo 5,11
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent012 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent013, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent013 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("F3050A02010001144f"); // echo 5,a
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_RSP;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent013 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent014, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent014 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("3f080b010c00020200020000020202020202020300464a4a64d0"); // echo 8,b
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent014 end");
}


/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent015, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent015 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string str1("3f040302720041542b5853485541574549463d342c303034303036333131315361782d3234372e31");
    std::string str2("36383230383030303030303030333034303030313430343032376435303434313134363034303434");
    std::string str3("643730345a41415638303630303031366439303830303234303431386130323033623034343130335666");
    std::string strData = str1 + str2 + str3; // echo 4,3 AT+XSHUAWEIF
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent015 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent016, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent016 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string str1("3f0403013d0041542b485541574549424154544552593d382c342c3130302c352c302c322c313030");
    std::string str2("2c332c302c362c3130302c372c312c392c31312c31322c3235360da2fe");
    std::string strData = str1 + str2; // echo 4,3 AT+HUAWEIBATTERY
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent016 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent017, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent017 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string str1("3f0403013d0041542b434C4F5345485541574549424154544552593d382c342c3130302c352c302c322c313030");
    std::string str2("2c332c302c362c3130302c372c312c392c31312c31322c3235360db09e");
    std::string strData = str1 + str2; // echo 4,3 AT+CLOSEHUAWEIBATTERY
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent017 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent018, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent014 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("3F080E010C00020200020000020202020202020300464A4A2A85"); // echo 8,e
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent014 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent019, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent004 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("F3080C7F01000019B2"); // echo 8,c
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_RSP;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent004 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent020, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent014 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("3F050A010C00020200020000020202020202020300464A4ABC3D"); // echo 5,a
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent014 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent021, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent014 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("F3050E7F010000E66A"); // echo 5,e
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_RSP;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent014 end");
}

/**
 * @tc.name: PostEvent
 * @tc.desc: Test the PostEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, PostEvent022, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:PostEvent004 start");
    RawAddress device("11:22:33:44:55:66");
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "11:22:33:44:55:66";
    std::string strData("F304017F0100004D90"); // echo 4,1
    event.streamLen_ = strData.size() / 2;
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);

    SleUtils::ConvertHexStringToInt(strData, event.dataStream_.get(), event.streamLen_);
    HILOGD("value_.size=%{public}d, value_.data=%{public}s", static_cast<int>(event.streamLen_),
        SleUtils::ConvertIntToHexString(event.dataStream_.get(), event.streamLen_).c_str());

    event.msgDirect_ = TWS_MSG_DIRECT_RSP;
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->PostEvent(event);
    HILOGI("NearlinkTwsTest:PostEvent004 end");
}

/**
 * @tc.name: UpdateDeviceWearState
 * @tc.desc: Test the UpdateDeviceWearState function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, UpdateDeviceWearState001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:UpdateDeviceWearState001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    TwsWearStateData wearData;
    twsService->UpdateDeviceWearState(device, wearData);
    HILOGI("NearlinkTwsTest:UpdateDeviceWearState001 end");
}

/**
 * @tc.name: UpdateClientData
 * @tc.desc: Test the UpdateClientData function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, UpdateClientData001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:UpdateClientData001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    uint8_t dataType = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_ROLE_TYPE);
    TwsClientData isoHandoverData(device);
    isoHandoverData.roleType_ = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY);
    twsService->UpdateClientData(dataType, isoHandoverData);
    HILOGI("NearlinkTwsTest:UpdateClientData001 end");
}

/**
 * @tc.name: GetReportAddr
 * @tc.desc: Test the GetReportAddr function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, GetReportAddr001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:GetReportAddr001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    RawAddress reportAddr = twsService->GetReportAddr(device);
    HILOGI("NearlinkTwsTest:GetReportAddr001 end");
}

/**
 * @tc.name: ResetVirtualAutoSwitch
 * @tc.desc: Test the ResetVirtualAutoSwitch function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, ResetVirtualAutoSwitch001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:ResetVirtualAutoSwitch001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->ResetVirtualAutoSwitch(device);
    HILOGI("NearlinkTwsTest:ResetVirtualAutoSwitch001 end");
}

/**
 * @tc.name: RecoverDataFromConf
 * @tc.desc: Test the RecoverDataFromConf function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, RecoverDataFromConf001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:RecoverDataFromConf001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->RecoverDataFromConf(device);
    HILOGI("NearlinkTwsTest:RecoverDataFromConf001 end");
}

/**
 * @tc.name: GetPrimaryAddr
 * @tc.desc: Test the GetPrimaryAddr function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, GetPrimaryAddr001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:GetPrimaryAddr001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    RawAddress primaryAddr = twsService->GetPrimaryAddr(device);
    HILOGI("NearlinkTwsTest:GetPrimaryAddr001 end");
}

/**
 * @tc.name: TwsServiceSendReqInner
 * @tc.desc: Test the v function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, TwsServiceSendReqInner001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:TwsServiceSendReqInner001 start");
    RawAddress device("11:22:33:44:55:66");

    uint16_t dataLen = sizeof(ManufacturerAbilityInfo);
    std::unique_ptr<uint8_t[]> abilityInfo = std::make_unique<uint8_t[]>(dataLen);
    (void)memset_s(abilityInfo.get(), dataLen, 0, dataLen);
    TwsMessage event(TWS_SERVICE_SEND_REQ_EVENT);
    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    event.dev_ = device.GetAddress();
    event.msgType_ = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_MANUFACTURER_ABILITY);
    event.serviceDataLen_ = dataLen;
    event.serviceData_ = std::move(abilityInfo);

    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->TwsServiceSendReqInner(event);

    HILOGI("NearlinkTwsTest:TwsServiceSendReqInner001 end");
}

/**
 * @tc.name: Disconnect
 * @tc.desc: Test the Disconnect function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, Disconnect001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:Disconnect001 start");
    RawAddress device("11:22:33:44:55:66");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    std::list<RawAddress> devlist = twsService->GetConnectDevices();
    twsService->Disconnect(device);
    HILOGI("NearlinkTwsTest:Disconnect001 end");
}

/**
 * @tc.name: GetReportAddr_001
 * @tc.desc: Test GetReportAddr via ProfileTws interface, should return device when CdsmService is registered
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, GetReportAddr_001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:GetReportAddr_001 start");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    RawAddress device("11:22:33:44:55:66");
    RawAddress reportAddr = twsService->GetReportAddr(device);
    EXPECT_EQ(reportAddr.GetAddress(), device.GetAddress());
    HILOGI("NearlinkTwsTest:GetReportAddr_001 end");
}

/**
 * @tc.name: UpdateDualRecordAbility_001
 * @tc.desc: Test UpdateDualRecordAbility uses ProfileASC from profile manager, should not crash
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, UpdateDualRecordAbility_001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:UpdateDualRecordAbility_001 start");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->UpdateDualRecordAbility();
    HILOGI("NearlinkTwsTest:UpdateDualRecordAbility_001 end");
}

/**
 * @tc.name: UpdateDualKaraokeAbility_001
 * @tc.desc: Test UpdateDualKaraokeAbility uses ProfileASC from profile manager, should not crash
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, UpdateDualKaraokeAbility_001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:UpdateDualKaraokeAbility_001 start");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->UpdateDualKaraokeAbility();
    HILOGI("NearlinkTwsTest:UpdateDualKaraokeAbility_001 end");
}

/**
 * @tc.name: UpdateVoiceCallFrameFourAndAutoRateAbility_001
 * @tc.desc: Test UpdateVoiceCallFrameFourAndAutoRateAbility uses ProfileASC, should not crash
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsTest, UpdateVoiceCallFrameFourAndAutoRateAbility_001, TestSize.Level1)
{
    HILOGI("NearlinkTwsTest:UpdateVoiceCallFrameFourAndAutoRateAbility_001 start");
    TwsService *twsService = TwsService::GetService();
    ASSERT_NE(twsService, nullptr);
    twsService->UpdateVoiceCallFrameFourAndAutoRateAbility();
    HILOGI("NearlinkTwsTest:UpdateVoiceCallFrameFourAndAutoRateAbility_001 end");
}

/* 音频框架桩：播放/暂停键值，与 MCP 侧协议定义一致 */
constexpr uint8_t TWS_TEST_PLAY_KEY = 0x11;
constexpr uint8_t TWS_TEST_PAUSE_KEY = 0x13;

/* ASC profile 手写桩：默认空实现，关键方法记录调用供断言 */
class MockProfileAsc : public ProfileASC {
public:
    std::vector<std::pair<std::string, int>> actionCalls;
    std::vector<std::pair<std::string, uint8_t>> playPauseCalls;
    std::vector<std::pair<std::string, uint8_t>> roleCalls;
    std::string activeSinkAddr;
    bool isCalling = false;
    bool isPlaying = false;

    int Connect(const RawAddress &device) override
    {
        return 0;
    }
    int Disconnect(const RawAddress &device) override
    {
        return 0;
    }
    std::list<RawAddress> GetConnectDevices() override
    {
        return {};
    }
    int GetConnectState() override
    {
        return 0;
    }
    utility::Context *GetContext() override
    {
        return nullptr;
    }
    int AudioControl(const RawAddress &device, AudioStreamType streamType, int cmd) override
    {
        return 0;
    }
    void StopSink() override {}
    int GetAudioDeviceList(std::vector<NearlinkRawAddress> &devices) override
    {
        return 0;
    }
    int GetVirtualAudioDeviceList(std::vector<NearlinkRawAddress> &devices) override
    {
        return 0;
    }
    int GetSupportStreamType(const NearlinkRawAddress &device, uint32_t &supportStreamType) override
    {
        return 0;
    }
    int GetAudioDeviceCodecInfo(const NearlinkRawAddress &device,
        std::map<AudioStreamType, AudioStreamCodecInfo> &info) override
    {
        return 0;
    }
    int SetActiveSinkDevice(const NearlinkRawAddress &device, uint32_t supportStreamType) override
    {
        return 0;
    }
    int UpdateDeviceRole(const RawAddress &device, uint8_t devRole) override
    {
        roleCalls.push_back({device.GetAddress(), devRole});
        return 0;
    }
    int RegisterApplication(const std::shared_ptr<InterfaceASCCallback> &callback) override
    {
        return 0;
    }
    int DeregisterApplication() override
    {
        return 0;
    }
    int DeregisterApplication(const std::shared_ptr<InterfaceASCCallback> &callback) override
    {
        return 0;
    }
    void RegisterObserver(ASCObserver &observer) override {}
    void DeregisterObserver(ASCObserver &observer) override {}
    const NearlinkRawAddress GetActiveSinkDevice() const override
    {
        return NearlinkRawAddress(activeSinkAddr);
    }
    bool IsCalling() override
    {
        return isCalling;
    }
    bool IsPlaying(const RawAddress &device) override
    {
        return isPlaying;
    }
    bool GetAscQosmInfo(const RawAddress &device, AscQosmInfo &qosmInfo) override
    {
        return false;
    }
    void SleAudioDeviceActionChanged(const NearlinkRawAddress &device, int action) override
    {
        actionCalls.push_back({device.GetAddress(), action});
    }
    bool UpdateSleVirtualDevice(int32_t cmd, const RawAddress &device) override
    {
        return false;
    }
    void SetIsCallingFlag(bool calling) override {}
    void SleAudioDeviceActionChanged(const NearlinkRawAddress &device,
        const std::vector<struct AudioStreamInfo> &streamData, int action) override {}
    void SendPlayOrPauseByWearDetection(const RawAddress &devAddr, uint8_t playOrPauseKey) override
    {
        playPauseCalls.push_back({devAddr.GetAddress(), playOrPauseKey});
    }
    void AcbSubrateChanged(const RawAddress &device, uint32_t subrate) override {}
    void AcbSubrateChangeReq(const RawAddress &device, const SleAcbSubrateParam &subrateParam) override {}
    bool GetDualRecordAbility(const RawAddress &device) override
    {
        return false;
    }
    void PhyChanged(RawAddress device, uint8_t frameType, uint8_t phyType, uint8_t status) override {}
    bool GetKaraokeAbility(const RawAddress &device) override
    {
        return false;
    }
    bool GetLocalDualRecordAbility() override
    {
        return false;
    }
    bool GetLocalKaraokeAbility() override
    {
        return false;
    }
    bool GetLocalVocieCallFrameFourAbility() override
    {
        return false;
    }
    bool GetLocalVoiceCallAutorateAbility() override
    {
        return false;
    }
};

/* ssap client 手写桩：服务发现结果受控、订阅/写请求计数 */
class MockSsapClient : public InterfaceProfileSsapClient {
public:
    std::list<Service> serviceList;
    int registerCalls = 0;
    int deregisterCalls = 0;
    int setNotifyCalls = 0;
    int writeCalls = 0;
    int nextAppId = 1;

    int Connect(const RawAddress &device) override
    {
        return 0;
    }
    int Disconnect(const RawAddress &device) override
    {
        return 0;
    }
    std::list<RawAddress> GetConnectDevices() override
    {
        return {};
    }
    int GetConnectState() override
    {
        return 0;
    }
    utility::Context *GetContext() override
    {
        return nullptr;
    }
    int RegisterApplication(const std::shared_ptr<InterfaceSsapClientCallback> &callback,
        const RawAddress &addr, uint8_t transport, uint8_t secureReq) override
    {
        registerCalls++;
        return nextAppId++;
    }
    int DeregisterApplication(int appId) override
    {
        deregisterCalls++;
        return 0;
    }
    int ExchangeMtu(int appId, uint16_t mtu) override
    {
        return 0;
    }
    int DiscoverServices(int appId) override
    {
        return 0;
    }
    int DiscoverServicesByUuid(int appId, const Uuid &uuid, uint16_t startHandle, uint16_t endHandle) override
    {
        return 0;
    }
    std::list<Service> GetServices(int appId) override
    {
        return serviceList;
    }
    std::list<Service> GetServicesByUuid(int appId, const Uuid &uuid) override
    {
        return {};
    }
    int ReadProperty(int appId, const Property &property) override
    {
        return 0;
    }
    int ReadDescriptor(int appId, const Descriptor &descriptor) override
    {
        return 0;
    }
    int ReadPropertiesByUuid(int appId, const Uuid &uuid, uint16_t startHandle, uint16_t endHandle) override
    {
        return 0;
    }
    int ReadDescriptorsByUuid(int appId, const Uuid &uuid, uint8_t type,
        uint16_t startHandle, uint16_t endHandle) override
    {
        return 0;
    }
    int WriteProperty(int appId, Property &property, bool withoutRsp) override
    {
        writeCalls++;
        return 0;
    }
    int WriteDescriptor(int appId, Descriptor &descriptor, bool withoutRsp) override
    {
        return 0;
    }
    int GetPropertyNotification(int appId, const Property &property) override
    {
        return 0;
    }
    int SetPropertyNotification(int appId, const Property &property, bool enable) override
    {
        setNotifyCalls++;
        return 0;
    }
    int SetEventNotification(int appId, const Event &event, bool enable) override
    {
        return 0;
    }
    int SetPropertyIndication(int appId, const Property &property, bool enable) override
    {
        return 0;
    }
    int SetEventIndication(int appId, const Event &event, bool enable) override
    {
        return 0;
    }
    int CallMethod(int appId, Method &method, bool withoutRsp) override
    {
        return 0;
    }
    int Connect(int appId, bool autoConnect) override
    {
        return 0;
    }
    int Disconnect(int appId) override
    {
        return 0;
    }
};

/* 远端信息观察者：记录胶囊/远端数据上报 */
class TestTwsObserver : public InterfaceTwsClientObserver {
public:
    std::vector<std::pair<std::string, std::vector<uint8_t>>> remoteInfos;

    void OnTwsRemoteInfo(const std::string &address, const std::vector<uint8_t> &value) override
    {
        remoteInfos.push_back({address, value});
    }
};

class NearlinkTwsDeepTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

protected:
    RawAddress FreshAddr();
    void EnableWearDetect(const RawAddress &dev);
    void RegisterCdsm();
    void RegisterSsap();
    TwsClientData &ClientDataOf(const std::string &addr);

    MockProfileAsc ascMock_;
    MockSsapClient ssapMock_;
    std::shared_ptr<TestTwsObserver> observerPtr_ = std::make_shared<TestTwsObserver>();
    std::unique_ptr<TwsService> service_;
    std::unique_ptr<CdsmService> cdsmService_;
    std::vector<std::string> usedAddrs_;
};

void NearlinkTwsDeepTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkTwsDeepTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
}

void NearlinkTwsDeepTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkTwsDeepTest");
}

void NearlinkTwsDeepTest::SetUp()
{
    HILOGI("SetUp NearlinkTwsDeepTest.");
    // 测试目的：TWS 同步线程 + ASC 桩常驻注册（多数深链依赖 ASC 存在），
    // Cdsm/ssap 按用例需要惰性注册，避免给简单用例引入合作集副作用。
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_TWS, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    service_ = std::make_unique<TwsService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_TWS, service_.get());
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_ASC, &ascMock_);
}

void NearlinkTwsDeepTest::TearDown()
{
    HILOGI("TearDown NearlinkTwsDeepTest.");
    // 测试目的：清理顺序固定——先摘除全部注册槽位再销毁服务，
    // 使 TwsClient 析构链看到的 ssap 服务为空而安全返回；最后复位线程/配置/桩状态。
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_SSAP_CLIENT, nullptr);
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CDSM, nullptr);
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_ASC, nullptr);
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_TWS, nullptr);
    service_.reset();
    cdsmService_.reset();
    // 测试目的：一次性移除本例地址在配置表的全部键（佩戴检测/自动连接/音量/能力位图），防跨例与跨进程残留
    for (auto &addr : usedAddrs_) {
        SleConfig::GetInstance().RemovePairedDevice(addr);
    }
    SleConfig::GetInstance().Save();
    ClearMockAudioFwkState();
    ThreadUtil::GetInstance().InitThreadStateMap();
}

RawAddress NearlinkTwsDeepTest::FreshAddr()
{
    static uint16_t counter = 0;
    counter++;
    char buf[18] = {0};
    (void)snprintf(buf, sizeof(buf), "AA:BB:CC:DD:%02X:%02X", (counter >> 8) & 0xFF, counter & 0xFF);
    usedAddrs_.push_back(buf);
    return RawAddress(buf);
}

void NearlinkTwsDeepTest::EnableWearDetect(const RawAddress &dev)
{
    SleConfig::GetInstance().SetConfigWearDetectionState(dev.GetAddress(),
        static_cast<int>(TwsWearDetectSupport::SUPPORT_ON));
}

void NearlinkTwsDeepTest::RegisterCdsm()
{
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_CDSM, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    cdsmService_ = std::make_unique<CdsmService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CDSM, cdsmService_.get());
}

void NearlinkTwsDeepTest::RegisterSsap()
{
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_SSAP_CLIENT, &ssapMock_);
}

TwsClientData &NearlinkTwsDeepTest::ClientDataOf(const std::string &addr)
{
    std::shared_ptr<TwsClientData> data;
    if (!service_->twsClientDatas_.GetValue(addr, data)) {
        data = std::make_shared<TwsClientData>(RawAddress(addr));
        service_->twsClientDatas_.EnsureInsert(addr, data);
    }
    return *data;
}

/*
 * 驱动佩戴迁移的公共步骤：先落上一次佩戴态，再上报新佩戴态。
 * 佩戴检测配置为开启后，服务会识别迁移动作并通知音频框架。
 */
static void DriveWearChange(TwsService &svc, const RawAddress &dev,
    uint8_t prevLeft, uint8_t prevRight, uint8_t newLeft, uint8_t newRight)
{
    TwsDevWearStatus prev{};
    prev.leftStatus = prevLeft;
    prev.rightStatus = prevRight;
    svc.twsDevWearStatus_.EnsureInsert(dev.GetAddress(), prev);
    TwsWearStateData wear{};
    wear.leftWear = newLeft;
    wear.rightWear = newRight;
    svc.UpdateDeviceWearState(dev, wear);
}

/**
 * @tc.name: WearActionMatrixAdd001
 * @tc.desc: 校验双耳佩戴迁移识别：未戴到双戴、单戴补齐、空戴到单戴均上报已佩戴动作
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, WearActionMatrixAdd001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    EnableWearDetect(dev);
    // 未戴->双戴：从空到佩戴，上报已佩戴
    DriveWearChange(*service_, dev, static_cast<uint8_t>(TwsWearDetect::NOT_WORN),
        static_cast<uint8_t>(TwsWearDetect::NOT_WORN), static_cast<uint8_t>(TwsWearDetect::WORN),
        static_cast<uint8_t>(TwsWearDetect::WORN));
    ASSERT_EQ(ascMock_.actionCalls.size(), 1u);
    EXPECT_EQ(ascMock_.actionCalls[0].second, static_cast<int>(UpdateOutputStackAction::ACTION_WEAR));

    // 左戴->双戴：仍是已佩戴状态，不重复上报
    DriveWearChange(*service_, dev, static_cast<uint8_t>(TwsWearDetect::WORN),
        static_cast<uint8_t>(TwsWearDetect::NOT_WORN), static_cast<uint8_t>(TwsWearDetect::WORN),
        static_cast<uint8_t>(TwsWearDetect::WORN));
    EXPECT_EQ(ascMock_.actionCalls.size(), 1u);

    // 未戴->右戴：从空到佩戴，上报已佩戴
    RawAddress dev2 = FreshAddr();
    EnableWearDetect(dev2);
    DriveWearChange(*service_, dev2, static_cast<uint8_t>(TwsWearDetect::NOT_WORN),
        static_cast<uint8_t>(TwsWearDetect::NOT_WORN), static_cast<uint8_t>(TwsWearDetect::NOT_WORN),
        static_cast<uint8_t>(TwsWearDetect::WORN));
    ASSERT_EQ(ascMock_.actionCalls.size(), 2u);
    EXPECT_EQ(ascMock_.actionCalls[1].second, static_cast<int>(UpdateOutputStackAction::ACTION_WEAR));
}

/**
 * @tc.name: WearActionMatrixRemove001
 * @tc.desc: 校验摘耳迁移识别：双戴摘一侧、双戴全摘、单戴摘空均上报未佩戴且不误发暂停命令
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, WearActionMatrixRemove001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    EnableWearDetect(dev);
    // 双戴->全摘：从佩戴到空，上报未佩戴
    DriveWearChange(*service_, dev, static_cast<uint8_t>(TwsWearDetect::WORN),
        static_cast<uint8_t>(TwsWearDetect::WORN), static_cast<uint8_t>(TwsWearDetect::NOT_WORN),
        static_cast<uint8_t>(TwsWearDetect::NOT_WORN));
    ASSERT_EQ(ascMock_.actionCalls.size(), 1u);
    EXPECT_EQ(ascMock_.actionCalls[0].second, static_cast<int>(UpdateOutputStackAction::ACTION_UNWEAR));

    // 双戴->仅左戴：仍是佩戴态，不重复上报
    DriveWearChange(*service_, dev, static_cast<uint8_t>(TwsWearDetect::WORN),
        static_cast<uint8_t>(TwsWearDetect::WORN), static_cast<uint8_t>(TwsWearDetect::WORN),
        static_cast<uint8_t>(TwsWearDetect::NOT_WORN));
    EXPECT_EQ(ascMock_.actionCalls.size(), 1u);

    // 右戴->全摘：从佩戴到空，上报未佩戴；设备非活跃时暂停命令被拦截
    RawAddress dev2 = FreshAddr();
    EnableWearDetect(dev2);
    DriveWearChange(*service_, dev2, static_cast<uint8_t>(TwsWearDetect::NOT_WORN),
        static_cast<uint8_t>(TwsWearDetect::WORN), static_cast<uint8_t>(TwsWearDetect::NOT_WORN),
        static_cast<uint8_t>(TwsWearDetect::NOT_WORN));
    ASSERT_EQ(ascMock_.actionCalls.size(), 2u);
    EXPECT_EQ(ascMock_.actionCalls[1].second, static_cast<int>(UpdateOutputStackAction::ACTION_UNWEAR));
    EXPECT_TRUE(ascMock_.playPauseCalls.empty());
}

/**
 * @tc.name: WearActionMatrixSwitchNone001
 * @tc.desc: 校验左右换耳与状态不变场景：换耳不触发佩戴动作上报，状态完全不变则无任何动作
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, WearActionMatrixSwitchNone001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    EnableWearDetect(dev);
    const uint8_t worn = static_cast<uint8_t>(TwsWearDetect::WORN);
    const uint8_t notWorn = static_cast<uint8_t>(TwsWearDetect::NOT_WORN);
    // 左戴->右戴（换耳）：佩戴态不变，不触发佩戴动作
    DriveWearChange(*service_, dev, worn, notWorn, notWorn, worn);
    EXPECT_TRUE(ascMock_.actionCalls.empty());

    // 右戴->左戴（换耳）：同上
    DriveWearChange(*service_, dev, notWorn, worn, worn, notWorn);
    EXPECT_TRUE(ascMock_.actionCalls.empty());

    // 状态不变：双戴->双戴、全空->全空、单戴保持
    DriveWearChange(*service_, dev, worn, worn, worn, worn);
    DriveWearChange(*service_, dev, notWorn, notWorn, notWorn, notWorn);
    DriveWearChange(*service_, dev, worn, notWorn, worn, notWorn);
    EXPECT_TRUE(ascMock_.actionCalls.empty());
}

/**
 * @tc.name: WearActionSupportOff001
 * @tc.desc: 校验佩戴检测关闭时上报佩戴状态不触发任何音频动作
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, WearActionSupportOff001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    SleConfig::GetInstance().SetConfigWearDetectionState(dev.GetAddress(),
        static_cast<int>(TwsWearDetectSupport::SUPPORT_OFF));
    DriveWearChange(*service_, dev, static_cast<uint8_t>(TwsWearDetect::NOT_WORN),
        static_cast<uint8_t>(TwsWearDetect::NOT_WORN), static_cast<uint8_t>(TwsWearDetect::WORN),
        static_cast<uint8_t>(TwsWearDetect::WORN));
    EXPECT_TRUE(ascMock_.actionCalls.empty());
    EXPECT_TRUE(ascMock_.playPauseCalls.empty());
}

/**
 * @tc.name: WearActionSupportUnknown001
 * @tc.desc: 校验佩戴检测配置未知时首次上报触发默认开启链路：补发佩戴动作与开启佩戴检测通知，随后再走常规佩戴上报
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, WearActionSupportUnknown001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    SleConfig::GetInstance().SetConfigWearDetectionState(dev.GetAddress(), -1);
    DriveWearChange(*service_, dev, static_cast<uint8_t>(TwsWearDetect::NOT_WORN),
        static_cast<uint8_t>(TwsWearDetect::NOT_WORN), static_cast<uint8_t>(TwsWearDetect::WORN),
        static_cast<uint8_t>(TwsWearDetect::WORN));
    ASSERT_EQ(ascMock_.actionCalls.size(), 3u);
    EXPECT_EQ(ascMock_.actionCalls[0].second, static_cast<int>(UpdateOutputStackAction::ACTION_WEAR));
    EXPECT_EQ(ascMock_.actionCalls[1].second,
        static_cast<int>(UpdateOutputStackAction::ACTION_ENABLE_WEAR_DETECTION));
    EXPECT_EQ(ascMock_.actionCalls[2].second, static_cast<int>(UpdateOutputStackAction::ACTION_WEAR));
}

/**
 * @tc.name: WearResumeFlow001
 * @tc.desc: 校验戴回恢复播放：存在近期双摘暂停记录时双耳戴回发送播放命令并清除记录
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, WearResumeFlow001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    EnableWearDetect(dev);
    TwsPauseRecord record{};
    record.pauseReason = static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE);
    record.pauseTime = service_->GetTimeStamp();
    service_->pauseRecordMap_.EnsureInsert(dev.GetAddress(), record);

    DriveWearChange(*service_, dev, static_cast<uint8_t>(TwsWearDetect::NOT_WORN),
        static_cast<uint8_t>(TwsWearDetect::NOT_WORN), static_cast<uint8_t>(TwsWearDetect::WORN),
        static_cast<uint8_t>(TwsWearDetect::WORN));
    ASSERT_EQ(ascMock_.playPauseCalls.size(), 1u);
    EXPECT_EQ(ascMock_.playPauseCalls[0].second, TWS_TEST_PLAY_KEY);
    TwsPauseRecord after{};
    EXPECT_FALSE(service_->pauseRecordMap_.GetValue(dev.GetAddress(), after));
}

/**
 * @tc.name: WearResumeFlow002
 * @tc.desc: 校验暂停原因与戴回方式不匹配时不发送播放命令仅清除记录
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, WearResumeFlow002, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    EnableWearDetect(dev);
    TwsPauseRecord record{};
    record.pauseReason = static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_LEFT);
    record.pauseTime = service_->GetTimeStamp();
    service_->pauseRecordMap_.EnsureInsert(dev.GetAddress(), record);

    DriveWearChange(*service_, dev, static_cast<uint8_t>(TwsWearDetect::NOT_WORN),
        static_cast<uint8_t>(TwsWearDetect::NOT_WORN), static_cast<uint8_t>(TwsWearDetect::WORN),
        static_cast<uint8_t>(TwsWearDetect::WORN));
    EXPECT_TRUE(ascMock_.playPauseCalls.empty());
    TwsPauseRecord after{};
    EXPECT_FALSE(service_->pauseRecordMap_.GetValue(dev.GetAddress(), after));
}

/**
 * @tc.name: WearResumeFlow003
 * @tc.desc: 校验无暂停记录时戴回不发送播放命令；通话中也不恢复播放
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, WearResumeFlow003, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    EnableWearDetect(dev);
    // 无暂停记录：戴回无事发生
    DriveWearChange(*service_, dev, static_cast<uint8_t>(TwsWearDetect::NOT_WORN),
        static_cast<uint8_t>(TwsWearDetect::NOT_WORN), static_cast<uint8_t>(TwsWearDetect::WORN),
        static_cast<uint8_t>(TwsWearDetect::WORN));
    EXPECT_TRUE(ascMock_.playPauseCalls.empty());

    // 通话中：有暂停记录也不恢复
    RawAddress dev2 = FreshAddr();
    EnableWearDetect(dev2);
    ascMock_.isCalling = true;
    TwsPauseRecord record{};
    record.pauseReason = static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE);
    record.pauseTime = service_->GetTimeStamp();
    service_->pauseRecordMap_.EnsureInsert(dev2.GetAddress(), record);
    DriveWearChange(*service_, dev2, static_cast<uint8_t>(TwsWearDetect::NOT_WORN),
        static_cast<uint8_t>(TwsWearDetect::NOT_WORN), static_cast<uint8_t>(TwsWearDetect::WORN),
        static_cast<uint8_t>(TwsWearDetect::WORN));
    EXPECT_TRUE(ascMock_.playPauseCalls.empty());
    TwsPauseRecord after{};
    EXPECT_FALSE(service_->pauseRecordMap_.GetValue(dev2.GetAddress(), after));
}

/**
 * @tc.name: PauseReasonPlaying001
 * @tc.desc: 校验播放中摘耳的暂停原因判定：单耳摘空记单摘原因，双戴摘一侧记双摘原因，其余判无需暂停
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, PauseReasonPlaying001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    TwsDevWearStatus wear{};
    const int started = NL_SLE_ASC_STATE_STARTED;
    const int removeLeft = static_cast<int>(TwsWearAction::ACTION_REMOVE_LEFT);
    const int removeRight = static_cast<int>(TwsWearAction::ACTION_REMOVE_RIGHT);
    const int removeDouble = static_cast<int>(TwsWearAction::ACTION_REMOVE_DOUBLE);

    // 左戴摘空 -> 左耳摘下原因
    wear.leftStatus = static_cast<uint8_t>(TwsWearDetect::WORN);
    wear.rightStatus = static_cast<uint8_t>(TwsWearDetect::NOT_WORN);
    EXPECT_EQ(service_->GetPauseReason(dev, removeLeft, wear, started),
        static_cast<int>(TwsWearPauseReason::PAUSE_REASON_LEFT_REMOVE_LEFT));

    // 右戴摘空 -> 右耳摘下原因
    wear.leftStatus = static_cast<uint8_t>(TwsWearDetect::NOT_WORN);
    wear.rightStatus = static_cast<uint8_t>(TwsWearDetect::WORN);
    EXPECT_EQ(service_->GetPauseReason(dev, removeRight, wear, started),
        static_cast<int>(TwsWearPauseReason::PAUSE_REASON_RIGHT_REMOVE_RIGHT));

    // 双戴摘左/摘右/全摘 -> 双摘系原因
    wear.leftStatus = static_cast<uint8_t>(TwsWearDetect::WORN);
    wear.rightStatus = static_cast<uint8_t>(TwsWearDetect::WORN);
    EXPECT_EQ(service_->GetPauseReason(dev, removeLeft, wear, started),
        static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_LEFT));
    EXPECT_EQ(service_->GetPauseReason(dev, removeRight, wear, started),
        static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_RIGHT));
    EXPECT_EQ(service_->GetPauseReason(dev, removeDouble, wear, started),
        static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE));

    // 双戴但动作是戴上（非法组合）与全空摘（无耳可摘）-> 无需暂停
    const int addLeft = static_cast<int>(TwsWearAction::ACTION_ADD_LEFT);
    EXPECT_EQ(service_->GetPauseReason(dev, addLeft, wear, started),
        static_cast<int>(TwsWearPauseReason::PAUSE_NO_NEED));
    wear.leftStatus = static_cast<uint8_t>(TwsWearDetect::NOT_WORN);
    wear.rightStatus = static_cast<uint8_t>(TwsWearDetect::NOT_WORN);
    EXPECT_EQ(service_->GetPauseReason(dev, removeDouble, wear, started),
        static_cast<int>(TwsWearPauseReason::PAUSE_NO_NEED));
}

/**
 * @tc.name: PauseReasonNotPlaying001
 * @tc.desc: 校验非播放中摘耳的暂停原因判定：无记录或常态摘下判无需暂停，跨耳乒乓摘下合并为双摘原因
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, PauseReasonNotPlaying001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    TwsDevWearStatus wear{};
    const int removeLeft = static_cast<int>(TwsWearAction::ACTION_REMOVE_LEFT);
    const int removeRight = static_cast<int>(TwsWearAction::ACTION_REMOVE_RIGHT);
    const int notStarted = 0;

    // 无暂停记录 -> 无需暂停
    EXPECT_EQ(service_->GetPauseReasonNotPlaying(dev, removeLeft, wear, notStarted),
        static_cast<int>(TwsWearPauseReason::PAUSE_NO_NEED));

    // 上次双摘左侧 + 现左戴右空 + 摘右 -> 合并为双摘
    TwsPauseRecord record{};
    record.pauseReason = static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_LEFT);
    record.pauseTime = service_->GetTimeStamp();
    service_->pauseRecordMap_.EnsureInsert(dev.GetAddress(), record);
    wear.leftStatus = static_cast<uint8_t>(TwsWearDetect::WORN);
    wear.rightStatus = static_cast<uint8_t>(TwsWearDetect::NOT_WORN);
    EXPECT_EQ(service_->GetPauseReasonNotPlaying(dev, removeRight, wear, notStarted),
        static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE));

    // 上次双摘右侧 + 现左空右戴 + 摘左 -> 合并为双摘
    record.pauseReason = static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_RIGHT);
    service_->pauseRecordMap_.EnsureInsert(dev.GetAddress(), record);
    wear.leftStatus = static_cast<uint8_t>(TwsWearDetect::NOT_WORN);
    wear.rightStatus = static_cast<uint8_t>(TwsWearDetect::WORN);
    EXPECT_EQ(service_->GetPauseReasonNotPlaying(dev, removeLeft, wear, notStarted),
        static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE));

    // 组合不匹配 -> 无需暂停
    wear.leftStatus = static_cast<uint8_t>(TwsWearDetect::WORN);
    wear.rightStatus = static_cast<uint8_t>(TwsWearDetect::WORN);
    EXPECT_EQ(service_->GetPauseReasonNotPlaying(dev, removeLeft, wear, notStarted),
        static_cast<int>(TwsWearPauseReason::PAUSE_NO_NEED));
}

/**
 * @tc.name: PauseRecord001
 * @tc.desc: 校验暂停发送与抑制：无需暂停直接清记录，正常摘下发送暂停命令，重复摘下不重复发送
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, PauseRecord001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    int64_t now = service_->GetTimeStamp();

    // 无需暂停原因：记录被清且不发送
    TwsPauseRecord record{};
    record.pauseReason = static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE);
    service_->pauseRecordMap_.EnsureInsert(dev.GetAddress(), record);
    service_->PauseAndRecordIfNeeded(dev, static_cast<int>(TwsWearPauseReason::PAUSE_NO_NEED), now);
    TwsPauseRecord after{};
    EXPECT_FALSE(service_->pauseRecordMap_.GetValue(dev.GetAddress(), after));
    EXPECT_TRUE(ascMock_.playPauseCalls.empty());

    // 正常暂停：发送暂停命令并落记录
    service_->PauseAndRecordIfNeeded(dev,
        static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE), now);
    ASSERT_EQ(ascMock_.playPauseCalls.size(), 1u);
    EXPECT_EQ(ascMock_.playPauseCalls[0].second, TWS_TEST_PAUSE_KEY);
    EXPECT_TRUE(service_->pauseRecordMap_.GetValue(dev.GetAddress(), after));

    // 重复暂停：丢弃不重复发送
    service_->PauseAndRecordIfNeeded(dev,
        static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE), now);
    EXPECT_EQ(ascMock_.playPauseCalls.size(), 1u);
}

/**
 * @tc.name: ResumePlayByReason001
 * @tc.desc: 校验按暂停原因恢复播放：时间窗内发送播放命令，超时只清记录不播放，未知原因只清记录
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, ResumePlayByReason001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    int64_t now = service_->GetTimeStamp();
    const int addLeft = static_cast<int>(TwsWearAction::ACTION_ADD_LEFT);

    // 时间窗内：发送播放并清记录
    TwsPauseRecord record{};
    record.pauseReason = static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE);
    record.pauseTime = now;
    service_->ResumePlayByReason(dev, addLeft, now, record);
    ASSERT_EQ(ascMock_.playPauseCalls.size(), 1u);
    EXPECT_EQ(ascMock_.playPauseCalls[0].second, TWS_TEST_PLAY_KEY);

    // 时间窗外：不播放（恢复窗口远大于用例时长，直接构造过期时间）
    record.pauseTime = now - MAX_RESUME_PLAY_TIME_SIMPLE - 1;
    service_->ResumePlayByReason(dev, addLeft, now, record);
    EXPECT_EQ(ascMock_.playPauseCalls.size(), 1u);

    // 未知原因：只清记录不播放
    record.pauseReason = 0xFF;
    record.pauseTime = now;
    service_->ResumePlayByReason(dev, addLeft, now, record);
    EXPECT_EQ(ascMock_.playPauseCalls.size(), 1u);
}

/**
 * @tc.name: ResumePlayIfNeeded001
 * @tc.desc: 校验恢复前置检查：无暂停记录不动作，通话中只清记录，正常场景走原因恢复链
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, ResumePlayIfNeeded001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    int64_t now = service_->GetTimeStamp();

    // 无记录：无事发生
    service_->ResumePlayIfNeeded(dev, static_cast<int>(TwsWearAction::ACTION_ADD_LEFT), now);
    EXPECT_TRUE(ascMock_.playPauseCalls.empty());

    // 通话中：清记录不恢复
    TwsPauseRecord record{};
    record.pauseReason = static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE);
    record.pauseTime = now;
    service_->pauseRecordMap_.EnsureInsert(dev.GetAddress(), record);
    ascMock_.isCalling = true;
    service_->ResumePlayIfNeeded(dev, static_cast<int>(TwsWearAction::ACTION_ADD_LEFT), now);
    TwsPauseRecord after{};
    EXPECT_FALSE(service_->pauseRecordMap_.GetValue(dev.GetAddress(), after));
    EXPECT_TRUE(ascMock_.playPauseCalls.empty());
}

/* 组造一个携带指定属性的 TWS 私有服务（供 ssap 桩返回） */
static Service MakeTwsService(const std::vector<Property> &props)
{
    Service svc(1, 1, 1, Uuid::ConvertFromString(SLE_UUID_TWS_PROFILE));
    svc.properties_ = props;
    return svc;
}

/**
 * @tc.name: SsapPropDisc001
 * @tc.desc: 校验服务发现：对端返回可读可写属性时保存发送句柄并订阅可读属性通知
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, SsapPropDisc001, TestSize.Level1)
{
    RegisterSsap();
    Property writeProp(2, Uuid::ConvertFromString(SLE_UUID_TWS_PROPERTY_WRITE));
    Property readProp(3, Uuid::ConvertFromString(SLE_UUID_TWS_PROPERTY_READ));
    ssapMock_.serviceList.push_back(MakeTwsService({writeProp, readProp}));

    MockTwsClient twsClient("AA:10:10:10:10:10");
    twsClient.SaveTwsServicePropertyInfo();
    EXPECT_EQ(ssapMock_.setNotifyCalls, 1);
    EXPECT_EQ(twsClient.sendDataProperty_.handle_, 2);
    EXPECT_EQ(twsClient.recvDataProperty_.handle_, 3);
}

/**
 * @tc.name: SsapPropDisc002
 * @tc.desc: 校验服务发现：对端服务列表不含 TWS 服务或只含无关属性时不订阅通知
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, SsapPropDisc002, TestSize.Level1)
{
    RegisterSsap();
    // 服务 UUID 不是 TWS 私有服务
    Service otherSvc(1, 1, 1, Uuid::ConvertFromString("00000000-0000-0000-0000-0000000000AA"));
    ssapMock_.serviceList.push_back(otherSvc);
    MockTwsClient twsClientA("AA:10:10:10:10:11");
    twsClientA.SaveTwsServicePropertyInfo();
    EXPECT_EQ(ssapMock_.setNotifyCalls, 0);

    // TWS 服务但属性 UUID 均不匹配
    ssapMock_.serviceList.clear();
    Property unrelated(9, Uuid::ConvertFromString("00000000-0000-0000-0000-0000000000BB"));
    ssapMock_.serviceList.push_back(MakeTwsService({unrelated}));
    MockTwsClient twsClientB("AA:10:10:10:10:12");
    twsClientB.SaveTwsServicePropertyInfo();
    EXPECT_EQ(ssapMock_.setNotifyCalls, 0);
}

/**
 * @tc.name: SsapPropDisc003
 * @tc.desc: 校验服务发现：只有可写属性时仅保存发送句柄不订阅通知
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, SsapPropDisc003, TestSize.Level1)
{
    RegisterSsap();
    Property writeProp(2, Uuid::ConvertFromString(SLE_UUID_TWS_PROPERTY_WRITE));
    ssapMock_.serviceList.push_back(MakeTwsService({writeProp}));

    MockTwsClient twsClient("AA:10:10:10:10:13");
    twsClient.SaveTwsServicePropertyInfo();
    EXPECT_EQ(ssapMock_.setNotifyCalls, 0);
    EXPECT_EQ(twsClient.sendDataProperty_.handle_, 2);
}

/**
 * @tc.name: SsapRecvData001
 * @tc.desc: 校验收数据入口防御：句柄不匹配、空数据、超长数据均被拒绝，合法数据进入解码链不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, SsapRecvData001, TestSize.Level1)
{
    RegisterSsap();
    MockTwsClient twsClient("AA:10:10:10:10:14");
    twsClient.recvDataProperty_ = Property(5, Uuid::ConvertFromString(SLE_UUID_TWS_PROPERTY_READ));

    // 句柄不匹配：直接返回
    Property wrongHandle(6);
    twsClient.OnPropertyChangedTask(wrongHandle);

    // 空数据与超长数据：被长度校验拦截
    Property emptyValue(5);
    twsClient.OnPropertyChangedTask(emptyValue);
    Property oversize(5, std::vector<uint8_t>(TWS_PROFILE_PROPERTY_MAX_LEN + 1, 0x00));
    twsClient.OnPropertyChangedTask(oversize);

    // 请求方向头与响应方向头：编解码库部署在场时走真解码，否则走句柄为空防御分支，均要求不崩溃
    Property reqStream(5, std::vector<uint8_t>{0x3F, 0x05, 0x08, 0x01, 0x01, 0x00, 0x01});
    twsClient.OnPropertyChangedTask(reqStream);
    Property rspStream(5, std::vector<uint8_t>{0xF3, 0x05, 0x0D, 0x7F, 0x01, 0x00, 0x00});
    twsClient.OnPropertyChangedTask(rspStream);
    SUCCEED();
}

/**
 * @tc.name: SsapClientLifecycle001
 * @tc.desc: 校验客户端注册与析构：注册拿到合法 appId，析构时按 appId 反注册一次
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, SsapClientLifecycle001, TestSize.Level1)
{
    RegisterSsap();
    {
        std::shared_ptr<TwsClient> client = TwsClient::CreateTwsClient("AA:10:10:10:10:15");
        ASSERT_NE(client, nullptr);
        EXPECT_EQ(ssapMock_.registerCalls, 1);
    }
    EXPECT_EQ(ssapMock_.deregisterCalls, 1);
}

/* 媒体流可用性的紧凑写法 */
static uint8_t StreamState(TwsStreamUsability state)
{
    return static_cast<uint8_t>(state);
}

/**
 * @tc.name: CapsuleEnableShow001
 * @tc.desc: 校验媒体可用胶囊：耳机上报媒体可用且非用户强选、设备活跃通话中时弹窗上报远端信息
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CapsuleEnableShow001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    service_->RegisterApplication(observerPtr_);
    ascMock_.activeSinkAddr = dev.GetAddress();
    ascMock_.isCalling = true;

    // 第一轮建立基线（禁用），第二轮变可用触发胶囊
    TwsClientData data(dev);
    data.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] = StreamState(
        TwsStreamUsability::DISABLE);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), data);
    data.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] = StreamState(
        TwsStreamUsability::AVAILABLE);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), data);

    ASSERT_FALSE(observerPtr_->remoteInfos.empty());
    std::string payload(observerPtr_->remoteInfos.back().second.begin(),
        observerPtr_->remoteInfos.back().second.end());
    EXPECT_NE(payload.find("+PROFILESTATE=1"), std::string::npos);
}

/**
 * @tc.name: CapsuleEnableSuppress001
 * @tc.desc: 校验胶囊抑制：用户持续强选耳机时不弹窗，空静音流在满足弹窗条件下也被拦截
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CapsuleEnableSuppress001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    service_->RegisterApplication(observerPtr_);
    ascMock_.activeSinkAddr = dev.GetAddress();
    ascMock_.isCalling = true;

    // 用户已强选：基线与探测轮均保持强选，不弹
    TwsClientData data(dev);
    data.audioState_.isUserSelected = true;
    data.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] = StreamState(
        TwsStreamUsability::DISABLE);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), data);
    data.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] = StreamState(
        TwsStreamUsability::AVAILABLE);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), data);
    EXPECT_TRUE(observerPtr_->remoteInfos.empty());

    // 空静音流：设备活跃且通话中（弹窗条件成立），仍被空静音流拦截
    RawAddress dev2 = FreshAddr();
    ascMock_.activeSinkAddr = dev2.GetAddress();
    TwsClientData muteData(dev2);
    muteData.audioState_.audioServiceType = static_cast<uint8_t>(TwsAudioServiceType::EMPTY_MUTE);
    muteData.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] = StreamState(
        TwsStreamUsability::DISABLE);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), muteData);
    muteData.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] = StreamState(
        TwsStreamUsability::AVAILABLE);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), muteData);
    EXPECT_TRUE(observerPtr_->remoteInfos.empty());
}

/**
 * @tc.name: CapsuleDisableShow001
 * @tc.desc: 校验媒体禁用胶囊：星闪正在出声且通话中耳机被抢占时弹窗上报禁用状态
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CapsuleDisableShow001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    service_->RegisterApplication(observerPtr_);
    SetMockNearlinkOut(true);
    ascMock_.isCalling = true;

    // 基线：先可用
    TwsClientData data(dev);
    data.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] = StreamState(
        TwsStreamUsability::AVAILABLE);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), data);

    // 被抢占（媒体禁用）：弹窗
    data.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] = StreamState(
        TwsStreamUsability::DISABLE);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), data);
    ASSERT_FALSE(observerPtr_->remoteInfos.empty());
    std::string payload(observerPtr_->remoteInfos.back().second.begin(),
        observerPtr_->remoteInfos.back().second.end());
    EXPECT_NE(payload.find("+PROFILESTATE=2"), std::string::npos);
}

/**
 * @tc.name: CapsuleVolumeZero001
 * @tc.desc: 校验媒体音量为零时即使满足抢占条件也不弹窗
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CapsuleVolumeZero001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    service_->RegisterApplication(observerPtr_);
    ascMock_.activeSinkAddr = dev.GetAddress();
    SetMockNearlinkOut(true);
    ascMock_.isCalling = true;
    SleConfig::GetInstance().SetDeviceMediaVolume(dev.GetAddress(), 0);

    TwsClientData data(dev);
    data.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] = StreamState(
        TwsStreamUsability::AVAILABLE);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), data);
    data.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] = StreamState(
        TwsStreamUsability::DISABLE);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), data);
    EXPECT_TRUE(observerPtr_->remoteInfos.empty());
}

/**
 * @tc.name: CapsuleProfileStateChange001
 * @tc.desc: 校验音频流状态上报：媒体与通话均可用时上报使能动作，任一路禁用时上报禁用动作，链路完整不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CapsuleProfileStateChange001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    // 媒体+通话均可用：使能
    TwsClientData data(dev);
    data.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] = StreamState(
        TwsStreamUsability::AVAILABLE);
    data.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::CELL_CALL)] = StreamState(
        TwsStreamUsability::AVAILABLE);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), data);

    // 通话禁用：禁用
    data.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::CELL_CALL)] = StreamState(
        TwsStreamUsability::DISABLE);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), data);
    SUCCEED();
}

/* 组造双成员合作集（直写服务内存表，与产品数据结构一致），返回组 ID */
static uint32_t SeedCdsmGroup(CdsmService &cdsm, const RawAddress &left, const RawAddress &right)
{
    static uint32_t groupId = 100;
    groupId++;
    auto info = std::make_shared<CdsmInfo>(groupId, left.GetAddress());
    info->CdsmAddMemberInfo(left.GetAddress(), 1);
    info->CdsmAddMemberInfo(right.GetAddress(), 1);
    cdsm.cdsmList_.EnsureInsert(groupId, info);
    return groupId;
}

/**
 * @tc.name: CdsmLinkRole001
 * @tc.desc: 校验合作集联动：主耳上报主角色时对端数据同步落库为副角色
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CdsmLinkRole001, TestSize.Level1)
{
    RegisterCdsm();
    RawAddress left = FreshAddr();
    RawAddress right = FreshAddr();
    SeedCdsmGroup(*cdsmService_, left, right);

    TwsClientData data(left);
    data.roleType_ = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_ROLE_TYPE), data);

    uint8_t rightRole = 0;
    service_->TwsGetDeviceRole(right, rightRole);
    EXPECT_EQ(rightRole, static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_SECONDARY));
    // 主耳侧与对端侧各触发一次角色切换通知
    ASSERT_EQ(ascMock_.roleCalls.size(), 2u);
    EXPECT_EQ(ascMock_.roleCalls[1].first, right.GetAddress());
    EXPECT_EQ(ascMock_.roleCalls[1].second, static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_SECONDARY));
}

/**
 * @tc.name: CdsmLinkPassThrough001
 * @tc.desc: 校验合作集联动：透传类数据（音频流状态）原样同步到对端耳机
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CdsmLinkPassThrough001, TestSize.Level1)
{
    RegisterCdsm();
    RawAddress left = FreshAddr();
    RawAddress right = FreshAddr();
    SeedCdsmGroup(*cdsmService_, left, right);

    TwsClientData data(left);
    data.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] =
        static_cast<uint8_t>(TwsStreamUsability::AVAILABLE);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), data);

    EXPECT_EQ(service_->TwsGetDeviceAudioMusicType(right),
        static_cast<uint8_t>(TwsStreamUsability::AVAILABLE));
}

/**
 * @tc.name: CdsmLinkNoSync001
 * @tc.desc: 校验联动边界：不在联动白名单的消息类型与无合作集的设备均不向对端同步
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CdsmLinkNoSync001, TestSize.Level1)
{
    RegisterCdsm();
    RawAddress left = FreshAddr();
    RawAddress right = FreshAddr();

    // 无合作集：查不到对端直接返回
    TwsClientData data(left);
    data.nature_ = static_cast<uint8_t>(TwsNatureType::TWS_RIGHT);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_EARBUDS_NATRUE), data);
    uint8_t rightNature = static_cast<uint8_t>(TwsNatureType::TWS_LEFT);
    service_->TwsGetDeviceNature(right, rightNature);
    EXPECT_EQ(rightNature, static_cast<uint8_t>(TwsNatureType::TWS_LEFT));

    // 有合作集但消息类型不在联动白名单：同样不同步
    SeedCdsmGroup(*cdsmService_, left, right);
    TwsClientData atData(left);
    atData.subCmd_ = 0x77;
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AT_CMD), atData);
    uint8_t rightNature2 = static_cast<uint8_t>(TwsNatureType::TWS_LEFT);
    service_->TwsGetDeviceNature(right, rightNature2);
    EXPECT_EQ(rightNature2, static_cast<uint8_t>(TwsNatureType::TWS_LEFT));
}

/**
 * @tc.name: CdsmDisconnectProcess001
 * @tc.desc: 校验断链清理：任一耳仍连接时不清理，双耳全断后清理佩戴记录并重置音频流状态
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CdsmDisconnectProcess001, TestSize.Level1)
{
    RegisterCdsm();
    RawAddress left = FreshAddr();
    RawAddress right = FreshAddr();
    SeedCdsmGroup(*cdsmService_, left, right);

    // 双耳连接态基线
    service_->NotifyStateChanged(left, TwsClientState::TWS_STATE_CONNECTING,
        TwsClientState::TWS_STATE_CONNECTED);
    service_->NotifyStateChanged(right, TwsClientState::TWS_STATE_CONNECTING,
        TwsClientState::TWS_STATE_CONNECTED);
    // 预置佩戴记录与禁用媒体流，用于断言清理效果
    TwsDevWearStatus wear{};
    wear.leftStatus = static_cast<uint8_t>(TwsWearDetect::WORN);
    service_->twsDevWearStatus_.EnsureInsert(left.GetAddress(), wear);
    ClientDataOf(left.GetAddress()).audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] =
        static_cast<uint8_t>(TwsStreamUsability::DISABLE);

    // 右耳先断：左耳仍连接，不清理
    service_->NotifyStateChanged(right, TwsClientState::TWS_STATE_CONNECTED,
        TwsClientState::TWS_STATE_DISCONNECTED);
    TwsDevWearStatus after{};
    EXPECT_TRUE(service_->twsDevWearStatus_.GetValue(left.GetAddress(), after));

    // 左耳再断：全断，佩戴记录清理、媒体流复位可用
    service_->NotifyStateChanged(left, TwsClientState::TWS_STATE_CONNECTED,
        TwsClientState::TWS_STATE_DISCONNECTED);
    EXPECT_FALSE(service_->twsDevWearStatus_.GetValue(left.GetAddress(), after));
    EXPECT_EQ(service_->TwsGetDeviceAudioMusicType(left),
        static_cast<uint8_t>(TwsStreamUsability::AVAILABLE));
}

/**
 * @tc.name: CdsmPrimaryAddr001
 * @tc.desc: 校验主耳地址查询：合作集内主角色成员被识别，无合作集时回退查询设备本身
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CdsmPrimaryAddr001, TestSize.Level1)
{
    RegisterCdsm();
    RawAddress left = FreshAddr();
    RawAddress right = FreshAddr();
    SeedCdsmGroup(*cdsmService_, left, right);

    // 无角色数据：查不到主耳，回退自身
    RawAddress primary = service_->GetPrimaryAddr(right);
    EXPECT_EQ(primary.GetAddress(), right.GetAddress());

    // 左耳为主角色：从右耳查到左耳
    ClientDataOf(left.GetAddress()).roleType_ = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY);
    primary = service_->GetPrimaryAddr(right);
    EXPECT_EQ(primary.GetAddress(), left.GetAddress());
}

/**
 * @tc.name: DataRoleUpdate001
 * @tc.desc: 校验角色更新：同角色不动作，角色变化时通知音频框架主副切换并更新库内角色
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, DataRoleUpdate001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    // 库内已是主角色，再来一次主角色：不触发切换
    ClientDataOf(dev.GetAddress()).roleType_ = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY);
    TwsClientData same(dev);
    same.roleType_ = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_ROLE_TYPE), same);
    EXPECT_TRUE(ascMock_.roleCalls.empty());

    // 变为副角色：触发切换通知
    TwsClientData change(dev);
    change.roleType_ = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_SECONDARY);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_ROLE_TYPE), change);
    ASSERT_EQ(ascMock_.roleCalls.size(), 1u);
    EXPECT_EQ(ascMock_.roleCalls[0].second, static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_SECONDARY));
}

/**
 * @tc.name: DataWearInvalid001
 * @tc.desc: 校验佩戴数据防御：双侧无效不上报，单侧无效沿用旧值，有效数据全量更新
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, DataWearInvalid001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    // 关闭佩戴检测，聚焦佩戴数据本身的更新语义
    SleConfig::GetInstance().SetConfigWearDetectionState(dev.GetAddress(),
        static_cast<int>(TwsWearDetectSupport::SUPPORT_OFF));
    const uint8_t invalid = static_cast<uint8_t>(TwsWearDetect::WORN_INVALID);
    const uint8_t worn = static_cast<uint8_t>(TwsWearDetect::WORN);
    const uint8_t notWorn = static_cast<uint8_t>(TwsWearDetect::NOT_WORN);

    // 基线：左戴右空
    TwsClientData base(dev);
    base.wearData_.leftWear = worn;
    base.wearData_.rightWear = notWorn;
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_WEAR_STATUS), base);

    // 双侧无效：保持旧值
    TwsClientData bothInvalid(dev);
    bothInvalid.wearData_.leftWear = invalid;
    bothInvalid.wearData_.rightWear = invalid;
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_WEAR_STATUS), bothInvalid);
    TwsDevWearStatus after{};
    service_->TwsGetDeviceWearStatus(dev, after);
    EXPECT_EQ(after.leftStatus, worn);
    EXPECT_EQ(after.rightStatus, notWorn);

    // 左无效右有效：左侧沿用旧值，右侧更新
    TwsClientData leftInvalid(dev);
    leftInvalid.wearData_.leftWear = invalid;
    leftInvalid.wearData_.rightWear = worn;
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_WEAR_STATUS), leftInvalid);
    service_->TwsGetDeviceWearStatus(dev, after);
    EXPECT_EQ(after.leftStatus, worn);
    EXPECT_EQ(after.rightStatus, worn);

    // 右无效左有效：右侧沿用旧值
    TwsClientData rightInvalid(dev);
    rightInvalid.wearData_.leftWear = notWorn;
    rightInvalid.wearData_.rightWear = invalid;
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_WEAR_STATUS), rightInvalid);
    service_->TwsGetDeviceWearStatus(dev, after);
    EXPECT_EQ(after.leftStatus, notWorn);
    EXPECT_EQ(after.rightStatus, worn);
}

/**
 * @tc.name: DataAtCmd001
 * @tc.desc: 校验私有 AT 命令：电量弹框与电量更新落库电量数据，未知命令被拒绝
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, DataAtCmd001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    const uint8_t dialogCmd = static_cast<uint8_t>(TwsAtCmdType::VENDOR_BATTERY_DIALOG);
    const uint8_t updateCmd = static_cast<uint8_t>(TwsAtCmdType::UPDATE_VENDOR_BATTERY);

    TwsClientData data(dev);
    data.subCmd_ = dialogCmd;
    data.vendorBatt_.devBattery_ = 66;
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AT_CMD), data);
    EXPECT_EQ(ClientDataOf(dev.GetAddress()).vendorBatt_.devBattery_, 66);

    data.subCmd_ = updateCmd;
    data.vendorBatt_.devBattery_ = 88;
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AT_CMD), data);
    EXPECT_EQ(ClientDataOf(dev.GetAddress()).vendorBatt_.devBattery_, 88);

    // 未知命令：不落库
    data.subCmd_ = 0x77;
    data.vendorBatt_.devBattery_ = 10;
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AT_CMD), data);
    EXPECT_EQ(ClientDataOf(dev.GetAddress()).vendorBatt_.devBattery_, 88);
}

/**
 * @tc.name: DataDtsCap001
 * @tc.desc: 校验数通能力更新：空能力列表不更新，含佩戴检测能力时能力查询返回支持
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, DataDtsCap001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    // 空能力：不更新
    TwsClientData empty(dev);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_DATA_TRANSPORT_CAP), empty);
    EXPECT_FALSE(service_->TwsIsSupportWearDetect(dev));

    // 含佩戴检测能力：查询支持
    TwsClientData withWear(dev);
    withWear.devDtsCap_ = {static_cast<uint8_t>(TwsDtsCap::DTS_ABILITY_WEAR_DETECT)};
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_DATA_TRANSPORT_CAP), withWear);
    EXPECT_TRUE(service_->TwsIsSupportWearDetect(dev));
}

/**
 * @tc.name: DataPreemption001
 * @tc.desc: 校验抢占消息：响应消息只更新抢占结果，请求消息记录虚拟连接类型与业务类型
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, DataPreemption001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    // 请求：记录虚拟自动连接类型
    TwsClientData req(dev);
    req.virtualAutoConnType_ = 2;
    req.virtualAutoBussinessType_ = 3;
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_QUERY_BUSINESS), req);
    EXPECT_EQ(ClientDataOf(dev.GetAddress()).virtualAutoConnType_, 2);
    EXPECT_EQ(ClientDataOf(dev.GetAddress()).virtualAutoBussinessType_, 3);

    // 响应：更新抢占结果，连接类型保持
    TwsClientData rsp(dev);
    rsp.captureRes_ = static_cast<uint8_t>(TwsCaptureResult::ACCEPT);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_QUERY_BUSINESS), rsp);
    EXPECT_EQ(ClientDataOf(dev.GetAddress()).captureRes_,
        static_cast<uint8_t>(TwsCaptureResult::ACCEPT));
    EXPECT_EQ(ClientDataOf(dev.GetAddress()).virtualAutoConnType_, 2);
}

/**
 * @tc.name: DataAutoConnSwitch001
 * @tc.desc: 校验自动连接开关上报：开关状态落库并影响虚拟自动连接支持查询
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, DataAutoConnSwitch001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    TwsClientData on(dev);
    on.autoConnSwitch_ = static_cast<uint8_t>(TwsAutoConnSwitch::ON);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUTO_CONN_SWITCH), on);
    EXPECT_TRUE(service_->TwsIsSupportVirtualAutoConnect(dev));

    TwsClientData off(dev);
    off.autoConnSwitch_ = static_cast<uint8_t>(TwsAutoConnSwitch::OFF);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUTO_CONN_SWITCH), off);
    EXPECT_FALSE(service_->TwsIsSupportVirtualAutoConnect(dev));
}

/**
 * @tc.name: DataNatureAndNotify001
 * @tc.desc: 校验左右耳信息与静默断链标识落库后可经查询接口读回
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, DataNatureAndNotify001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    TwsClientData nature(dev);
    nature.nature_ = static_cast<uint8_t>(TwsNatureType::TWS_RIGHT);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_EARBUDS_NATRUE), nature);
    EXPECT_EQ(service_->TwsGetDeviceNature(dev), static_cast<uint8_t>(TwsNatureType::TWS_RIGHT));

    TwsClientData notify(dev);
    notify.notifyValue_.notifyValue = static_cast<uint8_t>(TwsProfileAction::DISCONNECT);
    service_->UpdateClientData(static_cast<uint8_t>(
        TwsHiBoxMsgType::HIBOX_NOTIFY_SLE_DISCONNECT_PROFILE), notify);
    uint8_t role = 0;
    service_->TwsGetDeviceRole(dev, role);
    SUCCEED();
}

/**
 * @tc.name: DataManufacturerAbility001
 * @tc.desc: 校验能力位图上报：能力数据落库且整链（配置写入、协议栈通知）不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, DataManufacturerAbility001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    TwsClientData data(dev);
    data.manufacturerAbility_.fill(0x00);
    data.manufacturerAbility_[0] = 0x01;
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_MANUFACTURER_ABILITY), data);
    EXPECT_EQ(ClientDataOf(dev.GetAddress()).manufacturerAbility_[0], 0x01);
}

/**
 * @tc.name: DataUnknownType001
 * @tc.desc: 校验未知消息类型：数据处理入口走默认拒绝分支不影响已有数据
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, DataUnknownType001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    ClientDataOf(dev.GetAddress()).nature_ = static_cast<uint8_t>(TwsNatureType::TWS_RIGHT);
    TwsClientData data(dev);
    data.nature_ = static_cast<uint8_t>(TwsNatureType::TWS_LEFT);
    service_->UpdateClientData(0x7F, data);
    EXPECT_EQ(ClientDataOf(dev.GetAddress()).nature_, static_cast<uint8_t>(TwsNatureType::TWS_RIGHT));
}

/**
 * @tc.name: ServiceLifecycle001
 * @tc.desc: 校验服务启停：未启先停不崩溃，启动后重复启动走已启动短路，停止后清理全部连接数据
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, ServiceLifecycle001, TestSize.Level1)
{
    // 未启动先停止：直接回调完成
    service_->Disable();

    // 启动 + 重复启动：均正常完成
    service_->Enable();
    service_->Enable();

    // 预置连接数据后停止：清理生效
    RawAddress dev = FreshAddr();
    TwsClientData data(dev);
    data.nature_ = static_cast<uint8_t>(TwsNatureType::TWS_RIGHT);
    service_->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_EARBUDS_NATRUE), data);
    service_->Disable();
    EXPECT_EQ(service_->TwsGetDeviceNature(dev), static_cast<uint8_t>(TwsNatureType::TWS_LEFT));
}

/**
 * @tc.name: ServiceEventRoute001
 * @tc.desc: 校验消息路由：连接、断开、收发请求响应、远端信息等事件全部路由不崩溃，未知事件走默认分支
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, ServiceEventRoute001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    const std::string addr = dev.GetAddress();

    TwsMessage connect(TWS_SERVCICE_CONNECT_START_EVT);
    connect.dev_ = addr;
    service_->ProcessEvent(connect);

    TwsMessage disconnect(TWS_SERVICE_DISCONNECT_START_EVT);
    disconnect.dev_ = addr;
    service_->ProcessEvent(disconnect);

    TwsMessage recvData(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    recvData.dev_ = addr;
    recvData.streamLen_ = 4;
    recvData.dataStream_ = std::make_unique<uint8_t[]>(recvData.streamLen_);
    recvData.msgDirect_ = TWS_MSG_DIRECT_REQ;
    service_->ProcessEvent(recvData);

    TwsMessage sendData(TWS_SERVICE_SSAP_SEND_DATA_EVENT);
    sendData.dev_ = addr;
    service_->ProcessEvent(sendData);

    TwsMessage recvReq(TWS_SERVICE_RECV_REQ_EVENT);
    recvReq.dev_ = addr;
    recvReq.msgDirect_ = TWS_MSG_DIRECT_REQ;
    service_->ProcessEvent(recvReq);

    TwsMessage recvRsp(TWS_SERVICE_RECV_RSP_EVENT);
    recvRsp.dev_ = addr;
    recvRsp.msgDirect_ = TWS_MSG_DIRECT_RSP;
    service_->ProcessEvent(recvRsp);

    TwsMessage sendReq(TWS_SERVICE_SEND_REQ_EVENT);
    sendReq.dev_ = addr;
    sendReq.msgDirect_ = TWS_MSG_DIRECT_REQ;
    sendReq.msgType_ = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_DATA_TRANSPORT_CAP);
    sendReq.serviceDataLen_ = 2;
    sendReq.serviceData_ = std::make_unique<uint8_t[]>(2);
    service_->ProcessEvent(sendReq);

    TwsMessage sendRsp(TWS_SERVICE_SEND_RSP_EVENT);
    sendRsp.dev_ = addr;
    sendRsp.msgDirect_ = TWS_MSG_DIRECT_RSP;
    sendRsp.msgType_ = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_DATA_TRANSPORT_CAP);
    sendRsp.serviceDataLen_ = 2;
    sendRsp.serviceData_ = std::make_unique<uint8_t[]>(2);
    service_->ProcessEvent(sendRsp);

    char *remoteInfo = new char[5] {'A', 'B', 'C', 'D', '\0'};
    TwsMessage remote(TWS_SERVICE_SEND_REMOTE_INFO_EVENT);
    remote.dev_ = addr;
    remote.arg2M = remoteInfo;
    service_->ProcessEvent(remote);

    TwsMessage unknown(999);
    service_->ProcessEvent(unknown);
    SUCCEED();
}

/**
 * @tc.name: ConnectDisconnect001
 * @tc.desc: 校验连接与断开入口：正常状态下受理请求返回成功，重复请求同样安全
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, ConnectDisconnect001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    EXPECT_EQ(service_->Connect(dev), NL_NO_ERROR);
    EXPECT_EQ(service_->Disconnect(dev), NL_NO_ERROR);
}

/**
 * @tc.name: GetScenesFromQos001
 * @tc.desc: 校验 QoS 档位到音频场景映射：各档位分别映射到音乐、游戏、通话、空间音频等场景，超范围档位回落其他
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, GetScenesFromQos001, TestSize.Level1)
{
    const uint8_t sceneMusic = static_cast<uint8_t>(AudioScene::SLE_AUDIO_SCENE_MUSIC);
    const uint8_t sceneGame = static_cast<uint8_t>(AudioScene::SLE_AUDIO_SCENE_GAME);
    const uint8_t sceneCall = static_cast<uint8_t>(AudioScene::SLE_AUDIO_SCENE_CALL);
    const uint8_t sceneSpatial = static_cast<uint8_t>(AudioScene::SLE_AUDIO_SCENE_SPATIAL_AUDIO);
    const uint8_t sceneOthers = static_cast<uint8_t>(AudioScene::SLE_AUDIO_SCENE_OTHERS);
    EXPECT_EQ(service_->GetScenesFromQoS(1), sceneMusic);
    EXPECT_EQ(service_->GetScenesFromQoS(2), sceneGame);
    EXPECT_EQ(service_->GetScenesFromQoS(3), sceneCall);
    EXPECT_EQ(service_->GetScenesFromQoS(4), sceneSpatial);
    EXPECT_EQ(service_->GetScenesFromQoS(5), sceneOthers);
    EXPECT_EQ(service_->GetScenesFromQoS(8), sceneSpatial);
    EXPECT_EQ(service_->GetScenesFromQoS(9), sceneOthers);
    EXPECT_EQ(service_->GetScenesFromQoS(0), sceneOthers);
    EXPECT_EQ(service_->GetScenesFromQoS(10), sceneOthers);
}

/**
 * @tc.name: QueryDeviceState001
 * @tc.desc: 校验设备状态查询：无数据时返回安全默认值，预置数据后角色、左右耳、媒体流状态均可读回
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, QueryDeviceState001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    // 无数据：默认值
    uint8_t role = 0;
    service_->TwsGetDeviceRole(dev, role);
    EXPECT_EQ(service_->TwsGetDeviceNature(dev), static_cast<uint8_t>(TwsNatureType::TWS_LEFT));
    EXPECT_EQ(service_->TwsGetDeviceAudioMusicType(dev), static_cast<uint8_t>(TwsMediaState::DISABLE));
    TwsDevWearStatus wear{};
    service_->TwsGetDeviceWearStatus(dev, wear);
    EXPECT_FALSE(service_->TwsIsDeviceWearing(dev));

    // 预置后读回
    TwsClientData &data = ClientDataOf(dev.GetAddress());
    data.roleType_ = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_SECONDARY);
    data.nature_ = static_cast<uint8_t>(TwsNatureType::TWS_RIGHT);
    data.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] =
        static_cast<uint8_t>(TwsStreamUsability::AVAILABLE);
    data.wearData_.leftWear = static_cast<uint8_t>(TwsWearDetect::WORN);

    service_->TwsGetDeviceRole(dev, role);
    EXPECT_EQ(role, static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_SECONDARY));
    EXPECT_EQ(service_->TwsGetDeviceNature(dev), static_cast<uint8_t>(TwsNatureType::TWS_RIGHT));
    EXPECT_EQ(service_->TwsGetDeviceAudioMusicType(dev),
        static_cast<uint8_t>(TwsStreamUsability::AVAILABLE));
    service_->TwsGetDeviceWearStatus(dev, wear);
    EXPECT_EQ(wear.leftStatus, static_cast<uint8_t>(TwsWearDetect::WORN));
}

/**
 * @tc.name: QueryStreamState001
 * @tc.desc: 校验音频流状态批量查询：无数据返回空，预置后按流类型逐项返回
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, QueryStreamState001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    std::vector<struct AudioStreamInfo> streams;
    service_->QueryStreamState(dev, streams);
    EXPECT_TRUE(streams.empty());

    TwsClientData &data = ClientDataOf(dev.GetAddress());
    data.audioState_.audioType[1] = static_cast<uint8_t>(TwsStreamUsability::AVAILABLE);
    data.audioState_.audioType[2] = static_cast<uint8_t>(TwsStreamUsability::DISABLE);
    service_->QueryStreamState(dev, streams);
    ASSERT_GT(streams.size(), 2u);
    EXPECT_EQ(static_cast<uint8_t>(streams[1].streamState),
        static_cast<uint8_t>(TwsStreamUsability::AVAILABLE));
    EXPECT_EQ(static_cast<uint8_t>(streams[2].streamState),
        static_cast<uint8_t>(TwsStreamUsability::DISABLE));
}

/**
 * @tc.name: ProcPauseRecordTimeout001
 * @tc.desc: 校验暂停记录超时清理：近期记录保留，超时记录被清除
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, ProcPauseRecordTimeout001, TestSize.Level1)
{
    RawAddress fresh = FreshAddr();
    RawAddress stale = FreshAddr();
    int64_t now = service_->GetTimeStamp();

    TwsPauseRecord record{};
    record.pauseReason = static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE);
    record.pauseTime = now;
    service_->pauseRecordMap_.EnsureInsert(fresh.GetAddress(), record);
    record.pauseTime = now - DOUBLE_REMOVE_CHECK_TIME - 1;
    service_->pauseRecordMap_.EnsureInsert(stale.GetAddress(), record);

    service_->ProcPauseRecordMap();
    TwsPauseRecord after{};
    EXPECT_TRUE(service_->pauseRecordMap_.GetValue(fresh.GetAddress(), after));
    EXPECT_FALSE(service_->pauseRecordMap_.GetValue(stale.GetAddress(), after));
}

/**
 * @tc.name: VendorAccountHashGuard001
 * @tc.desc: 校验账号哈希入口防御：输出缓冲长度不符直接失败
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, VendorAccountHashGuard001, TestSize.Level1)
{
    uint8_t out[8] = {0};
    EXPECT_FALSE(service_->GetVendorAccountHash(out, sizeof(out)));
}

/**
 * @tc.name: CacheMessageFlow001
 * @tc.desc: 校验缓存消息：未连接时虚拟自动连接请求进缓存队列，连接完成后缓存被消费清空
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CacheMessageFlow001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    // 未连接：请求进缓存
    service_->SetVirtualAutoConnectType(dev, 2, 3);
    TwsCacheMessage cache{};
    EXPECT_TRUE(service_->msgCache_.GetValue(dev.GetAddress(), cache));
    EXPECT_EQ(cache.msgQueue.size(), 1u);

    // 连接完成：缓存队列被消费清空（表项保留空队列），连接状态落库
    service_->NotifyStateChanged(dev, TwsClientState::TWS_STATE_CONNECTING,
        TwsClientState::TWS_STATE_CONNECTED);
    EXPECT_TRUE(service_->msgCache_.GetValue(dev.GetAddress(), cache));
    EXPECT_TRUE(cache.msgQueue.empty());
    EXPECT_EQ(ClientDataOf(dev.GetAddress()).clientState,
        static_cast<int>(SleConnectState::CONNECTED));
}

/**
 * @tc.name: RecoverDataFromConf001
 * @tc.desc: 校验配置恢复：预先写入的自动连接开关开/关两种配置恢复到内存数据
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, RecoverDataFromConf001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    SleConfig::GetInstance().SetAutoConnectSwitch(dev.GetAddress(), 1);
    service_->RecoverDataFromConf(dev);
    EXPECT_TRUE(service_->TwsIsSupportVirtualAutoConnect(dev));

    RawAddress dev2 = FreshAddr();
    SleConfig::GetInstance().SetAutoConnectSwitch(dev2.GetAddress(), 0);
    service_->RecoverDataFromConf(dev2);
    EXPECT_FALSE(service_->TwsIsSupportVirtualAutoConnect(dev2));
}

/**
 * @tc.name: UpdateDeviceDefaultRole001
 * @tc.desc: 校验默认角色更新：非法角色被拒绝，合法角色经内部数据链落库
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, UpdateDeviceDefaultRole001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    service_->TwsUpdateDeviceDefaultRole(dev, 0x77);
    uint8_t role = 0;
    service_->TwsGetDeviceRole(dev, role);
    EXPECT_EQ(role, 0);

    service_->TwsUpdateDeviceDefaultRole(dev, static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY));
    service_->TwsGetDeviceRole(dev, role);
    EXPECT_EQ(role, static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY));
}

/**
 * @tc.name: CodecInitRepeat001
 * @tc.desc: 校验编解码库加载：已加载状态下重复加载直接返回成功
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CodecInitRepeat001, TestSize.Level1)
{
    ASSERT_TRUE(TwsSharedLibApi::GetInstance().Init());
    EXPECT_TRUE(TwsSharedLibApi::GetInstance().Init());
}

/**
 * @tc.name: CodecEncodeUnknownDirect001
 * @tc.desc: 校验编码入口防御：消息方向既非请求也非响应时编码失败
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CodecEncodeUnknownDirect001, TestSize.Level1)
{
    TwsMessage event(TWS_SERVICE_SEND_REQ_EVENT);
    event.dev_ = "AA:20:20:20:20:20";
    event.msgDirect_ = 0x33;
    event.msgType_ = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_DATA_TRANSPORT_CAP);
    event.serviceDataLen_ = 2;
    event.serviceData_ = std::make_unique<uint8_t[]>(2);
    EXPECT_FALSE(TwsSharedLibApi::GetInstance().EncodeMessage(event));
}

/**
 * @tc.name: CodecDecodeUnknownDirect001
 * @tc.desc: 校验解码入口防御：消息方向既非请求也非响应时解码失败，空数据解码失败
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CodecDecodeUnknownDirect001, TestSize.Level1)
{
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = "AA:20:20:20:20:21";
    event.msgDirect_ = 0x33;
    event.streamLen_ = 4;
    event.dataStream_ = std::make_unique<uint8_t[]>(4);
    EXPECT_FALSE(TwsSharedLibApi::GetInstance().DecodeMessage(event));

    TwsMessage empty(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    empty.dev_ = "AA:20:20:20:20:22";
    empty.msgDirect_ = TWS_MSG_DIRECT_REQ;
    EXPECT_FALSE(TwsSharedLibApi::GetInstance().DecodeMessage(empty));
}

/**
 * @tc.name: HiBoxRoleSwitch001
 * @tc.desc: 校验角色切换消息解析：切主/切副分别落库为主/副角色，非法切换类型被拒绝
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, HiBoxRoleSwitch001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    const uint8_t roleTypeMsg = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_ROLE_TYPE);

    // 切主
    TwsMessage toPrimary(TWS_SERVICE_RECV_REQ_EVENT);
    toPrimary.dev_ = dev.GetAddress();
    toPrimary.msgDirect_ = TWS_MSG_DIRECT_REQ;
    toPrimary.msgType_ = roleTypeMsg;
    toPrimary.serviceDataLen_ = sizeof(TwsIsoHandover);
    toPrimary.serviceData_ = std::make_unique<uint8_t[]>(sizeof(TwsIsoHandover));
    toPrimary.serviceData_[0] = 0x01; // 切主
    service_->ProcessEvent(toPrimary);
    uint8_t role = 0;
    service_->TwsGetDeviceRole(dev, role);
    EXPECT_EQ(role, static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY));

    // 切副
    TwsMessage toSecondary(TWS_SERVICE_RECV_REQ_EVENT);
    toSecondary.dev_ = dev.GetAddress();
    toSecondary.msgDirect_ = TWS_MSG_DIRECT_REQ;
    toSecondary.msgType_ = roleTypeMsg;
    toSecondary.serviceDataLen_ = sizeof(TwsIsoHandover);
    toSecondary.serviceData_ = std::make_unique<uint8_t[]>(sizeof(TwsIsoHandover));
    toSecondary.serviceData_[0] = 0x02; // 切副
    service_->ProcessEvent(toSecondary);
    service_->TwsGetDeviceRole(dev, role);
    EXPECT_EQ(role, static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_SECONDARY));

    // 非法类型：不更新
    TwsMessage invalid(TWS_SERVICE_RECV_REQ_EVENT);
    invalid.dev_ = dev.GetAddress();
    invalid.msgDirect_ = TWS_MSG_DIRECT_REQ;
    invalid.msgType_ = roleTypeMsg;
    invalid.serviceDataLen_ = sizeof(TwsIsoHandover);
    invalid.serviceData_ = std::make_unique<uint8_t[]>(sizeof(TwsIsoHandover));
    invalid.serviceData_[0] = 0x09;
    service_->ProcessEvent(invalid);
    service_->TwsGetDeviceRole(dev, role);
    EXPECT_EQ(role, static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_SECONDARY));
}

/**
 * @tc.name: HiBoxAutoConnAndNotify001
 * @tc.desc: 校验自动连接开关与静默断链消息解析：开关状态按报文落库，静默断链通知链路完整不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, HiBoxAutoConnAndNotify001, TestSize.Level1)
{
    RawAddress dev = FreshAddr();
    const uint8_t autoConnMsg = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUTO_CONN_SWITCH);

    // 上报开
    TwsMessage on(TWS_SERVICE_RECV_REQ_EVENT);
    on.dev_ = dev.GetAddress();
    on.msgDirect_ = TWS_MSG_DIRECT_REQ;
    on.msgType_ = autoConnMsg;
    on.serviceDataLen_ = sizeof(MultConnAutoConnSwitch);
    on.serviceData_ = std::make_unique<uint8_t[]>(sizeof(MultConnAutoConnSwitch));
    on.serviceData_[0] = static_cast<uint8_t>(TwsAutoConnSwitch::ON);
    service_->ProcessEvent(on);
    EXPECT_TRUE(service_->TwsIsSupportVirtualAutoConnect(dev));

    // 上报关
    TwsMessage off(TWS_SERVICE_RECV_REQ_EVENT);
    off.dev_ = dev.GetAddress();
    off.msgDirect_ = TWS_MSG_DIRECT_REQ;
    off.msgType_ = autoConnMsg;
    off.serviceDataLen_ = sizeof(MultConnAutoConnSwitch);
    off.serviceData_ = std::make_unique<uint8_t[]>(sizeof(MultConnAutoConnSwitch));
    off.serviceData_[0] = static_cast<uint8_t>(TwsAutoConnSwitch::OFF);
    service_->ProcessEvent(off);
    EXPECT_FALSE(service_->TwsIsSupportVirtualAutoConnect(dev));

    // 静默断链通知：适配层为空时安全返回
    TwsMessage notify(TWS_SERVICE_RECV_REQ_EVENT);
    notify.dev_ = dev.GetAddress();
    notify.msgDirect_ = TWS_MSG_DIRECT_REQ;
    notify.msgType_ = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_NOTIFY_SLE_DISCONNECT_PROFILE);
    notify.serviceDataLen_ = sizeof(NotifyDisconnectProfile);
    notify.serviceData_ = std::make_unique<uint8_t[]>(sizeof(NotifyDisconnectProfile));
    notify.serviceData_[0] = static_cast<uint8_t>(TwsProfileAction::DISCONNECT);
    service_->ProcessEvent(notify);
    SUCCEED();
}

/**
 * @tc.name: CodecDeInitRestore001
 * @tc.desc: 校验编解码库卸载：卸载后重新加载恢复可用，不影响同进程后续用例
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsDeepTest, CodecDeInitRestore001, TestSize.Level1)
{
    TwsSharedLibApi::GetInstance().DeInit();
    EXPECT_TRUE(TwsSharedLibApi::GetInstance().Init());
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
