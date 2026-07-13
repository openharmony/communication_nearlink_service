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
#include <gmock/gmock.h>

#include <thread>
#include "nearlink_ssap_client_stub.h"
#include "nearlink_ssap_client_callback_stub.h"
#include "nearlink_ssap_client_callback_proxy.h"
#include "nearlink_ssap_client_server.cpp"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "log.h"
 
 
namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
 
namespace {
sptr<NearlinkSsapClientServer> g_ssapServer = new (std::nothrow) NearlinkSsapClientServer();
}

class NearlinkSsapClientCallbackStubTest : public NearlinkSsapClientCallbackStub {
public:
    void OnConnectionStateChanged(int32_t state, int32_t newState) override
    {}
    void OnPropertyChanged(const NearlinkSsapPropertyParcel &property) override
    {}
    void OnEventNotified(const NearlinkSsapEventParcel &event) override
    {}
    void OnReadProperty(int32_t ret, const NearlinkSsapPropertyParcel &property) override
    {}
    void OnCallMethod(int32_t ret, const NearlinkSsapMethodParcel &method) override
    {}
    void OnReadPropertiesByUuid(int32_t ret, const std::vector<NearlinkSsapPropertyParcel> &properties) override
    {}
    void OnWriteProperty(int32_t ret, const NearlinkSsapPropertyParcel &property) override
    {}
    void OnSetPropertyNotification(int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property) override
    {}
    void OnSetPropertyIndication(int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property) override
    {}
    void OnReadDescriptor(int32_t ret, const NearlinkSsapDescriptorParcel &descriptor) override
    {}
    void OnWriteDescriptor(int32_t ret, const NearlinkSsapDescriptorParcel &descriptor) override
    {}
    void OnMtuChanged(int32_t state, uint16_t mtu) override
    {}
    void OnServicesDiscovered(int32_t status) override
    {}
    void OnServicesDiscoveredByUuid(int32_t status, const Uuid &uuid) override
    {}
    void OnConnectionParameterChanged(int32_t interval, int32_t latency, int32_t timeout, int32_t status) override
    {}

    explicit NearlinkSsapClientCallbackStubTest()
    {}
    ~NearlinkSsapClientCallbackStubTest() override
    {}
};

class NearlinkSsapClientCallbackMock : public INearlinkSsapClientCallback {
public:
    MOCK_METHOD(void, OnConnectionStateChanged, (int32_t state, int32_t newState), (override));
    MOCK_METHOD(void, OnPropertyChanged, (const NearlinkSsapPropertyParcel &property), (override));
    MOCK_METHOD(void, OnEventNotified, (const NearlinkSsapEventParcel &event), (override));
    MOCK_METHOD(void, OnReadProperty, (int32_t ret, const NearlinkSsapPropertyParcel &property), (override));
    MOCK_METHOD(void, OnCallMethod, (int32_t ret, const NearlinkSsapMethodParcel &method), (override));
    MOCK_METHOD(void, OnReadPropertiesByUuid,
        (int32_t ret, const std::vector<NearlinkSsapPropertyParcel> &properties), (override));
    MOCK_METHOD(void, OnWriteProperty, (int32_t ret, const NearlinkSsapPropertyParcel &property), (override));
    MOCK_METHOD(void, OnSetPropertyNotification,
        (int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property), (override));
    MOCK_METHOD(void, OnSetPropertyIndication,
        (int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property), (override));
    MOCK_METHOD(void, OnReadDescriptor, (int32_t ret, const NearlinkSsapDescriptorParcel &descriptor), (override));
    MOCK_METHOD(void, OnWriteDescriptor,
        (int32_t ret, const NearlinkSsapDescriptorParcel &descriptor), (override));
    MOCK_METHOD(void, OnMtuChanged, (int32_t state, uint16_t mtu), (override));
    MOCK_METHOD(void, OnServicesDiscovered, (int32_t status), (override));
    MOCK_METHOD(void, OnServicesDiscoveredByUuid, (int32_t status, const Uuid &uuid), (override));
    MOCK_METHOD(void, OnConnectionParameterChanged,
        (int32_t interval, int32_t latency, int32_t timeout, int32_t status), (override));

    sptr<OHOS::IRemoteObject> AsObject() override
    {
        sptr<NearlinkSsapClientCallbackStubTest> nearlinkSsapClientCallbackStubTest =
        new NearlinkSsapClientCallbackStubTest();
        return nearlinkSsapClientCallbackStubTest;
    }
    ~NearlinkSsapClientCallbackMock() override = default;
};

class NearlinkSsapClientServerTest : public testing::Test {
public:
    NearlinkSsapClientServerTest()
    {}
    ~NearlinkSsapClientServerTest()
    {}

    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

    int appId_ = -1;
    sptr<NearlinkSsapClientCallbackMock> callback_ {nullptr};
    sptr<NearlinkSsapClientCallbackStubTest> nearlinkSsapClientCallbackStubTest_ =
        new NearlinkSsapClientCallbackStubTest();
    std::unique_ptr<NearlinkSsapClientServer> nearlinkSsapClientServer_ {nullptr};
    std::unique_ptr<NearlinkSsapClientServer::impl::SsapClientCallbackImpl> ssapServerCallbackImpl_ {nullptr};
};

void NearlinkSsapClientServerTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkSsapClientServerTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    HILOGI("SetUpTestCase end");
}

void NearlinkSsapClientServerTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkSsapClientServerTest");
}

void NearlinkSsapClientServerTest::SetUp()
{
    HILOGI("SetUp NearlinkSsapClientServerTest.");
    nearlinkSsapClientServer_ = std::make_unique<NearlinkSsapClientServer>();
    callback_ = sptr<NearlinkSsapClientCallbackMock>(new NearlinkSsapClientCallbackMock());
    RawAddress remoteAddr("11:22:33:44:55:66");
    ssapServerCallbackImpl_ = std::make_unique<NearlinkSsapClientServer::impl::
        SsapClientCallbackImpl>(callback_, *nearlinkSsapClientServer_);
    nearlinkSsapClientServer_->RegisterApplication(callback_,
        NearlinkRawAddress(remoteAddr), SleTransport::ADAPTER_SLE, appId_);
}

void NearlinkSsapClientServerTest::TearDown()
{
    HILOGI("TearDown NearlinkSsapClientServerTest.");
    nearlinkSsapClientServer_->DeregisterApplication(appId_);
    nearlinkSsapClientServer_ = nullptr;
    callback_ = nullptr;
    ssapServerCallbackImpl_ = nullptr;
}

/**
 * @tc.name: ConnectTest001
 * @tc.desc: Test ConnectInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, ConnectTest001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:ConnectTest001 start");
    bool isAutoConnect = true;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteBool(isAutoConnect);
    NearlinkSsapClientStub::ConnectInner(
        nearlinkSsapClientServer_.get(), data, reply);
    HILOGI("NearlinkSsapClientServerTest:ConnectTest001 end");
}

/**
 * @tc.name: ConnectTest002
 * @tc.desc: Test Connect.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, ConnectTest002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:ConnectTest002 start");
    bool isAutoConnect = true;
    NlErrCode result = nearlinkSsapClientServer_->Connect(appId_, isAutoConnect);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapClientServerTest:ConnectTest002 end");
}

/**
 * @tc.name: Disconnect001
 * @tc.desc: Test DisconnectInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, Disconnect001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:Disconnect001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(appId_);
    NearlinkSsapClientStub::DisconnectInner(
        nearlinkSsapClientServer_.get(), data, reply);
    HILOGI("NearlinkSsapClientServerTest:Disconnect001 end");
}

/**
 * @tc.name: Disconnect002
 * @tc.desc: Test Disconnect.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, Disconnect002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:Disconnect002 start");
    NlErrCode result = nearlinkSsapClientServer_->Disconnect(appId_);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapClientServerTest:Disconnect002 end");
}

/**
 * @tc.name: DiscoveryServices001
 * @tc.desc: Test DiscoveryServicesInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, DiscoveryServices001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:DiscoveryServices001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(appId_);
    NearlinkSsapClientStub::DiscoveryServicesInner(
        nearlinkSsapClientServer_.get(), data, reply);
    HILOGI("NearlinkSsapClientServerTest:DiscoveryServices001 end");
}

/**
 * @tc.name: DiscoveryServices002
 * @tc.desc: Test DiscoveryServices.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, DiscoveryServices002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:DiscoveryServices002 start");
    NlErrCode result = nearlinkSsapClientServer_->DiscoveryServices(appId_);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapClientServerTest:DiscoveryServices002 end");
}

/**
 * @tc.name: DiscoverServiceByUuid001
 * @tc.desc: Test DiscoverServiceByUuidInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, DiscoverServiceByUuid001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:DiscoverServiceByUuid001 start");
    NearlinkUuidParcel uuid(Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000000609")); // SLE_UUID_DIS
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&uuid);
    NearlinkSsapClientStub::DiscoverServiceByUuidInner(
        nearlinkSsapClientServer_.get(), data, reply);
    HILOGI("NearlinkSsapClientServerTest:DiscoverServiceByUuid001 end");
}

/**
 * @tc.name: DiscoverServiceByUuid002
 * @tc.desc: Test DiscoverServiceByUuid.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, DiscoverServiceByUuid002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:DiscoverServiceByUuid002 start");
    Uuid uuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000000609"); // SLE_UUID_DIS
    NlErrCode result = nearlinkSsapClientServer_->DiscoverServiceByUuid(appId_, uuid);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapClientServerTest:DiscoverServiceByUuid002 end");
}

/**
 * @tc.name: ReadProperty001
 * @tc.desc: Test ReadPropertyInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, ReadProperty001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:ReadProperty001 start");
    NearlinkSsapPropertyParcel property {};
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&property);
    NearlinkSsapClientStub::ReadPropertyInner(
        nearlinkSsapClientServer_.get(), data, reply);
    HILOGI("NearlinkSsapClientServerTest:ReadProperty001 end");
}

/**
 * @tc.name: ReadProperty002
 * @tc.desc: Test ReadProperty.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, ReadProperty002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:ReadProperty002 start");
    NearlinkSsapPropertyParcel property {};
    NlErrCode result = nearlinkSsapClientServer_->ReadProperty(appId_, property);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapClientServerTest:ReadProperty002 end");
}

/**
 * @tc.name: CallMethod001
 * @tc.desc: Test CallMethodInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, CallMethod001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:CallMethod001 start");
    NearlinkSsapMethodParcel method {};
    bool withoutRespond = true;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&method);
    data.WriteBool(withoutRespond);
    NearlinkSsapClientStub::CallMethodInner(
        nearlinkSsapClientServer_.get(), data, reply);
    HILOGI("NearlinkSsapClientServerTest:CallMethod001 end");
}

/**
 * @tc.name: CallMethod002
 * @tc.desc: Test CallMethod.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, CallMethod002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:CallMethod002 start");
    NearlinkSsapMethodParcel method {};
    bool withoutRespond = true;
    NlErrCode result = nearlinkSsapClientServer_->CallMethod(appId_, &method, withoutRespond);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapClientServerTest:CallMethod002 end");
}

/**
 * @tc.name: WriteProperty001
 * @tc.desc: Test WritePropertyInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, WriteProperty001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:WriteProperty001 start");
    NearlinkSsapPropertyParcel property {};
    bool withoutRespond = true;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&property);
    data.WriteBool(withoutRespond);
    NearlinkSsapClientStub::WritePropertyInner(
        nearlinkSsapClientServer_.get(), data, reply);
    HILOGI("NearlinkSsapClientServerTest:WriteProperty001 end");
}

/**
 * @tc.name: WriteProperty002
 * @tc.desc: Test WriteProperty.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, WriteProperty002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:WriteProperty002 start");
    NearlinkSsapPropertyParcel property {};
    bool withoutRespond = true;
    NlErrCode result = nearlinkSsapClientServer_->WriteProperty(appId_, &property, withoutRespond);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapClientServerTest:WriteProperty002 end");
}

/**
 * @tc.name: ReadDescriptor001
 * @tc.desc: Test ReadDescriptorInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, ReadDescriptor001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:ReadDescriptor001 start");
    NearlinkSsapDescriptorParcel descriptor {};
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&descriptor);
    NearlinkSsapClientStub::ReadDescriptorInner(
        nearlinkSsapClientServer_.get(), data, reply);
    HILOGI("NearlinkSsapClientServerTest:ReadDescriptor001 end");
}

/**
 * @tc.name: ReadDescriptor002
 * @tc.desc: Test ReadDescriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, ReadDescriptor002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:ReadDescriptor002 start");
    NearlinkSsapDescriptorParcel descriptor {};
    NlErrCode result = nearlinkSsapClientServer_->ReadDescriptor(appId_, descriptor);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapClientServerTest:ReadDescriptor002 end");
}

/**
 * @tc.name: WriteDescriptor001
 * @tc.desc: Test WriteDescriptorInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, WriteDescriptor001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:WriteDescriptor001 start");
    NearlinkSsapDescriptorParcel descriptor {};
    bool withoutRespond = true;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&descriptor);
    data.WriteBool(withoutRespond);
    NearlinkSsapClientStub::WriteDescriptorInner(
        nearlinkSsapClientServer_.get(), data, reply);
    HILOGI("NearlinkSsapClientServerTest:WriteDescriptor001 end");
}

/**
 * @tc.name: WriteDescriptor002
 * @tc.desc: Test WriteDescriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, WriteDescriptor002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:WriteDescriptor002 start");
    NearlinkSsapDescriptorParcel descriptor {};
    bool withoutRespond = true;
    NlErrCode result = nearlinkSsapClientServer_->WriteDescriptor(appId_, &descriptor, withoutRespond);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapClientServerTest:WriteDescriptor002 end");
}

/**
 * @tc.name: RequestExchangeMtu001
 * @tc.desc: Test RequestExchangeMtuInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, RequestExchangeMtu001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:RequestExchangeMtu001 start");
    int32_t mtu = 512; // mtu 512 bytes
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteInt32(mtu);
    NearlinkSsapClientStub::RequestExchangeMtuInner(
        nearlinkSsapClientServer_.get(), data, reply);
    HILOGI("NearlinkSsapClientServerTest:RequestExchangeMtu001 end");
}

/**
 * @tc.name: RequestExchangeMtu002
 * @tc.desc: Test RequestExchangeMtu.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, RequestExchangeMtu002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:RequestExchangeMtu002 start");
    int32_t mtu = 512; // mtu 512 bytes
    NlErrCode result = nearlinkSsapClientServer_->RequestExchangeMtu(appId_, mtu);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapClientServerTest:RequestExchangeMtu002 end");
}

/**
 * @tc.name: RequestConnectionPriority001
 * @tc.desc: Test RequestConnectionPriorityInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, RequestConnectionPriority001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:RequestConnectionPriority001 start");
    int32_t connPriority = 0;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteInt32(connPriority);
    NearlinkSsapClientStub::RequestConnectionPriorityInner(
        nearlinkSsapClientServer_.get(), data, reply);
    HILOGI("NearlinkSsapClientServerTest:RequestConnectionPriority001 end");
}

/**
 * @tc.name: RequestConnectionPriority002
 * @tc.desc: Test RequestConnectionPriority.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, RequestConnectionPriority002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:RequestConnectionPriority002 start");
    int32_t connPriority = 0;
    NlErrCode result = nearlinkSsapClientServer_->RequestConnectionPriority(appId_, connPriority);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapClientServerTest:RequestConnectionPriority002 end");
}


/**
 * @tc.name: GetServices001
 * @tc.desc: Test GetServicesInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, GetServices001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:GetServices001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(appId_);
    NearlinkSsapClientStub::GetServicesInner(
        nearlinkSsapClientServer_.get(), data, reply);
    HILOGI("NearlinkSsapClientServerTest:GetServices001 end");
}

/**
 * @tc.name: GetServices002
 * @tc.desc: Test GetServices.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, GetServices002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:GetServices002 start");
    std::vector<NearlinkSsapServiceParcel> service {};
    NlErrCode result = nearlinkSsapClientServer_->GetServices(appId_, service);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapClientServerTest:GetServices002 end");
}

/**
 * @tc.name: GetServicesByUuid001
 * @tc.desc: Test GetServicesByUuidInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, GetServicesByUuid001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:GetServicesByUuid001 start");
    NearlinkUuidParcel uuid(Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000000609")); // SLE_UUID_DIS
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&uuid);
    NearlinkSsapClientStub::GetServicesByUuidInner(
        nearlinkSsapClientServer_.get(), data, reply);
    HILOGI("NearlinkSsapClientServerTest:GetServicesByUuid001 end");
}

/**
 * @tc.name: GetServicesByUuid002
 * @tc.desc: Test GetServicesByUuid.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, GetServicesByUuid002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:GetServicesByUuid002 start");
    Uuid uuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000000609"); // SLE_UUID_DIS
    std::vector<NearlinkSsapServiceParcel> service {};
    NlErrCode result = nearlinkSsapClientServer_->GetServicesByUuid(appId_, uuid, service);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapClientServerTest:GetServicesByUuid002 end");
}

/**
 * @tc.name: RequestPropertyNotification001
 * @tc.desc: Test RequestNotificationInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, RequestPropertyNotification001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:RequestPropertyNotification001 start");
    uint16_t propertyHandle = 0;
    bool enable = true;
    uint8_t notifyOption = 0;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteInt16(propertyHandle);
    data.WriteBool(enable);
    data.WriteInt8(notifyOption);
    NearlinkSsapClientStub::RequestNotificationInner(
        nearlinkSsapClientServer_.get(), data, reply);
    HILOGI("NearlinkSsapClientServerTest:RequestPropertyNotification001 end");
}

/**
 * @tc.name: RequestPropertyNotification002
 * @tc.desc: Test RequestPropertyNotification.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, RequestPropertyNotification002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:RequestPropertyNotification002 start");
    uint16_t propertyHandle = 0;
    bool enable = true;
    uint8_t notifyOption = 0;
    std::vector<NearlinkSsapServiceParcel> service {};
    NlErrCode result = nearlinkSsapClientServer_->RequestPropertyNotification(
            appId_, propertyHandle, enable, notifyOption);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapClientServerTest:RequestPropertyNotification002 end");
}

/**
 * @tc.name: OnConnectionStateChanged001
 * @tc.desc: Test OnConnectionStateChanged.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnConnectionStateChanged001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnConnectionStateChanged001 start");
    EXPECT_CALL(*callback_, OnConnectionStateChanged);
    uint8_t newState = 1;
    int state = 0;
    ssapServerCallbackImpl_->OnConnectionStateChanged(newState, state);
    HILOGI("NearlinkSsapClientServerTest:OnConnectionStateChanged001 end");
}

/**
 * @tc.name: OnPropertyChanged001
 * @tc.desc: Test OnPropertyChanged.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnPropertyChanged001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnPropertyChanged001 start");
    EXPECT_CALL(*callback_, OnPropertyChanged);
    Property property {};
    ssapServerCallbackImpl_->OnPropertyChanged(property);
    HILOGI("NearlinkSsapClientServerTest:OnPropertyChanged001 end");
}

/**
 * @tc.name: OnEvent001
 * @tc.desc: Test OnEvent.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnEvent001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnEvent001 start");
    EXPECT_CALL(*callback_, OnEventNotified);
    Event event {};
    ssapServerCallbackImpl_->OnEvent(event);
    HILOGI("NearlinkSsapClientServerTest:OnEvent001 end");
}

/**
 * @tc.name: OnReadProperty001
 * @tc.desc: Test OnReadProperty.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnReadProperty001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnReadProperty001 start");
    EXPECT_CALL(*callback_, OnReadProperty);
    Property property {};
    int ret = 0;
    ssapServerCallbackImpl_->OnReadProperty(property, ret);
    HILOGI("NearlinkSsapClientServerTest:OnReadProperty001 end");
}

/**
 * @tc.name: OnCallMethod001
 * @tc.desc: Test OnCallMethod.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnCallMethod001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnCallMethod001 start");
    EXPECT_CALL(*callback_, OnCallMethod);
    Method method {};
    int ret = 0;
    ssapServerCallbackImpl_->OnCallMethod(method, ret);
    HILOGI("NearlinkSsapClientServerTest:OnCallMethod001 end");
}

/**
 * @tc.name: OnReadPropertiesByUuid001
 * @tc.desc: Test OnReadPropertiesByUuid.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnReadPropertiesByUuid001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnReadPropertiesByUuid001 start");
    EXPECT_CALL(*callback_, OnReadPropertiesByUuid);
    std::list<Property> list {};
    int ret = 0;
    ssapServerCallbackImpl_->OnReadPropertiesByUuid(list, ret);
    HILOGI("NearlinkSsapClientServerTest:OnReadPropertiesByUuid001 end");
}

/**
 * @tc.name: OnWriteProperty001
 * @tc.desc: Test OnWriteProperty.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnWriteProperty001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnWriteProperty001 start");
    EXPECT_CALL(*callback_, OnWriteProperty);
    Property property {};
    int ret = 0;
    ssapServerCallbackImpl_->OnWriteProperty(property, ret);
    HILOGI("NearlinkSsapClientServerTest:OnWriteProperty001 end");
}

/**
 * @tc.name: OnSetPropertyNotification001
 * @tc.desc: Test OnSetPropertyNotification.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnSetPropertyNotification001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnSetPropertyNotification001 start");
    EXPECT_CALL(*callback_, OnSetPropertyNotification);
    Property property {};
    bool enable = true;
    int ret = 0;
    ssapServerCallbackImpl_->OnSetPropertyNotification(property, enable, ret);
    HILOGI("NearlinkSsapClientServerTest:OnSetPropertyNotification001 end");
}

/**
 * @tc.name: OnSetPropertyIndication001
 * @tc.desc: Test OnSetPropertyIndication.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnSetPropertyIndication001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnSetPropertyIndication001 start");
    EXPECT_CALL(*callback_, OnSetPropertyIndication);
    Property property {};
    bool enable = true;
    int ret = 0;
    ssapServerCallbackImpl_->OnSetPropertyIndication(property, enable, ret);
    HILOGI("NearlinkSsapClientServerTest:OnSetPropertyIndication001 end");
}

/**
 * @tc.name: OnReadDescriptor001
 * @tc.desc: Test OnReadDescriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnReadDescriptor001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnReadDescriptor001 start");
    EXPECT_CALL(*callback_, OnReadDescriptor);
    Descriptor descriptor {};
    int ret = 0;
    ssapServerCallbackImpl_->OnReadDescriptor(descriptor, ret);
    HILOGI("NearlinkSsapClientServerTest:OnReadDescriptor001 end");
}

/**
 * @tc.name: OnReadDescriptorsByUuid001
 * @tc.desc: Test OnReadDescriptorsByUuid.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnReadDescriptorsByUuid001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnReadDescriptorsByUuid001 start");
    std::list<Descriptor> list {};
    int ret = 0;
    ssapServerCallbackImpl_->OnReadDescriptorsByUuid(list, ret);
    HILOGI("NearlinkSsapClientServerTest:OnReadDescriptorsByUuid001 end");
}

/**
 * @tc.name: OnWriteDescriptor001
 * @tc.desc: Test OnWriteDescriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnWriteDescriptor001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnWriteDescriptor001 start");
    EXPECT_CALL(*callback_, OnWriteDescriptor);
    Descriptor descriptor {};
    int ret = 0;
    ssapServerCallbackImpl_->OnWriteDescriptor(descriptor, ret);
    HILOGI("NearlinkSsapClientServerTest:OnWriteDescriptor001 end");
}

/**
 * @tc.name: OnMtuChanged001
 * @tc.desc: Test OnMtuChanged.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnMtuChanged001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnMtuChanged001 start");
    EXPECT_CALL(*callback_, OnMtuChanged);
    uint16_t mtu = 512; // mtu 512 bytes
    int state = 0;
    ssapServerCallbackImpl_->OnMtuChanged(mtu, state);
    HILOGI("NearlinkSsapClientServerTest:OnMtuChanged001 end");
}

/**
 * @tc.name: OnDiscoverComplete001
 * @tc.desc: Test OnDiscoverComplete.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnDiscoverComplete001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnDiscoverComplete001 start");
    EXPECT_CALL(*callback_, OnServicesDiscovered);
    int status = 0;
    ssapServerCallbackImpl_->OnDiscoverComplete(status);
    HILOGI("NearlinkSsapClientServerTest:OnDiscoverComplete001 end");
}

/**
 * @tc.name: OnDiscoverByUuidComplete001
 * @tc.desc: Test OnDiscoverByUuidComplete.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServerTest, OnDiscoverByUuidComplete001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServerTest:OnDiscoverByUuidComplete001 start");
    EXPECT_CALL(*callback_, OnServicesDiscoveredByUuid);
    Uuid uuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000000609"); // SLE_UUID_DIS;
    int status = 0;
    ssapServerCallbackImpl_->OnDiscoverByUuidComplete(uuid, status);
    HILOGI("NearlinkSsapClientServerTest:OnDiscoverByUuidComplete001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS