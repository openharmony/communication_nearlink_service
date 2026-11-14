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

// 目标: frameworks/native/nearlink_ssap_property.cpp 零覆盖分支
// 覆盖: 拷贝构造与赋值的深拷贝/清空分支, GetDescriptor 类型命中分支, GetValue 空出参分支

#include <gtest/gtest.h>
#include <cstring>

#include "log.h"
#include "nearlink_uuid.h"
// 编入产品源文件, 使本测试进程直接产出该文件的 gcda
#include "nearlink_ssap_property.cpp"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
const std::string PROP_UUID_STRING = "11223344-0000-1000-8000-00805F9B1111";
const std::string SVC_UUID_STRING = "11223344-0000-1000-8000-00805F9B34FB";
constexpr int TEST_PROP_TYPE = 2;      // ENTRY_TYPE_PROPERTY
constexpr int TEST_PROP_PERM = 17;
constexpr uint32_t TEST_OPER_IND = 0x0F;
constexpr uint16_t TEST_PROP_HANDLE = 0x0011;
constexpr int DESC_TYPE_A = 1;
constexpr int DESC_TYPE_B = 2;
constexpr int DESC_TYPE_ABSENT = 3;
constexpr size_t TEST_VALUE_LEN = 3;
const uint8_t TEST_VALUE[TEST_VALUE_LEN] = {1, 2, 3};
} // namespace

class SsapPropertyFrameworkTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: PropertyConstructThreeWays001
 * @tc.desc: 三种构造: (type,uuid,oper,perm)/(handle,type,uuid,oper,perm)/(handle,uuid,oper,perm)
 * @tc.type: FUNC
 */
HWTEST_F(SsapPropertyFrameworkTest, PropertyConstructThreeWays001, TestSize.Level1)
{
    UUID propUuid = UUID::FromString(PROP_UUID_STRING);
    SsapProperty byType(TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    EXPECT_EQ(0, byType.GetHandle());
    EXPECT_EQ(TEST_PROP_TYPE, byType.GetPropertyType());
    EXPECT_EQ(TEST_OPER_IND, byType.GetOperationIndication());
    EXPECT_EQ(TEST_PROP_PERM, byType.GetValuePermissions());
    EXPECT_TRUE(byType.GetUuid().Equals(propUuid));
    EXPECT_EQ(SsapProperty::PROPERTY_WRITE_DEFAULT, byType.GetWriteType());

    SsapProperty byHandleType(TEST_PROP_HANDLE, TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    EXPECT_EQ(TEST_PROP_HANDLE, byHandleType.GetHandle());
    EXPECT_EQ(TEST_PROP_TYPE, byHandleType.GetPropertyType());

    SsapProperty byHandle(TEST_PROP_HANDLE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    EXPECT_EQ(TEST_PROP_HANDLE, byHandle.GetHandle());
    EXPECT_EQ(0, byHandle.GetPropertyType());
}

/**
 * @tc.name: PropertyCopyConstructWithValue001
 * @tc.desc: 拷贝构造深拷贝分支(value_ 非空且 length_ 非零), descriptor 一并复制
 * @tc.type: FUNC
 */
HWTEST_F(SsapPropertyFrameworkTest, PropertyCopyConstructWithValue001, TestSize.Level1)
{
    UUID propUuid = UUID::FromString(PROP_UUID_STRING);
    SsapProperty src(TEST_PROP_HANDLE, TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    src.SetValue(TEST_VALUE, TEST_VALUE_LEN);
    src.AddDescriptor(SsapDescriptor(0x0041, DESC_TYPE_A, TEST_PROP_PERM));
    SsapProperty dst(src);
    EXPECT_EQ(TEST_PROP_HANDLE, dst.GetHandle());
    EXPECT_EQ(TEST_PROP_TYPE, dst.GetPropertyType());
    EXPECT_TRUE(dst.GetUuid().Equals(propUuid));
    size_t size = 0;
    const auto &value = dst.GetValue(&size);
    ASSERT_NE(nullptr, value.get());
    EXPECT_EQ(TEST_VALUE_LEN, size);
    EXPECT_EQ(0, std::memcmp(value.get(), TEST_VALUE, TEST_VALUE_LEN));
    EXPECT_EQ(1U, dst.GetDescriptors().size());
    // 深拷贝: 副本与源不是同一块缓冲
    size_t srcSize = 0;
    EXPECT_NE(src.GetValue(&srcSize).get(), value.get());
}

/**
 * @tc.name: PropertyCopyConstructEmpty002
 * @tc.desc: 拷贝构造清空分支(value_ 为空), 副本无值
 * @tc.type: FUNC
 */
HWTEST_F(SsapPropertyFrameworkTest, PropertyCopyConstructEmpty002, TestSize.Level1)
{
    UUID propUuid = UUID::FromString(PROP_UUID_STRING);
    SsapProperty src(TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    SsapProperty dst(src);
    size_t size = 1;
    const auto &value = dst.GetValue(&size);
    EXPECT_EQ(nullptr, value.get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: PropertyAssignSelf001
 * @tc.desc: 赋值自赋值分支(this == &src), 不改变自身
 * @tc.type: FUNC
 */
HWTEST_F(SsapPropertyFrameworkTest, PropertyAssignSelf001, TestSize.Level1)
{
    UUID propUuid = UUID::FromString(PROP_UUID_STRING);
    SsapProperty prop(TEST_PROP_HANDLE, TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    prop.SetValue(TEST_VALUE, TEST_VALUE_LEN);
    const SsapProperty *srcPtr = &prop;
    prop = *srcPtr; // 自赋值
    size_t size = 0;
    const auto &value = prop.GetValue(&size);
    ASSERT_NE(nullptr, value.get());
    EXPECT_EQ(TEST_VALUE_LEN, size);
    EXPECT_EQ(TEST_PROP_HANDLE, prop.GetHandle());
}

/**
 * @tc.name: PropertyAssignWithValue002
 * @tc.desc: 赋值深拷贝分支: 目标原为空, 源带值
 * @tc.type: FUNC
 */
HWTEST_F(SsapPropertyFrameworkTest, PropertyAssignWithValue002, TestSize.Level1)
{
    UUID propUuid = UUID::FromString(PROP_UUID_STRING);
    SsapProperty src(TEST_PROP_HANDLE, TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    src.SetValue(TEST_VALUE, TEST_VALUE_LEN);
    SsapProperty dst(1, TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    dst = src;
    size_t size = 0;
    const auto &value = dst.GetValue(&size);
    ASSERT_NE(nullptr, value.get());
    EXPECT_EQ(TEST_VALUE_LEN, size);
    EXPECT_EQ(TEST_PROP_HANDLE, dst.GetHandle());
}

/**
 * @tc.name: PropertyAssignEmptyOverValue003
 * @tc.desc: 赋值清空分支: 目标带值, 源为空, 目标被重置
 * @tc.type: FUNC
 */
HWTEST_F(SsapPropertyFrameworkTest, PropertyAssignEmptyOverValue003, TestSize.Level1)
{
    UUID propUuid = UUID::FromString(PROP_UUID_STRING);
    SsapProperty dst(TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    dst.SetValue(TEST_VALUE, TEST_VALUE_LEN);
    SsapProperty src(2, TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    dst = src;
    size_t size = 1;
    const auto &value = dst.GetValue(&size);
    EXPECT_EQ(nullptr, value.get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: PropertyGetDescriptorByType001
 * @tc.desc: GetDescriptor 类型命中分支: 返回首个同类型描述符
 * @tc.type: FUNC
 */
HWTEST_F(SsapPropertyFrameworkTest, PropertyGetDescriptorByType001, TestSize.Level1)
{
    UUID propUuid = UUID::FromString(PROP_UUID_STRING);
    SsapProperty prop(TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    SsapDescriptor *absent = prop.GetDescriptor(DESC_TYPE_A);
    EXPECT_EQ(nullptr, absent); // 空表未命中
    prop.AddDescriptor(SsapDescriptor(0x0041, DESC_TYPE_A, TEST_PROP_PERM));
    prop.AddDescriptor(SsapDescriptor(0x0042, DESC_TYPE_B, TEST_PROP_PERM));
    SsapDescriptor *hit = prop.GetDescriptor(DESC_TYPE_A);
    ASSERT_NE(nullptr, hit);
    EXPECT_EQ(0x0041, hit->GetHandle());
    EXPECT_EQ(DESC_TYPE_A, hit->GetDescriptorType());
    SsapDescriptor *hitB = prop.GetDescriptor(DESC_TYPE_B);
    ASSERT_NE(nullptr, hitB);
    EXPECT_EQ(0x0042, hitB->GetHandle());
    SsapDescriptor *miss = prop.GetDescriptor(DESC_TYPE_ABSENT);
    EXPECT_EQ(nullptr, miss); // 有表未命中
    EXPECT_EQ(2U, prop.GetDescriptors().size());
}

/**
 * @tc.name: PropertyAddDescriptorSetsUuid001
 * @tc.desc: AddDescriptor 后描述符继承属性 uuid(GetPropertyUuid 被回填)
 * @tc.type: FUNC
 */
HWTEST_F(SsapPropertyFrameworkTest, PropertyAddDescriptorSetsUuid001, TestSize.Level1)
{
    UUID propUuid = UUID::FromString(PROP_UUID_STRING);
    SsapProperty prop(TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    prop.AddDescriptor(SsapDescriptor(0x0041, DESC_TYPE_A, TEST_PROP_PERM));
    SsapDescriptor *desc = prop.GetDescriptor(DESC_TYPE_A);
    ASSERT_NE(nullptr, desc);
    EXPECT_TRUE(desc->GetPropertyUuid().Equals(propUuid));
}

/**
 * @tc.name: PropertyGetValueWithNullSize001
 * @tc.desc: GetValue 空出参分支(size == nullptr 不解引用, 不崩溃)
 * @tc.type: FUNC
 */
HWTEST_F(SsapPropertyFrameworkTest, PropertyGetValueWithNullSize001, TestSize.Level1)
{
    UUID propUuid = UUID::FromString(PROP_UUID_STRING);
    SsapProperty prop(TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    prop.SetValue(TEST_VALUE, TEST_VALUE_LEN);
    const auto &value = prop.GetValue(nullptr);
    ASSERT_NE(nullptr, value.get());
    EXPECT_EQ(0, std::memcmp(value.get(), TEST_VALUE, TEST_VALUE_LEN));
}

/**
 * @tc.name: PropertySetValueAndReject001
 * @tc.desc: SetValue 正常路径与空指针/零长度拒绝分支
 * @tc.type: FUNC
 */
HWTEST_F(SsapPropertyFrameworkTest, PropertySetValueAndReject001, TestSize.Level1)
{
    UUID propUuid = UUID::FromString(PROP_UUID_STRING);
    SsapProperty prop(TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    prop.SetValue(TEST_VALUE, TEST_VALUE_LEN);
    size_t size = 0;
    EXPECT_NE(nullptr, prop.GetValue(&size).get());
    EXPECT_EQ(TEST_VALUE_LEN, size);

    SsapProperty rejected(TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    rejected.SetValue(nullptr, TEST_VALUE_LEN);
    rejected.SetValue(TEST_VALUE, 0);
    size = 1;
    EXPECT_EQ(nullptr, rejected.GetValue(&size).get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: PropertySetWriteType001
 * @tc.desc: SetWriteType 合法三种类型生效, 非法值返回 SSAP_INVALID_PARAM 且不改变原值
 * @tc.type: FUNC
 */
HWTEST_F(SsapPropertyFrameworkTest, PropertySetWriteType001, TestSize.Level1)
{
    UUID propUuid = UUID::FromString(PROP_UUID_STRING);
    SsapProperty prop(TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_SUCCESS),
        prop.SetWriteType(SsapProperty::PROPERTY_WRITE_CMD));
    EXPECT_EQ(SsapProperty::PROPERTY_WRITE_CMD, prop.GetWriteType());
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_SUCCESS),
        prop.SetWriteType(SsapProperty::PROPERTY_WRITE_REQ));
    EXPECT_EQ(SsapProperty::PROPERTY_WRITE_REQ, prop.GetWriteType());
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_SUCCESS),
        prop.SetWriteType(SsapProperty::PROPERTY_WRITE_DEFAULT));
    EXPECT_EQ(SsapProperty::PROPERTY_WRITE_DEFAULT, prop.GetWriteType());
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_INVALID_PARAM), prop.SetWriteType(0x99));
    EXPECT_EQ(SsapProperty::PROPERTY_WRITE_DEFAULT, prop.GetWriteType());
}

/**
 * @tc.name: PropertySetOperationIndication001
 * @tc.desc: SetOperationIndication 覆盖构造值, SetServiceUuid 往返一致
 * @tc.type: FUNC
 */
HWTEST_F(SsapPropertyFrameworkTest, PropertySetOperationIndication001, TestSize.Level1)
{
    UUID propUuid = UUID::FromString(PROP_UUID_STRING);
    UUID svcUuid = UUID::FromString(SVC_UUID_STRING);
    SsapProperty prop(TEST_PROP_TYPE, propUuid, TEST_OPER_IND, TEST_PROP_PERM);
    constexpr uint32_t newOperInd = 0x20;
    prop.SetOperationIndication(newOperInd);
    EXPECT_EQ(newOperInd, prop.GetOperationIndication());
    prop.SetServiceUuid(svcUuid);
    EXPECT_TRUE(prop.GetServiceUuid().Equals(svcUuid));
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
