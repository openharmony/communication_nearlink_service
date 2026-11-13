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

// 目标: frameworks/native/nearlink_ssap_event.cpp 零覆盖分支
// 覆盖: 拷贝构造与赋值的深拷贝/清空分支, GetParameter 空出参分支

#include <gtest/gtest.h>
#include <cstring>

#include "log.h"
#include "nearlink_uuid.h"
// 编入产品源文件, 使本测试进程直接产出该文件的 gcda
#include "nearlink_ssap_event.cpp"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
const std::string EVT_UUID_STRING = "11223344-0000-1000-8000-00805F9B2222";
const std::string SVC_UUID_STRING = "11223344-0000-1000-8000-00805F9B34FB";
constexpr int TEST_EVT_TYPE = 5;      // eventType_ 测试值
constexpr uint16_t TEST_EVT_HANDLE = 0x0031;
constexpr size_t TEST_PARAM_LEN = 2;
const uint8_t TEST_PARAM[TEST_PARAM_LEN] = {0xA5, 0x5A};
} // namespace

class SsapEventFrameworkTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: EventConstructThreeWays001
 * @tc.desc: 三种构造: (type,uuid)/(handle,type,uuid)/(handle,uuid) 各字段默认与生效值
 * @tc.type: FUNC
 */
HWTEST_F(SsapEventFrameworkTest, EventConstructThreeWays001, TestSize.Level1)
{
    UUID evtUuid = UUID::FromString(EVT_UUID_STRING);
    SsapEvent byType(TEST_EVT_TYPE, evtUuid);
    EXPECT_EQ(0, byType.GetHandle());
    EXPECT_EQ(TEST_EVT_TYPE, byType.GetEventType());
    EXPECT_TRUE(byType.GetUuid().Equals(evtUuid));

    SsapEvent byHandleType(TEST_EVT_HANDLE, TEST_EVT_TYPE, evtUuid);
    EXPECT_EQ(TEST_EVT_HANDLE, byHandleType.GetHandle());
    EXPECT_EQ(TEST_EVT_TYPE, byHandleType.GetEventType());

    SsapEvent byHandle(TEST_EVT_HANDLE, evtUuid);
    EXPECT_EQ(TEST_EVT_HANDLE, byHandle.GetHandle());
    EXPECT_EQ(0, byHandle.GetEventType());
    EXPECT_TRUE(byHandle.GetUuid().Equals(evtUuid));
}

/**
 * @tc.name: EventCopyConstructWithParameter001
 * @tc.desc: 拷贝构造深拷贝分支(parameter_ 非空且 length_ 非零)
 * @tc.type: FUNC
 */
HWTEST_F(SsapEventFrameworkTest, EventCopyConstructWithParameter001, TestSize.Level1)
{
    UUID evtUuid = UUID::FromString(EVT_UUID_STRING);
    SsapEvent src(TEST_EVT_HANDLE, TEST_EVT_TYPE, evtUuid);
    src.SetParameter(TEST_PARAM, TEST_PARAM_LEN);
    SsapEvent dst(src);
    EXPECT_EQ(TEST_EVT_HANDLE, dst.GetHandle());
    EXPECT_EQ(TEST_EVT_TYPE, dst.GetEventType());
    EXPECT_TRUE(dst.GetUuid().Equals(evtUuid));
    size_t size = 0;
    const auto &param = dst.GetParameter(&size);
    ASSERT_NE(nullptr, param.get());
    EXPECT_EQ(TEST_PARAM_LEN, size);
    EXPECT_EQ(0, std::memcmp(param.get(), TEST_PARAM, TEST_PARAM_LEN));
    // 深拷贝: 副本与源不是同一块缓冲
    size_t srcSize = 0;
    EXPECT_NE(src.GetParameter(&srcSize).get(), param.get());
}

/**
 * @tc.name: EventCopyConstructEmpty002
 * @tc.desc: 拷贝构造清空分支(parameter_ 为空), 副本无参数
 * @tc.type: FUNC
 */
HWTEST_F(SsapEventFrameworkTest, EventCopyConstructEmpty002, TestSize.Level1)
{
    UUID evtUuid = UUID::FromString(EVT_UUID_STRING);
    SsapEvent src(TEST_EVT_TYPE, evtUuid);
    SsapEvent dst(src);
    size_t size = 1;
    const auto &param = dst.GetParameter(&size);
    EXPECT_EQ(nullptr, param.get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: EventAssignSelf001
 * @tc.desc: 赋值自赋值分支(this == &src), 不改变自身
 * @tc.type: FUNC
 */
HWTEST_F(SsapEventFrameworkTest, EventAssignSelf001, TestSize.Level1)
{
    UUID evtUuid = UUID::FromString(EVT_UUID_STRING);
    SsapEvent evt(TEST_EVT_HANDLE, TEST_EVT_TYPE, evtUuid);
    evt.SetParameter(TEST_PARAM, TEST_PARAM_LEN);
    const SsapEvent *srcPtr = &evt;
    evt = *srcPtr; // 自赋值
    size_t size = 0;
    const auto &param = evt.GetParameter(&size);
    ASSERT_NE(nullptr, param.get());
    EXPECT_EQ(TEST_PARAM_LEN, size);
    EXPECT_EQ(TEST_EVT_HANDLE, evt.GetHandle());
}

/**
 * @tc.name: EventAssignWithParameter002
 * @tc.desc: 赋值深拷贝分支: 目标原为空, 源带参数
 * @tc.type: FUNC
 */
HWTEST_F(SsapEventFrameworkTest, EventAssignWithParameter002, TestSize.Level1)
{
    UUID evtUuid = UUID::FromString(EVT_UUID_STRING);
    SsapEvent src(TEST_EVT_HANDLE, TEST_EVT_TYPE, evtUuid);
    src.SetParameter(TEST_PARAM, TEST_PARAM_LEN);
    SsapEvent dst(1, evtUuid);
    dst = src;
    size_t size = 0;
    const auto &param = dst.GetParameter(&size);
    ASSERT_NE(nullptr, param.get());
    EXPECT_EQ(TEST_PARAM_LEN, size);
    EXPECT_EQ(0, std::memcmp(param.get(), TEST_PARAM, TEST_PARAM_LEN));
    EXPECT_EQ(TEST_EVT_HANDLE, dst.GetHandle());
}

/**
 * @tc.name: EventAssignEmptyOverParameter003
 * @tc.desc: 赋值清空分支: 目标带参数, 源为空, 目标被重置
 * @tc.type: FUNC
 */
HWTEST_F(SsapEventFrameworkTest, EventAssignEmptyOverParameter003, TestSize.Level1)
{
    UUID evtUuid = UUID::FromString(EVT_UUID_STRING);
    SsapEvent dst(TEST_EVT_TYPE, evtUuid);
    dst.SetParameter(TEST_PARAM, TEST_PARAM_LEN);
    SsapEvent src(2, evtUuid);
    dst = src;
    size_t size = 1;
    const auto &param = dst.GetParameter(&size);
    EXPECT_EQ(nullptr, param.get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: EventGetParameterWithNullSize001
 * @tc.desc: GetParameter 空出参分支(size == nullptr 不解引用, 不崩溃)
 * @tc.type: FUNC
 */
HWTEST_F(SsapEventFrameworkTest, EventGetParameterWithNullSize001, TestSize.Level1)
{
    UUID evtUuid = UUID::FromString(EVT_UUID_STRING);
    SsapEvent evt(TEST_EVT_HANDLE, TEST_EVT_TYPE, evtUuid);
    evt.SetParameter(TEST_PARAM, TEST_PARAM_LEN);
    const auto &param = evt.GetParameter(nullptr);
    ASSERT_NE(nullptr, param.get());
    EXPECT_EQ(0, std::memcmp(param.get(), TEST_PARAM, TEST_PARAM_LEN));
}

/**
 * @tc.name: EventSetParameterReject001
 * @tc.desc: SetParameter 空指针/零长度拒绝分支, 原值保持
 * @tc.type: FUNC
 */
HWTEST_F(SsapEventFrameworkTest, EventSetParameterReject001, TestSize.Level1)
{
    UUID evtUuid = UUID::FromString(EVT_UUID_STRING);
    SsapEvent evt(TEST_EVT_TYPE, evtUuid);
    evt.SetParameter(nullptr, TEST_PARAM_LEN);
    evt.SetParameter(TEST_PARAM, 0);
    size_t size = 1;
    const auto &param = evt.GetParameter(&size);
    EXPECT_EQ(nullptr, param.get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.name: EventSetUuidAndServiceUuid001
 * @tc.desc: SetUuid/SetServiceUuid 与 getter 往返一致
 * @tc.type: FUNC
 */
HWTEST_F(SsapEventFrameworkTest, EventSetUuidAndServiceUuid001, TestSize.Level1)
{
    UUID evtUuid = UUID::FromString(EVT_UUID_STRING);
    UUID svcUuid = UUID::FromString(SVC_UUID_STRING);
    SsapEvent evt(TEST_EVT_TYPE, evtUuid);
    evt.SetUuid(svcUuid);
    EXPECT_TRUE(evt.GetUuid().Equals(svcUuid));
    evt.SetServiceUuid(svcUuid);
    EXPECT_TRUE(evt.GetServiceUuid().Equals(svcUuid));
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
