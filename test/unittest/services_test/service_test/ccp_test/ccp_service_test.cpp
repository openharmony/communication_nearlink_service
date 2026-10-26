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
#include "CcpService.h"
#include "CcpDefines.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "mock_nearlink_call_client.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
using namespace OHOS::Telephony;
using ::testing::Return;
using ::testing::_;

class CcpServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

private:
    std::unique_ptr<CcpService> service_;
};

void CcpServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase CcpServiceTest start");
    // CCP service tests own their service; vendor stack startup is an integration dependency.
    HILOGI("SetUpTestCase CcpServiceTest end");
}

void CcpServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase CcpServiceTest");
}

void CcpServiceTest::SetUp()
{
    HILOGI("SetUp CcpServiceTest.");
    // CCP tasks execute synchronously in the test thread; no sleep is needed after calls.
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_CCP, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    service_ = std::make_unique<CcpService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CCP, service_.get());
    ASSERT_EQ(service_.get(), CcpService::GetService());
}

void CcpServiceTest::TearDown()
{
    HILOGI("TearDown CcpServiceTest.");
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CCP, nullptr);
    service_.reset();
    ThreadUtil::GetInstance().InitThreadStateMap();
}

/**
 * @tc.number: GetService001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, GetService001, TestSize.Level1)
{
    HILOGI("GetService001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    HILOGI("GetService001 end");
}

/**
 * @tc.number: Disable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, Disable001, TestSize.Level1)
{
    HILOGI("Disable001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    ccpService->Disable();
    EXPECT_NE(nullptr, ccpService);

    ccpService->Disable();
    EXPECT_NE(nullptr, ccpService);
    HILOGI("Disable001 end");
}

/**
 * @tc.number: Enable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, Enable001, TestSize.Level1)
{
    HILOGI("Enable001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    ccpService->Enable();
    EXPECT_NE(nullptr, ccpService);

    ccpService->Enable();
    EXPECT_NE(nullptr, ccpService);
    HILOGI("Enable001 end");
}

/**
 * @tc.number: HandleAnswer001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandleAnswer001, TestSize.Level1)
{
    HILOGI("HandleAnswer001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t instanceId = 0;
    uint32_t requestId = 0;
    uint8_t callId = 0;

    ccpService->HandleAnswer(*device, instanceId, requestId, callId);
    EXPECT_NE(nullptr, ccpService);
    HILOGI("HandleAnswer001 end");
}

/**
 * @tc.number: HandleHangUp001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandleHangUp001, TestSize.Level1)
{
    HILOGI("HandleHangUp001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t instanceId = 0;
    uint32_t requestId = 0;
    uint8_t callId = 0;

    ccpService->HandleHangUp(*device, instanceId, requestId, callId);
    EXPECT_NE(nullptr, ccpService);

    instanceId = 1;
    requestId = 1;
    callId = 1;
    ccpService->HandleHangUp(*device, instanceId, requestId, callId);
    EXPECT_NE(nullptr, ccpService);
    HILOGI("HandleHangUp001 end");
}

/**
 * @tc.number: HandleAuthorize001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandleAuthorize001, TestSize.Level1)
{
    HILOGI("HandleAuthorize001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    int32_t instanceId = 0;
    uint32_t requestId = 0;
    int32_t property = 0;
    ccpService->HandleAuthorize(instanceId, requestId, property);
    EXPECT_NE(nullptr, ccpService);
    HILOGI("HandleAuthorize001 end");
}

/**
 * @tc.number: HandlePhoneStateChange001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandlePhoneStateChange001, TestSize.Level1)
{
    HILOGI("HandlePhoneStateChange001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    NearlinkCallPhoneState phoneState;
    phoneState.SetActiveNum(0);
    phoneState.SetHeldNum(0);
    phoneState.SetCallState(0);
    phoneState.SetCallType(0);

    ccpService->HandlePhoneStateChange(phoneState);
    EXPECT_NE(nullptr, ccpService);
    HILOGI("HandlePhoneStateChange001 end");
}

/**
 * @tc.number: HandleCallDetailChange001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandleCallDetailChange001, TestSize.Level1)
{
    HILOGI("HandleCallDetailChange001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    CallAttributeInfo callAttributeInfo;

    callAttributeInfo.callState = TelCallState::CALL_STATUS_DIALING;
    ccpService->HandleCallDetailChange(callAttributeInfo);
    EXPECT_NE(nullptr, ccpService);

    callAttributeInfo.callState = TelCallState::CALL_STATUS_INCOMING;
    ccpService->HandleCallDetailChange(callAttributeInfo);
    EXPECT_NE(nullptr, ccpService);

    callAttributeInfo.callState = TelCallState::CALL_STATUS_WAITING;
    ccpService->HandleCallDetailChange(callAttributeInfo);
    EXPECT_NE(nullptr, ccpService);

    callAttributeInfo.callState = TelCallState::CALL_STATUS_DISCONNECTED;
    ccpService->HandleCallDetailChange(callAttributeInfo);
    EXPECT_NE(nullptr, ccpService);

    callAttributeInfo.callState = TelCallState::CALL_STATUS_UNKNOWN;
    ccpService->HandleCallDetailChange(callAttributeInfo);
    EXPECT_NE(nullptr, ccpService);
    HILOGI("HandleCallDetailChange001 end");
}

/**
 * @tc.number: HandleVoipStop001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandleVoipStop001, TestSize.Level1)
{
    HILOGI("HandleVoipStop001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);

    ccpService->HandleVoipStop(*device);
    EXPECT_NE(nullptr, ccpService);

    ccpService->HandleVoipStop(*device);
    EXPECT_NE(nullptr, ccpService);
    HILOGI("HandleVoipStop001 end");
}

/**
 * @tc.number: HandleVoipStart001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandleVoipStart001, TestSize.Level1)
{
    HILOGI("HandleVoipStart001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);

    ccpService->HandleVoipStart(*device);
    EXPECT_NE(nullptr, ccpService);

    ccpService->HandleVoipStart(*device);
    EXPECT_NE(nullptr, ccpService);
    HILOGI("HandleVoipStart001 end");
}

/**
 * @tc.number: TryResumeCurrentCalls001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, TryResumeCurrentCalls001, TestSize.Level1)
{
    HILOGI("TryResumeCurrentCalls001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    ccpService->TryResumeCurrentCalls();
    EXPECT_NE(nullptr, ccpService);
    HILOGI("TryResumeCurrentCalls001 end");
}

/**
 * @tc.number: NotifyAudioDeviceAction001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, NotifyAudioDeviceAction001, TestSize.Level1)
{
    HILOGI("NotifyAudioDeviceAction001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int action = 0;
    ccpService->NotifyAudioDeviceAction(*device, action);
    EXPECT_NE(nullptr, ccpService);
    HILOGI("NotifyAudioDeviceAction001 end");
}

/**
 * @tc.number: ProcessCallStateInfo001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, ProcessCallStateInfo001, TestSize.Level1)
{
    HILOGI("ProcessCallStateInfo001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::vector<Telephony::CallAttributeInfo> callInfoVec;
    CallAttributeInfo callAttributeInfo;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_INCOMING;
    callInfoVec.push_back(callAttributeInfo);

    ccpService->ProcessCallStateInfo(callInfoVec);
    EXPECT_NE(nullptr, ccpService);
    HILOGI("ProcessCallStateInfo001 end");
}

/**
 * @tc.number: ProcessAccountInfo001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, ProcessAccountInfo001, TestSize.Level1)
{
    HILOGI("ProcessAccountInfo001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::shared_ptr<CallInOutInfoProp> prop = std::make_shared<CallInOutInfoProp>();
    prop->callId = 1;
    prop->callFlag = 0;
    CallAttributeInfo callAttributeInfo;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_INCOMING;
    ccpService->ProcessAccountInfo(prop, callAttributeInfo);
    EXPECT_NE(nullptr, ccpService);
    HILOGI("ProcessAccountInfo001 end");
}

/**
 * @tc.number: IsNearlinkActiveDevice001
 * @tc.name: IsNearlinkActiveDevice via ProfileASC interface
 * @tc.desc: Test IsNearlinkActiveDevice uses ProfileASC from profile manager, should not crash
 */
HWTEST_F(CcpServiceTest, IsNearlinkActiveDevice001, TestSize.Level1)
{
    HILOGI("IsNearlinkActiveDevice001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);
    RawAddress reportAddr("11:22:33:44:55:66");
    bool ret = ccpService->IsNearlinkActiveDevice(reportAddr);
    EXPECT_FALSE(ret);
    HILOGI("IsNearlinkActiveDevice001 end");
}

/**
 * @tc.number: NotifyAudioDeviceActionViaProfileAsc_001
 * @tc.name: NotifyAudioDeviceAction via ProfileASC interface
 * @tc.desc: Test NotifyAudioDeviceAction uses ProfileASC from profile manager, should not crash
 */
HWTEST_F(CcpServiceTest, NotifyAudioDeviceActionViaProfileAsc_001, TestSize.Level1)
{
    HILOGI("NotifyAudioDeviceActionViaProfileAsc_001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);
    RawAddress devAddr("11:22:33:44:55:66");
    ccpService->NotifyAudioDeviceAction(devAddr, 1);
    HILOGI("NotifyAudioDeviceActionViaProfileAsc_001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS