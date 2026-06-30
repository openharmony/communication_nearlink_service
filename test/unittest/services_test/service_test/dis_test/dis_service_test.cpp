/*
 * Copyright (C) 2026 Huawei Device Co., Ltd. All rights reserved.
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
#include "nearlink_access_token_mock.h"
#include "DisClientStackAdapter.h"
#include "DisService.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

// class IcceClientCallbackCommon : public IcceObserver {
// public:
//     IcceClientCallbackCommon() = default;
//     virtual ~IcceClientCallbackCommon() = default;

//     void OnConnectionStateChanged(const RawAddress &device, int curState, int prevState){}
// };

namespace {
constexpr int DIS_SERVICE_UT_DELAY_50_MS = 50;
constexpr int DIS_SERVICE_UT_DELAY_1000_MS = 1000;
// IcceClientCallbackCommon g_icceClientCallback_;
}

class NearlinkDisServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkDisServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkDisServiceTest start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::DIS_SERVICE_UT_DELAY_1000_MS));
    HILOGI("SetUpTestCase NearlinkDisServiceTest end");
}

void NearlinkDisServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkDisServiceTest");
}

void NearlinkDisServiceTest::SetUp()
{
    HILOGI("SetUp NearlinkDisServiceTest.");
}

void NearlinkDisServiceTest::TearDown()
{
    HILOGI("TearDown NearlinkDisServiceTest.");
}

/**
 * @tc.number: GetDisService001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkDisServiceTest, GetDisService001, TestSize.Level1)
{
    HILOGI("GetDisService001 start");
    DisService* disService = DisService::GetDisService();
    EXPECT_NE(nullptr, disService);
    HILOGI("GetDisService001 end");
}

/**
 * @tc.number: Disable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkDisServiceTest, Disable001, TestSize.Level1)
{
    HILOGI("Disable001 start");
    DisService* disService = DisService::GetDisService();
    disService->Disable();
    EXPECT_NE(nullptr, disService);

    disService->Disable();
    EXPECT_NE(nullptr, disService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::DIS_SERVICE_UT_DELAY_50_MS));
    HILOGI("Disable001 end");
}

/**
 * @tc.number: Enable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkDisServiceTest, Enable001, TestSize.Level1)
{
    HILOGI("Enable001 start");
    DisService* disService = DisService::GetDisService();
    disService->Enable();
    EXPECT_NE(nullptr, disService);

    disService->Enable();
    EXPECT_NE(nullptr, disService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::DIS_SERVICE_UT_DELAY_50_MS));
    HILOGI("Enable001 end");
}

/**
 * @tc.number: Connect001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkDisServiceTest, Connect001, TestSize.Level1)
{
    HILOGI("Connect001 start");
    DisService* disService = DisService::GetDisService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = disService->Connect(*device);
    EXPECT_EQ(DIS_SUCCESS, ret);
    HILOGI("Connect001 end");
}

/**
 * @tc.number: Disconnect001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkDisServiceTest, Disconnect001, TestSize.Level1)
{
    HILOGI("Disconnect001 start");
    DisService* disService = DisService::GetDisService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = disService->Disconnect(*device);
    EXPECT_EQ(DIS_SUCCESS, ret);
    HILOGI("Disconnect001 end");
}

/*
 * @tc.number: NotifyStateChanged001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkDisServiceTest, NotifyStateChanged001, TestSize.Level1)
{
    HILOGI("GetPortNotifyStateChanged001001 start");
    DisService* disService = DisService::GetDisService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    disService->NotifyStateChanged(*device, SleConnectState::CONNECTED, SleConnectState::DISCONNECTED);
    EXPECT_NE(nullptr, disService);
    HILOGI("GetPorNotifyStateChanged001t001 end");
}

/*
 * @tc.number: GetConnectDevices001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkDisServiceTest, GetConnectDevices001, TestSize.Level1)
{
    HILOGI("GetConnectDevices001 start");
    DisService* disService = DisService::GetDisService();
    std::list<RawAddress> devices = disService->GetConnectDevices();
    EXPECT_EQ(0, devices.size());
    HILOGI("GetConnectDevices001 end");
}

/*
 * @tc.number: GetConnectState001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkDisServiceTest, GetConnectState001, TestSize.Level1)
{
    HILOGI("GetConnectState001 start");
    DisService* disService = DisService::GetDisService();
    int ret = disService->GetConnectState();
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), ret);
    HILOGI("GetConnectState001 end");
}

/*
 * @tc.number: GetDeviceVendorId001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkDisServiceTest, GetDeviceVendorId001, TestSize.Level1)
{
    HILOGI("GetDeviceVendorId001 start");
    DisService* disService = DisService::GetDisService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = disService->GetDeviceVendorId(*device);
    EXPECT_NE(nullptr, disService);
    HILOGI("GetDeviceVendorId001 end");
}

/*
 * @tc.number: GetDeviceVersion001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkDisServiceTest, GetDeviceVersion001, TestSize.Level1)
{
    HILOGI("GetDeviceVersion001 start");
    DisService* disService = DisService::GetDisService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = disService->GetDeviceVersion(*device);
    EXPECT_NE(nullptr, disService);
    HILOGI("GetDeviceVersion001 end");
}

/*
 * @tc.number: GetDisServiceData001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkDisServiceTest, GetDisServiceData001, TestSize.Level1)
{
    HILOGI("GetDisServiceData001 start");
    DisService* disService = DisService::GetDisService();
    std::string serviceData;
    disService->GetDisServiceData(serviceData);
    EXPECT_NE(nullptr, disService);
    HILOGI("GetDisServiceData001 end");
}

/*
 * @tc.number: GetDisServiceAppearance001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkDisServiceTest, GetDisServiceAppearance001, TestSize.Level1)
{
    HILOGI("GetDisServiceAppearance001 start");
    DisService* disService = DisService::GetDisService();
    int ret = disService->GetDisServiceAppearance();
    EXPECT_NE(nullptr, disService);
    HILOGI("GetDisServiceAppearance001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS