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

#include "nearlink_access_token_mock.h"
#include "nearlink_def.h"
#include "nearlink_host.h"
#include "nearlink_tws_client.h"
#include "nearlink_ASC_source.h"
#include "log.h"
#include "sle_uuid.h"
#include "log_util.h"
#include "parameters.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
using namespace OHOS::Nearlink;

class NearlinkTwsClientTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkTwsClientTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    if (!NearlinkHost::GetInstance().IsSleEnabled()) {
        HILOGI("start enable nearlink");
        NearlinkHost::GetInstance().EnableNl();
        sleep(2);
    }
    HILOGI("SetUpTestCase end");
}

void NearlinkTwsClientTest::TearDownTestCase()
{}

void NearlinkTwsClientTest::SetUp()
{}

void NearlinkTwsClientTest::TearDown()
{}

class NearlinkTwsClientObserverTest final : public OHOS::Nearlink::NearlinkTwsClientObserver {
public:
    NearlinkTwsClientObserverTest(){};
    ~NearlinkTwsClientObserverTest(){};

private:
    void OnTwsRemoteInfo(const std::string &address, const std::vector<uint8_t> &value) override
    {
        HILOGI("OnTwsRemoteInfo %{public}s", GetEncryptAddr(address).c_str());
    }
};

/**
 * @tc.number: NearlinkTwsClientTest001
 * @tc.name: NearlinkTwsClientTest.
 * @tc.desc: RegisterTwsClientObserver success.
 */
HWTEST_F(NearlinkTwsClientTest, NearlinkTwsClientTest001, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientTest001 start");
    bool isAudioSupported = OHOS::system::GetIntParameter("const.nearlink.audio", 0) != 0;

    std::shared_ptr<NearlinkTwsClientObserver> hiechoObserver_ = std::make_shared<NearlinkTwsClientObserver>();
    NlErrCode ret = NearlinkTwsClient::GetInstance().RegisterTwsClientObserver(hiechoObserver_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    std::string address("11:22:33:44:55:66");
    ret = NearlinkTwsClient::GetInstance().EnableWearDetection(address);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }


    int32_t state = -1;
    ret = NearlinkTwsClient::GetInstance().GetWearDetectionState(address, state);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }

    bool isWearing = false;
    ret = NearlinkTwsClient::GetInstance().IsDeviceWearing(address, isWearing);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }

    bool isSupported = false;
    ret = NearlinkTwsClient::GetInstance().IsWearDetectionSupported(address, isSupported);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }

    int32_t roleInfo = 0;
    ret = NearlinkTwsClient::GetInstance().GetTwsRoleInfo(address, roleInfo);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }

    uint32_t delayValue = 0;
    ret = NearlinkTwsClient::GetInstance().GetTwsAudioDelay(address, delayValue);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }

    bool isVirtualAutoConnectSupported = false;
    ret = NearlinkTwsClient::GetInstance().IsSupportVirtualAutoConnect(address, isVirtualAutoConnectSupported);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }

    ret = NearlinkTwsClient::GetInstance().SetVirtualAutoConnectType(address, 0, 0);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }

    std::vector<struct AudioStreamInfo> streamData;
    ret = NearlinkTwsClient::GetInstance().SendUserSelection(address, streamData);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }

    std::vector<struct AudioStreamInfo> queryStreamData;
    ret = NearlinkTwsClient::GetInstance().QueryStreamState(address, queryStreamData);
    if (isAudioSupported) {
        EXPECT_EQ(NL_ERR_INVALID_PARAM, ret); // 没查询到，返回401
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }

    ret = NearlinkTwsClient::GetInstance().DisableWearDetection(address);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }

    ret = NearlinkTwsClient::GetInstance().DeregisterTwsClientObserver(hiechoObserver_);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("NearlinkTwsClientTest001 end");
}
}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS