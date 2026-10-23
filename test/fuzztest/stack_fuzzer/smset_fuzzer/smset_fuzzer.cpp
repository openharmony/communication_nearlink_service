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
#include "fuzzer/FuzzedDataProvider.h"
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

    void FuzzSetSecuParams(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        NLSTK_SmLocalParams_S *params = (NLSTK_SmLocalParams_S *)SDF_MemAlloc(sizeof(NLSTK_SmLocalParams_S));
        if (params == NULL) {
            NLSTK_LOG_ERROR("malloc failed");
            return;
        }
        (void)memset_s(params, sizeof(NLSTK_SmLocalParams_S), 0, sizeof(NLSTK_SmLocalParams_S));

        params->ioAbility = fdp.ConsumeIntegral<uint8_t>();
        params->authMethodMask = fdp.ConsumeIntegral<uint8_t>();
        params->oobDataFlag = fdp.ConsumeIntegral<uint8_t>();
        params->secKeyMaxLen = fdp.ConsumeIntegral<uint8_t>();
        params->distAddrFlag = fdp.ConsumeIntegral<uint8_t>();
        params->distIrkFlag = fdp.ConsumeIntegral<uint8_t>();
        params->pskFlag = fdp.ConsumeIntegral<uint8_t>();
        params->authReq.mitmDefend = fdp.ConsumeIntegral<uint8_t>();
        params->authReq.secAttribute = fdp.ConsumeIntegral<uint8_t>();
        params->authReq.kpressNotif = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SM_OCTETS_4; i++) {
            params->codeAlgoCap[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        if ((params->ioAbility >= SM_IO_RESERVED) ||
            (params->oobDataFlag > SM_WITH_OOB_DATA) ||
            (params->secKeyMaxLen < SM_MAX_KEY_MIN_LEN) ||
            (params->distIrkFlag > SM_OCTETS_1) ||
            (params->distAddrFlag > SM_OCTETS_1) ||
            (params->pskFlag > SM_PSK_ON) ||
            (params->authReq.secAttribute > SM_PAIRING_MODE_RESERVED) ||
            (params->authReq.mitmDefend > SM_MITM_DEFEND_SUPPORT) ||
            (params->authReq.kpressNotif > SM_KEYPRESS_NOTIF_USED)) {
            SDF_MemFree(params);
            return;
        }

        NLSTK_SmSetSecurityParams(params);
        SDF_MemFree(params);
    }

    void FuzzSetStartParing(const uint8_t* fuzzData, size_t size)
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
        NLSTK_SmStartPairing(&addr);
    }

    void FuzzSetConfirm(const uint8_t* fuzzData, size_t size)
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
        slink->negoParams.authMethod = SM_AUTH_METHOD_MAX;
        NLSTK_SmSetConfirm(&addr);
    }

    void FuzzSetPassCode(const uint8_t* fuzzData, size_t size)
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
        NLSTK_SmPassCode_S passCode = {
            .addr = addr,
            .passCode = fdp.ConsumeIntegral<uint32_t>()
        };
        NLSTK_SmSetPassCode(&passCode);
    }

    void FuzzSetPassWord(const uint8_t* fuzzData, size_t size)
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

        uint8_t passWordLen = fdp.ConsumeIntegral<uint8_t>();
        if (passWordLen > FUZZ_ONEHUNDREDTWENTYEIGHT) {
            passWordLen = FUZZ_ONEHUNDREDTWENTYEIGHT;
        }
        size_t totalSize = sizeof(NLSTK_SmPassWord_S) + passWordLen;
        NLSTK_SmPassWord_S *passWord = (NLSTK_SmPassWord_S *)SDF_MemAlloc(totalSize);
        if (passWord == NULL) {
            return;
        }
        passWord->addr = addr;
        passWord->passWordLen = passWordLen;

        for (uint8_t i = 0; i < passWordLen; i++) {
            passWord->passWord[i] = (char)fdp.ConsumeIntegral<uint8_t>();
        }
        NLSTK_SmSetPassWord(passWord);
        SDF_MemFree(passWord);
    }

    void FuzzSetLocalPsk(const uint8_t* fuzzData, size_t size)
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

        NLSTK_SmPsk_S psk = {{0}};
        psk.addr = addr;
        for (int i = 0; i < SM_PSK_SEC_KEY_LEN; i++) {
            psk.psk[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        NLSTK_SmSetLocalPsk(&psk);
    }

    void FuzzSMApi(const uint8_t *fuzzData, size_t size)
    {
        if (g_slinkMap != NULL) {
            /* 先释放 SmInit 建立或上一输入遗留的表, 避免直接覆盖丢引用 */
            SDF_MapDtor(g_slinkMap);
        }
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