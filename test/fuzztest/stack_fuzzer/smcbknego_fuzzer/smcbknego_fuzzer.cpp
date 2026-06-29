/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "sle_crypto.h"
#include "smcbknego_fuzzer.h"
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

    char random_hex_byte()
    {
        const char* hex_digits = "0123456789abcdef";
        int random_byte = rand() % FUZZ_TWOHUNDREDFIFTYSIX;
        return hex_digits[random_byte];
    }

    void FuzzSMNego(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S g_gNodeAddr1 = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };

        SLE_Addr_S g_tNodeAddr1 = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(),
            },
        };

        uint8_t g_gNodeRa1[SM_RANDOM_NUMBER_R_LEN] = {
            random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
            random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
            random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
            random_hex_byte(),
        };
        uint8_t g_tNodeRb1[SM_RANDOM_NUMBER_R_LEN] = {
            random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
            random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
            random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
            random_hex_byte(),
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
        int random_number = rand() % FUZZ_TWO;
        slink->role = random_number ? SM_T_NODE : SM_G_NODE;

        memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_gNodeRa1, SM_RANDOM_NUMBER_R_LEN);
        memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_tNodeRb1, SM_RANDOM_NUMBER_R_LEN);

        SDF_MapMoveInsert(g_slinkMap, newAddr, slink);

        SmNegoPkgDispatcher((int[]){307, 308, 309, 310, 311}[rand() % FUZZ_FIVE], slink, fuzzData, size);

        SDF_MapErase(g_slinkMap, newAddr);
    }

    void FuzzSMNegoParamRequest(const uint8_t* fuzzData, size_t size)
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
        slink->gNode.pskFlag = 0;
        slink->tNode.pskFlag = 0;
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        SmPairReqRspMsg_S pkg = {
            .ioAbility = 0x04,
            .oobDataFlag = 0,
            .authReq = { .secAttribute = 0, .mitmDefend = 0, .kpressNotif = 0 },
            .secKeyMaxLen = 16,
            .secInfoDis = 0,
            .codeAlgoCap = {2, 2, 2, 2},
            .pskFlag = 0,
        };

        SmNegoPkgDispatcher(SM_NEGO_PAIRING_REQUEST, slink, (uint8_t *)(&pkg), sizeof(SmPairReqRspMsg_S));
    }

    void FuzzSMNegoParamResponse(const uint8_t* fuzzData, size_t size)
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
        slink->gNode.pskFlag = 0;
        slink->tNode.pskFlag = 0;
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        SmPairReqRspMsg_S pkg = {
            .ioAbility = 0x04,
            .oobDataFlag = 0,
            .authReq = { .secAttribute = 0, .mitmDefend = 0, .kpressNotif = 0 },
            .secKeyMaxLen = 16,
            .secInfoDis = 0,
            .codeAlgoCap = {2, 2, 2, 2},
            .pskFlag = 0,
        };

        SmNegoPkgDispatcher(SM_NEGO_PAIRING_RESPONSE, slink, (uint8_t *)(&pkg), sizeof(SmPairReqRspMsg_S));
    }

    void FuzzSMApi(const uint8_t *fuzzData, size_t size)
    {
        g_slinkMap = SDF_MapCtor(SLE_ADDR_TRAITS(), MAKE_TRAITS(SmSLinkDtor, NULL));
        FuzzSMNego(fuzzData, size);
        FuzzSMNegoParamRequest(fuzzData, size);
        FuzzSMNegoParamResponse(fuzzData, size);
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