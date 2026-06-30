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
#include "nearlink_sle_central_manager_stub.h"
#include "nearlink_sle_central_manager_callback_proxy.h"
#include "nearlink_sle_central_manager_server.cpp"
#include "nearlink_sle_central_manager_callback_stub.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "nearlink_uuid_parcel.h"
#include "log.h"
#include "ipc_types.h"
#include "string_ex.h"


namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

// 用于void函数测试，可仅检测日志打印来判断
std::string g_hiLog;
void MyLogCallback(const LogType type, const LogLevel level, const unsigned int domain, const char *tag,
                const char *msg)
{
    if (strstr(tag, "nearlink_server") != nullptr || strstr(tag, "nearlink_test") != nullptr) {
        g_hiLog = msg;
    }
}

namespace {
    enum class CentralManagerCBTest : int {
        ON_SCAN_CB_EVENT_TEST = 0,
        ON_SLE_BATCH_SCAN_RESULT_EVENT_TEST = 1,
        ON_START_OR_STOP_SACN_EVENT_TEST = 2,
        CENTRAL_MANAGER_EVENT_TEST_MAX = 3,
    };

    uint32_t scannerId_ = SLE_SCAN_INVALID_ID;
    int centralManagerCbTestStatuts_ = static_cast<int>(CentralManagerCBTest::CENTRAL_MANAGER_EVENT_TEST_MAX);
    void ResetCentralManagerCbTestStatuts()
    {
        centralManagerCbTestStatuts_ = static_cast<int>(CentralManagerCBTest::CENTRAL_MANAGER_EVENT_TEST_MAX);
    }

    class NearlinkSleCentralManagerCallBackStubTest : public NearlinkSleCentralManagerCallBackStub {
    public:
        void OnScanCallback(const NearlinkSleScanResult &result) override {
            centralManagerCbTestStatuts_ = static_cast<int>(CentralManagerCBTest::ON_SCAN_CB_EVENT_TEST);
        }
        void OnStartOrStopScanEvent(int resultCode, bool isStartScan) override {
            centralManagerCbTestStatuts_ = static_cast<int>(CentralManagerCBTest::ON_START_OR_STOP_SACN_EVENT_TEST);
        }
        bool AddDeathRecipient(const sptr<DeathRecipient> &recipient) override {
            return true;
        }
        void OnSleBatchScanResultsEvent(std::vector<NearlinkSleScanResult> &results) {
        }
    };

    class MockNearlinkSleCentralManagerCallbackStub : public IRemoteStub<INearlinkSleCentralManagerCallback> {
    public:
        void OnScanCallback(const NearlinkSleScanResult &result) {
        }
        void OnStartOrStopScanEvent(int resultCode, bool isStartScan) {
        }
        sptr<OHOS::IRemoteObject> AsObject() override
        {
            sptr<NearlinkSleCentralManagerCallBackStubTest> nearlinkSleCentralManagerCallBackStubTest =
            new NearlinkSleCentralManagerCallBackStubTest();
            return nearlinkSleCentralManagerCallBackStubTest;
        }
        virtual void OnSleBatchScanResultsEvent(std::vector<NearlinkSleScanResult> &results) {
        }
    };

    // 用于ipc传输
    NearlinkSleCentralManagerServer* g_centralManagerServer = nullptr;
    // 用于构造数据
    sptr<INearlinkSleCentralManagerCallback> g_sleCentralManagerCb =
        new (std::nothrow) MockNearlinkSleCentralManagerCallbackStub();
    // 用于写proxy内函数用例
    sptr<IRemoteObject> mockImpl = nullptr;
    sptr<NearlinkSleCentralManagerCallBackProxy> g_centralManagerCbProxy =
        new (std::nothrow) NearlinkSleCentralManagerCallBackProxy(mockImpl);
}

class NearlinkCentralManagerServerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkCentralManagerServerTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkCentralManagerServerTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    g_centralManagerServer = new (std::nothrow) NearlinkSleCentralManagerServer();
}

void NearlinkCentralManagerServerTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkCentralManagerServerTest");
    delete g_centralManagerServer;
}

void NearlinkCentralManagerServerTest::SetUp()
{
    HILOGI("SetUp NearlinkCentralManagerServerTest.");
}

void NearlinkCentralManagerServerTest::TearDown()
{
    HILOGI("TearDown NearlinkCentralManagerServerTest.");
}

int32_t CentralManagerOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_centralManagerServer, TRANSACTION_ERR, "g_centralManagerServer is nullptr");
    MessageOption option = { MessageOption::TF_SYNC };
    HILOGI("g_centralManagerServer OnRemoteRequest, cmd(%{public}d)", code);
    return g_centralManagerServer->OnRemoteRequest(code, data, reply, option);
}

/**
 * @tc.name: CentralManagerStubTest001
 * @tc.desc: 测试 central manager stub RegisterSleCentralManagerCallbackInner
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCentralManagerServerTest, CentralManagerStubTest001, TestSize.Level1)
{
    HILOGI("CentralManagerStubTest001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    if (g_sleCentralManagerCb != nullptr) {
        data.WriteRemoteObject(g_sleCentralManagerCb->AsObject());
    }
    int32_t result = CentralManagerOnRemoteRequest(SLE_REGISTER_SLE_CENTRAL_MANAGER_CALLBACK, data, reply);
    EXPECT_EQ(result, NO_ERROR);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        scannerId_ = reply.ReadUint32();
    }
    HILOGI("scannerId = %{public}u", scannerId_);
    HILOGI("CentralManagerStubTest001 end");
}

/**
 * @tc.name: CentralManagerStubTest002
 * @tc.desc: 测试 central manager stub DeregisterSleCentralManagerCallbackInner
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCentralManagerServerTest, CentralManagerStubTest002, TestSize.Level1)
{
    HILOGI("CentralManagerStubTest002 start");
    MessageParcel data;
    MessageParcel reply;
    uint32_t scannerId = SLE_SCAN_INVALID_ID;
    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    data.WriteUint32(scannerId);
    if (g_sleCentralManagerCb != nullptr) {
        data.WriteRemoteObject(g_sleCentralManagerCb->AsObject());
    }
    int32_t result = CentralManagerOnRemoteRequest(SLE_DE_REGISTER_SLE_CENTRAL_MANAGER_CALLBACK, data, reply);
    EXPECT_EQ(result, NO_ERROR);
    HILOGI("CentralManagerStubTest002 end");
}

/**
 * @tc.name: CentralManagerStubTest003
 * @tc.desc: 测试 central manager stub StartFullScanInner
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCentralManagerServerTest, CentralManagerStubTest003, TestSize.Level1)
{
    HILOGI("CentralManagerStubTest003 start");
    MessageParcel data;
    MessageParcel reply;
    uint32_t scannerId = 1;
    NearlinkSleScanSettings settings;
    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    data.WriteUint32(scannerId);
    data.WriteParcelable(&settings);
    int32_t result = CentralManagerOnRemoteRequest(SLE_START_FULL_SCAN, data, reply);
    EXPECT_EQ(result, NO_ERROR);
    HILOGI("CentralManagerStubTest003 end");
}

/**
 * @tc.name: CentralManagerStubTest004
 * @tc.desc: 测试 central manager stub StartScanWithFilterInner
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCentralManagerServerTest, CentralManagerStubTest004, TestSize.Level1)
{
    HILOGI("CentralManagerStubTest004 start");
    MessageParcel data;
    MessageParcel reply;
    uint32_t scannerId = 1;
    NearlinkSleScanSettings settings;
    std::vector<NearlinkSleScanFilter> filters = {NearlinkSleScanFilter()};

    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    data.WriteUint32(scannerId);
    data.WriteParcelable(&settings);
    data.WriteUint32(filters.size());
    for (size_t i = 0; i < filters.size(); i++) {
        data.WriteParcelable(&filters[i]);
    }
    int32_t result = CentralManagerOnRemoteRequest(SLE_START_SCAN_WITH_FILTER, data, reply);
    EXPECT_EQ(result, NO_ERROR);
    HILOGI("CentralManagerStubTest004 end");
}

/**
 * @tc.name: CentralManagerStubTest005
 * @tc.desc: 测试 central manager stub StopScanInner
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCentralManagerServerTest, CentralManagerStubTest005, TestSize.Level1)
{
    HILOGI("CentralManagerStubTest005 start");
    MessageParcel data;
    MessageParcel reply;
    uint32_t scannerId = 1;
    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    data.WriteUint32(scannerId);
    int32_t result = CentralManagerOnRemoteRequest(SLE_STOP_SCAN, data, reply);
    EXPECT_EQ(result, NO_ERROR);
    HILOGI("CentralManagerStubTest005 end");
}

/**
 * @tc.name: CentralManagerCbProxyTest001
 * @tc.desc: 测试 central manager call back proxy OnScanCallback
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCentralManagerServerTest, CentralManagerCbProxyTest001, TestSize.Level1)
{
    HILOGI("CentralManagerCbProxyTest001 start");
    g_hiLog = "";
    LOG_SetCallback(MyLogCallback);
    NearlinkSleScanResult sleScanResult;
    g_centralManagerCbProxy->OnScanCallback(sleScanResult);
    EXPECT_TRUE(g_hiLog.find("done fail") != std::string::npos);
    HILOGI("CentralManagerCbProxyTest001 end");
}

/**
 * @tc.name: CentralManagerCbProxyTest002
 * @tc.desc: 测试 central manager call back proxy OnStartOrStopScanEvent
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCentralManagerServerTest, CentralManagerCbProxyTest002, TestSize.Level1)
{
    HILOGI("CentralManagerCbProxyTest002 start");
    g_hiLog = "";
    LOG_SetCallback(MyLogCallback);
    int resultCode = 0;
    bool isStartScan = true;
    g_centralManagerCbProxy->OnStartOrStopScanEvent(resultCode, isStartScan);
    EXPECT_TRUE(g_hiLog.find("done fail") != std::string::npos);
    HILOGI("CentralManagerCbProxyTest002 end");
}

/**
 * @tc.name: CentralManagerServerTest001
 * @tc.desc: central manager server OnRemoteDied
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCentralManagerServerTest, CentralManagerServerTest001, TestSize.Level1)
{
    HILOGI("CentralManagerServerTest001 start");
    g_hiLog = "";
    LOG_SetCallback(MyLogCallback);
    g_centralManagerServer->pimpl->remoteContainer_->OnRemoteDied(mockImpl);
    EXPECT_TRUE(g_hiLog.find("remote info unexpectedly not found") != std::string::npos);
    HILOGI("CentralManagerServerTest001 end");
}

/**
 * @tc.name: CentralManagerServerTest002
 * @tc.desc: central manager server OnSystemStateChange & startScan
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCentralManagerServerTest, CentralManagerServerTest002, TestSize.Level1)
{
    HILOGI("CentralManagerServerTest002 start");
    g_centralManagerServer->pimpl->systemStateObserver_->OnSystemStateChange(SleSystemState::ON);
    NearlinkSleScanSettings settings;
    NearlinkSleScanFilter filter1;
    NearlinkSleScanFilter filter2;
    filter1.SetDeviceId("00:01:02:03:04:05");
    filter2.SetDeviceId("0A:0B:0C:0D:0E:0F");
    filter2.SetName("TESTNAME");
    std::vector<uint8_t> serviceData = {0xEE, 0XEE};
    std::vector<uint8_t> serviceDataMask = {0xFF, 0XFF};
    filter2.SetServiceData(serviceData);
    filter2.SetServiceDataMask(serviceDataMask);
    std::vector<NearlinkSleScanFilter> filters = {filter1, filter2};
    NlErrCode res = g_centralManagerServer->StartScanWithFilter(scannerId_, settings, filters);
    EXPECT_EQ(res, NL_NO_ERROR);
    res = g_centralManagerServer->StartFullScan(0, settings);
    EXPECT_EQ(res, NL_ERR_INTERNAL_ERROR);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    HILOGI("CentralManagerServerTest002 end");
}

/**
 * @tc.name: CentralManagerServerTest003
 * @tc.desc: central manager server OnStartOrStopScanEvent
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCentralManagerServerTest, CentralManagerServerTest003, TestSize.Level1)
{
    HILOGI("CentralManagerServerTest003 start");
    ResetCentralManagerCbTestStatuts();
    g_centralManagerServer->pimpl->observerImp_->OnStartOrStopScanEvent(0, true);
    EXPECT_EQ(centralManagerCbTestStatuts_ , static_cast<int>(CentralManagerCBTest::ON_START_OR_STOP_SACN_EVENT_TEST));
    HILOGI("CentralManagerServerTest003 end");
}

/**
 * @tc.name: CentralManagerServerTest004
 * @tc.desc: central manager server StopScan & DeregisterSleCentralManagerCallback
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCentralManagerServerTest, CentralManagerServerTest004, TestSize.Level1)
{
    HILOGI("CentralManagerServerTest004 start");
    NlErrCode res = g_centralManagerServer->StopScan(scannerId_);
    EXPECT_EQ(res, NL_NO_ERROR);

    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    data.WriteUint32(scannerId_);
    if (g_sleCentralManagerCb != nullptr) {
        data.WriteRemoteObject(g_sleCentralManagerCb->AsObject());
    }
    int32_t result = CentralManagerOnRemoteRequest(SLE_DE_REGISTER_SLE_CENTRAL_MANAGER_CALLBACK, data, reply);
    EXPECT_EQ(result, NO_ERROR);
    HILOGI("CentralManagerServerTest004 end");
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
