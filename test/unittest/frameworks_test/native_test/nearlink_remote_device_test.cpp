/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "nearlink_def.h"
#include "nearlink_host.h"
#include "nearlink_remote_device.h"
#include "nearlink_access_token_mock.h"
#include "nearlink_native_token_mock.h"
#include "parameters.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class NearlinkRemoteDeviceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkRemoteDeviceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkRemoteDeviceTest.");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    if (!NearlinkHost::GetInstance().IsSleEnabled()) {
        HILOGI("start enable nearlink");
        NearlinkHost::GetInstance().EnableNl();
        sleep(2);
    }
}

void NearlinkRemoteDeviceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkRemoteDeviceTest");
}

void NearlinkRemoteDeviceTest::SetUp()
{
    HILOGI("SetUp NearlinkRemoteDeviceTest.");
}

void NearlinkRemoteDeviceTest::TearDown()
{
    HILOGI("TearDown NearlinkRemoteDeviceTest.");
}

/**
 * @tc.name: SetDeviceAlias001
 * @tc.desc: 非法地址，返回NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, SetDeviceAlias001, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: SetDeviceAlias001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:00:00:00:00"; // "C8:C4:5A:00:01:00"
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    std::string name("abc");
    NlErrCode status = device->SetDeviceAlias(name);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: SetDeviceAlias001 end");
}

/**
 * @tc.name: SetPairingPassCode001
 * @tc.desc: 非法地址，返回NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, SetPairingPassCode001, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: SetPairingPassCode001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:00:00:00:00"; // "C8:C4:5A:00:01:00"
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    std::string passCode("123456");
    NlErrCode status = device->SetPairingPassCode(passCode);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: SetPairingPassCode001 end");
}

/**
 * @tc.name: SetPairingPassCode001
 * @tc.desc: 合法地址
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, SetPairingPassCode002, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: SetPairingPassCode002 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:11:22:33:44:55"; // "00:11:22:33:44:55"
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    std::string passCode("123456");
    NlErrCode status = device->SetPairingPassCode(passCode);
    EXPECT_NE(NL_NO_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: SetPairingPassCode002 end");
}

/**
 * @tc.name: SetPairingConfirmation001
 * @tc.desc: 非法地址，返回NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, SetPairingConfirmation001, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: SetPairingConfirmation001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:00:00:00:00"; // "C8:C4:5A:00:01:00"
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    bool flag = true;
    NlErrCode status = device->SetPairingConfirmation(flag);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: SetPairingConfirmation001 end");
}

/**
 * @tc.name: SetPairingConfirmation001
 * @tc.desc: 合法地址
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, SetPairingConfirmation002, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: SetPairingConfirmation002 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:11:22:33:44:55"; // "00:11:22:33:44:55"
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    bool flag = true;
    NlErrCode status = device->SetPairingConfirmation(flag);
    EXPECT_NE(NL_NO_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: SetPairingConfirmation002 end");
}

/**
 * @tc.name: StartPair001
 * @tc.desc: 非法地址，返回NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, StartPair001, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: StartPair001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:00:00:00:00"; // "C8:C4:5A:00:01:00"
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    NlErrCode status = device->StartPair();
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: StartPair001 end");
}

/**
 * @tc.name: StartPair002
 * @tc.desc: 合法地址
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, StartPair002, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: StartPair002 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:11:22:33:44:55"; // "00:11:22:33:44:55"
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    NlErrCode status = device->StartPair();
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: StartPair002 end");
}

/**
 * @tc.name: StartCrediblePair001
 * @tc.desc: 非法地址，返回NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, StartCrediblePair001, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: StartCrediblePair001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:00:00:00:00"; // "C8:C4:5A:00:01:00"
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    NlErrCode status = device->StartCrediblePair();
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: StartCrediblePair001 end");
}

/**
 * @tc.name: StartCrediblePair002
 * @tc.desc: 合法地址
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, StartCrediblePair002, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: StartCrediblePair002 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:11:22:33:44:55"; // "00:11:22:33:44:55"
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    NlErrCode status = device->StartCrediblePair();
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: StartCrediblePair002 end");
}

/**
 * @tc.name: CancelDevicePairing001
 * @tc.desc: 非法地址，返回NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, CancelDevicePairing001, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: CancelDevicePairing001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:00:00:00:00"; // "C8:C4:5A:00:01:00"
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    NlErrCode status = device->CancelDevicePairing();
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: CancelDevicePairing001 end");
}

/**
 * @tc.name: CancelDevicePairing002
 * @tc.desc: 合法地址
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, CancelDevicePairing002, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: CancelDevicePairing002 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:11:22:33:44:55"; // "00:11:22:33:44:55"
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    NlErrCode status = device->CancelDevicePairing();
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: CancelDevicePairing002 end");
}

/**
 * @tc.name: GetPairState001
 * @tc.desc: 非法地址，返回NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, GetPairState001, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: GetPairState001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:00:00:00:00";
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    int pairState = static_cast<int>(SlePairState::SLE_PAIR_NONE);
    NlErrCode status = device->GetPairState(pairState);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: GetPairState001 end");
}

/**
 * @tc.name: GetPairState001
 * @tc.desc: 非法地址，返回NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, GetPairState002, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: GetPairState001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:11:22:33:44:55"; // "00:11:22:33:44:55"
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    int pairState = static_cast<int>(SlePairState::SLE_PAIR_NONE);
    NlErrCode status = device->GetPairState(pairState);
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: GetPairState001 end");
}

/**
 * @tc.name: IsBondedFromLocal001
 * @tc.desc: 非法地址，返回NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, IsBondedFromLocal001, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: IsBondedFromLocal001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:00:00:00:00"; // 非法地址
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    bool isBondedFromLocal = false;
    NlErrCode status = device->IsBondedFromLocal(isBondedFromLocal);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: IsBondedFromLocal001 end");
}

/**
 * @tc.name: IsBondedFromLocal002
 * @tc.desc: 合法地址
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, IsBondedFromLocal002, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: IsBondedFromLocal002 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:11:22:33:44:55"; // 合法地址
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    bool isBondedFromLocal = false;
    NlErrCode status = device->IsBondedFromLocal(isBondedFromLocal);
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: IsBondedFromLocal002 end");
}

/**
 * @tc.name: IsAcbConnected001
 * @tc.desc: 非法地址，返回NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, IsAcbConnected001, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: IsAcbConnected001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:00:00:00:00"; // 非法地址
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    bool isAcbConnected = false;
    NlErrCode status = device->IsAcbConnected(isAcbConnected);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: IsAcbConnected001 end");
}

/**
 * @tc.name: IsAcbConnected002
 * @tc.desc: 合法地址
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, IsAcbConnected002, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: IsAcbConnected002 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:11:22:33:44:55"; // 合法地址
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    bool isAcbConnected = false;
    NlErrCode status = device->IsAcbConnected(isAcbConnected);
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: IsAcbConnected002 end");
}

/**
 * @tc.name: IsAcbEncrypted001
 * @tc.desc: 非法地址，返回NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, IsAcbEncrypted001, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: IsAcbEncrypted001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:00:00:00:00"; // 非法地址
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    bool isAcbEncrypted = false;
    NlErrCode status = device->IsAcbEncrypted(isAcbEncrypted);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: IsAcbEncrypted001 end");
}

/**
 * @tc.name: IsAcbEncrypted002
 * @tc.desc: 合法地址
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, IsAcbEncrypted002, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: IsAcbEncrypted002 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:11:22:33:44:55"; // 合法地址
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    bool isAcbEncrypted = false;
    NlErrCode status = device->IsAcbEncrypted(isAcbEncrypted);
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: IsAcbEncrypted002 end");
}

/**
 * @tc.name: PairRequestReply001
 * @tc.desc: 非法地址，返回NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, PairRequestReply001, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: PairRequestReply001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:00:00:00:00"; // 非法地址
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    bool accept = true;
    NlErrCode status = device->PairRequestReply(accept);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: PairRequestReply001 end");
}

/**
 * @tc.name: PairRequestReply002
 * @tc.desc: 合法地址
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, PairRequestReply002, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: PairRequestReply002 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:11:22:33:44:55"; // 合法地址
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    bool accept = true;
    NlErrCode status = device->PairRequestReply(accept);
    EXPECT_NE(NL_NO_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: PairRequestReply002 end");
}

/**
 * @tc.name: ReadRemoteRssiValue001
 * @tc.desc: 非法地址，返回NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, ReadRemoteRssiValue001, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: ReadRemoteRssiValue001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:00:00:00:00"; // 非法地址
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    NlErrCode status = device->ReadRemoteRssiValue();
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: ReadRemoteRssiValue001 end");
}

/**
 * @tc.name: ReadRemoteRssiValue002
 * @tc.desc: 合法地址
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, ReadRemoteRssiValue002, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: ReadRemoteRssiValue002 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:11:22:33:44:55"; // 合法地址
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    NlErrCode status = device->ReadRemoteRssiValue();
    EXPECT_NE(NL_NO_ERROR, status);//没有在peerConnDeviceSafeList_中设置acbstate
    HILOGI("NearlinkRemoteDeviceTest: ReadRemoteRssiValue002 end");
}

/**
 * @tc.name: GetDeviceInfo001
 * @tc.desc: 非法地址，返回NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, GetDeviceInfo001, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: GetDeviceInfo001 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:00:00:00:00"; // 非法地址
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    std::string name("");
    NlErrCode status = device->GetDeviceName(name);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    std::string alais("");
    status = device->GetDeviceAlias(alais);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    std::vector<std::string> uuids;
    status = device->GetDeviceUuids(uuids);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    int appearance = 0;
    status = device->GetDeviceAppearance(appearance);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    uint16_t productId = 0;
    status = device->GetDeviceProductId(productId);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    uint16_t vendorId = 0;
    status = device->GetDeviceVendorId(vendorId);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    DeviceModel model;
    status = device->GetDeviceModel(model);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    int acbState = static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED);
    status = device->GetAcbState(acbState);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: GetDeviceInfo001 end");
}

/**
 * @tc.name: GetDeviceInfo002
 * @tc.desc: 合法地址
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkRemoteDeviceTest, GetDeviceInfo002, TestSize.Level1)
{
    HILOGI("NearlinkRemoteDeviceTest: GetDeviceInfo002 start");
    std::shared_ptr<NearlinkRemoteDevice> device = nullptr;
    char addr[] = "00:11:22:33:44:55"; // 合法地址
    std::string deviceId(addr);
    device = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    std::string name("");
    NlErrCode status = device->GetDeviceName(name);
    EXPECT_EQ(NL_NO_ERROR, status);
    std::string alais("");
    status = device->GetDeviceAlias(alais);
    EXPECT_EQ(NL_NO_ERROR, status);
    std::vector<std::string> uuids;
    status = device->GetDeviceUuids(uuids);
    EXPECT_EQ(NL_NO_ERROR, status);
    int appearance = 0;
    status = device->GetDeviceAppearance(appearance);
    EXPECT_EQ(NL_NO_ERROR, status);
    uint16_t productId = 0;
    status = device->GetDeviceProductId(productId);
    EXPECT_EQ(NL_NO_ERROR, status);
    uint16_t vendorId = 0;
    status = device->GetDeviceVendorId(vendorId);
    EXPECT_EQ(NL_NO_ERROR, status);
    DeviceModel model;
    status = device->GetDeviceModel(model);
    EXPECT_EQ(NL_NO_ERROR, status);
    int acbState = static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED);
    status = device->GetAcbState(acbState);
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkRemoteDeviceTest: GetDeviceInfo002 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
