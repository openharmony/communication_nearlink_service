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
#include "nearlink_sle_advertiser_stub.h"
#include "nearlink_sle_advertiser_callback_proxy.h"
#include "nearlink_sle_advertiser_server.cpp"
#include "nearlink_sle_advertise_callback_stub.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "log.h"
#include "ipc_types.h"
#include "string_ex.h"


namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

// 用于void函数测试，可仅检测日志打印来判断
std::string g_advHiLog;
void AdvLogCallback(const LogType type, const LogLevel level, const unsigned int domain, const char *tag,
                const char *msg)
{
    g_advHiLog = msg;
}

namespace {
    enum class AdvertiserCBTest : int {
        ON_START_RESULT_EVENT_TEST = 0,
        ON_STOP_RESULT_EVENT_TEST = 1,
        ON_ENABLE_RESULT_EVENT_TEST = 2,
        ON_DISABLE_RESULT_EVENT_TEST = 3,
        ON_AUTO_STOP_ADV_EVENT_TEST = 4,
        ON_SET_ADV_DATA_EVENT_TEST = 5,
        ADVERTISER_EVENT_TEST_MAX = 6,
    };

    int32_t advHandle_ = -1;
    int advertiserCbTestStatus_ = static_cast<int>(AdvertiserCBTest::ADVERTISER_EVENT_TEST_MAX);
    void ResetAdvertiserCbTestStatus()
    {
        advertiserCbTestStatus_ = static_cast<int>(AdvertiserCBTest::ADVERTISER_EVENT_TEST_MAX);
    }

    class NearlinkSleAdvertiserCallBackStubTest : public NearlinkSleAdvertiseCallbackStub {
    public:
        void OnStartResultEvent(int32_t result, int32_t advHandle, int32_t opcode) override {
            advertiserCbTestStatus_ = static_cast<int>(AdvertiserCBTest::ON_START_RESULT_EVENT_TEST);
        }
        void OnStopResultEvent(int32_t result, int32_t advHandle) override {
            advertiserCbTestStatus_ = static_cast<int>(AdvertiserCBTest::ON_STOP_RESULT_EVENT_TEST);
        }
        void OnEnableResultEvent(int32_t result, int32_t advHandle) override {
            advertiserCbTestStatus_ = static_cast<int>(AdvertiserCBTest::ON_ENABLE_RESULT_EVENT_TEST);
        }
        void OnDisableResultEvent(int32_t result, int32_t advHandle) override {
            advertiserCbTestStatus_ = static_cast<int>(AdvertiserCBTest::ON_DISABLE_RESULT_EVENT_TEST);
        }
        void OnAutoStopAdvEvent(int32_t advHandle) override {
            advertiserCbTestStatus_ = static_cast<int>(AdvertiserCBTest::ON_AUTO_STOP_ADV_EVENT_TEST);
        }
        void OnSetAdvDataEvent(int32_t result, int32_t advHandle) override {
            advertiserCbTestStatus_ = static_cast<int>(AdvertiserCBTest::ON_SET_ADV_DATA_EVENT_TEST);
        }
        bool AddDeathRecipient(const sptr<DeathRecipient> &recipient) override {
            return true;
        }
    };

    class MockNearlinkSleAdvertiserCallbackStub : public IRemoteStub<INearlinkSleAdvertiseCallback> {
    public:
        void OnStartResultEvent(int32_t result, int32_t advHandle, int32_t opcode) {
        }
        void OnStopResultEvent(int32_t result, int32_t advHandle) {
        }
        void OnEnableResultEvent(int32_t result, int32_t advHandle) {
        }
        void OnDisableResultEvent(int32_t result, int32_t advHandle) {
        }
        void OnAutoStopAdvEvent(int32_t advHandle) {
        }
        void OnSetAdvDataEvent(int32_t result, int32_t advHandle) {
        }
        sptr<OHOS::IRemoteObject> AsObject() override
        {
            sptr<NearlinkSleAdvertiserCallBackStubTest> nearlinkSleAdvertiserCallBackStubTest =
            new NearlinkSleAdvertiserCallBackStubTest();
            return nearlinkSleAdvertiserCallBackStubTest;
        }
    };

    // 用于ipc传输
    sptr<NearlinkSleAdvertiserServer> g_advertiserServer = new (std::nothrow) NearlinkSleAdvertiserServer();
    // 用于构造数据
    sptr<INearlinkSleAdvertiseCallback> g_sleAdvertiserCb =
        new (std::nothrow) MockNearlinkSleAdvertiserCallbackStub();
    // 用于写proxy内函数用例
    sptr<IRemoteObject> mockImpl = nullptr;
    sptr<NearlinkSleAdvertiserCallbackProxy> g_advertiserCbProxy =
        new (std::nothrow) NearlinkSleAdvertiserCallbackProxy(mockImpl);
}

class NearlinkAdvertiserServerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkAdvertiserServerTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkAdvertiserServerTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
}

void NearlinkAdvertiserServerTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkAdvertiserServerTest");
}

void NearlinkAdvertiserServerTest::SetUp()
{
    HILOGI("SetUp NearlinkAdvertiserServerTest.");
    
}

void NearlinkAdvertiserServerTest::TearDown()
{
    HILOGI("TearDown NearlinkAdvertiserServerTest.");
}

int32_t AdvertiserOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_advertiserServer, TRANSACTION_ERR, "g_advertiserServer is nullptr");
    MessageOption option = { MessageOption::TF_SYNC };
    HILOGI("g_advertiserServer OnRemoteRequest, cmd(%{public}d)", code);
    return g_advertiserServer->OnRemoteRequest(code, data, reply, option);
}

/**
 * @tc.name: AdvertiserStubTest001SLE_REGISTER_SLE_ADVERTISER_CALLBACK
 * @tc.desc: 测试 advertiser stub RegisterSleAdvertiserCallbackInner
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserStubTest001, TestSize.Level1)
{
    HILOGI("AdvertiserStubTest001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());
    if (g_sleAdvertiserCb != nullptr) {
        data.WriteRemoteObject(g_sleAdvertiserCb->AsObject());
    }
    int32_t result = AdvertiserOnRemoteRequest(SLE_REGISTER_SLE_ADVERTISER_CALLBACK, data, reply);
    EXPECT_EQ(result, NO_ERROR);
    HILOGI("AdvertiserStubTest001 end");
}

/**
 * @tc.name: AdvertiserStubTest002
 * @tc.desc: 测试 advertiser stub DeregisterSleAdvertiserCallbackInner
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserStubTest002, TestSize.Level1)
{
    HILOGI("AdvertiserStubTest002 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());
    if (g_sleAdvertiserCb != nullptr) {
        data.WriteRemoteObject(g_sleAdvertiserCb->AsObject());
    }
    int32_t result = AdvertiserOnRemoteRequest(SLE_DE_REGISTER_SLE_ADVERTISER_CALLBACK, data, reply);
    EXPECT_EQ(result, NO_ERROR);
    HILOGI("AdvertiserStubTest002 end");
}

/**
 * @tc.name: AdvertiserStubTest003
 * @tc.desc: 测试 advertiser stub StartAdvertisingInner
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserStubTest003, TestSize.Level1)
{
    HILOGI("AdvertiserStubTest003 start");
    MessageParcel reply;
    MessageParcel data;
    int32_t advHandle = 1;
    NearlinkSleAdvertiserSettings settings;
    NearlinkSleAdvertiserData advData;
    NearlinkSleAdvertiserData scanResponse;
    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());
    data.WriteParcelable(&settings);
    data.WriteParcelable(&advData);
    data.WriteParcelable(&scanResponse);
    data.WriteInt32(advHandle);
    int32_t result = AdvertiserOnRemoteRequest(SLE_START_ADVERTISING, data, reply);
    EXPECT_EQ(result, NO_ERROR);
    HILOGI("AdvertiserStubTest003 end");
}

/**
 * @tc.name: AdvertiserStubTest004
 * @tc.desc: 测试 advertiser stub StopAdvertisingInner
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserStubTest004, TestSize.Level1)
{
    HILOGI("AdvertiserStubTest004 start");
    MessageParcel data;
    MessageParcel reply;
    int32_t advHandle = 1;
    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());
    data.WriteInt32(advHandle);
    int32_t result = AdvertiserOnRemoteRequest(SLE_STOP_ADVERTISING, data, reply);
    EXPECT_EQ(result, NO_ERROR);
    HILOGI("AdvertiserStubTest004 end");
}

/**
 * @tc.name: AdvertiserStubTest005
 * @tc.desc: 测试 advertiser stub GetAdvertiserHandleInner
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserStubTest005, TestSize.Level1)
{
    HILOGI("AdvertiserStubTest005 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());
    int32_t result = AdvertiserOnRemoteRequest(SLE_GET_ADVERTISER_HANDLE, data, reply);
    EXPECT_EQ(result, NO_ERROR);
    HILOGI("AdvertiserStubTest005 end");
}

/**
 * @tc.name: AdvertiserStubTest006
 * @tc.desc: 测试 advertiser stub SetAdvertisingDataInner
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserStubTest006, TestSize.Level1)
{
    HILOGI("AdvertiserStubTest006 start");
    MessageParcel reply;
    MessageParcel data; 
    int32_t advHandle = 1;
    NearlinkSleAdvertiserData advData;
    NearlinkSleAdvertiserData scanResponse;
    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());
    data.WriteParcelable(&advData);
    data.WriteParcelable(&scanResponse);
    data.WriteInt32(advHandle);
    int32_t result = AdvertiserOnRemoteRequest(SLE_SET_ADVERTISING_DATA, data, reply);
    EXPECT_EQ(result, NO_ERROR);
    HILOGI("AdvertiserStubTest006 end");
}

/**
 * @tc.name: AdvertiserStubTest007
 * @tc.desc: 测试 advertiser stub EnableAdvertisingInner
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserStubTest007, TestSize.Level1)
{
    HILOGI("AdvertiserStubTest007 start");
    MessageParcel data;
    MessageParcel reply;
    int32_t advHandle = 1;
    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());
    data.WriteInt32(advHandle);
    int32_t result = AdvertiserOnRemoteRequest(SLE_ENABLE_ADVERTISING, data, reply);
    EXPECT_EQ(result, NO_ERROR);
    HILOGI("AdvertiserStubTest007 end");
}

/**
 * @tc.name: AdvertiserStubTest008
 * @tc.desc: 测试 advertiser stub DisableAdvertisingInner
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserStubTest008, TestSize.Level1)
{
    HILOGI("AdvertiserStubTest008 start");
    MessageParcel data;
    MessageParcel reply;
    int32_t advHandle = 1;
    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());
    data.WriteInt32(advHandle);
    int32_t result = AdvertiserOnRemoteRequest(SLE_DISABLE_ADVERTISING, data, reply);
    EXPECT_EQ(result, NO_ERROR);
    HILOGI("AdvertiserStubTest008 end");
}

/**
 * @tc.name: AdvertiserCbProxyTest001
 * @tc.desc: 测试 advertiser call back proxy OnStartResultEvent
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserCbProxyTest001, TestSize.Level1)
{
    HILOGI("AdvertiserCbProxyTest001 start");
    g_advHiLog = "";
    LOG_SetCallback(AdvLogCallback);
    g_advertiserCbProxy->OnStartResultEvent(0, 1, 0);
    EXPECT_TRUE(g_advHiLog.find("done fail") != std::string::npos);
    HILOGI("AdvertiserCbProxyTest001 end");
}

/**
 * @tc.name: AdvertiserCbProxyTest002
 * @tc.desc: 测试 advertiser call back proxy OnStopResultEvent
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserCbProxyTest002, TestSize.Level1)
{
    HILOGI("AdvertiserCbProxyTest002 start");
    g_advHiLog = "";
    LOG_SetCallback(AdvLogCallback);
    g_advertiserCbProxy->OnStopResultEvent(0, 1);
    EXPECT_TRUE(g_advHiLog.find("done fail") != std::string::npos);
    HILOGI("AdvertiserCbProxyTest002 end");
}

/**
 * @tc.name: AdvertiserCbProxyTest003
 * @tc.desc: 测试 advertiser call back proxy OnEnableResultEvent
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserCbProxyTest003, TestSize.Level1)
{
    HILOGI("AdvertiserCbProxyTest003 start");
    g_advHiLog = "";
    LOG_SetCallback(AdvLogCallback);
    g_advertiserCbProxy->OnEnableResultEvent(0, 1);
    EXPECT_TRUE(g_advHiLog.find("done fail") != std::string::npos);
    HILOGI("AdvertiserCbProxyTest003 end");
}

/**
 * @tc.name: AdvertiserCbProxyTest004
 * @tc.desc: 测试 advertiser call back proxy OnDisableResultEvent
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserCbProxyTest004, TestSize.Level1)
{
    HILOGI("AdvertiserCbProxyTest004 start");
    g_advHiLog = "";
    LOG_SetCallback(AdvLogCallback);
    g_advertiserCbProxy->OnDisableResultEvent(0, 1);
    EXPECT_TRUE(g_advHiLog.find("done fail") != std::string::npos);
    HILOGI("AdvertiserCbProxyTest004 end");
}

/**
 * @tc.name: AdvertiserCbProxyTest005
 * @tc.desc: 测试 advertiser call back proxy OnAutoStopAdvEvent
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserCbProxyTest005, TestSize.Level1)
{
    HILOGI("AdvertiserCbProxyTest005 start");
    g_advHiLog = "";
    LOG_SetCallback(AdvLogCallback);
    g_advertiserCbProxy->OnAutoStopAdvEvent(1);
    EXPECT_TRUE(g_advHiLog.find("done fail") != std::string::npos);
    HILOGI("AdvertiserCbProxyTest005 end");
}

/**
 * @tc.name: AdvertiserCbProxyTest006
 * @tc.desc: 测试 advertiser call back proxy OnSetAdvDataEvent
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserCbProxyTest006, TestSize.Level1)
{
    HILOGI("AdvertiserCbProxyTest006 start");
    g_advHiLog = "";
    LOG_SetCallback(AdvLogCallback);
    g_advertiserCbProxy->OnSetAdvDataEvent(0, 1);
    EXPECT_TRUE(g_advHiLog.find("done fail") != std::string::npos);
    HILOGI("AdvertiserCbProxyTest006 end");
}

/**
 * @tc.name: AdvertiserServerTest001
 * @tc.desc: advertiser server GetAdvertiserHandle
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserServerTest001, TestSize.Level1)
{
    HILOGI("AdvertiserServerTest001 start");
    NlErrCode res = g_advertiserServer->RegisterSleAdvertiserCallback(g_sleAdvertiserCb);
    EXPECT_EQ(res, NL_NO_ERROR);
    res = g_advertiserServer->GetAdvertiserHandle(advHandle_);
    EXPECT_EQ(res, NL_NO_ERROR);
    HILOGI("AdvertiserServerTest001 end");
}

/**
 * @tc.name: AdvertiserServerTest002
 * @tc.desc: advertiser server OnSystemStateChange & RegisterCallback & StartAdvertising
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserServerTest002, TestSize.Level1)
{
    HILOGI("AdvertiserServerTest002 start");
    g_advertiserServer->pimpl->systemStateObserver_->OnSystemStateChange(SleSystemState::OFF); // 分支覆盖
    g_advertiserServer->pimpl->systemStateObserver_->OnSystemStateChange(SleSystemState::ON);
    NearlinkSleAdvertiserSettings settings;
    NearlinkSleAdvertiserData advData;
    NearlinkSleAdvertiserData scanResponse;
    int32_t advHandle = advHandle_;
    NlErrCode res = g_advertiserServer->StartAdvertising(settings, advData, scanResponse, advHandle);
    EXPECT_EQ(res, NL_NO_ERROR);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // 分支覆盖
    advData.SetIncludeDeviceName(true);
    res = g_advertiserServer->StartAdvertising(settings, advData, scanResponse, advHandle);
    EXPECT_EQ(res, NL_NO_ERROR);
    std::string longDataStr(0xFF, 'a');
    advData.SetPayload(longDataStr);
    res = g_advertiserServer->StartAdvertising(settings, advData, scanResponse, advHandle);
    EXPECT_EQ(res, NL_NO_ERROR);
    HILOGI("AdvertiserServerTest002 end");
}

/**
 * @tc.name: AdvertiserServerTest003
 * @tc.desc: advertiser server OnStartResultEvent & OnStopResultEvent
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserServerTest003, TestSize.Level1)
{
    HILOGI("AdvertiserServerTest003 start");
    ResetAdvertiserCbTestStatus();
    g_advertiserServer->pimpl->observerImp_->OnStartResultEvent(0, advHandle_);
    EXPECT_EQ(advertiserCbTestStatus_, static_cast<int>(AdvertiserCBTest::ON_START_RESULT_EVENT_TEST));
    ResetAdvertiserCbTestStatus();
    g_advertiserServer->pimpl->observerImp_->OnStopResultEvent(0, advHandle_);
    EXPECT_EQ(advertiserCbTestStatus_, static_cast<int>(AdvertiserCBTest::ON_STOP_RESULT_EVENT_TEST));
    HILOGI("AdvertiserServerTest003 end");
}

/**
 * @tc.name: AdvertiserServerTest004
 * @tc.desc: advertiser server OnEnableResultEvent & OnDisableResultEvent
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserServerTest004, TestSize.Level1)
{
    HILOGI("AdvertiserServerTest004 start");
    ResetAdvertiserCbTestStatus();
    g_advertiserServer->pimpl->observerImp_->OnEnableResultEvent(0, advHandle_);
    EXPECT_EQ(advertiserCbTestStatus_, static_cast<int>(AdvertiserCBTest::ON_ENABLE_RESULT_EVENT_TEST));
    ResetAdvertiserCbTestStatus();
    g_advertiserServer->pimpl->observerImp_->OnDisableResultEvent(0, advHandle_);
    EXPECT_EQ(advertiserCbTestStatus_, static_cast<int>(AdvertiserCBTest::ON_DISABLE_RESULT_EVENT_TEST));
    HILOGI("AdvertiserServerTest004 end");
}

/**
 * @tc.name: AdvertiserServerTest005
 * @tc.desc: advertiser server OnAutoStopAdvEvent & OnSetAdvDataEvent & StopAdvertising
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserServerTest005, TestSize.Level1)
{
    HILOGI("AdvertiserServerTest005 start");
    ResetAdvertiserCbTestStatus();
    g_advertiserServer->pimpl->observerImp_->OnAutoStopAdvEvent(advHandle_);
    EXPECT_EQ(advertiserCbTestStatus_, static_cast<int>(AdvertiserCBTest::ON_AUTO_STOP_ADV_EVENT_TEST));
    ResetAdvertiserCbTestStatus();
    g_advertiserServer->pimpl->observerImp_->OnSetAdvDataEvent(0, advHandle_);
    EXPECT_EQ(advertiserCbTestStatus_, static_cast<int>(AdvertiserCBTest::ON_SET_ADV_DATA_EVENT_TEST));
    HILOGI("AdvertiserServerTest005 end");
}

/**
 * @tc.name: AdvertiserServerTest006
 * @tc.desc: advertiser server DeregisterSleAdvertiserCallback with null callback
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserServerTest006, TestSize.Level2)
{
    HILOGI("AdvertiserServerTest006 start");
    NlErrCode res = g_advertiserServer->DeregisterSleAdvertiserCallback(nullptr);
    EXPECT_EQ(res, NL_ERR_IMPL_ERROR);
    HILOGI("AdvertiserServerTest006 end");
}

/**
 * @tc.name: AdvertiserServerTest007
 * @tc.desc: advertiser server RegisterSleAdvertiserCallback ERR
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvertiserServerTest, AdvertiserServerTest007, TestSize.Level2)
{
    HILOGI("AdvertiserServerTest007 start");
    NlErrCode res = g_advertiserServer->RegisterSleAdvertiserCallback(nullptr);
    EXPECT_EQ(res, NL_ERR_INVALID_PARAM);

    sptr<INearlinkSleAdvertiseCallback> mockSleAdvertiserCb;
    for (int i = 0; i < MAX_OBSERVER_SIZE - 1; ++i) { // server第一个用例已经注册了一次，这里再注册199个
        mockSleAdvertiserCb = new (std::nothrow) MockNearlinkSleAdvertiserCallbackStub(); // TDD用例跑完进程结束会释放资源
        g_advertiserServer->RegisterSleAdvertiserCallback(mockSleAdvertiserCb);
    }
    mockSleAdvertiserCb = new (std::nothrow) MockNearlinkSleAdvertiserCallbackStub();
    res = g_advertiserServer->RegisterSleAdvertiserCallback(mockSleAdvertiserCb); // 注册第201个的时候，会失败
    EXPECT_EQ(res, NL_ERR_INTERNAL_ERROR);
    HILOGI("AdvertiserServerTest007 end");
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS