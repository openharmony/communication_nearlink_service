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

// 目标: frameworks/native/nearlink_sle_scanner.cpp 零覆盖分支(进程内驱动, 不依赖真实扫描栈)
// 手法: 编入产品源文件(private=public) 直达 pimpl; 用产品 Stub 派生的 Mock 服务注入
//       NearlinkBaseSaManager::profileRemoteMap_(GetProxy 命中 map 即返回本地 Stub,
//       NewInstance 走 AsInterface 直连虚表, 无 IPC 无真实栈), 瞄准 94/141/184-215/248/434
//       行分支; 141 的 false 腿(make_shared 后判空)结构不可达, 不设用例。

#include <gtest/gtest.h>
#include <cstring>
#include <vector>

#include "nearlink_test_enable_wait.h"
#include "nearlink_access_token_mock.h"

#include "nearlink_base_sa_manager.h"
#include "nearlink_sle_central_manager_stub.h"
#include "nearlink_sle_scanner.cpp"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
const std::string TEST_DEVICE_ADDR = "11:22:33:44:55:66";
const std::string TEST_DEVICE_NAME = "scanner_ut_device";
const std::string UUID_STRING = "11223344-0000-1000-8000-00805F9B34FB";
constexpr uint16_t TEST_MANUFACTURER_ID = 0x1234;
constexpr uint32_t TEST_DEVICE_CLASS = 0x0804;
constexpr uint8_t PRIM_FRAME_TYPE_1 = 0x00; // SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_1
constexpr uint8_t PRIM_FRAME_TYPE_4 = 0x01; // SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_4
constexpr uint8_t PRIM_FRAME_TYPE_INVALID = 0x05; // 非法帧型, 触发告警腿并回退 TYPE_1

// Mock 服务: 派生产品 Stub, 覆写全部接口方法为受控返回+调用记录; 注入后 GetProxy 直连本对象
class MockSleCentralManagerService : public NearlinkSleCentralManagerStub {
public:
    NlErrCode regRet = NL_NO_ERROR;
    uint32_t regScannerId = 42;
    NlErrCode startRet = NL_NO_ERROR;
    int startWithFilterCount = 0;
    int startFullScanCount = 0;
    int stopScanCount = 0;
    uint32_t lastScannerId = 0;
    size_t lastFilterCount = 0;
    std::vector<uint8_t> lastFilterFrameTypes; // parcel operator= 未从 so 导出, 只记录标量

    NlErrCode RegisterSleCentralManagerCallback(uint32_t &scannerId, bool enableRandomAddrMode,
        const sptr<INearlinkSleCentralManagerCallback> &callback) override
    {
        (void)enableRandomAddrMode;
        (void)callback;
        scannerId = regScannerId;
        return regRet;
    }

    NlErrCode DeregisterSleCentralManagerCallback(uint32_t scannerId,
        const sptr<INearlinkSleCentralManagerCallback> &callback) override
    {
        (void)scannerId;
        (void)callback;
        return NL_NO_ERROR;
    }

    NlErrCode StartScanWithFilter(uint32_t scannerId, const NearlinkSleScanSettings &settings,
        const std::vector<NearlinkSleScanFilter> &filters) override
    {
        (void)settings;
        startWithFilterCount++;
        lastScannerId = scannerId;
        lastFilterCount = filters.size();
        lastFilterFrameTypes.clear();
        for (const auto &filter : filters) {
            lastFilterFrameTypes.push_back(filter.GetFrameType());
        }
        return startRet;
    }

    NlErrCode StartFullScan(uint32_t scannerId, const NearlinkSleScanSettings &settings) override
    {
        (void)scannerId;
        (void)settings;
        startFullScanCount++;
        return NL_NO_ERROR;
    }

    NlErrCode StopScan(uint32_t scannerId) override
    {
        stopScanCount++;
        lastScannerId = scannerId;
        return NL_NO_ERROR;
    }
};

sptr<MockSleCentralManagerService> g_mockService = nullptr;

void InjectMockCentralManagerService()
{
    if (g_mockService == nullptr) {
        g_mockService = new MockSleCentralManagerService();
    }
    // private=public 直插 SA 管理器 profile map: GetProxy 优先命中 map, 不触达真实 samgr
    NearlinkBaseSaManager::GetInstance().profileRemoteMap_.EnsureInsert(SLE_CENTRAL_MANAGER_SERVER,
        g_mockService);
}
} // namespace

// 记录型 Fake: 实现框架侧回调, 记录转换后的扫描结果
class FakeSleCentralManagerCallback : public SleCentralManagerCallback {
public:
    void OnScanCallback(const SleScanResult &result) override
    {
        scanCount_++;
        last_ = result;
    }

    void OnSleBatchScanResultsEvent(const std::vector<SleScanResult> &results) override
    {
        (void)results;
    }

    void OnStartOrStopScanEvent(int resultCode, bool isStartScan) override
    {
        startStopCount_++;
        lastResultCode_ = resultCode;
        lastIsStartScan_ = isStartScan;
    }

    int scanCount_ = 0;
    int startStopCount_ = 0;
    int lastResultCode_ = -1;
    bool lastIsStartScan_ = false;
    SleScanResult last_;
};

class SleScannerFrameworkTest : public testing::Test {
public:
    std::shared_ptr<FakeSleCentralManagerCallback> fakeCallback_;
    std::shared_ptr<SleCentralManager> manager_;

    static void SetUpTestCase()
    {
        NearlinkAccessTokenMock::SetNativeTokenInfo();
        if (!NearlinkHost::GetInstance().IsSleEnabled()) {
            NearlinkHost::GetInstance().EnableNl();
        }
        ASSERT_TRUE(WaitForSleEnabled()) << "SLE enable not finished in 20s";
        InjectMockCentralManagerService();
    }

    static void TearDownTestCase()
    {
        g_mockService = nullptr;
    }

    void SetUp() override
    {
        fakeCallback_ = std::make_shared<FakeSleCentralManagerCallback>();
        manager_ = SleCentralManager::CreateSleCentralManager(fakeCallback_);
        ASSERT_NE(nullptr, manager_);
        ASSERT_NE(nullptr, manager_->pimpl->callbackImp_);
    }

    void TearDown() override
    {
        manager_ = nullptr;
        fakeCallback_ = nullptr;
    }

    // 构造服务侧上报的扫描结果 parcel(帧型可变); 出参填充: parcel 拷贝构造为 explicit 不可按值返回
    void FillScanResult(NearlinkSleScanResult &result, uint8_t primFrameType)
    {
        result.SetPeripheralDevice(RawAddress(TEST_DEVICE_ADDR));
        result.SetRssi(-42);
        result.SetConnectable(true);
        result.SetAdvertiseFlag(0x02);
        result.SetName(TEST_DEVICE_NAME);
        result.SetDeviceClass(TEST_DEVICE_CLASS);
        result.SetPayload(std::string("\x01\x02\x03", 3)); // scan_def 侧是 string
        result.SetPrimFrameType(primFrameType);
        result.AddManufacturerData(TEST_MANUFACTURER_ID, std::string("\xAA\xBB", 2));
        result.AddServiceUuid(Uuid::ConvertFromString(UUID_STRING));
        result.AddServiceData(Uuid::ConvertFromString(UUID_STRING), std::string("\xCC", 1));
    }
};

/**
 * @tc.number: SleScannerFramework_OnScanCallback_001
 * @tc.name: OnScanCallback primFrameType=TYPE_1 转换与字段透传
 * @tc.desc: 驱动 impl 回调桩: 原始帧型 0 -> SleScanFrameType::SLE_SCAN_FRAME_TYPE_1, 全字段映射
 */
HWTEST_F(SleScannerFrameworkTest, SleScannerFramework_OnScanCallback_001, TestSize.Level1)
{
    NearlinkSleScanResult result;
    FillScanResult(result, PRIM_FRAME_TYPE_1);
    manager_->pimpl->callbackImp_->OnScanCallback(result);

    ASSERT_EQ(1, fakeCallback_->scanCount_);
    EXPECT_EQ(static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_1),
        fakeCallback_->last_.GetFrameType());
    EXPECT_EQ(-42, fakeCallback_->last_.GetRssi());
    EXPECT_TRUE(fakeCallback_->last_.IsConnectable());
    EXPECT_EQ(0x02, fakeCallback_->last_.GetAdvertiseFlag());
    EXPECT_EQ(TEST_DEVICE_NAME, fakeCallback_->last_.GetName());
    EXPECT_EQ(TEST_DEVICE_CLASS, fakeCallback_->last_.GetDeviceClass());
    ASSERT_EQ(3UL, fakeCallback_->last_.GetPayload().size());
    EXPECT_EQ(0x01, fakeCallback_->last_.GetPayload()[0]);
    ASSERT_EQ(1UL, fakeCallback_->last_.GetManufacturerData().size());
    EXPECT_NE(fakeCallback_->last_.GetManufacturerData().end(),
        fakeCallback_->last_.GetManufacturerData().find(TEST_MANUFACTURER_ID));
    ASSERT_EQ(1UL, fakeCallback_->last_.GetServiceUuids().size());
    ASSERT_EQ(1UL, fakeCallback_->last_.GetServiceData().size());
    EXPECT_EQ(TEST_DEVICE_ADDR, fakeCallback_->last_.GetPeripheralDevice().GetDeviceAddr());
}

/**
 * @tc.number: SleScannerFramework_OnScanCallback_002
 * @tc.name: OnScanCallback primFrameType=TYPE_4 转换
 * @tc.desc: 原始帧型 1 -> SleScanFrameType::SLE_SCAN_FRAME_TYPE_4
 */
HWTEST_F(SleScannerFrameworkTest, SleScannerFramework_OnScanCallback_002, TestSize.Level1)
{
    NearlinkSleScanResult result;
    FillScanResult(result, PRIM_FRAME_TYPE_4);
    manager_->pimpl->callbackImp_->OnScanCallback(result);

    ASSERT_EQ(1, fakeCallback_->scanCount_);
    EXPECT_EQ(static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_4),
        fakeCallback_->last_.GetFrameType());
}

/**
 * @tc.number: SleScannerFramework_OnScanCallback_003
 * @tc.name: OnScanCallback 非法 primFrameType 回退 TYPE_1
 * @tc.desc: 原始帧型 0x05 走告警腿, 转换结果仍回退为 SLE_SCAN_FRAME_TYPE_1
 */
HWTEST_F(SleScannerFrameworkTest, SleScannerFramework_OnScanCallback_003, TestSize.Level1)
{
    NearlinkSleScanResult result;
    FillScanResult(result, PRIM_FRAME_TYPE_INVALID);
    manager_->pimpl->callbackImp_->OnScanCallback(result);

    ASSERT_EQ(1, fakeCallback_->scanCount_);
    EXPECT_EQ(static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_1),
        fakeCallback_->last_.GetFrameType());
}

/**
 * @tc.number: SleScannerFramework_OnStartOrStopScanEvent_001
 * @tc.name: OnStartOrStopScanEvent 转发
 * @tc.desc: 驱动 impl 回调桩: 结果码与开关标志透传到框架回调
 */
HWTEST_F(SleScannerFrameworkTest, SleScannerFramework_OnStartOrStopScanEvent_001, TestSize.Level1)
{
    manager_->pimpl->callbackImp_->OnStartOrStopScanEvent(0, true);
    ASSERT_EQ(1, fakeCallback_->startStopCount_);
    EXPECT_EQ(0, fakeCallback_->lastResultCode_);
    EXPECT_TRUE(fakeCallback_->lastIsStartScan_);

    manager_->pimpl->callbackImp_->OnStartOrStopScanEvent(201, false);
    ASSERT_EQ(2, fakeCallback_->startStopCount_);
    EXPECT_EQ(201, fakeCallback_->lastResultCode_);
    EXPECT_FALSE(fakeCallback_->lastIsStartScan_);
}

/**
 * @tc.number: SleScannerFramework_ConvertSleScanFilter_001
 * @tc.name: 过滤器组合矩阵: 全字段填充转换
 * @tc.desc: 直调 impl::ConvertSleScanFilter, 全部 Has 与 empty 判定走 true 腿并核对逐字段映射
 */
HWTEST_F(SleScannerFrameworkTest, SleScannerFramework_ConvertSleScanFilter_001, TestSize.Level1)
{
    SleScanFilter filter;
    UUID uuid = UUID::FromString(UUID_STRING);
    filter.SetDeviceId(TEST_DEVICE_ADDR);
    filter.SetName(TEST_DEVICE_NAME);
    filter.SetServiceUuid(uuid);
    filter.SetServiceUuidMask(uuid);
    filter.SetServiceSolicitationUuid(uuid);
    filter.SetServiceSolicitationUuidMask(uuid);
    filter.SetServiceData(std::vector<uint8_t>{0x01, 0x02});
    filter.SetServiceDataMask(std::vector<uint8_t>{0x03, 0x04});
    filter.SetRssiThreshold(-60);
    filter.SetManufacturerId(TEST_MANUFACTURER_ID);
    filter.SetManufactureData(std::vector<uint8_t>{0x05, 0x06});
    filter.SetManufactureDataMask(std::vector<uint8_t>{0x07, 0x08});
    filter.SetSensorHubChannel(true);

    std::vector<NearlinkSleScanFilter> out;
    manager_->pimpl->ConvertSleScanFilter({filter}, out);
    ASSERT_EQ(1UL, out.size());

    const auto &converted = out[0];
    EXPECT_EQ(TEST_DEVICE_ADDR, converted.GetDeviceId());
    EXPECT_EQ(TEST_DEVICE_NAME, converted.GetName());
    // 服务侧 Uuid 与框架 UUID 经 ToString 往返一致
    EXPECT_TRUE(converted.GetServiceUuid() == Uuid::ConvertFromString(uuid.ToString()));
    EXPECT_TRUE(converted.GetServiceUuidMask() == Uuid::ConvertFromString(uuid.ToString()));
    EXPECT_TRUE(converted.GetServiceSolicitationUuid() == Uuid::ConvertFromString(uuid.ToString()));
    EXPECT_TRUE(converted.GetServiceSolicitationUuidMask() == Uuid::ConvertFromString(uuid.ToString()));
    EXPECT_EQ(std::vector<uint8_t>({0x01, 0x02}), converted.GetServiceData());
    EXPECT_EQ(std::vector<uint8_t>({0x03, 0x04}), converted.GetServiceDataMask());
    EXPECT_EQ(-60, converted.GetRssiThreshold());
    EXPECT_EQ(TEST_MANUFACTURER_ID, converted.GetManufacturerId());
    EXPECT_EQ(std::vector<uint8_t>({0x05, 0x06}), converted.GetManufactureData());
    EXPECT_EQ(std::vector<uint8_t>({0x07, 0x08}), converted.GetManufactureDataMask());
    EXPECT_TRUE(converted.IsSensorHubChannel());
}

/**
 * @tc.number: SleScannerFramework_ConvertSleScanFilter_002
 * @tc.name: 过滤器组合矩阵: 裸过滤器转换
 * @tc.desc: 默认构造过滤器, 全部 Has 与 empty 判定走 false 腿, 可选字段不写入
 */
HWTEST_F(SleScannerFrameworkTest, SleScannerFramework_ConvertSleScanFilter_002, TestSize.Level1)
{
    SleScanFilter bare;
    std::vector<NearlinkSleScanFilter> out;
    manager_->pimpl->ConvertSleScanFilter({bare}, out);
    ASSERT_EQ(1UL, out.size());

    const auto &converted = out[0];
    EXPECT_TRUE(converted.GetManufactureData().empty());
    EXPECT_TRUE(converted.GetManufactureDataMask().empty());
    // 设备号/厂商ID/传感器hub仍无条件拷贝
    EXPECT_EQ(0, converted.GetManufacturerId());
    EXPECT_FALSE(converted.IsSensorHubChannel());
}

/**
 * @tc.number: SleScannerFramework_StartScanWithFilter_001
 * @tc.name: FRAME_TYPE_4 设置下过滤器帧型全量改写
 * @tc.desc: mock 代理记录下发过滤器; 设置帧型 TYPE_4 时每个转换后过滤器的 frameType 被改写为 4 型
 */
