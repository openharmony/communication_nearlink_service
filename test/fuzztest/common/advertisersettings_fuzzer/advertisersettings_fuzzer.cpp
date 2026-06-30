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

#include "advertisersettings_fuzzer.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "log.h"
#include "securec.h"
#include "nearlink_advertiser_def.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
{
    HILOGI("start");
    FuzzedDataProvider provider(data, size);

    AdvertiserSettings advertiserSettings;
    advertiserSettings.IsConnectable();
    advertiserSettings.IsLegacyMode();
    advertiserSettings.GetInterval();
    advertiserSettings.GetTxPower();
    advertiserSettings.GetLinkRole();
    advertiserSettings.SetConnectable(true);
    advertiserSettings.SetLegacyMode(true);
    advertiserSettings.SetInterval(provider.ConsumeIntegral<uint16_t>());
    advertiserSettings.SetTxPower(provider.ConsumeIntegral<uint8_t>());
    advertiserSettings.GetPrimaryPhy();
    advertiserSettings.SetPrimaryPhy(provider.ConsumeIntegral<int>());
    advertiserSettings.GetSecondaryPhy();
    advertiserSettings.SetSecondaryPhy(provider.ConsumeIntegral<int>());
    std::array<uint8_t, RawAddress::SLE_ADDRESS_BYTE_LEN> addrArray = advertiserSettings.GetOwnAddr();
    advertiserSettings.SetOwnAddr(addrArray);
    advertiserSettings.GetOwnAddrType();
    advertiserSettings.SetOwnAddrType(provider.ConsumeIntegral<int8_t>());
    advertiserSettings.SetLinkRole(provider.ConsumeIntegral<uint8_t>());
    return true;
}
}

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        HILOGI("invalid data");
        return 0;
    }

    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}

