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

#include "gtest/gtest.h"
#include "cm_errno.h"
#include "cm_signaling_version.h"
#include "sle_logic_link_mgr.h"

#define UT_COMPANY_ID_HUAWEI  0x0009

class UT_SIGNALING_VERSION : public testing::Test {
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {}

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {}

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {
        SleLogicLinkInit();
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
        SleLogicLinkDeInit();
    }
};

TEST_F(UT_SIGNALING_VERSION, CM_SetLinkProtocolVersion)
{
    SLE_Addr_S addr = {0};
    SleLogicLink_S *link = SleLogicLinkAdd(&addr);
    EXPECT_NE(&addr, nullptr);
    EXPECT_NE(link, nullptr);
    CM_SetLinkProtocolVersion(CM_INVALID_LCID, 0x00);
}

TEST_F(UT_SIGNALING_VERSION, CM_SetLinkTransMode)
{
    SLE_Addr_S addr = {0};
    EXPECT_NE(&addr, nullptr);
    SleLogicLink_S *link = SleLogicLinkAdd(&addr);
    CM_TransMode mode = {0};
    EXPECT_NE(link, nullptr);
    CM_SetLinkTransMode(CM_INVALID_LCID, mode);
}

TEST_F(UT_SIGNALING_VERSION, CM_SetLinkMtu)
{
    SLE_Addr_S addr = {0};
    EXPECT_NE(&addr, nullptr);
    SleLogicLink_S *link = SleLogicLinkAdd(&addr);
    EXPECT_NE(link, nullptr);
    CM_SetLinkMtu(CM_INVALID_LCID, 0x00);
}

TEST_F(UT_SIGNALING_VERSION, CM_SetLinkExchangeVersion)
{
    SLE_Addr_S addr = {0};
    EXPECT_NE(&addr, nullptr);
    SleLogicLink_S *link = SleLogicLinkAdd(&addr);
    EXPECT_NE(link, nullptr);
    CM_SetLinkExchangeVersion(CM_INVALID_LCID, 0x00);
}

TEST_F(UT_SIGNALING_VERSION, CM_SetLinkRxWindow)
{
    SLE_Addr_S addr = {0};
    EXPECT_NE(&addr, nullptr);
    SleLogicLink_S *link = SleLogicLinkAdd(&addr);
    EXPECT_NE(link, nullptr);
    CM_SetLinkRxWindow(CM_INVALID_LCID, 0x00);
}

TEST_F(UT_SIGNALING_VERSION, CM_GetDeviceLinkDeviceType)
{
    SLE_Addr_S addr = {0};
    EXPECT_NE(&addr, nullptr);
    SleLogicLink_S *link = SleLogicLinkAdd(&addr);
    EXPECT_NE(link, nullptr);
    EXPECT_EQ(CM_GetDeviceLinkDeviceType(CM_INVALID_LCID), CM_DEVTYPE_THIRD_PARTY);
    EXPECT_EQ(CM_GetDeviceLinkDeviceType(0), CM_DEVTYPE_UNKNOWN);
}

TEST_F(UT_SIGNALING_VERSION, CM_GetDeviceLinkDeviceType_01)
{
    SLE_Addr_S addr = {0};
    EXPECT_NE(&addr, nullptr);
    SleLogicLink_S *link = SleLogicLinkAdd(&addr);
    EXPECT_NE(link, nullptr);
    link->lcid = 0x43;
    link->devType = CM_DEVTYPE_UNKNOWN;
    link->companyId = UT_COMPANY_ID_HUAWEI;
    link->protocolVersion = CM_INVALID_VERSION;
    EXPECT_EQ(CM_GetDeviceLinkDeviceType(link->lcid), CM_DEVTYPE_OLD);
}

TEST_F(UT_SIGNALING_VERSION, CM_SetDeviceLinkDeviceType)
{
    SLE_Addr_S addr = {0};
    EXPECT_NE(&addr, nullptr);
    SleLogicLink_S *link = SleLogicLinkAdd(&addr);
    EXPECT_NE(link, nullptr);
    CM_SetDeviceLinkDeviceType(CM_INVALID_LCID, true);
}

TEST_F(UT_SIGNALING_VERSION, CM_SetDeviceLinkDeviceType_01)
{
    SLE_Addr_S addr = {0};
    EXPECT_NE(&addr, nullptr);
    SleLogicLink_S *link = SleLogicLinkAdd(&addr);
    EXPECT_NE(link, nullptr);
    link->lcid = 0x43;
    link->devType = CM_DEVTYPE_UNKNOWN;
    link->companyId = UT_COMPANY_ID_HUAWEI;
    link->protocolVersion = CM_INVALID_VERSION;
    CM_SetDeviceLinkDeviceType(link->lcid, false);
}