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


namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing;
using namespace testing::ext;

constexpr int DELAY_LITTLE_MS = 100;

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
};

void NearlinkTwsTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkTwsTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

void NearlinkTwsTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkTwsTest");
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

void NearlinkTwsTest::SetUp()
{
    HILOGI("SetUp NearlinkTwsTest.");
}

void NearlinkTwsTest::TearDown()
{
    HILOGI("TearDown NearlinkTwsTest.");
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    int ret = twsService->Connect(device);
    EXPECT_EQ(ret, NL_NO_ERROR);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::list<RawAddress> devlist = twsService->GetConnectDevices();
    uint8_t nature = twsService->TwsGetDeviceNature(device);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    uint8_t mediaState = twsService->TwsGetDeviceAudioMusicType(device);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    uint32_t audioDelay;
    twsService->GetTwsAudioDelay(device, audioDelay);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    TwsDevWearStatus wearStatus;
    twsService->TwsGetDeviceWearStatus(device, wearStatus);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->TwsUpdateDeviceDefaultRole(device, static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY));
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->ProcPauseRecordMap();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->UpdateHangUpTimeStamp(device);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->SendProfileConnected(device);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->NotifyStateChanged(device, TwsClientState::TWS_STATE_CONNECTED, TwsClientState::TWS_STATE_CONNECTED);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->NotifyStateChanged(device, TwsClientState::TWS_STATE_CONNECTED, TwsClientState::TWS_STATE_DISCONNECTED);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->NotifyStateChanged(device, TwsClientState::TWS_STATE_CONNECTED, TwsClientState::TWS_STATE_DISCONNECTED);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->PostEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    TwsWearStateData wearData;
    twsService->UpdateDeviceWearState(device, wearData);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    uint8_t dataType = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_ROLE_TYPE);
    TwsClientData isoHandoverData(device);
    isoHandoverData.roleType_ = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY);
    twsService->UpdateClientData(dataType, isoHandoverData);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    RawAddress reportAddr = twsService->GetReportAddr(device);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->ResetVirtualAutoSwitch(device);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->RecoverDataFromConf(device);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    RawAddress primaryAddr = twsService->GetPrimaryAddr(device);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    twsService->TwsServiceSendReqInner(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
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
    std::list<RawAddress> devlist = twsService->GetConnectDevices();
    twsService->Disconnect(device);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkTwsTest:Disconnect001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
