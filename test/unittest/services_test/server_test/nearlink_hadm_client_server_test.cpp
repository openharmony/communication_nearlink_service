/*
 * Copyright (C) 2026 Huawei Device Co., Ltd. All rights reserved.
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

#include "nearlink_hadm_client_server.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "nearlink_sle_advertiser_callback_proxy.h"
#include "nearlink_hadm_client_callback_stub.h"
#include "nearlink_hadm_client_server.cpp"
#include "log.h"
#include <gmock/gmock.h>

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
constexpr int TDD_DELAY_50_MS = 50;
constexpr int HOST_SERVER_TDD_DELAY_1000_MS = 1000;
sptr<NearlinkHadmClientServer> g_hadmClient = new (std::nothrow) NearlinkHadmClientServer();
sptr<NearlinkHadmClientServer> g_hadmClientServer = new (std::nothrow) NearlinkHadmClientServer();
}

class NearlinkHadmClientCallbackStubTest : public NearlinkHadmClientCallbackStub {
public:
    void OnSoundingResult(const NearlinkRawAddress &addr, const NearlinkHadmClientSoundingResult &result) override
    {}
    void OnSoundingStateChange(const NearlinkRawAddress &addr, int newState, int errorCode) override
    {}

    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient) override {
        HILOGI("Mock AddDeathRecipient is true");
        return true;
    }

    explicit NearlinkHadmClientCallbackStubTest()
    {}
    ~NearlinkHadmClientCallbackStubTest() override
    {}
};

class MockNearlinkHadmClientCallback : public INearlinkHadmClientCallback {
public:
    MockNearlinkHadmClientCallback() = default;
    virtual ~MockNearlinkHadmClientCallback() = default;

    sptr<OHOS::IRemoteObject> AsObject() override
    {
        sptr<NearlinkHadmClientCallbackStubTest> nearlinkHadmClientCallbackStubTest =
        new NearlinkHadmClientCallbackStubTest();
        return nearlinkHadmClientCallbackStubTest;
    }
    
    void OnSoundingResult(const NearlinkRawAddress &addr, const NearlinkHadmClientSoundingResult &result) override 
    {
        HILOGI("MockNearlinkHadmClientCallback::OnSoundingResult");
    }

    void OnSoundingStateChange(const NearlinkRawAddress &addr, int newState, int errorCode) override 
    {
        HILOGI("MockNearlinkHadmClientCallback::OnSoundingStateChange");
    }
};

class NearlinkHadmClientSeverStubTest : public testing::Test {
public:
    NearlinkHadmClientSeverStubTest()
    {}
    ~NearlinkHadmClientSeverStubTest()
    {}

    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

    uint32_t hadmId = 0;
    NearlinkHadmClientSoundingResult hadmClientSoundingResult_ {};
    sptr<MockNearlinkHadmClientCallback> callback_ {nullptr};
    sptr<NearlinkHadmClientCallbackStubTest> nearlinkHadmClientCallbackStubTest =
        new NearlinkHadmClientCallbackStubTest();
};

void NearlinkHadmClientSeverStubTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkHadmClientSeverStubTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
}

void NearlinkHadmClientSeverStubTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkHadmClientSeverStubTest");
}

void NearlinkHadmClientSeverStubTest::SetUp()
{
    HILOGI("SetUp NearlinkHadmClientSeverStubTest.");
    callback_ = sptr<MockNearlinkHadmClientCallback>(new MockNearlinkHadmClientCallback());
    RawAddress remoteAddr("00:11:22:33:44:55");
    hadmClientSoundingResult_.addr_ = remoteAddr;
    g_hadmClient->RegisterNearlinkHadmClientCallback(hadmId, callback_);
}

void NearlinkHadmClientSeverStubTest::TearDown()
{
    HILOGI("TearDown NearlinkHadmClientSeverStubTest.");
    g_hadmClient->DeregisterNearlinkHadmClientCallback(hadmId, callback_);
    callback_ = nullptr;
}

int32_t HadmClientOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    if (!g_hadmClient) {
        g_hadmClient = new (std::nothrow) NearlinkHadmClientServer();
    }
    NL_CHECK_RETURN_RET(g_hadmClient, TRANSACTION_ERR, "g_hadmClient is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("HadmClientOnRemoteRequest, cmd(%{public}d)", code);
    return g_hadmClient->OnRemoteRequest(code, data, reply, option);
}

/**
 * @tc.name: StartSounding
 * @tc.desc: Test the StartSounding function with valid parameters.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHadmClientSeverStubTest, StartSounding001, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientSeverStubTest:StartSounding001 start");
    NearlinkRawAddress addr; 
    addr.SetAddress("00:11:22:33:44:55"); 
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHadmClientStub::GetDescriptor());
    data.WriteUint32(hadmId);
    data.WriteParcelable(&addr);
    int32_t result = HadmClientOnRemoteRequest(NearlinkHadmClientInterfaceCode::NL_HADM_CLIENT_START_SOUNDING, data, reply);
    EXPECT_EQ(NO_ERROR, result);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::TDD_DELAY_50_MS));
    HILOGI("NearlinkHadmClientSeverStubTest:StartSounding001 end");
}

/**
 * @tc.name: GetHadmFeature
 * @tc.desc: Test GetHadmFeature function
 * @tc.type: FUNC
 */
 HWTEST_F(NearlinkHadmClientSeverStubTest, GetHadmFeature001, TestSize.Level1)
 {
    HILOGI("NearlinkHadmClientSeverStubTest:GetHadmFeature001 end");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHadmClientStub::GetDescriptor());
    int32_t result = HadmClientOnRemoteRequest(NearlinkHadmClientInterfaceCode::NL_GET_HADM_FEATURE, data, reply);
    EXPECT_EQ(NO_ERROR, result);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::TDD_DELAY_50_MS));
    HILOGI("NearlinkHadmClientSeverStubTest:GetHadmFeature001 end");
}

/**
 * @tc.name: StopSounding
 * @tc.desc: Test the StopSounding function with valid parameters.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHadmClientSeverStubTest, StopSounding001, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientSeverStubTest:StopSounding001 start");
    NearlinkRawAddress addr;
    addr.SetAddress("00:11:22:33:44:55");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHadmClientStub::GetDescriptor());
    data.WriteUint32(hadmId);
    data.WriteParcelable(&addr);
    int32_t result = HadmClientOnRemoteRequest(NearlinkHadmClientInterfaceCode::NL_HADM_CLIENT_STOP_SOUNDING, data, reply);
    EXPECT_EQ(NO_ERROR, result);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::TDD_DELAY_50_MS));
    HILOGI("NearlinkHadmClientSeverStubTest:StopSounding001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS