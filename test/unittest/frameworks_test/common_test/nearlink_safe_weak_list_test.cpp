/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "log.h"
#include "nearlink_safe_weak_list.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Nearlink {
namespace TEST {
class NearlinkSafeWeakListTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        HILOGI("SetUpTestCase start");
    }

    static void TearDownTestCase()
    {
        HILOGI("SetUpTestCase start");
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

struct DeviceWeakListTest {
    DeviceWeakListTest(uint8_t transportIn, std::string addrIn) : transport(transportIn), addr(addrIn)
    {}
    uint8_t transport;
    std::string addr;
};

/**
 * @tc.name: Insert001
 * @tc.desc: Verify Insert function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSafeWeakListTest, Insert001, TestSize.Level1)
{
    HILOGI("Insert001 start");
    NearlinkSafeWeakList<DeviceWeakListTest> testList;
    EXPECT_EQ(0, testList.Size());

    // Insert succeeded.
    std::shared_ptr<DeviceWeakListTest> dev = std::make_shared<DeviceWeakListTest>(1, "11:22:33:44:55:66");
    testList.Insert(dev);
    EXPECT_EQ(1, testList.Size());

    // Insert duplicate dev failed.
    auto dev1 = dev;
    testList.Insert(dev1);
    EXPECT_EQ(1, testList.Size());
    HILOGI("Insert001 end");
}

/**
 * @tc.name: Erase001
 * @tc.desc: Verify Erase function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSafeWeakListTest, Erase001, TestSize.Level1)
{
    HILOGI("Erase001 start");
    NearlinkSafeWeakList<DeviceWeakListTest> testList;
    EXPECT_EQ(0, testList.Size());

    // Insert succeeded.
    std::shared_ptr<DeviceWeakListTest> dev = std::make_shared<DeviceWeakListTest>(1, "11:22:33:44:55:66");
    testList.Insert(dev);
    EXPECT_EQ(1, testList.Size());

    // Erase succeeded.
    testList.Erase(dev);
    EXPECT_EQ(0, testList.Size());
    HILOGI("Erase001 end");
}

/**
 * @tc.name: Insert002
 * @tc.desc: Verify Insert function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSafeWeakListTest, Iterate001, TestSize.Level1)
{
    HILOGI("Iterate001 start");
    NearlinkSafeWeakList<DeviceWeakListTest> testList;
    // Insert succeeded.
    std::shared_ptr<DeviceWeakListTest> dev1 = std::make_shared<DeviceWeakListTest>(1, "11:22:33:44:55:66");
    std::shared_ptr<DeviceWeakListTest> dev2 = std::make_shared<DeviceWeakListTest>(2, "22:22:33:44:55:66");
    testList.Insert(dev1);
    testList.Insert(dev2);
    EXPECT_EQ(2, testList.Size());

    // Iterate.
    std::map<uint8_t, std::string> tmpMap;
    testList.Iterate([&tmpMap](std::shared_ptr<DeviceWeakListTest> dev) -> void {
        tmpMap.insert(std::pair<uint8_t, std::string>(dev->transport, dev->addr));
    });
    EXPECT_EQ(2, tmpMap.size());
    EXPECT_EQ(1, tmpMap.count(1));
    EXPECT_EQ(1, tmpMap.count(2));

    // Iterate, expired.
    tmpMap.clear();
    EXPECT_EQ(0, tmpMap.size());
    EXPECT_EQ(1, dev1.use_count());
    //Destroy dev1.
    dev1 = nullptr;
    EXPECT_EQ(0, dev1.use_count());

    testList.Iterate([&tmpMap](std::shared_ptr<DeviceWeakListTest> dev) -> void {
        tmpMap.insert(std::pair<uint8_t, std::string>(dev->transport, dev->addr));
    });
    EXPECT_EQ(1, tmpMap.size());
    EXPECT_EQ(0, tmpMap.count(1));
    EXPECT_EQ(1, tmpMap.count(2));
    HILOGI("Iterate001 end");
}
}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS