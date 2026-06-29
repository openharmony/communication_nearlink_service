/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "smdisp_fuzzer.h"
#include "sm_algos.h"
#include "nai_log.h"
#include "securec.h"
#include "nai_log.h"
#include "sdf_mem.h"
#include "datatype.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"
#include "cm_api.h"
#include "sm_auth.h"
#include "sm_encp.h"
#include "sm_nego.h"
#include "sm_noentry.h"
#include "sm_slink.h"
#include "sm_stm.h"
#include "sm.h"
#include "sdf_map.h"


typedef enum {
    FUZZ_0      = 0,
    FUZZ_1      = 1,
    FUZZ_2      = 2,
    FUZZ_3      = 3,
    FUZZ_4      = 4,
    FUZZ_256    = 256,
    FUZZ_307    = 307,
    FUZZ_308    = 308,
    FUZZ_309    = 309,
    FUZZ_310    = 310,
    FUZZ_311    = 311,
    FUZZ_312    = 312,
    FUZZ_313    = 313,
    FUZZ_314    = 314,
    FUZZ_321    = 321,
    FUZZ_322    = 322,
    FUZZ_NUM_END
} FUZZ_NUM_E;

extern SDF_Map *g_slinkMap;

namespace OHOS {

    uint16_t get_random_opcode()
    {
        uint16_t opcodes[] = {0x0133, 0x0134, 0x0135};
        int num_opcodes = sizeof(opcodes) / sizeof(opcodes[0]);

        int random_index = rand() % num_opcodes;

        return opcodes[random_index];
    }

    void FuzzSMPkgDispatcher(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S g_gNodeAddr = { .type = PUBLIC_ADDRESS, .addr = { 0x18, 0xda, 0x8f, 0x00, 0x01, 0x00, }, };

        uint8_t *msg = (uint8_t*)malloc(size);

        memcpy_s(msg, size, fuzzData, size);

        int opcode1 = get_random_opcode();

        SmPkgDispatcher(opcode1, &g_gNodeAddr, msg, size);

        free(msg);
    }

    void FuzzSMApi(const uint8_t *fuzzData, size_t size)
    {
        g_slinkMap = SDF_MapCtor(SLE_ADDR_TRAITS(), MAKE_TRAITS(SmSLinkDtor, NULL));
        FuzzSMPkgDispatcher(fuzzData, size);
        if (g_slinkMap) {
            SDF_MapDtor(g_slinkMap);
            g_slinkMap = NULL;
        }
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    NLSTK_InitStack();
    NLSTK_EnableStack();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    uint8_t *fuzzData = (uint8_t*)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzSMApi(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}