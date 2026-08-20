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
#include "sdf_thread.h"
#include "securec.h"

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

}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    NLSTK_InitStack();
    NLSTK_EnableStack();
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
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
    (void)memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzSmSendImgSecuConfig(fuzzData, size);
    OHOS::FuzzSmEnableImgEncp(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}