HWTEST_F(SleScannerFrameworkTest, SleScannerFramework_StartScanWithFilter_001, TestSize.Level1)
{
    const uint32_t scannerId = 5;
    {
        std::lock_guard<std::mutex> lock(manager_->pimpl->scannerIdMutex_);
        manager_->pimpl->scannerId_ = scannerId; // 注册为异步, 直写保证确定可达网关后代码
    }
    g_mockService->startWithFilterCount = 0;

    SleScanSettings settings;
    settings.SetFrameType(static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_4));
    SleScanFilter filter;
    std::vector<SleScanFilter> filters{filter};

    NlErrCode ret = manager_->StartScanWithFilter(settings, filters);
    EXPECT_EQ(NL_NO_ERROR, ret);
    ASSERT_EQ(1, g_mockService->startWithFilterCount);
    EXPECT_EQ(scannerId, g_mockService->lastScannerId);
    ASSERT_EQ(1UL, g_mockService->lastFilterCount);
    ASSERT_EQ(1UL, g_mockService->lastFilterFrameTypes.size());
    EXPECT_EQ(static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_4),
        g_mockService->lastFilterFrameTypes[0]);
}

/**
 * @tc.number: SleScannerFramework_StartScanWithFilter_002
 * @tc.name: FRAME_TYPE_1 设置下过滤器帧型保持默认
 * @tc.desc: 设置帧型 TYPE_1 时不改写过滤器 frameType(false 腿)
 */
HWTEST_F(SleScannerFrameworkTest, SleScannerFramework_StartScanWithFilter_002, TestSize.Level1)
{
    const uint32_t scannerId = 6;
    {
        std::lock_guard<std::mutex> lock(manager_->pimpl->scannerIdMutex_);
        manager_->pimpl->scannerId_ = scannerId;
    }
    g_mockService->startWithFilterCount = 0;

    SleScanSettings settings;
    settings.SetFrameType(static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_1));
    SleScanFilter filter;
    std::vector<SleScanFilter> filters{filter};

    NlErrCode ret = manager_->StartScanWithFilter(settings, filters);
    EXPECT_EQ(NL_NO_ERROR, ret);
    ASSERT_EQ(1, g_mockService->startWithFilterCount);
    ASSERT_EQ(1UL, g_mockService->lastFilterCount);
    ASSERT_EQ(1UL, g_mockService->lastFilterFrameTypes.size());
    EXPECT_EQ(static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_1),
        g_mockService->lastFilterFrameTypes[0]);
}

/**
 * @tc.number: SleScannerFramework_StartScanWithFilter_003
 * @tc.name: 非法帧型被网关拒绝
 * @tc.desc: 帧型越界(0/越上界)在代理调用前被拒绝, mock 不被触达
 */
HWTEST_F(SleScannerFrameworkTest, SleScannerFramework_StartScanWithFilter_003, TestSize.Level1)
{
    const uint32_t scannerId = 7;
    {
        std::lock_guard<std::mutex> lock(manager_->pimpl->scannerIdMutex_);
        manager_->pimpl->scannerId_ = scannerId;
    }
    g_mockService->startWithFilterCount = 0;

    SleScanSettings settings;
    settings.SetFrameType(0); // 下界之下
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, manager_->StartScanWithFilter(settings, {}));
    settings.SetFrameType(static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_4) + 1); // 上界之上
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, manager_->StartScanWithFilter(settings, {}));
    EXPECT_EQ(0, g_mockService->startWithFilterCount);
}

/**
 * @tc.number: SleScannerFramework_SetScanMode_001
 * @tc.name: 扫描模式入参范围校验
 * @tc.desc: 越下界/越上界返回 RET_BAD_PARAM, 合法值存取一致
 */
HWTEST_F(SleScannerFrameworkTest, SleScannerFramework_SetScanMode_001, TestSize.Level1)
{
    SleScanSettings settings;
    EXPECT_EQ(static_cast<int>(ReturnValue::RET_BAD_PARAM), settings.SetScanMode(SCAN_MODE_LOW_POWER - 1));
    EXPECT_EQ(static_cast<int>(ReturnValue::RET_BAD_PARAM), settings.SetScanMode(SCAN_MODE_INVALID));
    EXPECT_EQ(static_cast<int>(ReturnValue::RET_NO_ERROR), settings.SetScanMode(SCAN_MODE_BALANCED));
    EXPECT_EQ(SCAN_MODE_BALANCED, settings.GetScanMode());
    EXPECT_EQ(static_cast<int>(ReturnValue::RET_NO_ERROR), settings.SetScanMode(SCAN_MODE_LOW_LATENCY));
    EXPECT_EQ(SCAN_MODE_LOW_LATENCY, settings.GetScanMode());
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
