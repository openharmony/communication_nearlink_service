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
#include "nearlink_safe_hashmap.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Nearlink {
namespace TEST {
class NearlinkSafeHashMapTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        HILOGI("SetUpTestCase start");
    }

    static void TearDownTestCase()
    {
        HILOGI("TearDownTestCase start");
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

struct DeviceHashMapTest {
    DeviceHashMapTest(uint8_t transportIn, std::string addrIn) : transport(transportIn), addr(addrIn)
    {}
    uint8_t transport;
    std::string addr;
};

/**
 * @tc.name: TestInsert001
 * @tc.desc: Verify Insert function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSafeHashMapTest, TestInsert001, TestSize.Level1)
{
    HILOGI("TestInsert001 start");
    NearlinkSafeHashMap<int32_t, std::shared_ptr<DeviceHashMapTest>> testMap;
    EXPECT_EQ(0, testMap.Size());
    EXPECT_EQ(true, testMap.IsEmpty());

    EXPECT_EQ(true, testMap.Insert(1, std::make_shared<DeviceHashMapTest>(1, "11:22:33:44:55:66")));
    EXPECT_EQ(true, testMap.Insert(2, std::make_shared<DeviceHashMapTest>(2, "AA:BB:CC:DD:EE:FF")));
    EXPECT_EQ(2, testMap.Size());
    EXPECT_EQ(false, testMap.IsEmpty());

    // Duplicate key.
    EXPECT_EQ(false, testMap.Insert(1, std::make_shared<DeviceHashMapTest>(0xFF, "22:22:33:44:55:66")));
    EXPECT_EQ(2, testMap.Size());
    std::shared_ptr<DeviceHashMapTest> dev = nullptr;
    EXPECT_EQ(true, testMap.GetValue(1, dev));
    EXPECT_EQ(1, dev->transport);

    // EnsureInsert
    testMap.EnsureInsert(1, std::make_shared<DeviceHashMapTest>(0xFF, "22:22:33:44:55:66"));
    EXPECT_EQ(2, testMap.Size());
    dev = nullptr;
    EXPECT_EQ(true, testMap.GetValue(1, dev));
    EXPECT_EQ(0xFF, dev->transport);

    // GetValue
    dev = nullptr;
    EXPECT_EQ(true, testMap.GetValue(1, dev));
    EXPECT_EQ(0xFF, dev->transport);

    dev = nullptr;
    EXPECT_EQ(false, testMap.GetValue(3, dev));
    EXPECT_EQ(true, dev == nullptr);
    HILOGI("TestInsert001 end");
}

/**
 * @tc.name: TestFind001
 * @tc.desc: Verify find function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSafeHashMapTest, TestFind001, TestSize.Level1)
{
    HILOGI("TestFind001 start");
    NearlinkSafeHashMap<int32_t, std::shared_ptr<DeviceHashMapTest>> testMap;

    EXPECT_EQ(true, testMap.Insert(1, std::make_shared<DeviceHashMapTest>(1, "11:22:33:44:55:66")));
    EXPECT_EQ(true, testMap.Insert(2, std::make_shared<DeviceHashMapTest>(0xFF, "22:22:33:44:55:66")));

    // Find
    std::shared_ptr<DeviceHashMapTest> dev = nullptr;
    uint8_t realKey = 2;
    bool ret = testMap.Find([&dev](int32_t key, std::shared_ptr<DeviceHashMapTest> val) -> bool {
        if (val && val->transport == 0xFF) {
            dev = val;
            return true;
        }
        return false;
    });
    EXPECT_EQ(true, ret);
    EXPECT_EQ(true, testMap.FindIf(realKey));

    EXPECT_STREQ("22:22:33:44:55:66", dev->addr.c_str());

    dev = nullptr;
    realKey = 3;
    ret = testMap.Find([&dev](int32_t key, std::shared_ptr<DeviceHashMapTest> val) -> bool {
        if (val && val->transport == 3) {
            dev = val;
            return true;
        }
        return false;
    });
    EXPECT_EQ(false, ret);
    EXPECT_EQ(false, testMap.FindIf(realKey));
    EXPECT_EQ(true, dev == nullptr);

    // Erase
    EXPECT_EQ(2, testMap.Size());
    testMap.Erase(1);
    EXPECT_EQ(1, testMap.Size());

    // Clear
    testMap.Clear();
    EXPECT_EQ(0, testMap.Size());
    HILOGI("TestFind001 end");
}

/**
 * @tc.name: TestFind002
 * @tc.desc: Verify find function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSafeHashMapTest, TestFind002, TestSize.Level1)
{
    HILOGI("TestFind002 start");
    NearlinkSafeHashMap<int32_t, std::shared_ptr<DeviceHashMapTest>> testMap;

    EXPECT_EQ(true, testMap.Insert(1, std::make_shared<DeviceHashMapTest>(1, "11:22:33:44:55:66")));
    EXPECT_EQ(true, testMap.Insert(2, std::make_shared<DeviceHashMapTest>(0xFF, "22:22:33:44:55:66")));

    std::shared_ptr<DeviceHashMapTest> dev = nullptr;
    uint8_t realKey = 2;
    auto updateFunc = [&dev](int32_t key, std::shared_ptr<DeviceHashMapTest> val) -> bool {
        if (val && val->transport == 0xFF) {
            val->addr = "11:22:33:44:55:00";
            dev = val;
            return true;
        }
        return false;
    };
    auto createValueFunc = [&dev]() -> std::shared_ptr<DeviceHashMapTest> {
        dev = std::make_shared<DeviceHashMapTest>(0xAA, "11:22:33:44:55:00");
        return dev;
    };

    // FindAndEnsureUpdate
    bool ret = testMap.FindAndEnsureUpdate(realKey, updateFunc, createValueFunc);
    EXPECT_EQ(true, ret);
    EXPECT_STREQ("11:22:33:44:55:00", dev->addr.c_str());

    dev = nullptr;
    realKey =0xAA;

    ret = testMap.FindAndEnsureUpdate(realKey, updateFunc, createValueFunc);
    EXPECT_EQ(false, ret);
    EXPECT_STREQ("11:22:33:44:55:00", dev->addr.c_str());
    HILOGI("TestFind002 end");
}

/**
 * @tc.name: TestIterate001
 * @tc.desc: Verify Iterate function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSafeHashMapTest, TestIterate001, TestSize.Level1)
{
    HILOGI("TestIterate001 start");
    NearlinkSafeHashMap<int32_t, std::shared_ptr<DeviceHashMapTest>> testMap;

    EXPECT_EQ(true, testMap.Insert(1, std::make_shared<DeviceHashMapTest>(1, "11:22:33:44:55:66")));
    EXPECT_EQ(true, testMap.Insert(2, std::make_shared<DeviceHashMapTest>(0xFF, "22:22:33:44:55:66")));

    // Iterate
    std::vector<std::string> vec;
    testMap.Iterate([&vec](int32_t key, std::shared_ptr<DeviceHashMapTest> val) -> void {
        if (val) {
            vec.push_back(val->addr);
        }
    });
    EXPECT_EQ(2, vec.size());
    EXPECT_NE(vec.end(), std::find(vec.begin(), vec.end(), "11:22:33:44:55:66"));
    EXPECT_NE(vec.end(), std::find(vec.begin(), vec.end(), "22:22:33:44:55:66"));

    // Erase
    EXPECT_EQ(2, testMap.Size());
    testMap.Erase(1);
    EXPECT_EQ(1, testMap.Size());

    // Clear
    testMap.Clear();
    EXPECT_EQ(0, testMap.Size());
    HILOGI("TestIterate001 end");
}

/**
 * @tc.name: TestIterate002
 * @tc.desc: Verify Iterate function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSafeHashMapTest, TestIterate002, TestSize.Level1)
{
    HILOGI("TestIterate002 start");
    NearlinkSafeHashMap<int32_t, std::shared_ptr<DeviceHashMapTest>> testMap;

    EXPECT_EQ(true, testMap.Insert(1, std::make_shared<DeviceHashMapTest>(1, "11:22:33:44:55:66")));
    EXPECT_EQ(true, testMap.Insert(2, std::make_shared<DeviceHashMapTest>(0xFF, "22:22:33:44:55:66")));
    EXPECT_EQ(true, testMap.Insert(3, std::make_shared<DeviceHashMapTest>(0x66, "11:22:33:44:55:66")));

    // IterateAndRmv
    std::vector<std::string> vec;
    testMap.IterateAndRmv([&vec](int32_t key, std::shared_ptr<DeviceHashMapTest> val) -> bool {
        if (val && val->addr == "11:22:33:44:55:66") {
            return true;
        }
        vec.push_back(val->addr);
        return false;
    });
    EXPECT_EQ(1, testMap.Size());
    if (testMap.Size() == 1) {
        EXPECT_STREQ("22:22:33:44:55:66", vec[0].c_str());
    }

    // Clear
    testMap.Clear();
    EXPECT_EQ(0, testMap.Size());
    HILOGI("TestIterate002 end");
}

/**
 * @tc.name: TestGetValueAndOpt001
 * @tc.desc: Verify GetValue function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSafeHashMapTest, TestGetValueAndOpt001, TestSize.Level1)
{
    HILOGI("TestGetValueAndOpt001 start");
    NearlinkSafeHashMap<int32_t, std::shared_ptr<DeviceHashMapTest>> testMap;

    EXPECT_EQ(true, testMap.Insert(1, std::make_shared<DeviceHashMapTest>(1, "11:22:33:44:55:66")));
    EXPECT_EQ(true, testMap.Insert(2, std::make_shared<DeviceHashMapTest>(0xFF, "22:22:33:44:55:66")));
    EXPECT_EQ(true, testMap.Insert(3, std::make_shared<DeviceHashMapTest>(0x66, "11:22:33:44:55:66")));

    std::string remoteName;
    auto func = [&remoteName](int32_t key, std::shared_ptr<DeviceHashMapTest> val) -> void {
        val->addr = "11:22:33:44:55:00";
        remoteName = val->addr;
    };

    // GetValueAndOpt
    bool ret = testMap.GetValueAndOpt(1, func);
    EXPECT_EQ(true, ret);
    if (ret) {
        EXPECT_STREQ("11:22:33:44:55:00", remoteName.c_str());
    }

    remoteName = "";
    ret = testMap.GetValueAndOpt(0xAA, func);
    EXPECT_EQ(false, ret);
    if (ret) {
        EXPECT_STREQ("", remoteName.c_str());
    }
    HILOGI("TestGetValueAndOpt001 end");
}
}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS