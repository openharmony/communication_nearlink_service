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
 * nearlink_utils 纯函数测试：地址格式校验的各类非法输入、UUID 格式校验的
 * 长度/连字符位/非十六进制字符、字符串转整数与 UUID 大写化。
 * 产品源码直接编入本测试目标。
 */

#include <gtest/gtest.h>
#include <string>

#include "log.h"
#include "nearlink_utils.cpp"

using namespace OHOS::Nearlink;

namespace {
/* 合法 UUID（含大小写十六进制），供破坏某一位时复制使用 */
const std::string VALID_UUID = "0123456A-9abc-4DEF-8aF0-56789ABCDEF0";
} // namespace

/* 地址校验：合法地址通过，空串、长度差一、非十六进制、冒号位错字符均被拒绝 */
TEST(NearlinkUtilsParseTest, IsValidAddressMatrix)
{
    EXPECT_TRUE(IsValidAddress("01:23:45:67:89:AB"));
    EXPECT_TRUE(IsValidAddress("00:00:00:00:00:00"));

    /* 空串与长度不符 */
    EXPECT_FALSE(IsValidAddress(""));
    EXPECT_FALSE(IsValidAddress("01:23:45:67:89:A"));
    EXPECT_FALSE(IsValidAddress("01:23:45:67:89:ABC"));

    /* 十六进制位出现非法字符（大小写十六进制本身合法） */
    EXPECT_FALSE(IsValidAddress("g1:23:45:67:89:AB"));
    EXPECT_FALSE(IsValidAddress("01:G3:45:67:89:AB"));
    EXPECT_TRUE(IsValidAddress("01:23:45:67:89:aB"));

    /* 冒号位被数字或其他符号占用 */
    EXPECT_FALSE(IsValidAddress("0123:45:67:89:AB"));
    EXPECT_FALSE(IsValidAddress("01.23.45.67.89.AB"));
    EXPECT_FALSE(IsValidAddress("01:23:45:67:89;AB"));
}

/* UUID 校验：合法大小写通过，长度不符、四个连字符位破坏、非十六进制字符被拒绝 */
TEST(NearlinkUtilsParseTest, IsValidUuidMatrix)
{
    EXPECT_TRUE(IsValidUuid(VALID_UUID));

    /* 长度不符 */
    EXPECT_FALSE(IsValidUuid(""));
    EXPECT_FALSE(IsValidUuid(VALID_UUID.substr(0, 35)));
    EXPECT_FALSE(IsValidUuid(VALID_UUID + "0"));

    /* 连字符位被替换 */
    std::string broken = VALID_UUID;
    broken[8] = '8';
    EXPECT_FALSE(IsValidUuid(broken));
    broken = VALID_UUID;
    broken[13] = 'a';
    EXPECT_FALSE(IsValidUuid(broken));
    broken = VALID_UUID;
    broken[18] = '0';
    EXPECT_FALSE(IsValidUuid(broken));
    broken = VALID_UUID;
    broken[23] = 'f';
    EXPECT_FALSE(IsValidUuid(broken));

    /* 非十六进制字符 */
    broken = VALID_UUID;
    broken[0] = 'g';
    EXPECT_FALSE(IsValidUuid(broken));
}

/* 十进制字符串转整数：合法还原，空串/非数字/带尾巴失败 */
TEST(NearlinkUtilsParseTest, ConvertStrToIntMatrix)
{
    int value = 0;
    ASSERT_TRUE(ConvertStrToInt("42", value));
    EXPECT_EQ(value, 42);
    ASSERT_TRUE(ConvertStrToInt("-7", value));
    EXPECT_EQ(value, -7);

    EXPECT_FALSE(ConvertStrToInt("", value));
    EXPECT_FALSE(ConvertStrToInt("12a", value));
    EXPECT_FALSE(ConvertStrToInt("a12", value));
}

/* UUID 大写化只转换 a-f 区间字母，g 及其后的字母与数字原样 */
TEST(NearlinkUtilsParseTest, ConvertUuidToUpperCaseOnlyHexLetters)
{
    std::string text = "0a1b-cDeF-ghij";
    ConvertUuidToUpperCase(text);
    EXPECT_EQ(text, "0A1B-CDEF-ghij");
}
