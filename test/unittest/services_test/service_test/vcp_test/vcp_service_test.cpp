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
#include <memory>
#include "ThreadUtil.h"
#include "ProfileServiceManager.cpp"
#include "SleServiceManager.h"
#include "nearlink_access_token_mock.h"
#include "VcpService.h"
#include "ASCService.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class VcpClientObserverCommon : public VcpClientObserver {
public:
    VcpClientObserverCommon() = default;
    virtual ~VcpClientObserverCommon() = default;

    void OnConnectionStateChanged(const RawAddress &device, int state, int oldState){}
};

namespace {
VcpClientObserverCommon g_vcpServiceObserver_;
}

class NearlinkVcpServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

private:
    std::unique_ptr<VcpService> service_;
};

void NearlinkVcpServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkVcpServiceTest start");
    // VCP service tests own their service; vendor stack startup is an integration dependency.
    HILOGI("SetUpTestCase NearlinkVcpServiceTest end");
}

void NearlinkVcpServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkVcpServiceTest");
}

void NearlinkVcpServiceTest::SetUp()
{
    HILOGI("SetUp NearlinkVcpServiceTest.");
    // VCP tasks execute synchronously in the test thread; no sleep is needed after calls.
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_VCP, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    service_ = std::make_unique<VcpService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_VCP, service_.get());
    ASSERT_EQ(service_.get(), VcpService::GetService());
}

void NearlinkVcpServiceTest::TearDown()
{
    HILOGI("TearDown NearlinkVcpServiceTest.");
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_VCP, nullptr);
    service_.reset();
    ThreadUtil::GetInstance().InitThreadStateMap();
}

/**
 * @tc.number: GetService001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVcpServiceTest, GetService001, TestSize.Level1)
{
    HILOGI("GetService001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    HILOGI("GetService001 end");
}

/**
 * @tc.number: Disable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVcpServiceTest, Disable001, TestSize.Level1)
{
    HILOGI("Disable001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    vcpService->Disable();
    EXPECT_NE(nullptr, vcpService);

    vcpService->Disable();
    EXPECT_NE(nullptr, vcpService);
    HILOGI("Disable001 end");
}

/**
 * @tc.number: Enable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVcpServiceTest, Enable001, TestSize.Level1)
{
    HILOGI("Enable001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    vcpService->Enable();
    EXPECT_NE(nullptr, vcpService);

    vcpService->Enable();
    EXPECT_NE(nullptr, vcpService);
    HILOGI("Enable001 end");
}

/*
 * @tc.number: RegisterObserver001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVcpServiceTest, RegisterObserver001, TestSize.Level1)
{
    HILOGI("RegisterObserver001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    vcpService->RegisterObserver(g_vcpServiceObserver_);
    EXPECT_NE(nullptr, vcpService);
    HILOGI("RegisterObserver001 end");
}

/*
 * @tc.number: DeregisterObserver001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVcpServiceTest, DeregisterObserver001, TestSize.Level1)
{
    HILOGI("DeregisterObserver001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    vcpService->DeregisterObserver(g_vcpServiceObserver_);
    EXPECT_NE(nullptr, vcpService);
    HILOGI("DeregisterObserver001 end");
}

/**
 * @tc.number: SetDeviceAbsoluteVolume001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVcpServiceTest, SetDeviceAbsoluteVolume001, TestSize.Level1)
{
    HILOGI("SetDeviceAbsoluteVolume001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t volumeInvalid = -1;
    vcpService->SetDeviceAbsoluteVolume(*device, volumeInvalid,
        static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_MEDIA));
    EXPECT_NE(nullptr, vcpService);

    int32_t volumeFive = 5;
    vcpService->SetDeviceAbsoluteVolume(*device, volumeFive,
        static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_MEDIA));
    EXPECT_NE(nullptr, vcpService);

    vcpService->SetDeviceAbsoluteVolume(*device, volumeFive,
        static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_CALL));
    EXPECT_NE(nullptr, vcpService);
    HILOGI("SetDeviceAbsoluteVolume001 end");
}

/**
 * @tc.number: GetDeviceMediaVolume001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVcpServiceTest, GetDeviceMediaVolume001, TestSize.Level1)
{
    HILOGI("GetDeviceMediaVolume001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t mediaVolume = vcpService->GetDeviceMediaVolume(*device);
    EXPECT_NE(nullptr, vcpService);
    HILOGI("GetDeviceMediaVolume001 end");
}

/**
 * @tc.number: GetDeviceCallVolume001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVcpServiceTest, GetDeviceCallVolume001, TestSize.Level1)
{
    HILOGI("GetDeviceCallVolume001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t callVolume = vcpService->GetDeviceCallVolume(*device);
    EXPECT_NE(nullptr, vcpService);
    HILOGI("GetDeviceCallVolume001 end");
}

/**
 * @tc.number: SwitchAbsVolumeDevice001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVcpServiceTest, SwitchAbsVolumeDevice001, TestSize.Level1)
{
    HILOGI("SwitchAbsVolumeDevice001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    vcpService->SwitchAbsVolumeDevice(*device, true);
    EXPECT_NE(nullptr, vcpService);
    HILOGI("SwitchAbsVolumeDevice001 end");
}

/**
 * @tc.number: SetAllStreamVolume001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVcpServiceTest, SetAllStreamVolume001, TestSize.Level1)
{
    HILOGI("SetAllStreamVolume001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    vcpService->SetAllStreamVolume(*device);
    EXPECT_NE(nullptr, vcpService);
    HILOGI("SetAllStreamVolume001 end");
}

/*
 * @tc.number: GetConnectState001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVcpServiceTest, GetConnectState001, TestSize.Level1)
{
    HILOGI("GetConnectState001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    int connectState = vcpService->GetConnectState();
    EXPECT_EQ(0, connectState);
    HILOGI("GetConnectState001 end");
}

/*
 * @tc.number: Connect001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVcpServiceTest, Connect001, TestSize.Level1)
{
    HILOGI("Connect001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = vcpService->Connect(*device);
    EXPECT_EQ(VCP_SUCCESS, ret);
    HILOGI("Connect001 end");
}

/*
 * @tc.number: Disconnect001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVcpServiceTest, Disconnect001, TestSize.Level1)
{
    HILOGI("Disconnect001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = vcpService->Disconnect(*device);
    EXPECT_EQ(VCP_SUCCESS, ret);
    HILOGI("Disconnect001 end");
}

/*
 * @tc.number: GetConnectDevices001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVcpServiceTest, GetConnectDevices001, TestSize.Level1)
{
    HILOGI("GetConnectDevices001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    std::list<RawAddress> connectedDevices = vcpService->GetConnectDevices();
    EXPECT_NE(nullptr, vcpService);
    HILOGI("GetConnectDevices001 end");
}

/*
 * @tc.number: ProcessVolumeChangeEvent001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVcpServiceTest, ProcessVolumeChangeEvent001, TestSize.Level1)
{
    HILOGI("ProcessVolumeChangeEvent001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    std::string addr = "00:11:22:33:44:55";
    std::string addrOther = "00:11:22:33:44:66";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t volume = 0;
    ASCService *ascService = ASCService::GetService();
    ASSERT_NE(nullptr, ascService);
    ascService->SetActiveSinkDevice(NearlinkRawAddress(addrOther), 0);
    vcpService->ProcessVolumeChangeEvent(*device, volume);
    EXPECT_NE(nullptr, vcpService);

    ascService->SetActiveSinkDevice(NearlinkRawAddress(addr), 0);
    ascService->SetIsCallingFlag(false);
    vcpService->ProcessVolumeChangeEvent(*device, volume);
    EXPECT_NE(nullptr, vcpService);

    ascService->SetIsCallingFlag(true);
    vcpService->ProcessVolumeChangeEvent(*device, volume);
    EXPECT_NE(nullptr, vcpService);
    HILOGI("ProcessVolumeChangeEvent001 end");
}

/*
 * @tc.number: NotifyStateChanged001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVcpServiceTest, NotifyStateChanged001, TestSize.Level1)
{
    HILOGI("NotifyStateChanged001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    vcpService->NotifyStateChanged(*device, static_cast<int>(SleConnectState::CONNECTED),
        static_cast<int>(SleConnectState::DISCONNECTED));
    EXPECT_NE(nullptr, vcpService);
    HILOGI("NotifyStateChanged001 end");
}

/*
 * @tc.number: NotifyVolumeChanged001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVcpServiceTest, NotifyVolumeChanged001, TestSize.Level1)
{
    HILOGI("NotifyVolumeChanged001 start");
    VcpService* vcpService = VcpService::GetService();
    ASSERT_NE(nullptr, vcpService);
    std::string addr = "00:11:22:33:44:55";
    std::string addrOther = "00:11:22:33:44:66";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);

    std::list<StreamVolume> streamVolumes;
    StreamVolume callStreamVolume(1, static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_CALL),
        static_cast<uint8_t>(VolumeAdditionalInfo::SLE_SERVER_CHANGE_VOLUME));
    StreamVolume mediaStreamVolume(1, static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_MEDIA),
        static_cast<uint8_t>(VolumeAdditionalInfo::SLE_SERVER_CHANGE_VOLUME));
    streamVolumes.emplace_back(callStreamVolume);
    streamVolumes.emplace_back(mediaStreamVolume);

    ASCService *ascService = ASCService::GetService();
    ASSERT_NE(nullptr, ascService);
    ascService->SetActiveSinkDevice(NearlinkRawAddress(addrOther), 0);
    vcpService->NotifyVolumeChanged(*device, streamVolumes);
    EXPECT_NE(nullptr, vcpService);

    ascService->SetActiveSinkDevice(NearlinkRawAddress(addr), 0);
    ascService->SetIsCallingFlag(false);
    vcpService->NotifyVolumeChanged(*device, streamVolumes);
    EXPECT_NE(nullptr, vcpService);

    ascService->SetIsCallingFlag(true);
    vcpService->NotifyVolumeChanged(*device, streamVolumes);
    EXPECT_NE(nullptr, vcpService);
    HILOGI("NotifyVolumeChanged001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS