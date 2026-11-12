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

#include "smimg_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "nlstk_sm_api.h"
#include "nai_log.h"
#include "nlstk_init_api.h"
#include "sdf_evc.h"
#include "sdf_mem.h"
#include "sdf_map.h"
#include "sdf_thread.h"
#include "sle_crypto.h"
#include "sm_algos.h"
#include "sm.h"
#include "sm_img.h"
#include "sm_slink.h"
#include "securec.h"

extern SDF_Map *g_slinkMap;

typedef struct SlePkt {
    uint8_t *data;
    uint32_t size;
} SlePkt;

int SleSendDliPacket(const SlePkt *packet)
{
    (void)packet;
    return 0;
}

namespace OHOS {

static void SmStartEventCbkImpl(NLSTK_SmPairingStart_S *params)
{
    (void)params;
}

static void SmRemoveEventCbkImpl(NLSTK_SmPairingRemove_S *params)
{
    (void)params;
}

static void SmRequestEventCbkImpl(NLSTK_SmPairingRequest_S *params)
{
    (void)params;
}

static void SmAuthEventCbkImpl(NLSTK_SmAuthComplete_S *params)
{
    (void)params;
}

static void SmEncEventCbkImpl(NLSTK_SmEncComplete_S *params)
{
    (void)params;
}

static void SmImgSendMsgCbkImpl(NLSTK_SmSendImgMsgCmpl_S *params)
{
    (void)params;
}

static void SmImgEncpCbkImpl(NLSTK_SmImgEncpCmpl_S *params)
{
    (void)params;
}

static void FillAddr(FuzzedDataProvider &fdp, SLE_Addr_S &addr)
{
    addr.type = fdp.ConsumeIntegral<uint8_t>();
    for (int i = 0; i < SLE_ADDR_LEN; i++) {
        addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
    }
}

void FuzzSmSendImgSecuConfig(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    NLSTK_SmImgSecuConfig_S config;
    (void)memset_s(&config, sizeof(config), 0, sizeof(config));
    FillAddr(fdp, config.addr);
    config.imgId = fdp.ConsumeIntegral<uint8_t>();
    for (int i = 0; i < SM_OCTETS_16; i++) {
        config.groupKey[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    config.cryptoAlgo = fdp.ConsumeIntegral<uint8_t>();
    config.keyDerivAlgo = fdp.ConsumeIntegral<uint8_t>();
    config.intgChkInd = fdp.ConsumeIntegral<uint8_t>();
    config.giv = fdp.ConsumeIntegral<uint64_t>();
    (void)NLSTK_SmSendImgSecuConfig(&config);
}

void FuzzSmEnableImgEncp(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    NLSTK_SmImgEncpParam_S param;
    (void)memset_s(&param, sizeof(param), 0, sizeof(param));
    param.imgHandle = fdp.ConsumeIntegral<uint8_t>();
    param.cryptoAlgo = fdp.ConsumeIntegral<uint8_t>();
    param.giv = fdp.ConsumeIntegral<uint64_t>();
    for (int i = 0; i < SM_OCTETS_16; i++) {
        param.groupKey[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    (void)NLSTK_SmEnableImgEncp(&param);
}

/* PR-10 SM-D6: IMG 内部路径直调(内部直调铺垫)——
 * SmGenerateImgSecuConfig 参数守卫/合法生成两腿; SmSendImgMsg slink 未命中/命中(真 CMAC 派生 kg)两腿;
 * SmEnableImgEncryption cryptoAlgo>0 与 ==0 两腿(内部 algo 换算分支)。
 * 命中腿的 slink 走共用插入进 g_slinkMap, 末尾 MapErase 保持全局表有界(同 smstm 手法)。 */
void FuzzSmImgInternal(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    /* 组密钥/giv 生成: 空指针守卫腿 + 合法真生成腿(双随机数 + CMAC) */
    uint64_t giv = 0;
    uint8_t groupKey[SM_OCTETS_16] = {0};
    SmGenerateImgSecuConfig(SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2, NULL, &giv);
    SmGenerateImgSecuConfig(SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2, groupKey, NULL);
    SmGenerateImgSecuConfig(SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2, groupKey, &giv);

    NLSTK_SmImgSecuConfig_S config;
    (void)memset_s(&config, sizeof(config), 0, sizeof(config));
    FillAddr(fdp, config.addr);
    config.imgId = fdp.ConsumeIntegral<uint8_t>();
    for (int i = 0; i < SM_OCTETS_16; i++) {
        config.groupKey[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    config.cryptoAlgo = SM_ENCRYPTION_ALGORITHM_ABILITY_AC1;
    config.keyDerivAlgo = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
    config.intgChkInd = SM_INTEGRITY_PROTECTION_ALGORITHM_ABILITY_IA2;
    config.giv = fdp.ConsumeIntegral<uint64_t>();
    /* slink 未命中腿: cmpl=SM_IMG_ERROR 回调 */
    SmSendImgMsg(&config);

    /* slink 命中腿: linkKey 铺随机材料, GenerateKg 真派生后组包发送 */
    if (g_slinkMap != NULL) {
        SmSLink_S *slink = SmSLinkCtor(&config.addr);
        if (slink != NULL) {
            SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
            if (key == NULL) {
                SmSLinkDtor(slink);
                return;
            }
            (void)memcpy_s(key, sizeof(SLE_Addr_S), &config.addr, sizeof(SLE_Addr_S));
            if (!SDF_MapMoveInsert(g_slinkMap, key, slink)) {
                SDF_MemFree(key);
                SmSLinkDtor(slink);
            } else {
                slink->lcid = fdp.ConsumeIntegral<uint16_t>();
                for (int i = 0; i < SM_LINK_KEY_LEN; i++) {
                    slink->linkKey[i] = fdp.ConsumeIntegral<uint8_t>();
                }
                SmSendImgMsg(&config);
                SDF_MapErase(g_slinkMap, &config.addr);
            }
        }
    }

    /* 组播使能: cryptoAlgo==0 与 >0 两腿(algo 换算分支), 结果由回调记账 */
    NLSTK_SmImgEncpParam_S param;
    (void)memset_s(&param, sizeof(param), 0, sizeof(param));
    param.imgHandle = fdp.ConsumeIntegral<uint8_t>();
    param.cryptoAlgo = fdp.ConsumeBool() ? 0 : SM_ENCRYPTION_ALGORITHM_ABILITY_AC2;
    param.giv = fdp.ConsumeIntegral<uint64_t>();
    for (int i = 0; i < SM_OCTETS_16; i++) {
        param.groupKey[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    SmEnableImgEncryption(&param);
}

}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    NLSTK_InitStack();
    NLSTK_EnableStack();
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    /* PR-10 SM-D6: 注册真算法, 否则 SmGenRandNum/SmCmacGenerate 恒失败,
     * GenerateGroupKey/GenerateKg 全部早退, SmSendImgMsg 只剩错误分支 */
    NLSTK_SmCryptoAlgoFuncs_S algoFuncs = {
        .randNumFunc = Crypto_RandNumGenerate,
        .pubPriKeyFunc = Crypto_PubPriKeyPairGenerate,
        .secKeyFunc = Crypto_SecKeyGenerate,
        .derivedKeyFunc = Crypto_DerivedKeyGenerate,
        .sha256HashFunc = Crypto_Sha256,
    };
    NLSTK_SmRegAlgoFuncs(&algoFuncs);
    NLSTK_SmCallbacks_S cbks = {0};
    cbks.startCbk = OHOS::SmStartEventCbkImpl;
    cbks.removeCbk = OHOS::SmRemoveEventCbkImpl;
    cbks.requestCbk = OHOS::SmRequestEventCbkImpl;
    cbks.authCbk = OHOS::SmAuthEventCbkImpl;
    cbks.encCbk = OHOS::SmEncEventCbkImpl;
    cbks.imgMsgCbk = OHOS::SmImgSendMsgCbkImpl;
    (void)NLSTK_SmRegExternalCbks(&cbks);
    NLSTK_SmImgCallbacks_S imgCbks = {0};
    imgCbks.imgEncpCbk = OHOS::SmImgEncpCbkImpl;
    (void)NLSTK_SmRegImgCbks(&imgCbks);
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    uint8_t *fuzzData = (uint8_t *)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    /* 直调腿先于异步投递用例执行: 命中腿会短暂改动共享 g_slinkMap(插入后即擦除),
     * 排在 NLSTK_SmSendImgSecuConfig 之前可将与调度线程任务的交错窗口压到最小(纪律: 单输入自洽) */
    (void)memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzSmImgInternal(fuzzData, size);
    OHOS::FuzzSmSendImgSecuConfig(fuzzData, size);
    OHOS::FuzzSmEnableImgEncp(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}
