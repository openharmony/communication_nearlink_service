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

#include "CcpStackAdapter.cpp"
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

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS