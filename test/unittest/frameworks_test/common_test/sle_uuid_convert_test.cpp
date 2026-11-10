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
 * Uuid 字节序列转换测试：16 字节小端输入完整复制，不足 16 字节
 * 返回默认空值。产品源码直接编入本测试目标。
 */

#include <gtest/gtest.h>
#include <array>
#include <cstdint>

#include "log.h"
#include "sle_uuid.cpp"

using namespace OHOS::Nearlink;

namespace {
Uuid::UUID128Bit SampleBytes()
{
    Uuid::UUID128Bit bytes = {};
    for (size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<uint8_t>(i + 1);
    }
    return bytes;
}
} // namespace

/* 16 字节输入逐字节复制，与 128 位构造结果一致 */
TEST(SleUuidConvertTest, ConvertFromBytesSleFullLength)
{
    const Uuid::UUID128Bit bytes = SampleBytes();
    Uuid converted = Uuid::ConvertFromBytesSle(bytes.data(), bytes.size());
    EXPECT_TRUE(converted == Uuid::ConvertFrom128Bits(bytes));
}

/* 输入不足 16 字节：返回默认空 UUID，不做任何复制 */
TEST(SleUuidConvertTest, ConvertFromBytesSleShortInput)
{
    const Uuid::UUID128Bit bytes = SampleBytes();
    Uuid converted = Uuid::ConvertFromBytesSle(bytes.data(), bytes.size() - 1);
    EXPECT_TRUE(converted == Uuid());

    Uuid empty = Uuid::ConvertFromBytesSle(bytes.data(), 0);
    EXPECT_TRUE(empty == Uuid());
}

/* 16 位 UUID 转换：字节序与 16 位回读一致 */
TEST(SleUuidConvertTest, ConvertFrom16BitsRoundTrip)
{
    const uint16_t value = 0x0605;
    Uuid converted = Uuid::ConvertFrom16Bits(value);
    EXPECT_EQ(converted.ConvertTo16Bits(), value);
}
