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

/*
 * 扫描结果拷贝构造测试：外设扫描结果转通用扫描结果时，携带字段按
 * 各自开关逐项搬运——全字段携带与全字段缺失两个方向都验证。
 * 产品源码直接编入本测试目标。
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <string>

#include "log.h"
#include "nearlink_scan_def.cpp"

using namespace OHOS::Nearlink;

namespace {
constexpr int8_t RSSI_VALUE = -42;
constexpr uint8_t AD_FLAG = 0x05;
constexpr uint16_t MANUFACTURER_ID_A = 0x0117;
constexpr uint16_t MANUFACTURER_ID_B = 0x0118;
constexpr uint32_t APPEARANCE_VALUE = 0x0940;

/* 构造携带全部广播字段的扫描结果 */
SleScanResultImpl BuildRichResult()
{
    SlePeripheralDevice device;
    device.SetRssi(RSSI_VALUE);
    device.SetAdFlag(AD_FLAG);
    device.SetManufacturerData(MANUFACTURER_ID_A, "firmwareA");
    device.SetManufacturerData(MANUFACTURER_ID_B, "firmwareB");
    device.SetServiceUUID(Uuid::ConvertFrom16Bits(0x0605));
    device.SetServiceDataUUID(Uuid::ConvertFrom16Bits(0xFDEE), std::string("\x01\x02\x03", 3));
    device.SetName("scanner-device");
    device.SetAppearance(APPEARANCE_VALUE);
    device.SetAddress(RawAddress("11:22:33:44:55:66"));

    uint8_t payload[] = {0xAA, 0xBB};
    device.SetPayload(payload, sizeof(payload));

    SleScanResultImpl impl;
    impl.SetPeripheralDevice(device);
    return impl;
}
} // namespace

/* 全字段携带：RSSI、广播标志、厂商数据、服务 UUID、服务数据、名称、外观、地址、载荷逐项一致 */
TEST(NearlinkScanDefCopyTest, CopyCarriesAllPresentFields)
{
    ScanResult result(BuildRichResult());

    EXPECT_EQ(result.GetRssi(), RSSI_VALUE);
    EXPECT_EQ(result.GetAdvertiseFlag(), AD_FLAG);

    auto manufacturer = result.GetManufacturerData();
    ASSERT_EQ(manufacturer.size(), 2U);
    EXPECT_EQ(manufacturer[MANUFACTURER_ID_A], "firmwareA");
    EXPECT_EQ(manufacturer[MANUFACTURER_ID_B], "firmwareB");

    auto serviceUuids = result.GetServiceUuids();
    ASSERT_EQ(serviceUuids.size(), 1U);
    EXPECT_TRUE(serviceUuids[0] == Uuid::ConvertFrom16Bits(0x0605));

    auto serviceData = result.GetServiceData();
    ASSERT_EQ(serviceData.size(), 1U);
    EXPECT_EQ(serviceData[Uuid::ConvertFrom16Bits(0xFDEE)], std::string("\x01\x02\x03", 3));

    EXPECT_EQ(result.GetName(), "scanner-device");
    EXPECT_EQ(result.GetDeviceClass(), APPEARANCE_VALUE);
    EXPECT_EQ(result.GetPeripheralDevice().GetAddress(), "11:22:33:44:55:66");
    EXPECT_EQ(result.GetPayload(), std::string("\xAA\xBB", 2));
}

/* 全字段缺失：仅地址与载荷为默认值，可选字段不触发搬运 */
TEST(NearlinkScanDefCopyTest, CopyWithEmptySourceKeepsDefaults)
{
    SleScanResultImpl impl;
    ScanResult result(impl);

    EXPECT_EQ(result.GetRssi(), 0);
    EXPECT_EQ(result.GetAdvertiseFlag(), 0);
    EXPECT_TRUE(result.GetManufacturerData().empty());
    EXPECT_TRUE(result.GetServiceUuids().empty());
    EXPECT_TRUE(result.GetServiceData().empty());
    EXPECT_EQ(result.GetName(), "");
    EXPECT_EQ(result.GetDeviceClass(), 0U);
}

/* 只带名称与服务 UUID：可选字段按各自开关独立搬运 */
TEST(NearlinkScanDefCopyTest, CopyCarriesOnlyNameAndServiceUuid)
{
    SlePeripheralDevice device;
    device.SetName("name-only");
    device.SetServiceUUID(Uuid::ConvertFrom16Bits(0x0606));

    SleScanResultImpl impl;
    impl.SetPeripheralDevice(device);
    ScanResult result(impl);

    EXPECT_EQ(result.GetName(), "name-only");
    ASSERT_EQ(result.GetServiceUuids().size(), 1U);
    EXPECT_TRUE(result.GetServiceUuids()[0] == Uuid::ConvertFrom16Bits(0x0606));
    EXPECT_EQ(result.GetRssi(), 0);
    EXPECT_TRUE(result.GetManufacturerData().empty());
}
