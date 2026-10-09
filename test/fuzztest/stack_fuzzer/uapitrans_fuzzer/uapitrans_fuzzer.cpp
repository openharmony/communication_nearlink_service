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
 
#include "uapitrans_fuzzer.h"
#include "securec.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "nai_log.h"
#include "sdf_mem.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"
#include "cm_api.h"
#include "devd_scan.h"
#include "devd_tbl.h"
#include "sdf_evc.h"
#include "sdf_thread.h"
#include "transport_errno.h"
#include "transport_internal.h"
#include "qosm_trans_channel.h"

#define FUZZ_TWO 2

namespace OHOS {

    void FuzzUapiSleTransportSendData(const uint8_t* Data, size_t size)
    {
        FuzzedDataProvider fdp(Data, size);
        uint16_t fuzzLen = FUZZ_TWO;

        TRANS_Addr_S* fuzzAddr = (TRANS_Addr_S *)SDF_MemZalloc(sizeof(TRANS_Addr_S));
        if (fuzzAddr == nullptr) {
            return;
        }
        fuzzAddr->devAddr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            fuzzAddr->devAddr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        fuzzAddr->tcid = fdp.ConsumeIntegral<uint8_t>();
        fuzzAddr->proto = static_cast<TRANS_Protocol_E>(fdp.ConsumeIntegral<uint8_t>());
        fuzzAddr->srcPort = fdp.ConsumeIntegral<uint16_t>();
        fuzzAddr->dstPort = fdp.ConsumeIntegral<uint16_t>();

        uint8_t* fuzzData = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t) * FUZZ_TWO);
        if (fuzzData == nullptr) {
            SDF_MemFree(fuzzAddr);
            return;
        }
        for (uint8_t i = 0; i < FUZZ_TWO; i++) {
            fuzzData[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        TRANS_SendData(fuzzAddr, fuzzData, fuzzLen);
        SDF_MemFree(fuzzData);
        SDF_MemFree(fuzzAddr);
    }

    void FuzzUapiTransportChannelCreate(const uint8_t* Data, size_t size)
    {
        FuzzedDataProvider fdp(Data, size);

        QOSM_TransChannelParams_S *fuzzParams = (QOSM_TransChannelParams_S *)SDF_MemZalloc(
            sizeof(QOSM_TransChannelParams_S));
        if (fuzzParams == nullptr) {
            return;
        }
        fuzzParams->addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            fuzzParams->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        fuzzParams->linkMode = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->accessTransMode = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->srcPort = fdp.ConsumeIntegral<uint16_t>();
        fuzzParams->dstPort = fdp.ConsumeIntegral<uint16_t>();
        fuzzParams->slqi = static_cast<QOSM_TransChannelSlqi_E>(fdp.ConsumeIntegral<uint8_t>());
        fuzzParams->tcConf.mode = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->frameType = static_cast<QOSM_TransConnFrameType_E>(fdp.ConsumeIntegral<uint8_t>());

        QOSM_TransChannelCreate(fuzzParams);
        SDF_MemFree(fuzzParams);
    }

    void FuzzUapiTransportChannelDestroy(const uint8_t* Data, size_t size)
    {
        FuzzedDataProvider fdp(Data, size);

        QOSM_TransChannelReleaseParams_S *fuzzParams = (QOSM_TransChannelReleaseParams_S *)SDF_MemZalloc(
            sizeof(QOSM_TransChannelReleaseParams_S));
        if (fuzzParams == nullptr) {
            return;
        }
        fuzzParams->addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            fuzzParams->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        fuzzParams->tcid = fdp.ConsumeIntegral<uint8_t>();

        QOSM_TransChannelDestroy(fuzzParams);
        SDF_MemFree(fuzzParams);
    }

    void FuzzUapiTrnspChannelParamsCheck(const uint8_t* Data, size_t size)
    {
        FuzzedDataProvider fdp(Data, size);
        QOSM_TransChannelParams_S *fuzzInput = (QOSM_TransChannelParams_S *)SDF_MemZalloc(
            sizeof(QOSM_TransChannelParams_S));
        if (fuzzInput == nullptr) {
            return;
        }

        fuzzInput->addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            fuzzInput->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        fuzzInput->srcPort = fdp.ConsumeIntegral<uint16_t>();

        fuzzInput->dstPort = fdp.ConsumeIntegral<uint16_t>();

        fuzzInput->linkMode = SLE_MODE_ACB;
        fuzzInput->accessTransMode = ACCESS_TRANS_MODE_UNICAST;
        fuzzInput->slqi = QOSM_TRANS_CHANNEL_SLQI_LOW;
        fuzzInput->tcConf.mode = TRANSPORT_MODE_BASIC;
        QOSM_TransChannelCreate(fuzzInput);
        SDF_MemFree(fuzzInput);
    }

    void FuzzUapiTrans(const uint8_t *Data, size_t size)
    {
        FuzzUapiSleTransportSendData(Data, size);
        FuzzUapiTransportChannelCreate(Data, size);
        FuzzUapiTransportChannelDestroy(Data, size);
        FuzzUapiTrnspChannelParamsCheck(Data, size);
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    uint16_t sdfThreadInitNum = 10;
    (void) argc;
    (void) argv;
    NLSTK_InitStack();
    NLSTK_EnableStack();
    (void)SDF_ThreadInit(sdfThreadInitNum);
    (void)SDF_EvcInit();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    uint8_t *fuzzData = (uint8_t*)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    (void)memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzUapiTrans(fuzzData, size);
    free(fuzzData);
    return 0;
}