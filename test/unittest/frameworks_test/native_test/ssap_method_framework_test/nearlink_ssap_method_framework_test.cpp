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

// 目标: frameworks/native/nearlink_ssap_method.cpp 零覆盖分支
// 覆盖: 拷贝构造 parameter_/result_ 深拷贝分支, 赋值自赋值与深拷贝分支

#include <gtest/gtest.h>
#include <cstring>
#include <vector>

#include "log.h"
#include "nearlink_uuid.h"
// 编入产品源文件, 使本测试进程直接产出该文件的 gcda
#include "nearlink_ssap_method.cpp"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
const std::string MTH_UUID_STRING = "11223344-0000-1000-8000-00805F9B3333";
const std::string SVC_UUID_STRING = "11223344-0000-1000-8000-00805F9B34FB";
constexpr int TEST_MTH_TYPE = 7;      // methodType_ 测试值
constexpr int TEST_MTH_PERM = 34;     // permissions_ 测试值
constexpr uint16_t TEST_MTH_HANDLE = 0x0021;
constexpr size_t TEST_PARAM_LEN = 2;
constexpr size_t TEST_RESULT_LEN = 4;
const uint8_t TEST_PARAM[TEST_PARAM_LEN] = {0x11, 0x22};
const uint8_t TEST_RESULT[TEST_RESULT_LEN] = {0xAA, 0xBB, 0xCC, 0xDD};
// 与产品 SSAP_METHOD_MAX_DATA_LEN(0x1000) 对齐的边界值
constexpr size_t SSAP_METHOD_MAX_DATA_LEN = 0x1000;
} // namespace

class SsapMethodFrameworkTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: MethodConstructAndGetters001
 * @tc.desc: 两种构造: (type,uuid,perm)/(handle,type,uuid,perm) 各字段默认与生效值
 * @tc.type: FUNC
 */
HWTEST_F(SsapMethodFrameworkTest, MethodConstructAndGetters001, TestSize.Level1)
{
    UUID mthUuid = UUID::FromString(MTH_UUID_STRING);
    SsapMethod byType(TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    EXPECT_EQ(0, byType.GetHandle());
    EXPECT_EQ(TEST_MTH_TYPE, byType.GetMethodType());
    EXPECT_EQ(TEST_MTH_PERM, byType.GetPermissions());
    EXPECT_TRUE(byType.GetUuid().Equals(mthUuid));
    size_t size = 1;
    EXPECT_EQ(nullptr, byType.GetParameter(&size).get());
    EXPECT_EQ(0U, size);
    size = 1;
    EXPECT_EQ(nullptr, byType.GetResult(&size).get());
    EXPECT_EQ(0U, size);

    SsapMethod byHandle(TEST_MTH_HANDLE, TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    EXPECT_EQ(TEST_MTH_HANDLE, byHandle.GetHandle());
    EXPECT_EQ(TEST_MTH_TYPE, byHandle.GetMethodType());
}

/**
 * @tc.name: MethodCopyConstructBothSet001
 * @tc.desc: 拷贝构造 parameter_/result_ 双深拷贝分支(两条件均真)
 * @tc.type: FUNC
 */
HWTEST_F(SsapMethodFrameworkTest, MethodCopyConstructBothSet001, TestSize.Level1)
{
    UUID mthUuid = UUID::FromString(MTH_UUID_STRING);
    SsapMethod src(TEST_MTH_HANDLE, TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    src.SetParameter(TEST_PARAM, TEST_PARAM_LEN);
    src.SetResult(TEST_RESULT, TEST_RESULT_LEN);
    SsapMethod dst(src);
    size_t paramSize = 0;
    const auto &param = dst.GetParameter(&paramSize);
    ASSERT_NE(nullptr, param.get());
    EXPECT_EQ(TEST_PARAM_LEN, paramSize);
    EXPECT_EQ(0, std::memcmp(param.get(), TEST_PARAM, TEST_PARAM_LEN));
    size_t resultSize = 0;
    const auto &result = dst.GetResult(&resultSize);
    ASSERT_NE(nullptr, result.get());
    EXPECT_EQ(TEST_RESULT_LEN, resultSize);
    EXPECT_EQ(0, std::memcmp(result.get(), TEST_RESULT, TEST_RESULT_LEN));
    // 深拷贝: 副本与源不是同一块缓冲
    size_t srcSize = 0;
    EXPECT_NE(src.GetParameter(&srcSize).get(), param.get());
    EXPECT_NE(src.GetResult(&srcSize).get(), result.get());
}

/**
 * @tc.name: MethodCopyConstructOnlyParameter002
 * @tc.desc: 拷贝构造仅 parameter_ 命中分支, result_ 走清空分支
 * @tc.type: FUNC
 */
HWTEST_F(SsapMethodFrameworkTest, MethodCopyConstructOnlyParameter002, TestSize.Level1)
{
    UUID mthUuid = UUID::FromString(MTH_UUID_STRING);
    SsapMethod src(TEST_MTH_HANDLE, TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    src.SetParameter(TEST_PARAM, TEST_PARAM_LEN);
    SsapMethod dst(src);
    size_t size = 0;
    EXPECT_NE(nullptr, dst.GetParameter(&size).get());
    EXPECT_EQ(TEST_PARAM_LEN, size);
    size = 1;
    EXPECT_EQ(nullptr, dst.GetResult(&size).get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: MethodCopyConstructOnlyResult003
 * @tc.desc: 拷贝构造 parameter_ 走清空分支, 仅 result_ 命中分支
 * @tc.type: FUNC
 */
HWTEST_F(SsapMethodFrameworkTest, MethodCopyConstructOnlyResult003, TestSize.Level1)
{
    UUID mthUuid = UUID::FromString(MTH_UUID_STRING);
    SsapMethod src(TEST_MTH_HANDLE, TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    src.SetResult(TEST_RESULT, TEST_RESULT_LEN);
    SsapMethod dst(src);
    size_t size = 1;
    EXPECT_EQ(nullptr, dst.GetParameter(&size).get());
    EXPECT_EQ(0U, size);
    size = 0;
    EXPECT_NE(nullptr, dst.GetResult(&size).get());
    EXPECT_EQ(TEST_RESULT_LEN, size);
}

/**
 * @tc.name: MethodCopyConstructBothEmpty004
 * @tc.desc: 拷贝构造 parameter_/result_ 双清空分支(两条件均假)
 * @tc.type: FUNC
 */
HWTEST_F(SsapMethodFrameworkTest, MethodCopyConstructBothEmpty004, TestSize.Level1)
{
    UUID mthUuid = UUID::FromString(MTH_UUID_STRING);
    SsapMethod src(TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    SsapMethod dst(src);
    size_t size = 1;
    EXPECT_EQ(nullptr, dst.GetParameter(&size).get());
    EXPECT_EQ(0U, size);
    size = 1;
    EXPECT_EQ(nullptr, dst.GetResult(&size).get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: MethodAssignSelf001
 * @tc.desc: 赋值自赋值分支(this == &src), 不改变自身
 * @tc.type: FUNC
 */
HWTEST_F(SsapMethodFrameworkTest, MethodAssignSelf001, TestSize.Level1)
{
    UUID mthUuid = UUID::FromString(MTH_UUID_STRING);
    SsapMethod mth(TEST_MTH_HANDLE, TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    mth.SetParameter(TEST_PARAM, TEST_PARAM_LEN);
    mth.SetResult(TEST_RESULT, TEST_RESULT_LEN);
    const SsapMethod *srcPtr = &mth;
    mth = *srcPtr; // 自赋值
    size_t size = 0;
    EXPECT_NE(nullptr, mth.GetParameter(&size).get());
    EXPECT_EQ(TEST_PARAM_LEN, size);
    EXPECT_EQ(TEST_MTH_HANDLE, mth.GetHandle());
}

/**
 * @tc.name: MethodAssignWithBoth002
 * @tc.desc: 赋值双深拷贝分支: 目标原为空, 源 parameter_/result_ 均带值
 * @tc.type: FUNC
 */
HWTEST_F(SsapMethodFrameworkTest, MethodAssignWithBoth002, TestSize.Level1)
{
    UUID mthUuid = UUID::FromString(MTH_UUID_STRING);
    SsapMethod src(TEST_MTH_HANDLE, TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    src.SetParameter(TEST_PARAM, TEST_PARAM_LEN);
    src.SetResult(TEST_RESULT, TEST_RESULT_LEN);
    SsapMethod dst(1, TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    dst = src;
    size_t size = 0;
    EXPECT_NE(nullptr, dst.GetParameter(&size).get());
    EXPECT_EQ(TEST_PARAM_LEN, size);
    size = 0;
    EXPECT_NE(nullptr, dst.GetResult(&size).get());
    EXPECT_EQ(TEST_RESULT_LEN, size);
    EXPECT_EQ(TEST_MTH_HANDLE, dst.GetHandle());
    EXPECT_TRUE(dst.GetUuid().Equals(mthUuid));
}

/**
 * @tc.name: MethodAssignEmptyOverBoth003
 * @tc.desc: 赋值双清空分支: 目标带值, 源为空, 目标被重置
 * @tc.type: FUNC
 */
HWTEST_F(SsapMethodFrameworkTest, MethodAssignEmptyOverBoth003, TestSize.Level1)
{
    UUID mthUuid = UUID::FromString(MTH_UUID_STRING);
    SsapMethod dst(TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    dst.SetParameter(TEST_PARAM, TEST_PARAM_LEN);
    dst.SetResult(TEST_RESULT, TEST_RESULT_LEN);
    SsapMethod src(2, TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    dst = src;
    size_t size = 1;
    EXPECT_EQ(nullptr, dst.GetParameter(&size).get());
    EXPECT_EQ(0U, size);
    size = 1;
    EXPECT_EQ(nullptr, dst.GetResult(&size).get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: MethodSetParameterReject001
 * @tc.desc: SetParameter 空指针/零长度/超长拒绝分支, 原值保持
 * @tc.type: FUNC
 */
HWTEST_F(SsapMethodFrameworkTest, MethodSetParameterReject001, TestSize.Level1)
{
    UUID mthUuid = UUID::FromString(MTH_UUID_STRING);
    SsapMethod mth(TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    mth.SetParameter(nullptr, TEST_PARAM_LEN);
    mth.SetParameter(TEST_PARAM, 0);
    std::vector<uint8_t> overMax(SSAP_METHOD_MAX_DATA_LEN + 1, 0x55);
    mth.SetParameter(overMax.data(), overMax.size());
    size_t size = 1;
    EXPECT_EQ(nullptr, mth.GetParameter(&size).get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: MethodSetParameterMaxLength002
 * @tc.desc: SetParameter 恰好最大长度(0x1000)被接受
 * @tc.type: FUNC
 */
HWTEST_F(SsapMethodFrameworkTest, MethodSetParameterMaxLength002, TestSize.Level1)
{
    UUID mthUuid = UUID::FromString(MTH_UUID_STRING);
    SsapMethod mth(TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    std::vector<uint8_t> maxLen(SSAP_METHOD_MAX_DATA_LEN, 0x66);
    mth.SetParameter(maxLen.data(), maxLen.size());
    size_t size = 0;
    EXPECT_NE(nullptr, mth.GetParameter(&size).get());
    EXPECT_EQ(SSAP_METHOD_MAX_DATA_LEN, size);
}

/**
 * @tc.name: MethodSetResultReject001
 * @tc.desc: SetResult 空指针/零长度/超长拒绝分支, 原值保持
 * @tc.type: FUNC
 */
HWTEST_F(SsapMethodFrameworkTest, MethodSetResultReject001, TestSize.Level1)
{
    UUID mthUuid = UUID::FromString(MTH_UUID_STRING);
    SsapMethod mth(TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    mth.SetResult(nullptr, TEST_RESULT_LEN);
    mth.SetResult(TEST_RESULT, 0);
    std::vector<uint8_t> overMax(SSAP_METHOD_MAX_DATA_LEN + 1, 0x77);
    mth.SetResult(overMax.data(), overMax.size());
    size_t size = 1;
    EXPECT_EQ(nullptr, mth.GetResult(&size).get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: MethodSetServiceUuid001
 * @tc.desc: SetServiceUuid 与 getter 往返一致
 * @tc.type: FUNC
 */
HWTEST_F(SsapMethodFrameworkTest, MethodSetServiceUuid001, TestSize.Level1)
{
    UUID mthUuid = UUID::FromString(MTH_UUID_STRING);
    UUID svcUuid = UUID::FromString(SVC_UUID_STRING);
    SsapMethod mth(TEST_MTH_TYPE, mthUuid, TEST_MTH_PERM);
    mth.SetServiceUuid(svcUuid);
    EXPECT_TRUE(mth.GetServiceUuid().Equals(svcUuid));
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
