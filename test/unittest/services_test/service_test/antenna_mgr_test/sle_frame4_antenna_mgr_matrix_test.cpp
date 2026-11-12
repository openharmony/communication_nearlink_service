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

#include <chrono>
#include <future>
#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>
#include "SleFrame4AntennaMgr.h"
#include "ThreadUtil.h"
#include "mock_service_manager_plugin.h"
#include "nearlink_def_types.h"
#include "nlstk_devd_scan_type_ext.h"

namespace OHOS {
namespace Nearlink {
using namespace testing;
using namespace testing::ext;
namespace {
constexpr uint8_t ADV_HANDLE_1 = 1;
constexpr uint8_t ADV_HANDLE_2 = 2;
constexpr uint32_t SCANNER_ID_1 = 10;
constexpr uint32_t SCANNER_ID_2 = 11;
constexpr uint8_t FRAME_TYPE_1_ADV = static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_1);
constexpr uint8_t FRAME_TYPE_4_ADV = static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_4);
constexpr uint8_t FRAME_TYPE_1_SCAN = static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_1);
constexpr uint8_t FRAME_TYPE_4_SCAN = static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_4);
constexpr int WAIT_QUEUE_IDLE_SEC = 5;

using AntennaFixCall = std::pair<bool, AntennaFixScene>;
const AntennaFixCall FIX_ADV_ON = { true, AntennaFixScene::ANTENNA_FIX_SCENE_FRAME4_ADV };
const AntennaFixCall FIX_ADV_OFF = { false, AntennaFixScene::ANTENNA_FIX_SCENE_FRAME4_ADV };
const AntennaFixCall FIX_SCAN_ON = { true, AntennaFixScene::ANTENNA_FIX_SCENE_FRAME4_SCAN };
const AntennaFixCall FIX_SCAN_OFF = { false, AntennaFixScene::ANTENNA_FIX_SCENE_FRAME4_SCAN };

// 被测事件走真实 antenna 串行队列。哨兵任务投到同一队列并等待执行：
// 队列按提交序串行，先于哨兵投递的任务必已全部完成，此后再断言录制结果。
// promise 用 shared_ptr 保活：即使等待超时返回，迟到的哨兵也不会悬垂引用。
static bool WaitAntennaQueueIdle()
{
    auto done = std::make_shared<std::promise<void>>();
    std::future<void> signal = done->get_future();
    DoInAntennaThread([done]() { done->set_value(); });
    return signal.wait_for(std::chrono::seconds(WAIT_QUEUE_IDLE_SEC)) == std::future_status::ready;
}

static void ExpectAntennaFixCalls(std::initializer_list<AntennaFixCall> expected)
{
    const auto &actual = GetAntennaMockPlugin().fixCalls;
    EXPECT_EQ(actual.size(), expected.size());
    auto exp = expected.begin();
    for (size_t i = 0; i < actual.size() && exp != expected.end(); ++i, ++exp) {
        EXPECT_EQ(actual[i].first, exp->first) << "antenna fix call[" << i << "] enable mismatch";
        EXPECT_EQ(static_cast<int>(actual[i].second), static_cast<int>(exp->second))
            << "antenna fix call[" << i << "] scene mismatch";
    }
}
}  // namespace

class SleFrame4AntennaMgrMatrixTest : public testing::Test {
public:
    SleFrame4AntennaMgrMatrixTest() {}
    ~SleFrame4AntennaMgrMatrixTest() {}

    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    // 每用例清空 mock 录制容器；被测对象为用例内局部实例（状态全在实例内，无单例残留），
    // 析构于用例末尾，此前队列必已用哨兵排空
    void SetUp() override
    {
        GetAntennaMockPlugin().Clear();
    }
    void TearDown() override {}
};

// 非帧4广播不参与计数，不触发天线固定
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_001, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr; // 局部实例：状态全在实例内，用例间无单例泄漏
    mgr.OnAdvStarted(ADV_HANDLE_1, FRAME_TYPE_1_ADV);
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({});
}

// 首个帧4广播启动时固定天线
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_002, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnAdvStarted(ADV_HANDLE_1, FRAME_TYPE_4_ADV);
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({ FIX_ADV_ON });
}

// 重复启动同一广播句柄只计数一次，不重复固定天线
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_003, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnAdvStarted(ADV_HANDLE_1, FRAME_TYPE_4_ADV);
    mgr.OnAdvStarted(ADV_HANDLE_1, FRAME_TYPE_4_ADV);
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({ FIX_ADV_ON });
}

// 第二个帧4广播启动时不再重复固定天线
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_004, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnAdvStarted(ADV_HANDLE_1, FRAME_TYPE_4_ADV);
    mgr.OnAdvStarted(ADV_HANDLE_2, FRAME_TYPE_4_ADV);
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({ FIX_ADV_ON });
}

// 最后一个帧4广播移除时释放天线
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_005, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnAdvStarted(ADV_HANDLE_1, FRAME_TYPE_4_ADV);
    mgr.OnAdvRemoved(ADV_HANDLE_1);
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({ FIX_ADV_ON, FIX_ADV_OFF });
}

// 移除两个帧4广播之一时天线保持固定
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_006, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnAdvStarted(ADV_HANDLE_1, FRAME_TYPE_4_ADV);
    mgr.OnAdvStarted(ADV_HANDLE_2, FRAME_TYPE_4_ADV);
    mgr.OnAdvRemoved(ADV_HANDLE_1);
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({ FIX_ADV_ON });
}

// 移除未计数的广播句柄无任何效果
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_007, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnAdvRemoved(ADV_HANDLE_1);
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({});
}

// MONITOR 模式启动扫描视为停止该扫描；未计数的扫描不产生任何天线调用
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_008, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnScanStarted(SCANNER_ID_1, SCAN_MODE_MONITOR, FRAME_TYPE_4_SCAN);
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({});
}

// 首个帧4扫描启动时固定天线
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_009, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnScanStarted(SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({ FIX_SCAN_ON });
}

// 重复启动同一扫描者只计数一次，不重复固定天线
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_010, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnScanStarted(SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    mgr.OnScanStarted(SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({ FIX_SCAN_ON });
}

// 第二个帧4扫描启动时不再重复固定天线
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_011, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnScanStarted(SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    mgr.OnScanStarted(SCANNER_ID_2, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({ FIX_SCAN_ON });
}

// 已计数的扫描以非帧4类型重启时移出计数并释放天线
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_012, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnScanStarted(SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    mgr.OnScanStarted(SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_1_SCAN);
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({ FIX_SCAN_ON, FIX_SCAN_OFF });
}

// 最后一个帧4扫描停止时释放天线
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_013, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnScanStarted(SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    mgr.OnScanStopped(SCANNER_ID_1);
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({ FIX_SCAN_ON, FIX_SCAN_OFF });
}

// 停止两个帧4扫描之一时天线保持固定
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_014, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnScanStarted(SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    mgr.OnScanStarted(SCANNER_ID_2, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    mgr.OnScanStopped(SCANNER_ID_1);
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({ FIX_SCAN_ON });
}

// 无帧4扫描在计时，停止全部扫描不产生任何天线调用
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_015, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnAllScanStopped();
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({});
}

// 停止全部扫描时清空所有计数并释放天线
HWTEST_F(SleFrame4AntennaMgrMatrixTest, UnitTest_SleFrame4AntennaMgrMatrix_016, TestSize.Level1)
{
    SleFrame4AntennaMgr mgr;
    mgr.OnScanStarted(SCANNER_ID_1, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    mgr.OnScanStarted(SCANNER_ID_2, SCAN_MODE_LOW_LATENCY, FRAME_TYPE_4_SCAN);
    mgr.OnAllScanStopped();
    ASSERT_TRUE(WaitAntennaQueueIdle());
    ExpectAntennaFixCalls({ FIX_SCAN_ON, FIX_SCAN_OFF });
}
}  // namespace Nearlink
}  // namespace OHOS
