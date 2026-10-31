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

#include "smpsk_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "sm_algos.h"
#include "nlstk_sm_algos.h"
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
#include "sle_crypto.h"
#include "sm_psk.h"
#include "SleDliThreadUtil.h"

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
    SleDliThreadUtil &g_dliThreadUtil = SleDliThreadUtil::GetInstance();

    /* PR-10 SM-D3: PSK 三腿真材料——固定向量(与 smpscode 同源) + 真算 DHKey,
     * 确认码 = CMAC(key=PSK, buff=G公钥||T公钥||随机数), 与 GenPskConfirmNum 同构 */
    static const uint8_t g_pskFuzzPrivKey[SM_PRIVATE_KEY_LEN] = {
        0x72, 0x53, 0x01, 0x65, 0x9f, 0xe0, 0xdb, 0xf1, 0xdf, 0x65, 0x55, 0x1b, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    static const uint8_t g_pskFuzzGPubKey[SM_PUBLIC_KEY_LEN] = {
        0x5b, 0x75, 0xdf, 0xb2, 0x5d, 0x78, 0xa0, 0x74, 0xa4, 0x37, 0x8d, 0xad, 0xc9, 0x02, 0x1e, 0xd1,
        0xd6, 0x2e, 0x42, 0xb8, 0xe7, 0x76, 0x88, 0x07, 0x95, 0x04, 0x83, 0x95, 0x37, 0x13, 0x81, 0xd5,
        0x85, 0x8d, 0xda, 0xbd, 0x99, 0xad, 0x9c, 0x2b, 0x6a, 0x16, 0x8e, 0x43, 0xcd, 0x7a, 0x6f, 0xc4,
        0xfd, 0x65, 0x72, 0x11, 0x7d, 0x62, 0xb1, 0xd1, 0x0c, 0x14, 0x59, 0x82, 0x13, 0xfe, 0x3c, 0x9c,
    };
    static const uint8_t g_pskFuzzTPubKey[SM_PUBLIC_KEY_LEN] = {
        0xc6, 0x0a, 0xe7, 0x16, 0xbd, 0x4a, 0x16, 0xa8, 0xdd, 0x7c, 0x8b, 0x22, 0x4d, 0x62, 0x82, 0x20,
        0xc1, 0xc6, 0x53, 0xec, 0x4e, 0x33, 0x87, 0xcb, 0x7c, 0xc8, 0x5f, 0xfb, 0x4f, 0x59, 0x05, 0x70,
        0x7a, 0x3c, 0xfc, 0x3c, 0xfc, 0xfc, 0x8d, 0xba, 0xc3, 0x4e, 0x8f, 0x8a, 0x0d, 0xcf, 0x40, 0xe9,
        0x8a, 0x2d, 0x40, 0xfb, 0xc1, 0xf0, 0x7b, 0x97, 0x1d, 0xc1, 0x27, 0xb1, 0xa1, 0xa6, 0x23, 0x11,
    };
    static const uint8_t g_pskFuzzRa[SM_RANDOM_NUMBER_R_LEN] = {
        0x2f, 0x18, 0x0c, 0x05, 0x68, 0x0c, 0xe0, 0x94, 0x5d, 0xe2, 0xe6, 0x66, 0x6e, 0xf5, 0xf8, 0x97,
    };
    static const uint8_t g_pskFuzzRb[SM_RANDOM_NUMBER_R_LEN] = {
        0x2f, 0xea, 0x58, 0x43, 0xa8, 0x73, 0xb2, 0x84, 0x8f, 0xab, 0x18, 0x04, 0xb9, 0xc5, 0x89, 0xd4,
    };

    static bool FuzzPskCmac(const uint8_t *key, const uint8_t *buff, size_t buffLen, uint8_t *out)
    {
        NLSTK_SmDerivedMac_S input = {
            .algo = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2,
            .buff = (uint8_t *)buff,
            .buffSize = buffLen,
        };
        (void)memcpy_s(input.key, SM_OCTETS_16, key, SM_OCTETS_16);
        return SmCmacGenerate(&input, out, SM_OCTETS_16);
    }

    /* slink 刻意不入 g_slinkMap(拒收腿迁移 REMV 后仍可观察, 同 smpscode R1 手法) */
    static SmSLink_S *FuzzPskMakeSLink(SmNodeType_E role, const uint8_t *psk)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {0x00, 0x01, 0x0c, 0xb3, 0x33, 0x47},
        };
        SmSLink_S *slink = SmSLinkCtor(&addr);
        if (slink == nullptr) {
            return nullptr;
        }
        slink->role = role;
        slink->negoParams.authMethod = SM_AUTH_PSK;
        slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
        slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY] = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;
        (void)memcpy_s(slink->gNode.pubKey, SM_PUBLIC_KEY_LEN, g_pskFuzzGPubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(slink->tNode.pubKey, SM_PUBLIC_KEY_LEN, g_pskFuzzTPubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(slink->priKey, SM_PRIVATE_KEY_LEN, g_pskFuzzPrivKey, SM_PRIVATE_KEY_LEN);
        (void)memcpy_s(slink->psk, SM_PSK_SEC_KEY_LEN, psk, SM_PSK_SEC_KEY_LEN);
        (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_pskFuzzRa, SM_RANDOM_NUMBER_R_LEN);
        (void)memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_pskFuzzRb, SM_RANDOM_NUMBER_R_LEN);
        NLSTK_SmKeyPair_S keyPair = {
            .algo = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2,
        };
        (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, g_pskFuzzPrivKey, SM_PRIVATE_KEY_LEN);
        (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, g_pskFuzzGPubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, g_pskFuzzTPubKey, SM_PUBLIC_KEY_LEN);
        if (!SmGenDhKey(&keyPair, slink->dhKey, SM_DHKEY_LEN)) {
            SmSLinkDtor(slink);
            return nullptr;
        }
        return slink;
    }

    /* PSK 确认码真派生: CMAC(key=psk, buff=G公钥||T公钥||随机数) */
    static bool FuzzPskGenConfirm(const uint8_t *psk, const uint8_t *randomNum, uint8_t *confirm)
    {
        uint8_t buff[SM_PUBLIC_KEY_LEN + SM_PUBLIC_KEY_LEN + SM_RANDOM_NUMBER_R_LEN];
        (void)memcpy_s(buff, SM_PUBLIC_KEY_LEN, g_pskFuzzGPubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(buff + SM_PUBLIC_KEY_LEN, SM_PUBLIC_KEY_LEN, g_pskFuzzTPubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(buff + SM_PUBLIC_KEY_LEN + SM_PUBLIC_KEY_LEN, SM_RANDOM_NUMBER_R_LEN, randomNum,
            SM_RANDOM_NUMBER_R_LEN);
        return FuzzPskCmac(psk, buff, sizeof(buff), confirm);
    }

    void FuzzSMPskThreeLeg(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        uint8_t psk[SM_PSK_SEC_KEY_LEN] = {0};
        for (int i = 0; i < SM_PSK_SEC_KEY_LEN; i++) {
            psk[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        uint8_t gConfirm[SM_OCTETS_16] = {0};
        uint8_t tConfirm[SM_OCTETS_16] = {0};
        if (!FuzzPskGenConfirm(psk, g_pskFuzzRa, gConfirm) || !FuzzPskGenConfirm(psk, g_pskFuzzRb, tConfirm)) {
            return;
        }

        /* 腿1: T 收 Ra -> 真派生 T 确认码 -> 发 Rb(内部含确认码生成/守卫腿) */
        SmSLink_S *tSlink = FuzzPskMakeSLink(SM_T_NODE, psk);
        if (tSlink != nullptr) {
            SmAuthRandomAMsg_S raMsg = {0};
            (void)memcpy_s(raMsg.authData, SM_RANDOM_NUMBER_R_LEN, g_pskFuzzRa, SM_RANDOM_NUMBER_R_LEN);
            SmPskAuthPkgDispatcher(SM_AUTH_RAND_NUM_RA, tSlink, (const uint8_t *)&raMsg, sizeof(raMsg));
            SmPskAuthPkgDispatcher(SM_AUTH_RAND_NUM_RA, tSlink, (const uint8_t *)&raMsg, sizeof(raMsg) - 1);
            /* 腿1b: T 收 G 确认码——合法走接受(->DHKey/LinkKey 真算->发T确认码), 翻转走 SM_ERR_PSK 拒收 */
            SmAuthGCfmMsg_S gMsg = {0};
            (void)memcpy_s(gMsg.authData, SM_OCTETS_16, gConfirm, SM_OCTETS_16);
            SmPskAuthPkgDispatcher(SM_AUTH_G_NODE_CFM, tSlink, (const uint8_t *)&gMsg, sizeof(gMsg));
            gMsg.authData[fdp.ConsumeIntegral<uint8_t>() % SM_OCTETS_16] ^=
                (uint8_t)(1u << (fdp.ConsumeIntegral<uint8_t>() % 8));
            SmPskAuthPkgDispatcher(SM_AUTH_G_NODE_CFM, tSlink, (const uint8_t *)&gMsg, sizeof(gMsg));
            static bool fuzzPskTLogged = false;
            if (!fuzzPskTLogged) {
                CP_LOG_ERROR("[FUZZ]SM-D3 T legs done, state=%d", (int)tSlink->curStateIndex);
                fuzzPskTLogged = true;
            }
            SmSLinkDtor(tSlink);
        }

        /* 腿2: G 收 Rb -> 真派生 G 确认码 -> 发 G 确认码 */
        SmSLink_S *gSlink = FuzzPskMakeSLink(SM_G_NODE, psk);
        if (gSlink != nullptr) {
            SmAuthRandomBMsg_S rbMsg = {0};
            (void)memcpy_s(rbMsg.authData, SM_RANDOM_NUMBER_R_LEN, g_pskFuzzRb, SM_RANDOM_NUMBER_R_LEN);
            SmPskAuthPkgDispatcher(SM_AUTH_RAND_NUM_RB, gSlink, (const uint8_t *)&rbMsg, sizeof(rbMsg));
            SmPskAuthPkgDispatcher(SM_AUTH_RAND_NUM_RB, gSlink, (const uint8_t *)&rbMsg, sizeof(rbMsg) - 1);
            SmSLinkDtor(gSlink);
        }

        /* 腿3: G 收 T 确认码——合法走接受(->DHKey/LinkKey->SmSendGNodeDhKey), 翻转走拒收 */
        SmSLink_S *gSlink2 = FuzzPskMakeSLink(SM_G_NODE, psk);
        if (gSlink2 != nullptr) {
            gSlink2->curStateIndex = SM_STATE_AUTH; /* SmDftReport 事件分流走 AUTH_G 分支 */
            SmAuthTCfmMsg_S tMsg = {0};
            (void)memcpy_s(tMsg.authData, SM_OCTETS_16, tConfirm, SM_OCTETS_16);
            SmPskAuthPkgDispatcher(SM_AUTH_T_NODE_CFM, gSlink2, (const uint8_t *)&tMsg, sizeof(tMsg));
            tMsg.authData[fdp.ConsumeIntegral<uint8_t>() % SM_OCTETS_16] ^=
                (uint8_t)(1u << (fdp.ConsumeIntegral<uint8_t>() % 8));
            SmPskAuthPkgDispatcher(SM_AUTH_T_NODE_CFM, gSlink2, (const uint8_t *)&tMsg, sizeof(tMsg));
            SmSLinkDtor(gSlink2);
        }
    }

    void FuzzSMPskAuth1(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S g_gNodeAddr1;
        (void)memset_s(&g_gNodeAddr1, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        g_gNodeAddr1.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            g_gNodeAddr1.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        SLE_Addr_S g_tNodeAddr1;
        (void)memset_s(&g_tNodeAddr1, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        g_tNodeAddr1.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            g_tNodeAddr1.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        uint8_t g_gNodeRa1[SM_RANDOM_NUMBER_R_LEN];
        for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
            g_gNodeRa1[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        uint8_t g_tNodeRb1[SM_RANDOM_NUMBER_R_LEN];
        for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
            g_tNodeRb1[i] = fdp.ConsumeIntegral<uint8_t>();
        }

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
        slink->role = fdp.ConsumeBool() ? SM_T_NODE : SM_G_NODE;

        memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_gNodeRa1, SM_RANDOM_NUMBER_R_LEN);
        memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_tNodeRb1, SM_RANDOM_NUMBER_R_LEN);

        SmPskAuthStart(slink);

        SmSLinkDtor(slink);
    }
    
    void FuzzSMPskAuth2(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        NLSTK_SmCryptoAlgoFuncs_S algoFuncs = {
            .randNumFunc = Crypto_RandNumGenerate,
            .pubPriKeyFunc = Crypto_PubPriKeyPairGenerate,
            .secKeyFunc = Crypto_SecKeyGenerate,
            .derivedKeyFunc = Crypto_DerivedKeyGenerate,
        .sha256HashFunc = Crypto_Sha256,
        };
        NLSTK_SmRegAlgoFuncs(&algoFuncs);

        SLE_Addr_S g_gNodeAddr1;
        (void)memset_s(&g_gNodeAddr1, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        g_gNodeAddr1.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            g_gNodeAddr1.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        SLE_Addr_S g_tNodeAddr1;
        (void)memset_s(&g_tNodeAddr1, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        g_tNodeAddr1.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            g_tNodeAddr1.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        uint8_t g_gNodeRa1[SM_RANDOM_NUMBER_R_LEN];
        for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
            g_gNodeRa1[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        uint8_t g_tNodeRb1[SM_RANDOM_NUMBER_R_LEN];
        for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
            g_tNodeRb1[i] = fdp.ConsumeIntegral<uint8_t>();
        }

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
        slink->role = fdp.ConsumeBool() ? SM_T_NODE : SM_G_NODE;

        memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_gNodeRa1, SM_RANDOM_NUMBER_R_LEN);
        memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_tNodeRb1, SM_RANDOM_NUMBER_R_LEN);

        SmPskAuthStart(slink);

        SmSLinkDtor(slink);
    }
    
    void FuzzSMDigit(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S g_gNodeAddr1;
        (void)memset_s(&g_gNodeAddr1, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        g_gNodeAddr1.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            g_gNodeAddr1.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        SLE_Addr_S g_tNodeAddr1;
        (void)memset_s(&g_tNodeAddr1, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        g_tNodeAddr1.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            g_tNodeAddr1.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        uint8_t g_gNodeRa1[SM_RANDOM_NUMBER_R_LEN];
        for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
            g_gNodeRa1[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        uint8_t g_tNodeRb1[SM_RANDOM_NUMBER_R_LEN];
        for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
            g_tNodeRb1[i] = fdp.ConsumeIntegral<uint8_t>();
        }
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
        slink->role = fdp.ConsumeBool() ? SM_T_NODE : SM_G_NODE;

        memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_gNodeRa1, SM_RANDOM_NUMBER_R_LEN);
        memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_tNodeRb1, SM_RANDOM_NUMBER_R_LEN);

        uint8_t *msg = (uint8_t*)SDF_MemZalloc(size);
        if (msg == NULL) {
            SmSLinkDtor(slink);
            SDF_MemFree(newAddr);
            return;
        }

        memcpy_s(msg, size, fuzzData, size);

        SmGenLinkKey(slink);

        SmPskAuthPkgDispatcher((int[]){1, 312, 313, 314, 315, 319, 321, 322}[fdp.ConsumeIntegral<uint8_t>() % FUZZ_EIGHT],
            slink, msg, size);

        SmSLinkDtor(slink);
        SDF_MemFree(msg);
        SDF_MemFree(newAddr);
    }

    void FuzzSMApi(const uint8_t *fuzzData, size_t size)
    {
        FuzzSMPskThreeLeg(fuzzData, size);
        FuzzSMDigit(fuzzData, size);
        FuzzSMPskAuth1(fuzzData, size);
        FuzzSMPskAuth2(fuzzData, size);
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    NLSTK_InitStack();
    NLSTK_EnableStack();
    /* PR-10 SM-D3: 初始化即注册真算法, 三腿确认码派生不再依赖用例中途注册 */
    NLSTK_SmCryptoAlgoFuncs_S algoFuncs = {
        .randNumFunc = Crypto_RandNumGenerate,
        .pubPriKeyFunc = Crypto_PubPriKeyPairGenerate,
        .secKeyFunc = Crypto_SecKeyGenerate,
        .derivedKeyFunc = Crypto_DerivedKeyGenerate,
        .sha256HashFunc = Crypto_Sha256,
    };
    NLSTK_SmRegAlgoFuncs(&algoFuncs);
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
    free(fuzzData);
    OHOS::g_dliThreadUtil.DestroyQueue();
    return 0;
}