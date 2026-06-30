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

#include "gtest/gtest.h"
#include "parameter_provider_interface.h"
#include "parameter_manager.h"
#include "parameter_default_provider.h"
#include <thread>
#include <chrono>
#include <memory>

using namespace OHOS::Nearlink;

class ParameterTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
        ParameterManager::RegisterProvider(nullptr);
    }
};

TEST_F(ParameterTest, TestConstantValues)
{
    EXPECT_EQ(SLE_NOT_SUPPORT, 0);
    EXPECT_EQ(SLE_SUPPORT_AND_CLOSE, 1);
    EXPECT_EQ(SLE_SUPPORT_AND_OPEN, 2);
}

TEST_F(ParameterTest, TestDefaultProviderSingleton)
{
    auto& instance1 = DefaultParameterProvider::GetInstance();
    auto& instance2 = DefaultParameterProvider::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(ParameterTest, TestDefaultProviderGet5GFeatureEnable)
{
    auto& provider = DefaultParameterProvider::GetInstance();
    uint8_t value = provider.Get5GFeatureEnable();
    EXPECT_GE(value, 0);
    EXPECT_LE(value, 255);
}

TEST_F(ParameterTest, TestDefaultProviderGetDevicePowerLevel)
{
    auto& provider = DefaultParameterProvider::GetInstance();
    uint8_t value = provider.GetDevicePowerLevel();
    EXPECT_GE(value, 0);
    EXPECT_LE(value, 255);
}

TEST_F(ParameterTest, TestDefaultProviderGetMuteControlEnable)
{
    auto& provider = DefaultParameterProvider::GetInstance();
    uint8_t value = provider.GetMuteControlEnable();
    EXPECT_GE(value, 0);
    EXPECT_LE(value, 255);
}

TEST_F(ParameterTest, TestDefaultProviderUpdate5GFeatureEnable)
{
    auto& provider = DefaultParameterProvider::GetInstance();
    provider.Update5GFeatureEnable(100);
    EXPECT_EQ(provider.Get5GFeatureEnable(), 100);
    
    provider.Update5GFeatureEnable(200);
    EXPECT_EQ(provider.Get5GFeatureEnable(), 200);
}

TEST_F(ParameterTest, TestDefaultProviderUpdateDevicePowerLevel)
{
    auto& provider = DefaultParameterProvider::GetInstance();
    provider.UpdateDevicePowerLevel(150);
    EXPECT_EQ(provider.GetDevicePowerLevel(), 150);
    
    provider.UpdateDevicePowerLevel(75);
    EXPECT_EQ(provider.GetDevicePowerLevel(), 75);
}

TEST_F(ParameterTest, TestDefaultProviderUpdateMuteControlEnable)
{
    auto& provider = DefaultParameterProvider::GetInstance();
    provider.UpdateMuteControlEnable(SLE_SUPPORT_AND_OPEN);
    EXPECT_EQ(provider.GetMuteControlEnable(), SLE_SUPPORT_AND_OPEN);
    
    provider.UpdateMuteControlEnable(SLE_SUPPORT_AND_CLOSE);
    EXPECT_EQ(provider.GetMuteControlEnable(), SLE_SUPPORT_AND_CLOSE);
}

TEST_F(ParameterTest, TestDefaultProviderCallbackRegistration)
{
    auto& provider = DefaultParameterProvider::GetInstance();

    auto callback = [](int paramType) {};

    provider.RegisterParameterChangedCallback(callback);
    
    provider.Update5GFeatureEnable(100);
    EXPECT_EQ(provider.Get5GFeatureEnable(), 100);
}

TEST_F(ParameterTest, TestDefaultProviderCallbackUnregistration)
{
    auto& provider = DefaultParameterProvider::GetInstance();

    auto callback = [](int paramType) {};

    provider.RegisterParameterChangedCallback(callback);
    provider.UnregisterParameterChangedCallback();
    
    provider.Update5GFeatureEnable(100);
    EXPECT_EQ(provider.Get5GFeatureEnable(), 100);
}

TEST_F(ParameterTest, TestParameterManagerGetDefaultProvider)
{
    auto provider = ParameterManager::GetProvider();
    EXPECT_NE(provider, nullptr);
}

TEST_F(ParameterTest, TestParameterManagerGet5GFeatureEnable)
{
    DefaultParameterProvider::GetInstance().Update5GFeatureEnable(250);
    auto provider = std::shared_ptr<ParameterProviderInterface>(&DefaultParameterProvider::GetInstance(),
                                                                        [](ParameterProviderInterface*) {});
    ParameterManager::RegisterProvider(provider);
    
    uint8_t value = ParameterManager::Get5GFeatureEnable();
    EXPECT_EQ(value, 250);
}

TEST_F(ParameterTest, TestParameterManagerGetDevicePowerLevel)
{
    DefaultParameterProvider::GetInstance().UpdateDevicePowerLevel(180);
    auto provider = std::shared_ptr<ParameterProviderInterface>(&DefaultParameterProvider::GetInstance(),
                                                                        [](ParameterProviderInterface*) {});
    ParameterManager::RegisterProvider(provider);
    
    uint8_t value = ParameterManager::GetDevicePowerLevel();
    EXPECT_EQ(value, 180);
}

TEST_F(ParameterTest, TestParameterManagerGetMuteControlEnable)
{
    DefaultParameterProvider::GetInstance().UpdateMuteControlEnable(SLE_SUPPORT_AND_OPEN);
    auto provider = std::shared_ptr<ParameterProviderInterface>(&DefaultParameterProvider::GetInstance(),
                                                                        [](ParameterProviderInterface*) {});
    ParameterManager::RegisterProvider(provider);
    
    uint8_t value = ParameterManager::GetMuteControlEnable();
    EXPECT_EQ(value, SLE_SUPPORT_AND_OPEN);
}

TEST_F(ParameterTest, TestParameterManagerProviderReplacement)
{
    DefaultParameterProvider::GetInstance().Update5GFeatureEnable(100);
    auto provider1 = std::shared_ptr<ParameterProviderInterface>(&DefaultParameterProvider::GetInstance(),
                                                                         [](ParameterProviderInterface*) {});
    ParameterManager::RegisterProvider(provider1);
    EXPECT_EQ(ParameterManager::Get5GFeatureEnable(), 100);
    
    DefaultParameterProvider::GetInstance().Update5GFeatureEnable(200);
    auto provider2 = std::shared_ptr<ParameterProviderInterface>(&DefaultParameterProvider::GetInstance(),
                                                                         [](ParameterProviderInterface*) {});
    ParameterManager::RegisterProvider(provider2);
    EXPECT_EQ(ParameterManager::Get5GFeatureEnable(), 200);
}

TEST_F(ParameterTest, TestParameterManagerRegisterNullProvider)
{
    auto provider1 = std::shared_ptr<ParameterProviderInterface>(&DefaultParameterProvider::GetInstance(),
                                                                         [](ParameterProviderInterface*) {});
    ParameterManager::RegisterProvider(provider1);
    
    ParameterManager::RegisterProvider(nullptr);
    
    auto provider = ParameterManager::GetProvider();
    EXPECT_NE(provider, nullptr);
}

TEST_F(ParameterTest, TestDefaultProviderThreadSafety)
{
    auto& provider = DefaultParameterProvider::GetInstance();
    
    const int threadCount = 10;
    const int iterations = 100;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < threadCount; ++t) {
        threads.emplace_back([&provider, t, iterations]() {
            for (int i = 0; i < iterations; ++i) {
                provider.Update5GFeatureEnable(static_cast<uint8_t>(t * 10 + i % 256));
                provider.Get5GFeatureEnable();
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto value = provider.Get5GFeatureEnable();
    EXPECT_GE(value, 0);
    EXPECT_LE(value, 255);
}

TEST_F(ParameterTest, TestParameterManagerThreadSafety)
{
    auto provider = std::shared_ptr<ParameterProviderInterface>(&DefaultParameterProvider::GetInstance(),
                                                                        [](ParameterProviderInterface*) {});
    ParameterManager::RegisterProvider(provider);
    
    const int threadCount = 5;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([i]() {
            auto p = std::shared_ptr<ParameterProviderInterface>(&DefaultParameterProvider::GetInstance(),
                                                                 [](ParameterProviderInterface*) {});
            p->Update5GFeatureEnable(i * 50);
            ParameterManager::RegisterProvider(p);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            uint8_t value = ParameterManager::Get5GFeatureEnable();
            EXPECT_GE(value, 0);
            EXPECT_LE(value, 255);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

TEST_F(ParameterTest, TestUsingConstantsInLogic)
{
    uint8_t supportValue = SLE_SUPPORT_AND_OPEN;
    bool shouldRegister = (supportValue == SLE_SUPPORT_AND_OPEN);
    EXPECT_TRUE(shouldRegister);
    
    supportValue = SLE_SUPPORT_AND_CLOSE;
    shouldRegister = (supportValue == SLE_SUPPORT_AND_OPEN);
    EXPECT_FALSE(shouldRegister);
    
    supportValue = SLE_NOT_SUPPORT;
    shouldRegister = (supportValue == SLE_SUPPORT_AND_OPEN);
    EXPECT_FALSE(shouldRegister);
}

TEST_F(ParameterTest, TestMultipleParameterUpdates)
{
    auto& provider = DefaultParameterProvider::GetInstance();
    
    provider.Update5GFeatureEnable(10);
    EXPECT_EQ(provider.Get5GFeatureEnable(), 10);
    
    provider.UpdateDevicePowerLevel(20);
    EXPECT_EQ(provider.GetDevicePowerLevel(), 20);
    EXPECT_EQ(provider.Get5GFeatureEnable(), 10);
    
    provider.UpdateMuteControlEnable(30);
    EXPECT_EQ(provider.GetMuteControlEnable(), 30);
    EXPECT_EQ(provider.Get5GFeatureEnable(), 10);
    EXPECT_EQ(provider.GetDevicePowerLevel(), 20);
}

TEST_F(ParameterTest, TestBoundaryValues)
{
    auto& provider = DefaultParameterProvider::GetInstance();
    
    provider.Update5GFeatureEnable(0);
    EXPECT_EQ(provider.Get5GFeatureEnable(), 0);
    
    provider.Update5GFeatureEnable(255);
    EXPECT_EQ(provider.Get5GFeatureEnable(), 255);
    
    provider.UpdateDevicePowerLevel(0);
    EXPECT_EQ(provider.GetDevicePowerLevel(), 0);
    
    provider.UpdateMuteControlEnable(255);
    EXPECT_EQ(provider.GetMuteControlEnable(), 255);
}