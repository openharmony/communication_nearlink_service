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

#include "smntry_fuzzer.h"
#include "sm_algos.h"
#include "nai_log.h"
#include "securec.h"
#include "nai_log.h"
#include "sdf_mem.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"
#include "cm_api.h"
#include "sm_auth.h"
#include "sm_encp.h"
#include "sm_nego.h"
#include "sm_noentry.h"
#include "sm_slink.h"
#include "sm_stm.h"
#include "nlstk_sm.h"
#include "sm.h"
#include "sdf_evc.h"
#include "sle_crypto.h"
#include "sdf_thread.h"
 
#define TEST_DEFAULT_NUM 30
typedef enum {
    FUZZ_0      = 0,
    FUZZ_1      = 1,
    FUZZ_2      = 2,
    FUZZ_3      = 3,
    FUZZ_4      = 4,
    FUZZ_5      = 5,
    FUZZ_16 = 16,
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

namespace OHOS {

    char RandomHexByte()
    {
        const char* hex_digits = "0123456789abcdef";
        int random_byte = rand() % FUZZ_256;
        return hex_digits[random_byte];
    }

    void FuzzSMNegoStart(const uint8_t* fuzzData, size_t size)
    {
        (void) fuzzData;
        (void) size;

        SLE_Addr_S g_gNodeAddr1 = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
                RandomHexByte(), RandomHexByte(),
            },
        };

        SLE_Addr_S g_tNodeAddr1 = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
                RandomHexByte(), RandomHexByte(),
            },
        };

        uint8_t g_gNodeRa1[SM_RANDOM_NUMBER_R_LEN] = {
            RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
            RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
            RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
        };
        uint8_t g_tNodeRb1[SM_RANDOM_NUMBER_R_LEN] = {
            RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
            RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
            RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
        };

        SLE_Addr_S *newAddr = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        if (newAddr == nullptr) {
            CP_LOG_ERROR("[SM] Construct newAddr failed");
            return;
        }

        SmSLink_S *slink = SmSLinkCtor(newAddr);
        if (slink == nullptr) {
            CP_LOG_ERROR("[SM] Construct slink failed");
            SDF_MemFree(newAddr);
            return;
        }
        slink->rmtAddr = g_tNodeAddr1;
        slink->localAddr = g_gNodeAddr1;
        int random_number = rand() % FUZZ_2;
        slink->role = random_number ? SM_T_NODE : SM_G_NODE;

        memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_gNodeRa1, SM_RANDOM_NUMBER_R_LEN);
        memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_tNodeRb1, SM_RANDOM_NUMBER_R_LEN);

        SmNoEntryAuthStart(slink);
        SmNoEntryContinueNoentry(slink);

        SmSLinkDtor(slink);
        SDF_MemFree(newAddr);
    }

    void FuzzSMNtryAuthPkgWithRegAlgo(const uint8_t* fuzzData, size_t size)
    {
        NLSTK_SmCryptoAlgoFuncs_S algoFuncs = {
            .randNumFunc = Crypto_RandNumGenerate,
            .pubPriKeyFunc = Crypto_PubPriKeyPairGenerate,
            .secKeyFunc = Crypto_SecKeyGenerate,
            .derivedKeyFunc = Crypto_DerivedKeyGenerate,
        };
        NLSTK_SmRegAlgoFuncs(&algoFuncs);

        (void) fuzzData;
        (void) size;

        SLE_Addr_S g_gNodeAddr1 = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
                RandomHexByte(), RandomHexByte(),
            },
        };

        SLE_Addr_S g_tNodeAddr1 = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
                RandomHexByte(), RandomHexByte(),
            },
        };

        uint8_t g_gNodeRa1[SM_RANDOM_NUMBER_R_LEN] = {
            RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
            RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
            RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
        };
        uint8_t g_tNodeRb1[SM_RANDOM_NUMBER_R_LEN] = {
            RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
            RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
            RandomHexByte(), RandomHexByte(), RandomHexByte(), RandomHexByte(),
        };

        SLE_Addr_S *newAddr = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        if (newAddr == nullptr) {
            CP_LOG_ERROR("[SM] Construct newAddr failed");
            return;
        }

        SmSLink_S *slink = SmSLinkCtor(newAddr);
        if (slink == nullptr) {
            CP_LOG_ERROR("[SM] Construct slink failed");
            SDF_MemFree(newAddr);
            return;
        }
        slink->rmtAddr = g_tNodeAddr1;
        slink->localAddr = g_gNodeAddr1;
        int random_number = rand() % FUZZ_2;
        slink->role = random_number ? SM_T_NODE : SM_G_NODE;

        memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_gNodeRa1, SM_RANDOM_NUMBER_R_LEN);
        memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_tNodeRb1, SM_RANDOM_NUMBER_R_LEN);

        SmNoEntryAuthStart(slink);
        SmNoEntryContinueNoentry(slink);

        SmSLinkDtor(slink);
        SDF_MemFree(newAddr);
    }

    void FuzzSMNtryAuthPkg(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S g_gNodeAddr1 = {
            .type = PUBLIC_ADDRESS, .addr = {0x18, 0xda, 0x8f, 0x00, 0x01, 0x00}
        };

        SLE_Addr_S g_tNodeAddr1 = {
            .type = PUBLIC_ADDRESS, .addr = {0x00, 0x01, 0x0c, 0xb3, 0x33, 0x47}
        };

        uint8_t g_gNodeRa1[SM_RANDOM_NUMBER_R_LEN] = {
            0x2f, 0x18, 0x0c, 0x05, 0x68, 0x0c, 0xe0, 0x94, 0x5d, 0xe2, 0xe6, 0x66, 0x6e, 0xf5, 0xf8, 0x97,
        };
        uint8_t g_tNodeRb1[SM_RANDOM_NUMBER_R_LEN] = {
            0x2f, 0xea, 0x58, 0x43, 0xa8, 0x73, 0xb2, 0x84, 0x8f, 0xab, 0x18, 0x04, 0xb9, 0xc5, 0x89, 0xd4,
        };
        SLE_Addr_S *newAddr = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        if (newAddr == NULL) {
            CP_LOG_ERROR("[SM] Construct newAddr failed");
            return;
        }

        SmSLink_S *slink = SmSLinkCtor(newAddr);
        if (slink == NULL) {
            CP_LOG_ERROR("[SM] Construct slink failed");
            SDF_MemFree(newAddr);
            return;
        }
        slink->rmtAddr = g_tNodeAddr1;
        slink->localAddr = g_gNodeAddr1;
        int random_number = rand() % FUZZ_2;
        slink->role = random_number ? SM_T_NODE : SM_G_NODE;

        memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_gNodeRa1, SM_RANDOM_NUMBER_R_LEN);
        memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_tNodeRb1, SM_RANDOM_NUMBER_R_LEN);

        uint8_t g_gNodePrivKey[SM_PRIVATE_KEY_LEN] = {
            0x72, 0x53, 0x01, 0x65, 0x9f, 0xe0, 0xdb, 0xf1, 0xdf, 0x65, 0x55, 0x1b, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        };

        uint8_t g_gNodePubKey[SM_PUBLIC_KEY_LEN] = {
            0x5b, 0x75, 0xdf, 0xb2, 0x5d, 0x78, 0xa0, 0x74, 0xa4, 0x37, 0x8d, 0xad, 0xc9, 0x02, 0x1e, 0xd1,
            0xd6, 0x2e, 0x42, 0xb8, 0xe7, 0x76, 0x88, 0x07, 0x95, 0x04, 0x83, 0x95, 0x37, 0x13, 0x81, 0xd5,
            0x85, 0x8d, 0xda, 0xbd, 0x99, 0xad, 0x9c, 0x2b, 0x6a, 0x16, 0x8e, 0x43, 0xcd, 0x7a, 0x6f, 0xc4,
            0xfd, 0x65, 0x72, 0x11, 0x7d, 0x62, 0xb1, 0xd1, 0x0c, 0x14, 0x59, 0x82, 0x13, 0xfe, 0x3c, 0x9c,
        };

        uint8_t g_tNodePubKey[SM_PUBLIC_KEY_LEN] = {
            0xc6, 0x0a, 0xe7, 0x16, 0xbd, 0x4a, 0x16, 0xa8, 0xdd, 0x7c, 0x8b, 0x22, 0x4d, 0x62, 0x82, 0x20,
            0xc1, 0xc6, 0x53, 0xec, 0x4e, 0x33, 0x87, 0xcb, 0x7c, 0xc8, 0x5f, 0xfb, 0x4f, 0x59, 0x05, 0x70,
            0x7a, 0x3c, 0xfc, 0x3c, 0xfc, 0xfc, 0x8d, 0xba, 0xc3, 0x4e, 0x8f, 0x8a, 0x0d, 0xcf, 0x40, 0xe9,
            0x8a, 0x2d, 0x40, 0xfb, 0xc1, 0xf0, 0x7b, 0x97, 0x1d, 0xc1, 0x27, 0xb1, 0xa1, 0xa6, 0x23, 0x11,
        };

        uint8_t dhKey[SM_DHKEY_LEN];
        NLSTK_SmKeyPair_S keyPair;
        keyPair.algo = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;
        (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, g_gNodePrivKey, SM_PRIVATE_KEY_LEN);
        (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, g_gNodePubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, g_tNodePubKey, SM_PUBLIC_KEY_LEN);
        SmGenDhKey(&keyPair, dhKey, SM_DHKEY_LEN);
        (void)memcpy_s(slink->dhKey, SM_DHKEY_LEN, dhKey, SM_DHKEY_LEN);

        uint8_t *msg = (uint8_t*)malloc(size);
        
        memcpy_s(msg, size, fuzzData, size);

        SmNoEntryAuthPkgDispatcher((int[]){FUZZ_312, FUZZ_321, FUZZ_322}[rand() % FUZZ_3], slink, msg, size);

        SmSLinkDtor(slink);
        free(msg);
        SDF_MemFree(newAddr);
    }

    void FuzzSMIsSLinkAuthComplete(const uint8_t* fuzzData, size_t size)
    {
        if (size < sizeof(uint16_t)) {
            return;
        }

        uint16_t lcid;
        memcpy_s(&lcid, sizeof(lcid), fuzzData, sizeof(lcid));

        SmIsSLinkAuthComplete(lcid);
        SmIsSLinkEncryptComplete(lcid);
    }

    void FuzzSMApi(const uint8_t *fuzzData, size_t size)
    {
        FuzzSMNegoStart(fuzzData, size);
        FuzzSMNtryAuthPkg(fuzzData, size);
        FuzzSMNtryAuthPkgWithRegAlgo(fuzzData, size);
        FuzzSMIsSLinkAuthComplete(fuzzData, size);
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    (void)SDF_ThreadInit(TEST_DEFAULT_NUM);
    (void)SDF_EvcInit();
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