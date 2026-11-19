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
 * 测距结果容器测试：拷贝赋值与自赋值、完整性判定逐条件不满足矩阵、
 * 数据追加与重置。产品源码直接编入本测试目标。
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <vector>

#include "log.h"
#include "nearlink_hadm_sounding_result.cpp"

using namespace OHOS::Nearlink;

namespace {
/* 构造一份满足完整性全部条件的结果 */
NearlinkHadmSoundingResult BuildCompleteResult()
{
    NearlinkHadmSoundingResult result;
    result.addr_ = RawAddress("11:22:33:44:55:66");
    result.dutRssi_ = 60;
    result.rtdRssi_ = 55;
    result.AppendDutIData({0x0102});
    result.AppendRtdIData({0x0304});
    result.AppendDutQData({0x0506});
    result.AppendRtdQData({0x0708});
    return result;
}
} // namespace

/* 完整性判定：地址无效、RSSI 为零、IQ 数据缺失逐条件挡下，全满足才通过 */
TEST(NearlinkHadmSoundingTest, IsCompleteDataConditionMatrix)
{
    /* 默认构造：地址无效 */
    NearlinkHadmSoundingResult noAddr = BuildCompleteResult();
    noAddr.addr_ = RawAddress(INVALID_MAC_ADDRESS);
    EXPECT_FALSE(noAddr.IsCompleteData());

    /* 地址有效但主侧 RSSI 为零 */
    NearlinkHadmSoundingResult noDutRssi = BuildCompleteResult();
    noDutRssi.dutRssi_ = 0;
    EXPECT_FALSE(noDutRssi.IsCompleteData());

    /* 对侧 RSSI 为零 */
    NearlinkHadmSoundingResult noRtdRssi = BuildCompleteResult();
    noRtdRssi.rtdRssi_ = 0;
    EXPECT_FALSE(noRtdRssi.IsCompleteData());

    /* I 路数据缺失（地址与 RSSI 均有效） */
    NearlinkHadmSoundingResult noIData;
    noIData.addr_ = RawAddress("11:22:33:44:55:66");
    noIData.dutRssi_ = 60;
    noIData.rtdRssi_ = 55;
    noIData.AppendDutQData({0x0506});
    noIData.AppendRtdQData({0x0708});
    EXPECT_FALSE(noIData.IsCompleteData());

    /* IQ 路数据长度不一致 */
    NearlinkHadmSoundingResult mismatched = BuildCompleteResult();
    mismatched.AppendDutQData({0x090A});
    EXPECT_FALSE(mismatched.IsCompleteData());

    /* 全条件满足 */
    NearlinkHadmSoundingResult complete = BuildCompleteResult();
    EXPECT_TRUE(complete.IsCompleteData());
}

/* 拷贝赋值：自赋值不变，赋值他人后数据一致且互不影响 */
TEST(NearlinkHadmSoundingTest, CopyAssignmentSemantics)
{
    NearlinkHadmSoundingResult source = BuildCompleteResult();
    source.timeStampSn_ = 0x1234;

    NearlinkHadmSoundingResult target;
    target = source;
    EXPECT_EQ(target.GetDutIData(), source.GetDutIData());
    EXPECT_EQ(target.GetRtdQData(), source.GetRtdQData());
    EXPECT_EQ(target.GetDutTof(), source.GetDutTof());
    EXPECT_EQ(target.GetTimeStampSn(), source.GetTimeStampSn());
    EXPECT_TRUE(target.IsCompleteData());

    /* 自赋值安全 */
    NearlinkHadmSoundingResult &self = target;
    target = self;
    EXPECT_TRUE(target.IsCompleteData());

    /* 拷贝构造与源一致 */
    NearlinkHadmSoundingResult copied(source);
    EXPECT_EQ(copied.GetRtdIData(), source.GetRtdIData());
}

/* 追加数据累加；重置回到默认值且完整性判定失效 */
TEST(NearlinkHadmSoundingTest, AppendAccumulatesAndResetRestoresDefaults)
{
    NearlinkHadmSoundingResult result;
    result.AppendDutIData({0x0001});
    result.AppendDutIData({0x0002});
    EXPECT_EQ(result.GetDutIData(), (std::vector<uint16_t>{0x0001, 0x0002}));

    result.Reset();
    EXPECT_TRUE(result.GetDutIData().empty());
    EXPECT_TRUE(result.GetRtdIData().empty());
    EXPECT_TRUE(result.GetDutQData().empty());
    EXPECT_TRUE(result.GetRtdQData().empty());
    EXPECT_EQ(result.GetDutTof(), 0);
    EXPECT_EQ(result.GetTimeStampSn(), 0);
    EXPECT_FALSE(result.IsCompleteData());
}
