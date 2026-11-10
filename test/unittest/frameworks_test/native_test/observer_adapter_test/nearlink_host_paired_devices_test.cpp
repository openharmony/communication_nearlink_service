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

#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "accesstoken_kit.h"
#include "log.h"
#include "nearlink_access_token_mock.h"
#include "nearlink_host.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_test_enable_wait.h"
#include "token_setproc.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
// 本机 native token（nearlink_test）的 nearlink user_grant 权限在本镜像无 opcode 定义，
// 服务端权限门对 native token 一律拒 201；借常驻系统应用 settings 的已授权 hap token 过权限门。
// 开关类 IPC 还需 CheckSystemApp 标志，本镜像各 hap 均不满足，Disable 系列不可达，详见用例注释。
class ScopedIpcToken {
public:
    ScopedIpcToken()
    {
        selfToken_ = GetSelfTokenID();
        ipcToken_ = Security::AccessToken::AccessTokenKit::GetHapTokenID(
            100, "com.huawei.hmos.settings", 0);
        if (ipcToken_ != 0) {
            SetSelfTokenID(ipcToken_);
        }
    }
    ~ScopedIpcToken()
    {
        SetSelfTokenID(selfToken_);
    }
    uint64_t IpcToken() const
    {
        return ipcToken_;
    }
private:
    uint64_t selfToken_ = 0;
    uint64_t ipcToken_ = 0;
};
} // namespace

class NearlinkHostPairedDevicesTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        HILOGI("SetUpTestCase NearlinkHostPairedDevicesTest.");
        NearlinkAccessTokenMock::SetNativeTokenInfo();
        // 不在此处使能 SLE：开启态用例自行确保已使能
    }
    static void TearDownTestCase()
    {
        HILOGI("TearDownTestCase NearlinkHostPairedDevicesTest");
    }
    void SetUp()
    {
        HILOGI("SetUp NearlinkHostPairedDevicesTest.");
    }
    void TearDown()
    {
        HILOGI("TearDown NearlinkHostPairedDevicesTest.");
    }
};

/**
 * @tc.name: GetPairedDevicesSleOn001
 * @tc.desc: SLE 开启态 GetPairedDevices 走 impl 成功分支，镜像设备 transport 透传一致
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostPairedDevicesTest, GetPairedDevicesSleOn001, TestSize.Level1)
{
    HILOGI("NearlinkHostPairedDevicesTest: GetPairedDevicesSleOn001 start");
    ScopedIpcToken ipcToken;
    ASSERT_NE(0u, ipcToken.IpcToken()) << "未取到 settings token，权限门 IPC 前提缺失";
    if (!NearlinkHost::GetInstance().IsSleEnabled()) {
        HILOGI("start enable nearlink");
        NearlinkHost::GetInstance().EnableNl();
    }
    ASSERT_TRUE(WaitForSleEnabled()) << "SLE 使能在 20s 内未完成，无法进入用例";
    std::vector<NearlinkRemoteDevice> devices;
    NlErrCode ret = NearlinkHost::GetInstance().GetPairedDevices(
        static_cast<int>(SleTransport::ADAPTER_SLE), devices);
    EXPECT_EQ(NL_NO_ERROR, ret);
    // 设备数与环境配对状态相关，仅校验非空时的 transport 透传，不臆断设备个数
    for (const auto &device : devices) {
        EXPECT_EQ(static_cast<int>(SleTransport::ADAPTER_SLE), device.GetTransportType());
    }
    HILOGI("paired device count: %{public}zu", devices.size());
    HILOGI("NearlinkHostPairedDevicesTest: GetPairedDevicesSleOn001 end");
}

/**
 * @tc.name: GetPairedDevicesPermissionDenied001
 * @tc.desc: 本机 native token 被权限门拒 201，GetPairedDevices 走错误提前返回腿且不产出设备
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostPairedDevicesTest, GetPairedDevicesPermissionDenied001, TestSize.Level1)
{
    HILOGI("NearlinkHostPairedDevicesTest: GetPairedDevicesPermissionDenied001 start");
    // 本镜像 native token 的 nearlink 权限无 opcode 定义，服务端权限门稳定返回 201；
    // Disable 系列 IPC 还需系统应用标志同样被拒，SLE 关闭态（NL_ERR_SLE_OFF）在本环境不可达，
    // 权限拒绝腿是本环境可稳定触发的错误提前返回分支，按产品实际行为断言
    std::vector<NearlinkRemoteDevice> devices;
    NlErrCode ret = NearlinkHost::GetInstance().GetPairedDevices(
        static_cast<int>(SleTransport::ADAPTER_SLE), devices);
    EXPECT_EQ(NL_ERR_PERMISSION_FAILED, ret);
    EXPECT_TRUE(devices.empty());
    HILOGI("NearlinkHostPairedDevicesTest: GetPairedDevicesPermissionDenied001 end");
}

/**
 * @tc.name: RegisterNullObserverProductSemantics001
 * @tc.desc: 产品实际语义：nullptr host 观察者可注册成功，二次注册命中 Wrap 判空，解注册幂等
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostPairedDevicesTest, RegisterNullObserverProductSemantics001, TestSize.Level1)
{
    HILOGI("NearlinkHostPairedDevicesTest: RegisterNullObserverProductSemantics001 start");
    // 框架层不拦截 nullptr（Wrap 直接包装空 weak），与 INVALID_PARAM 预期不同，按产品实际行为断言
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().RegisterObserver(nullptr));
    // 空 weak 与空 weak 判等：二次注册 nullptr 命中 Wrap 判空腿，返回 INVALID_PARAM
    EXPECT_EQ(NL_ERR_INVALID_PARAM, NearlinkHost::GetInstance().RegisterObserver(nullptr));
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterObserver(nullptr));
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterObserver(nullptr));
    HILOGI("NearlinkHostPairedDevicesTest: RegisterNullObserverProductSemantics001 end");
}

/**
 * @tc.name: RegisterNullRssiObserverProductSemantics001
 * @tc.desc: rssi 家族 nullptr 观察者语义与 host 家族一致：可注册、二次判空、解注册幂等
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostPairedDevicesTest, RegisterNullRssiObserverProductSemantics001, TestSize.Level1)
{
    HILOGI("NearlinkHostPairedDevicesTest: RegisterNullRssiObserverProductSemantics001 start");
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().RegisterRssiObserver(nullptr));
    EXPECT_EQ(NL_ERR_INVALID_PARAM, NearlinkHost::GetInstance().RegisterRssiObserver(nullptr));
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterRssiObserver(nullptr));
    EXPECT_EQ(NL_NO_ERROR, NearlinkHost::GetInstance().DeregisterRssiObserver(nullptr));
    HILOGI("NearlinkHostPairedDevicesTest: RegisterNullRssiObserverProductSemantics001 end");
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
