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
#include "PortService.h"
#include "PortDefines.h"
#include "ClassCreator.h"
#include "log.h"
#include <thread>

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
namespace {
const std::string DEVICE_ADDR = "11:22:33:44:55:66";
}

class MockPortObserver : public OHOS::Nearlink::PortObserver {
public:
    MockPortObserver() = default;
    ~MockPortObserver() override = default;

    void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) override
    {
        HILOGI("OnConnectionStateChanged process");
    }
};

namespace {
static MockPortObserver g_observer;
}

class NearlinkPortServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkPortServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    HILOGI("SetUpTestCase end");
}

void NearlinkPortServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase start");
}

void NearlinkPortServiceTest::SetUp()
{
    HILOGI("SetUp start");
}

void NearlinkPortServiceTest::TearDown()
{
    HILOGI("TearDown start");
}

/**
 * @tc.number: RegisterObserver001
 * @tc.name: Test RegisterObserver001.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkPortServiceTest, RegisterObserver001, TestSize.Level1)
{
    HILOGI("RegisterObserver001 start");
    PortService *portService = PortService::GetPortService();
    EXPECT_NE(portService, nullptr);
    portService->RegisterObserver(g_observer);
    HILOGI("RegisterObserver001 end");
}

/**
 * @tc.number: Enable001
 * @tc.name: Test Enable001.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkPortServiceTest, Enable001, TestSize.Level1)
{
    HILOGI("Enable001 start");
    PortService *portService = PortService::GetPortService();
    EXPECT_NE(portService, nullptr);
    portService->Enable();
    portService->Enable();
    HILOGI("Enable001 end");
}

/**
 * @tc.number: ConnectWithParam001
 * @tc.name: Test ConnectWithParam001.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkPortServiceTest, ConnectWithParam001, TestSize.Level1)
{
    HILOGI("ConnectWithParam001 start");
    PortService *portService = PortService::GetPortService();
    EXPECT_NE(portService, nullptr);
    RawAddress addr(DEVICE_ADDR);
    PortServiceConnParam param(static_cast<uint8_t>(SleConnFrameType::SLE_CONN_FRAME_TYPE_1));
    bool ret = portService->ConnectWithParam(addr, param);
    EXPECT_EQ(ret, PORT_SUCCESS);
    HILOGI("ConnectWithParam001 end");
}

/**
 * @tc.number: Disconnect001
 * @tc.name: Test Disconnect001.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkPortServiceTest, Disconnect001, TestSize.Level1)
{
    HILOGI("Disconnect001 start");
    PortService *portService = PortService::GetPortService();
    EXPECT_NE(portService, nullptr);
    RawAddress addr(DEVICE_ADDR);
    bool ret = portService->Disconnect(addr);
    EXPECT_EQ(ret, PORT_SUCCESS);
    HILOGI("Disconnect001 end");
}

/**
 * @tc.number: GetConnectDevices001
 * @tc.name: Test GetConnectDevices001.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkPortServiceTest, GetConnectDevices001, TestSize.Level1)
{
    HILOGI("GetConnectDevices001 start");
    PortService *portService = PortService::GetPortService();
    EXPECT_NE(portService, nullptr);
    RawAddress addr(DEVICE_ADDR);
    auto list = portService->GetConnectDevices();
    EXPECT_EQ(list.size(), 0);
    HILOGI("GetConnectDevices001 end");
}

/**
 * @tc.number: DeregisterObserver001
 * @tc.name: Test DeregisterObserver001.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkPortServiceTest, DeregisterObserver001, TestSize.Level1)
{
    HILOGI("DeregisterObserver001 start");
    PortService *portService = PortService::GetPortService();
    EXPECT_NE(portService, nullptr);
    portService->DeregisterObserver(g_observer);
    HILOGI("DeregisterObserver001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS