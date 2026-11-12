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
#include "ipc_types.h"
#include "nearlink_ccp_client_stub.h"
#include "nearlink_ccp_client_server.h"
#include "nearlink_service_ipc_interface_code.h"
#include "nearlink_ccp_export.h"
#include "nearlink_errorcode.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceProfileManager.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
constexpr int CCP_CLIENT_SERVER_TDD_DELAY_50_MS = 50;
constexpr int CCP_CLIENT_SERVER_TDD_DELAY_1000_MS = 1000;
sptr<NearlinkCcpClientServer> g_ccpClient = new (std::nothrow) NearlinkCcpClientServer();

/* CCP 服务对象是否创建取决于产品配置与起栈结果，测试按实际可用性推导期望值：
 * 可用时受理回 0，不可用时在受理前拒绝回 NL_CCP_ERR_UNAVAILABLE_PROXY */
NlErrCode ExpectedCcpPushResult()
{
    bool isAvailable =
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CCP) != nullptr;
    return isAvailable ? NL_NO_ERROR : static_cast<NlErrCode>(NL_CCP_ERR_UNAVAILABLE_PROXY);
}
}

class NearlinkCcpClientServerStubTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkCcpClientServerStubTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkCcpClientServerStubTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    /* PhoneStateChanged 会投递到 CCP 串行线程，先拉起接口线程避免任务丢失 */
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_CLIENT_SERVER_TDD_DELAY_1000_MS));
    HILOGI("SetUpTestCase end NearlinkCcpClientServerStubTest");
}

void NearlinkCcpClientServerStubTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkCcpClientServerStubTest");
}

void NearlinkCcpClientServerStubTest::SetUp()
{
    HILOGI("SetUp NearlinkCcpClientServerStubTest.");
}

void NearlinkCcpClientServerStubTest::TearDown()
{
    HILOGI("TearDown NearlinkCcpClientServerStubTest.");
}

int32_t CcpClientOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_ccpClient, TRANSACTION_ERR, "g_ccpClient is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("NearlinkCcpClientServerStubTest, cmd(%{public}d)", code);
    return g_ccpClient->OnRemoteRequest(code, data, reply, option);
}

/**
 * @tc.number: PhoneStateChanged001
 * @tc.name:
 * @tc.desc: stub 反序列化(callId/numActive/numHeld/callState/number)后转发服务，星闪开启时立即回包：
 *           服务对象可用回 NL_NO_ERROR，不可用时受理前拒绝回 NL_CCP_ERR_UNAVAILABLE_PROXY
 *           （星闪开关由 mock_nearlink_host_server 恒置为开，故不考虑 NL_CCP_ERR_SLE_OFF）
 */
HWTEST_F(NearlinkCcpClientServerStubTest, PhoneStateChanged001, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged001 start");
    const std::string number = "11111111111";
    int32_t callId = 1;
    int32_t numActive = 1;
    int32_t numHeld = 0;
    int32_t callState = 0;

    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCcpClientStub::GetDescriptor());
    data.WriteInt32(callId);
    data.WriteInt32(numActive);
    data.WriteInt32(numHeld);
    data.WriteInt32(callState);
    data.WriteString(number);
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_PHONE_STATE_CHANGED, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(ExpectedCcpPushResult(), result);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_CLIENT_SERVER_TDD_DELAY_50_MS));
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged001 end");
}

/**
 * @tc.number: PhoneStateChanged002
 * @tc.name:
 * @tc.desc: callId 为 -1（no specific call）也应正常透传，服务侧立即回包，不得因异常入参崩溃
 */
HWTEST_F(NearlinkCcpClientServerStubTest, PhoneStateChanged002, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged002 start");
    const std::string number = "";
    int32_t callId = -1;
    int32_t numActive = 0;
    int32_t numHeld = 0;
    int32_t callState = 0;

    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCcpClientStub::GetDescriptor());
    data.WriteInt32(callId);
    data.WriteInt32(numActive);
    data.WriteInt32(numHeld);
    data.WriteInt32(callState);
    data.WriteString(number);
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_PHONE_STATE_CHANGED, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(ExpectedCcpPushResult(), result);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_CLIENT_SERVER_TDD_DELAY_50_MS));
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged002 end");
}

/**
 * @tc.number: PhoneStateChanged003
 * @tc.name:
 * @tc.desc: interface token 不匹配时拒绝请求，返回 IPC 传输失败，不落业务线程
 */
HWTEST_F(NearlinkCcpClientServerStubTest, PhoneStateChanged003, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged003 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(u"ohos.ipc.INearlinkWrongDescriptor");
    data.WriteInt32(1);
    data.WriteInt32(0);
    data.WriteInt32(0);
    data.WriteInt32(0);
    data.WriteString("10086");
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_PHONE_STATE_CHANGED, data, reply);
    EXPECT_EQ(static_cast<int32_t>(NL_ERR_IPC_TRANS_FAILED), ret);
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged003 end");
}

/**
 * @tc.number: CallDetailsChanged001
 * @tc.name:
 * @tc.desc: stub 反序列化(callId/callState/callType)后转发服务，星闪开启时立即回包：
 *           服务对象可用回 NL_NO_ERROR，不可用时受理前拒绝回 NL_CCP_ERR_UNAVAILABLE_PROXY
 *           （星闪开关由 mock_nearlink_host_server 恒置为开，故不考虑 NL_CCP_ERR_SLE_OFF）
 */
HWTEST_F(NearlinkCcpClientServerStubTest, CallDetailsChanged001, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:CallDetailsChanged001 start");
    int32_t callId = 1;
    int32_t callState = 0;
    int32_t callType = 0;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCcpClientStub::GetDescriptor());
    data.WriteInt32(callId);
    data.WriteInt32(callState);
    data.WriteInt32(callType);
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_CALL_DETAILS_CHANGED, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(ExpectedCcpPushResult(), result);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_CLIENT_SERVER_TDD_DELAY_50_MS));
    HILOGI("NearlinkCcpClientServerStubTest:CallDetailsChanged001 end");
}

/**
 * @tc.number: CallDetailsChanged002
 * @tc.name:
 * @tc.desc: callId 为 -1（no specific call）也应正常透传，服务侧立即回包，不得因异常入参崩溃
 */
HWTEST_F(NearlinkCcpClientServerStubTest, CallDetailsChanged002, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:CallDetailsChanged002 start");
    int32_t callId = -1;
    int32_t callState = 0;
    int32_t callType = 0;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCcpClientStub::GetDescriptor());
    data.WriteInt32(callId);
    data.WriteInt32(callState);
    data.WriteInt32(callType);
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_CALL_DETAILS_CHANGED, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(ExpectedCcpPushResult(), result);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_CLIENT_SERVER_TDD_DELAY_50_MS));
    HILOGI("NearlinkCcpClientServerStubTest:CallDetailsChanged002 end");
}

/**
 * @tc.number: CallDetailsChanged003
 * @tc.name:
 * @tc.desc: interface token 不匹配时拒绝请求，返回 IPC 传输失败，不落业务线程
 */
HWTEST_F(NearlinkCcpClientServerStubTest, CallDetailsChanged003, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:CallDetailsChanged003 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(u"ohos.ipc.INearlinkWrongDescriptor");
    data.WriteInt32(1);
    data.WriteInt32(0);
    data.WriteInt32(0);
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_CALL_DETAILS_CHANGED, data, reply);
    EXPECT_EQ(static_cast<int32_t>(NL_ERR_IPC_TRANS_FAILED), ret);
    HILOGI("NearlinkCcpClientServerStubTest:CallDetailsChanged003 end");
}

/**
 * @tc.number: PhoneStateChanged004
 * @tc.name:
 * @tc.desc: callId 为 0 不属于协议受理范围（1~255 或 -1），stub 在转服务前直接拒绝，不投递到 CCP 线程
 */
HWTEST_F(NearlinkCcpClientServerStubTest, PhoneStateChanged004, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged004 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCcpClientStub::GetDescriptor());
    data.WriteInt32(0);
    data.WriteInt32(0);
    data.WriteInt32(0);
    data.WriteInt32(0);
    data.WriteString("10086");
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_PHONE_STATE_CHANGED, data, reply);
    EXPECT_EQ(static_cast<int32_t>(ERR_INVALID_VALUE), ret);
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged004 end");
}

/**
 * @tc.number: PhoneStateChanged005
 * @tc.name:
 * @tc.desc: 参数区不足（只写 3 个 int32）时立即拒绝，ReadInt32 静默补 0 的截断报文不得穿透
 */
HWTEST_F(NearlinkCcpClientServerStubTest, PhoneStateChanged005, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged005 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCcpClientStub::GetDescriptor());
    data.WriteInt32(1);
    data.WriteInt32(0);
    data.WriteInt32(0);
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_PHONE_STATE_CHANGED, data, reply);
    EXPECT_EQ(static_cast<int32_t>(ERR_INVALID_VALUE), ret);
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged005 end");
}

/**
 * @tc.number: PhoneStateChanged006
 * @tc.name:
 * @tc.desc: numActive 超出值域上界（0~255）时拒绝，通话路数越界不得投递到 CCP 线程
 */
HWTEST_F(NearlinkCcpClientServerStubTest, PhoneStateChanged006, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged006 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCcpClientStub::GetDescriptor());
    data.WriteInt32(1);
    data.WriteInt32(256);
    data.WriteInt32(0);
    data.WriteInt32(0);
    data.WriteString("10086");
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_PHONE_STATE_CHANGED, data, reply);
    EXPECT_EQ(static_cast<int32_t>(ERR_INVALID_VALUE), ret);
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged006 end");
}

/**
 * @tc.number: PhoneStateChanged007
 * @tc.name:
 * @tc.desc: numHeld 为负数时拒绝，值域下界（0）不得被突破
 */
HWTEST_F(NearlinkCcpClientServerStubTest, PhoneStateChanged007, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged007 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCcpClientStub::GetDescriptor());
    data.WriteInt32(1);
    data.WriteInt32(0);
    data.WriteInt32(-1);
    data.WriteInt32(0);
    data.WriteString("10086");
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_PHONE_STATE_CHANGED, data, reply);
    EXPECT_EQ(static_cast<int32_t>(ERR_INVALID_VALUE), ret);
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged007 end");
}

/**
 * @tc.number: PhoneStateChanged008
 * @tc.name:
 * @tc.desc: callState 超出 TelCallState 值域（-1~9）时拒绝，非法状态不得穿透到业务层
 */
HWTEST_F(NearlinkCcpClientServerStubTest, PhoneStateChanged008, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged008 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCcpClientStub::GetDescriptor());
    data.WriteInt32(1);
    data.WriteInt32(0);
    data.WriteInt32(0);
    data.WriteInt32(10);
    data.WriteString("10086");
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_PHONE_STATE_CHANGED, data, reply);
    EXPECT_EQ(static_cast<int32_t>(ERR_INVALID_VALUE), ret);
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged008 end");
}

/**
 * @tc.number: PhoneStateChanged009
 * @tc.name:
 * @tc.desc: callId 超出协议上界（255）时拒绝，与 004（下界 0）构成双向覆盖
 */
HWTEST_F(NearlinkCcpClientServerStubTest, PhoneStateChanged009, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged009 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCcpClientStub::GetDescriptor());
    data.WriteInt32(256);
    data.WriteInt32(0);
    data.WriteInt32(0);
    data.WriteInt32(0);
    data.WriteString("10086");
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_PHONE_STATE_CHANGED, data, reply);
    EXPECT_EQ(static_cast<int32_t>(ERR_INVALID_VALUE), ret);
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged009 end");
}

/**
 * @tc.number: PhoneStateChanged010
 * @tc.name:
 * @tc.desc: callState 低于 TelCallState 值域下界（-1）时拒绝，与 008（上界 10）构成双向覆盖
 */
HWTEST_F(NearlinkCcpClientServerStubTest, PhoneStateChanged010, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged010 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCcpClientStub::GetDescriptor());
    data.WriteInt32(1);
    data.WriteInt32(0);
    data.WriteInt32(0);
    data.WriteInt32(-2);
    data.WriteString("10086");
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_PHONE_STATE_CHANGED, data, reply);
    EXPECT_EQ(static_cast<int32_t>(ERR_INVALID_VALUE), ret);
    HILOGI("NearlinkCcpClientServerStubTest:PhoneStateChanged010 end");
}

/**
 * @tc.number: CallDetailsChanged004
 * @tc.name:
 * @tc.desc: callType 超出已知枚举（TYPE_CS~TYPE_BLUETOOTH）时拒绝，范围外值不得穿透到业务层
 */
HWTEST_F(NearlinkCcpClientServerStubTest, CallDetailsChanged004, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:CallDetailsChanged004 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCcpClientStub::GetDescriptor());
    data.WriteInt32(1);
    data.WriteInt32(0);
    data.WriteInt32(7);
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_CALL_DETAILS_CHANGED, data, reply);
    EXPECT_EQ(static_cast<int32_t>(ERR_INVALID_VALUE), ret);
    HILOGI("NearlinkCcpClientServerStubTest:CallDetailsChanged004 end");
}

/**
 * @tc.number: CallDetailsChanged005
 * @tc.name:
 * @tc.desc: 参数区不足（只写 2 个 int32）时立即拒绝，callType 不得被静默补 0 后放过
 */
HWTEST_F(NearlinkCcpClientServerStubTest, CallDetailsChanged005, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:CallDetailsChanged005 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCcpClientStub::GetDescriptor());
    data.WriteInt32(1);
    data.WriteInt32(0);
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_CALL_DETAILS_CHANGED, data, reply);
    EXPECT_EQ(static_cast<int32_t>(ERR_INVALID_VALUE), ret);
    HILOGI("NearlinkCcpClientServerStubTest:CallDetailsChanged005 end");
}

/**
 * @tc.number: CallDetailsChanged006
 * @tc.name:
 * @tc.desc: callType 低于已知枚举下界（TYPE_CS=0）时拒绝，与 004（上界 7）构成双向覆盖
 */
HWTEST_F(NearlinkCcpClientServerStubTest, CallDetailsChanged006, TestSize.Level1)
{
    HILOGI("NearlinkCcpClientServerStubTest:CallDetailsChanged006 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkCcpClientStub::GetDescriptor());
    data.WriteInt32(1);
    data.WriteInt32(0);
    data.WriteInt32(-1);
    int32_t ret = CcpClientOnRemoteRequest(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_CALL_DETAILS_CHANGED, data, reply);
    EXPECT_EQ(static_cast<int32_t>(ERR_INVALID_VALUE), ret);
    HILOGI("NearlinkCcpClientServerStubTest:CallDetailsChanged006 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
