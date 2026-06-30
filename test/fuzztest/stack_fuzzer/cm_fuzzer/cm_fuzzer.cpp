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
#include "cm_fuzzer.h"
#include "cm_link_fuzzer.h"
#include "cm_common_fuzzer.h"
#include "cm_concurrent_conn_fuzzer.h"
#include <cstdint>
#include "securec.h"

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    return FuzzCmLinkInit();
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzCmLinkInit();
    uint8_t *fuzzData = (uint8_t *)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    (void)memcpy_s(fuzzData, size, data, size);
    FuzzCmLinkApi(fuzzData, size);
    FuzzCmLinkDeInit();

    FuzzCmConcurrentConnInit();
    FuzzCmConcurrentConnApi(fuzzData, size);
    FuzzCmConcurrentConnDeInit();
    
    FuzzCmDliAdaptor(fuzzData, size);
    free(fuzzData);
    return 0;
}