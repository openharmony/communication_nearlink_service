/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "log.h"
#include "icce_utils.h"
#include "hid_client_api.h"
#include "HidHostService.h"
#include "HidHostDefines.h"
#include "hid_ssap.h"
#include "hid_common.h"
#include "hid_stm.h"
#include "sdf_addr.h"
#include "ssap_utils.h"
#include "HidHostStackAdapter.h"
#include "SleServiceManager.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
// 37BEA880-FC70-11EA-B720-000000000B06
static NLSTK_SsapUuid_S HID_UUID = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20,
                                  0x00, 0x00, 0x00, 0x00, 0x0B, 0x06};
constexpr int DELAY_LITTLE_MS = 100;

class HidHostServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HidHostServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

void HidHostServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase start");
    SleInterfaceManager::GetInstance()->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

}

void HidHostServiceTest::SetUp()
{
    HILOGI("SetUp start");

}

void HidHostServiceTest::TearDown()
{
    HILOGI("TearDown start");
}

/**
 * @tc.number: HidHostServiceTest001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(HidHostServiceTest, HidHostServiceTest001, TestSize.Level1)
{
    HILOGI("HidHostServiceTest001 start");
    std::string addr = "00:11:22:33:44:55";
    RawAddress rawAddress(addr);
    HidHostService *hidHostService = HidHostService::GetService();
    ASSERT_NE(hidHostService, nullptr);
    hidHostService->GetContext();
    hidHostService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    int ret = hidHostService->Connect(rawAddress);
    EXPECT_EQ(ret, HID_HOST_SUCCESS);
    hidHostService->GetConnectDevices();
    hidHostService->GetConnectState();
    std::vector<int> states;
    states.push_back(1);
    hidHostService->GetDevicesByStates(states);
    ret = hidHostService->GetDeviceState(rawAddress);
    bool isValid = ret >= 0 && ret <= static_cast<int>(SleConnectState::DISCONNECTED);
    EXPECT_EQ(isValid, true);
    ret = hidHostService->Disconnect(rawAddress);
    EXPECT_EQ(ret, HID_HOST_SUCCESS);
    hidHostService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("HidHostServiceTest001 end");
}


/**
 * @tc.number: HidHostServiceTest002
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(HidHostServiceTest, HidHostServiceTest002, TestSize.Level1)
{
    HILOGI("HidHostServiceTest002 start");
    std::string addr = "00:11:22:33:44:55";
    RawAddress rawAddress(addr);
    HidHostService *hidHostService = HidHostService::GetService();
    ASSERT_NE(hidHostService, nullptr);
    // disable connect
    int ret = hidHostService->Connect(rawAddress);
    EXPECT_EQ(ret, HID_HOST_SUCCESS);
    hidHostService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    // same connect
    hidHostService->Connect(rawAddress);
    hidHostService->Connect(rawAddress);
    // max connected size
    RawAddress rawAddress1("11:11:22:33:44:55");
    hidHostService->Connect(rawAddress1);
    RawAddress rawAddress2("22:11:22:33:44:55");
    hidHostService->Connect(rawAddress2);
    RawAddress rawAddress3("33:11:22:33:44:55");
    hidHostService->Connect(rawAddress3);
    RawAddress rawAddress4("44:11:22:33:44:55");
    hidHostService->Connect(rawAddress4);
    RawAddress rawAddress5("55:11:22:33:44:55");
    hidHostService->Connect(rawAddress5);
    RawAddress rawAddress6("66:11:22:33:44:55");
    hidHostService->Connect(rawAddress6);
    ret = hidHostService->HidHostVCUnplug(addr, 0, 0, 0);
    hidHostService->GetHidDeviceInfo(rawAddress, 0);
    hidHostService->GetHidDeviceInfo(rawAddress, 1);
    hidHostService->GetHidDeviceInfo(rawAddress, 2);
    hidHostService->GetHidDeviceInfo(rawAddress, 3);
    EXPECT_EQ(ret, HID_HOST_SUCCESS);
    HidHostObserver hidHostCallback_;
    hidHostService->RegisterObserver(hidHostCallback_);
    hidHostService->NotifyStateChanged(rawAddress, 1, 0, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    hidHostService->NotifyStateChanged(rawAddress, 2, 1, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    hidHostService->NotifyStateChanged(rawAddress, 3, 2, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    hidHostService->DeregisterObserver(hidHostCallback_);
    hidHostService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    hidHostService->ShutDownDone(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("HidHostServiceTest002 end");
}

/**
 * @tc.number: HidHostServiceTest003
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(HidHostServiceTest, HidHostServiceTest003, TestSize.Level1)
{
    HILOGI("HidHostServiceTest003 start");
    std::string addr = "00:11:22:33:44:55";
    RawAddress rawAddress(addr);
    HidHostService *hidHostService = HidHostService::GetService();
    ASSERT_NE(hidHostService, nullptr);
    hidHostService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    int ret = hidHostService->Connect(rawAddress);
    EXPECT_EQ(ret, HID_HOST_SUCCESS);
    HidHostStackAdapter adapter;
    SLE_Addr_S stackAddr = ConvertToSleAddr(rawAddress);
    HidHostStackAdapter::HidConnectStateChangeCbk(&stackAddr, HID_CONNECTING, HID_DISCONNECTED, NLSTK_ERRCODE_SUCCESS);
    HidHostStackAdapter::HidConnectStateChangeCbk(&stackAddr, HID_CONNECTED, HID_CONNECTING, NLSTK_ERRCODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    std::string str = "0123456789";
    std::string str1 = "0123456789GA";
    std::string str2 = "0123456789AG";
    std::vector<uint8_t> data = {0x00, 0x01};
    HidReportInfo_S value = {
            .reportIdAndType = {.reportId = 0, .reportType =1},
            .reportInfoValue = {.len=0}
    };
    HidReportInfo reportInfo = {};
    reportInfo.dev_ = rawAddress;
    reportInfo.reportType_ = HID_HOST_INPUT_REPORT;
    reportInfo.reportId_ = 0;
    hidHostService->SendData(reportInfo);
    reportInfo.dataLength_ = data.size();
    reportInfo.reportId_ = 1;
    reportInfo.data_ = std::make_unique<uint8_t[]>(data.size());
    memcpy_s(reportInfo.data_.get(), data.size(), data.data(), data.size());
    hidHostService->SendData(reportInfo);
    ret = hidHostService->HidHostSendReport(addr, HID_HOST_INPUT_REPORT, str.length(), str);
    EXPECT_EQ(ret, HID_HOST_SUCCESS);
    ret = hidHostService->HidHostSendReport(addr, HID_HOST_INPUT_REPORT, str.length(), str1);
    EXPECT_EQ(ret, HID_HOST_FAILURE);
    ret = hidHostService->HidHostSendReport(addr, HID_HOST_INPUT_REPORT, str.length(), str2);
    EXPECT_EQ(ret, HID_HOST_FAILURE);
    ret = hidHostService->HidHostSendReport(addr, HID_HOST_OUTPUT_REPORT, str.length(), str);
    EXPECT_EQ(ret, HID_HOST_SUCCESS);
    ret = hidHostService->HidHostSendReport(addr, HID_HOST_FEATURE_REPORT, str.length(), str);
    EXPECT_EQ(ret, HID_HOST_SUCCESS);
    ret = hidHostService->HidHostSetReport(addr, HID_HOST_OUTPUT_REPORT, 1, data.data());
    EXPECT_EQ(ret, HID_HOST_SUCCESS);
    hidHostService->HidHostSetReport(addr, HID_HOST_INPUT_REPORT, sizeof(data), data.data());
    hidHostService->HidHostGetReport(addr, 1, 1, HID_HOST_INPUT_REPORT);
    adapter.SendGetReport(reportInfo);
    HidHostStackAdapter::HidReadPropertyCbk(&stackAddr, HID_INPUT_REPORT_INFO, &value, NLSTK_ERRCODE_SUCCESS);
    value.reportInfoValue.data = data.data();
    value.reportInfoValue.len = data.size();
    HidHostStackAdapter::HidReadPropertyCbk(&stackAddr, HID_INPUT_REPORT_INFO, &value, NLSTK_ERRCODE_SUCCESS);
    HidHostStackAdapter::HidReadPropertyCbk(&stackAddr, HID_OUTPUT_REPORT_INFO, nullptr, NLSTK_ERRCODE_FAIL);
    HidHostStackAdapter::HidReadPropertyCbk(&stackAddr, HID_TYPE_AND_FORMAT_DESC, nullptr, NLSTK_ERRCODE_SUCCESS);
    HidHostStackAdapter::HidWritePropertyCbk(&stackAddr, HID_INPUT_REPORT_INFO, NLSTK_ERRCODE_SUCCESS);
    HidHostStackAdapter::HidNotifyPropertyCbk(&stackAddr, HID_INPUT_REPORT_INFO, &value);
    HidHostStackAdapter::HidConnectStateChangeCbk(&stackAddr, HID_DISCONNECTING, HID_CONNECTED, NLSTK_ERRCODE_SUCCESS);
    HidHostStackAdapter::HidConnectStateChangeCbk(&stackAddr, HID_DISCONNECTED, HID_DISCONNECTING,
            NLSTK_ERRCODE_SUCCESS);
    hidHostService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("HidHostServiceTest003 end");
}


} // namespace TEST
} // namespace Nearlink
} // namespace OHOS