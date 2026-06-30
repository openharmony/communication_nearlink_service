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

#include "log.h"
#include "nearlink_safe_list.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Nearlink {
namespace TEST {
class NearlinkSafeListTest : public testing::Test {
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

struct DeviceListTest {
    DeviceListTest(uint8_t transportIn, std::string addrIn) : transport(transportIn), addr(addrIn)
    {}
    uint8_t transport;
    std::string addr;
};

/**
 * @tc.name: Insert001
 * @tc.desc: Verify Insert function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSafeListTest, Insert001, TestSize.Level1)
{
    HILOGI("Insert001 start");
    NearlinkSafeList<std::string> list;
    EXPECT_EQ(0, list.Size());
    std::string test1 {"test1"};
    list.Insert(test1);
    EXPECT_EQ(1, list.Size());

    std::string test2 {"test1"};
    list.Insert(test2);
    EXPECT_EQ(1, list.Size());
    HILOGI("Insert001 end");
}

/**
 * @tc.name: Erase001
 * @tc.desc: Verify Insert function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSafeListTest, Erase001, TestSize.Level1)
{
    HILOGI("Erase001 start");
    NearlinkSafeList<std::string> list;
    EXPECT_EQ(0, list.Size());
    std::string test1 {"test1"};
    list.Insert(test1);
    EXPECT_EQ(1, list.Size());

    std::string test2 = test1;
    list.Erase(test2);
    EXPECT_EQ(true, list.IsEmpty());
    HILOGI("Erase001 end");
}

/**
 * @tc.name: Iterate001
 * @tc.desc: Verify Insert function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSafeListTest, Iterate001, TestSize.Level1)
{
    HILOGI("Iterate001 start");
    NearlinkSafeList<std::string> list;
    EXPECT_EQ(0, list.Size());

    std::string test1 {"test1"};
    std::string test2 {"test2"};
    list.Insert(test1);
    list.Insert(test2);
    EXPECT_EQ(2, list.Size());

    std::string str {""};
    list.Iterate([&str](std::string val) -> void {
        str += val;
    });
    EXPECT_STREQ("test1test2", str.c_str());

    list.Erase("test2");
    list.Iterate([&str](std::string val) -> void {
        str += val;
    });
    EXPECT_STREQ("test1test2test1", str.c_str());
    EXPECT_EQ(false, list.IsEmpty());
    HILOGI("Iterate001 end");
}

/**
 * @tc.name: Find001
 * @tc.desc: Verify Insert function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSafeListTest, Find001, TestSize.Level1)
{
    HILOGI("Find001 start");
    NearlinkSafeList<std::shared_ptr<DeviceListTest>> devList;
    std::string addr1("11:22:33:44:55:66");
    std::string addr2("AA:BB:CC:DD:EE:FF");
    auto dev1 = std::make_shared<DeviceListTest>(1, addr1);
    auto dev2 = std::make_shared<DeviceListTest>(2, addr2);
    devList.Insert(dev1);
    EXPECT_EQ(1, devList.Size());
    devList.Insert(dev2);
    EXPECT_EQ(2, devList.Size());

    // find dev1
    std::shared_ptr<DeviceListTest> dev = nullptr;
    auto func1 = [&dev, &addr1](std::shared_ptr<DeviceListTest> device) -> bool {
        if (device->addr == addr1 && device->transport == 1) {
            dev = device;
            return true;
        }
        return false;
    };
    bool ret1 = devList.Find(func1);
    EXPECT_EQ(true, ret1);
    EXPECT_EQ(dev.get(), dev1.get());

    // find nothing
    auto func2 = [&dev, &addr1](std::shared_ptr<DeviceListTest> device) -> bool {
        if (device->addr == addr1 && device->transport == 2) {
            dev = device;
            return true;
        }
        return false;
    };
    bool ret2 = devList.Find(func2);
    EXPECT_EQ(false, ret2);
    HILOGI("Find001 end");
}
}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS