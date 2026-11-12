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
#include <string.h>

#include "sdf_addr.h"
#include "sdf_mem.h"
#include "sdf_string.h"
#include "sdf_util.h"

TEST(SdfStringTest, FormatHexWithSpacesAllLengths)
{
    // 字节流按两位十六进制加空格输出：空流为空串、单字节无尾随空格、多字节以空格分隔
    const uint8_t single[] = {0xAB};
    const uint8_t triple[] = {0x01, 0x02, 0x03};
    EXPECT_STREQ(GetFormatHexWithSpaces(nullptr, 0).buf, "");
    EXPECT_STREQ(GetFormatHexWithSpaces(single, 1).buf, "AB");
    EXPECT_STREQ(GetFormatHexWithSpaces(triple, 3).buf, "01 02 03");
}

TEST(SdfStringTest, FormatHexWithSpacesTruncatesLongInput)
{
    // 超长字节流输出被截断在缓冲区内，始终以空字符结尾
    uint8_t big[100];
    for (size_t i = 0; i < sizeof(big); i++) {
        big[i] = static_cast<uint8_t>(i);
    }
    struct SDF_Uint8LogString res = GetFormatHexWithSpaces(big, sizeof(big));
    size_t len = strnlen(res.buf, SDF_UINT8_LOG_STR_LEN);
    EXPECT_LT(len, static_cast<size_t>(SDF_UINT8_LOG_STR_LEN));
    EXPECT_EQ(res.buf[len], '\0');
}

TEST(SdfStringTest, FormatHexWithoutSpaces)
{
    // 无空格格式把字节连成连续十六进制串
    const uint8_t bytes[] = {0x0A, 0xBC, 0xD0};
    EXPECT_STREQ(GetFormatHexWithoutSpaces(bytes, sizeof(bytes)).buf, "0ABCD0");
}

TEST(SdfStringTest, EncryptedHexKeepsShortStreamsPlain)
{
    // 长度 1 与 2 的字节流不脱敏，原样输出
    const uint8_t one[] = {0x11};
    const uint8_t two[] = {0x11, 0x22};
    EXPECT_STREQ(GetEncryptedHexWithSpaces(one, 1).buf, "11");
    EXPECT_STREQ(GetEncryptedHexWithSpaces(two, 2).buf, "11 22");
}

TEST(SdfStringTest, EncryptedHexMasksMiddleBytes)
{
    // 长度 3 只掩中间字节；更长时保留头两字节与尾一字节，中间全部掩码
    const uint8_t three[] = {0x11, 0x22, 0x33};
    const uint8_t four[] = {0x11, 0x22, 0x33, 0x44};
    const uint8_t five[] = {0x11, 0x22, 0x33, 0x44, 0x55};
    EXPECT_STREQ(GetEncryptedHexWithSpaces(three, 3).buf, "11 ** 33");
    EXPECT_STREQ(GetEncryptedHexWithSpaces(four, 4).buf, "11 22 ** 44");
    EXPECT_STREQ(GetEncryptedHexWithSpaces(five, 5).buf, "11 22 ** ** 55");
}

TEST(SdfStringTest, EncryptedHexTruncatesLongStreams)
{
    // 超长字节流的掩码段受缓冲区上限约束，结果始终空字符结尾
    uint8_t big[120];
    for (size_t i = 0; i < sizeof(big); i++) {
        big[i] = static_cast<uint8_t>(i);
    }
    struct SDF_Uint8LogString res = GetEncryptedHexWithSpaces(big, sizeof(big));
    size_t len = strnlen(res.buf, SDF_UINT8_LOG_STR_LEN);
    EXPECT_LT(len, static_cast<size_t>(SDF_UINT8_LOG_STR_LEN));
    EXPECT_EQ(res.buf[len], '\0');
    EXPECT_EQ(strncmp(res.buf, "00 01", 5), 0); // 头两字节保留
}

TEST(SdfAddrTest, EncryptAddrFormatsWithTypeAndEnds)
{
    // 地址日志保留大端序首尾字节并掩中间三字节，类型追加在括号内
    SLE_Addr_S addr = {};
    addr.type = RANDOM_ADDRESS;
    addr.addr[0] = 0xA0;
    addr.addr[4] = 0xB4;
    addr.addr[5] = 0xC5;
    struct SDF_EncryptedLogString res = GetEncryptAddr(&addr);
    EXPECT_STREQ(res.buf, "C5:B4:**:**:**:A0(6)");
}

TEST(SdfAddrTest, EncryptAddrToleratesNull)
{
    // 空地址返回空串
    EXPECT_STREQ(GetEncryptAddr(nullptr).buf, "");
}

TEST(SdfAddrTest, CompareSleAddrByTypeThenBytes)
{
    // 先比类型再按字节序比地址：类型差返回类型差、字节差返回首个差异、全等返回 0
    SLE_Addr_S lhs = {};
    SLE_Addr_S rhsSame = {};
    SLE_Addr_S rhsType = {};
    SLE_Addr_S rhsByte = {};
    lhs.type = PUBLIC_ADDRESS;
    rhsSame.type = PUBLIC_ADDRESS;
    rhsType.type = RANDOM_ADDRESS;
    rhsByte.type = PUBLIC_ADDRESS;
    lhs.addr[2] = 0x05;
    rhsSame.addr[2] = 0x05;
    rhsByte.addr[2] = 0x07;
    EXPECT_EQ(SDF_CompareSleAddr(&lhs, &rhsSame), 0);
    EXPECT_EQ(SDF_CompareSleAddr(&lhs, &rhsType), PUBLIC_ADDRESS - RANDOM_ADDRESS);
    EXPECT_EQ(SDF_CompareSleAddr(&lhs, &rhsByte), 0x05 - 0x07);
}

TEST(SdfUtilTest, StrDupCopiesAndToleratesNull)
{
    // 空指针返回空；复制内容一致且与源串独立，空串也能复制
    EXPECT_EQ(SDF_StrDup(nullptr), nullptr);

    char *copy = SDF_StrDup("nearlink");
    ASSERT_NE(copy, nullptr);
    EXPECT_STREQ(copy, "nearlink");
    SDF_MemFree(copy);

    char *empty = SDF_StrDup("");
    ASSERT_NE(empty, nullptr);
    EXPECT_STREQ(empty, "");
    SDF_MemFree(empty);
}

TEST(SdfUtilTest, PtrAssignUint32ReadsLittleEndian)
{
    // 空指针读出 0；合法指针按小端序组装 4 字节整数
    uint8_t bytes[] = {0x01, 0x00, 0x00, 0x00};
    EXPECT_EQ(SDF_PtrAssignUint32(nullptr), 0U);
    EXPECT_EQ(SDF_PtrAssignUint32(bytes), 0x1U);
}

TEST(SdfUtilTest, PtrAssignUint64ReadsLittleEndian)
{
    // 合法指针按小端序组装 8 字节整数
    uint8_t bytes[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    EXPECT_EQ(SDF_PtrAssignUint64(bytes), 0x1ULL);
}
