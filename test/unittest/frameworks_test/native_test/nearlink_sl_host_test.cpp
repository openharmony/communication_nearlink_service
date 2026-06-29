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

#include "log.h"
#include "nearlink_access_token_mock.h"
#include "nearlink_def.h"
#include "nearlink_host.h"
#include "nearlink_sl_host.h"
#include "parameters.h"

namespace OHOS {
namespace Nearlink {

namespace VAL {
    const int ENABLE_NL_DELAY_SEC = 2;
    static uint16_t groupInitOutHdl = 0;
    static uint16_t groupJoinOutHdl = 0;
    uint8_t advHdl = 17;
    uint8_t maxSlotNum = 8;
    uint8_t maxMsgLen = 200;
    uint8_t joinMode = 0;
    uint8_t timeoutSec = 5;
    uint16_t period = 100;
    uint32_t groupId = 10001;
    uint32_t nodeGid = 2001;
    uint32_t nodeTid = 2002;
    const std::vector<uint8_t> slot{0, 3};
    std::array<uint8_t, NEARLINK_SL_ADDR_LEN> addr{0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    std::vector<uint8_t> msg{1, 2, 3, 4, 5};
}

using namespace testing;
using namespace testing::ext; // for TestSize

class SlGroupEventCallbackStub : public SlGroupEventCallback {
public:
    SlGroupEventCallbackStub() = default;
    ~SlGroupEventCallbackStub() = default;

    void OnNodeChangeEvent(uint32_t groupId, uint32_t nodeId, int32_t status, int32_t reason, uint16_t handle) override;
    void OnSendResultEvent(int32_t result, const std::vector<uint32_t> &unAckNode, uint16_t handle) override;
    void OnRecvEvent(uint32_t srcNodeId, const std::vector<uint8_t> &msg, uint16_t handle) override;
};

void SlGroupEventCallbackStub::OnNodeChangeEvent(uint32_t groupId, uint32_t nodeId,
    int32_t status, int32_t reason, uint16_t handle)
{
    (void)groupId;
    (void)nodeId;
    (void)status;
    (void)reason;
    (void)handle;
}

void SlGroupEventCallbackStub::OnSendResultEvent(int32_t result,
    const std::vector<uint32_t> &unAckNode, uint16_t handle)
{
    (void)result;
    (void)unAckNode;
    (void)handle;
}

void SlGroupEventCallbackStub::OnRecvEvent(uint32_t srcNodeId, const std::vector<uint8_t> &msg, uint16_t handle)
{
    (void)srcNodeId;
    (void)msg;
    (void)handle;
}

class NearlinkSlHostUtTest : public testing::Test {
public:
    NearlinkSlHostUtTest() = default;
    ~NearlinkSlHostUtTest() = default;

    // execute once before/after the first/last test case
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    // execute once before/after each test case
    void SetUp(void);
    void TearDown();

    // used to check if the current device supports SL
    static bool FeatureSupported(void);
    static bool featureSupported;
};

bool NearlinkSlHostUtTest::featureSupported = false;

void NearlinkSlHostUtTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase enter");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    if (!NearlinkHost::GetInstance().IsSleEnabled()) {
        HILOGI("start to enable nearlink");
        NearlinkHost::GetInstance().EnableNl();
        sleep(VAL::ENABLE_NL_DELAY_SEC);
    }
    NearlinkSlHostUtTest::featureSupported = NearlinkSlHostUtTest::FeatureSupported();
    HILOGI("SetUpTestCase end, feature supported %d", NearlinkSlHostUtTest::featureSupported);
}

void NearlinkSlHostUtTest::TearDownTestCase() {}

void NearlinkSlHostUtTest::SetUp() {}

void NearlinkSlHostUtTest::TearDown() {}

bool NearlinkSlHostUtTest::FeatureSupported()
{
    bool supportSl = OHOS::system::GetBoolParameter("const.nearlink.support_sparklink", false);
    bool enableRetail = OHOS::system::GetBoolParameter("const.dfx.enable_retail", false);
    return (supportSl && !enableRetail);
}

/*
 * @tc.name: NearlinkSlHostUtTest_CreateSlHost_001
 * @tc.desc: CreateSlHost
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostUtTest, NearlinkSlHostUtTest_CreateSlHost_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostUtTest_CreateSlHost_001 start");
    std::shared_ptr<SlHost> sl = SlHost::CreateSlHost();
    if (NearlinkSlHostUtTest::featureSupported) {
        EXPECT_NE(sl, nullptr);
    } else {
        EXPECT_EQ(sl, nullptr);
    }
    HILOGI("NearlinkSlHostUtTest_CreateSlHost_001 end");
}

/*
 * @tc.name: NearlinkSlHostUtTest_IsSlAppGroupEmpty_001
 * @tc.desc: IsSlAppGroupEmpty
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostUtTest, NearlinkSlHostUtTest_IsSlAppGroupEmpty_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostUtTest_IsSlAppGroupEmpty_001 start");
    bool ret = SlHost::IsSlAppGroupEmpty();
    EXPECT_EQ(ret, true);
    HILOGI("NearlinkSlHostUtTest_IsSlAppGroupEmpty_001 end");
}

/*
 * @tc.name: NearlinkSlHostUtTest_SetSensorHubSlGroupExist_001
 * @tc.desc: SetSensorHubSlGroupExist
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostUtTest, NearlinkSlHostUtTest_SetSensorHubSlGroupExist_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostUtTest_SetSensorHubSlGroupExist_001 start");
    NlErrCode ret = SlHost::SetSensorHubSlGroupExist(true);
    EXPECT_EQ(ret, NL_NO_ERROR);
    ret = SlHost::SetSensorHubSlGroupExist(false);
    EXPECT_EQ(ret, NL_NO_ERROR);
    HILOGI("NearlinkSlHostUtTest_SetSensorHubSlGroupExist_001 end");
}

/*
 * @tc.name: NearlinkSlHostUtTest_IsFeatureSupport_001
 * @tc.desc: IsFeatureSupport
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostUtTest, NearlinkSlHostUtTest_IsFeatureSupport_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostUtTest_IsFeatureSupport_001 start");
    std::shared_ptr<SlHost> sl = SlHost::CreateSlHost();
    if (NearlinkSlHostUtTest::featureSupported) {
        ASSERT_NE(sl, nullptr);
        (void)sl->IsFeatureSupport();
    } else {
        EXPECT_EQ(sl, nullptr);
    }
    HILOGI("NearlinkSlHostUtTest_IsFeatureSupport_001 end");
}

/*
 * @tc.name: NearlinkSlHostUtTest_GroupInitDeinit_001
 * @tc.desc: GroupInit && GroupDeinit
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostUtTest, NearlinkSlHostUtTest_GroupInitDeinit_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostUtTest_GroupInitDeinit_001 start");
    std::shared_ptr<SlHost> sl = SlHost::CreateSlHost();
    if (!NearlinkSlHostUtTest::featureSupported) {
        EXPECT_EQ(sl, nullptr);
    } else {
        ASSERT_NE(sl, nullptr);
        SlGroupInitParams param = {VAL::advHdl, VAL::maxSlotNum, VAL::maxMsgLen, VAL::joinMode, VAL::period,
            VAL::groupId};
        std::shared_ptr<SlGroupEventCallback> cb = std::make_shared<SlGroupEventCallbackStub>();
        (void)sl->GroupInit(VAL::groupInitOutHdl, param, cb);
        (void)sl->GroupDeinit(VAL::groupInitOutHdl);
        bool ret = SlHost::IsSlAppGroupEmpty();
        EXPECT_EQ(ret, true);
    }
    HILOGI("NearlinkSlHostUtTest_GroupInitDeinit_001 end");
}

/*
 * @tc.name: NearlinkSlHostUtTest_GroupAddRemoveNode_001
 * @tc.desc: GroupAddNode && GroupRemoveNode
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostUtTest, NearlinkSlHostUtTest_GroupAddRemoveNode_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostUtTest_GroupAddRemoveNode_001 start");
    std::shared_ptr<SlHost> sl = SlHost::CreateSlHost();
    if (!NearlinkSlHostUtTest::featureSupported) {
        EXPECT_EQ(sl, nullptr);
    } else {
        ASSERT_NE(sl, nullptr);
        SlGroupAddNodeParams param = {static_cast<uint8_t>(SlGroupMemberRole::SL_GROUP_MEMBER_ROLE_G),
            VAL::groupInitOutHdl, VAL::nodeGid, VAL::groupId, VAL::slot};
        NlErrCode ret = sl->GroupAddNode(param);
        EXPECT_EQ(ret, NL_ERR_INVALID_PARAM);
        ret = sl->GroupRemoveNode(VAL::groupInitOutHdl, VAL::nodeGid);
        EXPECT_EQ(ret, NL_ERR_INVALID_PARAM);
    }
    HILOGI("NearlinkSlHostUtTest_GroupAddRemoveNode_001 end");
}

/*
 * @tc.name: NearlinkSlHostUtTest_GroupJoinQuit_001
 * @tc.desc: GroupJoin && GroupQuit
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostUtTest, NearlinkSlHostUtTest_GroupJoinQuit_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostUtTest_GroupJoinQuit_001 start");
    std::shared_ptr<SlHost> sl = SlHost::CreateSlHost();
    if (!NearlinkSlHostUtTest::featureSupported) {
        EXPECT_EQ(sl, nullptr);
    } else {
        ASSERT_NE(sl, nullptr);
        SlGroupJoinParams param;
        param.SetAddress(VAL::addr);
        param.SetAddressType(0);
        param.SetGroupId(VAL::groupId);
        param.SetNodeId(VAL::nodeTid);
        param.SetTimeout(VAL::timeoutSec);
        std::shared_ptr<SlGroupEventCallback> cb = std::make_shared<SlGroupEventCallbackStub>();
        (void)sl->GroupJoin(param, cb, VAL::groupJoinOutHdl);
        (void)sl->GroupQuit(VAL::groupJoinOutHdl);
    }
    HILOGI("NearlinkSlHostUtTest_GroupJoinQuit_001 end");
}

/*
 * @tc.name: NearlinkSlHostUtTest_GroupSend_001
 * @tc.desc: GroupSend
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostUtTest, NearlinkSlHostUtTest_GroupSend_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostUtTest_GroupSend_001 start");
    std::shared_ptr<SlHost> sl = SlHost::CreateSlHost();
    if (!NearlinkSlHostUtTest::featureSupported) {
        EXPECT_EQ(sl, nullptr);
    } else {
        ASSERT_NE(sl, nullptr);
        SlGroupSendParams param;
        param.SetSysHandle(VAL::groupInitOutHdl);
        param.SetDstNodeId(VAL::nodeTid);
        param.SetReliable(false);
        param.SetTimeout(VAL::timeoutSec);
        param.SetMsg(VAL::msg);
        NlErrCode ret = sl->GroupSend(param);
        EXPECT_NE(ret, NL_NO_ERROR);
    }
    HILOGI("NearlinkSlHostUtTest_GroupSend_001 end");
}

/*
 * @tc.name: NearlinkSlHostUtTest_GroupSetSniffMode_001
 * @tc.desc: GroupSetSniffMode
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSlHostUtTest, NearlinkSlHostUtTest_GroupSetSniffMode_001, TestSize.Level1)
{
    HILOGI("NearlinkSlHostUtTest_GroupSetSniffMode_001");
    std::shared_ptr<SlHost> sl = SlHost::CreateSlHost();

    if (!NearlinkSlHostUtTest::featureSupported) {
        EXPECT_EQ(sl, nullptr);
    } else {
        ASSERT_NE(sl, nullptr);
        NlErrCode ret = sl->GroupSetSniffMode(VAL::groupInitOutHdl,
            static_cast<uint8_t>(SlGroupSniffMode::SL_SNIFF_MODE_ACTIVE));
        EXPECT_NE(ret, NL_NO_ERROR);
    }
    HILOGI("NearlinkSlHostUtTest_GroupSetSniffMode_001");
}

} // end namespace Nearlink
} // end namespace OHOS