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
#include "HidHostUhid.h"
#include "HidHostDefines.h"
#include "SleServiceManager.h"
#include "nearlink_access_token_mock.h"
#include "SleUtils.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

// 37BEA880-FC70-11EA-B720-000000000B06
// static NLSTK_SsapUuid_S HID_UUID = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20,
//                                  0x00, 0x00, 0x00, 0x00, 0x0B, 0x06};
// Mouse DescInfo
//    0x05, 0x01,       /* Usage Page (Generic Desktop Ctrls)                                           */
//    0x09, 0x02,       /* Usage (Mouse)                                                                */
//    0xA1, 0x01,       /* Collection (Application)                                                     */
//    0x85, (id),       /*   Report ID (1)                                                              */
//    0x09, 0x01,       /*   Usage (Pointer)                                                            */
//    0xA1, 0x00,       /*   Collection (Physical)                                                      */
//    0x95, 0x05,       /*     Report Count (5)                                                         */
//    0x75, 0x01,       /*     Report Size (1)                                                          */
//    0x05, 0x09,       /*     Usage Page (Button)                                                      */
//    0x19, 0x01,       /*     Usage Minimum (0x01)                                                     */
//    0x29, 0x05,       /*     Usage Maximum (0x05)                                                     */
//    0x15, 0x00,       /*     Logical Minimum (0)                                                      */
//    0x25, 0x01,       /*     Logical Maximum (1)                                                      */
//    0x81, 0x02,       /*     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)     */
//    0x95, 0x01,       /*     Report Count (1)                                                         */
//    0x75, 0x03,       /*     Report Size (3)                                                          */
//    0x81, 0x01,       /*     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)  */
//    0x05, 0x01,       /*     Usage Page (Generic Desktop Ctrls)                                       */
//    0x09, 0x30,       /*     Usage (X)                                                                */
//    0x09, 0x31,       /*     Usage (Y)                                                                */
//    0x95, 0x02,       /*     Report Count (2)                                                         */
//    0x75, 0x10,       /*     Report Size (16)                                                         */
//    0x16, 0x01, 0x80, /*     Logical Minimum (-32767)                                                 */
//    0x26, 0xFF, 0x7F, /*     Logical Maximum (32767)                                                  */
//    0x81, 0x06,       /*     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)     */
//    0x09, 0x38,       /*     Usage (Wheel)                                                            */
//    0x95, 0x01,       /*     Report Count (1)                                                         */
//    0x75, 0x08,       /*     Report Size (8)                                                          */
//    0x15, 0x81,       /*     Logical Minimum (-127)                                                   */
//    0x25, 0x7F,       /*     Logical Maximum (127)                                                    */
//    0x81, 0x06,       /*     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)     */
//    0xC0,             /*   End Collection                                                             */
//    0xC0,             /* End Collection                                                               */

static uint8_t MOUSE_DESCRIPTOR[] = {0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x85, 0x01, 0x09, 0x01,
                                     0xA1, 0x00, 0x95, 0x05, 0x75, 0x01, 0x05, 0x09, 0x19, 0x01,
                                     0x29, 0x05, 0x15, 0x00, 0x25, 0x01, 0x81, 0x02, 0x95, 0x01,
                                     0x75, 0x03, 0x81, 0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31,
                                     0x95, 0x02, 0x75, 0x10, 0x16, 0x01, 0x80, 0x26, 0xFF, 0x7F,
                                     0x81, 0x06, 0x09, 0x38, 0x95, 0x01, 0x75, 0x08, 0x15, 0x81,
                                     0x25, 0x7F, 0x81, 0x06, 0xC0, 0xC0};

constexpr int DELAY_LITTLE_MS = 100;

class HidHostUhidTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HidHostUhidTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

void HidHostUhidTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase start");
    SleInterfaceManager::GetInstance()->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

}

void HidHostUhidTest::SetUp()
{
    HILOGI("SetUp start");

}

void HidHostUhidTest::TearDown()
{
    HILOGI("TearDown start");
}

/**
 * @tc.number: HidHostUhidTest001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(HidHostUhidTest, HidHostUhidTest001, TestSize.Level1)
{
    HILOGI("HidHostUhidTest001 start");
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr <HidHostUhid> uhid = std::make_shared<HidHostUhid>(addr);
    int ret = uhid->Open();
    EXPECT_EQ(HID_HOST_SUCCESS, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HidInformation hidInfo;
    size_t len = sizeof(MOUSE_DESCRIPTOR);
    hidInfo.descInfo = std::make_unique<uint8_t[]>(len);
    memcpy_s(hidInfo.descInfo.get(), len, MOUSE_DESCRIPTOR, len);
    hidInfo.ctryCode = 0;
    hidInfo.descLength = len;
    uhid->SendHidInfo(hidInfo);
    // 0F031A000600000000FDFF00
    std::string hidStr = "0F031A000600000000FDFF00";
    std::vector <uint8_t> hidPtk;
    SleUtils::ConvertHexStringToInt(hidStr, hidPtk);
    uhid->SendData(hidPtk.data(), hidPtk.size());
    uhid->SendHandshake(HID_HOST_HANDSHAKE_ERROR);
    uhid->SendControlData(hidPtk.data(), hidPtk.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS * 5));

    ret = uhid->SendControlData(hidPtk.data(), hidPtk.size());
    EXPECT_EQ(HID_HOST_SUCCESS, ret);
    uhid->SendData(hidPtk.data(), hidPtk.size());
    EXPECT_EQ(HID_HOST_SUCCESS, ret);
    struct uhid_event ev;
    ev.u.output.rtype = UHID_FEATURE_REPORT;
    uhid->ReadUhidOutPut(ev);
    ev.u.output.rtype = UHID_OUTPUT_REPORT;
    uhid->ReadUhidOutPut(ev);
    ev.u.output.rtype = HID_HOST_INPUT_REPORT;
    uhid->ReadUhidOutPut(ev);
    ev.u.feature.id = 1;
    ev.u.feature.rnum = 1;
    uhid->ReadUhidFeature(ev);
    uhid->SendHandshake(HID_HOST_HANDSHAKE_ERROR);
    uhid->ReadUhidSetReport(ev);
    uhid->SendHandshake(HID_HOST_HANDSHAKE_ERROR);
    uhid->Close();
    HILOGI("HidHostUhidTest001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS