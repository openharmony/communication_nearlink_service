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
#include "nearlink_permission_manager.h"
#include "nearlink_access_token_mock.h"

namespace OHOS {
namespace Nearlink {
using namespace testing;
using namespace testing::ext;

class NearlinkPermissionManagerTest : public testing::Test {
public:
    NearlinkPermissionManagerTest() = default;
    ~NearlinkPermissionManagerTest() = default;

    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkPermissionManagerTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkPermissionManagerTest.");
    OHOS::Nearlink::NearlinkAccessTokenMock::SetNativeTokenInfo();
}

void NearlinkPermissionManagerTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkPermissionManagerTest");
}

void NearlinkPermissionManagerTest::SetUp()
{
    HILOGI("SetUp NearlinkPermissionManagerTest.");
}

void NearlinkPermissionManagerTest::TearDown()
{
    HILOGI("TearDown NearlinkPermissionManagerTest.");
}

/**
 * @tc.name: permission_verifypermission_001
 * @tc.desc: verify VerifyPermission
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkPermissionManagerTest, permission_verifypermission_001, TestSize.Level1)
{
    HILOGI("permission_verifypermission_001 enter");
    EXPECT_TRUE(NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK));
    EXPECT_TRUE(NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK));
    EXPECT_TRUE(NearLinkPermissionManager::VerifyPermission(GET_NEARLINK_LOCAL_MAC));
    EXPECT_TRUE(NearLinkPermissionManager::VerifyPermission(GET_NEARLINK_PEER_MAC));
    HILOGI("permission_verifypermission_001 end");
}

/**
 * @tc.name: permission_verifypermission_002
 * @tc.desc: verify VerifyPermission with tokenId
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkPermissionManagerTest, permission_verifypermission_002, TestSize.Level1)
{
    HILOGI("permission_verifypermission_002 enter");
    uint32_t tokenId = 0;
    EXPECT_FALSE(NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, tokenId));
    EXPECT_FALSE(NearLinkPermissionManager::VerifyPermission(MANAGE_NEARLINK, tokenId));
    EXPECT_FALSE(NearLinkPermissionManager::VerifyPermission(GET_NEARLINK_LOCAL_MAC, tokenId));
    EXPECT_FALSE(NearLinkPermissionManager::VerifyPermission(GET_NEARLINK_PEER_MAC, tokenId));
    HILOGI("permission_verifypermission_002 end");
}

/**
 * @tc.name: permission_verifymultipermissionsd_001
 * @tc.desc: verify VerifyMultiPermissions
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkPermissionManagerTest, permission_verifymultipermissionsd_001, TestSize.Level1)
{
    HILOGI("permission_verifymultipermissionsd_001 enter");
    auto item = NearLinkPermissionManager::CreateItem(true, {ACCESS_NEARLINK});
    EXPECT_EQ(0, NearLinkPermissionManager::VerifyMultiPermissions(item));
    HILOGI("permission_verifymultipermissionsd_001 end");
}

/**
 * @tc.name: permission_getapiversion_001
 * @tc.desc: verify GetApiVersion
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkPermissionManagerTest, permission_getapiversion_001, TestSize.Level1)
{
    HILOGI("permission_getapiversion_001 enter");
    EXPECT_EQ(API_VERSION_INVALID, NearLinkPermissionManager::GetNearlinkApiVersion());
    uint32_t tokenId = 0;
    EXPECT_EQ(API_VERSION_INVALID, NearLinkPermissionManager::GetNearlinkApiVersion(tokenId));
    HILOGI("permission_getapiversion_001 end");
}

/**
 * @tc.name: permission_getcallingName_001
 * @tc.desc: verify GetCallingName
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkPermissionManagerTest, permission_getcallingName_001, TestSize.Level1)
{
    HILOGI("permission_getcallingName_001 enter");
    EXPECT_STRNE("", NearLinkPermissionManager::GetCallingName().c_str());
    uint32_t tokenId = 0;
    EXPECT_STREQ("", NearLinkPermissionManager::GetCallingName(tokenId).c_str());
    HILOGI("permission_getcallingName_001 end");
}

/**
 * @tc.name: permission_ishapcaller_001
 * @tc.desc: verify IsHapCaller
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkPermissionManagerTest, permission_ishapcaller_001, TestSize.Level1)
{
    HILOGI("permission_ishapcaller_001 enter");
    EXPECT_FALSE(NearLinkPermissionManager::IsHapCaller());
    uint32_t tokenId = 0;
    EXPECT_FALSE(NearLinkPermissionManager::IsHapCaller(tokenId));
    HILOGI("permission_ishapcaller_001 end");
}

/**
 * @tc.name: permission_isnativecaller_001
 * @tc.desc: verify IsNativeCaller
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkPermissionManagerTest, permission_isnativecaller_001, TestSize.Level1)
{
    HILOGI("permission_isnativecaller_001 enter");
    EXPECT_TRUE(NearLinkPermissionManager::IsNativeCaller());
    HILOGI("permission_isnativecaller_001 end");
}

/**
 * @tc.name: permission_issystemhap_001
 * @tc.desc: verify IsSystemHap
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkPermissionManagerTest, permission_issystemhap_001, TestSize.Level1)
{
    HILOGI("permission_issystemhap_001 enter");
    EXPECT_FALSE(NearLinkPermissionManager::IsSystemHap());
    HILOGI("permission_issystemhap_001 end");
}

/**
 * @tc.name: permission_checksystempermission_001
 * @tc.desc: verify SystemPermission
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkPermissionManagerTest, permission_checksystempermission_001, TestSize.Level1)
{
    HILOGI("permission_checksystempermission_001 enter");
    EXPECT_TRUE(NearLinkPermissionManager::CheckSystemPermission());
    HILOGI("permission_checksystempermission_001 end");
}

/**
 * @tc.name: permission_isuserealAddr_001
 * @tc.desc: verify IsUseRealAddr
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkPermissionManagerTest, permission_isuserealAddr_001, TestSize.Level1)
{
    HILOGI("permission_isuserealAddr_001 enter");
    EXPECT_TRUE(NearLinkPermissionManager::IsUseRealAddr());
    HILOGI("permission_isuserealAddr_001 end");
}
}  // namespace Nearlink
}  // namespace OHOS