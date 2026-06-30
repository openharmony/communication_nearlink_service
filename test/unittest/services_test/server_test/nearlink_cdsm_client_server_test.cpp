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
#include "nearlink_cdsm_client_server.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "log.h"


namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
constexpr int HOST_FUZZ_DELAY_50_MS = 50;
constexpr int HOST_SERVER_TDD_DELAY_1000_MS = 1000;
sptr<NearlinkCdsmClientServer> g_cdsmServer = new (std::nothrow) NearlinkCdsmClientServer();
}

class NearlinkCdsmClientServerStubTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkCdsmClientServerStubTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkCdsmClientServerStubTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
}

void NearlinkCdsmClientServerStubTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkCdsmClientServerStubTest");
}

void NearlinkCdsmClientServerStubTest::SetUp()
{
    HILOGI("SetUp NearlinkCdsmClientServerStubTest.");
}

void NearlinkCdsmClientServerStubTest::TearDown()
{
    HILOGI("TearDown NearlinkCdsmClientServerStubTest.");
}

class NearlinkCdsmClientCallbackStubImplTest : public IRemoteStub<INearlinkCdsmClientCallback> {
public:
    void OnCdsInfoChanged(const NearlinkCdsInfoParcel &cdsInfo) override
    {
        HILOGI("OnCdsInfoChanged");
    }

private:
};

int32_t CdsmClientOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_cdsmServer, TRANSACTION_ERR, "g_cdsmServer is nullptr");
    MessageOption option = { MessageOption::TF_SYNC };
    HILOGI("g_cdsmServer OnRemoteRequest, cmd(%{public}d)", code);
    return g_cdsmServer->OnRemoteRequest(code, data, reply, option);
}

/**
 * @tc.name: RegisterApplication
 * @tc.desc: Test the RegisterApplication function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmClientServerStubTest, RegisterApplication001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmClientServerTest:RegisterApplication001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCdsmClientStub::GetDescriptor());
    int32_t ret = CdsmClientOnRemoteRequest(
        NearlinkCdsmClientInterfaceCode::NL_REGISTER_CDSM_CLIENT_CALLBACK, data, reply);
    EXPECT_NE(NO_ERROR, ret);
    HILOGI("NearlinkCdsmClientServerTest:RegisterApplication001 end");
}

/**
 * @tc.name: RegisterApplication
 * @tc.desc: Test the RegisterApplication function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmClientServerStubTest, RegisterApplication002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmClientServerTest:RegisterApplication002 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCdsmClientStub::GetDescriptor());

    NearlinkRawAddress addr("11:22:33:44:55:66");
    data.WriteParcelable(&addr);

    int32_t ret = CdsmClientOnRemoteRequest(
        NearlinkCdsmClientInterfaceCode::NL_REGISTER_CDSM_CLIENT_CALLBACK, data, reply);
    EXPECT_NE(NO_ERROR, ret);
    HILOGI("NearlinkCdsmClientServerTest:RegisterApplication002 end");
}

/**
 * @tc.name: RegisterApplication
 * @tc.desc: Test the RegisterApplication function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmClientServerStubTest, RegisterApplication003, TestSize.Level1)
{
    HILOGI("NearlinkCdsmClientServerTest:RegisterApplication003 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCdsmClientStub::GetDescriptor());

    NearlinkRawAddress addr("11:22:33:44:55:66");
    data.WriteParcelable(&addr);

    sptr<NearlinkCdsmClientCallbackStubImplTest> clientCallback =
        new (std::nothrow) NearlinkCdsmClientCallbackStubImplTest();
    data.WriteRemoteObject(clientCallback->AsObject());
    int32_t ret = CdsmClientOnRemoteRequest(
        NearlinkCdsmClientInterfaceCode::NL_REGISTER_CDSM_CLIENT_CALLBACK, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkCdsmClientServerTest:RegisterApplication003 end");
}

/**
 * @tc.name: DeregisterApplication
 * @tc.desc: Test the DeregisterApplication function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmClientServerStubTest, DeregisterApplication001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmClientServerTest:DeregisterApplication001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCdsmClientStub::GetDescriptor());
    int32_t ret = CdsmClientOnRemoteRequest(
        NearlinkCdsmClientInterfaceCode::NL_DE_REGISTER_CDSM_CLIENT_CALLBACK, data, reply);
    EXPECT_NE(NO_ERROR, ret);
    HILOGI("NearlinkCdsmClientServerTest:DeregisterApplication001 end");
}

/**
 * @tc.name: DeregisterApplication
 * @tc.desc: Test the DeregisterApplication function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmClientServerStubTest, DeregisterApplication002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmClientServerTest:DeregisterApplication002 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCdsmClientStub::GetDescriptor());

    NearlinkRawAddress addr("11:22:33:44:55:66");
    data.WriteParcelable(&addr);

    int32_t ret = CdsmClientOnRemoteRequest(
        NearlinkCdsmClientInterfaceCode::NL_DE_REGISTER_CDSM_CLIENT_CALLBACK, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkCdsmClientServerTest:DeregisterApplication002 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
