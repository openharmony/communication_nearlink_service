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

#include "nearlink_access_token_mock.h"
#include "nearlink_def.h"
#include "nearlink_host.h"
#include "nearlink_sle_scanner.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
using namespace OHOS::Nearlink;

class SleScannerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SleScannerTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    if (!NearlinkHost::GetInstance().IsSleEnabled()) {
        HILOGI("start enable nearlink");
        NearlinkHost::GetInstance().EnableNl();
        sleep(2); // sleep 2s
    }
    HILOGI("SetUpTestCase end");
}

void SleScannerTest::TearDownTestCase()
{}

void SleScannerTest::SetUp()
{}

void SleScannerTest::TearDown()
{}

class SleCentralManagerCallbackTest final: public Nearlink::SleCentralManagerCallback {
public:
    SleCentralManagerCallbackTest() {};
    ~SleCentralManagerCallbackTest() {};

private:
    void OnScanCallback(const SleScanResult &result) {}
    void OnSleBatchScanResultsEvent(const std::vector<SleScanResult> &results) {}
    void OnStartOrStopScanEvent(int resultCode, bool isStartScan)
    {
        HILOGI("resultCode: %{public}d, isStartScan: %{public}d", resultCode, isStartScan);
    }
};

/**
 * @tc.number: CreateSleCentralManager001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleScannerTest, CreateSleCentralManager001, TestSize.Level1)
{
    HILOGI("CreateSleCentralManager001 start");
    std::shared_ptr<SleCentralManagerCallbackTest> callback = std::make_shared<SleCentralManagerCallbackTest>();
    std::shared_ptr<SleCentralManager> scanInst = SleCentralManager::CreateSleCentralManager(callback);
    EXPECT_NE(nullptr, scanInst);
    scanInst = nullptr;
    HILOGI("CreateSleCentralManager001 end");
}

/**
 * @tc.number: StartAndStopScan001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleScannerTest, StartAndStopScan001, TestSize.Level1)
{
    HILOGI("StartAndStopScan001 start");
    std::shared_ptr<SleCentralManagerCallbackTest> callback = std::make_shared<SleCentralManagerCallbackTest>();
    std::shared_ptr<SleCentralManager> scanInst = SleCentralManager::CreateSleCentralManager(callback);
    NlErrCode ret = scanInst->StartScan();
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the scan is completely started.
    ret = scanInst->StopScan();
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the scan is completely stopped.
    HILOGI("StartAndStopScan001 end");
}

/**
 * @tc.number: StartAndStopScan002
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleScannerTest, StartAndStopScan002, TestSize.Level1)
{
    HILOGI("StartAndStopScan002 start");
    std::shared_ptr<SleCentralManagerCallbackTest> callback = std::make_shared<SleCentralManagerCallbackTest>();
    std::shared_ptr<SleCentralManager> scanInst = SleCentralManager::CreateSleCentralManager(callback);
    SleScanSettings settings;
    settings.SetScanMode(SCAN_MODE_LOW_POWER);
    NlErrCode ret = scanInst->StartFullScan(settings);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the scan is completely started.
    ret = scanInst->StopScan();
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the scan is completely stopped.
    HILOGI("StartAndStopScan002 end");
}

/**
 * @tc.number: StartScanWithFilter001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleScannerTest, StartScanWithFilter001, TestSize.Level1)
{
    HILOGI("StartScanWithFilter001 start");
    std::shared_ptr<SleCentralManagerCallbackTest> callback = std::make_shared<SleCentralManagerCallbackTest>();
    std::shared_ptr<SleCentralManager> scanInst = SleCentralManager::CreateSleCentralManager(callback);
    SleScanSettings settings;
    settings.SetFrameType(static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_4));
    settings.SetScanMode(SCAN_MODE_LOW_POWER);
    SleScanFilter filter;
    UUID uuid = UUID::FromString("00000000-0000-1000-8000-00805F9B34FB"); // 参数为test使用，无实际意义，下同
    filter.SetServiceUuid(uuid);
    UUID uuidMask = UUID::FromString("00000000-0000-1000-8000-00FFFFFFFFFF");
    filter.SetServiceUuidMask(uuidMask);
    UUID solicitationUuid = UUID::FromString("00000000-0000-1000-8000-00805F9B34FB");
    filter.SetServiceSolicitationUuid(solicitationUuid);
    UUID solicitationUuidMask = UUID::FromString("00000000-0000-1000-8000-00FFFFFFFFFF");
    filter.SetServiceSolicitationUuidMask(solicitationUuidMask);
    std::vector<uint8_t> serviceData {0x00, 0x11};
    filter.SetServiceData(serviceData);
    std::vector<uint8_t> serviceDataMask {0xFF, 0xFF};
    filter.SetServiceDataMask(serviceDataMask);
    filter.SetRssiThreshold(90);
    std::vector<uint8_t> maufactureData {0x00, 0x11};
    filter.SetManufactureData(maufactureData);
    std::vector<uint8_t> maufactureDataMask {0xFF, 0xFF};
    filter.SetManufactureDataMask(maufactureDataMask);
    std::vector<SleScanFilter> filters {filter};
    NlErrCode ret = scanInst->StartScanWithFilter(settings, filters);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the scan is completely started.
    ret = scanInst->StopScan();
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the scan is completely stopped.
    HILOGI("StartScanWithFilter001 end");
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
