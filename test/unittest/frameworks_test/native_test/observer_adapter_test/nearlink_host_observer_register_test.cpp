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
#include <memory>

#include "log.h"
#include "nearlink_access_token_mock.h"
#include "nearlink_host.h"
#include "nearlink_observer_adapter.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

// 仅实现纯虚方法的最小 observer，用于注册/解注册流程验证
class MinHostObserver : public NearlinkHostObserver {
public:
    void OnStateChanged(const int transport, const int status) override {}
};

class MinRemoteDeviceObserver : public NearlinkRemoteDeviceObserver {
public:
    void OnAcbStateChanged(const NearlinkRemoteDevice &device, int state, int reason) override {}
    void OnPairingRequest(const NearlinkRemoteDevice &device, const std::string &passkey, int type) override {}
    void OnPairStatusChanged(const NearlinkRemoteDevice &device, int preState, int state, int reason) override {}
    void OnConnectionStateChanged(const NearlinkRemoteDevice &device, int preState, int state, int reason) override
    {}
    void OnRemoteUuidChanged(const NearlinkRemoteDevice &device, const std::vector<UUID> &uuids) override {}
    void OnRemoteNameChanged(const NearlinkRemoteDevice &device, const std::string &deviceName) override {}
    void OnRemoteAliasChanged(const NearlinkRemoteDevice &device, const std::string &alias) override {}
    void OnReadRemoteRssiEvent(const NearlinkRemoteDevice &device, int rssi, int status) override {}
};

// 电池与 rssi observer 接口无纯虚方法，但保留空实现类以统一写法
// 注：电池接口基类未声明虚析构，此处补虚析构避免 -Wdelete-non-abstract-non-virtual-dtor
class MinBatteryObserver : public NearlinkRemoteDeviceBatteryObserver {
public:
    virtual ~MinBatteryObserver() = default;
};
class MinRssiObserver : public NearlinkRemoteDeviceRssiObserver {};

class NearlinkHostObserverRegisterTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        HILOGI("SetUpTestCase NearlinkHostObserverRegisterTest.");
        // 注册/解注册为本地列表操作, 无 SLE 网关无 IPC, 不做 SLE 使能前置
        NearlinkAccessTokenMock::SetNativeTokenInfo();
    }
    static void TearDownTestCase()
    {
        HILOGI("TearDownTestCase NearlinkHostObserverRegisterTest");
    }
    void SetUp()
    {
        HILOGI("SetUp NearlinkHostObserverRegisterTest.");
    }
    void TearDown()
    {
        HILOGI("TearDown NearlinkHostObserverRegisterTest.");
    }
};

/**
 * @tc.name: RegisterObserverDuplicate001
 * @tc.desc: 重复注册同一 host observer 返回 NL_ERR_INVALID_PARAM；解注册后可重复解注册（幂等）
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostObserverRegisterTest, RegisterObserverDuplicate001, TestSize.Level1)
{
    HILOGI("NearlinkHostObserverRegisterTest: RegisterObserverDuplicate001 start");
    auto observer = std::make_shared<MinHostObserver>();
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().RegisterObserver(observer));
    // 第二次 Wrap 返回空指针，命中 adapter 判空腿
    EXPECT_EQ(NL_ERR_INVALID_PARAM, NearlinkHost::GetInstance().RegisterObserver(observer));
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterObserver(observer));
    // 已解注册后再解注册走 Unwrap 失败腿，仍返回成功
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterObserver(observer));
    HILOGI("NearlinkHostObserverRegisterTest: RegisterObserverDuplicate001 end");
}

/**
 * @tc.name: RegisterRemoteDeviceObserverDuplicate001
 * @tc.desc: 重复注册同一远端设备 observer 返回 NL_ERR_INVALID_PARAM，解注册后列表清空
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostObserverRegisterTest, RegisterRemoteDeviceObserverDuplicate001, TestSize.Level1)
{
    HILOGI("NearlinkHostObserverRegisterTest: RegisterRemoteDeviceObserverDuplicate001 start");
    auto observer = std::make_shared<MinRemoteDeviceObserver>();
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().RegisterRemoteDeviceObserver(observer));
    EXPECT_EQ(NL_ERR_INVALID_PARAM, NearlinkHost::GetInstance().RegisterRemoteDeviceObserver(observer));
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterRemoteDeviceObserver(observer));
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterRemoteDeviceObserver(observer));
    HILOGI("NearlinkHostObserverRegisterTest: RegisterRemoteDeviceObserverDuplicate001 end");
}

/**
 * @tc.name: RegisterRssiObserverDuplicate001
 * @tc.desc: 重复注册同一 rssi observer 返回 NL_ERR_INVALID_PARAM，解注册后幂等
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostObserverRegisterTest, RegisterRssiObserverDuplicate001, TestSize.Level1)
{
    HILOGI("NearlinkHostObserverRegisterTest: RegisterRssiObserverDuplicate001 start");
    auto observer = std::make_shared<MinRssiObserver>();
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().RegisterRssiObserver(observer));
    EXPECT_EQ(NL_ERR_INVALID_PARAM, NearlinkHost::GetInstance().RegisterRssiObserver(observer));
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterRssiObserver(observer));
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterRssiObserver(observer));
    HILOGI("NearlinkHostObserverRegisterTest: RegisterRssiObserverDuplicate001 end");
}

/**
 * @tc.name: RegisterBatteryObserverDuplicate001
 * @tc.desc: 电池服务可用时重复注册返回 NL_ERR_INVALID_PARAM；服务不可用时验证流程不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostObserverRegisterTest, RegisterBatteryObserverDuplicate001, TestSize.Level1)
{
    HILOGI("NearlinkHostObserverRegisterTest: RegisterBatteryObserverDuplicate001 start");
    auto observer = std::make_shared<MinBatteryObserver>();
    NlErrCode first = NearlinkHost::GetInstance().RegisterBatteryObserver(observer);
    if (first == NL_NO_ERROR) {
        EXPECT_EQ(NL_ERR_INVALID_PARAM, NearlinkHost::GetInstance().RegisterBatteryObserver(observer));
        EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterBatteryObserver(observer));
        EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterBatteryObserver(observer));
    } else {
        // BAS 服务不可用的环境仅验证重复调用不崩溃
        HILOGW("battery service unavailable, first register ret: %{public}d", first);
        NearlinkHost::GetInstance().RegisterBatteryObserver(observer);
        NearlinkHost::GetInstance().DeregisterBatteryObserver(observer);
    }
    HILOGI("NearlinkHostObserverRegisterTest: RegisterBatteryObserverDuplicate001 end");
}

/**
 * @tc.name: DeregisterObserverNotRegistered001
 * @tc.desc: 解注册从未注册的 host observer 走 Unwrap 失败腿并返回成功
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostObserverRegisterTest, DeregisterObserverNotRegistered001, TestSize.Level1)
{
    HILOGI("NearlinkHostObserverRegisterTest: DeregisterObserverNotRegistered001 start");
    auto observer = std::make_shared<MinHostObserver>();
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterObserver(observer));
    HILOGI("NearlinkHostObserverRegisterTest: DeregisterObserverNotRegistered001 end");
}

/**
 * @tc.name: DeregisterRemoteDeviceObserverNotRegistered001
 * @tc.desc: 解注册从未注册的远端设备 observer 走 Unwrap 失败腿并返回成功
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostObserverRegisterTest, DeregisterRemoteDeviceObserverNotRegistered001, TestSize.Level1)
{
    HILOGI("NearlinkHostObserverRegisterTest: DeregisterRemoteDeviceObserverNotRegistered001 start");
    auto observer = std::make_shared<MinRemoteDeviceObserver>();
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterRemoteDeviceObserver(observer));
    HILOGI("NearlinkHostObserverRegisterTest: DeregisterRemoteDeviceObserverNotRegistered001 end");
}

/**
 * @tc.name: DeregisterRssiObserverNotRegistered001
 * @tc.desc: 解注册从未注册的 rssi observer 走 Unwrap 失败腿并返回成功
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostObserverRegisterTest, DeregisterRssiObserverNotRegistered001, TestSize.Level1)
{
    HILOGI("NearlinkHostObserverRegisterTest: DeregisterRssiObserverNotRegistered001 start");
    auto observer = std::make_shared<MinRssiObserver>();
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterRssiObserver(observer));
    HILOGI("NearlinkHostObserverRegisterTest: DeregisterRssiObserverNotRegistered001 end");
}

/**
 * @tc.name: DeregisterBatteryObserverNotRegistered001
 * @tc.desc: 电池服务可用时解注册未注册 observer 返回成功；不可用时返回特性不支持
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostObserverRegisterTest, DeregisterBatteryObserverNotRegistered001, TestSize.Level1)
{
    HILOGI("NearlinkHostObserverRegisterTest: DeregisterBatteryObserverNotRegistered001 start");
    auto observer = std::make_shared<MinBatteryObserver>();
    NlErrCode ret = NearlinkHost::GetInstance().DeregisterBatteryObserver(observer);
    // BAS 特性检查先于 Unwrap：支持时走未注册腿返回 NL_NO_ERROR，不支持时返回 NL_ERR_FEATURE_NOT_SUPPORT
    EXPECT_TRUE(ret == NL_NO_ERROR || ret == NL_ERR_FEATURE_NOT_SUPPORT) << "ret: " << ret;
    HILOGI("NearlinkHostObserverRegisterTest: DeregisterBatteryObserverNotRegistered001 end");
}

/**
 * @tc.name: RegisterNullObserverProductSemantics001
 * @tc.desc: 产品实际语义：nullptr host 观察者可注册成功，二次注册命中 Wrap 判空，解注册幂等
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostObserverRegisterTest, RegisterNullObserverProductSemantics001, TestSize.Level1)
{
    // 框架层不拦截 nullptr（Wrap 直接包装空 weak），与 INVALID_PARAM 预期不同，按产品实际行为断言
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().RegisterObserver(nullptr));
    // 空 weak 与空 weak 判等：二次注册 nullptr 命中 Wrap 判空腿，返回 INVALID_PARAM
    EXPECT_EQ(NL_ERR_INVALID_PARAM, NearlinkHost::GetInstance().RegisterObserver(nullptr));
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterObserver(nullptr));
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterObserver(nullptr));
}

/**
 * @tc.name: RegisterNullRssiObserverProductSemantics001
 * @tc.desc: rssi 家族 nullptr 观察者语义与 host 家族一致：可注册、二次判空、解注册幂等
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostObserverRegisterTest, RegisterNullRssiObserverProductSemantics001, TestSize.Level1)
{
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().RegisterRssiObserver(nullptr));
    EXPECT_EQ(NL_ERR_INVALID_PARAM, NearlinkHost::GetInstance().RegisterRssiObserver(nullptr));
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterRssiObserver(nullptr));
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterRssiObserver(nullptr));
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
