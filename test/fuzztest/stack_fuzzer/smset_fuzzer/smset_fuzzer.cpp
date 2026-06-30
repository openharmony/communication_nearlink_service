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
#include "sdf_map.h"
#include "sdf_mem.h"
#include "nlstk_init_api.h"
#include "nlstk_log.h"
#include "nlstk_sm_api.h"
#include "sm_struct.h"
#include "smset_fuzzer.h"
#include "sm_stm.h"
#include "sm_slink.h"

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

    void FuzzSetSecuParams(const uint8_t* fuzzData, size_t size)
    {
        if (size < FUZZ_TWENTY) {
            return;
        }

        NLSTK_SmLocalParams_S *params = (NLSTK_SmLocalParams_S *)SDF_MemAlloc(sizeof(NLSTK_SmLocalParams_S));
        if (params == NULL) {
            NLSTK_LOG_ERROR("malloc failed");
            return;
        }
        int ii = 0;

        params->ioAbility = fuzzData[ii++];
        params->authMethodMask = fuzzData[ii++];
        params->oobDataFlag = fuzzData[ii++];
        params->secKeyMaxLen = fuzzData[ii++];
        params->distAddrFlag = fuzzData[ii++];
        params->distIrkFlag = fuzzData[ii++];
        params->pskFlag = fuzzData[ii++];
        params->authReq.mitmDefend = fuzzData[ii++];
        params->authReq.secAttribute = fuzzData[ii++];
        params->authReq.kpressNotif = fuzzData[ii++];
        uint8_t tempCap[SM_OCTETS_4] = {0x01, 0x02, 0x03, 0x04};
        memcpy_s(params->codeAlgoCap, SM_OCTETS_4, tempCap, SM_OCTETS_4);
        if ((params->ioAbility >= SM_IO_RESERVED) ||
            (params->oobDataFlag > SM_WITH_OOB_DATA) ||
            (params->secKeyMaxLen < SM_MAX_KEY_MIN_LEN) ||
            (params->distIrkFlag > SM_OCTETS_1) ||
            (params->distAddrFlag > SM_OCTETS_1) ||
            (params->pskFlag > SM_PSK_ON) ||
            (params->authReq.secAttribute > SM_PAIRING_MODE_RESERVED) ||
            (params->authReq.mitmDefend > SM_MITM_DEFEND_SUPPORT) ||
            (params->authReq.kpressNotif > SM_KEYPRESS_NOTIF_USED)) {
            return;
        }

        NLSTK_SmSetSecurityParams(params);
        SDF_MemFree(params);
    }

    void FuzzSetStartParing(const uint8_t* fuzzData, size_t size)
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
        NLSTK_SmStartPairing(&addr);
    }

    void FuzzSetConfirm(const uint8_t* fuzzData, size_t size)
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
        slink->negoParams.authMethod = SM_AUTH_METHOD_MAX;
        NLSTK_SmSetConfirm(&addr);
    }

    void FuzzSetPassCode(const uint8_t* fuzzData, size_t size)
    {
        if (size == 0) {
            return;
        }

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
        NLSTK_SmPassCode_S passCode = {
            .addr = addr,
            .passCode = fuzzData[0]
        };
        NLSTK_SmSetPassCode(&passCode);
    }

    void FuzzSetPassWord(const uint8_t* fuzzData, size_t size)
    {
        if (size == 0) {
            return;
        }

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

        size_t totalSize = sizeof(NLSTK_SmPassWord_S) + size;
        NLSTK_SmPassWord_S *passWord = (NLSTK_SmPassWord_S *)SDF_MemAlloc(totalSize);
        if (passWord == NULL) {
            return;
        }
        passWord->addr = addr;
        passWord->passWordLen = (uint8_t)size;

        for (size_t i = 0; i < size; i++) {
            passWord->passWord[i] = (char)fuzzData[i];
        }
        NLSTK_SmSetPassWord(passWord);
        SDF_MemFree(passWord);
    }

    void FuzzSetLocalPsk(const uint8_t* fuzzData, size_t size)
    {
        if (size < SM_PSK_SEC_KEY_LEN) {
            return;
        }

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

        NLSTK_SmPsk_S psk = {{0}};
        psk.addr = addr;
        memcpy_s(psk.psk, SM_PSK_SEC_KEY_LEN, fuzzData, SM_PSK_SEC_KEY_LEN);
        NLSTK_SmSetLocalPsk(&psk);
    }

    void FuzzSMApi(const uint8_t *fuzzData, size_t size)
    {
        g_slinkMap = SDF_MapCtor(SLE_ADDR_TRAITS(), MAKE_TRAITS(SmSLinkDtor, NULL));
        FuzzSetSecuParams(fuzzData, size);
        FuzzSetStartParing(fuzzData, size);
        FuzzSetConfirm(fuzzData, size);
        FuzzSetPassCode(fuzzData, size);
        FuzzSetPassWord(fuzzData, size);
        FuzzSetLocalPsk(fuzzData, size);
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