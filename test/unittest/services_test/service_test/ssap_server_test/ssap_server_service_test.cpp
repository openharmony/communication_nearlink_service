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
#include <thread>

#include "ssap_server_service.cpp"
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

class NearlinkSsapServerServiceTest : public testing::Test {
public:
    NearlinkSsapServerServiceTest() {}
    ~NearlinkSsapServerServiceTest() {}

    static void SetUpTestCase(void)
    {
        HILOGI("SetUpTestCase NearlinkSsapServerServiceTest.");
        NearlinkAccessTokenMock::SetNativeTokenInfo();
        SleInterfaceManager::GetInstance()->Start();
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_1000_MS));
        HILOGI("SetUpTestCase NearlinkSsapServerServiceTest end");
    }
    static void TearDownTestCase(void)
    {
        HILOGI("TearDownTestCase NearlinkSsapServerServiceTest");
    }
    void SetUp();
    void TearDown();

    std::shared_ptr<SsapServerService> serverService_ {nullptr};
    int appId_ = -1;
    std::shared_ptr<SsapServerCallbackMock> callback_ {nullptr};
    RawAddress remoteAddr_ = RawAddress("11:22:33:44:55:66");
};

void NearlinkSsapServerServiceTest::SetUp()
{ 
    serverService_ = std::make_shared<SsapServerService>();
    EXPECT_NE(serverService_, nullptr);
    if (serverService_ == nullptr) {
        HILOGE("NearlinkSsapServerServiceTest serverService_ is nullptr.");
        return;
    }
    serverService_->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    callback_ = std::make_shared<SsapServerCallbackMock>();
    appId_ = serverService_->RegisterApplication(callback_);
    EXPECT_NE(appId_, -1);
}

void NearlinkSsapServerServiceTest::TearDown()
{
    HILOGI("TearDown NearlinkSsapServerServiceTest.");
    if (serverService_ != nullptr) {
        HILOGI("NearlinkSsapServerServiceTest disable serverService_.");
        serverService_->Disable();
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
        serverService_ = nullptr;
    }
    appId_ = -1;
    callback_ = nullptr;
}

/**
 * @tc.name: NearlinkSsapServerServiceTest_001
 * @tc.desc: Test SetMtu.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerServiceTest_001, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServiceTest_001 start");
    uint16_t mtu = 512; // MTU 512 bytes
    int ret = serverService_->SetMtu(appId_, mtu);
    EXPECT_EQ(ret, 0);
    HILOGI("NearlinkSsapServerServiceTest_001 end");
}

/**
 * @tc.name: NearlinkSsapServerServiceTest_002
 * @tc.desc: Test AddService and RemoveService.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerServiceTest_002, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServiceTest_002 start");
    Service service {};
    int ret = serverService_->AddService(appId_, service);
    EXPECT_EQ(ret, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    ret = serverService_->RemoveService(appId_, service);
    EXPECT_EQ(ret, 0);
    HILOGI("NearlinkSsapServerServiceTest_002 end");
}

/**
 * @tc.name: NearlinkSsapServerServiceTest_003
 * @tc.desc: Test ClearServices.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerServiceTest_003, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServiceTest_003 start");
    Service service {};
    int ret = serverService_->AddService(appId_, service);
    EXPECT_EQ(ret, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    ret = serverService_->ClearServices(appId_);
    EXPECT_EQ(ret, 0);
    HILOGI("NearlinkSsapServerServiceTest_003 end");
}

/**
 * @tc.name: NearlinkSsapServerServiceTest_004
 * @tc.desc: Test CheckServiceExistByUuid.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerServiceTest_004, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServiceTest_004 start");
    Property property {};
    property.handle_ = 1;
    Uuid propertyUuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000004321");
    property.uuid_ = propertyUuid;
    property.value_ = {4, 3, 2, 1};
    Service service {};
    service.uuid_ = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000001234");
    service.properties_.push_back(property);
    int ret = serverService_->AddService(appId_, service);
    EXPECT_EQ(ret, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));

    serverService_->pimpl->adapterCallback_.OnAddService(appId_, service, 0);
    serverService_->pimpl->adapterCallback_.OnAddService(appId_ + 100, service, 0);
    bool isUuidExist = serverService_->CheckServiceExistByUuid(appId_, service.uuid_);
    EXPECT_EQ(isUuidExist, true);
    Uuid anotherUuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000005678");
    isUuidExist = serverService_->CheckServiceExistByUuid(appId_, anotherUuid);
    EXPECT_EQ(isUuidExist, false);
    HILOGI("NearlinkSsapServerServiceTest_004 end");
}

/**
 * @tc.name: NearlinkSsapServerServiceTest_005
 * @tc.desc: Test SetPropertyValue and SetDescriptorValue.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerServiceTest_005, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServiceTest_005 start");
    Property property {};
    property.handle_ = 1;
    Uuid propertyUuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000004321");
    property.uuid_ = propertyUuid;
    property.value_ = {4, 3, 2, 1};
    Descriptor descriptor {};
    descriptor.handle_ = 1;
    descriptor.writeInd_ = SSAP_DESC_WRITE_IND_OK;
    descriptor.value_ = {8, 4, 2, 1};
    Service service {};
    service.uuid_ = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000001234");
    service.properties_.push_back(property);
    service.descriptors_.push_back(descriptor);
    int ret = serverService_->AddService(appId_, service);
    EXPECT_EQ(ret, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));

    property.value_ = {8, 7, 6, 5};
    ret = serverService_->SetPropertyValue(appId_, property);
    EXPECT_EQ(ret, 0);

    descriptor.value_ = {9, 6, 3, 1};
    ret = serverService_->SetDescriptorValue(appId_, descriptor);
    EXPECT_EQ(ret, 0);

    property.uuid_ = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000008765");
    std::vector<uint8_t> propertyValueResult = serverService_->GetPropertyValue(appId_, property);
    EXPECT_EQ(propertyValueResult.size(), 0);
    descriptor.handle_ = 0;
    std::vector<uint8_t> descriptorValueResult = serverService_->GetDescriptorValue(appId_, descriptor);
    EXPECT_EQ(descriptorValueResult.size(), 0);
    HILOGI("NearlinkSsapServerServiceTest_005 end");
}

/**
 * @tc.name: NearlinkSsapServerServiceTest_006
 * @tc.desc: Test AuthorizeResponse.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerServiceTest_006, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServiceTest_006 start");
    int ret = serverService_->AuthorizeResponse(appId_, 0, true);
    EXPECT_EQ(ret, 0);
    HILOGI("NearlinkSsapServerServiceTest_006 end");
}

/**
 * @tc.name: NearlinkSsapServerServiceTest_007
 * @tc.desc: Test OnConnectionStateChanged.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerServiceTest_007, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServiceTest_007 start");
    EXPECT_CALL(*callback_, OnConnectionStateChanged);
    serverService_->pimpl->adapterCallback_.OnConnectionStateChanged(appId_, remoteAddr_, 0, 0, 0);
    serverService_->pimpl->adapterCallback_.OnConnectionStateChanged(appId_ + 100, remoteAddr_, 0, 0, 0);
    HILOGI("NearlinkSsapServerServiceTest_007 end");
}

/**
 * @tc.name: NearlinkSsapServerServiceTest_008
 * @tc.desc: Test OnMtuChanged.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerServiceTest_008, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServiceTest_008 start");
    uint16_t mtu = 512;
    EXPECT_CALL(*callback_, OnMtuChanged);
    serverService_->pimpl->adapterCallback_.OnMtuChanged(appId_, remoteAddr_, mtu);
    serverService_->pimpl->adapterCallback_.OnMtuChanged(appId_ + 100, remoteAddr_, mtu);
    HILOGI("NearlinkSsapServerServiceTest_008 end");
}

/**
 * @tc.name: NearlinkSsapServerServiceTest_009
 * @tc.desc: Test callbacks about property and descriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerServiceTest_009, TestSize.Level1)
{
    HILOGI("NearlinkSsapServerServiceTest_009 start");
    Property property {};
    property.handle_ = 1;
    Uuid propertyUuid = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000004321");
    property.uuid_ = propertyUuid;
    property.value_ = {4, 3, 2, 1};
    Descriptor descriptor {};
    descriptor.handle_ = 1;
    descriptor.writeInd_ = SSAP_DESC_WRITE_IND_OK;
    descriptor.value_ = {8, 4, 2, 1};
    Service service {};
    service.uuid_ = Uuid::ConvertFromString("37BEA880-FC70-11EA-B720-000000001234");
    service.properties_.push_back(property);
    service.descriptors_.push_back(descriptor);
    int ret = serverService_->AddService(appId_, service);
    EXPECT_EQ(ret, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    property.value_ = {8, 7, 6, 5};
    descriptor.value_ = {9, 6, 3, 1};

    {
        InSequence s;
        EXPECT_CALL(*callback_, OnSetPropertyValue);
        EXPECT_CALL(*callback_, OnReadProperty);
        EXPECT_CALL(*callback_, OnWriteProperty);
        EXPECT_CALL(*callback_, OnNotifyProperty);
        EXPECT_CALL(*callback_, OnReadPropertyAuthorizeRequest);
        EXPECT_CALL(*callback_, OnWritePropertyAuthorizeRequest);

        EXPECT_CALL(*callback_, OnSetDescriptorValue);
        EXPECT_CALL(*callback_, OnReadDescriptor);
        EXPECT_CALL(*callback_, OnWriteDescriptor);
        EXPECT_CALL(*callback_, OnReadDescriptorAuthorizeRequest);
        EXPECT_CALL(*callback_, OnWriteDescriptorAuthorizeRequest);
    }
    serverService_->pimpl->adapterCallback_.OnSetPropertyValue(appId_, property, 0);
    serverService_->pimpl->adapterCallback_.OnReadProperty(appId_, remoteAddr_, property, 0);
    serverService_->pimpl->adapterCallback_.OnWriteProperty(appId_, remoteAddr_, property, 0);
    serverService_->pimpl->adapterCallback_.OnNotifyProperty(appId_, remoteAddr_, property, 0);
    serverService_->pimpl->adapterCallback_.OnReadPropertyAuthorizeRequest(appId_, remoteAddr_, 0, property);
    serverService_->pimpl->adapterCallback_.OnWritePropertyAuthorizeRequest(appId_, remoteAddr_, 0, property);

    serverService_->pimpl->adapterCallback_.OnSetDescriptorValue(appId_, descriptor, 0);
    serverService_->pimpl->adapterCallback_.OnReadDescriptor(appId_, remoteAddr_, descriptor, 0);
    serverService_->pimpl->adapterCallback_.OnWriteDescriptor(appId_, remoteAddr_, descriptor, 0);
    serverService_->pimpl->adapterCallback_.OnReadDescriptorAuthorizeRequest(appId_, remoteAddr_, 0, descriptor);
    serverService_->pimpl->adapterCallback_.OnWriteDescriptorAuthorizeRequest(appId_, remoteAddr_, 0, descriptor);

    serverService_->pimpl->adapterCallback_.OnSetPropertyValue(appId_ + 100, property, 0);
    serverService_->pimpl->adapterCallback_.OnReadProperty(appId_ + 100, remoteAddr_, property, 0);
    serverService_->pimpl->adapterCallback_.OnWriteProperty(appId_ + 100, remoteAddr_, property, 0);
    serverService_->pimpl->adapterCallback_.OnNotifyProperty(appId_ + 100, remoteAddr_, property, 0);
    serverService_->pimpl->adapterCallback_.OnReadPropertyAuthorizeRequest(appId_ + 100, remoteAddr_, 0, property);
    serverService_->pimpl->adapterCallback_.OnWritePropertyAuthorizeRequest(appId_ + 100, remoteAddr_, 0, property);

    serverService_->pimpl->adapterCallback_.OnSetDescriptorValue(appId_ + 100, descriptor, 0);
    serverService_->pimpl->adapterCallback_.OnReadDescriptor(appId_ + 100, remoteAddr_, descriptor, 0);
    serverService_->pimpl->adapterCallback_.OnWriteDescriptor(appId_ + 100, remoteAddr_, descriptor, 0);
    serverService_->pimpl->adapterCallback_.OnReadDescriptorAuthorizeRequest(appId_ + 100, remoteAddr_, 0, descriptor);
    serverService_->pimpl->adapterCallback_.OnWriteDescriptorAuthorizeRequest(appId_ + 100, remoteAddr_, 0, descriptor);
    HILOGI("NearlinkSsapServerServiceTest_009 end");
}

/**
 * @tc.name: NearlinkSsapServerStackAdapter_001
 * @tc.desc: Test callbacks about property and descriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerStackAdapter_001, TestSize.Level1)
{
    HILOGI("OnSetPropertyValue001 start");
    NLSTK_SsapServerOnSetPropertyParam_S param {};
    NLSTK_SsapUuid_S uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
    NLSTK_VariableData_S nlstkValue = {
    .len = 10,
    .data = static_cast<uint8_t*>(malloc(10 * sizeof(uint8_t)))
    };
    // 初始化数据
    for (uint16_t i = 0; i < nlstkValue.len; ++i) {
        nlstkValue.data[i] = static_cast<uint8_t>(i); // 初始化为0到9
    }
    NLSTK_VariableData_S lenVal = {0};
    param.handle = 1;
    param.uuid = uuid1;
    param.value = nlstkValue;
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    serverService_->pimpl->stackAdapter_.OnSetPropertyValue(appId_, &param, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    EXPECT_NE(0, 1);
    HILOGI("OnSetPropertyValue001 end");
}

/**
 * @tc.name: NearlinkSsapServerStackAdapter_002
 * @tc.desc: Test callbacks about property and descriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerStackAdapter_002, TestSize.Level1)
{
    HILOGI("OnSetDescriptorValue001 start");
    NLSTK_SsapServerOnSetDescriptorParam_S param {};
    NLSTK_SsapUuid_S uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
    NLSTK_VariableData_S nlstkValue = {
    .len = 10,
    .data = static_cast<uint8_t*>(malloc(10 * sizeof(uint8_t)))
    };
    // 初始化数据
    for (uint16_t i = 0; i < nlstkValue.len; ++i) {
        nlstkValue.data[i] = static_cast<uint8_t>(i); // 初始化为0到9
    }
    param.handle = 1;
    param.uuid = uuid1;
    param.type = 0;
    param.value = nlstkValue;
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    serverService_->pimpl->stackAdapter_.OnSetDescriptorValue(appId_, &param, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    EXPECT_NE(0, 1);
    HILOGI("OnSetDescriptorValue001 end");
}

/**
 * @tc.name: NearlinkSsapServerStackAdapter_003
 * @tc.desc: Test callbacks about property and descriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerStackAdapter_003, TestSize.Level1)
{
    HILOGI("OnReadPropertyAuthorizeRequest001 start");
    uint16_t requestId = 0;
    NLSTK_SsapServerReadPropertyInfo_S param {};
    NLSTK_SsapUuid_S uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
    param.addr = {1, {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
    param.uuid = uuid1;
    param.handle = 1;
    serverService_->pimpl->stackAdapter_.OnReadPropertyAuthorizeRequest(appId_, requestId, &param);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    EXPECT_NE(0, 1);
    HILOGI("OnReadPropertyAuthorizeRequest001 end");
}

/**
 * @tc.name: NearlinkSsapServerStackAdapter_004
 * @tc.desc: Test callbacks about property and descriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerStackAdapter_004, TestSize.Level1)
{
    HILOGI("OnReadDescriptorAuthorizeRequest001 start");
    uint16_t requestId = 0;
    NLSTK_SsapServerReadDescriptorInfo_S param {};
    NLSTK_SsapUuid_S uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
    param.addr = {1, {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
    param.handle = 1;
    param.uuid = uuid1;
    param.type = 0;
    serverService_->pimpl->stackAdapter_.OnReadDescriptorAuthorizeRequest(appId_, requestId, &param);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    EXPECT_NE(0, 1);
    HILOGI("OnReadDescriptorAuthorizeRequest001 end");
}

/**
 * @tc.name: NearlinkSsapServerStackAdapter_005
 * @tc.desc: Test callbacks about property and descriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerStackAdapter_005, TestSize.Level1)
{
    HILOGI("OnWritePropertyAuthorizeRequest001 start");
    uint16_t requestId = 0;
    NLSTK_SsapServerWritePropertyInfo_S param {};
    NLSTK_SsapUuid_S uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
    NLSTK_VariableData_S nlstkValue = {
    .len = 10,
    .data = static_cast<uint8_t*>(malloc(10 * sizeof(uint8_t)))
    };
    // 初始化数据
    for (uint16_t i = 0; i < nlstkValue.len; ++i) {
        nlstkValue.data[i] = static_cast<uint8_t>(i); // 初始化为0到9
    }
    param.addr = {1, {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
    param.handle = 1;
    param.uuid = uuid1;
    param.value = nlstkValue;
    serverService_->pimpl->stackAdapter_.OnWritePropertyAuthorizeRequest(appId_, requestId, &param);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    EXPECT_NE(0, 1);
    HILOGI("OnWritePropertyAuthorizeRequest001 end");
}

/**
 * @tc.name: NearlinkSsapServerStackAdapter_006
 * @tc.desc: Test callbacks about property and descriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerStackAdapter_006, TestSize.Level1)
{
    HILOGI("OnWriteDescriptorAuthorizeRequest001 start");
    uint16_t requestId = 0;
    NLSTK_SsapServerWriteDescriptorInfo_S param {};
    NLSTK_SsapUuid_S uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
    NLSTK_VariableData_S nlstkValue = {
    .len = 10,
    .data = static_cast<uint8_t*>(malloc(10 * sizeof(uint8_t)))
    };
    // 初始化数据
    for (uint16_t i = 0; i < nlstkValue.len; ++i) {
        nlstkValue.data[i] = static_cast<uint8_t>(i); // 初始化为0到9
    }
    param.addr = {1, {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
    param.handle = 1;
    param.uuid = uuid1;
    param.type = 0;
    param.value = nlstkValue;
    serverService_->pimpl->stackAdapter_.OnWriteDescriptorAuthorizeRequest(appId_, requestId, &param);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    EXPECT_NE(0, 1);
    HILOGI("OnWriteDescriptorAuthorizeRequest001 end");
}

/**
 * @tc.name: NearlinkSsapServerStackAdapter_007
 * @tc.desc: Test callbacks about property and descriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerStackAdapter_007, TestSize.Level1)
{
    HILOGI("OnReadProperty001 start");
    uint16_t requestId = 0;
    NLSTK_SsapServerReadPropertyInfo_S param {};
    NLSTK_SsapUuid_S uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
    param.addr = {1, {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
    param.uuid = uuid1;
    param.handle = 1;
    serverService_->pimpl->stackAdapter_.OnReadProperty(appId_, &param);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    EXPECT_NE(0, 1);
    HILOGI("OnReadProperty001 end");
}

/**
 * @tc.name: NearlinkSsapServerStackAdapter_008
 * @tc.desc: Test callbacks about property and descriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerStackAdapter_008, TestSize.Level1)
{
    HILOGI("OnReadDescriptor001 start");
    uint16_t requestId = 0;
    NLSTK_SsapServerReadDescriptorInfo_S param {};
    NLSTK_SsapUuid_S uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
    param.addr = {1, {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
    param.handle = 1;
    param.uuid = uuid1;
    param.type = 0;
    serverService_->pimpl->stackAdapter_.OnReadDescriptor(appId_, &param);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    EXPECT_NE(0, 1);
    HILOGI("OnReadDescriptor001 end");
}

/**
 * @tc.name: NearlinkSsapServerStackAdapter_009
 * @tc.desc: Test callbacks about property and descriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerStackAdapter_009, TestSize.Level1)
{
    HILOGI("OnWriteProperty001 start");
    uint16_t requestId = 0;
    NLSTK_SsapServerWritePropertyInfo_S param {};
    NLSTK_SsapUuid_S uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
    NLSTK_VariableData_S nlstkValue = {
    .len = 10,
    .data = static_cast<uint8_t*>(malloc(10 * sizeof(uint8_t)))
    };
    // 初始化数据
    for (uint16_t i = 0; i < nlstkValue.len; ++i) {
        nlstkValue.data[i] = static_cast<uint8_t>(i); // 初始化为0到9
    }
    param.addr = {1, {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
    param.handle = 1;
    param.uuid = uuid1;
    param.value = nlstkValue;
    serverService_->pimpl->stackAdapter_.OnWriteProperty(appId_, &param);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    EXPECT_NE(0, 1);
    HILOGI("OnWriteProperty001 end");
}

/**
 * @tc.name: NearlinkSsapServerStackAdapter_010
 * @tc.desc: Test callbacks about property and descriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerStackAdapter_010, TestSize.Level1)
{
    HILOGI("OnWriteDescriptor001 start");
    uint16_t requestId = 0;
    NLSTK_SsapServerWriteDescriptorInfo_S param {};
    NLSTK_SsapUuid_S uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
    NLSTK_VariableData_S nlstkValue = {
    .len = 10,
    .data = static_cast<uint8_t*>(malloc(10 * sizeof(uint8_t)))
    };
    // 初始化数据
    for (uint16_t i = 0; i < nlstkValue.len; ++i) {
        nlstkValue.data[i] = static_cast<uint8_t>(i); // 初始化为0到9
    }
    param.addr = {1, {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
    param.handle = 1;
    param.uuid = uuid1;
    param.type = 0;
    param.value = nlstkValue;
    serverService_->pimpl->stackAdapter_.OnWriteDescriptor(appId_, &param);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    EXPECT_NE(0, 1);
    HILOGI("OnWriteDescriptor001 end");
}

/**
 * @tc.name: NearlinkSsapServerStackAdapter_011
 * @tc.desc: Test callbacks about property and descriptor.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSsapServerServiceTest, NearlinkSsapServerStackAdapter_011, TestSize.Level1)
{
    HILOGI("OnNotifyProperty001 start");
    NLSTK_SsapServerOnNotifyPropertyParam_S param {};
    NLSTK_SsapUuid_S uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
    NLSTK_VariableData_S nlstkValue = {
    .len = 10,
    .data = static_cast<uint8_t*>(malloc(10 * sizeof(uint8_t)))
    };
    // 初始化数据
    for (uint16_t i = 0; i < nlstkValue.len; ++i) {
        nlstkValue.data[i] = static_cast<uint8_t>(i); // 初始化为0到9
    }
    param.addr = {1, {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
    param.handle = 1;
    param.uuid = uuid1;
    param.value = nlstkValue;
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    serverService_->pimpl->stackAdapter_.OnNotifyProperty(appId_, &param, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    EXPECT_NE(0, 1);
    HILOGI("OnNotifyProperty001 end");
}

} // namespace Nearlink
} // namespace OHOS