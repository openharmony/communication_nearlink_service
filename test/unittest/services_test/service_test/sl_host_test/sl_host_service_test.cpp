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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

#include "log.h"
#include "nearlink_access_token_mock.h"
#include "SleServiceManager.h"
#include "nearlink_sl_def.h"
#include "nearlink_sl_host_service.h"
#include "nearlink_sl_host_server.h"
#include "sl_host_struct.h"
#include "SleAdapter.h"
#include "SleSecurity.h"

namespace OHOS {
namespace Nearlink {

namespace {
    const int DELAY_MS = 1000;
    const std::vector<uint8_t> MSG {1, 2, 3, 4, 5};
    auto g_adapter = std::make_shared<SleAdapter>();
    SleSecurity sleSecurity_(*g_adapter);
}

using namespace testing;
using namespace testing::ext; // for TestSize

class MockISlHostServiceCallbackStub : public ISlHostServiceCallback {
public:
    MockISlHostServiceCallbackStub() = default;
    ~MockISlHostServiceCallbackStub() = default;

    void OnNodeChangeEvent(uint32_t groupId, uint32_t nodeId, int32_t status, int32_t reason, uint16_t handle) override;
    void OnSendResultEvent(int32_t result, const std::vector<uint32_t> &unAckNode, uint16_t handle) override;
    void OnRecvEvent(uint32_t srcNodeId, const std::vector<uint8_t> &msg, uint16_t handle) override;
};

void MockISlHostServiceCallbackStub::OnNodeChangeEvent(uint32_t groupId, uint32_t nodeId, int32_t status,
    int32_t reason, uint16_t handle)
{
    (void)groupId;
    (void)nodeId;
    (void)status;
    (void)reason;
    (void)handle;
}

void MockISlHostServiceCallbackStub::OnSendResultEvent(int32_t result, const std::vector<uint32_t> &unAckNode,
    uint16_t handle)
{
    (void)result;
    (void)unAckNode;
    (void)handle;
}

void MockISlHostServiceCallbackStub::OnRecvEvent(uint32_t srcNodeId, const std::vector<uint8_t> &msg, uint16_t handle)
{
    (void)srcNodeId;
    (void)msg;
    (void)handle;
}

class NearlinkSlHostServiceUtTest : public testing::Test {
public:
    NearlinkSlHostServiceUtTest() = default;
    ~NearlinkSlHostServiceUtTest() = default;

    // execute once before/after the first/last test case
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    // execute once before/after each test case
    void SetUp(void);
    void TearDown();

