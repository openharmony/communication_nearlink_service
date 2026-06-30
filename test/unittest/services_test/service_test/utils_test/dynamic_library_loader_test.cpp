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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include "DynamicLibraryLoader.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Nearlink {
static const char *HIBOX_LIB_NAME = "libnearlink_hibox.z.so";
static const char *HIBOX_LIB_FUNC_NAME = "HiboxRecvIndMsg";

#define CHECK_DLL_SM_STATE(sm, expect) EXPECT_EQ((sm).GetState()->Name(), std::string(expect))

class DynamicLibraryLoaderTest : public testing::Test {
public:
    DynamicLibraryLoaderTest(void) = default;
    ~DynamicLibraryLoaderTest() override = default;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void DynamicLibraryLoaderTest::SetUpTestCase(void)
{}

void DynamicLibraryLoaderTest::TearDownTestCase(void)
{}

void DynamicLibraryLoaderTest::SetUp(void)
{}

void DynamicLibraryLoaderTest::TearDown(void)
{}

/**
 * @tc.name: DynamicLibraryLoaderTest001
 * @tc.desc: load success
 * @tc.type: FUNC
 */
HWTEST_F(DynamicLibraryLoaderTest, DynamicLibraryLoaderTest001, TestSize.Level0)
{
    HILOGI("DynamicLibraryLoaderTest001 enter");

    DynamicLibraryLoader loader(HIBOX_LIB_NAME);
    CHECK_DLL_SM_STATE(loader, DLL_UNLOAD_STATE);

    loader.OpenLib();
    CHECK_DLL_SM_STATE(loader, DLL_LOADED_STATE);

    EXPECT_NE(loader.GetSymbol(HIBOX_LIB_FUNC_NAME), nullptr);

    loader.CloseLib();
    CHECK_DLL_SM_STATE(loader, DLL_WAIT_FOR_UNLOAD_STATE);

    HILOGI("DynamicLibraryLoaderTest001 end");
}

/**
 * @tc.name: DynamicLibraryLoaderTest002
 * @tc.desc: load fail
 * @tc.type: FUNC
 */
HWTEST_F(DynamicLibraryLoaderTest, DynamicLibraryLoaderTest002, TestSize.Level0)
{
    HILOGI("DynamicLibraryLoaderTest002 enter");

    DynamicLibraryLoader loader(HIBOX_LIB_NAME);
    loader.OpenLib();
    CHECK_DLL_SM_STATE(loader, DLL_LOADED_STATE);

    EXPECT_EQ(loader.GetSymbol("xxxxx"), nullptr);
    loader.CloseLib();
    CHECK_DLL_SM_STATE(loader, DLL_WAIT_FOR_UNLOAD_STATE);

    HILOGI("DynamicLibraryLoaderTest002 end");
}

/**
 * @tc.name: DynamicLibraryLoaderTest003
 * @tc.desc: Check load times
 * @tc.type: FUNC
 */
HWTEST_F(DynamicLibraryLoaderTest, DynamicLibraryLoaderTest003, TestSize.Level0)
{
    HILOGI("DynamicLibraryLoaderTest003 enter");

    DynamicLibraryLoader loader(HIBOX_LIB_NAME);
    EXPECT_EQ(loader.loadTimes_, 0);

    loader.OpenLib();
    loader.OpenLib();
    loader.OpenLib();
    EXPECT_EQ(loader.loadTimes_, 3);
    CHECK_DLL_SM_STATE(loader, DLL_LOADED_STATE);

    loader.CloseLib();
    EXPECT_EQ(loader.loadTimes_, 2);
    CHECK_DLL_SM_STATE(loader, DLL_LOADED_STATE);

    loader.CloseLib();
    loader.CloseLib();
    EXPECT_EQ(loader.loadTimes_, 0);
    CHECK_DLL_SM_STATE(loader, DLL_WAIT_FOR_UNLOAD_STATE);

    HILOGI("DynamicLibraryLoaderTest003 end");
}

/**
 * @tc.name: DynamicLibraryLoaderTest004
 * @tc.desc: Check delay unload
 * @tc.type: FUNC
 */
HWTEST_F(DynamicLibraryLoaderTest, DynamicLibraryLoaderTest004, TestSize.Level0)
{
    HILOGI("DynamicLibraryLoaderTest004 enter");

    DynamicLibraryLoader loader(HIBOX_LIB_NAME, 100);

    loader.OpenLib();
    CHECK_DLL_SM_STATE(loader, DLL_LOADED_STATE);

    loader.CloseLib();
    CHECK_DLL_SM_STATE(loader, DLL_WAIT_FOR_UNLOAD_STATE);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    CHECK_DLL_SM_STATE(loader, DLL_UNLOAD_STATE);

    HILOGI("DynamicLibraryLoaderTest004 end");
}

/**
 * @tc.name: DynamicLibraryLoaderTest005
 * @tc.desc: Check get symbol
 * @tc.type: FUNC
 */
HWTEST_F(DynamicLibraryLoaderTest, DynamicLibraryLoaderTest005, TestSize.Level0)
{
    HILOGI("DynamicLibraryLoaderTest005 enter");

    DynamicLibraryLoader loader(HIBOX_LIB_NAME);
    EXPECT_EQ(loader.GetSymbol(HIBOX_LIB_FUNC_NAME), nullptr);

    loader.OpenLib();
    EXPECT_NE(loader.GetSymbol(HIBOX_LIB_FUNC_NAME), nullptr);
    EXPECT_NE(loader.GetSymbol(HIBOX_LIB_FUNC_NAME), nullptr);
    EXPECT_EQ(loader.GetSymbol("xxxx"), nullptr);

    HILOGI("DynamicLibraryLoaderTest005 end");
}

/**
 * @tc.name: DynamicLibraryLoaderTest006
 * @tc.desc: Check unexpect event
 * @tc.type: FUNC
 */
HWTEST_F(DynamicLibraryLoaderTest, DynamicLibraryLoaderTest006, TestSize.Level0)
{
    HILOGI("DynamicLibraryLoaderTest006 enter");

    DynamicLibraryLoader loader(HIBOX_LIB_NAME);
    CHECK_DLL_SM_STATE(loader, DLL_UNLOAD_STATE);
    utility::Message msg1(DynamicLibraryLoader::UNLOAD_EVENT);
    loader.ProcessMessage(msg1);

    loader.OpenLib();
    CHECK_DLL_SM_STATE(loader, DLL_LOADED_STATE);
    utility::Message msg2(DynamicLibraryLoader::UNLOAD_TIMER_EVENT);
    loader.ProcessMessage(msg2);

    loader.CloseLib();
    CHECK_DLL_SM_STATE(loader, DLL_WAIT_FOR_UNLOAD_STATE);
    utility::Message msg3(DynamicLibraryLoader::UNLOAD_EVENT);
    loader.ProcessMessage(msg3);

    HILOGI("DynamicLibraryLoaderTest006 end");
}

/**
 * @tc.name: DynamicLibraryLoaderTest007
 * @tc.desc: Check load -> wait for unload -> load
 * @tc.type: FUNC
 */
HWTEST_F(DynamicLibraryLoaderTest, DynamicLibraryLoaderTest007, TestSize.Level0)
{
    HILOGI("DynamicLibraryLoaderTest007 enter");

    DynamicLibraryLoader loader(HIBOX_LIB_NAME);
    CHECK_DLL_SM_STATE(loader, DLL_UNLOAD_STATE);

    loader.OpenLib();
    CHECK_DLL_SM_STATE(loader, DLL_LOADED_STATE);

    loader.CloseLib();
    CHECK_DLL_SM_STATE(loader, DLL_WAIT_FOR_UNLOAD_STATE);

    loader.OpenLib();
    CHECK_DLL_SM_STATE(loader, DLL_LOADED_STATE);

    HILOGI("DynamicLibraryLoaderTest007 end");
}

/**
 * @tc.name: DynamicLibraryLoaderTest008
 * @tc.desc: Check CDynamicLibraryLoader
 * @tc.type: FUNC
 */
HWTEST_F(DynamicLibraryLoaderTest, DynamicLibraryLoaderTest008, TestSize.Level0)
{
    HILOGI("DynamicLibraryLoaderTest008 enter");

    CDynamicLibraryLoader loader(HIBOX_LIB_NAME);
    CHECK_DLL_SM_STATE(loader, DLL_UNLOAD_STATE);

    loader.OpenLib();
    CHECK_DLL_SM_STATE(loader, DLL_LOADED_STATE);

    EXPECT_NE(loader.GetSymbol(HIBOX_LIB_FUNC_NAME), nullptr);

    loader.CloseLib();
    CHECK_DLL_SM_STATE(loader, DLL_WAIT_FOR_UNLOAD_STATE);

    HILOGI("DynamicLibraryLoaderTest008 end");
}

/**
 * @tc.name: DynamicLibraryLoaderTest009
 * @tc.desc: Check CDynamicLibraryLoader
 * @tc.type: FUNC
 */
HWTEST_F(DynamicLibraryLoaderTest, DynamicLibraryLoaderTest009, TestSize.Level0)
{
    HILOGI("DynamicLibraryLoaderTest009 enter");

    CDynamicLibraryLoader loader(HIBOX_LIB_NAME);
    CHECK_DLL_SM_STATE(loader, DLL_UNLOAD_STATE);

    loader.OpenLib();
    loader.LoadTask();

    CHECK_DLL_SM_STATE(loader, DLL_LOADED_STATE);

    EXPECT_NE(loader.GetSymbol(HIBOX_LIB_FUNC_NAME), nullptr);

    loader.CloseLib();
    loader.UnloadTask();
    CHECK_DLL_SM_STATE(loader, DLL_WAIT_FOR_UNLOAD_STATE);
    HILOGI("DynamicLibraryLoaderTest009 end");
}

/**
 * @tc.name: DynamicLibraryLoaderTest010
 * @tc.desc: Check load -> wait for unload -> load
 * @tc.type: FUNC
 */
HWTEST_F(DynamicLibraryLoaderTest, DynamicLibraryLoaderTest010, TestSize.Level0)
{
    HILOGI("DynamicLibraryLoaderTest010 enter");

    DynamicLibraryLoader loader(HIBOX_LIB_NAME);
    CHECK_DLL_SM_STATE(loader, DLL_UNLOAD_STATE);

    loader.OpenLib();
    loader.LoadTask();
    CHECK_DLL_SM_STATE(loader, DLL_LOADED_STATE);

    loader.CloseLib();
    CHECK_DLL_SM_STATE(loader, DLL_WAIT_FOR_UNLOAD_STATE);

    loader.OpenLib();
    loader.UnloadTask();
    CHECK_DLL_SM_STATE(loader, DLL_LOADED_STATE);

    HILOGI("DynamicLibraryLoaderTest010 end");
}

/**
 * @tc.name: DynamicLibraryLoaderTest011
 * @tc.desc: Check unexpect event
 * @tc.type: FUNC
 */
HWTEST_F(DynamicLibraryLoaderTest, DynamicLibraryLoaderTest011, TestSize.Level0)
{
    HILOGI("DynamicLibraryLoaderTest011 enter");

    DynamicLibraryLoader loader(HIBOX_LIB_NAME);
    CHECK_DLL_SM_STATE(loader, DLL_UNLOAD_STATE);
    utility::Message msg1(DynamicLibraryLoader::UNLOAD_EVENT);
    loader.ProcessMessage(msg1);

    loader.OpenLib();
    loader.LoadTask();
    CHECK_DLL_SM_STATE(loader, DLL_LOADED_STATE);
    utility::Message msg2(DynamicLibraryLoader::UNLOAD_TIMER_EVENT);
    loader.ProcessMessage(msg2);

    loader.CloseLib();
    loader.UnloadTask();
    CHECK_DLL_SM_STATE(loader, DLL_WAIT_FOR_UNLOAD_STATE);
    utility::Message msg3(DynamicLibraryLoader::UNLOAD_EVENT);
    loader.ProcessMessage(msg3);

    HILOGI("DynamicLibraryLoaderTest011 end");
}

/**
 * @tc.name: DynamicLibraryLoaderTest012
 * @tc.desc: Check get symbol
 * @tc.type: FUNC
 */
HWTEST_F(DynamicLibraryLoaderTest, DynamicLibraryLoaderTest012, TestSize.Level0)
{
    HILOGI("DynamicLibraryLoaderTest012 enter");

    DynamicLibraryLoader loader(HIBOX_LIB_NAME);
    EXPECT_EQ(loader.GetSymbol(HIBOX_LIB_FUNC_NAME), nullptr);

    loader.OpenLib();
    loader.LoadTask();
    EXPECT_NE(loader.GetSymbol(HIBOX_LIB_FUNC_NAME), nullptr);
    EXPECT_NE(loader.GetSymbol(HIBOX_LIB_FUNC_NAME), nullptr);
    EXPECT_EQ(loader.GetSymbol("xxxx"), nullptr);

    HILOGI("DynamicLibraryLoaderTest012 end");
}

/**
 * @tc.name: DynamicLibraryLoaderTest013
 * @tc.desc: get load times
 * @tc.type: FUNC
 */
HWTEST_F(DynamicLibraryLoaderTest, DynamicLibraryLoaderTest013, TestSize.Level0)
{
    HILOGI("DynamicLibraryLoaderTest013 enter");
    DynamicLibraryLoader loader(HIBOX_LIB_NAME);
    EXPECT_EQ(loader.GetSymbol(HIBOX_LIB_FUNC_NAME), nullptr);
    loader.OpenLib();
    loader.LoadTask();
    EXPECT_NE(loader.GetSymbol(HIBOX_LIB_FUNC_NAME), nullptr);
    EXPECT_NE(loader.GetSymbol(HIBOX_LIB_FUNC_NAME), nullptr);
    EXPECT_EQ(loader.GetSymbol("xxxx"), nullptr);
    loader.IsLibraryLoaded();
    HILOGI("DynamicLibraryLoaderTest013 end");
}

/**
 * @tc.name: DynamicLibraryLoaderTest014
 * @tc.desc: Check unexpect event
 * @tc.type: FUNC
 */
HWTEST_F(DynamicLibraryLoaderTest, DynamicLibraryLoaderTest014, TestSize.Level0)
{
    HILOGI("DynamicLibraryLoaderTest014 enter");

    DynamicLibraryLoader loader(HIBOX_LIB_NAME, 100, [](const ThreadUtilFunc &func) { DoInMcpThread(func); });
    loader.OpenLib();
    CHECK_DLL_SM_STATE(loader, DLL_LOADED_STATE);

    loader.CloseLib();
    CHECK_DLL_SM_STATE(loader, DLL_WAIT_FOR_UNLOAD_STATE);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    CHECK_DLL_SM_STATE(loader, DLL_UNLOAD_STATE);

    HILOGI("DynamicLibraryLoaderTest014 end");
}
}  // namespace Nearlink
}  // namespace OHOS
