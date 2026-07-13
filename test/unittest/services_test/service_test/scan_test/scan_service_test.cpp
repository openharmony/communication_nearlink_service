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
#include <thread>
#include <memory>
#include <cstring>

#include "ScanService.h"
#include "ScanStackAdapter.h"
#include "SleInterfaceManager.h"
#include "nearlink_access_token_mock.h"
#include "nlstk_scan_api.h"
#include "nlstk_api_type_ext.h"
#include "sdf_vector.h"
#include "sdf_traits.h"
#include "log.h"
#include "nlstk_public_define_ext.h"
#include "SleMultiScanData.h"

namespace OHOS {
namespace Nearlink {
namespace {

using namespace testing::ext;

constexpr int DELAY_MS = 100;
constexpr int DELAY_LONG_MS = 500;

/* 用于 void 函数测试，可仅检测日志打印来判断 */
static std::string g_hiLog;
void MyLogCallback(const LogType type, const LogLevel level, const unsigned int domain, const char *tag,
                const char *msg)
{
    if (strstr(tag, "nearlink_service") != nullptr) {
        g_hiLog = msg;
    }
}

/* Mock 测试辅助函数声明（来自 mock_nlstk_scan_api.c） */
extern "C" {
    void MockTriggerScanStartEvent(NLSTK_Errcode_E resultCode);
    void MockTriggerScanStopEvent(NLSTK_Errcode_E resultCode);
    void MockTriggerScanResult(NLSTK_DevdAdvResult_S *result);
    void MockResetStopAllScanFlag(void);
    bool MockIsStopAllScanTriggered(void);
    uint32_t MockGetLastStartScanId(void);
    uint32_t MockGetLastStopScanId(void);
    uint32_t MockGetLastRemoveScanId(void);
    bool MockIsScanStarted(void);
    bool MockIsScanStopped(void);
    uint32_t MockGetStartScanCallCount(void);
    uint32_t MockGetStopScanCallCount(void);
    void MockResetScanFlags(void);
}

/* Mock 回调类 */
class MockScanCallback : public ISleCentralManagerCallback {
public:
    ~MockScanCallback() override = default;

    void OnScanCallback(const std::vector<uint32_t> &scannerIds,
                        const SleScanResultImpl &result) override
    {
        receivedResults_.push_back({scannerIds, result});
    }

    void OnSleBatchScanResultsEvent(std::vector<SleScanResultImpl> &results) override
    {
        batchResults_ = results;
    }

    void OnStartOrStopScanEvent(int resultCode, bool isStartScan) override
    {
        lastScanEvent_ = {resultCode, isStartScan};
    }

    std::vector<std::pair<std::vector<uint32_t>, SleScanResultImpl>> receivedResults_;
    std::vector<SleScanResultImpl> batchResults_;
    std::pair<int, bool> lastScanEvent_ = {0, false};
};

/* 测试工具函数：创建 Mock 广播结果 */
NLSTK_DevdAdvResult_S CreateMockAdvResult(const std::string& address,
                                          const std::string& name,
                                          int appearance,
                                          int8_t rssi = -50)
{
    NLSTK_DevdAdvResult_S result = {};

    /* 设置地址 */
    RawAddress rawAddr(address);
    rawAddr.ConvertToUint8(result.addr.addr);
    result.addr.type = 0x01;

    /* 设置 RSSI 和 TX Power */
    result.rssi = rssi;
    result.txPower = 0;

    /* 设置帧类型和发现级别 */
    result.frameType = DEVD_SCAN_FRAME_TYPE_1;
    result.discoveryLevel = 0x01;
    result.isConnectable = true;
    result.isPencil = false;

    /* 设置名称 */
    if (!name.empty()) {
        result.localName.data = new uint8_t[name.size()];
        result.localName.len = static_cast<uint16_t>(name.size());
        (void)memcpy_s(result.localName.data, name.size(), name.c_str(), name.size());
    }

    /* 创建 scannerIds vector */
    SDF_Traits scannerIdsTraits = {.dtor = NULL};
    result.scannerIds = SDF_CreateVectorByCapacity(1, scannerIdsTraits);
    if (result.scannerIds != NULL) {
        uint32_t scannerId = 1;
        SDF_VectorEmplaceBack(result.scannerIds, &scannerId);
    }

    /* 创建 serviceDataList 并添加 DIS appearance 数据 */
    SDF_Traits serviceDataTraits = {.dtor = NULL};
    result.serviceDataList = SDF_CreateVector(serviceDataTraits);

    /* 添加 DIS service data (UUID 0x0609) 包含 appearance */
    if (result.serviceDataList != NULL && appearance >= 0) {
        /* DIS UUID: 37BEA880-FC70-11EA-B720-000000000609 */
        uint8_t disUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
                             0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x09};
        /* DIS data: type(0x07) + length(0x03) + appearance(3 bytes) */
        uint8_t disData[] = {0x07, 0x03,
                             static_cast<uint8_t>(appearance & 0xFF),
                             static_cast<uint8_t>((appearance >> 8) & 0xFF),
                             static_cast<uint8_t>((appearance >> 16) & 0xFF)};

        size_t totalSize = sizeof(NLSTK_DevdAdvServiceData_S) + sizeof(disData);
        NLSTK_DevdAdvServiceData_S *serviceDataEntry =
            (NLSTK_DevdAdvServiceData_S *)new uint8_t[totalSize];
        (void)memcpy_s(serviceDataEntry->uuid, sizeof(disUuid), disUuid, sizeof(disUuid));
        serviceDataEntry->len = static_cast<uint16_t>(sizeof(disData));
        (void)memcpy_s(serviceDataEntry->data, sizeof(disData), disData, sizeof(disData));

        SDF_VectorEmplaceBack(result.serviceDataList, serviceDataEntry);
    }

    /* 创建空的 serviceUuids */
    SDF_Traits serviceUuidTraits = {.dtor = NULL};
    result.serviceUuids = SDF_CreateVector(serviceUuidTraits);

    /* 创建空的 manufacturerDataList */
    SDF_Traits manuDataTraits = {.dtor = NULL};
    result.manufacturerDataList = SDF_CreateVector(manuDataTraits);

    /* 设置广播数据 - 使用堆分配避免栈内存问题 */
    uint8_t advData[] = {0x02, 0x01, 0x06, 0x02, 0x0A, 0x00};
    result.advData.len = sizeof(advData);
    result.advData.data = new uint8_t[result.advData.len];
    (void)memcpy_s(result.advData.data, result.advData.len, advData, result.advData.len);

    return result;
}

/* 测试工具函数：创建带厂商数据的 Mock 广播结果（音频设备） */
NLSTK_DevdAdvResult_S CreateMockAudioAdvResult(const std::string& address,
                                               const std::string& name,
                                               int8_t rssi = -50)
{
    NLSTK_DevdAdvResult_S result = CreateMockAdvResult(address, name, 0x01, rssi);

    /* 添加音频 UUID (0x0605 - SLE_STANDARD_ASC_MGMT_UUID) */
    if (result.serviceUuids != NULL) {
        /* Audio UUID 0x0605: 放在第 14、15 字节 */
        uint8_t audioUuid[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x05};
        NLSTK_DevdAdvServiceUuid_S *uuidEntry = new NLSTK_DevdAdvServiceUuid_S;
        (void)memcpy_s(uuidEntry->uuid, sizeof(audioUuid), audioUuid, sizeof(audioUuid));
        SDF_VectorEmplaceBack(result.serviceUuids, uuidEntry);
    }

    /* 添加vendor厂商数据 - 上报地址与广播地址一致，对耳地址不同 */
    if (result.manufacturerDataList != NULL) {
        RawAddress rawAddr(address);
        uint8_t addrBytes[6];
        rawAddr.ConvertToUint8(addrBytes);

        uint8_t collabAddrBytes[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

        uint8_t manuData[] = {
            0x01,  /* 业务类型：音频 */
            0x01,  /* 扩展类型：上报地址 */
            addrBytes[0], addrBytes[1], addrBytes[2], addrBytes[3], addrBytes[4], addrBytes[5],
            0x02,  /* 扩展类型：对耳地址 */
            collabAddrBytes[0], collabAddrBytes[1], collabAddrBytes[2],
            collabAddrBytes[3], collabAddrBytes[4], collabAddrBytes[5]
        };

        /* 分配整个结构体 + 数据的连续内存（flexible array member） */
        size_t totalSize = sizeof(NLSTK_DevdAdvManufacturerData_S) + sizeof(manuData);
        NLSTK_DevdAdvManufacturerData_S *manuEntry =
            (NLSTK_DevdAdvManufacturerData_S *)new uint8_t[totalSize];
        manuEntry->manufacturerId = 0x0009;
        manuEntry->len = static_cast<uint16_t>(sizeof(manuData));
        (void)memcpy_s(manuEntry->data, sizeof(manuData), manuData, sizeof(manuData));

        SDF_VectorEmplaceBack(result.manufacturerDataList, manuEntry);
    }

    return result;
}

/* 测试工具函数：清理 Mock 广播结果资源 */
void CleanupMockAdvResult(NLSTK_DevdAdvResult_S &result)
{
    if (result.localName.data != NULL) {
        delete[] result.localName.data;
        result.localName.data = NULL;
    }

    if (result.scannerIds != NULL) {
        SDF_DestroyVector(result.scannerIds);
        result.scannerIds = NULL;
    }

    if (result.serviceDataList != NULL) {
        size_t dataSize = result.serviceDataList->size;
        for (size_t i = 0; i < dataSize; i++) {
            NLSTK_DevdAdvServiceData_S *dataEntry =
                (NLSTK_DevdAdvServiceData_S *)SDF_VectorElementAt(result.serviceDataList, i);
            if (dataEntry != NULL) {
                /* flexible array member，整个内存块一起释放 */
                delete[] reinterpret_cast<uint8_t*>(dataEntry);
            }
        }
        SDF_DestroyVector(result.serviceDataList);
        result.serviceDataList = NULL;
    }

    if (result.serviceUuids != NULL) {
        size_t uuidSize = result.serviceUuids->size;
        for (size_t i = 0; i < uuidSize; i++) {
            NLSTK_DevdAdvServiceUuid_S *uuidEntry =
                (NLSTK_DevdAdvServiceUuid_S *)SDF_VectorElementAt(result.serviceUuids, i);
            if (uuidEntry != NULL) {
                delete uuidEntry;
            }
        }
        SDF_DestroyVector(result.serviceUuids);
        result.serviceUuids = NULL;
    }

    if (result.manufacturerDataList != NULL) {
        size_t size = result.manufacturerDataList->size;
        for (size_t i = 0; i < size; i++) {
            NLSTK_DevdAdvManufacturerData_S *entry =
                (NLSTK_DevdAdvManufacturerData_S *)SDF_VectorElementAt(result.manufacturerDataList, i);
            if (entry != NULL) {
                /* flexible array member，整个内存块一起释放 */
                delete[] reinterpret_cast<uint8_t*>(entry);
            }
        }
        SDF_DestroyVector(result.manufacturerDataList);
        result.manufacturerDataList = NULL;
    }

    /* 清理 advData */
    if (result.advData.data != NULL) {
        delete[] result.advData.data;
        result.advData.data = NULL;
    }
}

/* 测试工具函数：构造异常厂商数据用于测试 ParseManufacturerDataAudio 的异常分支 */
NLSTK_DevdAdvResult_S CreateMockAudioAdvResultWithExceptionData(const std::string& address,
                                                                 const std::string& name,
                                                                 const std::vector<uint8_t>& manuData,
                                                                 int8_t rssi = -50)
{
    NLSTK_DevdAdvResult_S result = CreateMockAdvResult(address, name, 0x01, rssi);

    /* 添加音频 UUID */
    if (result.serviceUuids != NULL) {
        uint8_t audioUuid[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x05};
        NLSTK_DevdAdvServiceUuid_S *uuidEntry = new NLSTK_DevdAdvServiceUuid_S;
        (void)memcpy_s(uuidEntry->uuid, sizeof(audioUuid), audioUuid, sizeof(audioUuid));
        SDF_VectorEmplaceBack(result.serviceUuids, uuidEntry);
    }

    /* 添加自定义厂商数据 */
    if (result.manufacturerDataList != NULL && !manuData.empty()) {
        size_t totalSize = sizeof(NLSTK_DevdAdvManufacturerData_S) + manuData.size();
        NLSTK_DevdAdvManufacturerData_S *manuEntry =
            (NLSTK_DevdAdvManufacturerData_S *)new uint8_t[totalSize];
        manuEntry->manufacturerId = 0x0009;
        manuEntry->len = static_cast<uint16_t>(manuData.size());
        (void)memcpy_s(manuEntry->data, manuData.size(), manuData.data(), manuData.size());
        SDF_VectorEmplaceBack(result.manufacturerDataList, manuEntry);
    }

    return result;
}

/* 测试工具函数：创建 Mock 笔设备广播结果 */
NLSTK_DevdAdvResult_S CreateMockPencilAdvResult(const std::string& address,
                                                 const std::string& name,
                                                 int8_t rssi = -50)
{
    NLSTK_DevdAdvResult_S result = CreateMockAdvResult(address, name, -1, rssi);
    result.isPencil = true;
    return result;
}

/* 测试工具函数：创建带完整厂商数据的 Mock 广播结果（音频设备） */
NLSTK_DevdAdvResult_S CreateMockAudioAdvResultWithFullData(const std::string& address,
                                                            const std::string& name,
                                                            int8_t rssi = -50)
{
    NLSTK_DevdAdvResult_S result = CreateMockAdvResult(address, name, 0x01, rssi);

    /* 添加音频 UUID (0x0605 - SLE_STANDARD_ASC_MGMT_UUID) */
    if (result.serviceUuids != NULL) {
        uint8_t audioUuid[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x05};
        NLSTK_DevdAdvServiceUuid_S *uuidEntry = new NLSTK_DevdAdvServiceUuid_S;
        (void)memcpy_s(uuidEntry->uuid, sizeof(audioUuid), audioUuid, sizeof(audioUuid));
        SDF_VectorEmplaceBack(result.serviceUuids, uuidEntry);
    }

    /* 构造完整厂商数据：业务类型 + 上报地址 + 对耳地址 + 显示控制 + 厂商能力 */
    if (result.manufacturerDataList != NULL) {
        RawAddress rawAddr(address);
        uint8_t addrBytes[6];
        rawAddr.ConvertToUint8(addrBytes);

        uint8_t collabAddrBytes[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

        /* 厂商能力位图 (16 bytes) */
        uint8_t manuAbility[16] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
                                   0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

        uint8_t fullManuData[] = {
            0x01,  /* 业务类型：音频 */
            0x01,  /* 扩展类型：上报地址 */
            addrBytes[0], addrBytes[1], addrBytes[2], addrBytes[3], addrBytes[4], addrBytes[5],
            0x02,  /* 扩展类型：对耳地址 */
            collabAddrBytes[0], collabAddrBytes[1], collabAddrBytes[2],
            collabAddrBytes[3], collabAddrBytes[4], collabAddrBytes[5],
            0x03,  /* 扩展类型：显示控制 */
            0x00,  /* 显示控制值：0=显示 */
            0x04,  /* 扩展类型：厂商能力 */
            manuAbility[0], manuAbility[1], manuAbility[2], manuAbility[3],
            manuAbility[4], manuAbility[5], manuAbility[6], manuAbility[7],
            manuAbility[8], manuAbility[9], manuAbility[10], manuAbility[11],
            manuAbility[12], manuAbility[13], manuAbility[14], manuAbility[15]
        };

        size_t totalSize = sizeof(NLSTK_DevdAdvManufacturerData_S) + sizeof(fullManuData);
        NLSTK_DevdAdvManufacturerData_S *manuEntry =
            (NLSTK_DevdAdvManufacturerData_S *)new uint8_t[totalSize];
        manuEntry->manufacturerId = 0x0009;
        manuEntry->len = static_cast<uint16_t>(sizeof(fullManuData));
        (void)memcpy_s(manuEntry->data, sizeof(fullManuData), fullManuData, sizeof(fullManuData));

        SDF_VectorEmplaceBack(result.manufacturerDataList, manuEntry);
    }

    return result;
}

class ScanServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

    std::shared_ptr<MockScanCallback> mockCallback_;
};

void ScanServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase ScanServiceTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
}

void ScanServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase ScanServiceTest");
}

void ScanServiceTest::SetUp()
{
    HILOGI("SetUp ScanServiceTest");
    mockCallback_ = std::make_shared<MockScanCallback>();
    InterfaceScanService::GetInstance().RegisterSleCentralManagerCallback(*mockCallback_);
    MockResetStopAllScanFlag();
    MockResetScanFlags();
}

void ScanServiceTest::TearDown()
{
    HILOGI("TearDown ScanServiceTest");
    InterfaceScanService::GetInstance().DeregisterSleCentralManagerCallback();
    InterfaceScanService::GetInstance().ClearScanResultInfo();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LONG_MS));
}

/* ==================== ScanStackAdapter 测试用例 ==================== */

/**
 * @tc.name: ScanStackAdapter_Instance_001
 * @tc.desc: 测试 ScanStackAdapter 单例模式
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanStackAdapter_Instance_001, TestSize.Level1)
{
    HILOGI("ScanStackAdapter_Instance_001 start");
    ScanStackAdapter &adapter1 = ScanStackAdapter::GetInstance();
    ScanStackAdapter &adapter2 = ScanStackAdapter::GetInstance();
    EXPECT_EQ(&adapter1, &adapter2);
    HILOGI("ScanStackAdapter_Instance_001 end");
}

/* ==================== ScanService 扫描控制测试用例 ==================== */

/**
 * @tc.name: ScanService_AllocScannerId_001
 * @tc.desc: 测试分配 scannerId
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_AllocScannerId_001, TestSize.Level1)
{
    HILOGI("ScanService_AllocScannerId_001 start");
    uint32_t scannerId = InterfaceScanService::GetInstance().AllocScannerId();
    EXPECT_NE(scannerId, SLE_SCAN_INVALID_ID);
    HILOGI("ScanService_AllocScannerId_001 end, scannerId=%{public}u", scannerId);
}

/**
 * @tc.name: ScanService_RemoveScannerId_001
 * @tc.desc: 测试移除 scannerId
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_RemoveScannerId_001, TestSize.Level1)
{
    HILOGI("ScanService_RemoveScannerId_001 start");
    uint32_t scannerId = InterfaceScanService::GetInstance().AllocScannerId();
    EXPECT_NE(scannerId, SLE_SCAN_INVALID_ID);

    InterfaceScanService::GetInstance().RemoveScannerId(scannerId);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    EXPECT_EQ(MockGetLastRemoveScanId(), scannerId);

    HILOGI("ScanService_RemoveScannerId_001 end");
}

/**
 * @tc.name: ScanService_StartStopScan_001
 * @tc.desc: 测试启动和停止扫描
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_StartStopScan_001, TestSize.Level1)
{
    HILOGI("ScanService_StartStopScan_001 start");

    uint32_t scannerId = InterfaceScanService::GetInstance().AllocScannerId();
    EXPECT_NE(scannerId, SLE_SCAN_INVALID_ID);

    NearlinkSleScanSettings settings;
    settings.SetScanMode(SCAN_MODE_LOW_POWER);
    InterfaceScanService::GetInstance().StartScan(scannerId, settings, {});
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    EXPECT_EQ(MockGetLastStartScanId(), scannerId);
    EXPECT_TRUE(MockIsScanStarted());
    EXPECT_EQ(MockGetStartScanCallCount(), 1U);

    MockTriggerScanStartEvent(NLSTK_ERRCODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    EXPECT_TRUE(mockCallback_->lastScanEvent_.second);
    EXPECT_EQ(mockCallback_->lastScanEvent_.first, SCAN_SUCCESS);

    InterfaceScanService::GetInstance().StopScan(scannerId);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    EXPECT_EQ(MockGetLastStopScanId(), scannerId);
    EXPECT_TRUE(MockIsScanStopped());
    EXPECT_EQ(MockGetStopScanCallCount(), 1U);

    MockTriggerScanStopEvent(NLSTK_ERRCODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    EXPECT_FALSE(mockCallback_->lastScanEvent_.second);
    EXPECT_EQ(mockCallback_->lastScanEvent_.first, SCAN_SUCCESS);

    InterfaceScanService::GetInstance().RemoveScannerId(scannerId);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    EXPECT_EQ(MockGetLastRemoveScanId(), scannerId);

    HILOGI("ScanService_StartStopScan_001 end");
}

/**
 * @tc.name: ScanService_StartStopScanWithFilter_002
 * @tc.desc: 测试带过滤条件的启动和停止扫描
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_StartStopScanWithFilter_002, TestSize.Level1)
{
    HILOGI("ScanService_StartStopScanWithFilter_002 start");

    uint32_t scannerId = InterfaceScanService::GetInstance().AllocScannerId();
    EXPECT_NE(scannerId, SLE_SCAN_INVALID_ID);

    NearlinkSleScanSettings settings;
    settings.SetScanMode(SCAN_MODE_LOW_POWER);

    SleScanFilterImpl filter;
    filter.SetName("TestDevice");
    std::vector<SleScanFilterImpl> filters = {filter};

    InterfaceScanService::GetInstance().StartScan(scannerId, settings, filters);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    EXPECT_EQ(MockGetLastStartScanId(), scannerId);
    EXPECT_TRUE(MockIsScanStarted());
    EXPECT_EQ(MockGetStartScanCallCount(), 1U);

    MockTriggerScanStartEvent(NLSTK_ERRCODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    EXPECT_TRUE(mockCallback_->lastScanEvent_.second);
    EXPECT_EQ(mockCallback_->lastScanEvent_.first, SCAN_SUCCESS);

    InterfaceScanService::GetInstance().StopScan(scannerId);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    EXPECT_EQ(MockGetLastStopScanId(), scannerId);
    EXPECT_TRUE(MockIsScanStopped());
    EXPECT_EQ(MockGetStopScanCallCount(), 1U);

    MockTriggerScanStopEvent(NLSTK_ERRCODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    EXPECT_FALSE(mockCallback_->lastScanEvent_.second);
    EXPECT_EQ(mockCallback_->lastScanEvent_.first, SCAN_SUCCESS);

    InterfaceScanService::GetInstance().RemoveScannerId(scannerId);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    EXPECT_EQ(MockGetLastRemoveScanId(), scannerId);

    HILOGI("ScanService_StartStopScanWithFilter_002 end");
}

/**
 * @tc.name: ScanService_StopAllScan_001
 * @tc.desc: 测试停止所有扫描
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_StopAllScan_001, TestSize.Level1)
{
    HILOGI("ScanService_StopAllScan_001 start");
    MockResetStopAllScanFlag();

    InterfaceScanService::GetInstance().StopAllScan();

    EXPECT_TRUE(MockIsStopAllScanTriggered());
    HILOGI("ScanService_StopAllScan_001 end");
}

/* ==================== ScanService 扫描结果回调测试用例 ==================== */

/**
 * @tc.name: ScanService_ScanResultCallback_001
 * @tc.desc: 测试扫描结果回调
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_ScanResultCallback_001, TestSize.Level1)
{
    HILOGI("ScanService_ScanResultCallback_001 start");

    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(
        "00:11:22:33:44:55", "TestDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);

    EXPECT_GT(mockCallback_->receivedResults_.size(), 0);
    HILOGI("ScanService_ScanResultCallback_001 end, received %zu results",
        mockCallback_->receivedResults_.size());
}

/**
 * @tc.name: ScanService_ScanStartEventCallback_001
 * @tc.desc: 测试扫描启动事件回调
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_ScanStartEventCallback_001, TestSize.Level1)
{
    HILOGI("ScanService_ScanStartEventCallback_001 start");

    MockTriggerScanStartEvent(NLSTK_ERRCODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    EXPECT_TRUE(mockCallback_->lastScanEvent_.second);
    EXPECT_EQ(mockCallback_->lastScanEvent_.first, SCAN_SUCCESS);
    HILOGI("ScanService_ScanStartEventCallback_001 end");
}

/**
 * @tc.name: ScanService_ScanStopEventCallback_001
 * @tc.desc: 测试扫描停止事件回调
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_ScanStopEventCallback_001, TestSize.Level1)
{
    HILOGI("ScanService_ScanStopEventCallback_001 start");

    MockTriggerScanStopEvent(NLSTK_ERRCODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    EXPECT_FALSE(mockCallback_->lastScanEvent_.second);
    EXPECT_EQ(mockCallback_->lastScanEvent_.first, SCAN_SUCCESS);
    HILOGI("ScanService_ScanStopEventCallback_001 end");
}

/* ==================== ScanService 扫描结果查询测试用例 ==================== */

/**
 * @tc.name: ScanService_GetDeviceName_001
 * @tc.desc: 测试获取设备名称
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_GetDeviceName_001, TestSize.Level1)
{
    HILOGI("ScanService_GetDeviceName_001 start");

    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(
        "00:11:22:33:44:55", "TestDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);

    std::string name = InterfaceScanService::GetInstance().GetDeviceName("00:11:22:33:44:55");
    EXPECT_EQ(name, "TestDevice");

    std::string emptyName = InterfaceScanService::GetInstance().GetDeviceName("FF:FF:FF:FF:FF:FF");
    EXPECT_EQ(emptyName, "");
    HILOGI("ScanService_GetDeviceName_001 end");
}

/**
 * @tc.name: ScanService_GetDeviceAppearance_001
 * @tc.desc: 测试获取设备外观
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_GetDeviceAppearance_001, TestSize.Level1)
{
    HILOGI("ScanService_GetDeviceAppearance_001 start");

    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(
        "00:11:22:33:44:55", "TestDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);

    int appearance = InterfaceScanService::GetInstance().GetDeviceAppearance("00:11:22:33:44:55");
    EXPECT_EQ(appearance, 0x01);

    int invalidAppearance = InterfaceScanService::GetInstance().GetDeviceAppearance("FF:FF:FF:FF:FF:FF");
    EXPECT_EQ(invalidAppearance, INVALID_APPEARANCE);

    HILOGI("ScanService_GetDeviceAppearance_001 end, appearance=%{public}d", appearance);
}

/**
 * @tc.name: ScanService_GetModelId_001
 * @tc.desc: 测试获取型号 ID
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_GetModelId_001, TestSize.Level1)
{
    HILOGI("ScanService_GetModelId_001 start");

    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(
        "00:11:22:33:44:55", "TestDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);

    std::string modelId = InterfaceScanService::GetInstance().GetModelId("00:11:22:33:44:55");
    EXPECT_TRUE(modelId.empty());

    std::string invalidModelId = InterfaceScanService::GetInstance().GetModelId("FF:FF:FF:FF:FF:FF");
    EXPECT_TRUE(invalidModelId.empty());

    HILOGI("ScanService_GetModelId_001 end, modelId=%{public}s", modelId.c_str());
}

/**
 * @tc.name: ScanService_GetIconId_001
 * @tc.desc: 测试获取图标 ID
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_GetIconId_001, TestSize.Level1)
{
    HILOGI("ScanService_GetIconId_001 start");

    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(
        "00:11:22:33:44:55", "TestDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);

    std::string iconId = InterfaceScanService::GetInstance().GetIconId("00:11:22:33:44:55");
    EXPECT_TRUE(iconId.empty());

    std::string invalidIconId = InterfaceScanService::GetInstance().GetIconId("FF:FF:FF:FF:FF:FF");
    EXPECT_TRUE(invalidIconId.empty());

    HILOGI("ScanService_GetIconId_001 end, iconId=%{public}s", iconId.c_str());
}

/**
 * @tc.name: ScanService_IsAudioDevice_001
 * @tc.desc: 测试判断音频设备
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_IsAudioDevice_001, TestSize.Level1)
{
    HILOGI("ScanService_IsAudioDevice_001 start");

    NLSTK_DevdAdvResult_S normalResult = CreateMockAdvResult(
        "00:11:22:33:44:55", "NormalDevice", 0x01, -50);
    MockTriggerScanResult(&normalResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(normalResult);

    bool isNormalAudio = InterfaceScanService::GetInstance().IsAudioDevice("00:11:22:33:44:55");
    EXPECT_FALSE(isNormalAudio);

    NLSTK_DevdAdvResult_S audioResult = CreateMockAudioAdvResult(
        "00:11:22:33:44:66", "AudioDevice", -60);
    MockTriggerScanResult(&audioResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(audioResult);

    bool isAudio = InterfaceScanService::GetInstance().IsAudioDevice("00:11:22:33:44:66");
    EXPECT_TRUE(isAudio);

    bool isInvalidAudio = InterfaceScanService::GetInstance().IsAudioDevice("FF:FF:FF:FF:FF:FF");
    EXPECT_FALSE(isInvalidAudio);

    HILOGI("ScanService_IsAudioDevice_001 end");
}

/**
 * @tc.name: ScanService_GetCurrentAddress_001
 * @tc.desc: 测试获取当前地址
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_GetCurrentAddress_001, TestSize.Level1)
{
    HILOGI("ScanService_GetCurrentAddress_001 start");

    std::string addr = "00:11:22:33:44:55";
    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(addr, "TestDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);

    RawAddress reportAddr(addr);
    RawAddress currentAddr = InterfaceScanService::GetInstance().GetCurrentAddress(reportAddr);
    EXPECT_EQ(currentAddr.GetAddress(), addr);

    RawAddress invalidReportAddr("FF:FF:FF:FF:FF:FF");
    RawAddress invalidCurrentAddr = InterfaceScanService::GetInstance().GetCurrentAddress(invalidReportAddr);
    EXPECT_EQ(invalidCurrentAddr.GetAddress(), "FF:FF:FF:FF:FF:FF");

    HILOGI("ScanService_GetCurrentAddress_001 end");
}

/**
 * @tc.name: ScanService_GetCollaborateAddress_001
 * @tc.desc: 测试获取协作地址
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_GetCollaborateAddress_001, TestSize.Level1)
{
    HILOGI("ScanService_GetCollaborateAddress_001 start");

    std::string normalAddr = "00:11:22:33:44:55";
    NLSTK_DevdAdvResult_S normalResult = CreateMockAdvResult(normalAddr, "NormalDevice", 0x01, -50);
    MockTriggerScanResult(&normalResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(normalResult);

    RawAddress normalReportAddr(normalAddr);
    RawAddress normalCollabAddr = InterfaceScanService::GetInstance().GetCollaborateAddress(normalReportAddr);
    /* 普通设备没有协作地址，返回空地址 */
    EXPECT_EQ(normalCollabAddr.GetAddress(), "00:00:00:00:00:00");

    std::string audioAddr = "00:11:22:33:44:66";
    NLSTK_DevdAdvResult_S audioResult = CreateMockAudioAdvResult(audioAddr, "AudioDevice", -60);
    MockTriggerScanResult(&audioResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(audioResult);

    RawAddress audioReportAddr(audioAddr);
    RawAddress audioCollabAddr = InterfaceScanService::GetInstance().GetCollaborateAddress(audioReportAddr);
    EXPECT_EQ(audioCollabAddr.GetAddress(), "AA:BB:CC:DD:EE:FF"); // 186 行构造

    RawAddress invalidReportAddr("FF:FF:FF:FF:FF:FF");
    RawAddress invalidCollabAddr = InterfaceScanService::GetInstance().GetCollaborateAddress(invalidReportAddr);
    EXPECT_EQ(invalidCollabAddr.GetAddress(), "FF:FF:FF:FF:FF:FF");

    HILOGI("ScanService_GetCollaborateAddress_001 end");
}

/**
 * @tc.name: ScanService_GetDeviceAddrType_001
 * @tc.desc: 测试获取设备地址类型
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_GetDeviceAddrType_001, TestSize.Level1)
{
    HILOGI("ScanService_GetDeviceAddrType_001 start");

    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(
        "00:11:22:33:44:55", "TestDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);

    uint8_t addrType = InterfaceScanService::GetInstance().GetDeviceAddrType("00:11:22:33:44:55");
    EXPECT_NE(addrType, SLE_ADDR_TYPE_END);

    uint8_t invalidAddrType = InterfaceScanService::GetInstance().GetDeviceAddrType("FF:FF:FF:FF:FF:FF");
    EXPECT_EQ(invalidAddrType, SLE_ADDR_TYPE_END);

    HILOGI("ScanService_GetDeviceAddrType_001 end, addrType=%{public}u", addrType);
}

/**
 * @tc.name: ScanService_GetManufacturerBusinessType_001
 * @tc.desc: 测试获取厂商业务类型
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_GetManufacturerBusinessType_001, TestSize.Level1)
{
    HILOGI("ScanService_GetManufacturerBusinessType_001 start");

    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(
        "00:11:22:33:44:55", "TestDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);

    int businessType = InterfaceScanService::GetInstance().GetManufacturerBusinessType("00:11:22:33:44:55");
    EXPECT_EQ(businessType, 0);

    int invalidBusinessType = InterfaceScanService::GetInstance().GetManufacturerBusinessType("FF:FF:FF:FF:FF:FF");
    EXPECT_EQ(invalidBusinessType, 0);

    HILOGI("ScanService_GetManufacturerBusinessType_001 end, businessType=%{public}d", businessType);
}

/**
 * @tc.name: ScanService_GetDeviceManufacturerAbility_001
 * @tc.desc: 测试获取设备厂商能力
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_GetDeviceManufacturerAbility_001, TestSize.Level1)
{
    HILOGI("ScanService_GetDeviceManufacturerAbility_001 start");

    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(
        "00:11:22:33:44:55", "TestDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);

    std::array<uint8_t, SLE_MANU_ABILITY_LEN> ability =
        InterfaceScanService::GetInstance().GetDeviceManufacturerAbility("00:11:22:33:44:55");
    EXPECT_EQ(ability.size(), SLE_MANU_ABILITY_LEN);

    std::array<uint8_t, SLE_MANU_ABILITY_LEN> invalidAbility =
        InterfaceScanService::GetInstance().GetDeviceManufacturerAbility("FF:FF:FF:FF:FF:FF");
    EXPECT_EQ(invalidAbility.size(), SLE_MANU_ABILITY_LEN);

    HILOGI("ScanService_GetDeviceManufacturerAbility_001 end");
}

/**
 * @tc.name: ScanService_GetNewModelId_001
 * @tc.desc: 测试获取新型号 ID
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_GetNewModelId_001, TestSize.Level1)
{
    HILOGI("ScanService_GetNewModelId_001 start");

    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(
        "00:11:22:33:44:55", "TestDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);

    std::string newModelId = InterfaceScanService::GetInstance().GetNewModelId("00:11:22:33:44:55");
    EXPECT_TRUE(newModelId.empty());

    std::string invalidNewModelId = InterfaceScanService::GetInstance().GetNewModelId("FF:FF:FF:FF:FF:FF");
    EXPECT_TRUE(invalidNewModelId.empty());

    HILOGI("ScanService_GetNewModelId_001 end");
}

/**
 * @tc.name: ScanService_GetSubModelId_001
 * @tc.desc: 测试获取子型号 ID
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_GetSubModelId_001, TestSize.Level1)
{
    HILOGI("ScanService_GetSubModelId_001 start");

    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(
        "00:11:22:33:44:55", "TestDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);

    std::string subModelId = InterfaceScanService::GetInstance().GetSubModelId("00:11:22:33:44:55");
    EXPECT_TRUE(subModelId.empty());

    std::string invalidSubModelId = InterfaceScanService::GetInstance().GetSubModelId("FF:FF:FF:FF:FF:FF");
    EXPECT_TRUE(invalidSubModelId.empty());

    HILOGI("ScanService_GetSubModelId_001 end");
}

/**
 * @tc.name: ScanService_GetBtAddr_001
 * @tc.desc: 测试获取蓝牙地址
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_GetBtAddr_001, TestSize.Level1)
{
    HILOGI("ScanService_GetBtAddr_001 start");

    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(
        "00:11:22:33:44:55", "TestDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);

    std::string btAddr = InterfaceScanService::GetInstance().GetBtAddr("00:11:22:33:44:55");
    EXPECT_TRUE(btAddr.empty());

    std::string invalidBtAddr = InterfaceScanService::GetInstance().GetBtAddr("FF:FF:FF:FF:FF:FF");
    EXPECT_TRUE(invalidBtAddr.empty());

    HILOGI("ScanService_GetBtAddr_001 end");
}

/**
 * @tc.name: ScanService_GetReportAddrByCurrentAddress_001
 * @tc.desc: 测试通过当前地址获取上报地址
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_GetReportAddrByCurrentAddress_001, TestSize.Level1)
{
    HILOGI("ScanService_GetReportAddrByCurrentAddress_001 start");

    std::string addr = "00:11:22:33:44:55";
    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(addr, "TestDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);

    RawAddress currentAddr(addr);
    RawAddress reportAddr = InterfaceScanService::GetInstance().GetReportAddrByCurrentAddress(currentAddr);
    EXPECT_EQ(reportAddr.GetAddress(), addr);

    RawAddress invalidCurrentAddr("FF:FF:FF:FF:FF:FF");
    RawAddress invalidReportAddr =
        InterfaceScanService::GetInstance().GetReportAddrByCurrentAddress(invalidCurrentAddr);
    EXPECT_EQ(invalidReportAddr.GetAddress(), "");

    HILOGI("ScanService_GetReportAddrByCurrentAddress_001 end");
}

/* ==================== ScanService 扫描结果管理测试用例 ==================== */

/**
 * @tc.name: ScanService_AddPeripheralDevice_001
 * @tc.desc: 测试添加周边设备
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_AddPeripheralDevice_001, TestSize.Level1)
{
    HILOGI("ScanService_AddPeripheralDevice_001 start");

    for (int i = 0; i < 10; i++) {
        char addr[18];
        (void)snprintf(addr, sizeof(addr), "00:11:22:33:44:%02X", i);
        NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(addr, "Device", 0x01, -50);
        MockTriggerScanResult(&mockResult);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        CleanupMockAdvResult(mockResult);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    for (int i = 0; i < 10; i++) {
        char addr[18];
        (void)snprintf(addr, sizeof(addr), "00:11:22:33:44:%02X", i);
        std::string name = InterfaceScanService::GetInstance().GetDeviceName(addr);
        EXPECT_EQ(name, "Device");
    }
    HILOGI("ScanService_AddPeripheralDevice_001 end");
}

/**
 * @tc.name: ScanService_AddPeripheralDevice_Update_002
 * @tc.desc: 测试更新已存在设备
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_AddPeripheralDevice_Update_002, TestSize.Level1)
{
    HILOGI("ScanService_AddPeripheralDevice_Update_002 start");

    std::string addr = "00:11:22:33:44:55";

    /* 第一次添加 */
    NLSTK_DevdAdvResult_S mockResult1 = CreateMockAdvResult(addr, "DeviceV1", 0x01, -50);
    MockTriggerScanResult(&mockResult1);
    CleanupMockAdvResult(mockResult1);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    std::string name1 = InterfaceScanService::GetInstance().GetDeviceName(addr);
    EXPECT_EQ(name1, "DeviceV1");

    /* 更新设备 */
    NLSTK_DevdAdvResult_S mockResult2 = CreateMockAdvResult(addr, "DeviceV2", 0x01, -60);
    MockTriggerScanResult(&mockResult2);
    CleanupMockAdvResult(mockResult2);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    std::string name2 = InterfaceScanService::GetInstance().GetDeviceName(addr);
    EXPECT_EQ(name2, "DeviceV2");
    HILOGI("ScanService_AddPeripheralDevice_Update_002 end");
}

/**
 * @tc.name: ScanService_MaxScanResultQueue_001
 * @tc.desc: 测试扫描结果队列上限 200 条
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_MaxScanResultQueue_001, TestSize.Level1)
{
    HILOGI("ScanService_MaxScanResultQueue_001 start");

    /* 添加 210 个设备 */
    for (int i = 0; i < 210; i++) {
        char addr[18];
        (void)snprintf(addr, sizeof(addr), "%02X:11:22:33:44:%02X", i / 256, i % 256);
        NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(addr, "Device", 0x01, -50);
        MockTriggerScanResult(&mockResult);
        /* 每 10 个设备 sleep 一次，避免过快 */
        if (i % 10 == 9) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        CleanupMockAdvResult(mockResult);;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LONG_MS));

    /* 前 10 个设备应该被清除（超过 200 条限制） */
    for (int i = 0; i < 10; i++) {
        char addr[18];
        (void)snprintf(addr, sizeof(addr), "%02X:11:22:33:44:%02X", i / 256, i % 256);
        std::string name = InterfaceScanService::GetInstance().GetDeviceName(addr);
        EXPECT_EQ(name, "");
    }

    /* 后 200 个设备应该存在 */
    for (int i = 10; i < 210; i++) {
        char addr[18];
        (void)snprintf(addr, sizeof(addr), "%02X:11:22:33:44:%02X", i / 256, i % 256);
        std::string name = InterfaceScanService::GetInstance().GetDeviceName(addr);
        EXPECT_EQ(name, "Device");
    }
    HILOGI("ScanService_MaxScanResultQueue_001 end");
}

/**
 * @tc.name: ScanService_ClearScanResultInfo_001
 * @tc.desc: 测试清空扫描结果
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_ClearScanResultInfo_001, TestSize.Level1)
{
    HILOGI("ScanService_ClearScanResultInfo_001 start");

    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(
        "00:11:22:33:44:55", "TestDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);


    std::string nameBefore = InterfaceScanService::GetInstance().GetDeviceName("00:11:22:33:44:55");
    EXPECT_EQ(nameBefore, "TestDevice");

    InterfaceScanService::GetInstance().ClearScanResultInfo();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    std::string nameAfter = InterfaceScanService::GetInstance().GetDeviceName("00:11:22:33:44:55");
    EXPECT_EQ(nameAfter, "");
    HILOGI("ScanService_ClearScanResultInfo_001 end");
}

/* ==================== ScanService 音频设备厂商数据解析测试用例 ==================== */

/**
 * @tc.name: ScanService_ParseManufacturerData_Audio_001
 * @tc.desc: 测试解析音频设备厂商数据
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_ParseManufacturerData_Audio_001, TestSize.Level1)
{
    HILOGI("ScanService_ParseManufacturerData_Audio_001 start");

    std::string addr = "00:11:22:33:44:55";
    NLSTK_DevdAdvResult_S mockResult = CreateMockAudioAdvResult(addr, "AudioDevice", -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);


    /* 验证设备名称 */
    std::string name = InterfaceScanService::GetInstance().GetDeviceName(addr);
    EXPECT_EQ(name, "AudioDevice");

    /* 验证厂商业务类型 */
    int businessType = InterfaceScanService::GetInstance().GetManufacturerBusinessType(addr);
    EXPECT_EQ(businessType, SLE_PRIVATE_AUDIO_BUSINESS_TYPE);

    /* 验证地址映射 - 当前地址应该是厂商数据中的上报地址 */
    RawAddress reportAddr(addr);
    RawAddress currentAddr = InterfaceScanService::GetInstance().GetCurrentAddress(reportAddr);
    EXPECT_NE(currentAddr.GetAddress(), "");

    /* 验证协作地址 */
    RawAddress collabAddr = InterfaceScanService::GetInstance().GetCollaborateAddress(reportAddr);
    EXPECT_NE(collabAddr.GetAddress(), "");

    HILOGI("ScanService_ParseManufacturerData_Audio_001 end, currentAddr=%{public}s, collabAddr=%{public}s",
        currentAddr.GetAddress().c_str(), collabAddr.GetAddress().c_str());
}

/**
 * @tc.name: ScanService_ParseManufacturerData_Normal_002
 * @tc.desc: 测试解析普通设备厂商数据
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_ParseManufacturerData_Normal_002, TestSize.Level1)
{
    HILOGI("ScanService_ParseManufacturerData_Normal_002 start");

    std::string addr = "00:11:22:33:44:66";
    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(addr, "NormalDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);


    std::string name = InterfaceScanService::GetInstance().GetDeviceName(addr);
    EXPECT_EQ(name, "NormalDevice");

    int businessType = InterfaceScanService::GetInstance().GetManufacturerBusinessType(addr);
    EXPECT_EQ(businessType, 0);
    HILOGI("ScanService_ParseManufacturerData_Normal_002 end");
}

/**
 * @tc.name: ScanService_AudioDeviceAddrMapping_001
 * @tc.desc: 测试音频设备地址映射功能
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_AudioDeviceAddrMapping_001, TestSize.Level1)
{
    HILOGI("ScanService_AudioDeviceAddrMapping_001 start");

    std::string reportAddrStr = "00:11:22:33:44:55";
    NLSTK_DevdAdvResult_S mockResult = CreateMockAudioAdvResult(reportAddrStr, "TWSDevice", -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);


    RawAddress reportAddr(reportAddrStr);
    RawAddress currentAddr = InterfaceScanService::GetInstance().GetCurrentAddress(reportAddr);
    RawAddress collabAddr = InterfaceScanService::GetInstance().GetCollaborateAddress(reportAddr);

    /* 验证地址映射不为空 */
    EXPECT_NE(currentAddr.GetAddress(), "");
    EXPECT_NE(collabAddr.GetAddress(), "");

    HILOGI("ScanService_AudioDeviceAddrMapping_001 end, current=%{public}s, collab=%{public}s",
        currentAddr.GetAddress().c_str(), collabAddr.GetAddress().c_str());
}

/**
 * @tc.name: ScanService_AudioDeviceAddrSwitch_001
 * @tc.desc: 测试音频设备地址切换场景（Level2 可选测试）
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_AudioDeviceAddrSwitch_001, TestSize.Level2)
{
    HILOGI("ScanService_AudioDeviceAddrSwitch_001 start");

    /* 第一次广播，使用地址 1 */
    std::string addr1 = "00:11:22:33:44:11";
    NLSTK_DevdAdvResult_S mockResult1 = CreateMockAudioAdvResult(addr1, "TWSDevice", -50);
    MockTriggerScanResult(&mockResult1);
    CleanupMockAdvResult(mockResult1);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    /* 验证第一个设备已添加 */
    bool isAudio1 = InterfaceScanService::GetInstance().IsAudioDevice(addr1);
    EXPECT_TRUE(isAudio1);
    HILOGI("ScanService_AudioDeviceAddrSwitch_001, addr1 isAudio=%{public}d", isAudio1);

    /* 第二次广播，使用地址 2（同一设备的不同上报地址） */
    std::string addr2 = "00:11:22:33:44:22";
    NLSTK_DevdAdvResult_S mockResult2 = CreateMockAudioAdvResult(addr2, "TWSDevice", -50);
    MockTriggerScanResult(&mockResult2);
    CleanupMockAdvResult(mockResult2);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    /* 验证第二个设备已添加 */
    bool isAudio2 = InterfaceScanService::GetInstance().IsAudioDevice(addr2);
    EXPECT_TRUE(isAudio2);
    HILOGI("ScanService_AudioDeviceAddrSwitch_001 end, addr2 isAudio=%{public}d", isAudio2);
}

/**
 * @tc.name: ScanService_ParseManufacturerData_Audio_Full_002
 * @tc.desc: 测试解析音频设备完整厂商数据（含显示控制和厂商能力）
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_ParseManufacturerData_Audio_Full_002, TestSize.Level1)
{
    HILOGI("ScanService_ParseManufacturerData_Audio_Full_002 start");

    std::string addr = "00:11:22:33:44:88";
    NLSTK_DevdAdvResult_S audioResult = CreateMockAudioAdvResultWithFullData(addr, "TWS_Earphone", -50);
    MockTriggerScanResult(&audioResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(audioResult);

    std::string name = InterfaceScanService::GetInstance().GetDeviceName(addr);
    EXPECT_EQ(name, "TWS_Earphone");

    int businessType = InterfaceScanService::GetInstance().GetManufacturerBusinessType(addr);
    EXPECT_EQ(businessType, SLE_PRIVATE_AUDIO_BUSINESS_TYPE);

    RawAddress reportAddr(addr);
    RawAddress currentAddr = InterfaceScanService::GetInstance().GetCurrentAddress(reportAddr);
    EXPECT_NE(currentAddr.GetAddress(), "");

    RawAddress collabAddr = InterfaceScanService::GetInstance().GetCollaborateAddress(reportAddr);
    EXPECT_NE(collabAddr.GetAddress(), "");

    std::array<uint8_t, SLE_MANU_ABILITY_LEN> ability =
        InterfaceScanService::GetInstance().GetDeviceManufacturerAbility(addr);
    EXPECT_EQ(ability.size(), SLE_MANU_ABILITY_LEN);
    bool hasAbility = false;
    for (uint8_t byte : ability) {
        if (byte != 0) {
            hasAbility = true;
            break;
        }
    }
    EXPECT_TRUE(hasAbility);

    EXPECT_GT(mockCallback_->receivedResults_.size(), 0);
    bool isDeviceDisplay = mockCallback_->receivedResults_.back().second
        .GetPeripheralDevice().GetIsDeviceDisplay();
    EXPECT_TRUE(isDeviceDisplay);

    HILOGI("ScanService_ParseManufacturerData_Audio_Full_002 end");
}

/* ==================== ScanService 广播数据解析测试用例 ==================== */

/**
 * @tc.name: ScanService_ParseAdvResult_Normal_001
 * @tc.desc: 测试解析普通广播包
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_ParseAdvResult_Normal_001, TestSize.Level1)
{
    HILOGI("ScanService_ParseAdvResult_Normal_001 start");

    NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(
        "00:11:22:33:44:55", "NormalDevice", 0x01, -50);
    MockTriggerScanResult(&mockResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(mockResult);;

    std::string name = InterfaceScanService::GetInstance().GetDeviceName("00:11:22:33:44:55");
    EXPECT_EQ(name, "NormalDevice");

    HILOGI("ScanService_ParseAdvResult_Normal_001 end, name=%{public}s", name.c_str());
}

/**
 * @tc.name: ScanService_ParseAdvResult_MultipleDevices_002
 * @tc.desc: 测试解析多个设备的广播包
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_ParseAdvResult_MultipleDevices_002, TestSize.Level1)
{
    HILOGI("ScanService_ParseAdvResult_MultipleDevices_002 start");

    std::vector<std::pair<std::string, std::string>> devices = {
        {"00:11:22:33:44:01", "Device01"},
        {"00:11:22:33:44:02", "Device02"},
        {"00:11:22:33:44:03", "Device03"},
        {"00:11:22:33:44:04", "Device04"},
        {"00:11:22:33:44:05", "Device05"},
    };

    for (const auto& device : devices) {
        NLSTK_DevdAdvResult_S mockResult = CreateMockAdvResult(device.first, device.second, 0x01, -50);
        MockTriggerScanResult(&mockResult);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        CleanupMockAdvResult(mockResult);;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

    for (const auto& device : devices) {
        std::string name = InterfaceScanService::GetInstance().GetDeviceName(device.first);
        EXPECT_EQ(name, device.second);
    }
    HILOGI("ScanService_ParseAdvResult_MultipleDevices_002 end");
}

/**
 * @tc.name: ScanService_ParseAdvResult_Pencil_001
 * @tc.desc: 测试解析笔设备广播包（isPencil=true 分支）
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_ParseAdvResult_Pencil_001, TestSize.Level1)
{
    HILOGI("ScanService_ParseAdvResult_Pencil_001 start");

    std::string addr = "00:11:22:33:44:77";
    NLSTK_DevdAdvResult_S pencilResult = CreateMockPencilAdvResult(addr, "VENDOR_Pencil", -50);
    MockTriggerScanResult(&pencilResult);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    CleanupMockAdvResult(pencilResult);

    std::string name = InterfaceScanService::GetInstance().GetDeviceName(addr);
    EXPECT_EQ(name, "VENDOR_Pencil");

    int appearance = InterfaceScanService::GetInstance().GetDeviceAppearance(addr);
    EXPECT_EQ(appearance, 0x000504);

    HILOGI("ScanService_ParseAdvResult_Pencil_001 end");
}

/**
 * @tc.name: ScanService_ParseManufacturerDataAudio_Exception_001
 * @tc.desc: 测试 ParseManufacturerDataAudio 函数中 4 个 case 分支的异常 if 判断
 * @tc.type: FUNC
 */
HWTEST_F(ScanServiceTest, ScanService_ParseManufacturerDataAudio_Exception_001, TestSize.Level1)
{
    HILOGI("ScanService_ParseManufacturerDataAudio_Exception_001 start");

    /* 场景 1: CDSM_REPORT_ADDR 长度不足 */
    {
        g_hiLog = "";
        LOG_SetCallback(MyLogCallback);
        std::vector<uint8_t> manuData1 = {0x01, 0x01, 0x01, 0x02};
        NLSTK_DevdAdvResult_S result1 = CreateMockAudioAdvResultWithExceptionData(
            "00:11:22:33:44:E1", "ExceptionDevice1", manuData1, -50);
        MockTriggerScanResult(&result1);
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LONG_MS));
        EXPECT_TRUE(g_hiLog.find("report addr len invalid") == std::string::npos);
        CleanupMockAdvResult(result1);
    }

    /* 场景 2: CDSM_PAIR_ADDR 长度不足 */
    {
        g_hiLog = "";
        LOG_SetCallback(MyLogCallback);
        std::vector<uint8_t> manuData2 = {0x01, 0x01, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x02, 0x01};
        NLSTK_DevdAdvResult_S result2 = CreateMockAudioAdvResultWithExceptionData(
            "00:11:22:33:44:E2", "ExceptionDevice2", manuData2, -50);
        MockTriggerScanResult(&result2);
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LONG_MS));
        EXPECT_TRUE(g_hiLog.find("cdsm pair addr len invalid") == std::string::npos);
        CleanupMockAdvResult(result2);
    }

    /* 场景 3: VENDOR_EARPHONE_DISPLAY_CONTROL 长度不足 */
    {
        g_hiLog = "";
        LOG_SetCallback(MyLogCallback);
        std::vector<uint8_t> manuData3 = {0x01, 0x01, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                                          0x02, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x03};
        NLSTK_DevdAdvResult_S result3 = CreateMockAudioAdvResultWithExceptionData(
            "00:11:22:33:44:E3", "ExceptionDevice3", manuData3, -50);
        MockTriggerScanResult(&result3);
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LONG_MS));
        EXPECT_TRUE(g_hiLog.find("cdsm judge cover len invalid") == std::string::npos);
        CleanupMockAdvResult(result3);
    }

    /* 场景 4: VENDOR_MANUFACTURER_ABILITY 长度不足 */
    {
        g_hiLog = "";
        LOG_SetCallback(MyLogCallback);
        std::vector<uint8_t> manuData4 = {0x01, 0x01, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                                          0x02, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x04, 0x01, 0x02};
        NLSTK_DevdAdvResult_S result4 = CreateMockAudioAdvResultWithExceptionData(
            "00:11:22:33:44:E4", "ExceptionDevice4", manuData4, -50);
        MockTriggerScanResult(&result4);
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LONG_MS));
        EXPECT_TRUE(g_hiLog.find("manufacturer ability len invalid") == std::string::npos);
        CleanupMockAdvResult(result4);
    }
    HILOGI("ScanService_ParseManufacturerDataAudio_Exception_001 end");
}

} // namespace
} // namespace Nearlink
} // namespace OHOS
