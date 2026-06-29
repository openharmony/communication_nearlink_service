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

#include "sleadvertisersettingsimpl_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "log.h"
#include "securec.h"
#include "sle_service_data.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
{
    HILOGI("start");
    FuzzedDataProvider provider(data, size);
    SleAdvertiserSettingsImpl sleAdvertiserSettingsImpl;
    sleAdvertiserSettingsImpl.IsConnectable();
    sleAdvertiserSettingsImpl.SetConnectable(true);
    sleAdvertiserSettingsImpl.IsLegacyMode();
    sleAdvertiserSettingsImpl.SetLegacyMode(true);
    sleAdvertiserSettingsImpl.GetInterval();
    sleAdvertiserSettingsImpl.SetInterval();
    sleAdvertiserSettingsImpl.GetTxPower();
    sleAdvertiserSettingsImpl.GetLinkRole();
    sleAdvertiserSettingsImpl.SetTxPower(provider.ConsumeIntegral<int8_t>());
    sleAdvertiserSettingsImpl.GetPrimaryPhy();
    sleAdvertiserSettingsImpl.SetPrimaryPhy(provider.ConsumeIntegral<int8_t>());
    sleAdvertiserSettingsImpl.GetSecondaryPhy();
    sleAdvertiserSettingsImpl.SetSecondaryPhy(provider.ConsumeIntegral<int8_t>());
    std::array<uint8_t, RawAddress::SLE_ADDRESS_BYTE_LEN> ownAddr = sleAdvertiserSettingsImpl.GetOwnAddr();
    sleAdvertiserSettingsImpl.SetOwnAddr(ownAddr);
    sleAdvertiserSettingsImpl.GetOwnAddrType();
    sleAdvertiserSettingsImpl.SetOwnAddrType(provider.ConsumeIntegral<int8_t>());
    sleAdvertiserSettingsImpl.SetLinkRole(provider.ConsumeIntegral<int8_t>());
    
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