    std::shared_ptr<MockISlHostServiceCallbackStub> cb_ = std::make_shared<MockISlHostServiceCallbackStub>();
};

void NearlinkSlHostServiceUtTest::SetUpTestCase(void)
{
    HILOGI("NearlinkSlHostServiceUtTest: SetUpTestCase enter");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    HILOGI("NearlinkSlHostServiceUtTest: SetUpTestCase end");
}

void NearlinkSlHostServiceUtTest::TearDownTestCase(void)
{
    HILOGI("NearlinkSlHostServiceUtTest: TearDownTestCase");
}

void NearlinkSlHostServiceUtTest::SetUp(void)
{
    SlHostService::GetInstance().RegisterCallback(NearlinkSlHostServiceUtTest::cb_);
}

void NearlinkSlHostServiceUtTest::TearDown(void)
{
    SlHostService::GetInstance().DeregisterCallback();
    SlHostService::GetInstance().DeregisterStackCallback();
}

/*
 * @tc.name: NearlinkSlHostServiceUtTest_GroupQuit_001
 * @tc.desc: GroupQuit
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostServiceUtTest, NearlinkSlHostServiceUtTest_GroupQuit_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostServiceUtTest_GroupQuit_001 start");
    uint16_t handle = 0;
    NlErrCode ret = SlHostService::GetInstance().GroupQuit(handle);
    EXPECT_EQ(ret, NL_NO_ERROR);
    HILOGI("NearlinkSlHostServiceUtTest_GroupQuit_001 end");
}

/*
 * @tc.name: NearlinkSlHostServiceUtTest_GroupInit_001
 * @tc.desc: GroupInit
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostServiceUtTest, NearlinkSlHostServiceUtTest_GroupInit_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostServiceUtTest_GroupInit_001 start");
    SlHostGroupInitParams param;
    param.SetJoinMode(100);
    uint16_t handle = 0;
    NlErrCode ret = SlHostService::GetInstance().GroupInit(param, handle);
    EXPECT_EQ(ret, NL_ERR_INTERNAL_ERROR);
    HILOGI("NearlinkSlHostServiceUtTest_GroupInit_001 end");
}

/*
 * @tc.name: NearlinkSlHostServiceUtTest_GroupDeinit_001
 * @tc.desc: GroupDeinit
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostServiceUtTest, NearlinkSlHostServiceUtTest_GroupDeinit_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostServiceUtTest_GroupDeinit_001 start");
    uint16_t handle = 200;
    NlErrCode ret = SlHostService::GetInstance().GroupDeinit(handle);
    EXPECT_EQ(ret, NL_NO_ERROR);
    HILOGI("NearlinkSlHostServiceUtTest_GroupDeinit_001 end");
}

/*
 * @tc.name: NearlinkSlHostServiceUtTest_GroupAddNode_001
 * @tc.desc: GroupAddNode
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostServiceUtTest, NearlinkSlHostServiceUtTest_GroupAddNode_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostServiceUtTest_GroupAddNode_001 start");
    SlHostGroupAddNodeParams param;
    param.SetRole(100);
    param.SetSlotSeq(std::vector<uint8_t> {0});
    NlErrCode ret = SlHostService::GetInstance().GroupAddNode(param);
    EXPECT_EQ(ret, NL_ERR_INTERNAL_ERROR);
    HILOGI("NearlinkSlHostServiceUtTest_GroupAddNode_001 end");
}

/*
 * @tc.name: NearlinkSlHostServiceUtTest_GroupRemoveNode_001
 * @tc.desc: GroupRemoveNode
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostServiceUtTest, NearlinkSlHostServiceUtTest_GroupRemoveNode_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostServiceUtTest_GroupRemoveNode_001 start");
    uint16_t handle = 0;
    uint32_t nodeId = 0;
    NlErrCode ret = SlHostService::GetInstance().GroupRemoveNode(handle, nodeId);
    EXPECT_EQ(ret, NL_NO_ERROR);
    HILOGI("NearlinkSlHostServiceUtTest_GroupRemoveNode_001 end");
}

/*
 * @tc.name: NearlinkSlHostServiceUtTest_GroupSend_001
 * @tc.desc: GroupSend
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostServiceUtTest, NearlinkSlHostServiceUtTest_GroupSend_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostServiceUtTest_GroupSend_001 start");
    SlHostGroupSendParams param;
    param.SetMsg(std::vector<uint8_t>(MSG));
    NlErrCode ret = SlHostService::GetInstance().GroupSend(param);
    EXPECT_EQ(ret, NL_NO_ERROR);
    HILOGI("NearlinkSlHostServiceUtTest_GroupSend_001 end");
}

/*
 * @tc.name: NearlinkSlHostServiceUtTest_GroupSetSniffMode_001
 * @tc.desc: GroupSetSniffMode
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostServiceUtTest, NearlinkSlHostServiceUtTest_GroupSetSniffMode_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostServiceUtTest_GroupSetSniffMode_001 start");
    uint16_t handle = 0;
    uint8_t mode = 0;
    NlErrCode ret = SlHostService::GetInstance().GroupSetSniffMode(handle, mode);
    EXPECT_EQ(ret, NL_ERR_INTERNAL_ERROR);
    HILOGI("NearlinkSlHostServiceUtTest_GroupSetSniffMode_001 end");
}

/*
 * @tc.name: NearlinkSlHostServiceUtTest_SetSensorHubSlGroupExist_001
 * @tc.desc: SetSensorHubSlGroupExist default return no error
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostServiceUtTest, NearlinkSlHostServiceUtTest_SetSensorHubSlGroupExist_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostServiceUtTest_SetSensorHubSlGroupExist_001 start");
    NlErrCode ret = SlHostService::GetInstance().SetSensorHubSlGroupExist(false);
    EXPECT_EQ(ret, NL_NO_ERROR);
    EXPECT_TRUE(SlHostService::GetInstance().IsSlAppGroupEmpty());
    ret = SlHostService::GetInstance().SetSensorHubSlGroupExist(true);
    EXPECT_EQ(ret, NL_NO_ERROR);
    EXPECT_FALSE(SlHostService::GetInstance().IsSlAppGroupEmpty());
    HILOGI("NearlinkSlHostServiceUtTest_SetSensorHubSlGroupExist_001 end");
}

/*
 * @tc.name: NearlinkSlHostServiceUtTest_EventCollections_001
 * @tc.desc: event collections test
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostServiceUtTest, NearlinkSlHostServiceUtTest_EventCollections_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostServiceUtTest_OnRecvEvent_001 start");
    // onRecv
    uint32_t srcNodeId = 100;
    const uint8_t msgLen = 3;
    uint8_t msg[msgLen] = {1, 2, 3};
    uint16_t handle = 20;
    SlHostService::GetInstance().OnRecvEvent(srcNodeId, msgLen, msg, handle);
    // onSend
    int32_t result = 0;
    uint32_t unAckNode = 300;
    uint8_t unAckNodeCnt = 1;
    SlHostService::GetInstance().OnSendResultEvent(result, &unAckNode, unAckNodeCnt, handle);
    // statusChange
    uint32_t groupId = 400;
    uint32_t nodeId = 600;
    int32_t status = SL_NODE_STATUS_GROUP_DEINITED;
    int32_t reason = 0;
    SlHostService::GetInstance().OnNodeChangeEvent(groupId, nodeId, status, reason, handle);
    status = SL_NODE_STATUS_QUIT_FAIL;
    SlHostService::GetInstance().OnNodeChangeEvent(groupId, nodeId, status, reason, handle);
    NlErrCode ret = SlHostService::GetInstance().SetSensorHubSlGroupExist(false);
    EXPECT_EQ(ret, NL_NO_ERROR);
    EXPECT_TRUE(SlHostService::GetInstance().IsSlAppGroupEmpty());
    HILOGI("NearlinkSlHostServiceUtTest_OnRecvEvent_001 end");
}

/*
 * @tc.name: NearlinkSlHostServiceUtTest_GroupJoin_001
 * @tc.desc: group join test
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostServiceUtTest, NearlinkSlHostServiceUtTest_GroupJoin_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostServiceUtTest_GroupJoin_001 start");
    SlHostGroupJoinParams param;
    std::array<uint8_t, RawAddress::SLE_ADDRESS_BYTE_LEN> addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    param.SetAddress(addr);
    param.SetAddressType(0);
    param.SetGroupId(0);
    param.SetNodeId(200);
    param.SetTimeout(5);
    uint16_t outHandle = 0;
    // fail case
    param.SetNodeId(SL_CONFIG_MAX_NODE_ID_VAL + 1);
    NlErrCode ret = SlHostService::GetInstance().GroupJoin(param, outHandle);
    EXPECT_EQ(ret, NL_ERR_INTERNAL_ERROR);
    HILOGI("NearlinkSlHostServiceUtTest_GroupJoin_001 end");
}

/*
 * @tc.name: NearlinkSlHostServiceUtTest_GroupDestroyByTokenId_001
 * @tc.desc: group destroy by token id test
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostServiceUtTest, NearlinkSlHostServiceUtTest_GroupDestroyByTokenId_001, TestSize.Level1) // 从报告上看函数都未进入
{
    HILOGI("NearlinkSlHostServiceUtTest_GroupDestroyByTokenId_001 start");
    uint64_t tokenId = 0;
    SlHostService::GetInstance().GroupDestroyByTokenId(tokenId);
    EXPECT_TRUE(SlHostService::GetInstance().IsSlAppGroupEmpty());
    HILOGI("NearlinkSlHostServiceUtTest_GroupDestroyByTokenId_001 end");
}

/*
 * @tc.name: NearlinkSlHostServiceUtTest_GroupDestroyAll_001
 * @tc.desc: group destroy all test
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostServiceUtTest, NearlinkSlHostServiceUtTest_GroupDestroyAll_001, TestSize.Level1) // 从报告上看函数都未进入
{
    HILOGI("NearlinkSlHostServiceUtTest_GroupDestroyAll_001 start");
    uint64_t tokenId = 0;
    SlHostService::GetInstance().GroupDestroyAll();
    EXPECT_TRUE(SlHostService::GetInstance().IsSlAppGroupEmpty());
    HILOGI("NearlinkSlHostServiceUtTest_GroupDestroyAll_001 end");
}

} // end namespace Nearlink
} // end namespace OHOS