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
#include "sm_dhkey.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "sdf_map.h"
#include "sle_crypto.h"

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
        /* PR-10 SM-D1: 派生算法必须指明 HA2(AES-CMAC): 缺省 0 使 CMAC 拒绝, SmGenLinkKey 每输入失败早退
         * (与 smpscode 同源的缺口, 本 fuzzer 此前未注册算法也未设能力, 链路密钥派生恒走失败腿) */
        slink.negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;

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

        uint8_t *msg = (uint8_t*)malloc(size);

        memcpy_s(msg, size, fuzzData, size);

        SmGenLinkKey(slink);

        SmNoEntryAuthPkgDispatcher((int[]){1, 312, 313, 314, 321, 322}[fdp.ConsumeIntegral<uint8_t>() % FUZZ_SIX],
            slink, msg, size);

        SmSLinkDtor(slink);
        free(msg);
        SDF_MemFree(newAddr);
    }

    void FuzzSMRegDliCbk(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        uint8_t dataLen = fdp.ConsumeIntegral<uint8_t>();
        if (dataLen > FUZZ_ONEHUNDREDTWENTYEIGHT) {
            dataLen = FUZZ_ONEHUNDREDTWENTYEIGHT;
        }
        DLI_ExecuteCmdRetParam param = {0};
        DLI_ControllerDataEvt *evt = (DLI_ControllerDataEvt *)SDF_MemZalloc(sizeof(DLI_ControllerDataEvt) + dataLen);
        if (evt == NULL) {
            return;
        }
        param.eventParameter = evt;
        for (int i = 0; i < dataLen; i++) {
            evt->data[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        evt->len = dataLen;
        for (int i = 0; i < g_smCbkFuncSize; i++) {
            g_smCbkFunc[i](NULL, fdp.ConsumeIntegral<uint8_t>(), &param);
        }
        SDF_MemFree(evt);
    }

    /* PR-10 SM-D1: DHKey 验证码真算法两腿(内部直调铺垫)。复刻 GenDhKeyCfmCodeKey 的两级 CMAC 构造:
     * 输入密钥 = CMAC(HA2, dhKey低128比特, "dk"||Ra||Rb||G地址||T地址),
     * 验证码 = CMAC(HA2, 输入密钥, Ra/Rb||盐||G IO||T IO||鉴权方式||算法能力4B||G PSK||T PSK||G地址||T地址)。
     * 合法码走 memcmp 接受腿, 翻转一位走 SM_ERR_DHKEY_AUTHENTICODE_CHECK_FAILED 拒收腿, 少一字节走 size 守卫腿。
     * 盐: 仅 SM_AUTH_PASSCODE 取 passCode 低 4 字节小端, 其余鉴权方式为全零, 与 GetDhKeySalt 一致。
     * slink 刻意不入 g_slinkMap: 拒收腿同步迁移 REMV, 不入表则对象存活, 用后手动 Dtor(同 smpscode R1 手法)。 */
    static bool FuzzDhKeyGenCfmCode(SmSLink_S *slink, const SLE_Addr_S *gAddr, const SLE_Addr_S *tAddr,
                                    const uint8_t *randomR, uint8_t *authCode)
    {
        const uint16_t dk = (uint16_t)(EXTEND_ASCII_CHAR_LITTLE_D << SM_SHIFT_BITS_8) | EXTEND_ASCII_CHAR_LITTLE_K;
        uint8_t keyBuff[sizeof(dk) + SM_RANDOM_NUMBER_R_LEN + SM_RANDOM_NUMBER_R_LEN + SLE_ADDR_LEN + SLE_ADDR_LEN];
        (void)memcpy_s(keyBuff, sizeof(dk), &dk, sizeof(dk));
        (void)memcpy_s(keyBuff + sizeof(dk), SM_RANDOM_NUMBER_R_LEN, slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN);
        (void)memcpy_s(keyBuff + sizeof(dk) + SM_RANDOM_NUMBER_R_LEN, SM_RANDOM_NUMBER_R_LEN,
            slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN);
        (void)memcpy_s(keyBuff + sizeof(dk) + SM_RANDOM_NUMBER_R_LEN + SM_RANDOM_NUMBER_R_LEN, SLE_ADDR_LEN,
            gAddr->addr, SLE_ADDR_LEN);
        (void)memcpy_s(keyBuff + sizeof(dk) + SM_RANDOM_NUMBER_R_LEN + SM_RANDOM_NUMBER_R_LEN + SLE_ADDR_LEN,
            SLE_ADDR_LEN, tAddr->addr, SLE_ADDR_LEN);
        uint8_t key[SM_OCTETS_16];
        NLSTK_SmDerivedMac_S input = {
            .algo = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2,
            .buff = keyBuff,
            .buffSize = sizeof(keyBuff),
        };
        (void)memcpy_s(input.key, SM_OCTETS_16, slink->dhKey, SM_OCTETS_16);
        if (!SmCmacGenerate(&input, key, SM_OCTETS_16)) {
            return false;
        }

        uint8_t cfmBuff[SM_RANDOM_NUMBER_R_LEN + SM_SALT_LEN + SM_OCTETS_9 + SLE_ADDR_LEN + SLE_ADDR_LEN] = {0};
        (void)memcpy_s(cfmBuff, SM_RANDOM_NUMBER_R_LEN, randomR, SM_RANDOM_NUMBER_R_LEN);
        uint32_t passCodeNumber = (slink->role == SM_G_NODE) ? slink->gNode.passCode : slink->tNode.passCode;
        if (slink->negoParams.authMethod == SM_AUTH_PASSCODE) {
            for (uint8_t idx = 0; idx < sizeof(passCodeNumber); idx++) {
                cfmBuff[SM_RANDOM_NUMBER_R_LEN + idx] =
                    (uint8_t)((passCodeNumber >> (idx * SM_SHIFT_BITS_8)) & SM_BYTE_MAX_VAL);
            }
        }
        cfmBuff[SM_RANDOM_NUMBER_R_LEN + SM_SALT_LEN] = slink->gNode.ioAbility;
        cfmBuff[SM_RANDOM_NUMBER_R_LEN + SM_SALT_LEN + SM_OCTETS_1] = slink->tNode.ioAbility;
        cfmBuff[SM_RANDOM_NUMBER_R_LEN + SM_SALT_LEN + SM_OCTETS_2] = slink->negoParams.authMethod;
        (void)memcpy_s(cfmBuff + SM_RANDOM_NUMBER_R_LEN + SM_SALT_LEN + SM_OCTETS_3, SM_OCTETS_4,
            slink->negoParams.codeAlgoCap, SM_OCTETS_4);
        cfmBuff[SM_RANDOM_NUMBER_R_LEN + SM_SALT_LEN + SM_OCTETS_7] = slink->gNode.pskFlag;
        cfmBuff[SM_RANDOM_NUMBER_R_LEN + SM_SALT_LEN + SM_OCTETS_8] = slink->tNode.pskFlag;
        (void)memcpy_s(cfmBuff + SM_RANDOM_NUMBER_R_LEN + SM_SALT_LEN + SM_OCTETS_9, SLE_ADDR_LEN,
            gAddr->addr, SLE_ADDR_LEN);
        (void)memcpy_s(cfmBuff + SM_RANDOM_NUMBER_R_LEN + SM_SALT_LEN + SM_OCTETS_9 + SLE_ADDR_LEN, SLE_ADDR_LEN,
            tAddr->addr, SLE_ADDR_LEN);
        (void)memset_s(&input, sizeof(NLSTK_SmDerivedMac_S), 0, sizeof(NLSTK_SmDerivedMac_S));
        input.algo = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
        input.buff = cfmBuff;
        input.buffSize = sizeof(cfmBuff);
        (void)memcpy_s(input.key, SM_OCTETS_16, key, SM_OCTETS_16);
        return SmCmacGenerate(&input, authCode, SM_DHKEY_AUTHCODE_LEN);
    }

    /* PR-10 SM-D1: 铺真材料的 slink——固定向量公私钥真算 DHKey, HA2/KE2 能力齐备,
     * 唯 passCode 与鉴权方式逐输入轮换以覆盖 GetDhKeySalt 的加盐/零盐两族 */
    static SmSLink_S *FuzzDhKeyMakeSLink(SmNodeType_E role, uint8_t authMethod)
    {
        SmSLink_S *slink = SmSLinkCtor(&g_tNodeAddr);
        if (slink == nullptr) {
            return nullptr;
        }
        slink->localAddr = g_gNodeAddr;
        slink->role = role;
        slink->negoParams.authMethod = authMethod;
        slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
        slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY] = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;
        (void)memcpy_s(slink->gNode.pubKey, SM_PUBLIC_KEY_LEN, g_gNodePubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(slink->tNode.pubKey, SM_PUBLIC_KEY_LEN, g_tNodePubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(slink->priKey, SM_PRIVATE_KEY_LEN, g_gNodePrivKey, SM_PRIVATE_KEY_LEN);
        (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_gNodeRa, SM_RANDOM_NUMBER_R_LEN);
        (void)memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_tNodeRb, SM_RANDOM_NUMBER_R_LEN);
        slink->gNode.passCode = 0x11223344;
        slink->tNode.passCode = 0x11223344;
        slink->gNode.ioAbility = SM_IO_DISPLAY_ONLY;
        slink->tNode.ioAbility = SM_IO_KEYBOARD_ONLY;
        slink->gNode.pskFlag = SM_PSK_OFF;
        slink->tNode.pskFlag = SM_PSK_ON;
        NLSTK_SmKeyPair_S keyPair = {
            .algo = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2,
        };
        (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, g_gNodePrivKey, SM_PRIVATE_KEY_LEN);
        (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, g_gNodePubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, g_tNodePubKey, SM_PUBLIC_KEY_LEN);
        if (!SmGenDhKey(&keyPair, slink->dhKey, SM_DHKEY_LEN)) {
            SmSLinkDtor(slink);
            return nullptr;
        }
        return slink;
    }

    void FuzzSMDhKeyCfmTwoLeg(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        uint8_t authMethod = fdp.ConsumeBool() ? SM_AUTH_PASSCODE : SM_AUTH_NUMBER_COMPARE;

        /* T 节点收 G 验证码: 接受 -> SmSendTNodeDhKey 真派生回发; 截断 -> size 守卫; 翻转 -> 拒收中止 */
        SmSLink_S *tSlink = FuzzDhKeyMakeSLink(SM_T_NODE, authMethod);
        if (tSlink != nullptr) {
            tSlink->curStateIndex = SM_STATE_AUTH; /* SmDftReport 事件分流走 AUTH_T 分支 */
            SmAuthDhkeyMsg_S msg = {0};
            if (FuzzDhKeyGenCfmCode(tSlink, &tSlink->rmtAddr, &tSlink->localAddr,
                tSlink->gNode.randomR, msg.authData)) {
                SmRecvGNodeDhKey(tSlink, (const uint8_t *)&msg, sizeof(msg));
                SmRecvGNodeDhKey(tSlink, (const uint8_t *)&msg, sizeof(msg) - 1);
                msg.authData[fdp.ConsumeIntegral<uint8_t>() % SM_OCTETS_16] ^=
                    (uint8_t)(1u << (fdp.ConsumeIntegral<uint8_t>() % 8));
                SmRecvGNodeDhKey(tSlink, (const uint8_t *)&msg, sizeof(msg));
                static bool fuzzDhTLogged = false;
                if (!fuzzDhTLogged) {
                    CP_LOG_ERROR("[FUZZ]SM-D1 T-side recv dhkey legs done, state=%d", (int)tSlink->curStateIndex);
                    fuzzDhTLogged = true;
                }
            }
            SmSLinkDtor(tSlink);
        }

        /* G 节点: 先迁移 AUTH(Entry 触发 NumCmpAuthStart 走通启动腿), 再铺回确定性材料,
         * 接受腿 -> AUTH_SUCCESS -> AuthCbk(SM_PAIR_OK, 内含 IMG 组密钥真生成) -> ENCP */
        SmSLink_S *gSlink = FuzzDhKeyMakeSLink(SM_G_NODE, authMethod);
        if (gSlink != nullptr) {
            STM_MFUNC(gSlink->stm, Transition, g_smStateName[SM_STATE_AUTH]);
            (void)memcpy_s(gSlink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, g_gNodeRa, SM_RANDOM_NUMBER_R_LEN);
            SmAuthDhkeyMsg_S msg = {0};
            if (FuzzDhKeyGenCfmCode(gSlink, &gSlink->localAddr, &gSlink->rmtAddr,
                gSlink->tNode.randomR, msg.authData)) {
                SmRecvTNodeDhKey(gSlink, (const uint8_t *)&msg, sizeof(msg));
                static bool fuzzDhGLogged = false;
                if (!fuzzDhGLogged) {
                    CP_LOG_ERROR("[FUZZ]SM-D1 G-side accept state=%d", (int)gSlink->curStateIndex);
                    fuzzDhGLogged = true;
                }
            }
            SmSendGNodeDhKey(gSlink); /* G 主动发送腿: 真派生验证码 + 组包发送 */
            SmSLinkDtor(gSlink);
        }
    }

    void FuzzSMApi(const uint8_t *fuzzData, size_t size)
    {
        FuzzGenDhKeyAndLinkKey(fuzzData, size);
        FuzzSMDhKeyCfmTwoLeg(fuzzData, size);
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
    /* PR-10 SM-D1: 注册真算法(与 smstm/smpscode 同款五字段), 否则 SmGenDhKey/SmCmacGenerate/SmGenRandNum
     * 恒失败, DHKey 验证码两腿全部早退 */
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
    uint8_t *fuzzData = (uint8_t*)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzSMApi(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}