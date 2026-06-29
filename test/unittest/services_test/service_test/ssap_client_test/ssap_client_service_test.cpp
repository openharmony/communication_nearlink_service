/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "ssap_client_service.cpp"
#include "ssap_server_callback_mock.h"
#include "ProfileServiceManager.h"
#include "nearlink_errorcode.h"
#include "nearlink_access_token_mock.h"
#include "SleServiceManager.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr int DELAY_1000_MS = 1000;
constexpr int DELAY_200_MS = 200;
}
using namespace testing;
using namespace testing::ext;

class SsapClientCallbackMock : public InterfaceSsapClientCallback {
public:
    MOCK_METHOD(void, OnMtuChanged, (uint16_t mtu, int ret), (override));
    MOCK_METHOD(void, OnDiscoverComplete, (int ret), (override));
    MOCK_METHOD(void, OnDiscoverByUuidComplete, (const Uuid &uuid, int ret), (override));
    MOCK_METHOD(void, OnReadProperty, (Property &property, int ret), (override));
    MOCK_METHOD(void, OnCallMethod, (Method &method, int ret), (override));
    MOCK_METHOD(void, OnReadDescriptor, (Descriptor &descriptor, int ret), (override));
    MOCK_METHOD(void, OnReadPropertiesByUuid, (std::list<Property> &list, int ret), (override));
    MOCK_METHOD(void, OnReadDescriptorsByUuid, (std::list<Descriptor> &list, int ret), (override));
    MOCK_METHOD(void, OnWriteProperty, (Property &property, int ret), (override));
    MOCK_METHOD(void, OnWriteDescriptor, (Descriptor &descriptor, int ret), (override));
    MOCK_METHOD(void, OnGetPropertyNotification, (const Property &property, bool enable, int ret), (override));
    MOCK_METHOD(void, OnGetPropertyIndication, (const Property &property, bool enable, int ret), (override));
    MOCK_METHOD(void, OnSetPropertyNotification, (const Property &property, bool enable, int ret), (override));
    MOCK_METHOD(void, OnSetEventNotification, (const Event &event, bool enable, int ret), (override));
    MOCK_METHOD(void, OnSetPropertyIndication, (const Property &property, bool enable, int ret), (override));
    MOCK_METHOD(void, OnSetEventIndication, (const Event &event, bool enable, int ret), (override));
    MOCK_METHOD(void, OnPropertyChanged, (const Property &property), (override));
    MOCK_METHOD(void, OnEvent, (const Event &event), (override));
    MOCK_METHOD(void, OnConnectionStateChanged, (uint8_t state, int ret), (override));
    ~SsapClientCallbackMock() override = default;
};

class NearlinkSsapClientServiceTest : public testing::Test {
public:
    NearlinkSsapClientServiceTest() {}
    ~NearlinkSsapClientServiceTest() {}

    static void SetUpTestCase(void)
    {
        HILOGI("SetUpTestCase NearlinkSsapClientServiceTest.");
        NearlinkAccessTokenMock::SetNativeTokenInfo();
        SleInterfaceManager::GetInstance()->Start();

        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_1000_MS));
        HILOGI("SetUpTestCase NearlinkSsapClientServiceTest end");
    }
    static void TearDownTestCase(void)
    {
        HILOGI("TearDownTestCase NearlinkSsapClientServiceTest");
    }
    void SetUp();
    void TearDown();

    std::shared_ptr<SsapClientService> clientService_ {nullptr};
    int appId_ = -1;
    std::shared_ptr<SsapClientCallbackMock> callback_ {nullptr};
    RawAddress remoteAddr_ = RawAddress("11:22:33:44:55:66");
};

void NearlinkSsapClientServiceTest::SetUp()
{ 
    clientService_ = std::make_shared<SsapClientService>();
    EXPECT_NE(clientService_, nullptr);
    if (clientService_ == nullptr) {
        HILOGE("NearlinkSsapClientServiceTest clientService_ is nullptr.");
        return;
    }
    clientService_->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    callback_ = std::make_shared<SsapClientCallbackMock>();
    appId_ = clientService_->RegisterApplication(callback_, remoteAddr_, SSAP_TRANSPORT_SLE, SSAP_SEC_NONE);
    EXPECT_NE(appId_, -1);
}

void NearlinkSsapClientServiceTest::TearDown()
{
    HILOGI("TearDown NearlinkSsapClientServiceTest.");
    if (clientService_ != nullptr) {
        HILOGI("NearlinkSsapClientServiceTest disable clientService_.");
        clientService_->Disable();
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
        clientService_ = nullptr;
    }
    appId_ = -1;
    callback_ = nullptr;
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_001
 * @tc.desc: Test ExchangeMtu.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_001, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_001 start");
    uint16_t mtu = 512; // MTU 512 bytes
    int ret = clientService_->ExchangeMtu(appId_, mtu);
    clientService_->pimpl->stackAdapter_.ExchangeMtu(appId_, mtu);
    EXPECT_EQ(ret, 0);
    HILOGI("NearlinkSsapClientServiceTest_001 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_002
 * @tc.desc: Test DiscoverServices and DiscoverServicesByUuid.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_002, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_002 start");
    int ret = clientService_->DiscoverServices(appId_);
    EXPECT_EQ(ret, 0);
    Uuid uuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000001234");
    uint16_t startHandle = MIN_ENTRY_HANDLE;
    uint16_t endHandle = MAX_RESRV_HANDLE;
    ret = clientService_->DiscoverServicesByUuid(appId_, uuid, startHandle, endHandle);
    EXPECT_EQ(ret, 0);
    HILOGI("NearlinkSsapClientServiceTest_002 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_003
 * @tc.desc: Test GetServices and GetServicesByUuid.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_003, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_003 start");
    std::list<Service> services;
    services = clientService_->GetServices(appId_);
    EXPECT_TRUE(services.empty());
    services = clientService_->pimpl->stackAdapter_.GetServices(appId_);
    EXPECT_TRUE(services.empty());
    Uuid uuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000001234");
    services = clientService_->GetServicesByUuid(appId_, uuid);
    EXPECT_TRUE(services.empty());
    services = clientService_->pimpl->stackAdapter_.GetServicesByUuid(appId_, uuid);
    EXPECT_TRUE(services.empty());
    HILOGI("NearlinkSsapClientServiceTest_003 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_004
 * @tc.desc: Test ReadProperty and ReadPropertiesByUuid.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_004, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_004 start");
    Property property {};
    property.handle_ = 1;
    Uuid propertyUuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000004321");
    property.uuid_ = propertyUuid;
    property.value_ = {4, 3, 2, 1};

    int ret = clientService_->ReadProperty(appId_, property);
    clientService_->pimpl->stackAdapter_.ReadProperty(appId_, property);
    EXPECT_EQ(ret, 0);
    Uuid uuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000001234");
    uint16_t startHandle = MIN_ENTRY_HANDLE;
    uint16_t endHandle = MAX_RESRV_HANDLE;
    ret = clientService_->ReadPropertiesByUuid(appId_, uuid, startHandle, endHandle);
    clientService_->pimpl->stackAdapter_.ReadPropertiesByUuid(appId_, uuid, startHandle, endHandle);
    EXPECT_EQ(ret, 0);
    HILOGI("NearlinkSsapClientServiceTest_004 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_005
 * @tc.desc: Test ReadDescriptor and ReadDescriptorsByUuid.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_005, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_005 start");
    Descriptor descriptor {};
    descriptor.handle_ = 1;
    descriptor.writeInd_ = SSAP_DESC_WRITE_IND_OK;
    descriptor.value_ = {8, 4, 2, 1};
    int ret = clientService_->ReadDescriptor(appId_, descriptor);
    clientService_->pimpl->stackAdapter_.ReadDescriptor(appId_, descriptor);
    EXPECT_EQ(ret, 0);
    Uuid uuid {};
    ret = clientService_->ReadDescriptorsByUuid(appId_, uuid, 0, 0, 0);
    EXPECT_EQ(ret, static_cast<int>(SsapStatus::SSAP_NOT_SUPPORT));
    HILOGI("NearlinkSsapClientServiceTest_005 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_006
 * @tc.desc: Test WriteProperty.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_006, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_006 start");
    Property property {};
    property.handle_ = 1;
    Uuid propertyUuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000004321");
    property.uuid_ = propertyUuid;
    property.value_ = {4, 3, 2, 1};

    int ret = clientService_->WriteProperty(appId_, property, true);
    clientService_->pimpl->stackAdapter_.WriteProperty(appId_, property, true);
    EXPECT_EQ(ret, 0);
    HILOGI("NearlinkSsapClientServiceTest_006 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_007
 * @tc.desc: Test WriteDescriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_007, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_007 start");
    Descriptor descriptor {};
    descriptor.handle_ = 1;
    descriptor.writeInd_ = SSAP_DESC_WRITE_IND_OK;
    descriptor.value_ = {8, 4, 2, 1};
    int ret = clientService_->WriteDescriptor(appId_, descriptor, true);
    clientService_->pimpl->stackAdapter_.WriteDescriptor(appId_, descriptor, true);
    EXPECT_EQ(ret, 0);
    HILOGI("NearlinkSsapClientServiceTest_007 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_008
 * @tc.desc: Test GetPropertyNotification.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_008, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_008 start");
    Property property {};
    property.handle_ = 1;
    Uuid propertyUuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000004321");
    property.uuid_ = propertyUuid;
    property.value_ = {4, 3, 2, 1};
    int ret = clientService_->GetPropertyNotification(appId_, property);
    clientService_->pimpl->stackAdapter_.GetPropertyNotification(appId_, property);
    EXPECT_EQ(ret, 0);
    HILOGI("NearlinkSsapClientServiceTest_008 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_009
 * @tc.desc: Test SetPropertyNotification.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_009, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_009 start");
    Property property {};
    property.handle_ = 1;
    Uuid propertyUuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000004321");
    property.uuid_ = propertyUuid;
    property.value_ = {4, 3, 2, 1};
    int ret = clientService_->SetPropertyNotification(appId_, property, true);
    clientService_->pimpl->stackAdapter_.SetPropertyNotification(appId_, property, true);
    EXPECT_EQ(ret, 0);
    HILOGI("NearlinkSsapClientServiceTest_009 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_010
 * @tc.desc: Test SetEventIndication.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_010, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_010 start");
    Event event {};
    int ret = clientService_->SetEventIndication(appId_, event, true);
    EXPECT_EQ(ret, static_cast<int>(SsapStatus::SSAP_NOT_SUPPORT));
    HILOGI("NearlinkSsapClientServiceTest_010 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_011
 * @tc.desc: Test SetEventIndication.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_011, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_011 start");
    Event event {};
    int ret = clientService_->SetEventNotification(appId_, event, true);
    EXPECT_EQ(ret, static_cast<int>(SsapStatus::SSAP_NOT_SUPPORT));
    HILOGI("NearlinkSsapClientServiceTest_011 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_012
 * @tc.desc: Test CallMethod.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_012, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_012 start");
    Method method {};
    method.parameter_ = std::vector<uint8_t>{4, 1}; // initialize parameter_
    int ret = clientService_->CallMethod(appId_, method, true);
    clientService_->pimpl->stackAdapter_.CallMethod(appId_, method, true);
    EXPECT_EQ(ret, 0);
    HILOGI("NearlinkSsapClientServiceTest_012 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_013
 * @tc.desc: Test SetPropertyIndication.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_013, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_013 start");
    Property property {};
    property.handle_ = 1;
    Uuid propertyUuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000004321");
    property.uuid_ = propertyUuid;
    property.value_ = {4, 3, 2, 1};
    int ret = clientService_->SetPropertyIndication(appId_, property, true);
    clientService_->pimpl->stackAdapter_.SetPropertyIndication(appId_, property, true);
    EXPECT_EQ(ret, 0);
    HILOGI("NearlinkSsapClientServiceTest_013 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_014
 * @tc.desc: Test Connect and Disconnect.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_014, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_014 start");
    int ret = clientService_->Connect(appId_, true);
    RawAddress addr;
    clientService_->pimpl->stackAdapter_.Connect(appId_, addr, true);
    EXPECT_EQ(ret, 0);

    ret = clientService_->Disconnect(appId_);
    clientService_->pimpl->stackAdapter_.Disconnect(appId_, addr);
    HILOGI("NearlinkSsapClientServiceTest_014 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_016
 * @tc.desc: Test OnMtuChanged.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_016, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_016 start");
    EXPECT_CALL(*callback_, OnMtuChanged).Times(AtLeast(1));
    uint16_t mtu = 512; // mtu 512 bytes
    clientService_->pimpl->adapterCallback_.OnMtuChanged(appId_, mtu, 0);
    clientService_->pimpl->adapterCallback_.OnMtuChanged(appId_ + 100, mtu, 0);
    SsapClientStackAdapter::OnMtuChanged(appId_, mtu, NLSTK_Errcode_E::NLSTK_ERRCODE_SUCCESS);
    HILOGI("NearlinkSsapClientServiceTest_016 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_017
 * @tc.desc: Test OnDiscoverComplete.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_017, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_017 start");
    EXPECT_CALL(*callback_, OnDiscoverComplete).Times(AtLeast(1));
    clientService_->pimpl->adapterCallback_.OnDiscoverComplete(appId_, 0);
    clientService_->pimpl->adapterCallback_.OnDiscoverComplete(appId_ + 100, 0);
    SsapClientStackAdapter::OnDiscoverComplete(appId_, NLSTK_Errcode_E::NLSTK_ERRCODE_SUCCESS);
    HILOGI("NearlinkSsapClientServiceTest_017 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_018
 * @tc.desc: Test OnDiscoverByUuidComplete.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_018, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_018 start");
    EXPECT_CALL(*callback_, OnDiscoverByUuidComplete).Times(AtLeast(1));
    Uuid uuid {};
    NLSTK_SsapUuid_S ssapUuid {};
    clientService_->pimpl->adapterCallback_.OnDiscoverByUuidComplete(appId_, uuid, 0);
    clientService_->pimpl->adapterCallback_.OnDiscoverByUuidComplete(appId_ + 100, uuid, 0);
    SsapClientStackAdapter::OnDiscoverByUuidComplete(appId_, &ssapUuid, NLSTK_Errcode_E::NLSTK_ERRCODE_SUCCESS);
    HILOGI("NearlinkSsapClientServiceTest_018 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_019
 * @tc.desc: Test OnReadProperty.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_019, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_019 start");
    EXPECT_CALL(*callback_, OnReadProperty).Times(AtLeast(1));
    Property property {};
    NLSTK_SsapClientReadPropertyInfo_S propertyInfo {};
    clientService_->pimpl->adapterCallback_.OnReadProperty(appId_, property, 0);
    clientService_->pimpl->adapterCallback_.OnReadProperty(appId_ + 100, property, 0);
    SsapClientStackAdapter::OnReadProperty(appId_, &propertyInfo, NLSTK_Errcode_E::NLSTK_ERRCODE_SUCCESS);
    HILOGI("NearlinkSsapClientServiceTest_019 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_020
 * @tc.desc: Test OnCallMethod.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_020, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_020 start");
    EXPECT_CALL(*callback_, OnCallMethod).Times(AtLeast(1));
    Method method {};
    NLSTK_SsapClientCallMethodResult_S methodResult {};
    clientService_->pimpl->adapterCallback_.OnCallMethod(appId_, method, 0);
    clientService_->pimpl->adapterCallback_.OnCallMethod(appId_ + 100, method, 0);
    SsapClientStackAdapter::OnCallMethod(appId_, &methodResult, NLSTK_Errcode_E::NLSTK_ERRCODE_SUCCESS);
    HILOGI("NearlinkSsapClientServiceTest_020 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_021
 * @tc.desc: Test OnReadDescriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_021, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_021 start");
    EXPECT_CALL(*callback_, OnReadDescriptor).Times(AtLeast(1));
    Descriptor descriptor {};
    NLSTK_SsapClientReadDescriptorInfo_S descriptorInfo {};
    clientService_->pimpl->adapterCallback_.OnReadDescriptor(appId_, descriptor, 0);
    clientService_->pimpl->adapterCallback_.OnReadDescriptor(appId_ + 100, descriptor, 0);
    SsapClientStackAdapter::OnReadDescriptor(appId_, &descriptorInfo, NLSTK_Errcode_E::NLSTK_ERRCODE_SUCCESS);
    HILOGI("NearlinkSsapClientServiceTest_021 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_022
 * @tc.desc: Test OnReadPropertiesByUuid.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_022, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_022 start");
    EXPECT_CALL(*callback_, OnReadPropertiesByUuid).Times(AtLeast(1));
    std::list<Property> properties {};
    NLSTK_SsapClientReadPropertyInfo_S propertyInfo {};
    clientService_->pimpl->adapterCallback_.OnReadPropertiesByUuid(appId_, properties, 0);
    clientService_->pimpl->adapterCallback_.OnReadPropertiesByUuid(appId_ + 100, properties, 0);
    SsapClientStackAdapter::OnReadPropertiesByUuid(appId_, &propertyInfo,
        1, NLSTK_Errcode_E::NLSTK_ERRCODE_SUCCESS);
    HILOGI("NearlinkSsapClientServiceTest_022 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_023
 * @tc.desc: Test OnWriteProperty.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_023, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_023 start");
    EXPECT_CALL(*callback_, OnWriteProperty).Times(AtLeast(1));
    Property property {};
    NLSTK_SsapClientReadPropertyInfo_S propertyInfo {};
    clientService_->pimpl->adapterCallback_.OnWriteProperty(appId_, property, 0);
    clientService_->pimpl->adapterCallback_.OnWriteProperty(appId_ + 100, property, 0);
    SsapClientStackAdapter::OnWriteProperty(appId_, &propertyInfo, NLSTK_Errcode_E::NLSTK_ERRCODE_SUCCESS);
    HILOGI("NearlinkSsapClientServiceTest_023 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_024
 * @tc.desc: Test OnWriteDescriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_024, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_024 start");
    EXPECT_CALL(*callback_, OnWriteDescriptor).Times(AtLeast(1));
    Descriptor descriptor {};
    NLSTK_SsapClientReadDescriptorInfo_S descriptorInfo {};
    clientService_->pimpl->adapterCallback_.OnWriteDescriptor(appId_, descriptor, 0);
    clientService_->pimpl->adapterCallback_.OnWriteDescriptor(appId_ + 100, descriptor, 0);
    SsapClientStackAdapter::OnWriteDescriptor(appId_, &descriptorInfo, NLSTK_Errcode_E::NLSTK_ERRCODE_SUCCESS);
    HILOGI("NearlinkSsapClientServiceTest_024 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_025
 * @tc.desc: Test OnGetPropertyNotification.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_025, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_025 start");
    EXPECT_CALL(*callback_, OnGetPropertyNotification).Times(AtLeast(1));
    Property property {};
    NLSTK_SsapUuid_S stackUuid {};
    clientService_->pimpl->adapterCallback_.OnGetPropertyNotification(appId_, property, true, 0);
    clientService_->pimpl->adapterCallback_.OnGetPropertyNotification(appId_ + 100, property, true, 0);
    SsapClientStackAdapter::OnGetPropertyNotification(appId_, &stackUuid, 0,
        true, NLSTK_Errcode_E::NLSTK_ERRCODE_SUCCESS);
    HILOGI("NearlinkSsapClientServiceTest_025 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_026
 * @tc.desc: Test OnGetPropertyIndication.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_026, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_026 start");
    EXPECT_CALL(*callback_, OnGetPropertyIndication).Times(AtLeast(1));
    Property property {};
    NLSTK_SsapUuid_S stackUuid {};
    clientService_->pimpl->adapterCallback_.OnGetPropertyIndication(appId_, property, true, 0);
    clientService_->pimpl->adapterCallback_.OnGetPropertyIndication(appId_ + 100, property, true, 0);
    SsapClientStackAdapter::OnGetPropertyIndication(appId_, &stackUuid, 0,
        true, NLSTK_Errcode_E::NLSTK_ERRCODE_SUCCESS);
    HILOGI("NearlinkSsapClientServiceTest_026 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_027
 * @tc.desc: Test OnSetPropertyNotification.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_027, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_027 start");
    EXPECT_CALL(*callback_, OnSetPropertyNotification).Times(AtLeast(1));
    Property property {};
    NLSTK_SsapUuid_S stackUuid {};
    clientService_->pimpl->adapterCallback_.OnSetPropertyNotification(appId_, property, true, 0);
    clientService_->pimpl->adapterCallback_.OnSetPropertyNotification(appId_ + 100, property, true, 0);
    SsapClientStackAdapter::OnSetPropertyNotification(appId_, &stackUuid, 0,
        true, NLSTK_Errcode_E::NLSTK_ERRCODE_SUCCESS);
    HILOGI("NearlinkSsapClientServiceTest_027 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_028
 * @tc.desc: Test OnSetPropertyIndication.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_028, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_028 start");
    EXPECT_CALL(*callback_, OnSetPropertyIndication).Times(AtLeast(1));
    Property property {};
    NLSTK_SsapUuid_S stackUuid {};
    clientService_->pimpl->adapterCallback_.OnSetPropertyIndication(appId_, property, true, 0);
    clientService_->pimpl->adapterCallback_.OnSetPropertyIndication(appId_ + 100, property, true, 0);
    SsapClientStackAdapter::OnSetPropertyIndication(appId_, &stackUuid, 0,
        true, NLSTK_Errcode_E::NLSTK_ERRCODE_SUCCESS);
    HILOGI("NearlinkSsapClientServiceTest_028 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_029
 * @tc.desc: Test OnPropertyChanged.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_029, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_029 start");
    EXPECT_CALL(*callback_, OnPropertyChanged).Times(AtLeast(1));
    Property property {};
    NLSTK_SsapClientReadPropertyInfo_S propertyInfo {};
    clientService_->pimpl->adapterCallback_.OnPropertyChanged(appId_, property);
    clientService_->pimpl->adapterCallback_.OnPropertyChanged(appId_ + 100, property);
    SsapClientStackAdapter::OnPropertyChanged(appId_, &propertyInfo);
    HILOGI("NearlinkSsapClientServiceTest_029 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_030
 * @tc.desc: Test OnEvent.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_030, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_030 start");
    EXPECT_CALL(*callback_, OnEvent).Times(AtLeast(1));
    Event event {};
    NLSTK_SsapClientEventInfo_S eventInfo {};
    clientService_->pimpl->adapterCallback_.OnEvent(appId_, event);
    clientService_->pimpl->adapterCallback_.OnEvent(appId_ + 100, event);
    SsapClientStackAdapter::OnEvent(appId_, &eventInfo);
    HILOGI("NearlinkSsapClientServiceTest_030 end");
}

/**
 * @tc.name: NearlinkSsapClientServiceTest_031
 * @tc.desc: Test OnConnectionStateChanged.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapClientServiceTest, NearlinkSsapClientServiceTest_031, TestSize.Level1)
{
    HILOGI("NearlinkSsapClientServiceTest_031 start");
    EXPECT_CALL(*callback_, OnConnectionStateChanged).Times(AtLeast(1));
    clientService_->pimpl->adapterCallback_.OnConnectionStateChanged(appId_, 0, 0, 0);
    SsapClientStackAdapter::OnConnectionStateChanged(appId_, 0, NLSTK_Errcode_E::NLSTK_ERRCODE_SUCCESS, 0);
    HILOGI("NearlinkSsapClientServiceTest_031 end");
}
} // namespace Nearlink
} // namespace OHOS