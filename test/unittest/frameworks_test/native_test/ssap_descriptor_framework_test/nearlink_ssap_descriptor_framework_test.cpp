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

// 目标: frameworks/native/nearlink_ssap_descriptor.cpp 零覆盖分支
// 瞄准行: 54/65/73(拷贝构造与赋值的深拷贝/清空分支), 102(SetValue 空参拒绝分支)

#include <gtest/gtest.h>
#include <cstring>

#include "log.h"
#include "nearlink_uuid.h"
// 编入产品源文件, 使本测试进程直接产出该文件的 gcda
#include "nearlink_ssap_descriptor.cpp"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
const std::string PROP_UUID_STRING = "11223344-0000-1000-8000-00805F9B1111";
const std::string SVC_UUID_STRING = "11223344-0000-1000-8000-00805F9B34FB";
constexpr int TEST_DESC_TYPE = 1;      // descriptorType_ 测试值
constexpr int TEST_DESC_PERM = 17;     // permission 测试值
constexpr uint16_t TEST_DESC_HANDLE = 0x0041;
constexpr size_t TEST_VALUE_LEN = 3;
const uint8_t TEST_VALUE[TEST_VALUE_LEN] = {1, 2, 3};
} // namespace

class SsapDescriptorFrameworkTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: DescriptorConstructAndGetters001
 * @tc.desc: 两参构造(type, permission), 验证各 getter 默认值
 * @tc.type: FUNC
 */
