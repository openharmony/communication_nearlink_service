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

#include "sleperipheraldevice_fuzzer.h"
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
    RawAddress rawAddress(BuildAddressString(provider));
    uint8_t value = provider.ConsumeIntegral<uint8_t>();
    uint8_t *payload = &value;

    SlePeripheralDevice slePeripheralDevice;
    slePeripheralDevice.GetRawAddress();
    slePeripheralDevice.GetAppearance();
    slePeripheralDevice.GetManufacturerData();
    slePeripheralDevice.GetName();
    slePeripheralDevice.GetRSSI();
    slePeripheralDevice.GetServiceData();
    slePeripheralDevice.GetServiceData();
    slePeripheralDevice.GetServiceDataUUID();
    slePeripheralDevice.GetServiceDataUUID();
    slePeripheralDevice.GetServiceUUID();
    slePeripheralDevice.GetServiceUUID(provider.ConsumeIntegral<uint8_t>());
    slePeripheralDevice.GetPayload();
    slePeripheralDevice.GetPayloadLen();
    slePeripheralDevice.GetAddressType();
    slePeripheralDevice.SetAddressType(provider.ConsumeIntegral<uint8_t>());
    slePeripheralDevice.IsManufacturerData();
    slePeripheralDevice.IsRSSI();
    slePeripheralDevice.IsServiceData();
    slePeripheralDevice.IsServiceUUID();
    slePeripheralDevice.IsName();
    slePeripheralDevice.SetAddress(rawAddress);
    slePeripheralDevice.SetRSSI(provider.ConsumeIntegral<int8_t>());
    slePeripheralDevice.IsConnectable();
    slePeripheralDevice.SetConnectable(true);
    struct SlePeripheralDeviceParseAdvData parseAdvData{payload, 0};
    slePeripheralDevice.ParseSleServiceData();
    slePeripheralDevice.SetServiceUUID16Bits(parseAdvData);
    slePeripheralDevice.SetServiceUUID128Bits(parseAdvData);
    slePeripheralDevice.SetServiceDataUUID16Bits(parseAdvData);
    slePeripheralDevice.SetServiceDataUUID128Bits(parseAdvData);
    slePeripheralDevice.SetName(BuildAddressString(provider));
    slePeripheralDevice.SetAppearance(provider.ConsumeIntegral<int>());
    slePeripheralDevice.SetRoles(provider.ConsumeIntegral<uint8_t>());
    slePeripheralDevice.GetLinkRole();
    slePeripheralDevice.SetBondedFromLocal(provider.ConsumeBool());
    slePeripheralDevice.SetAcbConnectState(provider.ConsumeIntegral<int>());
    slePeripheralDevice.GetAcbConnectState();
    slePeripheralDevice.SetLcid(provider.ConsumeIntegral<uint16_t>());
    slePeripheralDevice.SetLocalIndex(provider.ConsumeIntegral<uint16_t>());
    slePeripheralDevice.SetConnDirect(provider.ConsumeIntegral<int>());
    slePeripheralDevice.GetConnDirect();
    slePeripheralDevice.IsAcbConnected();
    slePeripheralDevice.IsAcbEncrypted();
    slePeripheralDevice.IsBondedFromLocal();
    slePeripheralDevice.GetLcid();
    slePeripheralDevice.GetLocalIndex();
    slePeripheralDevice.GetAdFlag();
    slePeripheralDevice.GetPairedStatus();
    slePeripheralDevice.GetPrePairedStatus();
    slePeripheralDevice.SetPairedStatus(provider.ConsumeIntegral<uint8_t>());
    slePeripheralDevice.SetPrePairedStatus(provider.ConsumeIntegral<uint8_t>());
    slePeripheralDevice.SetAliasName(BuildAddressString(provider));
    slePeripheralDevice.GetAliasName();
    slePeripheralDevice.SetIoCapability(provider.ConsumeIntegral<uint8_t>());
    slePeripheralDevice.GetIoCapability();
    slePeripheralDevice.SetManufacturerData(provider.ConsumeIntegral<uint16_t>(), BuildAddressString(provider));

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

