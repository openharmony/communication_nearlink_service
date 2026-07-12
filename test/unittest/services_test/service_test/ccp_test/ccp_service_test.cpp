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
#include <thread>
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

namespace {
constexpr int CCP_SERVICE_UT_DELAY_50_MS = 50;
constexpr int CCP_SERVICE_UT_DELAY_1000_MS = 1000;
}

class CcpServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void CcpServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase CcpServiceTest start");
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_1000_MS));
    HILOGI("SetUpTestCase CcpServiceTest end");
}

void CcpServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase CcpServiceTest");
}

void CcpServiceTest::SetUp()
{
    HILOGI("SetUp CcpServiceTest.");
}

void CcpServiceTest::TearDown()
{
    HILOGI("TearDown CcpServiceTest.");
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
    EXPECT_NE(nullptr, ccpService);
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
    ccpService->Disable();
    EXPECT_NE(nullptr, ccpService);

    ccpService->Disable();
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
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
    ccpService->Enable();
    EXPECT_NE(nullptr, ccpService);

    ccpService->Enable();
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
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
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t instanceId = 0;
    uint32_t requestId = 0;
    uint8_t callId = 0;

    ccpService->HandleAnswer(*device, instanceId, requestId, callId);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
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
    int32_t instanceId = 0;
    uint32_t requestId = 0;
    int32_t property = 0;
    ccpService->HandleAuthorize(instanceId, requestId, property);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
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
    NearlinkCallPhoneState phoneState;
    phoneState.SetActiveNum(0);
    phoneState.SetHeldNum(0);
    phoneState.SetCallState(0);
    phoneState.SetCallType(0);

    ccpService->HandlePhoneStateChange(phoneState);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
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
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("HandleCallDetailChange001 end");
}

/**
 * @tc.number: HandleMeeTimeDetailsChange001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandleMeeTimeDetailsChange001, TestSize.Level1)
{
    HILOGI("HandleMeeTimeDetailsChange001 start");
    CcpService* ccpService = CcpService::GetService();
    CallAttributeInfo callAttributeInfo;

    callAttributeInfo.callState = Telephony::TelCallState::CALL_STATUS_DIALING;
    ccpService->HandleMeeTimeDetailsChange(callAttributeInfo);
    EXPECT_NE(nullptr, ccpService);

    callAttributeInfo.callState = Telephony::TelCallState::CALL_STATUS_INCOMING;
    ccpService->HandleMeeTimeDetailsChange(callAttributeInfo);
    EXPECT_NE(nullptr, ccpService);

    callAttributeInfo.callState = Telephony::TelCallState::CALL_STATUS_UNKNOWN;
    ccpService->HandleMeeTimeDetailsChange(callAttributeInfo);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("HandleMeeTimeDetailsChange001 end");
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
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);

    ccpService->HandleVoipStop(*device);
    EXPECT_NE(nullptr, ccpService);

    ccpService->HandleVoipStop(*device);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
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
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);

    ccpService->HandleVoipStart(*device);
    EXPECT_NE(nullptr, ccpService);

    ccpService->HandleVoipStart(*device);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
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
    ccpService->TryResumeCurrentCalls();
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
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
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int action = 0;
    ccpService->NotifyAudioDeviceAction(*device, action);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
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
    std::vector<Telephony::CallAttributeInfo> callInfoVec;
    CallAttributeInfo callAttributeInfo;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_INCOMING;
    callInfoVec.push_back(callAttributeInfo);

    ccpService->ProcessCallStateInfo(callInfoVec);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
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
    std::shared_ptr<CallInOutInfoProp> prop = std::make_shared<CallInOutInfoProp>();
    prop->callId = 1;
    prop->callFlag = 0;
    CallAttributeInfo callAttributeInfo;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_INCOMING;
    ccpService->ProcessAccountInfo(prop, callAttributeInfo);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("ProcessAccountInfo001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS