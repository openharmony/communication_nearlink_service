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
 * 设备配置门面测试：单例取用、配置文件缺失时按需创建再加载成功，
 * 以及各类字段经门面写入后能从同一文件读回。
 */

#include <gtest/gtest.h>
#include <string>

#include "AdapterDeviceConfig.h"
#include "SleConfig.h"

namespace {
/* 用例专属配置段落，TearDown 整段删除，避免污染设备上的真实配置 */
constexpr const char *TEST_SECTION = "DmUnitTestSection";
} // namespace

using namespace OHOS::Nearlink;

class AdapterDeviceConfigTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        config_ = AdapterDeviceConfig::GetInstance();
    }

    void TearDown() override
    {
        /* 只清理本用例写入的段落；设备原有配置内容保持不动 */
        (void)config_->RemoveSection(TEST_SECTION);
        (void)SleConfig::GetInstance().Save();
    }

    IAdapterDeviceConfig *config_ = nullptr;
};

/* 单例：两次取用返回同一实例 */
TEST_F(AdapterDeviceConfigTest, GetInstanceIsSingleton)
{
    EXPECT_EQ(AdapterDeviceConfig::GetInstance(), config_);
}

/* 文件不存在时按需创建后加载成功；文件已存在时直接加载成功 */
TEST_F(AdapterDeviceConfigTest, LoadOrCreateThenLoadSucceeds)
{
    if (!config_->Load()) {
        ASSERT_TRUE(config_->CreateFile()) << "config dir not writable on this environment";
        ASSERT_TRUE(config_->Load());
    }
    /* 已加载后再次 Load 仍成功 */
    EXPECT_TRUE(config_->Load());
}

/* 经门面写入整型/字符串/布尔并保存，从同一文件读回原值 */
TEST_F(AdapterDeviceConfigTest, ValueRoundTripThroughFacade)
{
    ASSERT_TRUE(config_->Load() || (config_->CreateFile() && config_->Load()));

    ASSERT_TRUE(config_->SetValue(TEST_SECTION, "count", 32));
    ASSERT_TRUE(config_->SetValue(TEST_SECTION, "memo", std::string("hello")));
    ASSERT_TRUE(config_->SetValue(TEST_SECTION, "enabled", true));
    ASSERT_TRUE(config_->Save());

    int intValue = 0;
    EXPECT_TRUE(config_->GetValue(TEST_SECTION, "count", intValue));
    EXPECT_EQ(intValue, 32);
    std::string strValue;
    EXPECT_TRUE(config_->GetValue(TEST_SECTION, "memo", strValue));
    EXPECT_EQ(strValue, "hello");
    bool boolValue = false;
    EXPECT_TRUE(config_->GetValue(TEST_SECTION, "enabled", boolValue));
    EXPECT_TRUE(boolValue);
}

/* 带子段的写入读回：子段下属性独立存放 */
TEST_F(AdapterDeviceConfigTest, SubSectionValueRoundTrip)
{
    ASSERT_TRUE(config_->Load() || (config_->CreateFile() && config_->Load()));

    ASSERT_TRUE(config_->SetValue(TEST_SECTION, "devA", "name", std::string("earbuds")));
    ASSERT_TRUE(config_->SetValue(TEST_SECTION, "devA", "volume", 11));
    ASSERT_TRUE(config_->Save());

    std::string strValue;
    EXPECT_TRUE(config_->GetValue(TEST_SECTION, "devA", "name", strValue));
    EXPECT_EQ(strValue, "earbuds");
    int intValue = 0;
    EXPECT_TRUE(config_->GetValue(TEST_SECTION, "devA", "volume", intValue));
    EXPECT_EQ(intValue, 11);

    /* 未写入的子段属性读取失败 */
    std::string absent;
    EXPECT_FALSE(config_->GetValue(TEST_SECTION, "devB", "name", absent));
}
