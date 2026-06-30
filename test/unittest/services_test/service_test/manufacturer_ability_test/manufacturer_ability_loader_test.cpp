/*
 * Copyright (C) 2026 Huawei Device Co., Ltd. All rights reserved.
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
#include <array>
#include <cstdint>
#include <thread>
#include <vector>

#include "ManufacturerAbilityLoader.h"
#include "IManufacturerAbilityManager.h"
#include "nearlink_def.h"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

constexpr int DELAY_LITTLE_MS = 100;

class ManufacturerAbilityLoaderTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: LoadSuccess001
 * @tc.desc: Test dynamic library load success
 * @tc.type: FUNC
 */
HWTEST_F(ManufacturerAbilityLoaderTest, LoadSuccess001, TestSize.Level1)
{
    HILOGI("LoadSuccess001 start");
    auto& loader = ManufacturerAbilityLoader::GetInstance();
    loader.Load();
    HILOGI("LoadSuccess001 end");
}

/**
 * @tc.name: UnloadSuccess002
 * @tc.desc: Test dynamic library unload success
 * @tc.type: FUNC
 */
HWTEST_F(ManufacturerAbilityLoaderTest, UnloadSuccess002, TestSize.Level1)
{
    HILOGI("UnloadSuccess002 start");
    auto& loader = ManufacturerAbilityLoader::GetInstance();
    loader.Load();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    loader.Unload();
    HILOGI("UnloadSuccess002 end");
}

/**
 * @tc.name: GetLocalAbilitySuccess003
 * @tc.desc: Test get local ability success
 * @tc.type: FUNC
 */
HWTEST_F(ManufacturerAbilityLoaderTest, GetLocalAbilitySuccess003, TestSize.Level1)
{
    HILOGI("GetLocalAbilitySuccess003 start");
    auto& loader = ManufacturerAbilityLoader::GetInstance();
    loader.Load();

    auto ability = loader.GetLocalAbility();
    EXPECT_TRUE(std::any_of(ability.begin(), ability.end(),
        [](uint8_t val) { return val != 0; }));
    HILOGI("GetLocalAbilitySuccess003 end");
}

/**
 * @tc.name: GetLocalAbilityEmpty004
 * @tc.desc: Test get local ability returns empty array when not loaded
 * @tc.type: FUNC
 */
HWTEST_F(ManufacturerAbilityLoaderTest, GetLocalAbilityEmpty004, TestSize.Level1)
{
    HILOGI("GetLocalAbilityEmpty004 start");
    auto& loader = ManufacturerAbilityLoader::GetInstance();
    loader.Unload();
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> expectedEmpty = {0};
    auto ability = loader.GetLocalAbility();

    EXPECT_EQ(ability, expectedEmpty);

    HILOGI("GetLocalAbilityEmpty004 end");
}

/**
 * @tc.name: FilterAbilitySuccess005
 * @tc.desc: Test filter ability success
 * @tc.type: FUNC
 */
HWTEST_F(ManufacturerAbilityLoaderTest, FilterAbilitySuccess005, TestSize.Level1)
{
    HILOGI("FilterAbilitySuccess005 start");
    auto& loader = ManufacturerAbilityLoader::GetInstance();
    loader.Load();

    std::array<uint8_t, SLE_MANU_ABILITY_LEN> deviceAbility = {0};
    for (uint8_t i = 0; i < SLE_MANU_ABILITY_LEN; ++i) {
        deviceAbility[i] = 0xFF;
    }

    loader.FilterAbility(deviceAbility);

    EXPECT_TRUE(std::any_of(deviceAbility.begin(), deviceAbility.end(),
        [](uint8_t val) { return val != 0xFF; }));

    HILOGI("FilterAbilitySuccess005 end");
}

/**
 * @tc.name: CheckAbilitySuccess006
 * @tc.desc: Test check ability success
 * @tc.type: FUNC
 */
HWTEST_F(ManufacturerAbilityLoaderTest, CheckAbilitySuccess006, TestSize.Level1)
{
    HILOGI("CheckAbilitySuccess006 start");
    auto& loader = ManufacturerAbilityLoader::GetInstance();
    loader.Load();

    bool result = loader.CheckAbility(0);
    EXPECT_TRUE(result);

    HILOGI("CheckAbilitySuccess006 end");
}

/**
 * @tc.name: GetAbilityIndexSuccess007
 * @tc.desc: Test get ability index success using MANU_ABILITY_ constants
 * @tc.type: FUNC
 */
HWTEST_F(ManufacturerAbilityLoaderTest, GetAbilityIndexSuccess007, TestSize.Level1)
{
    HILOGI("GetAbilityIndexSuccess007 start");
    auto& loader = ManufacturerAbilityLoader::GetInstance();
    loader.Load();

    int qosIndex = loader.GetAbilityIndex(MANU_ABILITY_QOSID_CONFIG);
    EXPECT_GE(qosIndex, 0);

    int fenceIndex = loader.GetAbilityIndex(MANU_ABILITY_FENCE_5G);
    EXPECT_GE(fenceIndex, 0);

    int adaptiveIndex = loader.GetAbilityIndex(MANU_ABILITY_ADAPTIVE_SWITCH_5G);
    EXPECT_GE(adaptiveIndex, 0);

    int ascMergeIndex = loader.GetAbilityIndex(MANU_ABILITY_ASC_START_PLAYING_MERGE);
    EXPECT_GE(ascMergeIndex, 0);

    int dualEarIndex = loader.GetAbilityIndex(MANU_ABILITY_DUAL_EAR_HIGH_QUALITY_RECORDING);
    EXPECT_GE(dualEarIndex, 0);

    HILOGI("GetAbilityIndexSuccess007 end");
}

/**
 * @tc.name: GetAbilityIndexFail008
 * @tc.desc: Test get ability index fail with invalid name
 * @tc.type: FUNC
 */
HWTEST_F(ManufacturerAbilityLoaderTest, GetAbilityIndexFail008, TestSize.Level1)
{
    HILOGI("GetAbilityIndexFail008 start");
    auto& loader = ManufacturerAbilityLoader::GetInstance();
    loader.Load();

    int invalidIndex = loader.GetAbilityIndex("INVALID_ABILITY_NAME");
    EXPECT_EQ(invalidIndex, -1);

    HILOGI("GetAbilityIndexFail008 end");
}

/**
 * @tc.name: SingletonTest009
 * @tc.desc: Test singleton pattern correctness
 * @tc.type: FUNC
 */
HWTEST_F(ManufacturerAbilityLoaderTest, SingletonTest009, TestSize.Level1)
{
    HILOGI("SingletonTest009 start");

    auto& loader1 = ManufacturerAbilityLoader::GetInstance();
    auto& loader2 = ManufacturerAbilityLoader::GetInstance();

    EXPECT_EQ(&loader1, &loader2);

    HILOGI("SingletonTest009 end");
}

/**
 * @tc.name: ThreadSafetyTest010
 * @tc.desc: Test thread safety with concurrent calls
 * @tc.type: FUNC
 */
HWTEST_F(ManufacturerAbilityLoaderTest, ThreadSafetyTest010, TestSize.Level1)
{
    HILOGI("ThreadSafetyTest010 start");
    auto& loader = ManufacturerAbilityLoader::GetInstance();
    loader.Load();

    std::vector<std::thread> threads;
    constexpr int THREAD_COUNT = 10;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&loader]() {
            auto ability = loader.GetLocalAbility();
            (void)ability;
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    HILOGI("ThreadSafetyTest010 end");
}

/**
 * @tc.name: LoadUnloadMultipleTimes011
 * @tc.desc: Test load and unload multiple times
 * @tc.type: FUNC
 */
HWTEST_F(ManufacturerAbilityLoaderTest, LoadUnloadMultipleTimes011, TestSize.Level1)
{
    HILOGI("LoadUnloadMultipleTimes011 start");
    auto& loader = ManufacturerAbilityLoader::GetInstance();

    for (int i = 0; i < 3; ++i) {
        loader.Load();
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
        loader.Unload();
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    }

    HILOGI("LoadUnloadMultipleTimes011 end");
}

/**
 * @tc.name: CheckAbilityInvalidIndex012
 * @tc.desc: Test check ability with invalid index
 * @tc.type: FUNC
 */
HWTEST_F(ManufacturerAbilityLoaderTest, CheckAbilityInvalidIndex012, TestSize.Level1)
{
    HILOGI("CheckAbilityInvalidIndex012 start");
    auto& loader = ManufacturerAbilityLoader::GetInstance();
    loader.Load();

    bool result = loader.CheckAbility(255);
    EXPECT_FALSE(result);

    HILOGI("CheckAbilityInvalidIndex012 end");
}

/**
 * @tc.name: FilterAbilityEmptyArray013
 * @tc.desc: Test filter ability with empty array
 * @tc.type: FUNC
 */
HWTEST_F(ManufacturerAbilityLoaderTest, FilterAbilityEmptyArray013, TestSize.Level1)
{
    HILOGI("FilterAbilityEmptyArray013 start");
    auto& loader = ManufacturerAbilityLoader::GetInstance();
    loader.Load();

    std::array<uint8_t, SLE_MANU_ABILITY_LEN> emptyAbility = {0};
    loader.FilterAbility(emptyAbility);

    EXPECT_TRUE(std::all_of(emptyAbility.begin(), emptyAbility.end(),
        [](uint8_t val) { return val == 0; }));

    HILOGI("FilterAbilityEmptyArray013 end");
}

/**
 * @tc.name: GetAbilityIndexAllConstants014
 * @tc.desc: Test get ability index with all defined constants
 * @tc.type: FUNC
 */
HWTEST_F(ManufacturerAbilityLoaderTest, GetAbilityIndexAllConstants014, TestSize.Level1)
{
    HILOGI("GetAbilityIndexAllConstants014 start");
    auto& loader = ManufacturerAbilityLoader::GetInstance();
    loader.Load();

    const std::vector<const char*> abilityNames = {
        MANU_ABILITY_FENCE_5G,
        MANU_ABILITY_ADAPTIVE_SWITCH_5G,
        MANU_ABILITY_QOSID_CONFIG,
        MANU_ABILITY_ASC_START_PLAYING_MERGE,
        MANU_ABILITY_DUAL_EAR_HIGH_QUALITY_RECORDING,
    };

    for (const auto& name : abilityNames) {
        int index = loader.GetAbilityIndex(name);
        EXPECT_GE(index, 0) << "Failed to get index for: " << name;
    }

    HILOGI("GetAbilityIndexAllConstants014 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
