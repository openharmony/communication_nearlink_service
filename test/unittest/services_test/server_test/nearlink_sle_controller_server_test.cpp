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
#include "nearlink_sle_controller_server.h"
#include "nearlink_sle_controller_stub.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
sptr<NearlinkSleControllerServer> g_sleController = new NearlinkSleControllerServer();
}

class NearlinkSleControllerServerStubTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkSleControllerServerStubTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkSleControllerServerTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
}

void NearlinkSleControllerServerStubTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkSleControllerServerTest");
}

void NearlinkSleControllerServerStubTest::SetUp()
{
    HILOGI("SetUp NearlinkSleControllerServerTest.");
}

void NearlinkSleControllerServerStubTest::TearDown()
{
    HILOGI("TearDown NearlinkSleControllerServerTest.");
}

int32_t SleControllerOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_sleController, TRANSACTION_ERR, "g_sleController is nullptr");
    MessageOption option = { MessageOption::TF_SYNC };
    HILOGI("g_sleController OnRemoteRequest, cmd(%{public}d)", code);
    return g_sleController->OnRemoteRequest(code, data, reply, option);
}

/**
 * @tc.name: SetSleCoexParam001
 * @tc.desc: 测试 SetSleCoexParam 校验 calling name 拦截
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSleControllerServerStubTest, SetSleCoexParam001, TestSize.Level1)
{
    HILOGI("NearlinkSleControllerServerStubTest:SetSleCoexParam001 start");
    uint16_t maxBitRate = 1000; // 假设设置最大比特率为 1000
    uint8_t dutyCycle = 0xFF;   // 假设设置非法占空比用于测试拦截
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSleControllerStub::GetDescriptor());
    data.WriteUint16(maxBitRate);
    data.WriteUint8(dutyCycle);
    int32_t ret = SleControllerOnRemoteRequest(
        NearlinkSleControllerInterfaceCode::NL_SET_SLE_COEX_PARAM, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkSleControllerServerStubTest:SetSleCoexParam001 end");
}

/**
 * @tc.name: UpdateConnectInterval001
 * @tc.desc: 测试 UpdateConnectInterval
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSleControllerServerStubTest, UpdateConnectInterval001, TestSize.Level1)
{
    HILOGI("NearlinkSleControllerServerStubTest:UpdateConnectInterval001 start");
    int32_t intervalType = 0x05; // 假设测试类型为 0x05
    std::string address = "00:00:00:00:00:00";
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSleControllerStub::GetDescriptor());
    data.WriteString(address);
    data.WriteInt32(intervalType);
    int32_t ret = SleControllerOnRemoteRequest(
        NearlinkSleControllerInterfaceCode::NL_SLE_UPDATE_INTERVAL, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkSleControllerServerStubTest:UpdateConnectInterval001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
