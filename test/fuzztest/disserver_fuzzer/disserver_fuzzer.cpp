/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "disserver_fuzzer.h"
#include <thread>
#include "../nl_utils/fuzztest_utils.h"
#include "DisServerStackAdapter.h"
#include "log.h"
#include "slem.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
constexpr int DIS_SERVER_PROFILE_FUZZ_DELAY_MS = 5000;
}

void DoSomethingDisServerWithMyAPI(const uint8_t* fuzzData, size_t size)
{
    (void) fuzzData;
    (void) size;
    DisServerInfo info;
    info.appearanceId_ = DIS_DEVICE_SMARTPHONE;
    info.manufactureInfo_ = "manufactureInfo_";
    info.nameInfo_ = "nameInfo_";
    info.versionInfo_ = "0";
    DisServerStackAdapter disServer;
    disServer.CreateServerInfo(info);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::DIS_SERVER_PROFILE_FUZZ_DELAY_MS));
    disServer.DestroyServerInfo();
}
}

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    int ret = slem_initialize();
    HILOGI("DisServerFuzzTest slem_initialize %{public}d", ret);
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    (void) size;
    if (data == nullptr) {
        return 0;
    }
    OHOS::DoSomethingDisServerWithMyAPI(data, size);
    return 0;
}