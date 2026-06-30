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

#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "nearlink_hadm_sounding_result.h"
#include "nearlink_hadm_client_callback_stub.h"
#include "nearlink_sle_ranging.cpp"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class MockSleRangingCallback : public SleRangingCallback {
public:
    void OnSleRangingResult(const RangingResult &result) override {
        HILOGI("MockSleRangingCallback OnSleRangingResult");
    }

    void OnSleRangingStateChange(const RangingState &state) override {
        HILOGI("MockSleRangingCallback OnSleRangingStateChange");
    }
};

namespace {
constexpr int TDD_DELAY_50_MS = 50;

std::shared_ptr<MockSleRangingCallback> callback = std::make_shared<MockSleRangingCallback>();
std::shared_ptr<NearlinkSleRanging> nearlinkSleRanging = NearlinkSleRanging::CreateNearlinkSleRanging(callback);

sptr<NearlinkSleRanging::impl::HadmClientCallbackStubImpl> g_hadmClientCallback = new (std::nothrow) NearlinkSleRanging::impl::HadmClientCallbackStubImpl(nearlinkSleRanging);
}

class NearlinkHadmClientCallbackStubTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkHadmClientCallbackStubTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkHadmClientCallbackStubTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
}

void NearlinkHadmClientCallbackStubTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkHadmClientCallbackStubTest");
}

void NearlinkHadmClientCallbackStubTest::SetUp()
{
    HILOGI("SetUp NearlinkHadmClientCallbackStubTest.");
}

void NearlinkHadmClientCallbackStubTest::TearDown()
{
    HILOGI("TearDown NearlinkHadmClientCallbackStubTest.");
}

int32_t HadmClientCallbackOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    if (!g_hadmClientCallback) {
        std::shared_ptr<MockSleRangingCallback> callback = std::make_shared<MockSleRangingCallback>();
        std::shared_ptr<NearlinkSleRanging> nearlinkSleRanging = NearlinkSleRanging::CreateNearlinkSleRanging(callback);
        g_hadmClientCallback = new (std::nothrow) NearlinkSleRanging::impl::HadmClientCallbackStubImpl(nearlinkSleRanging);
    }

    NL_CHECK_RETURN_RET(g_hadmClientCallback, TRANSACTION_ERR, "g_hadmClientCallback is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("HadmClientCallbackOnRemoteRequest, cmd(%{public}d)", code);
    return g_hadmClientCallback->OnRemoteRequest(code, data, reply, option);
}

HWTEST_F(NearlinkHadmClientCallbackStubTest, OnSoundingResult001, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientCallbackStubTest:OnSoundingResult001 start");
    NearlinkRawAddress addr; 
    addr.SetAddress("00:11:22:33:44:55");
    NearlinkHadmSoundingResult result;
    result.addr_ = RawAddress("00:11:22:33:44:55");
    result.dutRssi_ = 203;
    result.rtdRssi_ = 201;
    result.dutTof_ = 1266;
    result.rtdTof_ = 1491;
    result.localNvOffset_ = 373;
    result.remoteNvOffset_ = 373;
    result.timeStampSn_ = 390720;
    result.localTofOffset_ = 692;
    result.remoteTofOffset_ = 688;
    result.dutIData_ = {
        157, 103, 2019, 66, 49, 2030, 6, 45, 85, 1977, 85, 166, 1979, 1940, 2047, 112, 1978, 1854, 177, 28, 254, 275, 2037, 1715, 257, 1934, 1722, 1950, 111, 166, 0, 13, 1922, 119, 29, 1938, 84, 6, 20, 2046, 1980, 106, 2007, 1930, 1962, 2044, 0, 1921, 2009, 85, 161, 1977, 90, 1901, 123, 62, 1822, 83, 116, 162, 1984, 112, 2047, 12, 1965, 109, 63, 1965, 1940, 2036, 27, 1966, 1969, 67, 124, 6, 88, 1983, 0
    };
    result.rtdIData_ = {
        1913, 1940, 1993, 14, 2025, 40, 28, 4, 2006, 94, 127, 122, 1945, 54, 0, 89, 135, 146, 1920, 70, 2030, 1952, 1852, 1772, 307, 273, 183, 128, 2038, 1985, 0, 113, 1971, 23, 2010, 1972, 72, 46, 2032, 2029, 1989, 76, 59, 15, 2007, 1965, 0, 82, 39, 2025, 1968, 117, 122, 1962, 35, 26, 1962, 118, 1916, 1930, 70, 40, 0, 1989, 1974, 74, 62, 45, 17, 2028, 2004, 60, 61, 46, 24, 2041, 29, 1998, 0
    };
    result.dutQData_ = {
        61, 1955, 1955, 29, 2006, 43, 48, 41, 1, 106, 131, 2025, 165, 1917, 2047, 1901, 1868, 1995, 1903, 252, 152, 1891, 1712, 1888, 1779, 1707, 2044, 244, 180, 8, 0, 145, 1991, 1996, 1919, 1968, 1991, 1987, 31, 2024, 1988, 1968, 1912, 2030, 68, 115, 2047, 39, 129, 116, 2034, 154, 132, 4, 1925, 196, 2041, 1851, 142, 2032, 151, 135, 0, 1910, 1974, 2039, 1937, 1943, 39, 82, 52, 2018, 87, 111, 2016, 1932, 37, 49, 0
    };
    result.rtdQData_ = {
        2014, 1997, 1982, 62, 44, 2035, 25, 50, 53, 2007, 2038, 35, 1961, 116, 0, 1942, 1995, 33, 1931, 178, 217, 222, 172, 94, 30, 81, 179, 178, 160, 118, 0, 27, 1970, 99, 97, 75, 2011, 6, 2021, 2, 2008, 71, 90, 91, 72, 38, 0, 59, 93, 106, 92, 1998, 0, 1981, 129, 1890, 155, 1942, 36, 2014, 101, 121, 0, 83, 30, 24, 67, 80, 79, 62, 17, 30, 66, 88, 92, 87, 1982, 37, 0
    };
    NearlinkHadmClientSoundingResult soundingResult(result);
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHadmClientCallbackStub::GetDescriptor());
    data.WriteParcelable(&addr);
    data.WriteParcelable(&soundingResult);
    int32_t ret = HadmClientCallbackOnRemoteRequest(NearlinkHadmClientCallbackInterfaceCode::NL_HADM_CLIENT_CALLBACK_RESULT_EVENT, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::TDD_DELAY_50_MS));
    HILOGI("NearlinkHadmClientCallbackStubTest:OnSoundingResult001 end");
}

HWTEST_F(NearlinkHadmClientCallbackStubTest, OnSoundingStateChange001, TestSize.Level1)
{
    HILOGI("NearlinkHadmClientCallbackStubTest:OnSoundingStateChange001 start");
    NearlinkRawAddress addr; 
    addr.SetAddress("00:11:22:33:44:55");
    int newState = 1;
    int errorCode = 0;
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHadmClientCallbackStub::GetDescriptor());
    data.WriteParcelable(&addr);
    data.WriteInt32(newState);
    data.WriteInt32(errorCode);
    int32_t result = HadmClientCallbackOnRemoteRequest(NearlinkHadmClientCallbackInterfaceCode::NL_HADM_CLIENT_CALLBACK_STATE_EVENT, data, reply);
    EXPECT_EQ(NO_ERROR, result);
    HILOGI("NearlinkHadmClientCallbackStubTest:OnSoundingStateChange001 end");
} 

}// namespace TEST
} // namespace Nearlink
} // namespace OHOS