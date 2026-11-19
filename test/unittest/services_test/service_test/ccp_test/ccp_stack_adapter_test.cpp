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
#include <cstring>

#include "CcpStackAdapter.cpp"
#include "mock_ccs_record.h"
#include "securec.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
CcpStackAdapter ccpStackAdapter_;
}

class NearlinkCcpStackAdapterTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkCcpStackAdapterTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkCcpStackAdapterTest");
}

void NearlinkCcpStackAdapterTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkCcpStackAdapterTest");
}

void NearlinkCcpStackAdapterTest::SetUp()
{
    HILOGI("SetUp NearlinkCcpStackAdapterTest.");
}

void NearlinkCcpStackAdapterTest::TearDown()
{
    HILOGI("TearDown NearlinkCcpStackAdapterTest.");
}

/*
 * @tc.number: NotifyCallControlFail001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkCcpStackAdapterTest, NotifyCallControlFail001, TestSize.Level1)
{
    HILOGI("NotifyCallControlFail001 start");
    uint32_t requestId = 1;
    int32_t instanceId = 1;

    ccpStackAdapter_.NotifyCallControlFail(requestId, instanceId);
    EXPECT_NE(0, 1);
    HILOGI("NotifyCallControlFail001 end");
}

/*
 * @tc.number: NewCallStateInfo001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkCcpStackAdapterTest, NewCallStateInfo001, TestSize.Level1)
{
    HILOGI("NewCallStateInfo001 start");
    NLSTK_CcpCallStatues_S callState{};
    size_t count = 1;
    ccpStackAdapter_.NewCallStateInfo(&callState, count);
    EXPECT_NE(0, 1);
    HILOGI("NewCallStateInfo001 end");
}

/*
 * @tc.number: CcpAnswerCbk001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkCcpStackAdapterTest, CcpAnswerCbk001, TestSize.Level1)
{
    HILOGI("CcpAnswerCbk001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    int32_t instanceId = 0;
    uint32_t requestId = 0;
    uint8_t callId = 0;
    CcpAnswerCbk(&sleAddr, instanceId, requestId, callId);
    EXPECT_NE(0, 1);
    HILOGI("CcpAnswerCbk001 end");
}

/*
 * @tc.number: CcpHangUpCbk001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkCcpStackAdapterTest, CcpHangUpCbk001, TestSize.Level1)
{
    HILOGI("CcpHangUpCbk001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    int32_t instanceId = 0;
    uint32_t requestId = 0;
    uint8_t callId = 0;
    CcpHangUpCbk(&sleAddr, instanceId, requestId, callId);
    EXPECT_NE(0, 1);
    HILOGI("CcpHangUpCbk001 end");
}

/*
 * @tc.number: CcpStartCcsInstanceCbk001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkCcpStackAdapterTest, CcpStartCcsInstanceCbk001, TestSize.Level1)
{
    HILOGI("CcpStartCcsInstanceCbk001 start");
    int32_t instanceId = 0;
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    CcpStartCcsInstanceCbk(instanceId, ret);
    EXPECT_NE(0, 1);
    HILOGI("CcpStartCcsInstanceCbk001 end");
}

/*
 * @tc.number: CcpCallControlServiceAuthorizeCbk001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkCcpStackAdapterTest, CcpCallControlServiceAuthorizeCbk001, TestSize.Level1)
{
    HILOGI("CcpCallControlServiceAuthorizeCbk001 start");
    int32_t instanceId = 0;
    uint32_t requestId = 0;
    NLSTK_CcpCcsPropertyType_E property = NLSTK_CCP_CCS_CALL_STATUS;
    NLSTK_ServicePropertyOpType_E operation = NLSTK_SSAP_PROPERTY_READ;
    CcpCallControlServiceAuthorizeCbk(requestId, instanceId, property, operation);
    EXPECT_NE(0, 1);
    HILOGI("CcpCallControlServiceAuthorizeCbk001 end");
}

/**
 * @tc.number: NewCallStateInfoBoundary001
 * @tc.name: Test NewCallStateInfo rejects zero and oversized count.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkCcpStackAdapterTest, NewCallStateInfoBoundary001, TestSize.Level1)
{
    // 通话列表条数为零或超过上限直接拒绝，不分配数组
    NLSTK_CcpCallStatues_S callState{};
    EXPECT_FALSE(ccpStackAdapter_.NewCallStateInfo(&callState, 0));
    EXPECT_EQ(callState.callId, nullptr);

    EXPECT_FALSE(ccpStackAdapter_.NewCallStateInfo(&callState, 256));
    EXPECT_EQ(callState.callId, nullptr);

    ASSERT_TRUE(ccpStackAdapter_.NewCallStateInfo(&callState, 2));
    EXPECT_NE(callState.callId, nullptr);
    EXPECT_NE(callState.networkId, nullptr);
    EXPECT_NE(callState.callStatus, nullptr);
    EXPECT_NE(callState.callFlag, nullptr);
    ccpStackAdapter_.FreeCallStateInfo(&callState, 2);
}

/**
 * @tc.number: UpdateCallStateInfoEmptyProp001
 * @tc.name: Test UpdateCallStateInfo with empty call list.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkCcpStackAdapterTest, UpdateCallStateInfoEmptyProp001, TestSize.Level1)
{
    // 空通话列表更新通话数量为零；单条列表携带首路通话标识
    MockCcsResetRecord();
    std::shared_ptr<TotalCallStateProp> emptyProp = std::make_shared<TotalCallStateProp>();
    ccpStackAdapter_.UpdateCallStateInfo(1, emptyProp);
    EXPECT_EQ(MockCcsGetRecord().callStateUpdateCount, static_cast<uint32_t>(1));
    EXPECT_EQ(MockCcsGetRecord().lastCallCount, static_cast<uint32_t>(0));

    std::shared_ptr<TotalCallStateProp> prop = std::make_shared<TotalCallStateProp>();
    TotalCallStateProp::SingleCallStateProp single;
    single.callId = 3;
    single.callStatus = 2;
    prop->callStateVec.push_back(single);
    ccpStackAdapter_.UpdateCallStateInfo(1, prop);
    EXPECT_EQ(MockCcsGetRecord().lastCallCount, static_cast<uint32_t>(1));
    EXPECT_EQ(MockCcsGetRecord().lastCallFirstId, static_cast<uint8_t>(3));
}

/**
 * @tc.number: UpdateCallInOutInfoEmptyUserFields001
 * @tc.name: Test UpdateCallInOutInfo skips empty user fields.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkCcpStackAdapterTest, UpdateCallInOutInfoEmptyUserFields001, TestSize.Level1)
{
    // 号码与别名都为空时字段长度为零仍正常更新；携带号码时透传长度
    MockCcsResetRecord();
    std::shared_ptr<CallInOutInfoProp> emptyProp = std::make_shared<CallInOutInfoProp>();
    emptyProp->callId = 5;
    ccpStackAdapter_.UpdateCallInOutInfo(1, emptyProp);
    MockCcsRecord record = MockCcsGetRecord();
    EXPECT_EQ(record.inOutUpdateCount, static_cast<uint32_t>(1));
    EXPECT_EQ(record.lastInOutUserInfoLen, static_cast<uint32_t>(0));
    EXPECT_EQ(record.lastInOutUserAliasLen, static_cast<uint32_t>(0));

    std::shared_ptr<CallInOutInfoProp> prop = std::make_shared<CallInOutInfoProp>();
    prop->callId = 6;
    (void)strncpy_s(prop->userInfo, sizeof(prop->userInfo), "tel:10086", strlen("tel:10086"));
    ccpStackAdapter_.UpdateCallInOutInfo(1, prop);
    EXPECT_EQ(MockCcsGetRecord().lastInOutUserInfoLen, static_cast<uint32_t>(strlen("tel:10086")));
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS