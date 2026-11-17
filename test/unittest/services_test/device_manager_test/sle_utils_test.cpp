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
 * SleUtils 工具函数测试：随机地址两种末字节形态、十六进制字符串与
 * 字节序列互转的合法与非法输入、字符串转十六进制文本。
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <string>
#include <vector>

#include "SleUtils.h"
#include "SleDefs.h"

using namespace OHOS::Nearlink;

/* 随机地址两种形态：静态随机地址末字节高两位置位，非可解析私有地址高两位清零 */
TEST(SleUtilsTest, GetRandomAddressTailByteByAddressKind)
{
    std::vector<uint8_t> staticAddr;
    SleUtils::GetRandomAddress(staticAddr, false);
    ASSERT_EQ(staticAddr.size(), static_cast<size_t>(SLE_ADDR_LEN));
    EXPECT_EQ(staticAddr.back() & SLE_STATIC_PRI_ADDR, SLE_STATIC_PRI_ADDR);

    std::vector<uint8_t> nonResolvableAddr;
    SleUtils::GetRandomAddress(nonResolvableAddr, true);
    ASSERT_EQ(nonResolvableAddr.size(), static_cast<size_t>(SLE_ADDR_LEN));
    EXPECT_EQ(nonResolvableAddr.back() & ~SLE_NON_RES_PRI_ADDR, 0);
}

/* 字节数组转十六进制大写文本，逐字节两位 */
TEST(SleUtilsTest, ConvertIntToHexStringUppercase)
{
    std::vector<uint8_t> bytes = {0x00, 0x0a, 0x7f, 0xff};
    EXPECT_EQ(SleUtils::ConvertIntToHexString(bytes), "000A7FFF");

    uint8_t raw[2] = {0xab, 0xcd};
    EXPECT_EQ(SleUtils::ConvertIntToHexString(raw, sizeof(raw)), "ABCD");
}

/* 十六进制文本转字节：合法输入逐字节还原，非法输入整趟失败 */
TEST(SleUtilsTest, ConvertHexStringToIntRoundTrip)
{
    const std::string hexText = "0A1B2C3D";
    uint8_t bytes[4] = {0};
    ASSERT_TRUE(SleUtils::ConvertHexStringToInt(hexText, bytes, sizeof(bytes)));
    EXPECT_EQ(bytes[0], 0x0A);
    EXPECT_EQ(bytes[1], 0x1B);
    EXPECT_EQ(bytes[2], 0x2C);
    EXPECT_EQ(bytes[3], 0x3D);

    /* 再转回文本保持一致 */
    EXPECT_EQ(SleUtils::ConvertIntToHexString(bytes, sizeof(bytes)), hexText);
}

/* 十六进制文本转字节的参数校验：非十六进制字符、长度为奇数、长度与缓冲不符 */
TEST(SleUtilsTest, ConvertHexStringToIntRejectsBadParams)
{
    uint8_t bytes[2] = {0};
    /* 非十六进制字符 */
    EXPECT_FALSE(SleUtils::ConvertHexStringToInt("GG", bytes, 1));
    /* 半个字节 */
    EXPECT_FALSE(SleUtils::ConvertHexStringToInt("0", bytes, 1));
    /* 文本对数与缓冲长度不符 */
    EXPECT_FALSE(SleUtils::ConvertHexStringToInt("0102", bytes, 1));
    EXPECT_FALSE(SleUtils::ConvertHexStringToInt("0102", bytes, 3));
}

/* 面向 C 字符串的十六进制转换：合法还原，非法返回失败 */
TEST(SleUtilsTest, ConvertHexCharToIntRoundTripAndReject)
{
    uint8_t bytes[2] = {0};
    ASSERT_TRUE(SleUtils::ConvertHexCharToInt("FE01", bytes, sizeof(bytes)));
    EXPECT_EQ(bytes[0], 0xFE);
    EXPECT_EQ(bytes[1], 0x01);

    EXPECT_FALSE(SleUtils::ConvertHexCharToInt("XY", bytes, 1));
    EXPECT_FALSE(SleUtils::ConvertHexCharToInt(nullptr, bytes, 1));
    EXPECT_FALSE(SleUtils::ConvertHexCharToInt("0102", bytes, 1));
}

/* 普通字符串按字节展开为两位十六进制文本 */
TEST(SleUtilsTest, StringDataToHexString)
{
    EXPECT_EQ(SleUtils::StringDataToHexString("Az"), "417A");
    EXPECT_EQ(SleUtils::StringDataToHexString(""), "");
}
