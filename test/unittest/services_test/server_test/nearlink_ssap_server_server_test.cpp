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
#include "nearlink_ssap_server_stub.h"
#include "nearlink_ssap_server_callback_stub.h"
#include "nearlink_ssap_server_callback_proxy.h"
#include "nearlink_ssap_server_server.cpp"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "log.h"
 
 
namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
 
namespace {
sptr<NearlinkSsapServerServer> g_ssapServer = new (std::nothrow) NearlinkSsapServerServer();
}

class NearlinkSsapServerCallbackStubTest : public NearlinkSsapServerCallbackStub {
public:
    void OnMtuChanged(const NearlinkSsapDevice &device, uint16_t mtu) override
    {}
    void OnAddService(const NearlinkSsapServiceParcel &service, int ret) override
    {}
    void OnPropertyReadRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapPropertyParcel &property, int ret) override
    {}
    void OnDescriptorReadRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapDescriptorParcel &descriptor, int ret) override
    {}
    void OnPropertyWriteRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapPropertyParcel &property, int ret) override
    {}
    void OnDescriptorWriteRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapDescriptorParcel &descriptor, int ret) override
    {}
    void OnNotifyPropertyChanged(
        const NearlinkSsapDevice &device, const Uuid &uuid, uint16_t handle, int ret) override
    {}
    void OnNotifyEventChanged(
        const NearlinkSsapDevice &device, const Uuid &uuid, uint16_t handle, int ret) override
    {}
    void OnConnectionStateChanged(const NearlinkSsapDevice &device, uint8_t state, int reason) override
    {}

    explicit NearlinkSsapServerCallbackStubTest()
    {}
    ~NearlinkSsapServerCallbackStubTest() override
    {}
};

class NearlinkSsapServerCallbackMock : public INearlinkSsapServerCallback {
public:
    MOCK_METHOD(void, OnMtuChanged, (const NearlinkSsapDevice &device, uint16_t mtu), (override));
    MOCK_METHOD(void, OnAddService, (const NearlinkSsapServiceParcel &service, int ret), (override));
    MOCK_METHOD(void, OnPropertyReadRequest,
        (const NearlinkSsapDevice &device, const NearlinkSsapPropertyParcel &property, int ret), (override));
    MOCK_METHOD(void, OnDescriptorReadRequest,
        (const NearlinkSsapDevice &device, const NearlinkSsapDescriptorParcel &descriptor, int ret), (override));
    MOCK_METHOD(void, OnPropertyWriteRequest,
        (const NearlinkSsapDevice &device, const NearlinkSsapPropertyParcel &property, int ret), (override));
    MOCK_METHOD(void, OnDescriptorWriteRequest,
        (const NearlinkSsapDevice &device, const NearlinkSsapDescriptorParcel &descriptor, int ret), (override));
    MOCK_METHOD(void, OnNotifyPropertyChanged,
        (const NearlinkSsapDevice &device, const Uuid &uuid, uint16_t handle, int ret), (override));
    MOCK_METHOD(void, OnNotifyEventChanged,
        (const NearlinkSsapDevice &device, const Uuid &uuid, uint16_t handle, int ret), (override));
    MOCK_METHOD(void, OnConnectionStateChanged,
        (const NearlinkSsapDevice &device, uint8_t state, int reason), (override));

    sptr<OHOS::IRemoteObject> AsObject() override
    {
        sptr<NearlinkSsapServerCallbackStubTest> nearlinkSsapServerCallbackStubTest =
        new NearlinkSsapServerCallbackStubTest();
        return nearlinkSsapServerCallbackStubTest;
    }
    ~NearlinkSsapServerCallbackMock() override = default;
};

class NearlinkSsapServerServerTest : public testing::Test {
public:
    NearlinkSsapServerServerTest()
    {}
    ~NearlinkSsapServerServerTest()
    {}

    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

    int appId_ = -1;
    NearlinkSsapDevice ssapDevice_ {};
    sptr<NearlinkSsapServerCallbackMock> callback_ {nullptr};
    sptr<NearlinkSsapServerCallbackStubTest> nearlinkSsapServerCallbackStubTest_ =
        new NearlinkSsapServerCallbackStubTest();
    std::unique_ptr<NearlinkSsapServerServer> nearlinkSsapServerServer_ {nullptr};
    std::unique_ptr<NearlinkSsapServerServer::impl::SsapServerCallbackImpl> ssapServerCallbackImpl_ {nullptr};
};

void NearlinkSsapServerServerTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkSsapServerServerTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    HILOGI("SetUpTestCase end");
}

void NearlinkSsapServerServerTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkSsapServerServerTest");
}

void NearlinkSsapServerServerTest::SetUp()
{
    HILOGI("SetUp NearlinkSsapServerServerTest.");
    nearlinkSsapServerServer_ = std::make_unique<NearlinkSsapServerServer>();
    callback_ = sptr<NearlinkSsapServerCallbackMock>(new NearlinkSsapServerCallbackMock());
    RawAddress remoteAddr("11:22:33:44:55:66");
    ssapDevice_.addr_ = remoteAddr;
    ssapDevice_.transport_ = SleTransport::ADAPTER_SLE;
    ssapServerCallbackImpl_ = std::make_unique<NearlinkSsapServerServer::impl::
        SsapServerCallbackImpl>(callback_);
    nearlinkSsapServerServer_->RegisterApplication(callback_, appId_);
}

void NearlinkSsapServerServerTest::TearDown()
{
    HILOGI("TearDown NearlinkSsapServerServerTest.");
    nearlinkSsapServerServer_ = nullptr;
    callback_ = nullptr;
    ssapServerCallbackImpl_ = nullptr;
}

/**
 * @tc.name: AddServiceTest001
 * @tc.desc: Test AddServiceInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, AddServiceTest001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:AddServiceTest001 start");
    NearlinkSsapServiceParcel service {};
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&service);
    NearlinkSsapServerStub::AddServiceInner(
        nearlinkSsapServerServer_.get(), data, reply);
    HILOGI("NearlinkSsapServerServerTest:AddServiceTest001 end");
}

/**
 * @tc.name: AddServiceTest002
 * @tc.desc: Test AddService.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, AddServiceTest002, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:AddServiceTest002 start");
    NearlinkSsapServiceParcel service {};
    NlErrCode result = nearlinkSsapServerServer_->AddService(appId_, &service);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapServerServerTest:AddServiceTest002 end");
}

/**
 * @tc.name: ClearServices001
 * @tc.desc: Test ClearServicesInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, ClearServices001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:ClearServices001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(appId_);
    NearlinkSsapServerStub::ClearServicesInner(
        nearlinkSsapServerServer_.get(), data, reply);
    HILOGI("NearlinkSsapServerServerTest:ClearServices001 end");
}

/**
 * @tc.name: ClearServices002
 * @tc.desc: Test ClearServices.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, ClearServices002, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:ClearServices002 start");
    NearlinkSsapServiceParcel service {};
    NlErrCode result = nearlinkSsapServerServer_->ClearServices(appId_);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapServerServerTest:ClearServices002 end");
}

/**
 * @tc.name: CancelConnection001
 * @tc.desc: Test CancelConnectionInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, CancelConnection001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:CancelConnection001 start");
    NearlinkSsapDevice device {ssapDevice_};
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&device);
    NearlinkSsapServerStub::CancelConnectionInner(
        nearlinkSsapServerServer_.get(), data, reply);
    HILOGI("NearlinkSsapServerServerTest:CancelConnection001 end");
}

/**
 * @tc.name: CancelConnection002
 * @tc.desc: Test CancelConnection.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, CancelConnection002, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:CancelConnection002 start");
    NearlinkSsapDevice device {ssapDevice_};
    NlErrCode result = nearlinkSsapServerServer_->CancelConnection(appId_, device);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, result);
    HILOGI("NearlinkSsapServerServerTest:CancelConnection002 end");
}

/**
 * @tc.name: NotifyClient001
 * @tc.desc: Test NotifyClientInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, NotifyClient001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:NotifyClient001 start");
    NearlinkSsapDevice device {ssapDevice_};
    NearlinkSsapPropertyParcel property {};
    bool needConfirm = false;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&property);
    data.WriteParcelable(&device);
    data.WriteBool(needConfirm);
    NearlinkSsapServerStub::NotifyClientInner(
        nearlinkSsapServerServer_.get(), data, reply);
    HILOGI("NearlinkSsapServerServerTest:NotifyClient001 end");
}

/**
 * @tc.name: NotifyClient002
 * @tc.desc: Test NotifyClient.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, NotifyClient002, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:NotifyClient002 start");
    NearlinkSsapDevice device {ssapDevice_};
    NearlinkSsapPropertyParcel property {};
    NlErrCode result = nearlinkSsapServerServer_->NotifyClient(
        appId_, &property, device, true);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapServerServerTest:NotifyClient002 end");
}

/**
 * @tc.name: NotifyEvent001
 * @tc.desc: Test NotifyEventInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, NotifyEvent001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:NotifyEvent001 start");
    std::vector<uint8_t> value(4, 0xFF);
    NearlinkSsapEventParcel event {};
    NearlinkSsapDevice device {ssapDevice_};
    bool needConfirm = false;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&event);
    data.WriteUInt8Vector(value);
    data.WriteParcelable(&device);
    data.WriteBool(needConfirm);
    NearlinkSsapServerStub::NotifyEventInner(
        nearlinkSsapServerServer_.get(), data, reply);
    HILOGI("NearlinkSsapServerServerTest:NotifyEvent001 end");
}

/**
 * @tc.name: NotifyEvent002
 * @tc.desc: Test NotifyEvent.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, NotifyEvent002, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:NotifyEvent002 start");
    std::vector<uint8_t> value(4, 0xFF);
    NearlinkSsapEventParcel event {};
    NearlinkSsapDevice device {ssapDevice_};
    bool needConfirm = false;
    NlErrCode result = nearlinkSsapServerServer_->NotifyEvent(
        appId_, &event, value, device, needConfirm);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, result);
    HILOGI("NearlinkSsapServerServerTest:NotifyEvent002 end");
}

/**
 * @tc.name: SetPropertyValue001
 * @tc.desc: Test SetPropertyValueInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, SetPropertyValue001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:SetPropertyValue001 start");
    NearlinkSsapPropertyParcel property {};
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&property);
    NearlinkSsapServerStub::SetPropertyValueInner(
        nearlinkSsapServerServer_.get(), data, reply);
    HILOGI("NearlinkSsapServerServerTest:SetPropertyValue001 end");
}

/**
 * @tc.name: SetPropertyValue002
 * @tc.desc: Test SetPropertyValue.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, SetPropertyValue002, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:SetPropertyValue002 start");
    NearlinkSsapPropertyParcel property {};
    NlErrCode result = nearlinkSsapServerServer_->SetPropertyValue(appId_, &property);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapServerServerTest:SetPropertyValue002 end");
}

/**
 * @tc.name: SetDescriptorValue001
 * @tc.desc: Test SetDescriptorValue.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, SetDescriptorValue001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:SetDescriptorValue001 start");
    NearlinkSsapDescriptorParcel descriptor {};
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&descriptor);
    NearlinkSsapServerStub::SetDescriptorValueInner(
        nearlinkSsapServerServer_.get(), data, reply);;
    HILOGI("NearlinkSsapServerServerTest:SetDescriptorValue001 end");
}

/**
 * @tc.name: SetDescriptorValue002
 * @tc.desc: Test SetDescriptorValue.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, SetDescriptorValue002, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:SetDescriptorValue002 start");
    NearlinkSsapDescriptorParcel descriptor {};
    NlErrCode result = nearlinkSsapServerServer_->SetDescriptorValue(appId_, &descriptor);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapServerServerTest:SetDescriptorValue002 end");
}

/**
 * @tc.name: Connect001
 * @tc.desc: Test ConnectInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, Connect001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:Connect001 start");
    NearlinkSsapDevice device {ssapDevice_};
    uint8_t secureReq = 0;
    bool autoConnect = false;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&device);
    data.WriteUint8(secureReq);
    data.WriteBool(autoConnect);
    NearlinkSsapServerStub::ConnectInner(
        nearlinkSsapServerServer_.get(), data, reply);
    HILOGI("NearlinkSsapServerServerTest:Connect001 end");
}

/**
 * @tc.name: Connect002
 * @tc.desc: Test Connect.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, Connect002, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:Connect002 start");
    NearlinkSsapDevice device {ssapDevice_};
    uint8_t secureReq = 0;
    bool autoConnect = false;
    NlErrCode result = nearlinkSsapServerServer_->Connect(
        appId_, device, secureReq, autoConnect);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, result);
    HILOGI("NearlinkSsapServerServerTest:Connect002 end");
}

/**
 * @tc.name: RemoveService001
 * @tc.desc: Test RemoveServiceInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, RemoveService001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:RemoveService001 start");
    NearlinkSsapServiceParcel services {};
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteParcelable(&services);
    NearlinkSsapServerStub::RemoveServiceInner(
        nearlinkSsapServerServer_.get(), data, reply);
    HILOGI("NearlinkSsapServerServerTest:RemoveService001 end");
}

/**
 * @tc.name: RemoveService002
 * @tc.desc: Test RemoveService.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, RemoveService002, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:RemoveService002 start");
    NearlinkSsapServiceParcel services {};
    NlErrCode result = nearlinkSsapServerServer_->RemoveService(appId_, services);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapServerServerTest:RemoveService002 end");
}

/**
 * @tc.name: AuthorizeResponse001
 * @tc.desc: Test AuthorizeResponseInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, AuthorizeResponse001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:AuthorizeResponse001 start");
    uint16_t requestId = 0;
    bool allow = true;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(appId_);
    data.WriteUint16(requestId);
    data.WriteBool(allow);
    NearlinkSsapServerStub::AuthorizeResponseInner(
        nearlinkSsapServerServer_.get(), data, reply);
    HILOGI("NearlinkSsapServerServerTest:AuthorizeResponse001 end");
}

/**
 * @tc.name: AuthorizeResponse002
 * @tc.desc: Test AuthorizeResponse.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, AuthorizeResponse002, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:AuthorizeResponse002 start");
    uint16_t requestId = 0;
    bool allow = true;
    NlErrCode result = nearlinkSsapServerServer_->AuthorizeResponse(appId_, requestId, allow);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapServerServerTest:AuthorizeResponse002 end");
}

/**
 * @tc.name: DeregisterApplication001
 * @tc.desc: Test DeregisterApplicationInner.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, DeregisterApplication001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:DeregisterApplication001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(appId_);
    NearlinkSsapServerStub::DeregisterApplicationInner(
        nearlinkSsapServerServer_.get(), data, reply);
    HILOGI("NearlinkSsapServerServerTest:DeregisterApplication001 end");
}

/**
 * @tc.name: DeregisterApplication002
 * @tc.desc: Test DeregisterApplication.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, DeregisterApplication002, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:DeregisterApplication002 start");
    NlErrCode result = nearlinkSsapServerServer_->DeregisterApplication(appId_);
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSsapServerServerTest:DeregisterApplication002 end");
}

/**
 * @tc.name: OnMtuChanged001
 * @tc.desc: Test OnMtuChanged.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnMtuChanged001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnMtuChanged001 start");
    uint16_t mtu = 512;
    ssapServerCallbackImpl_->OnMtuChanged(ssapDevice_.addr_, ssapDevice_.transport_, mtu);
    HILOGI("NearlinkSsapServerServerTest:OnMtuChanged001 end");
}

/**
 * @tc.name: OnAddService001
 * @tc.desc: Test OnAddService.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnAddService001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnAddService001 start");
    Service service {};
    ssapServerCallbackImpl_->OnAddService(service, 0);
    HILOGI("NearlinkSsapServerServerTest:OnAddService001 end");
}

/**
 * @tc.name: OnSetPropertyValue001
 * @tc.desc: Test OnSetPropertyValue.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnSetPropertyValue001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnSetPropertyValue001 start");
    Property property {};
    ssapServerCallbackImpl_->OnSetPropertyValue(property, 0);
    HILOGI("NearlinkSsapServerServerTest:OnSetPropertyValue001 end");
}

/**
 * @tc.name: OnSetDescriptorValue001
 * @tc.desc: Test OnSetDescriptorValue.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnSetDescriptorValue001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnSetDescriptorValue001 start");
    Descriptor descriptor {};
    ssapServerCallbackImpl_->OnSetDescriptorValue(descriptor, 0);
    HILOGI("NearlinkSsapServerServerTest:OnSetDescriptorValue001 end");
}

/**
 * @tc.name: OnReadPropertyAuthorizeRequest001
 * @tc.desc: Test OnReadPropertyAuthorizeRequest.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnReadPropertyAuthorizeRequest001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnReadPropertyAuthorizeRequest001 start");
    uint16_t requestId = 1;
    Property property {};
    ssapServerCallbackImpl_->OnReadPropertyAuthorizeRequest(
        ssapDevice_.addr_, ssapDevice_.transport_, requestId, property);
    HILOGI("NearlinkSsapServerServerTest:OnReadPropertyAuthorizeRequest001 end");
}

/**
 * @tc.name: OnReadDescriptorAuthorizeRequest001
 * @tc.desc: Test OnReadDescriptorAuthorizeRequest.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnReadDescriptorAuthorizeRequest001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnReadDescriptorAuthorizeRequest001 start");
    uint16_t requestId = 1;
    Descriptor descriptor {};
    ssapServerCallbackImpl_->OnReadDescriptorAuthorizeRequest(
        ssapDevice_.addr_, ssapDevice_.transport_, requestId, descriptor);
    HILOGI("NearlinkSsapServerServerTest:OnReadDescriptorAuthorizeRequest001 end");
}

/**
 * @tc.name: OnWritePropertyAuthorizeRequest001
 * @tc.desc: Test OnWritePropertyAuthorizeRequest.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnWritePropertyAuthorizeRequest001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnWritePropertyAuthorizeRequest001 start");
    uint16_t requestId = 1;
    Property property {};
    ssapServerCallbackImpl_->OnWritePropertyAuthorizeRequest(
        ssapDevice_.addr_, ssapDevice_.transport_, requestId, property);
    HILOGI("NearlinkSsapServerServerTest:OnWritePropertyAuthorizeRequest001 end");
}

/**
 * @tc.name: OnWriteDescriptorAuthorizeRequest001
 * @tc.desc: Test OnWriteDescriptorAuthorizeRequest.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnWriteDescriptorAuthorizeRequest001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnWriteDescriptorAuthorizeRequest001 start");
    uint16_t requestId = 1;
    Descriptor descriptor {};
    ssapServerCallbackImpl_->OnWriteDescriptorAuthorizeRequest(
        ssapDevice_.addr_, ssapDevice_.transport_, requestId, descriptor);
    HILOGI("NearlinkSsapServerServerTest:OnWriteDescriptorAuthorizeRequest001 end");
}

/**
 * @tc.name: OnReadProperty001
 * @tc.desc: Test OnReadProperty.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnReadProperty001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnReadProperty001 start");
    Property property {};
    int ret = 0;
    ssapServerCallbackImpl_->OnReadProperty(
        ssapDevice_.addr_, ssapDevice_.transport_, property, ret);
    HILOGI("NearlinkSsapServerServerTest:OnReadProperty001 end");
}

/**
 * @tc.name: OnReadDescriptor001
 * @tc.desc: Test OnReadDescriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnReadDescriptor001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnReadDescriptor001 start");
    Descriptor descriptor {};
    int ret = 0;
    ssapServerCallbackImpl_->OnReadDescriptor(
        ssapDevice_.addr_, ssapDevice_.transport_, descriptor, ret);
    HILOGI("NearlinkSsapServerServerTest:OnReadDescriptor001 end");
}

/**
 * @tc.name: OnWriteProperty001
 * @tc.desc: Test OnWriteProperty.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnWriteProperty001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnWriteProperty001 start");
    Property property {};
    int ret = 0;
    ssapServerCallbackImpl_->OnWriteProperty(
        ssapDevice_.addr_, ssapDevice_.transport_, property, ret);
    HILOGI("NearlinkSsapServerServerTest:OnWriteProperty001 end");
}

/**
 * @tc.name: OnWriteDescriptor001
 * @tc.desc: Test OnWriteDescriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnWriteDescriptor001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnWriteDescriptor001 start");
    Descriptor descriptor {};
    int ret = 0;
    ssapServerCallbackImpl_->OnWriteDescriptor(
        ssapDevice_.addr_, ssapDevice_.transport_, descriptor, ret);
    HILOGI("NearlinkSsapServerServerTest:OnWriteDescriptor001 end");
}

/**
 * @tc.name: OnNotifyProperty001
 * @tc.desc: Test OnNotifyProperty.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnNotifyProperty001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnNotifyProperty001 start");
    Property property {};
    int ret = 0;
    ssapServerCallbackImpl_->OnNotifyProperty(
        ssapDevice_.addr_, ssapDevice_.transport_, property, ret);
    HILOGI("NearlinkSsapServerServerTest:OnNotifyProperty001 end");
}


/**
 * @tc.name: OnNotifyEvent001
 * @tc.desc: Test OnNotifyEvent.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnNotifyEvent001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnNotifyEvent001 start");
    Event event {};
    int ret = 0;
    ssapServerCallbackImpl_->OnNotifyEvent(
        ssapDevice_.addr_, ssapDevice_.transport_, event, ret);
    HILOGI("NearlinkSsapServerServerTest:OnNotifyEvent001 end");
}

/**
 * @tc.name: OnConnectionStateChanged001
 * @tc.desc: Test OnConnectionStateChanged.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServerTest, OnConnectionStateChanged001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServerTest:OnConnectionStateChanged001 start");
    uint8_t state = static_cast<uint8_t>(SleConnState::SLE_CONNECTION_STATE_CONNECTED);
    ssapServerCallbackImpl_->OnConnectionStateChanged(ssapDevice_.addr_, ssapDevice_.transport_, state, 0, 0);
    HILOGI("NearlinkSsapServerServerTest:OnConnectionStateChanged001 end");
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS