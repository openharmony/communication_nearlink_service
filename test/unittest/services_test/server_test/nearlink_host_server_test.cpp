/*
 * Copyright (C) 2026 Huawei Device Co., Ltd. All rights reserved.
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
#include "nearlink_host_server.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "nearlink_def.h"
#include "SleInterfaceAdapterSub.h"
#include "log.h"


namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
constexpr int HOST_FUZZ_DELAY_50_MS = 50;
constexpr int HOST_SERVER_TDD_DELAY_1000_MS = 1000;
}

class NearlinkHostServerStubTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkHostServerStubTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkHostServerStubTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    HILOGI("HostServerTest OnStart");
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::HOST_SERVER_TDD_DELAY_1000_MS));
}

void NearlinkHostServerStubTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkHostServerStubTest");
}

void NearlinkHostServerStubTest::SetUp()
{
    HILOGI("SetUp NearlinkHostServerStubTest.");
}

void NearlinkHostServerStubTest::TearDown()
{
    HILOGI("TearDown NearlinkHostServerStubTest.");
}

int32_t HostOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    NL_CHECK_RETURN_RET(hostServer, TRANSACTION_ERR, "hostServer is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("HostOnRemoteRequest cmd(%{public}d)", code);
    return hostServer->OnRemoteRequest(code, data, reply, option);
}

/**
 * @tc.name: DisableSle
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, DisableSle001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerTest:DisableSle001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_DISABLE_SLE, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::HOST_SERVER_TDD_DELAY_1000_MS));
    HILOGI("NearlinkHostServerTest:DisableSle001 end");
}

/**
 * @tc.name: EnableSle
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, EnableSle001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerTest:EnableSle001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    bool enbleAutoConnAudioDev = true; // 测试用例中设置为true
    data.WriteBool(enbleAutoConnAudioDev);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_ENABLE_SLE, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::HOST_SERVER_TDD_DELAY_1000_MS));
    HILOGI("NearlinkHostServerTest:EnableSle001 end");
}

/**
 * @tc.name: GetProfile
 * @tc.desc: Test the GetProfile function to ensure it correctly retrieves the profile.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, GetProfile001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerTest:GetProfile001 start");
    std::string profileName = "SleAdvertiserServer";
    sptr<IRemoteObject> remoteProfile = nullptr;
    ErrCode status = NearlinkHostServer::GetInstance()->GetProfile(profileName, remoteProfile);
    EXPECT_EQ(NL_NO_ERROR, status);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::HOST_FUZZ_DELAY_50_MS));
    HILOGI("NearlinkHostServerTest:GetProfile001 end");
}

/**
 * @tc.name: GetAcbState
 * @tc.desc: Test the GetAcbState function to ensure it correctly retrieves the ACB state.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, GetAcbState001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerTest:GetAcbState001 start");
    std::string address = "testAddress";
    int acbState = 0;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_ACB_STATE, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    if (status == NL_NO_ERROR) {
        acbState = reply.ReadInt32();
        EXPECT_GE(acbState, 0); // 假设ACB状态为非负整数
    }
    HILOGI("NearlinkHostServerTest:GetAcbState001 end");
}

/**
 * @tc.name: GetAdapterConnectState
 * @tc.desc: Test the GetAdapterConnectState function to ensure it correctly retrieves the adapter connection state.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, GetAdapterConnectState001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerTest:GetAdapterConnectState001 start");
    int32_t state = 0;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_ADAPTER_CONNECTION_STATE, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    if (status == NL_NO_ERROR) {
        state = reply.ReadInt32();
        EXPECT_GE(state, 0); // 假设连接状态为非负整数
    }

    HILOGI("NearlinkHostServerTest:GetAdapterConnectState001 end");
}

/**
 * @tc.name: GetProfileConnState
 * @tc.desc: Test the GetProfileConnState function to ensure it correctly retrieves the profile connection state.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, GetProfileConnState001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerTest:GetProfileConnState001 start");
    std::string address = "testAddress";
    int32_t state = 0;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_PROFILE_CONNECTION_STATE, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    if (status == NL_NO_ERROR) {
        state = reply.ReadInt32();
        EXPECT_GE(state, 0); // 假设连接状态为非负整数
    }
    HILOGI("NearlinkHostServerTest:GetProfileConnState001 end");
}

/**
 * @tc.name: Get&SetLocalName
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, GetSetLocalName001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:GetSetLocalName001 start");
    std::string localDeviceName;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_LOCAL_NAME, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        localDeviceName = reply.ReadString();
    }
    MessageParcel data1;
    MessageParcel reply1;
    data1.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data1.WriteString(localDeviceName);
    ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_LOCAL_NAME, data1, reply1);
    EXPECT_EQ(NO_ERROR, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::HOST_FUZZ_DELAY_50_MS));
    HILOGI("NearlinkHostServerStubTest:GetSetLocalName001 end");
}

/**
 * @tc.name: GetLocalAddress
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, GetLocalAddress001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:GetLocalAddress001 start");
    std::string localDeviceAddres;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_LOCAL_ADDR, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::HOST_FUZZ_DELAY_50_MS));
    HILOGI("NearlinkHostServerStubTest:GetLocalAddress001 end");
}

/**
 * @tc.name: GetPairedDevices
 * @tc.desc: Test the GetPairedDevices function to ensure it correctly retrieves the list of paired devices.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, GetPairedDevices001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerTest:GetPairedDevices001 start");
    std::vector<NearlinkRawAddress> pairedAddr;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_PAIRED_DEVICES, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkHostServerTest:GetPairedDevices001 end");
}

/**
 * @tc.name: SetConnectionMode
 * @tc.desc: Test the SetConnectionMode function to ensure it correctly sets the connection mode and duration.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, SetConnectionMode001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerTest:SetConnectionMode001 start");
    int32_t connectionMode = 1; // 假设连接模式为1
    int32_t duration = 10;      // 假设持续时间为10秒
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(connectionMode);
    data.WriteInt32(duration);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_CONNECTION_MODE, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkHostServerTest:SetConnectionMode001 end");
}

/**
 * @tc.name: RemovePair_allPairs
 * @tc.desc: Test the RemovePair and RemoveAllPairs function with valid parameters.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, RemovePair_allPairs001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerTest:RemovePair001 start");
    std::string deviceAddr = "00:00:00:00:00:00"; // 格式正确但无效的设备地址
    sptr<NearlinkRawAddress> device = new NearlinkRawAddress(deviceAddr);
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteParcelable(device.GetRefPtr());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_REMOVE_PAIR, data, reply);
    EXPECT_EQ(NL_NO_ERROR, ret);
    MessageParcel data1;
    MessageParcel reply1;
    data1.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_REMOVE_ALL_PAIRS, data1, reply1);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("NearlinkHostServerTest:RemovePair001 end");
}

/**
 * @tc.name: GetInfo
 * @tc.desc: 
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, GetInfo001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerTest:GetInfo001 start");
    std::string address = "00:11:22:33:44:55"; // 合法地址
    int32_t transport = SleTransport::ADAPTER_SLE;
    MessageParcel dataPairState;
    MessageParcel replyPairState;
    dataPairState.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    dataPairState.WriteInt32(transport);
    dataPairState.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_PAIR_STATE, dataPairState, replyPairState);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkHostServerTest:GetInfo001 end");
}

/**
 * @tc.name: GetDeviceInfo
 * @tc.desc: 
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, GetDeviceInfo001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerTest:GetDeviceInfo001 start");
    std::string address = "00:11:22:33:44:55"; // 合法地址
    int32_t transport = SleTransport::ADAPTER_SLE;
    MessageParcel dataName;
    MessageParcel replyName;
    dataName.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    dataName.WriteInt32(transport);
    dataName.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_NAME, dataName, replyName);
    EXPECT_EQ(NO_ERROR, ret);
    MessageParcel dataAlias;
    MessageParcel replyAlias;
    dataAlias.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    dataAlias.WriteInt32(transport);
    dataAlias.WriteString(address);
    ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_ALIAS, dataAlias, replyAlias);
    EXPECT_EQ(NO_ERROR, ret);
    MessageParcel dataUuids;
    MessageParcel replyUuids;
    dataUuids.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    dataUuids.WriteInt32(transport);
    dataUuids.WriteString(address);
    ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_UUIDS, dataUuids, replyUuids);
    EXPECT_EQ(NO_ERROR, ret);
    MessageParcel dataAppearance;
    MessageParcel replyAppearance;
    dataAppearance.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    dataAppearance.WriteString(address);
    ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_APPEARANCE, dataAppearance, replyAppearance);
    EXPECT_EQ(NO_ERROR, ret);
    MessageParcel dataProductId;
    MessageParcel replyProductId;
    dataProductId.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    dataProductId.WriteString(address);
    ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_PRODUCT_ID, dataProductId, replyProductId);
    EXPECT_EQ(NO_ERROR, ret);
    MessageParcel dataVendorId;
    MessageParcel replyVendorId;
    dataVendorId.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    dataVendorId.WriteString(address);
    ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_VENDOR_ID, dataVendorId, replyVendorId);
    EXPECT_EQ(NO_ERROR, ret);
    MessageParcel dataDeviceModel;
    MessageParcel replyDeviceModel;
    dataDeviceModel.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    dataDeviceModel.WriteString(address);
    ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_MODEL, dataDeviceModel, replyDeviceModel);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkHostServerTest:GetDeviceInfo001 end");
}

/**
 * @tc.name: IsFeatureSupported
 * @tc.desc: 测试 IsFeatureSupported 接口是否能正确返回功能是否被支持
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, IsFeatureSupported001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:IsFeatureSupported001 start");
    int32_t feature = 0;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(feature);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_IS_SLE_FEATURE_SUPPORTED, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkHostServerStubTest:IsFeatureSupported001 end");
}

/**
 * @tc.name: IsConnectionExist
 * @tc.desc: 测试 IsConnectionExist 接口是否能正确返回连接是否存在
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, IsConnectionExist001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:IsFeatureSupported001 start");
    int32_t feature = 0;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(feature);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_IS_SLE_CONNECTION_EXIST, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkHostServerStubTest:IsFeatureSupported001 end");
}

/**
 * @tc.name: GetSleMaxAdvertisingDataLength001
 * @tc.desc: 测试 GetSleMaxAdvertisingDataLength 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, GetSleMaxAdvertisingDataLength001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:GetSleMaxAdvertisingDataLength001 start");
    uint32_t maxAdvDataLen = 0;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_SLE_MAX_ADVERTISING_DATALENGTH, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    if (status == NL_NO_ERROR) {
        maxAdvDataLen = static_cast<uint32_t>(reply.ReadInt32());
        EXPECT_EQ(maxAdvDataLen, 0xFF);
    }
    HILOGI("NearlinkHostServerStubTest:GetSleMaxAdvertisingDataLength001 end");
}

/**
 * @tc.name: SetDeviceAlias001
 * @tc.desc: 测试 SetDeviceAlias 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, SetDeviceAlias001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:SetDeviceAlias001 start");
    int32_t transport = static_cast<int32_t>(NlTransportType::NL_TRANSPORT_SLE);
    std::string address = "00:11:22:33:44:55"; // 合法地址
    std::string alias = "TestDevice"; // 合法别名
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(transport);
    data.WriteString(address);
    data.WriteString(alias);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_DEVICE_ALIAS, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkHostServerStubTest:SetDeviceAlias001 end");
}

/**
 * @tc.name: StartPair001
 * @tc.desc: 测试 StartPair 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, StartPair001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:StartPair001 start");
    int32_t transport = static_cast<int32_t>(NlTransportType::NL_TRANSPORT_SLE);
    std::string address = "00:11:22:33:44:55"; // 合法地址
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(transport);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_START_PAIR, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:StartPair001 end");
}

/**
 * @tc.name: StartCrediblePair001
 * @tc.desc: 测试 StartCrediblePair 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, StartCrediblePair001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:StartCrediblePair001 start");
    int32_t transport = static_cast<int32_t>(NlTransportType::NL_TRANSPORT_SLE);
    std::string address = "00:11:22:33:44:55"; // 合法地址
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(transport);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_START_CREDIBLE_PAIR, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:StartCrediblePair001 end");
}

/**
 * @tc.name: CancelPairing001
 * @tc.desc: 测试 CancelPairing 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, CancelPairing001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:CancelPairing001 start");
    int32_t transport = static_cast<int32_t>(NlTransportType::NL_TRANSPORT_SLE);
    std::string address = "00:11:22:33:44:55"; // 合法地址
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(transport);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_CANCEL_PAIRING, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:CancelPairing001 end");
}

/**
 * @tc.name: SetPairingPassCode
 * @tc.desc: 测试 SetPairingPassCode 函数的正常执行情况及边界值处理
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, SetPairingPassCode001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:SetPairingPassCode001 start");
    int32_t transport = 1;           
    std::string address = "00:11:22:33:44:55";  
    std::string passCode = "1234";    
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(transport);
    data.WriteString(address);
    data.WriteString(passCode);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_PASSCODE, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:SetPairingPassCode001 end");
}

/**
 * @tc.name: IsBondedFromLocal001
 * @tc.desc: 测试 IsBondedFromLocal 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, IsBondedFromLocal001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:IsBondedFromLocal001 start");
    int32_t transport = static_cast<int32_t>(NlTransportType::NL_TRANSPORT_SLE);
    std::string address = "00:11:22:33:44:55";
    bool isBondedFromLocal = false;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(transport);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_IS_BONDED_FROM_LOCAL, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:IsBondedFromLocal001 end");
}

/**
 * @tc.name: IsAcbConnected
 * @tc.desc: 测试 IsAcbConnected 函数的正常执行情况及边界值处理
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, IsAcbConnected001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:IsAcbConnected001 start");
    int32_t transport = 1;
    std::string address = "00:11:22:33:44:55";
    bool isAcbConnected = false;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(transport);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_IS_ACB_CONNECTED, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:IsAcbConnected001 end");
}

/**
 * @tc.name: SetPairingConfirmation001
 * @tc.desc: 测试 SetPairingConfirmation 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, SetPairingConfirmation001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:SetPairingConfirmation001 start");
    int32_t transport = 1;
    std::string address = "00:11:22:33:44:55";
    bool cfm = true;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(transport);
    data.WriteString(address);
    data.WriteBool(cfm);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_CFM, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:SetPairingConfirmation001 end");
}

/**
 * @tc.name: SetPairingConfirmation002
 * @tc.desc: 测试 SetPairingConfirmation 函数在 transport 不等于 NL_TRANSPORT_SLE 时的执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, SetPairingConfirmation002, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:SetPairingConfirmation002 start");
    int32_t transport = 0;
    std::string address = "00:11:22:33:44:55";
    bool cfm = true;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(transport);
    data.WriteString(address);
    data.WriteBool(cfm);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_CFM, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_ERR_INVALID_STATE, status);
    HILOGI("NearlinkHostServerStubTest:SetPairingConfirmation002 end");
}

/**
 * @tc.name: SetPairingConfirmation003
 * @tc.desc: 测试 SetPairingConfirmation 函数在 cfm 为 false 时的执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, SetPairingConfirmation003, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:SetPairingConfirmation003 start");
    int32_t transport = 1;
    std::string address = "00:11:22:33:44:55";
    bool cfm = false;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(transport);
    data.WriteString(address);
    data.WriteBool(cfm);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_CFM, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:SetPairingConfirmation003 end");
}

/**
 * @tc.name: IsAcbEncrypted
 * @tc.desc: 测试 IsAcbEncrypted 函数的正常执行情况及边界值处理
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, IsAcbEncrypted001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:IsAcbEncrypted001 start");
    int32_t transport = 1;
    std::string address = "00:11:22:33:44:55";
    bool isAcbEncrypted = false;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(transport);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_IS_ACB_ENCRYPTED, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:IsAcbEncrypted001 end");
}

/**
 * @tc.name: GetLinkRole
 * @tc.desc: 测试 GetLinkRole 函数的正常执行情况及边界值处理
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, GetLinkRole001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:GetLinkRole001 start");
    int32_t transport = 1;
    std::string address = "00:11:22:33:44:55";
    uint8_t role = SLE_INVALID_LINK_ROLE;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(transport);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_LINK_ROLE, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_ERR_DEVICE_DISCONNECTED, status);
    HILOGI("NearlinkHostServerStubTest:GetLinkRole001 end");
}

/**
 * @tc.name: PairRequestReply
 * @tc.desc: 测试 PairRequestReply 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, PairRequestReply001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:PairRequestReply001 start");
    int32_t transport = 1;      
    std::string address = "00:11:22:33:44:55";
    bool accept = true;
    MessageParcel data;              
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(transport);
    data.WriteString(address);
    data.WriteBool(accept);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_PAIR_REQUEST_PEPLY, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:PairRequestReply001 end");
}

/**
 * @tc.name: ReadRemoteRssiValue
 * @tc.desc: 测试 ReadRemoteRssiValue 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, ReadRemoteRssiValue001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:ReadRemoteRssiValue001 start");
    std::string address = "00:11:22:33:44:55";
    int rssiValue = 0;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_READ_REMOTE_RSSI_VALUE, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:ReadRemoteRssiValue001 end");
}

/**
 * @tc.name: ConnectAllowedProfiles
 * @tc.desc: 测试 ConnectAllowedProfiles 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, ConnectAllowedProfiles001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:ConnectAllowedProfiles001 start");
    std::string remoteAddr = "00:11:22:33:44:55";  // 示例参数，根据实际需求调整
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteString(remoteAddr);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_CONNECT_ALLOWED_PROFILES, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:ConnectAllowedProfiles001 end");
}

/**
 * @tc.name: DisconnectAllowedProfiles
 * @tc.desc: 测试 DisconnectAllowedProfiles 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, DisconnectAllowedProfiles001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:DisconnectAllowedProfiles001 start");
    std::string remoteAddr = "00:11:22:33:44:55";
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteString(remoteAddr);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_DISCONNECT_ALLOWED_PROFILES, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:DisconnectAllowedProfiles001 end");
}

/**
 * @tc.name: SetFreqHopping
 * @tc.desc: 测试 SetFreqHopping 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, SetFreqHopping001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:SetFreqHopping001 start");
    std::vector<uint8_t> freq(SLE_FREQ_HOPPING_LEN, 0); // 填充一个长度为SLE_FREQ_HOPPING_LEN的向量
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteUInt8Vector(freq);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_FREQ_HOPPING, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:SetFreqHopping001 end");
}

/**
 * @tc.name: UpdateSleVirtualDevice
 * @tc.desc: 测试 UpdateSleVirtualDevice 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, UpdateSleVirtualDevice001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:UpdateSleVirtualDevice001 start");
    int32_t cmd = 0;
    std::string address = "00:11:22:33:44:55";
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(cmd);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_UPDATE_SLE_VIRTUAL_DEVICE, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(false, status);
    HILOGI("NearlinkHostServerStubTest:UpdateSleVirtualDevice001 end");
}
 
/**
 * @tc.name: GetSleAddrByBtAddr
 * @tc.desc: GetSleAddrByBtAddr 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, GetSleAddrByBtAddr001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:GetSleAddrByBtAddr001 start");
    std::string btAddr = "00:11:22:33:44:55";
    std::string sleAddr = "";
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteString(btAddr);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_SLE_ADDR, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:GetSleAddrByBtAddr001 end");
}
 
/**
 * @tc.name: GetSleAddrByBtAddr
 * @tc.desc: GetSleAddrByBtAddr 星闪地址不为空情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, GetSleAddrByBtAddr002, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:GetSleAddrByBtAddr002 start");
    std::string btAddr = "00:11:22:33:44:55";
    std::string sleAddr = "55:44:33:22:11:00";
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteString(btAddr);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_SLE_ADDR, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:GetSleAddrByBtAddr002 end");
}
 
/**
 * @tc.name: GetBtAddrBySleAddr
 * @tc.desc: 测试 GetBtAddrBySleAddr 函数的正常执行情况
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHostServerStubTest, GetBtAddrBySleAddr001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:GetBtAddrBySleAddr001 start");
    std::string sleAddr = "00:11:22:33:44:55";
    std::string btAddr = "";
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteString(sleAddr);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_BT_ADDR, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:GetBtAddrBySleAddr001 end");
}
 
/**
 * @tc.name: SetBtAddrBySleAddr
 * @tc.desc: 测试 SetBtAddrBySleAddr 函数的正常执行情况
 * @tc.type:
 */
HWTEST_F(NearlinkHostServerStubTest, SetBtAddrBySleAddr001, TestSize.Level1)
{
    HILOGI("NearlinkHostServerStubTest:SetBtAddrBySleAddr001 start");
    std::string sleAddr = "123456";
    std::string btAddr = "00:11:22:33:44:55";
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteString(sleAddr);
    data.WriteString(btAddr);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_BT_ADDR, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode status = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, status);
    HILOGI("NearlinkHostServerStubTest:SetBtAddrBySleAddr001 end");
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
