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

#include "smncmp_fuzzer.h"
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
#include "sm_numcmp.h"
#include "sm_slink.h"
#include "sm_stm.h"
#include "sle_crypto.h"
#include "sm.h"
#include "SleDliThreadUtil.h"

typedef enum {
    FUZZ_0      = 0,
    FUZZ_1      = 1,
    FUZZ_2      = 2,
    FUZZ_3      = 3,
    FUZZ_4      = 4,
    FUZZ_5      = 5,
    FUZZ_16     = 16,
    FUZZ_256    = 256,
    FUZZ_307    = 307,
    FUZZ_308    = 308,
    FUZZ_309    = 309,
    FUZZ_310    = 310,
    FUZZ_311    = 311,
    FUZZ_312    = 312,
    FUZZ_313    = 313,
    FUZZ_314    = 314,
    FUZZ_316    = 316,
    FUZZ_321    = 321,
    FUZZ_322    = 322,
    FUZZ_NUM_END
} FUZZ_NUM_E;

namespace OHOS {
    SleDliThreadUtil &g_dliThreadUtil = SleDliThreadUtil::GetInstance();

    /* PR-10 SM-D4: 数值比较真材料——固定向量 + 真算 DHKey; T 确认码 = CMAC(key=Rb, buff=G公钥||T公钥),
     * 与 SmGenConfirmNum 同构。slink 刻意不入 g_slinkMap(同 smpscode R1 手法)。 */
    static const uint8_t g_numCmpPrivKey[SM_PRIVATE_KEY_LEN] = {
        0x72, 0x53, 0x01, 0x65, 0x9f, 0xe0, 0xdb, 0xf1, 0xdf, 0x65, 0x55, 0x1b, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    static const uint8_t g_numCmpGPubKey[SM_PUBLIC_KEY_LEN] = {
        0x5b, 0x75, 0xdf, 0xb2, 0x5d, 0x78, 0xa0, 0x74, 0xa4, 0x37, 0x8d, 0xad, 0xc9, 0x02, 0x1e, 0xd1,
        0xd6, 0x2e, 0x42, 0xb8, 0xe7, 0x76, 0x88, 0x07, 0x95, 0x04, 0x83, 0x95, 0x37, 0x13, 0x81, 0xd5,
        0x85, 0x8d, 0xda, 0xbd, 0x99, 0xad, 0x9c, 0x2b, 0x6a, 0x16, 0x8e, 0x43, 0xcd, 0x7a, 0x6f, 0xc4,
        0xfd, 0x65, 0x72, 0x11, 0x7d, 0x62, 0xb1, 0xd1, 0x0c, 0x14, 0x59, 0x82, 0x13, 0xfe, 0x3c, 0x9c,
    };
    static const uint8_t g_numCmpTPubKey[SM_PUBLIC_KEY_LEN] = {
        0xc6, 0x0a, 0xe7, 0x16, 0xbd, 0x4a, 0x16, 0xa8, 0xdd, 0x7c, 0x8b, 0x22, 0x4d, 0x62, 0x82, 0x20,
        0xc1, 0xc6, 0x53, 0xec, 0x4e, 0x33, 0x87, 0xcb, 0x7c, 0xc8, 0x5f, 0xfb, 0x4f, 0x59, 0x05, 0x70,
        0x7a, 0x3c, 0xfc, 0x3c, 0xfc, 0xfc, 0x8d, 0xba, 0xc3, 0x4e, 0x8f, 0x8a, 0x0d, 0xcf, 0x40, 0xe9,
        0x8a, 0x2d, 0x40, 0xfb, 0xc1, 0xf0, 0x7b, 0x97, 0x1d, 0xc1, 0x27, 0xb1, 0xa1, 0xa6, 0x23, 0x11,
    };
    static const uint8_t g_numCmpRa[SM_RANDOM_NUMBER_R_LEN] = {
        0x2f, 0x18, 0x0c, 0x05, 0x68, 0x0c, 0xe0, 0x94, 0x5d, 0xe2, 0xe6, 0x66, 0x6e, 0xf5, 0xf8, 0x97,
    };
    static const uint8_t g_numCmpRb[SM_RANDOM_NUMBER_R_LEN] = {
        0x2f, 0xea, 0x58, 0x43, 0xa8, 0x73, 0xb2, 0x84, 0x8f, 0xab, 0x18, 0x04, 0xb9, 0xc5, 0x89, 0xd4,
    };

    static bool FuzzNumCmpGenConfirm(const uint8_t *key, uint8_t *confirm)
    {
        uint8_t buff[SM_PUBLIC_KEY_LEN + SM_PUBLIC_KEY_LEN];
        (void)memcpy_s(buff, SM_PUBLIC_KEY_LEN, g_numCmpGPubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(buff + SM_PUBLIC_KEY_LEN, SM_PUBLIC_KEY_LEN, g_numCmpTPubKey, SM_PUBLIC_KEY_LEN);
        NLSTK_SmDerivedMac_S input = {
            .algo = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2,
            .buff = buff,
            .buffSize = sizeof(buff),
        };
        (void)memcpy_s(input.key, SM_OCTETS_16, key, SM_OCTETS_16);
        return SmCmacGenerate(&input, confirm, SM_OCTETS_16);
    }

    static SmSLink_S *FuzzNumCmpMakeSLink(SmNodeType_E role, uint16_t lcid)
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
        slink->lcid = lcid;
        slink->negoParams.authMethod = SM_AUTH_NUMBER_COMPARE;
        slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
        slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY] = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;
        (void)memcpy_s(slink->gNode.pubKey, SM_PUBLIC_KEY_LEN, g_numCmpGPubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(slink->tNode.pubKey, SM_PUBLIC_KEY_LEN, g_numCmpTPubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(slink->priKey, SM_PRIVATE_KEY_LEN, g_numCmpPrivKey, SM_PRIVATE_KEY_LEN);
        (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_numCmpRa, SM_RANDOM_NUMBER_R_LEN);
        (void)memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_numCmpRb, SM_RANDOM_NUMBER_R_LEN);
        NLSTK_SmKeyPair_S keyPair = {
            .algo = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2,
        };
        (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, g_numCmpPrivKey, SM_PRIVATE_KEY_LEN);
        (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, g_numCmpGPubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, g_numCmpTPubKey, SM_PUBLIC_KEY_LEN);
        if (!SmGenDhKey(&keyPair, slink->dhKey, SM_DHKEY_LEN)) {
            SmSLinkDtor(slink);
            return nullptr;
        }
        return slink;
    }

    void FuzzSMNumCmpConfirmLegs(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        uint16_t lcid = fdp.ConsumeIntegral<uint16_t>();
        uint8_t tConfirm[SM_OCTETS_16] = {0};
        if (!FuzzNumCmpGenConfirm(g_numCmpRb, tConfirm)) {
            return;
        }

        /* G 腿: 真 T 确认码 -> 保存 -> 发 Ra -> 比对一致 -> 六位数字上报 + 等北向输入 */
        SmSLink_S *gSlink = FuzzNumCmpMakeSLink(SM_G_NODE, lcid);
        if (gSlink != nullptr) {
            (void)memcpy_s(gSlink->tNode.confirm, SM_CONFIRM_NUMBER_LEN, tConfirm, SM_CONFIRM_NUMBER_LEN);
            SmAuthTCfmWithRbMsg_S msg = {0};
            (void)memcpy_s(msg.tNodeCfm, SM_OCTETS_16, tConfirm, SM_OCTETS_16);
            (void)memcpy_s(msg.randomB, SM_OCTETS_16, g_numCmpRb, SM_OCTETS_16);
            gSlink->curStateIndex = SM_STATE_AUTH;
            SmNumCmpAuthPkgDispatcher(SM_AUTH_T_NODE_CFM_WITH_RB, gSlink, (const uint8_t *)&msg, sizeof(msg));
            static bool fuzzNcmpGLogged = false;
            if (!fuzzNcmpGLogged) {
                CP_LOG_ERROR("[FUZZ]SM-D4 G confirm leg state=%d expect=0x%x",
                    (int)gSlink->curStateIndex, (unsigned)gSlink->expectOpCode);
                fuzzNcmpGLogged = true;
            }
            /* 拒收腿: 翻转一位 T 确认码 -> SM_ERR_NUMBER_COMPARE 中止 */
            msg.tNodeCfm[fdp.ConsumeIntegral<uint8_t>() % SM_OCTETS_16] ^=
                (uint8_t)(1u << (fdp.ConsumeIntegral<uint8_t>() % 8));
            SmNumCmpAuthPkgDispatcher(SM_AUTH_T_NODE_CFM_WITH_RB, gSlink, (const uint8_t *)&msg, sizeof(msg));
            SmSLinkDtor(gSlink);
        }

        /* T 腿: 收 Ra -> 六位数字上报 + 等输入 + 等 G DHKEY */
        SmSLink_S *tSlink = FuzzNumCmpMakeSLink(SM_T_NODE, lcid);
        if (tSlink != nullptr) {
            SmAuthRandomAMsg_S raMsg = {0};
            (void)memcpy_s(raMsg.authData, SM_RANDOM_NUMBER_R_LEN, g_numCmpRa, SM_RANDOM_NUMBER_R_LEN);
            tSlink->curStateIndex = SM_STATE_AUTH;
            SmNumCmpAuthPkgDispatcher(SM_AUTH_RAND_NUM_RA, tSlink, (const uint8_t *)&raMsg, sizeof(raMsg));
            SmNumCmpAuthPkgDispatcher(SM_AUTH_RAND_NUM_RA, tSlink, (const uint8_t *)&raMsg, sizeof(raMsg) - 1);
            /* DHKEY 先到缓存分支: uapiTimer 占用 -> 报文缓存于全局, 用户确认后再校验 */
            SmAuthDhkeyMsg_S dhMsg = {0};
            for (int i = 0; i < SM_OCTETS_16; i++) {
                dhMsg.authData[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            /* 先删 Ra 腿经 SmSLinkWaitUapiInput 挂上的真实 30s 定时器, 否则假句柄覆盖后
             * timerfd 悬垂(args 指向本 slink), 30s 后 evc 线程 WaitTimeoutCbk 读已 Dtor 的 slink -> UAF(红队 P0-2) */
            SmSLinkWaitDelUapiTimer(tSlink);
            tSlink->uapiTimerHandle = 0x7FFF0000; /* 假句柄: 仅制造占用语义, Dtor 删除时为安全空操作 */
            SmNumCmpAuthPkgDispatcher(SM_AUTH_G_NODE_DHKEY, tSlink, (const uint8_t *)&dhMsg, sizeof(dhMsg));
            /* 用户确认: DHKey/LinkKey 真算后回放缓存报文(校验失败腿, 接受腿由 sm_fuzzer SM-D1 覆盖) */
            SmNumCmpContinueNumComparison(tSlink);
            SmSLinkDtor(tSlink);
        }
    }

    void FuzzSMNegoStart(const uint8_t* fuzzData, size_t size)
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

        SmNumCmpAuthStart(slink);
        SmNumCmpContinueNumComparison(slink);

        SmSLinkDtor(slink);
    }

    void FuzzSMAuthPkg(const uint8_t* fuzzData, size_t size)
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

        SmNumCmpAuthPkgDispatcher((int[]){FUZZ_1, FUZZ_313, FUZZ_316, FUZZ_321, FUZZ_322}[fdp.ConsumeIntegral<uint8_t>() % FUZZ_5],
            slink, msg, size);

        SmSLinkDtor(slink);
        free(msg);
    }

    void FuzzSMNegoStartWithRegAlgo(const uint8_t* fuzzData, size_t size)
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

        SmNumCmpAuthStart(slink);
        SmNumCmpContinueNumComparison(slink);

        SmSLinkDtor(slink);
    }

    void FuzzSMApi(const uint8_t *fuzzData, size_t size)
    {
        FuzzSMNumCmpConfirmLegs(fuzzData, size);
        FuzzSMNegoStart(fuzzData, size);
        FuzzSMAuthPkg(fuzzData, size);
        FuzzSMNegoStartWithRegAlgo(fuzzData, size);
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
    SDF_MemFree(fuzzData);
    OHOS::g_dliThreadUtil.DestroyQueue();
    return 0;
}