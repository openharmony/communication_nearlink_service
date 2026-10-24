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

#include "smpscode_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "sm_algos.h"
#include "nai_log.h"
#include "securec.h"
#include "nai_log.h"
#include "sdf_mem.h"
#include "sle_crypto.h"
#include "nlstk_sm_api.h"
#include "nlstk_sm_algos.h"
#include <string.h>
#include "nlstk_init_api.h"

#include "cpfwk_log.h"
#include "cm_api.h"
#include "sm_auth.h"
#include "sm_encp.h"
#include "sm_nego.h"
#include "sm_noentry.h"
#include "sm_slink.h"
#include "sm_stm.h"
#include "sm_struct.h"
#include "sm.h"
#include "sdf_map.h"
#include "sm_passcode.h"

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
    FUZZ_SIXTEEN = 16,
    FUZZ_ONEHUNDREDTWENTYEIGHT = 128,
    FUZZ_TWOHUNDREDFIFTYSIX    = 256,
    FUZZ_NUM_END
} FUZZ_NUM_E;

namespace OHOS {

    SLE_Addr_S g_gNodeAddr = {
        .type = PUBLIC_ADDRESS,
        .addr = {
            0x18, 0xda, 0x8f, 0x00, 0x01, 0x00,
        },
    };

    SLE_Addr_S g_tNodeAddr = {
        .type = PUBLIC_ADDRESS,
        .addr = {
            0x00, 0x01, 0x0c, 0xb3, 0x33, 0x47,
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
        0x2f, 0x18, 0x0c, 0x05, 0x68, 0x0c, 0xe0, 0x94, 0x5d, 0xe2, 0xe6, 0x66, 0x6e, 0xf5, 0xf8, 0x97,
    };
    uint8_t g_tNodeRb[SM_RANDOM_NUMBER_R_LEN] = {
        0x2f, 0xea, 0x58, 0x43, 0xa8, 0x73, 0xb2, 0x84, 0x8f, 0xab, 0x18, 0x04, 0xb9, 0xc5, 0x89, 0xd4,
    };

    uint8_t g_gNodeDhKey[SM_DHKEY_LEN] = {
        0xdf, 0x21, 0x93, 0x16, 0x77, 0x5c, 0xa0, 0x96, 0x64, 0x4f, 0x40, 0x1d, 0x77, 0x39, 0x0a, 0x43,
        0xdb, 0xeb, 0xfa, 0x26, 0x27, 0xd8, 0xe7, 0x99, 0xc2, 0x06, 0x09, 0x5b, 0x4f, 0x81, 0xb9, 0xd7,
    };
    uint8_t g_linkKey[SM_LINK_KEY_LEN] = {
        0x0f, 0xe3, 0xa0, 0x70, 0x13, 0xb6, 0xaf, 0x9a, 0x81, 0x91, 0xb7, 0xd3, 0x76, 0xa0, 0xa9, 0x0a,
    };

    /* PR-5 SM-A3: 外部结果回调记录桩, 认证成功/失败副作用观测 */
    static uint32_t g_fuzzSmAuthCbkCnt = 0;
    static void FuzzSmStartCbk(NLSTK_SmPairingStart_S *params) { (void)params; }
    static void FuzzSmRemoveCbk(NLSTK_SmPairingRemove_S *params) { (void)params; }
    static void FuzzSmRequestCbk(NLSTK_SmPairingRequest_S *params) { (void)params; }
    static void FuzzSmAuthCbk(NLSTK_SmAuthComplete_S *params) { (void)params; g_fuzzSmAuthCbkCnt++; }
    static void FuzzSmEncCbk(NLSTK_SmEncComplete_S *params) { (void)params; }
    static void FuzzSmImgMsgCbk(NLSTK_SmSendImgMsgCmpl_S *params) { (void)params; }

    /* SDF_MapMoveInsert 重复 key 时返回 false 且不接管 key/slink: 回收并复用已登记条目,
     * 避免每次输入恒定泄漏(设备实证: 修复前 RSS 线性增长至 OOM)。产品侧同款用法见 sm.c:689。 */
    static SmSLink_S *FuzzStmGetSLink(const SLE_Addr_S *addr)
    {
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        if (key == nullptr) {
            return nullptr;
        }
        (void)memcpy_s(key, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(addr);
        if (slink == nullptr) {
            SDF_MemFree(key);
            return nullptr;
        }
        if (!SDF_MapMoveInsert(g_slinkMap, key, slink)) {
            SDF_MemFree(key);
            SmSLinkDtor(slink);
            SDF_MapIter *iter = SDF_MapFind(g_slinkMap, addr);
            if (iter == nullptr) {
                return nullptr;
            }
            slink = (SmSLink_S *)iter->val;
        }
        return slink;
    }

    void FuzzGenDhKeyAndLinkKey(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SmSLink_S slink = {
            .rmtAddr = g_tNodeAddr,
            .localAddr = g_gNodeAddr,
            .role = fdp.ConsumeBool() ? SM_T_NODE : SM_G_NODE,
        };
        memcpy_s(&slink.gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_gNodeRa, SM_RANDOM_NUMBER_R_LEN);
        memcpy_s(&slink.tNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_tNodeRb, SM_RANDOM_NUMBER_R_LEN);
        /* 派生算法必须指明 HA2(AES-CMAC): 缺省 0 使 Crypto_DerivedKeyGenerate 走 default 拒绝,
         * SmGenLinkKey 每输入失败早退, 后续向量比对与认证回调腿全部变死代码(红队 P0-2) */
        slink.negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;

        uint8_t dhKey[SM_DHKEY_LEN];
        NLSTK_SmKeyPair_S keyPair;
        keyPair.algo = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;
        (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, g_gNodePrivKey, SM_PRIVATE_KEY_LEN);
        (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, g_gNodePubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, g_tNodePubKey, SM_PUBLIC_KEY_LEN);
        if (!SmGenDhKey(&keyPair, dhKey, SM_DHKEY_LEN)) {
            return;
        }
        (void)memcpy_s(slink.dhKey, SM_DHKEY_LEN, dhKey, SM_DHKEY_LEN);
        if (!SmGenLinkKey(&slink)) {
            return;
        }
        /* PR-5 SM-A3: 固定向量比对——向量来源未核实, 仅记录不冻结(计划 A3.3 "待核对"口径),
         * 不符不判失败: 成组关系与角色未经上游证实, 不得把当前实现输出定为权威答案 */
        static bool fuzzVecLogged = false;
        if (!fuzzVecLogged && slink.role == SM_G_NODE) {
            bool dhOk = (memcmp(dhKey, g_gNodeDhKey, SM_DHKEY_LEN) == 0);
            bool lkOk = (memcmp(slink.linkKey, g_linkKey, SM_LINK_KEY_LEN) == 0);
            CP_LOG_ERROR("[FUZZ]SM-A3 vectors(wei-heid): dhKey match=%d linkKey match=%d", (int)dhOk, (int)lkOk);
            fuzzVecLogged = true;
        }
        /* 认证回调腿: 真实 DH/LinkKey 之后经 AUTH_SUCCESS 触发外部 authCbk(强制迁移, 内部路径记账)。
         * 栈 slink 的 stm 为聚合初始化留下的 NULL, 必须用 SmSLinkCtor 建的堆 slink 驱动状态机 */
        SmSLink_S *authSlink = SmSLinkCtor(&g_tNodeAddr);
        if (authSlink != nullptr) {
            authSlink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] =
                SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
            STM_MFUNC(authSlink->stm, Transition, g_smStateName[SM_STATE_AUTH]);
            STM_MFUNC(authSlink->stm, ProcessMessage, (Message) { .what = SM_AUTH_SUCCESS });
            SmSLinkDtor(authSlink);
        }
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

        (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_gNodeRa1, SM_RANDOM_NUMBER_R_LEN);
        (void)memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_tNodeRb1, SM_RANDOM_NUMBER_R_LEN);

        uint8_t *msg = (uint8_t*)SDF_MemZalloc(size);
        if (msg == NULL) {
            SDF_MemFree(newAddr);
            return;
        }

        (void)memcpy_s(msg, size, fuzzData, size);

        SmGenLinkKey(slink);

        SmPassCodeAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, slink, msg, size);
        SmPassCodeAuthPkgDispatcher(SM_AUTH_T_NODE_CFM_WITH_RB, slink, msg, size);
        SmPassCodeAuthPkgDispatcher(SM_AUTH_G_NODE_DHKEY, slink, msg, size);
        SmPassCodeAuthPkgDispatcher(SM_AUTH_T_NODE_DHKEY, slink, msg, size);
        SmPassCodeAuthPkgDispatcher(FUZZ_ONE, slink, msg, size);

        SmSLinkDtor(slink);
        SDF_MemFree(msg);
        SDF_MemFree(newAddr);
    }

    void FuzzSMPassCodeAuthStartCase1(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }

        slink->gNode.ioAbility = SM_IO_DISPLAY_ONLY;
        slink->tNode.ioAbility = SM_IO_KEYBOARD_ONLY;
        slink->role = SM_G_NODE;
        SmPassCodeAuthStart(slink);
    }

    void FuzzSMPassCodeAuthStartCase2(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }

        slink->gNode.ioAbility = SM_IO_DISPLAY_ONLY;
        slink->tNode.ioAbility = SM_IO_KEYBOARD_ONLY;
        slink->role = SM_T_NODE;
        SmPassCodeAuthStart(slink);
    }

    void FuzzSMPassCodeAuthStartCase3(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }

        slink->gNode.ioAbility = SM_IO_KEYBOARD_ONLY;
        slink->tNode.ioAbility = SM_IO_DISPLAY_ONLY;
        slink->role = SM_G_NODE;
        SmPassCodeAuthStart(slink);
    }

    void FuzzSMPassCodeAuthStartCase4(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }

        slink->gNode.ioAbility = SM_IO_KEYBOARD_ONLY;
        slink->tNode.ioAbility = SM_IO_DISPLAY_ONLY;
        slink->role = SM_T_NODE;
        SmPassCodeAuthStart(slink);
    }

    void FuzzSMSetPassCodeG(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        SmPassCodeSetGPassCode(slink);
    }

    void FuzzSMSetPassCodeT(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        SmPassCodeSetTPassCode(slink);
    }
    
    /* PR-5 SM-A3/SM-R1: 真算法生成确认码——合法应被接受、翻转一位/截断应被拒收且无认证成功副作用。
     * 确认码 = CMAC(key=G.randomR, buff=G公钥||T公钥||T.mixCode), 与产品校验路径同一算法。
     * slink 刻意不入 g_slinkMap: 拒收与发送失败都会同步迁移 REMV, 而 SmRemoveSLink 只做
     * MapErase, 不入表则对象存活, 可安全观察 expectOpCode/curStateIndex, 用后手动 Dtor。 */
    static SmSLink_S *FuzzR1MakeTNodeSLink(void)
    {
        SLE_Addr_S addr = g_gNodeAddr;
        SmSLink_S *slink = SmSLinkCtor(&addr);
        if (slink == nullptr) {
            return nullptr;
        }
        slink->role = SM_T_NODE;
        (void)memcpy_s(slink->gNode.pubKey, SM_PUBLIC_KEY_LEN, g_gNodePubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(slink->tNode.pubKey, SM_PUBLIC_KEY_LEN, g_tNodePubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_gNodeRa, SM_RANDOM_NUMBER_R_LEN);
        (void)memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_tNodeRb, SM_RANDOM_NUMBER_R_LEN);
        slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
        const uint8_t mixCode[SM_MIX_PASSCODE_LEN] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        (void)memcpy_s(slink->tNode.mixCode, SM_MIX_PASSCODE_LEN, mixCode, SM_MIX_PASSCODE_LEN);
        return slink;
    }

    void FuzzSMR1CfmAcceptAndReject(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        /* 确认码: 与 GenPassCodeConfirmNum 同构(G公钥||T公钥||mixCode, key=G随机数) */
        uint8_t cfmBuff[SM_PUBLIC_KEY_LEN * 2 + SM_MIX_PASSCODE_LEN];
        (void)memcpy_s(cfmBuff, sizeof(cfmBuff), g_gNodePubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(cfmBuff + SM_PUBLIC_KEY_LEN, SM_PUBLIC_KEY_LEN, g_tNodePubKey, SM_PUBLIC_KEY_LEN);
        const uint8_t mixCode[SM_MIX_PASSCODE_LEN] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        (void)memcpy_s(cfmBuff + SM_PUBLIC_KEY_LEN * 2, SM_MIX_PASSCODE_LEN, mixCode, SM_MIX_PASSCODE_LEN);
        NLSTK_SmDerivedMac_S macInput = {};
        macInput.algo = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
        (void)memcpy_s(macInput.key, SM_OCTETS_16, g_gNodeRa, SM_OCTETS_16);
        macInput.buff = cfmBuff;
        macInput.buffSize = sizeof(cfmBuff);
        SmAuthGCfmWithRaMsg_S msg = {};
        if (!SmCmacGenerate(&macInput, msg.gNodeCfm, SM_OCTETS_16)) {
            return;
        }
        (void)memcpy_s(msg.randomA, SM_OCTETS_16, g_gNodeRa, SM_OCTETS_16);

        /* 1) 合法确认码: T 节点校验通过并发送 Rb 确认(发送腿在桩环境可能失败, 属环境前置, 如实记录) */
        SmSLink_S *slink = FuzzR1MakeTNodeSLink();
        if (slink == nullptr) {
            return;
        }
        SmPassCodeAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, slink, (const uint8_t *)&msg, sizeof(msg));
        uint16_t expectAfterLegal = slink->expectOpCode;
        SmState_E stateAfterLegal = slink->curStateIndex;
        /* 2) 截断报文: 处理端 size 守卫应直接拒绝, 不触碰状态 */
        SmPassCodeAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, slink, (const uint8_t *)&msg, sizeof(msg) - 1);
        SmSLinkDtor(slink);

        /* 3) 翻转一位确认码: 校验不一致 -> SM_ERR_PASSCODE 中止, 不产生认证成功副作用 */
        SmSLink_S *slink2 = FuzzR1MakeTNodeSLink();
        if (slink2 == nullptr) {
            return;
        }
        msg.gNodeCfm[fdp.ConsumeIntegral<uint8_t>() % SM_OCTETS_16] ^=
            (uint8_t)(1u << (fdp.ConsumeIntegral<uint8_t>() % 8));
        SmPassCodeAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, slink2, (const uint8_t *)&msg, sizeof(msg));
        static bool fuzzR1Logged = false;
        if (!fuzzR1Logged) {
            CP_LOG_ERROR("[FUZZ]SM-R1 accept(expect=%u st=%d) reject(st=%d) authCbkCnt=%u",
                (unsigned)expectAfterLegal, (int)stateAfterLegal,
                (int)slink2->curStateIndex, (unsigned)g_fuzzSmAuthCbkCnt);
            fuzzR1Logged = true;
        }
        SmSLinkDtor(slink2);
    }

    void FuzzSMApi(const uint8_t *fuzzData, size_t size)
    {
        if (g_slinkMap != NULL) {
            /* 先释放 SmInit 建立或上一输入遗留的表, 避免直接覆盖丢引用 */
            SDF_MapDtor(g_slinkMap);
        }
        g_slinkMap = SDF_MapCtor(SLE_ADDR_TRAITS(), MAKE_TRAITS(SmSLinkDtor, NULL));
        FuzzGenDhKeyAndLinkKey(fuzzData, size);
        FuzzSMR1CfmAcceptAndReject(fuzzData, size);
        FuzzSMDigit(fuzzData, size);
        FuzzSMPassCodeAuthStartCase1(fuzzData, size);
        FuzzSMPassCodeAuthStartCase2(fuzzData, size);
        FuzzSMPassCodeAuthStartCase3(fuzzData, size);
        FuzzSMPassCodeAuthStartCase4(fuzzData, size);
        FuzzSMSetPassCodeG(fuzzData, size);
        FuzzSMSetPassCodeT(fuzzData, size);
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
        .sha256HashFunc = Crypto_Sha256,
    };
    NLSTK_SmRegAlgoFuncs(&algoFuncs);
    NLSTK_SmCallbacks_S extCbks = {
        .startCbk = FuzzSmStartCbk,
        .removeCbk = FuzzSmRemoveCbk,
        .requestCbk = FuzzSmRequestCbk,
        .authCbk = FuzzSmAuthCbk,
        .encCbk = FuzzSmEncCbk,
        .imgMsgCbk = FuzzSmImgMsgCbk,
    };
    NLSTK_SmRegExternalCbks(&extCbks);
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
    free(fuzzData);
    return 0;
}