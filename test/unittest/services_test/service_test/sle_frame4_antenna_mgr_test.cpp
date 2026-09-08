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

#include "SleFrame4AntennaMgr.h"
#include "ThreadUtil.h"
#include "nearlink_def_types.h"
#include "nlstk_devd_scan_type_ext.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
using namespace testing;
using namespace testing::ext;
namespace {
constexpr uint8_t TEST_ADV_HANDLE_1 = 1;
constexpr uint8_t TEST_ADV_HANDLE_2 = 2;
constexpr uint32_t TEST_SCANNER_ID_1 = 10;
constexpr uint32_t TEST_SCANNER_ID_2 = 11;
constexpr uint8_t FRAME_TYPE_1_ADV = static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_1);
constexpr uint8_t FRAME_TYPE_4_ADV = static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_4);
constexpr uint8_t FRAME_TYPE_1_SCAN = static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_1);
constexpr uint8_t FRAME_TYPE_4_SCAN = static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_4);
}

class SleFrame4AntennaMgrTest : public testing::Test {
public:
    SleFrame4AntennaMgrTest() {}
    ~SleFrame4AntennaMgrTest() {}

    static void SetUpTestCase(void)
    {
        HILOGI("SetUpTestCase SleFrame4AntennaMgrTest.");
        // antenna队列任务在调用线程内联同步执行，保证测试断言的同步性
        ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(THREAD_ID_ANTENNA, ThreadUtil::NOT_SWITCH_THREAD);
    }
    static void TearDownTestCase(void)
    {
        HILOGI("TearDownTestCase SleFrame4AntennaMgrTest");
        ThreadUtil::GetInstance().InitThreadStateMap();
    }
    void SetUp() {}
    void TearDown()
    {
        auto &mgr = SleFrame4AntennaMgr::GetInstance();
        mgr.frame4AdvCount_ = 0;
        mgr.frame4ScanCount_ = 0;
        mgr.frame4AdvHandles_.clear();
        mgr.frame4ScannerIds_.clear();
    }
};

/*
 * @tc.number: UnitTest_SleFrame4AntennaMgr_001
 * @tc.name: frame4 adv start and remove
 * @tc.desc: frame4 adv start increments adv count, remove decrements adv count
 */
HWTEST_F(SleFrame4AntennaMgrTest, UnitTest_SleFrame4AntennaMgr_001, TestSize.Level1)
{
    HILOGI("UnitTest_SleFrame4AntennaMgr_001 start");
    auto &mgr = SleFrame4AntennaMgr::GetInstance();
    EXPECT_EQ(mgr.frame4AdvCount_, 0);

    mgr.OnAdvStarted(TEST_ADV_HANDLE_1, FRAME_TYPE_4_ADV);
    EXPECT_EQ(mgr.frame4AdvCount_, 1);

    mgr.OnAdvRemoved(TEST_ADV_HANDLE_1);
    EXPECT_EQ(mgr.frame4AdvCount_, 0);
    HILOGI("UnitTest_SleFrame4AntennaMgr_001 end");
}

/*
 * @tc.number: UnitTest_SleFrame4AntennaMgr_002
 * @tc.name: non-frame4 adv ignored
 * @tc.desc: non-frame4 adv start and remove do not affect adv count
 */
HWTEST_F(SleFrame4AntennaMgrTest, UnitTest_SleFrame4AntennaMgr_002, TestSize.Level1)
{
    HILOGI("UnitTest_SleFrame4AntennaMgr_002 start");
    auto &mgr = SleFrame4AntennaMgr::GetInstance();

    mgr.OnAdvStarted(TEST_ADV_HANDLE_1, FRAME_TYPE_1_ADV);
    EXPECT_EQ(mgr.frame4AdvCount_, 0);

    mgr.OnAdvRemoved(TEST_ADV_HANDLE_1);
    EXPECT_EQ(mgr.frame4AdvCount_, 0);
    HILOGI("UnitTest_SleFrame4AntennaMgr_002 end");
}

/*
 * @tc.number: UnitTest_SleFrame4AntennaMgr_003
 * @tc.name: frame4 scan start and stop
 * @tc.desc: frame4 scan start increments scan count, stop decrements scan count
 */
HWTEST_F(SleFrame4AntennaMgrTest, UnitTest_SleFrame4AntennaMgr_003, TestSize.Level1)
{
    HILOGI("UnitTest_SleFrame4AntennaMgr_003 start");
    auto &mgr = SleFrame4AntennaMgr::GetInstance();

    mgr.OnScanStarted(TEST_SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    EXPECT_EQ(mgr.frame4ScanCount_, 1);

    mgr.OnScanStopped(TEST_SCANNER_ID_1);
    EXPECT_EQ(mgr.frame4ScanCount_, 0);
    HILOGI("UnitTest_SleFrame4AntennaMgr_003 end");
}

/*
 * @tc.number: UnitTest_SleFrame4AntennaMgr_004
 * @tc.name: non-frame4 scan and monitor mode ignored
 * @tc.desc: non-frame4 scan and monitor mode start do not increment scan count
 */
HWTEST_F(SleFrame4AntennaMgrTest, UnitTest_SleFrame4AntennaMgr_004, TestSize.Level1)
{
    HILOGI("UnitTest_SleFrame4AntennaMgr_004 start");
    auto &mgr = SleFrame4AntennaMgr::GetInstance();

    mgr.OnScanStarted(TEST_SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_1_SCAN);
    EXPECT_EQ(mgr.frame4ScanCount_, 0);

    mgr.OnScanStarted(TEST_SCANNER_ID_2, SCAN_MODE_MONITOR, FRAME_TYPE_4_SCAN);
    EXPECT_EQ(mgr.frame4ScanCount_, 0);
    HILOGI("UnitTest_SleFrame4AntennaMgr_004 end");
}

/*
 * @tc.number: UnitTest_SleFrame4AntennaMgr_005
 * @tc.name: adv and scan count independently
 * @tc.desc: frame4 adv count and frame4 scan count are independent of each other
 */
HWTEST_F(SleFrame4AntennaMgrTest, UnitTest_SleFrame4AntennaMgr_005, TestSize.Level1)
{
    HILOGI("UnitTest_SleFrame4AntennaMgr_005 start");
    auto &mgr = SleFrame4AntennaMgr::GetInstance();

    mgr.OnAdvStarted(TEST_ADV_HANDLE_1, FRAME_TYPE_4_ADV);
    mgr.OnScanStarted(TEST_SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    EXPECT_EQ(mgr.frame4AdvCount_, 1);
    EXPECT_EQ(mgr.frame4ScanCount_, 1);

    mgr.OnAdvRemoved(TEST_ADV_HANDLE_1);
    EXPECT_EQ(mgr.frame4AdvCount_, 0);
    EXPECT_EQ(mgr.frame4ScanCount_, 1);

    mgr.OnScanStopped(TEST_SCANNER_ID_1);
    EXPECT_EQ(mgr.frame4AdvCount_, 0);
    EXPECT_EQ(mgr.frame4ScanCount_, 0);
    HILOGI("UnitTest_SleFrame4AntennaMgr_005 end");
}

/*
 * @tc.number: UnitTest_SleFrame4AntennaMgr_006
 * @tc.name: repeated start is idempotent
 * @tc.desc: repeated start of the same adv handle or scanner id is counted only once
 */
HWTEST_F(SleFrame4AntennaMgrTest, UnitTest_SleFrame4AntennaMgr_006, TestSize.Level1)
{
    HILOGI("UnitTest_SleFrame4AntennaMgr_006 start");
    auto &mgr = SleFrame4AntennaMgr::GetInstance();

    mgr.OnAdvStarted(TEST_ADV_HANDLE_1, FRAME_TYPE_4_ADV);
    mgr.OnAdvStarted(TEST_ADV_HANDLE_1, FRAME_TYPE_4_ADV);
    EXPECT_EQ(mgr.frame4AdvCount_, 1);

    mgr.OnScanStarted(TEST_SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    mgr.OnScanStarted(TEST_SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    EXPECT_EQ(mgr.frame4ScanCount_, 1);

    mgr.OnAdvRemoved(TEST_ADV_HANDLE_1);
    mgr.OnAdvRemoved(TEST_ADV_HANDLE_1);
    mgr.OnScanStopped(TEST_SCANNER_ID_1);
    mgr.OnScanStopped(TEST_SCANNER_ID_1);
    EXPECT_EQ(mgr.frame4AdvCount_, 0);
    EXPECT_EQ(mgr.frame4ScanCount_, 0);
    HILOGI("UnitTest_SleFrame4AntennaMgr_006 end");
}

/*
 * @tc.number: UnitTest_SleFrame4AntennaMgr_007
 * @tc.name: restart with non-frame4 or monitor mode
 * @tc.desc: restart of a counted scanner with non-frame4 or monitor mode removes it from count
 */
HWTEST_F(SleFrame4AntennaMgrTest, UnitTest_SleFrame4AntennaMgr_007, TestSize.Level1)
{
    HILOGI("UnitTest_SleFrame4AntennaMgr_007 start");
    auto &mgr = SleFrame4AntennaMgr::GetInstance();

    mgr.OnScanStarted(TEST_SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    EXPECT_EQ(mgr.frame4ScanCount_, 1);

    mgr.OnScanStarted(TEST_SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_1_SCAN);
    EXPECT_EQ(mgr.frame4ScanCount_, 0);

    mgr.OnScanStarted(TEST_SCANNER_ID_2, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    EXPECT_EQ(mgr.frame4ScanCount_, 1);

    mgr.OnScanStarted(TEST_SCANNER_ID_2, SCAN_MODE_MONITOR, FRAME_TYPE_4_SCAN);
    EXPECT_EQ(mgr.frame4ScanCount_, 0);
    HILOGI("UnitTest_SleFrame4AntennaMgr_007 end");
}

/*
 * @tc.number: UnitTest_SleFrame4AntennaMgr_008
 * @tc.name: stop all scan
 * @tc.desc: OnAllScanStopped removes all counted scanners
 */
HWTEST_F(SleFrame4AntennaMgrTest, UnitTest_SleFrame4AntennaMgr_008, TestSize.Level1)
{
    HILOGI("UnitTest_SleFrame4AntennaMgr_008 start");
    auto &mgr = SleFrame4AntennaMgr::GetInstance();

    mgr.OnScanStarted(TEST_SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    mgr.OnScanStarted(TEST_SCANNER_ID_2, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    mgr.OnAdvStarted(TEST_ADV_HANDLE_1, FRAME_TYPE_4_ADV);
    EXPECT_EQ(mgr.frame4ScanCount_, 2);
    EXPECT_EQ(mgr.frame4AdvCount_, 1);

    mgr.OnAllScanStopped();
    EXPECT_EQ(mgr.frame4ScanCount_, 0);
    EXPECT_EQ(mgr.frame4AdvCount_, 1);

    mgr.OnAllScanStopped();
    EXPECT_EQ(mgr.frame4ScanCount_, 0);

    mgr.OnAdvRemoved(TEST_ADV_HANDLE_1);
    EXPECT_EQ(mgr.frame4AdvCount_, 0);
    HILOGI("UnitTest_SleFrame4AntennaMgr_008 end");
}

/*
 * @tc.number: UnitTest_SleFrame4AntennaMgr_009
 * @tc.name: stop unregistered scanner or adv
 * @tc.desc: stopping unregistered scanner or removing unregistered adv does not affect count
 */
HWTEST_F(SleFrame4AntennaMgrTest, UnitTest_SleFrame4AntennaMgr_009, TestSize.Level1)
{
    HILOGI("UnitTest_SleFrame4AntennaMgr_009 start");
    auto &mgr = SleFrame4AntennaMgr::GetInstance();

    mgr.OnScanStopped(TEST_SCANNER_ID_1);
    mgr.OnAdvRemoved(TEST_ADV_HANDLE_1);
    mgr.OnAllScanStopped();
    EXPECT_EQ(mgr.frame4AdvCount_, 0);
    EXPECT_EQ(mgr.frame4ScanCount_, 0);
    HILOGI("UnitTest_SleFrame4AntennaMgr_009 end");
}
}  // namespace Nearlink
}  // namespace OHOS
