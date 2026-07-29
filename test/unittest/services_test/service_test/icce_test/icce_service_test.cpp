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
#include "nearlink_access_token_mock.h"
#include "IcceDefines.h"
#include "IcceService.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "icce_utils.h"

#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class IcceClientCallbackCommon : public IcceObserver {
public:
    IcceClientCallbackCommon() = default;
    virtual ~IcceClientCallbackCommon() = default;

    void OnConnectionStateChanged(const RawAddress &device, int curState, int prevState){}
};

namespace {
constexpr int ICCE_SERVICE_UT_DELAY_50_MS = 50;
constexpr int ICCE_SERVICE_UT_DELAY_1000_MS = 1000;
IcceClientCallbackCommon g_icceClientCallback_;
}

class NearlinkIcceServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkIcceServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkIcceServiceTest start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::ICCE_SERVICE_UT_DELAY_1000_MS));
    HILOGI("SetUpTestCase NearlinkIcceServiceTest end");
}

void NearlinkIcceServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkIcceServiceTest");
}

void NearlinkIcceServiceTest::SetUp()
{
    HILOGI("SetUp NearlinkIcceServiceTest.");
}

void NearlinkIcceServiceTest::TearDown()
{
    HILOGI("TearDown NearlinkIcceServiceTest.");
}

/**
 * @tc.number: GetService001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkIcceServiceTest, GetService001, TestSize.Level1)
{
    HILOGI("GetService001 start");
    IcceService* icceService = IcceService::GetService();
    EXPECT_NE(nullptr, icceService);
    HILOGI("GetService001 end");
}

/**
 * @tc.number: Disable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkIcceServiceTest, Disable001, TestSize.Level1)
{
    HILOGI("Disable001 start");
    IcceService* icceService = IcceService::GetService();
    icceService->Disable();
    EXPECT_NE(nullptr, icceService);

    icceService->Disable();
    EXPECT_NE(nullptr, icceService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::ICCE_SERVICE_UT_DELAY_50_MS));
    HILOGI("Disable001 end");
}

/**
 * @tc.number: Enable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkIcceServiceTest, Enable001, TestSize.Level1)
{
    HILOGI("Enable001 start");
    IcceService* icceService = IcceService::GetService();
    icceService->Enable();
    EXPECT_NE(nullptr, icceService);

    icceService->Enable();
    EXPECT_NE(nullptr, icceService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::ICCE_SERVICE_UT_DELAY_50_MS));
    HILOGI("Enable001 end");
}

/*
 * @tc.number: RegisterObserver001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkIcceServiceTest, RegisterObserver001, TestSize.Level1)
{
    HILOGI("RegisterObserver001 start");
    IcceService* icceService = IcceService::GetService();
    icceService->RegisterObserver(g_icceClientCallback_);
    EXPECT_NE(nullptr, icceService);
    HILOGI("RegisterObserver001 end");
}

/*
 * @tc.number: DeregisterObserver001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkIcceServiceTest, DeregisterObserver001, TestSize.Level1)
{
    HILOGI("DeregisterObserver001 start");
    IcceService* icceService = IcceService::GetService();
    icceService->DeregisterObserver(g_icceClientCallback_);
    EXPECT_NE(nullptr, icceService);
    HILOGI("DeregisterObserver001 end");
}

/**
 * @tc.number: Connect001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkIcceServiceTest, Connect001, TestSize.Level1)
{
    HILOGI("Connect001 start");
    IcceService* icceService = IcceService::GetService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = icceService->Connect(*device);
    EXPECT_EQ(ICCE_SUCCESS, ret);
    HILOGI("Connect001 end");
}

/**
 * @tc.number: Disconnect001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkIcceServiceTest, Disconnect001, TestSize.Level1)
{
    HILOGI("Disconnect001 start");
    IcceService* icceService = IcceService::GetService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = icceService->Disconnect(*device);
    EXPECT_EQ(ICCE_SUCCESS, ret);
    HILOGI("Disconnect001 end");
}

/*
 * @tc.number: GetPort001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkIcceServiceTest, GetPort001, TestSize.Level1)
{
    HILOGI("GetPort001 start");
    IcceService* icceService = IcceService::GetService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = icceService->GetPort(*device);
    EXPECT_NE(nullptr, icceService);
    HILOGI("GetPort001 end");
}

/*
 * @tc.number: GetConnectionsDeviceNum001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkIcceServiceTest, GetConnectionsDeviceNum001, TestSize.Level1)
{
    HILOGI("GetConnectionsDeviceNum001 start");
    IcceService* icceService = IcceService::GetService();
    uint8_t ret = icceService->GetConnectionsDeviceNum();
    EXPECT_NE(nullptr, icceService);
    HILOGI("GetConnectionsDeviceNum001 end");
}

/*
 * @tc.number: ConvertToStackAddr_001
 * @tc.name: ConvertToStackAddr with valid address
 * @tc.desc: Verify ConvertToStackAddr uses IRemoteDeviceQuery to get addr type and returns correct SLE_Addr_S
 */
HWTEST_F(NearlinkIcceServiceTest, ConvertToStackAddr_001, TestSize.Level1)
{
    HILOGI("ConvertToStackAddr_001 start");
    std::string addr = "00:11:22:33:44:55";
    RawAddress device(addr);
    SLE_Addr_S sleAddr = ConvertToStackAddr(device);
    EXPECT_EQ(0, sleAddr.type);
    EXPECT_EQ(0x00, sleAddr.addr[0]);
    EXPECT_EQ(0x11, sleAddr.addr[1]);
    EXPECT_EQ(0x22, sleAddr.addr[2]);
    EXPECT_EQ(0x33, sleAddr.addr[3]);
    EXPECT_EQ(0x44, sleAddr.addr[4]);
    EXPECT_EQ(0x55, sleAddr.addr[5]);
    HILOGI("ConvertToStackAddr_001 end");
}

/*
 * @tc.number: ConvertSleAddrToRawAddress_001
 * @tc.name: ConvertSleAddrToRawAddress with valid SLE_Addr_S
 * @tc.desc: Verify ConvertSleAddrToRawAddress returns correct RawAddress
 */
HWTEST_F(NearlinkIcceServiceTest, ConvertSleAddrToRawAddress_001, TestSize.Level1)
{
    HILOGI("ConvertSleAddrToRawAddress_001 start");
    SLE_Addr_S sleAddr = {0};
    sleAddr.addr[0] = 0xAA;
    sleAddr.addr[1] = 0xBB;
    sleAddr.addr[2] = 0xCC;
    sleAddr.addr[3] = 0xDD;
    sleAddr.addr[4] = 0xEE;
    sleAddr.addr[5] = 0xFF;
    RawAddress rawAddr = ConvertSleAddrToRawAddress(&sleAddr);
    EXPECT_EQ(std::string("AA:BB:CC:DD:EE:FF"), rawAddr.GetAddress());
    HILOGI("ConvertSleAddrToRawAddress_001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS