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
#include "nearlink_cdsm_client.h"
#include "nearlink_ASC_source.h"
#include "log.h"
#include "sle_uuid.h"
#include "log_util.h"

namespace OHOS::Nearlink {
namespace TEST {
using namespace testing::ext;
using namespace OHOS::Nearlink;

class NearlinkCdsmClientTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkCdsmClientTest::SetUpTestCase()
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

void NearlinkCdsmClientTest::TearDownTestCase()
{}

void NearlinkCdsmClientTest::SetUp()
{}

void NearlinkCdsmClientTest::TearDown()
{}

class NearlinkCdsmClientCallbackTest : public OHOS::Nearlink::NearlinkCdsmClientCallback {
public:
    void OnCdsInfoChanged(const NearlinkCdsInfo& cdsInfo) override
    {
        HILOGI("OnCdsInfoChanged");
    }
};

/**
 * @tc.number: NearlinkCdsmClientTest001
 * @tc.name: NearlinkCdsmClientTest.
 * @tc.desc: RegisterTwsClientObserver success.
 */
HWTEST_F(NearlinkCdsmClientTest, NearlinkCdsmClientTest001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmClientTest001 start");
    std::shared_ptr<NearlinkCdsmClientCallbackTest> cdsmCbkTest = std::make_shared<NearlinkCdsmClientCallbackTest>();
    auto cdsmCbk = std::dynamic_pointer_cast<OHOS::Nearlink::NearlinkCdsmClientCallback>(cdsmCbkTest);
    NearlinkRemoteDevice device("11:22:33:44:55:66", static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    auto cdsmListener = Nearlink::NearlinkCdsmClient::CreateNearlinkCdsmClient(device, cdsmCbk);
    HILOGI("NearlinkCdsmClientTest001 end");
}
}  // namespace TEST
}  // namespace OHOS::Nearlink
