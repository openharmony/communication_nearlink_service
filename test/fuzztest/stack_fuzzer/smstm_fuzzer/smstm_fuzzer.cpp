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

#include "securec.h"
#include "sle_crypto.h"
#include "sdf_mem.h"
#include "sdf_map.h"
#include "nlstk_init_api.h"
#include "nlstk_sm_api.h"
#include "sm_stm.h"
#include "sm_slink.h"
#include "sm_errcode.h"
#include "smstm_fuzzer.h"

extern SDF_Map *g_slinkMap;

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
    FUZZ_TWENTY  = 20,
    FUZZ_ONEHUNDREDTWENTYEIGHT = 128,
    FUZZ_TWOHUNDREDFIFTYSIX    = 256,
    FUZZ_NUM_END
} FUZZ_NUM_E;

namespace OHOS {

    char random_hex_byte()
    {
        const char* hex_digits = "0123456789abcdef";
        int random_byte = rand() % FUZZ_TWOHUNDREDFIFTYSIX;
        return hex_digits[random_byte];
    }

    void FuzzStmInitEncpParamReqReply(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_INIT]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ENCP_PARAM_REQ_REPLY });
    }

    void FuzzStmInitRemovePair(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_INIT]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_REMOVE_PAIR });
    }

    void FuzzStmNegoSuccess(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_NEGO]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_NEGO_SUCCESS });
    }

    void FuzzStmNegoTimeout(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_NEGO]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_TIMEOUT, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
    }

    void FuzzStmNegoRemovePair(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_NEGO]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_REMOVE_PAIR, .extData = (void *)(uintptr_t)SM_ERR_ACTIVE_CANCEL });
    }

    void FuzzStmNegoEncpParamReqReply(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_NEGO]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ENCP_PARAM_REQ_REPLY });
    }

    void FuzzStmAuthSuccess(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_AUTH]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_AUTH_SUCCESS });
    }

    void FuzzStmAuthTimeout(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_AUTH]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_TIMEOUT, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
    }

    void FuzzStmAuthRemovePair(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_AUTH]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_REMOVE_PAIR, .extData = (void *)(uintptr_t)SM_ERR_ACTIVE_CANCEL });
    }

    void FuzzStmEncpActiveStart(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_ENCP]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ACTIVE_START });
    }

    void FuzzStmEncpSuccess(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_ENCP]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ENCP_SUCCESS });
    }

    void FuzzStmEncpParamReqReply(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_ENCP]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ENCP_PARAM_REQ_REPLY });
    }

    void FuzzStmEncpInternalError(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_ENCP]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
    }

    void FuzzStmEncpExternalError(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_ENCP]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_EXTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_STATUS_KEY_MISSING });
    }

    void FuzzStmEncpRemovePair(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_ENCP]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_REMOVE_PAIR, .extData = (void *)(uintptr_t)SM_ERR_ACTIVE_CANCEL });
    }

    void FuzzStmFullDisconnect(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_FULL]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_LCHANNEL_DISCONN });
    }

    void FuzzStmFullRemovePair(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_FULL]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_REMOVE_PAIR });
    }

    void FuzzSMApi(const uint8_t *fuzzData, size_t size)
    {
        FuzzStmInitEncpParamReqReply(fuzzData, size);
        FuzzStmInitRemovePair(fuzzData, size);
        FuzzStmNegoSuccess(fuzzData, size);
        FuzzStmNegoTimeout(fuzzData, size);
        FuzzStmNegoRemovePair(fuzzData, size);
        FuzzStmNegoEncpParamReqReply(fuzzData, size);
        FuzzStmAuthSuccess(fuzzData, size);
        FuzzStmAuthTimeout(fuzzData, size);
        FuzzStmAuthRemovePair(fuzzData, size);
        FuzzStmEncpActiveStart(fuzzData, size);
        FuzzStmEncpSuccess(fuzzData, size);
        FuzzStmEncpParamReqReply(fuzzData, size);
        FuzzStmEncpInternalError(fuzzData, size);
        FuzzStmEncpExternalError(fuzzData, size);
        FuzzStmEncpRemovePair(fuzzData, size);
        FuzzStmFullDisconnect(fuzzData, size);
        FuzzStmFullRemovePair(fuzzData, size);
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    NLSTK_InitStack();
    NLSTK_EnableStack();
    NLSTK_SmCryptoAlgoFuncs_S algoFuncs = {
        .randNumFunc = Crypto_RandNumGenerate,
        .pubPriKeyFunc = Crypto_PubPriKeyPairGenerate,
        .secKeyFunc = Crypto_SecKeyGenerate,
        .derivedKeyFunc = Crypto_DerivedKeyGenerate,
    };
    NLSTK_SmRegAlgoFuncs(&algoFuncs);
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