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

#include "smdigit_fuzzer.h"
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
#include "sm.h"
#include "sdf_map.h"
#include "SleDliThreadUtil.h"

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
    SleDliThreadUtil &g_dliThreadUtil = SleDliThreadUtil::GetInstance();

    char random_hex_byte()
    {
        const char* hex_digits = "0123456789abcdef";
        int random_byte = rand() % FUZZ_TWOHUNDREDFIFTYSIX; // 0到255之间的随机数
        return hex_digits[random_byte]; // 将随机数转换为16进制字符
    }

    void FuzzSMDigit(const uint8_t* fuzzData, size_t size)
    {
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

        SmNoEntryAuthPkgDispatcher((int[]){312, 313, 314, 321, 322}[rand() % FUZZ_FIVE], slink, msg, sizeof(msg));

        SmSLinkDtor(slink);
        free(msg);
        SDF_MemFree(newAddr);
    }

    void FuzzSMApi(const uint8_t *fuzzData, size_t size)
    {
        FuzzSMDigit(fuzzData, size);
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
    OHOS::g_dliThreadUtil.InitThread();
    uint8_t *fuzzData = (uint8_t*)malloc(size);
    if (fuzzData == nullptr) {
        OHOS::g_dliThreadUtil.DestroyQueue();
        return 0;
    }
    memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzSMApi(fuzzData, size);
    SDF_MemFree(fuzzData);
    OHOS::g_dliThreadUtil.DestroyQueue();
    return 0;
}