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

#include "sm_fuzzer.h"
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
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "sdf_map.h"

extern SDF_Map *g_slinkMap;
extern DLI_ExecuteCmdCbk g_smCbkFunc[10];
extern int g_smCbkFuncSize;

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
    FUZZ_SIXTEEN = 16,
    FUZZ_ONEHUNDREDTWENTYEIGHT = 128,
    FUZZ_TWOHUNDREDFIFTYSIX    = 256,
    FUZZ_NUM_END
} FUZZ_NUM_E;

namespace OHOS {

    char random_hex_byte()
    {
        const char* hex_digits = "0123456789abcdef";
        int random_byte = rand() % FUZZ_TWOHUNDREDFIFTYSIX; // 0到255之间的随机数
        return hex_digits[random_byte]; // 将随机数转换为16进制字符
    }

    SLE_Addr_S g_gNodeAddr = {
        .type = PUBLIC_ADDRESS,
        .addr = {
            random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
            random_hex_byte(), random_hex_byte(),
        },
    };

    SLE_Addr_S g_tNodeAddr = {
        .type = PUBLIC_ADDRESS,
        .addr = {
            random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
            random_hex_byte(), random_hex_byte(),
        },
    };

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

    uint8_t g_gNodeRa[SM_RANDOM_NUMBER_R_LEN] = {
        random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
        random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
        random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
        random_hex_byte(),
    };
    uint8_t g_tNodeRb[SM_RANDOM_NUMBER_R_LEN] = {
        random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
        random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
        random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte(),
        random_hex_byte(),
    };

    void FuzzGenDhKeyAndLinkKey(const uint8_t* fuzzData, size_t size)
    {
        (void) fuzzData;
        (void) size;

        int random_number = rand() % FUZZ_TWO;

        SmSLink_S slink = {
            .rmtAddr = g_tNodeAddr,
            .localAddr = g_gNodeAddr,
            .role = random_number ? SM_T_NODE : SM_G_NODE,
        };
        memcpy_s(&slink.gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_gNodeRa, SM_RANDOM_NUMBER_R_LEN);
        memcpy_s(&slink.tNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_tNodeRb, SM_RANDOM_NUMBER_R_LEN);

        uint8_t dhKey[SM_DHKEY_LEN];
        NLSTK_SmKeyPair_S keyPair;
        keyPair.algo = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;
        (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, g_gNodePrivKey, SM_PRIVATE_KEY_LEN);
        (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, g_gNodePubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, g_tNodePubKey, SM_PUBLIC_KEY_LEN);
        SmGenDhKey(&keyPair, dhKey, SM_DHKEY_LEN);
        (void)memcpy_s(slink.dhKey, SM_DHKEY_LEN, dhKey, SM_DHKEY_LEN);
        SmGenLinkKey(&slink);
    }

    void FuzzSMDigit(const uint8_t* fuzzData, size_t size)
    {
        if (size < FUZZ_SIXTEEN) {
            return;
        }
        SLE_Addr_S g_gNodeAddr1 = {.type = PUBLIC_ADDRESS, .addr = { random_hex_byte(), random_hex_byte(),
            random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte()}
        };

        SLE_Addr_S g_tNodeAddr1 = {.type = PUBLIC_ADDRESS, .addr = {random_hex_byte(), random_hex_byte(),
            random_hex_byte(), random_hex_byte(), random_hex_byte(), random_hex_byte()}
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

        uint8_t *msg = (uint8_t*)malloc(size);

        memcpy_s(msg, size, fuzzData, size);

        SmGenLinkKey(slink);

        SmNoEntryAuthPkgDispatcher((int[]){1, 312, 313, 314, 321, 322}[rand() % FUZZ_SIX], slink, msg, size);

        SmSLinkDtor(slink);
        free(msg);
        SDF_MemFree(newAddr);
    }

    void FuzzSMRegDliCbk(const uint8_t* fuzzData, size_t size)
    {
        if (size >= FUZZ_TWOHUNDREDFIFTYSIX) {
            return;
        }
        DLI_ExecuteCmdRetParam param = {0};
        DLI_ControllerDataEvt *evt = (DLI_ControllerDataEvt *)SDF_MemZalloc(sizeof(DLI_ControllerDataEvt) + size);
        if (evt == NULL) {
            return;
        }
        param.eventParameter = evt;
        (void)memcpy_s(evt->data, size, fuzzData, size);
        evt->len = size;
        for (int i = 0; i < g_smCbkFuncSize; i++) {
            g_smCbkFunc[i](NULL, random_hex_byte(), &param);
        }
        SDF_MemFree(evt);
    }

    void FuzzSMApi(const uint8_t *fuzzData, size_t size)
    {
        FuzzGenDhKeyAndLinkKey(fuzzData, size);
        FuzzSMDigit(fuzzData, size);
        FuzzSMRegDliCbk(fuzzData, size);
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