HWTEST_F(SsapDescriptorFrameworkTest, DescriptorConstructAndGetters001, TestSize.Level1)
{
    SsapDescriptor desc(TEST_DESC_TYPE, TEST_DESC_PERM);
    EXPECT_EQ(0, desc.GetHandle());
    EXPECT_EQ(TEST_DESC_TYPE, desc.GetDescriptorType());
    EXPECT_EQ(TEST_DESC_PERM, desc.GetDescriptorPermission());
    size_t size = 1; // 初值非 0, 验证被覆写
    const auto &value = desc.GetValue(&size);
    EXPECT_EQ(nullptr, value.get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: DescriptorConstructWithHandle002
 * @tc.desc: 三参构造(handle, type, permission), 验证 handle 生效
 * @tc.type: FUNC
 */
HWTEST_F(SsapDescriptorFrameworkTest, DescriptorConstructWithHandle002, TestSize.Level1)
{
    SsapDescriptor desc(TEST_DESC_HANDLE, TEST_DESC_TYPE, TEST_DESC_PERM);
    EXPECT_EQ(TEST_DESC_HANDLE, desc.GetHandle());
    EXPECT_EQ(TEST_DESC_TYPE, desc.GetDescriptorType());
    EXPECT_EQ(TEST_DESC_PERM, desc.GetDescriptorPermission());
}

/**
 * @tc.name: DescriptorCopyConstructWithValue001
 * @tc.desc: 拷贝构造深拷贝分支(length_ != 0 && value_ != nullptr 均真)
 * @tc.type: FUNC
 */
HWTEST_F(SsapDescriptorFrameworkTest, DescriptorCopyConstructWithValue001, TestSize.Level1)
{
    SsapDescriptor src(TEST_DESC_HANDLE, TEST_DESC_TYPE, TEST_DESC_PERM);
    src.SetValue(TEST_VALUE, TEST_VALUE_LEN);
    SsapDescriptor dst(src);
    EXPECT_EQ(TEST_DESC_HANDLE, dst.GetHandle());
    EXPECT_EQ(TEST_DESC_TYPE, dst.GetDescriptorType());
    EXPECT_EQ(TEST_DESC_PERM, dst.GetDescriptorPermission());
    size_t size = 0;
    const auto &value = dst.GetValue(&size);
    ASSERT_NE(nullptr, value.get());
    EXPECT_EQ(TEST_VALUE_LEN, size);
    EXPECT_EQ(0, std::memcmp(value.get(), TEST_VALUE, TEST_VALUE_LEN));
    // 深拷贝: 副本与源不是同一块缓冲
    size_t srcSize = 0;
    EXPECT_NE(src.GetValue(&srcSize).get(), value.get());
}

/**
 * @tc.name: DescriptorCopyConstructEmpty002
 * @tc.desc: 拷贝构造清空分支(length_ == 0 || value_ == nullptr), 副本无值
 * @tc.type: FUNC
 */
HWTEST_F(SsapDescriptorFrameworkTest, DescriptorCopyConstructEmpty002, TestSize.Level1)
{
    SsapDescriptor src(TEST_DESC_TYPE, TEST_DESC_PERM);
    SsapDescriptor dst(src);
    size_t size = 1;
    const auto &value = dst.GetValue(&size);
    EXPECT_EQ(nullptr, value.get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: DescriptorAssignSelf001
 * @tc.desc: 赋值自赋值分支(this == &src), 不改变自身
 * @tc.type: FUNC
 */
HWTEST_F(SsapDescriptorFrameworkTest, DescriptorAssignSelf001, TestSize.Level1)
{
    SsapDescriptor desc(TEST_DESC_HANDLE, TEST_DESC_TYPE, TEST_DESC_PERM);
    desc.SetValue(TEST_VALUE, TEST_VALUE_LEN);
    const SsapDescriptor *srcPtr = &desc;
    desc = *srcPtr; // 自赋值
    size_t size = 0;
    const auto &value = desc.GetValue(&size);
    ASSERT_NE(nullptr, value.get());
    EXPECT_EQ(TEST_VALUE_LEN, size);
    EXPECT_EQ(TEST_DESC_HANDLE, desc.GetHandle());
}

/**
 * @tc.name: DescriptorAssignWithValue002
 * @tc.desc: 赋值深拷贝分支: 目标原为空, 源带值
 * @tc.type: FUNC
 */
HWTEST_F(SsapDescriptorFrameworkTest, DescriptorAssignWithValue002, TestSize.Level1)
{
    SsapDescriptor src(TEST_DESC_HANDLE, TEST_DESC_TYPE, TEST_DESC_PERM);
    src.SetValue(TEST_VALUE, TEST_VALUE_LEN);
    SsapDescriptor dst(1, 2);
    dst = src;
    size_t size = 0;
    const auto &value = dst.GetValue(&size);
    ASSERT_NE(nullptr, value.get());
    EXPECT_EQ(TEST_VALUE_LEN, size);
    EXPECT_EQ(0, std::memcmp(value.get(), TEST_VALUE, TEST_VALUE_LEN));
    EXPECT_EQ(TEST_DESC_HANDLE, dst.GetHandle());
}

/**
 * @tc.name: DescriptorAssignEmptyOverValue003
 * @tc.desc: 赋值清空分支: 目标带值, 源为空, 目标被重置
 * @tc.type: FUNC
 */
HWTEST_F(SsapDescriptorFrameworkTest, DescriptorAssignEmptyOverValue003, TestSize.Level1)
{
    SsapDescriptor dst(TEST_DESC_TYPE, TEST_DESC_PERM);
    dst.SetValue(TEST_VALUE, TEST_VALUE_LEN);
    SsapDescriptor src(3, 4);
    dst = src;
    size_t size = 1;
    const auto &value = dst.GetValue(&size);
    EXPECT_EQ(nullptr, value.get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: DescriptorSetValue001
 * @tc.desc: SetValue 正常路径: 值与长度写入
 * @tc.type: FUNC
 */
HWTEST_F(SsapDescriptorFrameworkTest, DescriptorSetValue001, TestSize.Level1)
{
    SsapDescriptor desc(TEST_DESC_TYPE, TEST_DESC_PERM);
    desc.SetValue(TEST_VALUE, TEST_VALUE_LEN);
    size_t size = 0;
    const auto &value = desc.GetValue(&size);
    ASSERT_NE(nullptr, value.get());
    EXPECT_EQ(TEST_VALUE_LEN, size);
    EXPECT_EQ(0, std::memcmp(value.get(), TEST_VALUE, TEST_VALUE_LEN));
}

/**
 * @tc.name: DescriptorSetValueRejectNull002
 * @tc.desc: SetValue 空指针拒绝分支: 原值保持
 * @tc.type: FUNC
 */
HWTEST_F(SsapDescriptorFrameworkTest, DescriptorSetValueRejectNull002, TestSize.Level1)
{
    SsapDescriptor desc(TEST_DESC_TYPE, TEST_DESC_PERM);
    desc.SetValue(nullptr, TEST_VALUE_LEN);
    size_t size = 1;
    const auto &value = desc.GetValue(&size);
    EXPECT_EQ(nullptr, value.get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: DescriptorSetValueRejectZeroLen003
 * @tc.desc: SetValue 零长度拒绝分支: 原值保持
 * @tc.type: FUNC
 */
HWTEST_F(SsapDescriptorFrameworkTest, DescriptorSetValueRejectZeroLen003, TestSize.Level1)
{
    SsapDescriptor desc(TEST_DESC_TYPE, TEST_DESC_PERM);
    desc.SetValue(TEST_VALUE, 0);
    size_t size = 1;
    const auto &value = desc.GetValue(&size);
    EXPECT_EQ(nullptr, value.get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: DescriptorUuidRoundtrip001
 * @tc.desc: SetPropertyUuid/SetServiceUuid 与 getter 往返一致
 * @tc.type: FUNC
 */
HWTEST_F(SsapDescriptorFrameworkTest, DescriptorUuidRoundtrip001, TestSize.Level1)
{
    SsapDescriptor desc(TEST_DESC_TYPE, TEST_DESC_PERM);
    UUID propUuid = UUID::FromString(PROP_UUID_STRING);
    UUID svcUuid = UUID::FromString(SVC_UUID_STRING);
    desc.SetPropertyUuid(propUuid);
    desc.SetServiceUuid(svcUuid);
    EXPECT_TRUE(desc.GetPropertyUuid().Equals(propUuid));
    EXPECT_TRUE(desc.GetServiceUuid().Equals(svcUuid));
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
