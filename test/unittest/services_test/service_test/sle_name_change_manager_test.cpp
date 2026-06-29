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
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "SleNameChangeManager.h"
#include "SleProperties.h"
#include "log.h"
#include "nearlink_access_token_mock.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class MockNameChangeListener : public ISleNameChangeListener {
public:
    explicit MockNameChangeListener(const std::string &id) : id_(id) {}
    ~MockNameChangeListener() override = default;

    void OnLocalNameChanged(const std::string &newLocalName) override
    {
        std::lock_guard<std::mutex> lock(mtx_);
        callCount_++;
        lastReceivedName_ = newLocalName;
        receivedNames_.push_back(newLocalName);
    }

    int GetCallCount()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return callCount_;
    }

    std::string GetLastReceivedName()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return lastReceivedName_;
    }

    std::vector<std::string> GetReceivedNames()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return receivedNames_;
    }

    void Reset()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        callCount_ = 0;
        lastReceivedName_ = "";
        receivedNames_.clear();
    }

private:
    std::string id_;
    std::mutex mtx_;
    int callCount_ = 0;
    std::string lastReceivedName_;
    std::vector<std::string> receivedNames_;
};

class SleNameChangeManagerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SleNameChangeManagerTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    HILOGI("SetUpTestCase end");
}

void SleNameChangeManagerTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase start");
}

void SleNameChangeManagerTest::SetUp()
{
    HILOGI("SetUp start");
}

void SleNameChangeManagerTest::TearDown()
{
    HILOGI("TearDown start");
}

HWTEST_F(SleNameChangeManagerTest, NotifyLocalNameChanged_001, TestSize.Level1)
{
    HILOGI("NotifyLocalNameChanged_001 start");
    auto observer = std::make_shared<MockNameChangeListener>("obs1");
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(observer);
    observer->Reset();

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("DeviceA");
    EXPECT_EQ(observer->GetCallCount(), 1);
    EXPECT_EQ(observer->GetLastReceivedName(), "DeviceA");

    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(observer);
    HILOGI("NotifyLocalNameChanged_001 end");
}

HWTEST_F(SleNameChangeManagerTest, NotifyLocalNameChanged_SameNameSkipped, TestSize.Level1)
{
    HILOGI("NotifyLocalNameChanged_SameNameSkipped start");
    auto observer = std::make_shared<MockNameChangeListener>("obs2");
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(observer);
    observer->Reset();

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("DeviceSame");
    EXPECT_EQ(observer->GetCallCount(), 1);
    EXPECT_EQ(observer->GetLastReceivedName(), "DeviceSame");

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("DeviceSame");
    EXPECT_EQ(observer->GetCallCount(), 1);

    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(observer);
    HILOGI("NotifyLocalNameChanged_SameNameSkipped end");
}

HWTEST_F(SleNameChangeManagerTest, NotifyLocalNameChanged_NameChangeSequence, TestSize.Level1)
{
    HILOGI("NotifyLocalNameChanged_NameChangeSequence start");
    auto observer = std::make_shared<MockNameChangeListener>("obs3");
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(observer);
    observer->Reset();

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("NameA");
    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("NameB");
    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("NameA");

    EXPECT_EQ(observer->GetCallCount(), 3);
    auto names = observer->GetReceivedNames();
    ASSERT_EQ(names.size(), 3U);
    EXPECT_EQ(names[0], "NameA");
    EXPECT_EQ(names[1], "NameB");
    EXPECT_EQ(names[2], "NameA");

    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(observer);
    HILOGI("NotifyLocalNameChanged_NameChangeSequence end");
}

HWTEST_F(SleNameChangeManagerTest, NotifyLocalNameChanged_MultipleObservers, TestSize.Level1)
{
    HILOGI("NotifyLocalNameChanged_MultipleObservers start");
    auto obs1 = std::make_shared<MockNameChangeListener>("multi1");
    auto obs2 = std::make_shared<MockNameChangeListener>("multi2");
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(obs1);
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(obs2);
    obs1->Reset();
    obs2->Reset();

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("SharedDevice");

    EXPECT_EQ(obs1->GetCallCount(), 1);
    EXPECT_EQ(obs1->GetLastReceivedName(), "SharedDevice");
    EXPECT_EQ(obs2->GetCallCount(), 1);
    EXPECT_EQ(obs2->GetLastReceivedName(), "SharedDevice");

    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(obs1);
    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(obs2);
    HILOGI("NotifyLocalNameChanged_MultipleObservers end");
}

HWTEST_F(SleNameChangeManagerTest, DeregisterLocalNameObserver_001, TestSize.Level1)
{
    HILOGI("DeregisterLocalNameObserver_001 start");
    auto observer = std::make_shared<MockNameChangeListener>("obs_dereg");
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(observer);
    observer->Reset();

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("BeforeDereg");
    EXPECT_EQ(observer->GetCallCount(), 1);

    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(observer);

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("AfterDereg_Unique1");
    EXPECT_EQ(observer->GetCallCount(), 1);

    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(observer);
    HILOGI("DeregisterLocalNameObserver_001 end");
}

HWTEST_F(SleNameChangeManagerTest, RegisterLocalNameObserver_Nullptr, TestSize.Level1)
{
    HILOGI("RegisterLocalNameObserver_Nullptr start");
    std::shared_ptr<MockNameChangeListener> nullObs = nullptr;
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(nullObs);
    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(nullObs);
    HILOGI("RegisterLocalNameObserver_Nullptr end");
}

HWTEST_F(SleNameChangeManagerTest, NotifyLocalNameChanged_EmptyString, TestSize.Level1)
{
    HILOGI("NotifyLocalNameChanged_EmptyString start");
    auto observer = std::make_shared<MockNameChangeListener>("obs_empty");
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(observer);
    observer->Reset();

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("EmptyPreempt");
    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("");
    EXPECT_EQ(observer->GetCallCount(), 2);
    EXPECT_EQ(observer->GetLastReceivedName(), std::string(""));

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("");
    EXPECT_EQ(observer->GetCallCount(), 2);

    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(observer);
    HILOGI("NotifyLocalNameChanged_EmptyString end");
}

HWTEST_F(SleNameChangeManagerTest, NotifyLocalNameChanged_ConcurrentSameValue, TestSize.Level1)
{
    HILOGI("NotifyLocalNameChanged_ConcurrentSameValue start");
    auto observer = std::make_shared<MockNameChangeListener>("obs_concurrent");
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(observer);
    observer->Reset();

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("ConcurrentBase");
    EXPECT_EQ(observer->GetCallCount(), 1);

    constexpr int threadCount = 4;
    std::thread threads[threadCount];
    for (int i = 0; i < threadCount; i++) {
        threads[i] = std::thread([]() {
            SleNameChangeManager::GetInstance().NotifyLocalNameChanged("ConcurrentSame");
        });
    }
    for (int i = 0; i < threadCount; i++) {
        threads[i].join();
    }

    int callCount = observer->GetCallCount();
    EXPECT_GE(callCount, 2);
    EXPECT_LE(callCount, threadCount + 1);
    EXPECT_EQ(observer->GetLastReceivedName(), "ConcurrentSame");

    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(observer);
    HILOGI("NotifyLocalNameChanged_ConcurrentSameValue end");
}

HWTEST_F(SleNameChangeManagerTest, SlePropertiesCallback_SameNameSkipped, TestSize.Level1)
{
    HILOGI("SlePropertiesCallback_SameNameSkipped start");
    std::string nameBefore = SleProperties::GetInstance().GetLocalName();
    HILOGI("nameBefore = %{public}s", nameBefore.c_str());

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged(nameBefore);

    std::string nameAfter = SleProperties::GetInstance().GetLocalName();
    EXPECT_EQ(nameBefore, nameAfter);
    HILOGI("SlePropertiesCallback_SameNameSkipped end");
}

HWTEST_F(SleNameChangeManagerTest, SlePropertiesCallback_DifferentNameUpdated, TestSize.Level1)
{
    HILOGI("SlePropertiesCallback_DifferentNameUpdated start");
    std::string originalName = SleProperties::GetInstance().GetLocalName();
    HILOGI("originalName = %{public}s", originalName.c_str());

    std::string newName = "TestDevice_Updated";
    SleNameChangeManager::GetInstance().NotifyLocalNameChanged(newName);

    std::string nameAfterUpdate = SleProperties::GetInstance().GetLocalName();
    EXPECT_EQ(nameAfterUpdate, newName);

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged(originalName);
    std::string nameAfterRestore = SleProperties::GetInstance().GetLocalName();
    EXPECT_EQ(nameAfterRestore, originalName);
    HILOGI("SlePropertiesCallback_DifferentNameUpdated end");
}

HWTEST_F(SleNameChangeManagerTest, NotifyLocalNameChanged_LazyInitThenNotify, TestSize.Level1)
{
    HILOGI("NotifyLocalNameChanged_LazyInitThenNotify start");
    auto observer = std::make_shared<MockNameChangeListener>("obs_lazy");
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(observer);
    observer->Reset();

    std::string cachedName = SleProperties::GetInstance().GetLocalName();
    HILOGI("cachedName = %{public}s", cachedName.c_str());

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged(cachedName);

    EXPECT_EQ(observer->GetCallCount(), 0);

    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(observer);
    HILOGI("NotifyLocalNameChanged_LazyInitThenNotify end");
}

HWTEST_F(SleNameChangeManagerTest, NotifyLocalNameChanged_CallbackDuplicateWriteProtected, TestSize.Level1)
{
    HILOGI("NotifyLocalNameChanged_CallbackDuplicateWriteProtected start");
    std::string currentName = SleProperties::GetInstance().GetLocalName();
    HILOGI("currentName = %{public}s", currentName.c_str());

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("DupWriteTest");
    std::string afterFirst = SleProperties::GetInstance().GetLocalName();
    EXPECT_EQ(afterFirst, std::string("DupWriteTest"));

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged("DupWriteTest");
    std::string afterSecond = SleProperties::GetInstance().GetLocalName();
    EXPECT_EQ(afterSecond, std::string("DupWriteTest"));

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged(currentName);
    HILOGI("NotifyLocalNameChanged_CallbackDuplicateWriteProtected end");
}

HWTEST_F(SleNameChangeManagerTest, NotifyLocalNameChanged_MultiObserverDedup, TestSize.Level1)
{
    HILOGI("NotifyLocalNameChanged_MultiObserverDedup start");
    auto obs1 = std::make_shared<MockNameChangeListener>("dedup1");
    auto obs2 = std::make_shared<MockNameChangeListener>("dedup2");
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(obs1);
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(obs2);
    obs1->Reset();
    obs2->Reset();

    std::string cachedName = SleProperties::GetInstance().GetLocalName();
    HILOGI("cachedName = %{public}s", cachedName.c_str());

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged(cachedName);
    EXPECT_EQ(obs1->GetCallCount(), 0);
    EXPECT_EQ(obs2->GetCallCount(), 0);

    std::string newName = "DedupTest_New";
    SleNameChangeManager::GetInstance().NotifyLocalNameChanged(newName);
    EXPECT_EQ(obs1->GetCallCount(), 1);
    EXPECT_EQ(obs2->GetCallCount(), 1);
    EXPECT_EQ(obs1->GetLastReceivedName(), newName);
    EXPECT_EQ(obs2->GetLastReceivedName(), newName);

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged(newName);
    EXPECT_EQ(obs1->GetCallCount(), 1);
    EXPECT_EQ(obs2->GetCallCount(), 1);

    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(obs1);
    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(obs2);

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged(cachedName);
    HILOGI("NotifyLocalNameChanged_MultiObserverDedup end");
}

HWTEST_F(SleNameChangeManagerTest, OnChange_DatashareSameAsCached, TestSize.Level1)
{
    HILOGI("OnChange_DatashareSameAsCached start");
    auto observer = std::make_shared<MockNameChangeListener>("obs_onchange");
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(observer);
    observer->Reset();

    std::string cachedName = SleProperties::GetInstance().GetLocalName();
    HILOGI("cachedName = %{public}s", cachedName.c_str());

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged(cachedName);
    EXPECT_EQ(observer->GetCallCount(), 0);
    HILOGI("NotifyLocalNameChanged with same cached name: no notification (lastNotifiedName_ dedup)");

    SleNameChangeObserver changeObserver;
    changeObserver.OnChange();

    EXPECT_EQ(observer->GetCallCount(), 0);
    HILOGI("OnChange when datashare returns same value: no notification");

    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(observer);

    SleNameChangeManager::GetInstance().NotifyLocalNameChanged(cachedName);
    HILOGI("OnChange_DatashareSameAsCached end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
