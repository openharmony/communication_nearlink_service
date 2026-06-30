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

#include "scanfilter_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "log.h"
#include "securec.h"
#include "nearlink_scan_def.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
{
    HILOGI("start");
    FuzzedDataProvider provider(data, size);

    ScanFilter scanFilter;
    scanFilter.SetDeviceId(BuildAddressString(provider));
    scanFilter.GetDeviceId();
    scanFilter.SetName(BuildAddressString(provider));
    scanFilter.GetName();
    scanFilter.SetServiceUuid(BuildUuid(provider));
    scanFilter.HasServiceUuid();
    scanFilter.GetServiceUuid();
    scanFilter.SetServiceUuidMask(BuildUuid(provider));
    scanFilter.HasServiceUuidMask();
    scanFilter.GetServiceUuidMask();
    scanFilter.SetServiceSolicitationUuid(BuildUuid(provider));
    scanFilter.HasSolicitationUuid();
    scanFilter.GetServiceSolicitationUuid();
    scanFilter.SetServiceSolicitationUuidMask(BuildUuid(provider));
    scanFilter.HasSolicitationUuidMask();
    scanFilter.GetServiceSolicitationUuidMask();
    uint8_t vecLen = provider.ConsumeIntegral<uint8_t>();
    scanFilter.SetServiceData(provider.ConsumeBytes<uint8_t>(vecLen));
    scanFilter.GetServiceData();
    scanFilter.SetServiceDataMask(provider.ConsumeBytes<uint8_t>(vecLen));
    scanFilter.GetServiceDataMask();
    scanFilter.SetManufacturerId(provider.ConsumeIntegral<uint16_t>());
    scanFilter.GetManufacturerId();
    scanFilter.SetManufactureData(provider.ConsumeBytes<uint8_t>(vecLen));
    scanFilter.GetManufactureData();
    scanFilter.SetManufactureDataMask(provider.ConsumeBytes<uint8_t>(vecLen));
    scanFilter.GetManufactureDataMask();
    
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

