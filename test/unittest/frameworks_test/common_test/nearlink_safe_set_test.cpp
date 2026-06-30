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
#include "nearlink_safe_set.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Nearlink {
namespace TEST {
class NearlinkSafeSetTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        HILOGI("SetUpTestCase NearlinkSafeSetTest start");
    }

    static void TearDownTestCase()
    {
        HILOGI("SetUpTestCase NearlinkSafeSetTest start");
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

struct TestKeyType {
public:
    TestKeyType() {}
    explicit TestKeyType(int input1) : key1(input1) {}
    TestKeyType(int input1, uint32_t input2) : key1(input1), key2(input2) {}
    TestKeyType(int input1, uint32_t input2, const string& input3) : key1(input1), key2(input2), key3(input3) {}

    bool operator < (const TestKeyType& other) const
    {
        if (key1 != other.key1) {
            return key1 < other.key1;
        }
        if (key2 != other.key2) {
            return key2 < other.key2;
        }
        return key3 < other.key3;
    }

    bool operator == (const TestKeyType& other) const
    {
        return (key1 == other.key1) && (key2 == other.key2) && (key3 == other.key3);
    }

    int key1 = 0;
    uint32_t key2 = 0;
    //  Variable-length data, which is empty at the beginning and does not need to be assigned a value.
    std::string key3;
};

HWTEST_F(NearlinkSafeSetTest, TestNearlinkSafeSetEmplace, TestSize.Level1)
{
    HILOGI("TestNearlinkSafeSetEmplace start");
    NearlinkSafeSet<TestKeyType> safeSet;
    EXPECT_TRUE(safeSet.Empty());
    EXPECT_TRUE(safeSet.Emplace(1));
    EXPECT_TRUE(safeSet.Emplace(0, 2));
    EXPECT_TRUE(safeSet.Emplace(-2, 4, "teststring"));
    EXPECT_EQ(safeSet.Size(), 3);

    TestKeyType key(0, 2);
    EXPECT_EQ(safeSet.Count(key), 1);
    key.key1 = 5;
    EXPECT_EQ(safeSet.Count(key), 0);
    EXPECT_TRUE(safeSet.Emplace(key));
    EXPECT_EQ(safeSet.Count(key), 1);
    EXPECT_EQ(safeSet.Size(), 4);
}

HWTEST_F(NearlinkSafeSetTest, TestNearlinkSafeSetInsert, TestSize.Level1)
{
    HILOGI("TestNearlinkSafeSetInsert start");
    NearlinkSafeSet<TestKeyType> safeSet;
    TestKeyType key(1, 2);
    EXPECT_TRUE(safeSet.Insert(key));
    EXPECT_EQ(safeSet.Size(), 1);
    EXPECT_EQ(safeSet.Count(key), 1);
}

HWTEST_F(NearlinkSafeSetTest, TestNearlinkSafeSetErase, TestSize.Level1)
{
    HILOGI("TestNearlinkSafeSetErase start");
    NearlinkSafeSet<TestKeyType> safeSet;
    EXPECT_TRUE(safeSet.Emplace(5, 4, "teststring1"));
    EXPECT_TRUE(safeSet.Emplace(-2, 7, "teststring2"));
    EXPECT_TRUE(safeSet.Emplace(3, 4, "teststring3"));
    EXPECT_EQ(safeSet.Size(), 3);
    TestKeyType key(3, 4, "teststring3");
    EXPECT_TRUE(safeSet.Erase(key));
    // Deduplication
    EXPECT_FALSE(safeSet.Erase(key));
    key.key1 = 0;
    // Erase a value that does not exist.
    EXPECT_FALSE(safeSet.Erase(key));
    EXPECT_EQ(safeSet.Size(), 2);
}

struct DecrementCmp {
public:
    bool operator()(const TestKeyType& a, const TestKeyType& b) const
    {
        if (a.key1 != b.key1) {
            return a.key1 > b.key1;
        }
        if (a.key2 != b.key2) {
            return a.key2 > b.key2;
        }
        return a.key3 > b.key3;
    }
};

HWTEST_F(NearlinkSafeSetTest, TestNearlinkSafeSetForEach, TestSize.Level1)
{
    HILOGI("TestNearlinkSafeSetForEach start");
    NearlinkSafeSet<TestKeyType> safeSet;
    NearlinkSafeSet<TestKeyType, DecrementCmp> safeSet2;
    EXPECT_TRUE(safeSet.Emplace(0, 4, "teststring1"));
    EXPECT_TRUE(safeSet.Emplace(1, 7, "teststring2"));
    EXPECT_TRUE(safeSet.Emplace(2, 4, "teststring3"));

    auto func1 = [](uint32_t idx, const TestKeyType& key) {
        EXPECT_EQ(key.key1, idx);
    };
    auto func2 = [](uint32_t idx, const TestKeyType& key) {
        EXPECT_EQ(key.key1, 2 - idx);
    };
    auto transferFunc = [&safeSet2](const TestKeyType& key) {
        safeSet2.Emplace(key);
    };
    safeSet.ForEach(func1);
    safeSet.ForEach(transferFunc);
    EXPECT_EQ(safeSet2.Size(), 3);
    safeSet2.ForEach(func2);
}
}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS