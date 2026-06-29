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

#include "slescanfilterimpl_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "log.h"
#include "securec.h"
#include "SleMultiScanData.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
{
    HILOGI("start");
    FuzzedDataProvider provider(data, size);
    uint8_t vecLen = provider.ConsumeIntegral<uint8_t>();

    SleScanFilterImpl sleScanFilterImpl;
    sleScanFilterImpl.SetDeviceId(BuildAddressString(provider));
    sleScanFilterImpl.GetDeviceId();
    sleScanFilterImpl.SetName(BuildAddressString(provider));
    sleScanFilterImpl.GetName();
    sleScanFilterImpl.SetServiceUuid(BuildUuid(provider));
    sleScanFilterImpl.HasServiceUuid();
    sleScanFilterImpl.GetServiceUuid();
    sleScanFilterImpl.SetServiceUuidMask(BuildUuid(provider));
    sleScanFilterImpl.HasServiceUuidMask();
    sleScanFilterImpl.GetServiceUuidMask();
    sleScanFilterImpl.SetServiceSolicitationUuid(BuildUuid(provider));
    sleScanFilterImpl.HasSolicitationUuid();
    sleScanFilterImpl.GetServiceSolicitationUuid();
    sleScanFilterImpl.SetServiceSolicitationUuidMask(BuildUuid(provider));
    sleScanFilterImpl.HasSolicitationUuidMask();
    sleScanFilterImpl.GetServiceSolicitationUuidMask();
    sleScanFilterImpl.SetServiceData(provider.ConsumeBytes<uint8_t>(vecLen));
    sleScanFilterImpl.GetServiceData();
    sleScanFilterImpl.SetServiceDataMask(provider.ConsumeBytes<uint8_t>(vecLen));
    sleScanFilterImpl.GetServiceDataMask();
    sleScanFilterImpl.SetManufacturerId(provider.ConsumeIntegral<uint16_t>());
    sleScanFilterImpl.GetManufacturerId();
    sleScanFilterImpl.SetManufactureData(provider.ConsumeBytes<uint8_t>(vecLen));
    sleScanFilterImpl.GetManufactureData();
    sleScanFilterImpl.SetManufactureDataMask(provider.ConsumeBytes<uint8_t>(vecLen));
    sleScanFilterImpl.GetManufactureDataMask();
    sleScanFilterImpl.SetClientId(provider.ConsumeIntegral<int>());
    sleScanFilterImpl.GetClientId();
    sleScanFilterImpl.SetFiltIndex(provider.ConsumeIntegral<uint8_t>());
    sleScanFilterImpl.GetFiltIndex();
    sleScanFilterImpl.SetFilterAction(provider.ConsumeIntegral<uint8_t>());
    sleScanFilterImpl.GetFilterAction();
    
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

