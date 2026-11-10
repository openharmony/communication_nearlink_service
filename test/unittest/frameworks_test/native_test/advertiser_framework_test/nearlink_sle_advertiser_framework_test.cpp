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

// 目标: frameworks/native/nearlink_sle_advertiser.cpp 零覆盖分支(进程内驱动, 不依赖真实广播栈)
// 手法: 编入产品源文件(private=public) 直达 pimpl; 用产品 Stub 派生的 Mock 服务注入
//       NearlinkBaseSaManager::profileRemoteMap_(GetProxy 命中 map 即返回本地 Stub,
//       NewInstance 走 AsInterface 直连虚表), 覆盖 79/83/96/108/120/141/265/273/310/
//       325/327/337/339/377 行分支; 回调桩直驱 + StartAdvertisingImp 受控矩阵。

#include <gtest/gtest.h>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "nearlink_test_enable_wait.h"
#include "nearlink_access_token_mock.h"

#include "nearlink_base_sa_manager.h"
#include "nearlink_sle_advertiser_stub.h"
#include "nearlink_sle_advertiser.cpp"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
const std::string UUID_STRING = "11223344-0000-1000-8000-00805F9B34FB";
constexpr uint16_t TEST_MANUFACTURER_ID = 0x1234;
constexpr int32_t TEST_HANDLE = 3;
constexpr int32_t NEW_HANDLE = 2;

// Mock 服务: 派生产品 Stub, 覆写全部接口方法为受控返回+调用记录; 注入后 GetProxy 直连本对象
class MockSleAdvertiserService : public NearlinkSleAdvertiserStub {
public:
    NlErrCode getHandleRet = NL_NO_ERROR;
    int32_t getHandleOut = NEW_HANDLE;
    NlErrCode startAdvRet = NL_NO_ERROR;
    NlErrCode stopAdvRet = NL_NO_ERROR;
    NlErrCode setAdvDataRet = NL_NO_ERROR;
    int getHandleCount = 0;
    int startAdvCount = 0;
    int stopAdvCount = 0;
    int setAdvDataCount = 0;
    int32_t lastStartHandle = -1;
    size_t lastAdvManufacturerCount = 0;
    size_t lastScanRespManufacturerCount = 0;

    NlErrCode RegisterSleAdvertiserCallback(const sptr<INearlinkSleAdvertiseCallback> &callback) override
    {
        (void)callback;
        return NL_NO_ERROR;
    }

    NlErrCode DeregisterSleAdvertiserCallback(const sptr<INearlinkSleAdvertiseCallback> &callback) override
    {
        (void)callback;
        return NL_NO_ERROR;
    }

    NlErrCode StartAdvertising(const NearlinkSleAdvertiserSettings &settings,
        const NearlinkSleAdvertiserData &advData, const NearlinkSleAdvertiserData &scanResponse,
        int32_t advHandle) override
    {
        (void)settings;
        startAdvCount++;
        lastStartHandle = advHandle;
        lastAdvManufacturerCount = advData.GetManufacturerData().size();
        lastScanRespManufacturerCount = scanResponse.GetManufacturerData().size();
        return startAdvRet;
    }

    NlErrCode StopAdvertising(int32_t advHandle) override
    {
        (void)advHandle;
        stopAdvCount++;
        return stopAdvRet;
    }

    NlErrCode GetAdvertiserHandle(int32_t &advHandle) override
    {
        getHandleCount++;
        advHandle = getHandleOut;
        return getHandleRet;
    }

    NlErrCode SetAdvertisingData(const NearlinkSleAdvertiserData &advData,
        const NearlinkSleAdvertiserData &scanResponse, int32_t advHandle) override
    {
        (void)advData;
        (void)scanResponse;
        (void)advHandle;
        setAdvDataCount++;
        return setAdvDataRet;
    }

    NlErrCode EnableAdvertising(int32_t advHandle) override
    {
        (void)advHandle;
        return NL_NO_ERROR;
    }

    NlErrCode DisableAdvertising(int32_t advHandle) override
    {
        (void)advHandle;
        return NL_NO_ERROR;
    }
};

sptr<MockSleAdvertiserService> g_mockService = nullptr;

void InjectMockAdvertiserService()
{
    if (g_mockService == nullptr) {
        g_mockService = new MockSleAdvertiserService();
    }
    NearlinkBaseSaManager::GetInstance().profileRemoteMap_.EnsureInsert(SLE_ADVERTISER_SERVER, g_mockService);
}
} // namespace

// 记录型 Fake: 实现框架侧广播回调
class FakeSleAdvertiseCallback : public SleAdvertiseCallback {
public:
    void OnStartResultEvent(int result, int advHandle) override
    {
        startCount_++;
        lastStartResult_ = result;
        lastStartHandle_ = advHandle;
    }

    void OnEnableResultEvent(int result, int advHandle) override
    {
        enableCount_++;
        lastEnableResult_ = result;
        lastEnableHandle_ = advHandle;
    }

    void OnDisableResultEvent(int result, int advHandle) override
    {
        disableCount_++;
        lastDisableResult_ = result;
        lastDisableHandle_ = advHandle;
    }

    void OnStopResultEvent(int result, int advHandle) override
    {
        stopCount_++;
        lastStopResult_ = result;
        lastStopHandle_ = advHandle;
    }

    void OnSetAdvDataEvent(int result) override
    {
        setDataCount_++;
        lastSetDataResult_ = result;
    }

    void OnGetAdvHandleEvent(int result, int advHandle) override
    {
        getHandleCount_++;
        lastGetHandleResult_ = result;
        lastGetHandle_ = advHandle;
    }

    int startCount_ = 0;
    int lastStartResult_ = -1;
    int lastStartHandle_ = -1;
    int enableCount_ = 0;
    int lastEnableResult_ = -1;
    int lastEnableHandle_ = -1;
    int disableCount_ = 0;
    int lastDisableResult_ = -1;
    int lastDisableHandle_ = -1;
    int stopCount_ = 0;
    int lastStopResult_ = -1;
    int lastStopHandle_ = -1;
    int setDataCount_ = 0;
    int lastSetDataResult_ = -1;
    int getHandleCount_ = 0;
    int lastGetHandleResult_ = -1;
    int lastGetHandle_ = -1;
};

class SleAdvertiserFrameworkTest : public testing::Test {
public:
    std::shared_ptr<FakeSleAdvertiseCallback> fakeCallback_;
    std::shared_ptr<SleAdvertiser> advertiser_;

    static void SetUpTestCase()
    {
        NearlinkAccessTokenMock::SetNativeTokenInfo();
        if (!NearlinkHost::GetInstance().IsSleEnabled()) {
            NearlinkHost::GetInstance().EnableNl();
        }
        ASSERT_TRUE(WaitForSleEnabled()) << "SLE enable not finished in 20s";
        InjectMockAdvertiserService();
    }

    static void TearDownTestCase()
    {
        g_mockService = nullptr;
    }

    void SetUp() override
    {
        fakeCallback_ = std::make_shared<FakeSleAdvertiseCallback>();
        advertiser_ = SleAdvertiser::CreateSleAdvertiser();
        ASSERT_NE(nullptr, advertiser_);
        ASSERT_NE(nullptr, advertiser_->pimpl->callbackImp_);
    }

    void TearDown() override
    {
        advertiser_ = nullptr;
        fakeCallback_ = nullptr;
    }

    // 构造最小可用的 StartAdvertisingImp 入参
    SleAdvertiserStartParams MakeStartParams()
    {
        NearlinkSleAdvertiserSettings setting;
        NearlinkSleAdvertiserData advData;
        NearlinkSleAdvertiserData scanResp;
        return SleAdvertiserStartParams(setting, advData, scanResp);
    }
};

/**
 * @tc.number: SleAdvertiserFramework_Callback_001
 * @tc.name: 五类结果回调: 已注册 handle 转发
 * @tc.desc: 驱动 impl 回调桩: OnStart/Stop/Enable/Disable/SetAdvData 全部转发到框架回调
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_Callback_001, TestSize.Level1)
{
    advertiser_->pimpl->callbacks_.EnsureInsert(TEST_HANDLE, fakeCallback_);
    auto &imp = advertiser_->pimpl->callbackImp_;

    imp->OnStartResultEvent(0, TEST_HANDLE, 0);
    EXPECT_EQ(1, fakeCallback_->startCount_);
    EXPECT_EQ(0, fakeCallback_->lastStartResult_);
    EXPECT_EQ(TEST_HANDLE, fakeCallback_->lastStartHandle_);

    imp->OnEnableResultEvent(0, TEST_HANDLE);
    EXPECT_EQ(1, fakeCallback_->enableCount_);

    imp->OnDisableResultEvent(0, TEST_HANDLE);
    EXPECT_EQ(1, fakeCallback_->disableCount_);

    imp->OnSetAdvDataEvent(0, TEST_HANDLE);
    EXPECT_EQ(1, fakeCallback_->setDataCount_);

    // OnStopResultEvent 会擦除注册, 必须放在最后
    imp->OnStopResultEvent(0, TEST_HANDLE);
    EXPECT_EQ(1, fakeCallback_->stopCount_);
}

/**
 * @tc.number: SleAdvertiserFramework_Callback_002
 * @tc.name: 五类结果回调: 未注册 handle 不转发
 * @tc.desc: callbackSptr 为空的腿: 未注册 handle 静默返回, 不触达框架回调
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_Callback_002, TestSize.Level1)
{
    auto &imp = advertiser_->pimpl->callbackImp_;
    const int32_t unregisteredHandle = 88;

    imp->OnStartResultEvent(0, unregisteredHandle, 0);
    imp->OnStopResultEvent(0, unregisteredHandle);
    imp->OnEnableResultEvent(0, unregisteredHandle);
    imp->OnDisableResultEvent(0, unregisteredHandle);
    imp->OnSetAdvDataEvent(0, unregisteredHandle);

    EXPECT_EQ(0, fakeCallback_->startCount_);
    EXPECT_EQ(0, fakeCallback_->stopCount_);
    EXPECT_EQ(0, fakeCallback_->enableCount_);
    EXPECT_EQ(0, fakeCallback_->disableCount_);
    EXPECT_EQ(0, fakeCallback_->setDataCount_);
}

/**
 * @tc.number: SleAdvertiserFramework_Callback_003
 * @tc.name: 弱引用过期回调不转发
 * @tc.desc: map 中 weak_ptr 已过期 -> lock 为空 -> callbackSptr 空腿
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_Callback_003, TestSize.Level1)
{
    constexpr int32_t staleHandle = 66;
    {
        std::shared_ptr<FakeSleAdvertiseCallback> temp = std::make_shared<FakeSleAdvertiseCallback>();
        advertiser_->pimpl->callbacks_.EnsureInsert(staleHandle, temp);
    } // temp 释放, weak 过期

    advertiser_->pimpl->callbackImp_->OnStartResultEvent(0, staleHandle, 0);
    advertiser_->pimpl->callbackImp_->OnEnableResultEvent(0, staleHandle);
    EXPECT_EQ(0, fakeCallback_->startCount_);
    EXPECT_EQ(0, fakeCallback_->enableCount_);
}

/**
 * @tc.number: SleAdvertiserFramework_Callback_004
 * @tc.name: 参数校验失败结果: 转发同时移除注册
 * @tc.desc: OnStartResultEvent 收到 PARA_FAIL: map 中该 handle 被擦除, 回调仍收到结果
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_Callback_004, TestSize.Level1)
{
    advertiser_->pimpl->callbacks_.EnsureInsert(TEST_HANDLE, fakeCallback_);
    EXPECT_EQ(TEST_HANDLE, advertiser_->pimpl->GetAdvHandleImp(fakeCallback_));

    advertiser_->pimpl->callbackImp_->OnStartResultEvent(
        static_cast<int32_t>(AdvResult::ADV_RESULT_FAILED_CHECK_PARA_FAIL), TEST_HANDLE, 0);

    EXPECT_EQ(1, fakeCallback_->startCount_);
    EXPECT_EQ(static_cast<int32_t>(AdvResult::ADV_RESULT_FAILED_CHECK_PARA_FAIL),
        fakeCallback_->lastStartResult_);
    // 失败结果触发擦除: handle 不再可查
    EXPECT_EQ(static_cast<int32_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
        advertiser_->pimpl->GetAdvHandleImp(fakeCallback_));

    // 非失败结果不擦除
    advertiser_->pimpl->callbacks_.EnsureInsert(TEST_HANDLE, fakeCallback_);
    advertiser_->pimpl->callbackImp_->OnStartResultEvent(0, TEST_HANDLE, 0);
    EXPECT_EQ(TEST_HANDLE, advertiser_->pimpl->GetAdvHandleImp(fakeCallback_));
}

/**
 * @tc.number: SleAdvertiserFramework_Callback_005
 * @tc.name: OnStopResultEvent 后移除注册
 * @tc.desc: 停止结果事件总是擦除 handle, 且已取到的回调仍被转发
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_Callback_005, TestSize.Level1)
{
    advertiser_->pimpl->callbacks_.EnsureInsert(TEST_HANDLE, fakeCallback_);
    advertiser_->pimpl->callbackImp_->OnStopResultEvent(0, TEST_HANDLE);

    EXPECT_EQ(1, fakeCallback_->stopCount_);
    EXPECT_EQ(static_cast<int32_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
        advertiser_->pimpl->GetAdvHandleImp(fakeCallback_));
}

/**
 * @tc.number: SleAdvertiserFramework_GetAdvHandleImp_001
 * @tc.name: handle 查找: 相等/不相等两腿
 * @tc.desc: map 中回调相等返回 handle; 不相等继续遍历最终返回无效 handle
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_GetAdvHandleImp_001, TestSize.Level1)
{
    advertiser_->pimpl->callbacks_.EnsureInsert(TEST_HANDLE, fakeCallback_);
    EXPECT_EQ(TEST_HANDLE, advertiser_->pimpl->GetAdvHandleImp(fakeCallback_));

    std::shared_ptr<FakeSleAdvertiseCallback> other = std::make_shared<FakeSleAdvertiseCallback>();
    EXPECT_EQ(static_cast<int32_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
        advertiser_->pimpl->GetAdvHandleImp(other));

    // 空 map 也返回无效 handle
    advertiser_->pimpl->callbacks_.Erase(TEST_HANDLE);
    EXPECT_EQ(static_cast<int32_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
        advertiser_->pimpl->GetAdvHandleImp(fakeCallback_));
}

/**
 * @tc.number: SleAdvertiserFramework_TotalBytes_001
 * @tc.name: 广播数据字节数: flags/厂商数据/包含名/包含发射功率
 * @tc.desc: 直调 GetAdvertiserTotalBytes: includeDeviceName/includeTxPower 两腿 + 各字段长度算术
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_TotalBytes_001, TestSize.Level1)
{
    NearlinkSleAdvertiserData emptyData;
    // 空 + 不含 flags 头 = 0; 含 flags 头 = 3
    EXPECT_EQ(0U, advertiser_->pimpl->GetAdvertiserTotalBytes(emptyData, false));
    EXPECT_EQ(3U, advertiser_->pimpl->GetAdvertiserTotalBytes(emptyData, true));

    // 厂商数据: 2 头 + 2 厂商ID + 数据长
    NearlinkSleAdvertiserData withManufacturer;
    withManufacturer.AddManufacturerData(TEST_MANUFACTURER_ID, std::string(4, 'x'));
    EXPECT_EQ(3U + 2U + 2U + 4U, advertiser_->pimpl->GetAdvertiserTotalBytes(withManufacturer, true));

    // 包含设备名: 至少加 头2 + 名长(截断到 DEVICE_NAME_MAX_LEN)
    NearlinkSleAdvertiserData withName;
    withName.SetIncludeDeviceName(true);
    uint32_t base = advertiser_->pimpl->GetAdvertiserTotalBytes(NearlinkSleAdvertiserData(), false);
    uint32_t with = advertiser_->pimpl->GetAdvertiserTotalBytes(withName, false);
    EXPECT_GE(with - base, 2U);
    EXPECT_LE(with - base, 2U + static_cast<uint32_t>(DEVICE_NAME_MAX_LEN));

    // 包含发射功率: 固定加 3 字节
    NearlinkSleAdvertiserData withTxPower;
    withTxPower.SetIncludeTxPower(true);
    uint32_t withTx = advertiser_->pimpl->GetAdvertiserTotalBytes(withTxPower, false);
    EXPECT_EQ(3U, withTx - base);
}

/**
 * @tc.number: SleAdvertiserFramework_ImpMatrix_001
 * @tc.name: StartAdvertisingImp 矩阵: 新 handle 获取失败
 * @tc.desc: GetAdvertiserHandle 返回错误 -> 回调收到失败, map 不注册
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_ImpMatrix_001, TestSize.Level1)
{
    g_mockService->getHandleRet = NL_ERR_INTERNAL_ERROR;
    g_mockService->getHandleOut = NEW_HANDLE;
    g_mockService->startAdvRet = NL_NO_ERROR;
    g_mockService->startAdvCount = 0;
    g_mockService->getHandleCount = 0;

    SleAdvertiserStartParams params = MakeStartParams();
    NlErrCode ret = advertiser_->pimpl->StartAdvertisingImp(params,
        static_cast<int32_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE), fakeCallback_, true);

    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, ret);
    EXPECT_EQ(1, g_mockService->getHandleCount);
    EXPECT_EQ(0, g_mockService->startAdvCount);
    EXPECT_EQ(1, fakeCallback_->startCount_);
    EXPECT_EQ(static_cast<int32_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
        fakeCallback_->lastStartHandle_);
    EXPECT_EQ(static_cast<int32_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
        advertiser_->pimpl->GetAdvHandleImp(fakeCallback_));
}

/**
 * @tc.number: SleAdvertiserFramework_ImpMatrix_002
 * @tc.name: StartAdvertisingImp 矩阵: 新 handle 服务返回无效 handle
 * @tc.desc: GetAdvertiserHandle 成功但 handle 仍为无效 -> 失败腿, 回调收到失败
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_ImpMatrix_002, TestSize.Level1)
{
    g_mockService->getHandleRet = NL_NO_ERROR;
    g_mockService->getHandleOut = static_cast<int32_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE);
    g_mockService->startAdvRet = NL_NO_ERROR;
    g_mockService->startAdvCount = 0;

    SleAdvertiserStartParams params = MakeStartParams();
    NlErrCode ret = advertiser_->pimpl->StartAdvertisingImp(params,
        static_cast<int32_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE), fakeCallback_, true);

    EXPECT_EQ(NL_NO_ERROR, ret); // 服务侧返回值原样透出
    EXPECT_EQ(0, g_mockService->startAdvCount);
    EXPECT_EQ(1, fakeCallback_->startCount_);
    EXPECT_EQ(static_cast<int32_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
        fakeCallback_->lastStartHandle_);
}

/**
 * @tc.number: SleAdvertiserFramework_ImpMatrix_003
 * @tc.name: StartAdvertisingImp 矩阵: 新 handle 启动失败回滚注册
 * @tc.desc: StartAdvertising 返回错误且 isNewHandle -> 注册被擦除(339 true 腿)
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_ImpMatrix_003, TestSize.Level1)
{
    g_mockService->getHandleRet = NL_NO_ERROR;
    g_mockService->getHandleOut = NEW_HANDLE;
    g_mockService->startAdvRet = NL_ERR_INTERNAL_ERROR;
    g_mockService->startAdvCount = 0;

    SleAdvertiserStartParams params = MakeStartParams();
    NlErrCode ret = advertiser_->pimpl->StartAdvertisingImp(params, NEW_HANDLE, fakeCallback_, true);

    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, ret);
    EXPECT_EQ(1, g_mockService->startAdvCount);
    // 失败回滚: 注册未保留
    EXPECT_EQ(static_cast<int32_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
        advertiser_->pimpl->GetAdvHandleImp(fakeCallback_));
    EXPECT_EQ(0, fakeCallback_->getHandleCount_);
}

/**
 * @tc.number: SleAdvertiserFramework_ImpMatrix_004
 * @tc.name: StartAdvertisingImp 矩阵: 旧 handle 启动失败保留注册
 * @tc.desc: StartAdvertising 返回错误且非新 handle -> 注册保留(339 false 腿)
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_ImpMatrix_004, TestSize.Level1)
{
    advertiser_->pimpl->callbacks_.EnsureInsert(TEST_HANDLE, fakeCallback_);
    g_mockService->startAdvRet = NL_ERR_INTERNAL_ERROR;
    g_mockService->startAdvCount = 0;
    g_mockService->getHandleCount = 0;

    SleAdvertiserStartParams params = MakeStartParams();
    NlErrCode ret = advertiser_->pimpl->StartAdvertisingImp(params, TEST_HANDLE, fakeCallback_, false);

    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, ret);
    EXPECT_EQ(1, g_mockService->startAdvCount);
    EXPECT_EQ(0, g_mockService->getHandleCount); // 旧 handle 不再取号
    // 非新 handle: 注册保留
    EXPECT_EQ(TEST_HANDLE, advertiser_->pimpl->GetAdvHandleImp(fakeCallback_));
}

/**
 * @tc.number: SleAdvertiserFramework_ImpMatrix_005
 * @tc.name: StartAdvertisingImp 矩阵: 成功路径
 * @tc.desc: 取号成功+启动成功 -> 注册保留且回调收到 handle
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_ImpMatrix_005, TestSize.Level1)
{
    g_mockService->getHandleRet = NL_NO_ERROR;
    g_mockService->getHandleOut = NEW_HANDLE;
    g_mockService->startAdvRet = NL_NO_ERROR;
    g_mockService->startAdvCount = 0;

    SleAdvertiserStartParams params = MakeStartParams();
    NlErrCode ret = advertiser_->pimpl->StartAdvertisingImp(params,
        static_cast<int32_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE), fakeCallback_, true);

    EXPECT_EQ(NL_NO_ERROR, ret);
    EXPECT_EQ(1, g_mockService->startAdvCount);
    EXPECT_EQ(NEW_HANDLE, advertiser_->pimpl->GetAdvHandleImp(fakeCallback_));
    EXPECT_EQ(1, fakeCallback_->getHandleCount_);
    EXPECT_EQ(NEW_HANDLE, fakeCallback_->lastGetHandle_);
    EXPECT_EQ(0, fakeCallback_->startCount_);
}

/**
 * @tc.number: SleAdvertiserFramework_StartAdvertising_001
 * @tc.name: 公共入口: 已注册回调复用 handle
 * @tc.desc: callbacks_ 已有该回调 -> 377 true 腿: 不取新号, 以旧 handle 走非新路径
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_StartAdvertising_001, TestSize.Level1)
{
    advertiser_->pimpl->callbacks_.EnsureInsert(TEST_HANDLE, fakeCallback_);
    g_mockService->startAdvCount = 0;
    g_mockService->getHandleCount = 0;
    g_mockService->startAdvRet = NL_NO_ERROR;

    SleAdvertiserSettings settings;
    SleAdvertiserData advData;
    advData.AddManufacturerData(TEST_MANUFACTURER_ID, std::string(4, 'x'));
    SleAdvertiserData scanResponse;
    scanResponse.AddManufacturerData(TEST_MANUFACTURER_ID + 1, std::string(2, 'y'));

    NlErrCode ret = advertiser_->StartAdvertising(settings, advData, scanResponse, 0, fakeCallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);
    EXPECT_EQ(1, g_mockService->startAdvCount);
    EXPECT_EQ(TEST_HANDLE, g_mockService->lastStartHandle);
    EXPECT_EQ(0, g_mockService->getHandleCount); // 复用: 未取号
    EXPECT_EQ(1UL, g_mockService->lastAdvManufacturerCount); // 数据经 ConvertSleAdvertiserData 透传
    EXPECT_EQ(1UL, g_mockService->lastScanRespManufacturerCount);
    EXPECT_EQ(1, fakeCallback_->getHandleCount_);
    EXPECT_EQ(TEST_HANDLE, fakeCallback_->lastGetHandle_);
}

/**
 * @tc.number: SleAdvertiserFramework_StartAdvertising_002
 * @tc.name: 公共入口: 新回调取新号
 * @tc.desc: callbacks_ 无该回调 -> 377 false 腿: 以新 handle 路径取号启动
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_StartAdvertising_002, TestSize.Level1)
{
    g_mockService->startAdvCount = 0;
    g_mockService->getHandleCount = 0;
    g_mockService->getHandleRet = NL_NO_ERROR;
    g_mockService->getHandleOut = NEW_HANDLE;
    g_mockService->startAdvRet = NL_NO_ERROR;

    SleAdvertiserSettings settings;
    SleAdvertiserData advData;
    SleAdvertiserData scanResponse;
    NlErrCode ret = advertiser_->StartAdvertising(settings, advData, scanResponse, 0, fakeCallback_);

    EXPECT_EQ(NL_NO_ERROR, ret);
    EXPECT_EQ(1, g_mockService->getHandleCount);
    EXPECT_EQ(1, g_mockService->startAdvCount);
    EXPECT_EQ(NEW_HANDLE, g_mockService->lastStartHandle);
    EXPECT_EQ(NEW_HANDLE, advertiser_->pimpl->GetAdvHandleImp(fakeCallback_));
}

/**
 * @tc.number: SleAdvertiserFramework_StartAdvertising_003
 * @tc.name: 公共入口: 空 callback 被拒
 * @tc.desc: callback 为空指针 -> 参数错误返回, 不触达代理
 */
HWTEST_F(SleAdvertiserFrameworkTest, SleAdvertiserFramework_StartAdvertising_003, TestSize.Level1)
{
    g_mockService->startAdvCount = 0;
    SleAdvertiserSettings settings;
    SleAdvertiserData advData;
    SleAdvertiserData scanResponse;
    EXPECT_EQ(NL_ERR_INVALID_PARAM,
        advertiser_->StartAdvertising(settings, advData, scanResponse, 0, nullptr));
    EXPECT_EQ(0, g_mockService->startAdvCount);
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
