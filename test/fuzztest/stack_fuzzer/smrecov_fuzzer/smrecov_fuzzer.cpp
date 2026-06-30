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

#include "smrecov_fuzzer.h"
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

extern SDF_Map *g_slinkMap;

typedef struct SlePkt {
    uint8_t *data;
    uint32_t size;
} SlePkt;

typedef enum {
    FUZZ_ZERO    = 0,
    FUZZ_ONE     = 1,
    FUZZ_TWO     = 2,
    FUZZ_THREE   = 3,
    FUZZ_FOUR    = 4,
    FUZZ_FIVE    = 5,
    FUZZ_SIX     = 6,
    FUZZ_SEVEN   = 7,
    FUZZ_EIGHT   = 8,
    FUZZ_ONEHUNDREDTWENTYEIGHT = 128,
    FUZZ_TWOHUNDREDFIFTYSIX    = 256,
    FUZZ_NUM_END
} FUZZ_NUM_E;

int SleSendDliPacket(const SlePkt *packet)
{
    return 0;
}

namespace OHOS {

    void FuzzUapiRecoverKey(const uint8_t* fuzzData, size_t size)
    {
        if (size < (sizeof(NLSTK_SmRecoverKeyParam_S))) {
            return;
        }

        NLSTK_SmRecoverKeyParam_S *recover_key_in =
            (NLSTK_SmRecoverKeyParam_S *)malloc(sizeof(NLSTK_SmRecoverKeyParam_S));
        if (recover_key_in == NULL) {
            NAI_LOG_ERROR("malloc failed");
            return;
        }

        int ii = 0;

        recover_key_in->addr.type = fuzzData[ii++];
        for (int i = 0; i < SLE_ADDR_LEN; i ++) {
            recover_key_in->addr.addr[i] = fuzzData[ii++];
        }

        for (int i = 0; i < SM_LINK_KEY_LEN; i ++) {
            recover_key_in->linkKey[i] = fuzzData[ii++];
        }

        recover_key_in->cryptoAlgo = fuzzData[ii++];
        recover_key_in->keyDerivAlgo = fuzzData[ii++];
        recover_key_in->intgChkInd = fuzzData[ii++];

        NLSTK_SmRecoverKey(recover_key_in, 1);
        free(recover_key_in);
    }

    void FuzzSMApi(const uint8_t *fuzzData, size_t size)
    {
        FuzzUapiRecoverKey(fuzzData, size);
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