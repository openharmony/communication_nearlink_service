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

#include "log.h"
#include "sle_service_data.cpp"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Nearlink {
namespace TEST {
class SleServiceDataTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        HILOGI("SetUpTestCase start");
    }

    static void TearDownTestCase()
    {
        HILOGI("SetUpTestCase start");
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

/**
 * @tc.name: TestSetTxPower001
 * @tc.desc: 测试设置发射功率函数，覆盖所有分支
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, TestSetTxPower001, TestSize.Level1) {
    SleAdvertiserSettingsImpl advertiser;
    // 1. 测试无效参数（返回 RET_BAD_PARAM）
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_INVAILD)),
              static_cast<int>(ReturnValue::RET_BAD_PARAM));
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_ULTRA_LOW) - 1),
              static_cast<int>(ReturnValue::RET_BAD_PARAM));
    // 2. 测试各有效功率级别（返回 RET_NO_ERROR）
    // SLE_ADV_TX_POWER_ULTRA_LOW
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_ULTRA_LOW)),
              static_cast<int>(ReturnValue::RET_NO_ERROR));
    EXPECT_EQ(advertiser.txPower_, static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_ULTRA_LOW_VALUE));
    // SLE_ADV_TX_POWER_LOW
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_LOW)),
              static_cast<int>(ReturnValue::RET_NO_ERROR));
    EXPECT_EQ(advertiser.txPower_, static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_LOW_VALUE));
    // SLE_ADV_TX_POWER_MEDIUM
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_MEDIUM)),
              static_cast<int>(ReturnValue::RET_NO_ERROR));
    EXPECT_EQ(advertiser.txPower_, static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_MEDIUM_VALUE));
    // SLE_ADV_TX_POWER_HIGH
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_HIGH)),
              static_cast<int>(ReturnValue::RET_NO_ERROR));
    EXPECT_EQ(advertiser.txPower_, static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_HIGH_VALUE));
    // SLE_ADV_TX_POWER_ULTRA_HIGH
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_ULTRA_HIGH)),
              static_cast<int>(ReturnValue::RET_NO_ERROR));
    EXPECT_EQ(advertiser.txPower_, static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_ULTRA_HIGH_VALUE));
    // SLE_ADV_TX_POWER_FIND
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_FIND)),
              static_cast<int>(ReturnValue::RET_NO_ERROR));
    EXPECT_EQ(advertiser.txPower_, static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_FIND_VALUE));
}

/**
 * @tc.name: SetGetInfo001
 * @tc.desc: Verify SlePeripheralDevice function.
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, SetGetInfo001, TestSize.Level1)
{
    HILOGI("SetGetInfo001 start");
    std::shared_ptr<SlePeripheralDevice> dev = std::make_shared<SlePeripheralDevice>();
    RawAddress address = RawAddress("00:00:00:00:00:00");
    int index = 0;
    string subModelId = "12345";
    uint8_t cryptoAlgo = 1;
    uint8_t KeyDerivAlgo = 2;
    uint8_t integrChkInd = 3;
    std::string encryptGroupKeyStr = "123456789";
    uint64_t giv = 4;
    bool isUserDisconnected = true;
    std::vector<std::string> cdsmDevList;
    std::vector<std::string> cdsmDevList1;
    cdsmDevList.push_back("12345");
    dev->SetCurrentRawAddress(address);
    dev->GetServiceData(index);
    dev->GetServiceDataUUID();
    dev->GetServiceUUID(index);
    EXPECT_EQ(address, dev->GetCurrentRawAddress());
    dev->SetCollaborateAddress(address);
    EXPECT_EQ(address, dev->GetCollaborateAddress());
    dev->SetSubModelId(subModelId);
    EXPECT_EQ(subModelId, dev->GetSubModelId());
    dev->SetCryptoAlgo(cryptoAlgo);
    EXPECT_EQ(cryptoAlgo, dev->GetCryptoAlgo());
    dev->SetKeyDerivAlgo(KeyDerivAlgo);
    EXPECT_EQ(KeyDerivAlgo, dev->GetKeyDerivAlgo());
    dev->SetIntegrChkInd(integrChkInd);
    EXPECT_EQ(integrChkInd, dev->GetIntegrChkInd());
    dev->SetEncryptGroupKeyStr(encryptGroupKeyStr);
    EXPECT_EQ(encryptGroupKeyStr, dev->GetEncryptGroupKeyStr());
    dev->SetGiv(giv);
    EXPECT_EQ(giv, dev->GetGiv());
    dev->SetIsUserDisconnected(isUserDisconnected);
    EXPECT_EQ(isUserDisconnected, dev->GetIsUserDisconnected());
    dev->SaveCdsmDeviceList(cdsmDevList);
    dev->GetCdsmDeviceList(cdsmDevList1);
    HILOGI("SetGetInfo001 end");
}
}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS