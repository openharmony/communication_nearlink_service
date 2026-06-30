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

#include "slescansettingsimpl_fuzzer.h"
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

    SleScanSettingsImpl sleScanSettingsImpl;
    sleScanSettingsImpl.SetReportDelay();
    sleScanSettingsImpl.GetReportDelayMillisValue();
    sleScanSettingsImpl.SetScanInterval(provider.ConsumeIntegral<uint16_t>());
    sleScanSettingsImpl.GetScanInterval();
    sleScanSettingsImpl.SetScanWindow(provider.ConsumeIntegral<uint16_t>());
    sleScanSettingsImpl.GetScanWindow();
    sleScanSettingsImpl.SetLegacy(provider.ConsumeBool());
    sleScanSettingsImpl.GetLegacy();
    sleScanSettingsImpl.SetPhy(provider.ConsumeIntegral<int>());
    sleScanSettingsImpl.GetPhy();

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

