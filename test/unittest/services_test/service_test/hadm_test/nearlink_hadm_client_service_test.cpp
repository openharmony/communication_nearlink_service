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

#include "interface_hadm_client_service.h"
#include "nearlink_raw_address.h"
#include "nearlink_hadm_client_server.cpp"
#include "nearlink_hadm_client_service.cpp"
#include "nearlink_hadm_stack_adapter.cpp"

#include "SleInterfaceAdapter.h"
#include "log.h"

#include "SleServiceManager.h"


namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
constexpr int VCP_SERVICE_UT_DELAY_50_MS = 50;
constexpr int VCP_SERVICE_TDD_DELAY_1000_MS = 1000;
}

class MockInterfaceHadmClientServiceCallback : public InterfaceHadmClientServiceCallback {
public:
    MockInterfaceHadmClientServiceCallback() = default;
    virtual ~MockInterfaceHadmClientServiceCallback() = default;
    
    void OnSoundingResult(const RawAddress &addr, const NearlinkHadmSoundingResult &result, uint32_t hadmId)
    {
        HILOGI("MockInterfaceHadmClientServiceCallback::OnSoundingResult");
    }
    void OnSoundingStateChange(const RawAddress &addr, int newState, int errorCode, uint32_t hadmId)
    {
        HILOGI("MockInterfaceHadmClientServiceCallback::OnSoundingStateChange");
    }
};

class NearlinkHadmClientServeiceTest : public testing::Test {
public:
    NearlinkHadmClientServeiceTest() = default;
    ~NearlinkHadmClientServeiceTest() = default;

    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkHadmClientServeiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkHadmClientServeiceTest.");
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::VCP_SERVICE_TDD_DELAY_1000_MS));
}

void NearlinkHadmClientServeiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkHadmClientServeiceTest");
}

void NearlinkHadmClientServeiceTest::SetUp()
{
    HILOGI("SetUp NearlinkHadmClientServeiceTest.");
}

void NearlinkHadmClientServeiceTest::TearDown()
{
    HILOGI("TearDown NearlinkHadmClientServeiceTest.");
}

HWTEST_F(NearlinkHadmClientServeiceTest, RegisterNearlinkHadmClientCallbackTest001, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientServeiceTest:RegisterNearlinkHadmClientCallbackTest001 start");
    std::shared_ptr<MockInterfaceHadmClientServiceCallback> callback = std::make_shared<MockInterfaceHadmClientServiceCallback>();
    InterfaceHadmClientService::GetInstance().RegisterNearlinkHadmClientCallback(callback);
    sleep(2);
    HILOGI("NearlinkHadmClientServeiceTest:RegisterNearlinkHadmClientCallbackTest001 end");
}

HWTEST_F(NearlinkHadmClientServeiceTest, DeregisterNearlinkHadmClientCallback, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientServeiceTest:DeregisterNearlinkHadmClientCallback start");
    InterfaceHadmClientService::GetInstance().DeregisterNearlinkHadmClientCallback();
    sleep(2);
    HILOGI("NearlinkHadmClientServeiceTest:DeregisterNearlinkHadmClientCallback end");
}

HWTEST_F(NearlinkHadmClientServeiceTest, StartSoundingTest001, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientServeiceTest:StartSoundingTest001 start");
    uint32_t hadmId = 1;
    NearlinkRawAddress addr;
    addr.SetAddress("00:11:22:33:44:55");
    InterfaceHadmClientService::GetInstance().StartSounding(hadmId, addr);
    sleep(2);
    HILOGI("NearlinkHadmClientServeiceTest:StartSoundingTest001 end");
}

HWTEST_F(NearlinkHadmClientServeiceTest, StopSoundingTest001, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientServeiceTest:StopSoundingTest001 start");
    uint32_t hadmId = 1;
    NearlinkRawAddress addr;
    addr.SetAddress("00:11:22:33:44:55");
    InterfaceHadmClientService::GetInstance().StopSounding(hadmId, addr);
    sleep(2);
    HILOGI("NearlinkHadmClientServeiceTest:StopSoundingTest001 end");
}

HWTEST_F(NearlinkHadmClientServeiceTest, StopSoundingByIdTest001, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientServeiceTest:StopSoundingByIdTest001 start");
    uint32_t hadmId = 1;
    InterfaceHadmClientService::GetInstance().StopSoundingById(hadmId);
    sleep(2);
    HILOGI("NearlinkHadmClientServeiceTest:StopSoundingByIdTest001 end");
}

HWTEST_F(NearlinkHadmClientServeiceTest, OnSoundingStateChange001, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientServeiceTest:onSetSoundingEnableDisable001 start");
    NearlinkHadmClientServer hadmClientServer;

    HadmClientService::impl hadmClientServiceImpl;
    HadmClientService::impl::StackAdapterCallback stackAdapterCallback(hadmClientServiceImpl);
    NearlinkHadmStackAdapter nearlinkHadmStackAdapterTest(stackAdapterCallback);

    SLE_Addr_S addrTest;
    addrTest.type = 1;
    addrTest.addr[0] = 0x01;
    addrTest.addr[1] = 0x02;
    addrTest.addr[2] = 0x03;
    addrTest.addr[3] = 0x04;
    addrTest.addr[4] = 0x05;
    addrTest.addr[5] = 0x06;

    NLSTK_Errcode_E errorCode = static_cast<NLSTK_Errcode_E>(HADM_SUCCESS);
    HadmUserOperate_E ctrlTypeTest = static_cast<HadmUserOperate_E>(HADM_SOUNDING_USER_START);
    nearlinkHadmStackAdapterTest.onSetSoundingEnableDisable(&addrTest, ctrlTypeTest, errorCode);
    sleep(2);

    ctrlTypeTest = static_cast<HadmUserOperate_E>(HADM_SOUNDING_USER_STOP);
    hadmClientServiceImpl.restartFlag_ = true;
    nearlinkHadmStackAdapterTest.onSetSoundingEnableDisable(&addrTest, ctrlTypeTest, errorCode);
    sleep(2);

    errorCode = static_cast<NLSTK_Errcode_E>(HADM_FAILURE);
    ctrlTypeTest = static_cast<HadmUserOperate_E>(HADM_SOUNDING_USER_START);
    nearlinkHadmStackAdapterTest.onSetSoundingEnableDisable(&addrTest, ctrlTypeTest, errorCode);
    sleep(2);
    HILOGI("NearlinkHadmClientServeiceTest:onSetSoundingEnableDisable001 end");
}

HWTEST_F(NearlinkHadmClientServeiceTest, onSoundingMeasureStateChange001, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientServeiceTest:onSoundingMeasureStateChange001 start");
    NearlinkHadmClientServer hadmClientServer;

    HadmClientService::impl hadmClientServiceImpl;
    HadmClientService::impl::StackAdapterCallback stackAdapterCallback(hadmClientServiceImpl);
    NearlinkHadmStackAdapter nearlinkHadmStackAdapterTest(stackAdapterCallback);

    HadmSoundingStateInfo_S stateInfoTest = {0, 0, 1};
    nearlinkHadmStackAdapterTest.onSoundingMeasureStateChange(&stateInfoTest);
    sleep(2);
    HILOGI("NearlinkHadmClientServeiceTest:onSoundingMeasureStateChange001 end");
}

HWTEST_F(NearlinkHadmClientServeiceTest, onReportSoundingIQResult001, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientServeiceTest:onReportSoundingIQResult001 start");
    NearlinkHadmClientServer hadmClientServer;

    HadmClientService::impl hadmClientServiceImpl;
    HadmClientService::impl::StackAdapterCallback stackAdapterCallback(hadmClientServiceImpl);
    NearlinkHadmStackAdapter nearlinkHadmStackAdapterTest(stackAdapterCallback);

    SLE_Addr_S addrTest;
    addrTest.type = 1;
    addrTest.addr[0] = 0x01;
    addrTest.addr[1] = 0x02;
    addrTest.addr[2] = 0x03;
    addrTest.addr[3] = 0x04;
    addrTest.addr[4] = 0x05;
    addrTest.addr[5] = 0x06;

    HadmReportIqData_S iqDataTest = {111, 222, 333, 444};
    HadmSoundingIqData_S soundingData;
    soundingData.dutRssi = 203;
    soundingData.rtdRssi = 201;
    soundingData.dutTof = 1266;
    soundingData.rtdTof = 1491;
    soundingData.timeStampSn = 390720;
    soundingData.iqChnlNum = 79;
    soundingData.iqData = &iqDataTest;

    nearlinkHadmStackAdapterTest.onReportSoundingIQResult(&addrTest, &soundingData);
    sleep(2);
    HILOGI("NearlinkHadmClientServeiceTest:onReportSoundingIQResult001 end");
}

HWTEST_F(NearlinkHadmClientServeiceTest, OnAcbStateChanged001, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientServeiceTest:OnAcbStateChanged001 start");
    HadmClientService::impl hadmClientServiceImpl;
    HadmClientService::impl::SleConnectionCallback sleConnectionCallback(hadmClientServiceImpl);
    RawAddress device("00:11:22:33:44:55");
    int state = static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED);
    int reason = 0x01;
    sleConnectionCallback.OnAcbStateChanged(device, state, reason);
    sleep(2);

    state = static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED);
    SleConfig::GetInstance().SetPeerAppearance(device.GetAddress(), DEVICE_CLASS_VEHICLE_LOCK);
    sleConnectionCallback.OnAcbStateChanged(device, state, reason);
    sleep(2);
    HILOGI("NearlinkHadmClientServeiceTest:OnAcbStateChanged001 end");
}

HWTEST_F(NearlinkHadmClientServeiceTest, OnConnectionUpdate001, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientServeiceTest:OnConnectionUpdate001 start");
    HadmClientService::impl hadmClientServiceImpl;
    HadmClientService::impl::SleConnectionUpdateCallback sleConnectionUpdateCallback(hadmClientServiceImpl);

    uint16_t connHandle = 0x01;
    uint16_t minInterval = 0x0032;
    uint16_t maxInterval = 0x0064;
    uint16_t maxLatency = 0;
    sleConnectionUpdateCallback.OnConnectionUpdate(connHandle, minInterval, maxInterval, maxLatency);
    sleep(2);

    minInterval = 0x321;
    sleConnectionUpdateCallback.OnConnectionUpdate(connHandle, minInterval, maxInterval, maxLatency);
    sleep(2);
    HILOGI("NearlinkHadmClientServeiceTest:OnConnectionUpdate001 end");
}

HWTEST_F(NearlinkHadmClientServeiceTest, SaveDutDataAndSaveRtdData001, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientServeiceTest:SaveDutDataAndSaveRtdData001 start");
    NearlinkHadmSoundingResult nearlinkHadmSoundingResult;
    nearlinkHadmSoundingResult.addr_ = RawAddress("01:02:03:04:05:06");
    nearlinkHadmSoundingResult.dutRssi_ = 203;
    nearlinkHadmSoundingResult.rtdRssi_ = 201;
    nearlinkHadmSoundingResult.dutTof_ = 1266;
    nearlinkHadmSoundingResult.rtdTof_ = 1491;
    nearlinkHadmSoundingResult.localNvOffset_ = 373;
    nearlinkHadmSoundingResult.remoteNvOffset_ = 373;
    nearlinkHadmSoundingResult.timeStampSn_ = 390720;
    nearlinkHadmSoundingResult.localTofOffset_ = 692;
    nearlinkHadmSoundingResult.remoteTofOffset_ = 688;
    nearlinkHadmSoundingResult.dutIData_ = {
        157, 103, 2019, 66, 49, 2030, 6, 45, 85, 1977, 85, 166, 1979, 1940, 2047, 112, 1978, 1854, 177, 28, 254, 275, 2037, 1715, 257, 1934, 1722, 1950, 111, 166, 0, 13, 1922, 119, 29, 1938, 84, 6, 20, 2046, 1980, 106, 2007, 1930, 1962, 2044, 0, 1921, 2009, 85, 161, 1977, 90, 1901, 123, 62, 1822, 83, 116, 162, 1984, 112, 2047, 12, 1965, 109, 63, 1965, 1940, 2036, 27, 1966, 1969, 67, 124, 6, 88, 1983, 0
    };
    nearlinkHadmSoundingResult.rtdIData_ = {
        1913, 1940, 1993, 14, 2025, 40, 28, 4, 2006, 94, 127, 122, 1945, 54, 0, 89, 135, 146, 1920, 70, 2030, 1952, 1852, 1772, 307, 273, 183, 128, 2038, 1985, 0, 113, 1971, 23, 2010, 1972, 72, 46, 2032, 2029, 1989, 76, 59, 15, 2007, 1965, 0, 82, 39, 2025, 1968, 117, 122, 1962, 35, 26, 1962, 118, 1916, 1930, 70, 40, 0, 1989, 1974, 74, 62, 45, 17, 2028, 2004, 60, 61, 46, 24, 2041, 29, 1998, 0
    };
    nearlinkHadmSoundingResult.dutQData_ = {
        61, 1955, 1955, 29, 2006, 43, 48, 41, 1, 106, 131, 2025, 165, 1917, 2047, 1901, 1868, 1995, 1903, 252, 152, 1891, 1712, 1888, 1779, 1707, 2044, 244, 180, 8, 0, 145, 1991, 1996, 1919, 1968, 1991, 1987, 31, 2024, 1988, 1968, 1912, 2030, 68, 115, 2047, 39, 129, 116, 2034, 154, 132, 4, 1925, 196, 2041, 1851, 142, 2032, 151, 135, 0, 1910, 1974, 2039, 1937, 1943, 39, 82, 52, 2018, 87, 111, 2016, 1932, 37, 49, 0
    };
    nearlinkHadmSoundingResult.rtdQData_ = {
        2014, 1997, 1982, 62, 44, 2035, 25, 50, 53, 2007, 2038, 35, 1961, 116, 0, 1942, 1995, 33, 1931, 178, 217, 222, 172, 94, 30, 81, 179, 178, 160, 118, 0, 27, 1970, 99, 97, 75, 2011, 6, 2021, 2, 2008, 71, 90, 91, 72, 38, 0, 59, 93, 106, 92, 1998, 0, 1981, 129, 1890, 155, 1942, 36, 2014, 101, 121, 0, 83, 30, 24, 67, 80, 79, 62, 17, 30, 66, 88, 92, 87, 1982, 37, 0
    };
    HadmClientService::GetInstance().SaveDutData(nearlinkHadmSoundingResult);
    HadmClientService::GetInstance().SaveRtdData(nearlinkHadmSoundingResult);
    sleep(2);
    HILOGI("NearlinkHadmClientServeiceTest:SaveDutDataAndSaveRtdData001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS