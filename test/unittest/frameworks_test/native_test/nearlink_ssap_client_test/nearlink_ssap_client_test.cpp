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

#include "nearlink_access_token_mock.h"
#include "nearlink_def.h"
#include "nearlink_host.h"
#include "nearlink_ssap_client.cpp"
#include "nearlink_ssap_client_proxy.h"
#include "nearlink_ssap_client_callback_stub.h"
#include "nearlink_remote_device.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
namespace {
constexpr int SLEEP_TIME = 2;
const std::string SERVICE_UUID_STRING = "11223344-0000-1000-8000-00805F9B34FB";
const std::string PROPERTY_UUID_STRING = "11223344-0000-1000-8000-00805F9B1111";

SsapService CreateSsapService()
{
    Nearlink::UUID serviceUuid =  Nearlink::UUID::FromString(SERVICE_UUID_STRING);
    SsapService service(serviceUuid, SsapServiceType::VENDOR_PROMARY);
    Nearlink::UUID propertyUuid =  Nearlink::UUID::FromString(PROPERTY_UUID_STRING);
    uint32_t operationIndication = static_cast<uint32_t>(SsapProperty::OperationIndication::OPERATION_READ) +
        static_cast<uint32_t>(SsapProperty::OperationIndication::OPERATION_WRITE_NO_RESPONSE) +
        static_cast<uint32_t>(SsapProperty::OperationIndication::OPERATION_NOTIFY);
    SsapProperty property(static_cast<int>(SsapProperty::PropertyType::ENTRY_TYPE_PROPERTY), propertyUuid,
        operationIndication, 0);
    const size_t valueLen = 3; // length of property value is 3
    uint8_t propertyValue[valueLen] = {1, 1, 1}; // Set property value as {1, 1, 1}
    property.SetValue(propertyValue, valueLen);
    service.AddProperty(property);
    SsapDescriptor descriptor(0,
        SsapDescriptor::PropertyDescriptorType::DESCRIPTOR_TYPE_CLIENT_PROPERTY_CONFIG, 17); // permission 17
    uint8_t descriptorValue[valueLen] = {2, 2, 2}; // Set descriptor value as {2, 2, 2}
    descriptor.SetValue(descriptorValue, valueLen);
    service.descriptors_.push_back(descriptor);
    return service;
}

NearlinkSsapServiceParcel CreateSsapServiceParcel(SsapService &service)
{
    NearlinkSsapServiceParcel serviceParcel;
    serviceParcel.isPrimary_ = service.IsPrimary();
    serviceParcel.uuid_ = Uuid::ConvertFrom128Bits(service.GetUuid().ConvertTo128Bits());

    for (auto &isvc : service.GetIncludedServices()) {
        serviceParcel.includeServices_.push_back(Service(isvc->GetHandle()));
    }

    for (auto &proper : service.GetProperty()) {
        size_t length = 0;
        uint8_t *value = proper.GetValue(&length).get();
        std::vector<uint8_t> vecValue(value, value + length);
        Property p(proper.GetHandle(),
            Uuid::ConvertFrom128Bits(proper.GetUuid().ConvertTo128Bits()),
            vecValue,
            proper.GetOperationIndication(),
            proper.GetValuePermissions());

        for (auto &desc : proper.GetDescriptors()) {
            value = desc.GetValue(&length).get();
            std::vector<uint8_t> temp(value, value + length);
            vecValue = std::move(temp);
            Descriptor d(desc.GetHandle(),
                desc.GetDescriptorType(),
                std::move(vecValue),
                desc.GetDescriptorPermission());

            p.descriptors_.push_back(std::move(d));
        }

        serviceParcel.properties_.push_back(std::move(p));
    }
    return NearlinkSsapServiceParcel{serviceParcel};
}
}

class SsapClientCallbackTest final: public Nearlink::SsapClientCallback {
public:
    SsapClientCallbackTest() {};
    ~SsapClientCallbackTest() {};

    void OnConnectionStateChanged(int connectionState, int ret)
    {
        HILOGI("connectionState(%{public}d), ret(%{public}d)", connectionState, ret);
    }
    void OnPropertyChanged(const SsapProperty &property) {}
    void OnPropertyReadResult(const SsapProperty &property, int ret) {}
    void OnPropertiesReadResult(const std::vector<SsapProperty> &properties, int ret) {}
    void OnReadRemoteRssiValueResult(int rssi, int status) {}
    void OnPropertyWriteResult(const SsapProperty &property, int ret) {}
    void OnMethodCallResult(const SsapMethod &method, int ret) {}
    void OnDescriptorReadResult(const SsapDescriptor &descriptor, int ret) {}
    void OnDescriptorWriteResult(const SsapDescriptor &descriptor, int ret) {}
    void OnMtuUpdate(uint16_t mtu, int ret) {}
    void OnServicesDiscovered(int status) {}
    void OnServicesDiscoveredByUuid(int status, const UUID &uuid) {}
    void OnConnectionParameterChanged(int interval, int latency, int timeout, int status) {}
    void OnSetPropertyNotifyResult(const SsapProperty &property, int enable, int ret) {}
};

class NearlinkClientServerCallbackStubTest : public NearlinkSsapClientCallbackStub {
public:
    void OnConnectionStateChanged(int32_t state, int32_t newState) override {}
    void OnPropertyChanged(const NearlinkSsapPropertyParcel &property) override {}
    void OnEventNotified(const NearlinkSsapEventParcel &event) override {}
    void OnReadProperty(int32_t ret, const NearlinkSsapPropertyParcel &property) override {}
    void OnCallMethod(int32_t ret, const NearlinkSsapMethodParcel &method) override {}
    void OnReadPropertiesByUuid(int32_t ret, const std::vector<NearlinkSsapPropertyParcel> &properties) override {}
    void OnWriteProperty(int32_t ret, const NearlinkSsapPropertyParcel &property) override {}
    void OnSetPropertyNotification(int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property) override {}
    void OnReadDescriptor(int32_t ret, const NearlinkSsapDescriptorParcel &descriptor) override {}
    void OnWriteDescriptor(int32_t ret, const NearlinkSsapDescriptorParcel &descriptor) override {}
    void OnMtuChanged(int32_t state, uint16_t mtu) override {}
    void OnServicesDiscovered(int32_t status) override {}
    void OnServicesDiscoveredByUuid(int32_t status, const Uuid &uuid) override {}
    void OnConnectionParameterChanged(int32_t interval, int32_t latency, int32_t timeout, int32_t status) override {}
};

class NearlinkSsapClientTest : public testing::Test {
public:
    int tempData_ = 0;
    std::shared_ptr<NearlinkRemoteDevice> device_;
    std::shared_ptr<SsapClient> ssapClient_ {nullptr};
    std::shared_ptr<SsapClientCallbackTest> ssapClientcallback_;
    sptr<SsapClient::impl::NearlinkSsapClientCallbackStubImpl> clientCallback_;
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkSsapClientTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    if (!NearlinkHost::GetInstance().IsSleEnabled()) {
        HILOGI("start enable nearlink");
        NearlinkHost::GetInstance().EnableNl();
        sleep(2); // sleep 2s
    }
    HILOGI("SetUpTestCase end");
}

void NearlinkSsapClientTest::TearDownTestCase()
{}

void NearlinkSsapClientTest::SetUp()
{
    tempData_ = 0;
    string randomAddr = "11:22:33:44:55:66";
    device_ = std::make_shared<NearlinkRemoteDevice>(
        randomAddr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    ssapClient_ = SsapClient::CreateSsapClient(device_);
    ssapClientcallback_ = std::make_shared<SsapClientCallbackTest>();
    clientCallback_ = new SsapClient::impl::NearlinkSsapClientCallbackStubImpl(ssapClient_);
    ssapClient_->pimpl->clientCallback_ = clientCallback_;
}

void NearlinkSsapClientTest::TearDown()
{
    ssapClient_->Close();
    sleep(2); // sleep 2s
    ssapClient_ = nullptr;
}

/**
 * @tc.name: ConnectAndDisconnect001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, ConnectAndDisconnect001, TestSize.Level1)
{
    HILOGI("ConnectAndDisconnect001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    // Asynchronous interface switching thread in the service.
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(SLEEP_TIME);
    ret = ssapClient_->Disconnect();
    // no paired remote deivce, is not connected, return NL_ERR_INVALID_STATE
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("ConnectAndDisconnect001 end");
}

/**
 * @tc.name: RequestConnectionPriority001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, RequestConnectionPriority001, TestSize.Level1)
{
    HILOGI("RequestConnectionPriority001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(SLEEP_TIME);
    ssapClient_->pimpl->connectionState_ = static_cast<int>(SleConnectState::CONNECTED);
    int connPriority = static_cast<int>(SsapConnectionPriority::BALANCED);
    ret = ssapClient_->RequestConnectionPriority(connPriority);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("RequestConnectionPriority001 end");
}

/**
 * @tc.name: FindStructure001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, FindStructure001, TestSize.Level1)
{
    HILOGI("FindStructure001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(SLEEP_TIME);
    ssapClient_->pimpl->connectionState_ = static_cast<int>(SleConnectState::CONNECTED);
    UUID serviceUuid =  UUID::FromString(SERVICE_UUID_STRING);
    ret = ssapClient_->FindStructureByUuid(serviceUuid);
    ssapClient_->pimpl->FindStructureComplete(NL_NO_ERROR);
    EXPECT_EQ(NL_NO_ERROR, ret);
    ret = ssapClient_->FindStructure();
    ssapClient_->pimpl->FindStructureComplete(NL_NO_ERROR);
    EXPECT_EQ(NL_NO_ERROR, ret);
    std::vector<SsapService> services {};
    ret = ssapClient_->GetService(services);
    ssapClient_->pimpl->FindStructureComplete(NL_NO_ERROR);
    EXPECT_EQ(NL_NO_ERROR, ret);
    EXPECT_TRUE(services.empty());
    std::shared_ptr<SsapService> servicePtr = nullptr;
    servicePtr = ssapClient_->GetService(serviceUuid);
    EXPECT_EQ(servicePtr, nullptr);
    HILOGI("FindStructure001 end");
}

/**
 * @tc.name: ReadAndWriteProperty001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, ReadAndWriteProperty001, TestSize.Level1)
{
    HILOGI("ReadAndWriteProperty001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    // Asynchronous interface switching thread in the service.
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(SLEEP_TIME);

    ssapClient_->pimpl->connectionState_ = static_cast<int>(SleConnectState::CONNECTED);
    SsapService ssapService = CreateSsapService();
    NearlinkSsapServiceParcel serviceParcel = CreateSsapServiceParcel(ssapService);
    vector<NearlinkSsapServiceParcel> serviceParcelVec {1, serviceParcel};
    ssapClient_->pimpl->BuildServiceList(serviceParcelVec);
    UUID uuid = UUID::FromString(PROPERTY_UUID_STRING);
    uint32_t operationIndication = 11; // operIndicate
    SsapProperty property(static_cast<int>(SsapProperty::PropertyType::ENTRY_TYPE_PROPERTY), uuid,
        operationIndication, 0);
    const size_t valueLen = 3; // length of property value is 3
    uint8_t propertyValue[valueLen] = {1, 2, 3}; // Set property value as {1, 2, 3}
    property.SetValue(propertyValue, valueLen);
    ret = ssapClient_->ReadProperty(property);
    EXPECT_EQ(NL_NO_ERROR, ret);
    ret = ssapClient_->WriteProperty(property);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("ReadAndWriteProperty001 end");
}

/**
 * @tc.name: CallMethod001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, CallMethod001, TestSize.Level1)
{
    HILOGI("CallMethod001 start");
    uint8_t parameter[] = {0x03, 0x01};
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    // Asynchronous interface switching thread in the service.
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(SLEEP_TIME);
    ssapClient_->pimpl->connectionState_ = static_cast<int>(SleConnectState::CONNECTED);
    uint16_t handle = 0;
    int type = 0;
    UUID uuid = UUID::FromString(SERVICE_UUID_STRING);
    int permission = 0;
    SsapMethod method(handle, type, uuid, permission);
    method.SetParameter(parameter, 2);
    ret = ssapClient_->CallMethod(method);
    EXPECT_NE(NL_NO_ERROR, ret);
    HILOGI("CallMethod001 end");
}

/**
 * @tc.name: ReadAndWriteDescriptor001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, ReadAndWriteDescriptor001, TestSize.Level1)
{
    HILOGI("ReadAndWriteDescriptor001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    // Asynchronous interface switching thread in the service.
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(SLEEP_TIME);

    ssapClient_->pimpl->connectionState_ = static_cast<int>(SleConnectState::CONNECTED);
    uint16_t handle = 0;
    UUID serviceUuid =  UUID::FromString(SERVICE_UUID_STRING);
    UUID propertyUuid =  UUID::FromString(PROPERTY_UUID_STRING);
    bool result = ssapClient_->GetHandle(serviceUuid, propertyUuid, handle);
    EXPECT_FALSE(result);
    HILOGI("get handle: %{public}d", handle);
    SsapDescriptor descriptor(handle,
        SsapDescriptor::PropertyDescriptorType::DESCRIPTOR_TYPE_CLIENT_PROPERTY_CONFIG, 17); // permission 17
    const size_t valueLen = 3; // length of descriptor value is 3
    uint8_t descriptorValue[valueLen] = {2, 2, 2}; // Set descriptor value as {2, 2, 2}
    descriptor.SetValue(descriptorValue, valueLen);
    ret = ssapClient_->ReadDescriptor(descriptor);
    EXPECT_EQ(NL_NO_ERROR, ret);
    ret = ssapClient_->WriteDescriptor(descriptor);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, ret);
    HILOGI("ReadAndWriteDescriptor001 end");
}

/**
 * @tc.name: RequestSleMtuSize001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, RequestSleMtuSize001, TestSize.Level1)
{
    HILOGI("RequestSleMtuSize001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(SLEEP_TIME);
    ssapClient_->pimpl->connectionState_ = static_cast<int>(SleConnectState::CONNECTED);
    int mtu = 512; // MTU 512 bytes
    ret = ssapClient_->RequestSleMtuSize(mtu);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("RequestSleMtuSize001 end");
}

/**
 * @tc.name: SetNotifyProperty001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, SetNotifyProperty001, TestSize.Level1)
{
    HILOGI("SetNotifyProperty001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    // Asynchronous interface switching thread in the service.
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(SLEEP_TIME);

    ssapClient_->pimpl->connectionState_ = static_cast<int>(SleConnectState::CONNECTED);
    SsapService ssapService = CreateSsapService();
    NearlinkSsapServiceParcel serviceParcel = CreateSsapServiceParcel(ssapService);
    vector<NearlinkSsapServiceParcel> serviceParcelVec {1, serviceParcel};
    ssapClient_->pimpl->BuildServiceList(serviceParcelVec);
    UUID uuid = UUID::FromString(PROPERTY_UUID_STRING);
    uint32_t operationIndication = 11; // operIndicate
    SsapProperty property(static_cast<int>(SsapProperty::PropertyType::ENTRY_TYPE_PROPERTY), uuid,
        operationIndication, 0);
    const size_t valueLen = 3; // length of property value is 3
    uint8_t propertyValue[valueLen] = {1, 2, 3}; // Set property value as {1, 2, 3}
    property.SetValue(propertyValue, valueLen);
    ret = ssapClient_->SetNotifyProperty(property, true);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("SetNotifyProperty001 end");
}

/**
 * @tc.name: OnConnectionStateChanged001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, OnConnectionStateChanged001, TestSize.Level1)
{
    HILOGI("OnConnectionStateChanged001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    int32_t state = static_cast<int>(SleConnectState::DISCONNECTED);
    int32_t newState = static_cast<int>(SleConnectState::CONNECTED);
    clientCallback_->OnConnectionStateChanged(state, newState);
    HILOGI("OnConnectionStateChanged001 end");
}

/**
 * @tc.name: OnEventNotified001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, OnEventNotified001, TestSize.Level1)
{
    HILOGI("OnEventNotified001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    NearlinkSsapEventParcel event {};
    clientCallback_->OnEventNotified(event);
    HILOGI("OnEventNotified001 end");
}

/**
 * @tc.name: OnPropertyChanged001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, OnPropertyChanged001, TestSize.Level1)
{
    HILOGI("OnPropertyChanged001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    NearlinkSsapPropertyParcel property {};
    clientCallback_->OnPropertyChanged(property);
    HILOGI("OnPropertyChanged001 end");
}

/**
 * @tc.name: OnReadProperty001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, OnReadProperty001, TestSize.Level1)
{
    HILOGI("OnReadProperty001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    int32_t retValue = 0;
    NearlinkSsapPropertyParcel property {};
    clientCallback_->OnReadProperty(retValue, property);
    HILOGI("OnReadProperty001 end");
}

/**
 * @tc.name: OnCallMethod001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, OnCallMethod001, TestSize.Level1)
{
    HILOGI("OnCallMethod001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    int32_t retValue = 0;
    NearlinkSsapMethodParcel method {};
    clientCallback_->OnCallMethod(retValue, method);
    HILOGI("OnCallMethod001 end");
}

/**
 * @tc.name: OnReadPropertiesByUuid001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, OnReadPropertiesByUuid001, TestSize.Level1)
{
    HILOGI("OnReadPropertiesByUuid001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    int32_t retValue = 0;
    NearlinkSsapPropertyParcel property {};
    std::vector<NearlinkSsapPropertyParcel> properties {1, property};
    clientCallback_->OnReadPropertiesByUuid(retValue, properties);
    HILOGI("OnReadPropertiesByUuid001 end");
}

/**
 * @tc.name: OnWriteProperty001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, OnWriteProperty001, TestSize.Level1)
{
    HILOGI("OnWriteProperty001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    int32_t retValue = 0;
    NearlinkSsapPropertyParcel property {};
    clientCallback_->OnWriteProperty(retValue, property);
    HILOGI("OnWriteProperty001 end");
}

/**
 * @tc.name: OnSetPropertyNotification001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, OnSetPropertyNotification001, TestSize.Level1)
{
    HILOGI("OnSetPropertyNotification001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    int32_t retValue = 0;
    NearlinkSsapPropertyParcel property {};
    clientCallback_->OnSetPropertyNotification(retValue, true, property);
    HILOGI("OnSetPropertyNotification001 end");
}

/**
 * @tc.name: OnReadDescriptor001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, OnReadDescriptor001, TestSize.Level1)
{
    HILOGI("OnReadDescriptor001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    int32_t retValue = 0;
    NearlinkSsapDescriptorParcel descriptor {};
    clientCallback_->OnReadDescriptor(retValue, descriptor);
    HILOGI("OnReadDescriptor001 end");
}

/**
 * @tc.name: OnWriteDescriptor001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, OnWriteDescriptor001, TestSize.Level1)
{
    HILOGI("OnWriteDescriptor001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    int32_t retValue = 0;
    NearlinkSsapDescriptorParcel descriptor {};
    clientCallback_->OnWriteDescriptor(retValue, descriptor);
    HILOGI("OnWriteDescriptor001 end");
}

/**
 * @tc.name: OnMtuChanged001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, OnMtuChanged001, TestSize.Level1)
{
    HILOGI("OnMtuChanged001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    int state = 0;
    uint16_t mtu = 512; // mtu 512 bytes
    clientCallback_->OnMtuChanged(state, mtu);
    HILOGI("OnMtuChanged001 end");
}

/**
 * @tc.name: OnServicesDiscovered001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, OnServicesDiscovered001, TestSize.Level1)
{
    HILOGI("OnServicesDiscovered001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    int32_t status = 0;
    clientCallback_->OnServicesDiscovered(status);
    HILOGI("OnServicesDiscovered001 end");
}

/**
 * @tc.name: OnServicesDiscoveredByUuid001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, OnServicesDiscoveredByUuid001, TestSize.Level1)
{
    HILOGI("OnServicesDiscoveredByUuid001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    int32_t status = 0;
    Uuid uuid {};
    clientCallback_->OnServicesDiscoveredByUuid(status, uuid);
    HILOGI("OnServicesDiscoveredByUuid001 end");
}

/**
 * @tc.name: OnConnectionParameterChanged001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientTest, OnConnectionParameterChanged001, TestSize.Level1)
{
    HILOGI("OnConnectionParameterChanged001 start");
    NlErrCode ret = ssapClient_->Connect(ssapClientcallback_);
    EXPECT_EQ(NL_NO_ERROR, ret);

    int32_t interval = 0;
    int32_t latency = 0;
    int32_t timeout = 0;
    int32_t status = 0;
    clientCallback_->OnConnectionParameterChanged(interval, latency, timeout, status);
    HILOGI("OnConnectionParameterChanged001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
