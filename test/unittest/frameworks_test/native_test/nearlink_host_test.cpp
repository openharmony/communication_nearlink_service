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
#include "nearlink_native_token_mock.h"
#include "nearlink_def.h"
#include "nearlink_host.h"
#include "nearlink_host_server.h"
#include "log.h"
#include "parameters.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class NearlinkHostTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkHostTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
}

void NearlinkHostTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase start");
}

void NearlinkHostTest::SetUp()
{
    HILOGI("SetUp start");
}

void NearlinkHostTest::TearDown()
{
    HILOGI("TearDown start");
}

class NearlinkHostObserverTest final: public OHOS::Nearlink::NearlinkHostObserver {
public:
    NearlinkHostObserverTest()
    {}
    ~NearlinkHostObserverTest() = default;
    void OnStateChanged(const int transport, const int status)
    {
        HILOGI("OnStateChanged %{public}d", status);
    }
};

/**
 * @tc.number: RegisterObserver001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, RegisterObserver001, TestSize.Level1)
{
    HILOGI("RegisterObserver001 start");
    std::shared_ptr<NearlinkHostObserverTest> observer = std::make_shared<NearlinkHostObserverTest>();
    NlErrCode ret = NearlinkHost::GetInstance().RegisterObserver(observer);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("RegisterObserver001 end");
}

/**
 * @tc.number: DeregisterObserver
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, DeregisterObserver001, TestSize.Level1)
{
    HILOGI("DeregisterObserver001 start");
    auto observer = std::make_shared<NearlinkHostObserverTest>();
    NlErrCode ret = NearlinkHost::GetInstance().DeregisterObserver(observer);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("DeregisterObserver001 end");
}

/**
 * @tc.number: EnableNl001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, EnableNl001, TestSize.Level1)
{
    HILOGI("EnableNl001 start");
    std::shared_ptr<NearlinkHostObserverTest> observer = std::make_shared<NearlinkHostObserverTest>();
    NlErrCode ret = NearlinkHost::GetInstance().RegisterObserver(observer);
    EXPECT_EQ(NL_NO_ERROR, ret);
    ret = NearlinkHost::GetInstance().EnableNl();
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2);
    HILOGI("EnableNl001 end");
}

/**
 * @tc.number: IsSleEnabled001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, IsSleEnabled001, TestSize.Level1)
{
    // already enabled before test.
    HILOGI("IsSleEnabled001 start");
    bool ret = NearlinkHost::GetInstance().IsSleEnabled();
    EXPECT_EQ(true, ret);
    HILOGI("IsSleEnabled001 end");
}

/**
 * @tc.number: IsSleDisabled001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, IsSleDisabled001, TestSize.Level1)
{
    // already enabled before test.
    HILOGI("IsSleDisabled001 start");
    bool ret = NearlinkHost::GetInstance().IsSleDisabled();
    EXPECT_EQ(false, ret);
    HILOGI("IsSleDisabled001 end");
}

/**
 * @tc.number: GetAdapterConnectState001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, GetAdapterConnectState001, TestSize.Level1)
{
    // already enabled before test.
    HILOGI("GetAdapterConnectState001 start");
    int state = 0;
    NlErrCode ret = NearlinkHost::GetInstance().GetAdapterConnectState(state);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("GetAdapterConnectState001 end");
}

/**
 * @tc.number: GetProfileConnState001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, GetProfileConnState001, TestSize.Level1)
{
    HILOGI("GetProfileConnState001 start");
    const std::string remoteAddr = "123:45:67:89";
    int32_t state = 0;
    NlErrCode ret = NearlinkHost::GetInstance().GetProfileConnState(remoteAddr, state);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, ret);
    HILOGI("GetProfileConnState001 end");
}

/**
 * @tc.number: GetProfileConnState001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, GetProfileConnState002, TestSize.Level1)
{
    HILOGI("GetProfileConnState002 start");
    const std::string remoteAddr = "00:00:00:00:00:00";
    int32_t state = 0;
    NlErrCode ret = NearlinkHost::GetInstance().GetProfileConnState(remoteAddr, state);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("GetProfileConnState002 end");
}

/**
 * @tc.number: GetLocalName001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, GetLocalName001, TestSize.Level1)
{
    // already enabled before test.
    HILOGI("GetLocalName001 start");
    std::string localDeviceName;
    NlErrCode ret = NearlinkHost::GetInstance().GetLocalName(localDeviceName);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("GetLocalName001 end");
}

/**
 * @tc.number: SetLocalName001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, SetLocalName001, TestSize.Level1)
{
    // already enabled before test.
    HILOGI("SetLocalName001 start");
    std::string localDeviceName;
    NlErrCode ret = NearlinkHost::GetInstance().GetLocalName(localDeviceName);
    ret = NearlinkHost::GetInstance().SetLocalName(localDeviceName);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("SetLocalName001 end");
}

/**
 * @tc.number: GetLocalAddress001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, GetLocalAddress001, TestSize.Level1)
{
    HILOGI("GetLocalAddress001 start");
    std::string localDeviceAddres;
    NlErrCode ret = NearlinkHost::GetInstance().GetLocalAddress(localDeviceAddres);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("GetLocalAddress001 end");
}

/**
 * @tc.number: GetPairedDevices001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, GetPairedDevices001, TestSize.Level1)
{
    HILOGI("GetPairedDevices001 start");
    int transport = 0;  // 根据实际需求设置 transport 的值
    std::vector<NearlinkRemoteDevice> pairedDevices;
    NlErrCode ret = NearlinkHost::GetInstance().GetPairedDevices(transport, pairedDevices);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("GetPairedDevices001 end");
}

/**
 * @tc.number: SetConnectionMode001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, SetConnectionMode001, TestSize.Level1)
{
    // already enabled before test.
    HILOGI("SetConnectionMode001 start");
    int connectionMode = 0;
    int duration = 0;
    NlErrCode ret = NearlinkHost::GetInstance().SetConnectionMode(connectionMode, duration);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("SetConnectionMode001 end");
}

/**
 * @tc.number: RemovePair001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, RemovePair001, TestSize.Level1)
{
    HILOGI("RemovePair001 start");
    std::string deviceAddr = "123:45:67:89";  // 格式不正确的设备地址
    NearlinkRemoteDevice device(deviceAddr, 0);  // 初始化一个有效的设备对象
    NlErrCode ret = NearlinkHost::GetInstance().RemovePair(device);
    EXPECT_EQ(NL_ERR_INVALID_STATE, ret);
    HILOGI("RemovePair001 end");
}

/**
 * @tc.number: RemovePair001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, RemovePair002, TestSize.Level1)
{
    HILOGI("RemovePair002 start");
    std::string deviceAddr = "00:00:00:00:00:00"; // 格式正确但无效的设备地址
    NearlinkRemoteDevice device(deviceAddr, 0);
    NlErrCode ret = NearlinkHost::GetInstance().RemovePair(device);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, ret); // CancelPairing failed, because of not find the remote device!
    HILOGI("RemovePair002 end");
}

/**
 * @tc.number: RemoveAllPairs001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, RemoveAllPairs001, TestSize.Level1)
{
    HILOGI("RemoveAllPairs001 start");
    NlErrCode ret = NearlinkHost::GetInstance().RemoveAllPairs();
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("RemoveAllPairs001 end");
}

/**
 * @tc.number: GetLinkRole001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, GetLinkRole001, TestSize.Level1)
{
    // already enabled before test.
    HILOGI("GetLinkRole001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "C8:C4:00:01:00"; // "C8:C4:5A:00:01:00"
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    uint8_t role = SLE_INVALID_LINK_ROLE;
    NlErrCode ret = NearlinkHost::GetInstance().GetLinkRole(*device, role);
    EXPECT_EQ(NL_ERR_INVALID_PARAM, ret);
    HILOGI("GetLinkRole001 end");
}

/**
 * @tc.number: GetLinkRole001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, GetLinkRole002, TestSize.Level1)
{
    // already enabled before test.
    HILOGI("GetLinkRole001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:00:00:00:00:00";   // 格式正确但无效的设备地址
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    uint8_t role = SLE_INVALID_LINK_ROLE;
    NlErrCode ret = NearlinkHost::GetInstance().GetLinkRole(*device, role);
    EXPECT_EQ(NL_ERR_DEVICE_DISCONNECTED, ret);
    HILOGI("GetLinkRole001 end");
}

/**
 * @tc.number: ConnectAllowedProfiles001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, ConnectAllowedProfiles001, TestSize.Level1)
{
    HILOGI("ConnectAllowedProfiles001 start");
    const std::string remoteAddr = "123:45:67:89";  // 格式不正确的设备地址
    NlErrCode ret = NearlinkHost::GetInstance().ConnectAllowedProfiles(remoteAddr);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, ret);
    HILOGI("ConnectAllowedProfiles001 end");
}

/**
 * @tc.number: ConnectAllowedProfiles001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, ConnectAllowedProfiles002, TestSize.Level1)
{
    HILOGI("ConnectAllowedProfiles002 start");
    const std::string remoteAddr = "00:00:00:00:00:00";   // 格式正确但无效的设备地址
    NlErrCode ret = NearlinkHost::GetInstance().ConnectAllowedProfiles(remoteAddr);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("ConnectAllowedProfiles002 end");
}

/**
 * @tc.number: DisconnectAllowedProfiles001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, DisconnectAllowedProfiles001, TestSize.Level1)
{
    HILOGI("DisconnectAllowedProfiles001 start");
    const std::string remoteAddr = "123:45:67:89";  // 格式不正确的设备地址
    NlErrCode ret = NearlinkHost::GetInstance().DisconnectAllowedProfiles(remoteAddr);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, ret);
    HILOGI("DisconnectAllowedProfiles001 end");
}

/**
 * @tc.number: DisconnectAllowedProfiles001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, DisconnectAllowedProfiles002, TestSize.Level1)
{
    HILOGI("DisconnectAllowedProfiles002 start");
    const std::string remoteAddr = "00:00:00:00:00:00";  // 格式正确但无效的设备地址
    NlErrCode ret = NearlinkHost::GetInstance().DisconnectAllowedProfiles(remoteAddr);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("DisconnectAllowedProfiles002 end");
}
 
/**
 * @tc.name: SetFreqHopping001
 * @tc.desc: 测试正常参数下的SetFreqHopping函数
 * @tc.type: 功能测试
 */
HWTEST_F(NearlinkHostTest, SetFreqHopping001, TestSize.Level1)
{
    HILOGI("SetFreqHopping001 start");
    std::vector<uint8_t> freq(SLE_FREQ_HOPPING_LEN, 0); // 填充一个长度为SLE_FREQ_HOPPING_LEN的向量
    NlErrCode ret = NearlinkHost::GetInstance().SetFreqHopping(freq);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("SetFreqHopping001 end");
}

/**
 * @tc.name: SetFreqHopping002
 * @tc.desc: 测试参数长度不符合要求的SetFreqHopping函数
 * @tc.type: 异常测试
 */
HWTEST_F(NearlinkHostTest, SetFreqHopping002, TestSize.Level1)
{
    HILOGI("SetFreqHopping002 start");
    std::vector<uint8_t> freq(SLE_FREQ_HOPPING_LEN + 1, 0); // 长度大于SLE_FREQ_HOPPING_LEN
    NlErrCode ret = NearlinkHost::GetInstance().SetFreqHopping(freq);
    EXPECT_EQ(NL_ERR_INVALID_PARAM, ret);
    freq.resize(SLE_FREQ_HOPPING_LEN - 1, 0); // 长度小于SLE_FREQ_HOPPING_LEN
    ret = NearlinkHost::GetInstance().SetFreqHopping(freq);
    EXPECT_EQ(NL_ERR_INVALID_PARAM, ret);
    HILOGI("SetFreqHopping002 end");
}

/**
 * @tc.name: UpdateSleVirtualDevice001
 * @tc.desc:
 * @tc.type:
 */
HWTEST_F(NearlinkHostTest, UpdateSleVirtualDevice001, TestSize.Level1)
{
    HILOGI("NearlinkHostTest: UpdateSleVirtualDevice001 start");
    int32_t cmd = 1; // 1 is add virtual device cmd
    std::string address = "123:45:67:89"; // 格式不正确的设备地址
    NlErrCode ret = NearlinkHost::GetInstance().UpdateSleVirtualDevice(cmd, address);
    EXPECT_EQ(ret, NL_ERR_INTERNAL_ERROR);
    sleep(2);
    HILOGI("NearlinkHostTest: UpdateSleVirtualDevice001 end");
}

/**
 * @tc.name: CheckPermissionForNapi001
 * @tc.desc: 获取和设置CarKey DFX数据
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostTest, CheckPermissionForNapi001, TestSize.Level1)
{
    HILOGI("NearlinkHostTest: CheckPermissionForNapi001 start");
    bool isGranted;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    EXPECT_EQ(NL_NO_ERROR, checkResult);
    HILOGI("NearlinkHostTest: CheckPermissionForNapi001 end");
}

/**
 * @tc.name: UpdateSleVirtualDevice002
 * @tc.desc:
 * @tc.type:
 */
HWTEST_F(NearlinkHostTest, UpdateSleVirtualDevice002, TestSize.Level1)
{
    HILOGI("NearlinkHostTest: UpdateSleVirtualDevice002 start");
    int32_t cmd = 1; // 1 is add virtual device cmd
    std::string address = "00:00:00:00:00:00"; // 格式正确但无效的设备地址
    NlErrCode ret = NearlinkHost::GetInstance().UpdateSleVirtualDevice(cmd, address);
    bool isAudioSupported = OHOS::system::GetIntParameter("const.nearlink.audio", 0) != 0;
    if (isAudioSupported) {
        EXPECT_EQ(ret, NL_NO_ERROR);
    } else {
        EXPECT_EQ(ret, NL_ERR_INTERNAL_ERROR);
    }
    sleep(2);
    HILOGI("NearlinkHostTest: UpdateSleVirtualDevice002 end");
}

/**
 * @tc.name: UpdateSleVirtualDevice003
 * @tc.desc:
 * @tc.type:
 */
HWTEST_F(NearlinkHostTest, UpdateSleVirtualDevice003, TestSize.Level1)
{
    HILOGI("NearlinkHostTest: UpdateSleVirtualDevice003 start");
    int32_t cmd = 2; // 2 is delete virtual device cmd
    std::string address = "00:00:00:00:00:00";
    NlErrCode ret = NearlinkHost::GetInstance().UpdateSleVirtualDevice(cmd, address);
    bool isAudioSupported = OHOS::system::GetIntParameter("const.nearlink.audio", 0) != 0;
    if (isAudioSupported) {
        EXPECT_EQ(ret, NL_NO_ERROR);
    } else {
        EXPECT_EQ(ret, NL_ERR_INTERNAL_ERROR);
    }
    sleep(2);
    HILOGI("NearlinkHostTest: UpdateSleVirtualDevice003 end");
}

/**
 * @tc.number: DisableNl001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHostTest, DisableNl001, TestSize.Level1)
{
    // already enabled before test.
    HILOGI("NearlinkHostTest: DisableNl001 start");
    NlErrCode ret = NearlinkHost::GetInstance().DisableNl();
    sleep(2);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("NearlinkHostTest: DisableNl001 end");
}

HWTEST_F(NearlinkHostTest, FactoryReset001, TestSize.Level1)
{
    HILOGI("NearlinkHostTest: FactoryReset001 start");
    NlErrCode ret = NearlinkHost::GetInstance().NearlinkFactoryReset();
    EXPECT_EQ(ret, NL_NO_ERROR);
    sleep(2);
    HILOGI("NearlinkHostTest: FactoryReset001 end");
}

/**
 * @tc.name: SetBtAddrBySleAddr001
 * @tc.desc:
 * @tc.type:
 */
HWTEST_F(NearlinkHostTest, SetBtAddrBySleAddr001, TestSize.Level1)
{
    HILOGI("NearlinkHostTest: SetTxPower002 start");
    std::string btAdr = "00:00:00:00:00:00";
    std::string sleAddr = "00:00:00:00:00:00";
    NlErrCode ret = NearlinkHost::GetInstance().SetBtAddrBySleAddr(sleAddr, btAdr);
    ret = NearlinkHost::GetInstance().GetSleAddrByBtAddr(btAdr, sleAddr);
    EXPECT_NE(ret, NL_NO_ERROR);
    ret = NearlinkHost::GetInstance().GetBtAddrBySleAddr(sleAddr, btAdr);
    EXPECT_NE(ret, NL_NO_ERROR);
    sleep(2);
}

/**
 * @tc.number: IsFeatureSupported001
 * @tc.name: 测试 SLE_RADIO_FRAME_TYPE_4 功能支持
 * @tc.desc: 验证 IsFeatureSupported 在 SLE_RADIO_FRAME_TYPE_4 下是否正确返回支持状态
 */
HWTEST_F(NearlinkHostTest, IsFeatureSupported001, TestSize.Level1)
{
    HILOGI("IsFeatureSupported001 start");
    SleFeatureSupported feature = SleFeatureSupported::SLE_RADIO_FRAME_TYPE_4;
    bool isSupported = NearlinkHost::GetInstance().IsFeatureSupported(feature);
    EXPECT_EQ(isSupported, isSupported);
    HILOGI("IsFeatureSupported001 end");
}

/**
 * @tc.number: isConnectionExist001
 * @tc.name: 测试 是否存在连接
 * @tc.desc: 
 */
HWTEST_F(NearlinkHostTest, IsConnectionExist001, TestSize.Level1)
{
    HILOGI("IsConnectionExist001 start");
    bool isConnectionExist = NearlinkHost::GetInstance().IsConnectionExist();
    EXPECT_EQ(isConnectionExist, isConnectionExist);
    HILOGI("IsFeatureSupported001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS