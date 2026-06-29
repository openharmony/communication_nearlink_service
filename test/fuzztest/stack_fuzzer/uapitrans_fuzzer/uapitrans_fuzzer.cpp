/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
        if (size < sizeof(TRANS_Addr_S) + sizeof(uint8_t) + sizeof(uint8_t)) {
            return;
        }
        uint8_t pos = 0;
        uint16_t fuzzLen = FUZZ_TWO;

        TRANS_Addr_S* fuzzAddr = (TRANS_Addr_S *)SDF_MemZalloc(sizeof(TRANS_Addr_S));
        if (fuzzAddr == nullptr) {
            return;
        }
        (void)memcpy_s(fuzzAddr, sizeof(TRANS_Addr_S), Data, sizeof(TRANS_Addr_S));
        pos += sizeof(TRANS_Addr_S);

        uint8_t* fuzzData = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t) * FUZZ_TWO);
        if (fuzzData == nullptr) {
            SDF_MemFree(fuzzAddr);
            return;
        }
        (void)memcpy_s(fuzzData, sizeof(uint8_t) * FUZZ_TWO, Data + pos, sizeof(uint8_t) * FUZZ_TWO);

        TRANS_SendData(fuzzAddr, fuzzData, fuzzLen);
        SDF_MemFree(fuzzData);
        SDF_MemFree(fuzzAddr);
    }

    void FuzzUapiTransportChannelCreate(const uint8_t* Data, size_t size)
    {
        if (size < sizeof(QOSM_TransChannelParams_S)) {
            return;
        }

        QOSM_TransChannelParams_S *fuzzParams = (QOSM_TransChannelParams_S *)SDF_MemZalloc(
            sizeof(QOSM_TransChannelParams_S));
        if (fuzzParams == nullptr) {
            return;
        }
        (void)memcpy_s(fuzzParams, sizeof(QOSM_TransChannelParams_S), Data, sizeof(QOSM_TransChannelParams_S));

        QOSM_TransChannelCreate(fuzzParams);
        SDF_MemFree(fuzzParams);
    }

    void FuzzUapiTransportChannelDestroy(const uint8_t* Data, size_t size)
    {
        if (size < sizeof(QOSM_TransChannelReleaseParams_S)) {
            return;
        }

        QOSM_TransChannelReleaseParams_S *fuzzParams = (QOSM_TransChannelReleaseParams_S *)SDF_MemZalloc(
            sizeof(QOSM_TransChannelReleaseParams_S));
        if (fuzzParams == nullptr) {
            return;
        }
        (void)memcpy_s(fuzzParams, sizeof(QOSM_TransChannelReleaseParams_S), Data,
            sizeof(QOSM_TransChannelReleaseParams_S));

        QOSM_TransChannelDestroy(fuzzParams);
        SDF_MemFree(fuzzParams);
    }

    void FuzzUapiTrnspChannelParamsCheck(const uint8_t* Data, size_t size)
    {
        if (size < sizeof(SLE_Addr_S) + sizeof(uint16_t) + sizeof(uint16_t)) {
            return;
        }
        QOSM_TransChannelParams_S *fuzzInput = (QOSM_TransChannelParams_S *)SDF_MemZalloc(
            sizeof(QOSM_TransChannelParams_S));
        if (fuzzInput == nullptr) {
            return;
        }
        uint8_t pos = 0;

        (void)memcpy_s(&fuzzInput->addr, sizeof(SLE_Addr_S), Data, sizeof(SLE_Addr_S));
        pos += sizeof(SLE_Addr_S);

        (void)memcpy_s(&fuzzInput->srcPort, sizeof(uint16_t), Data + pos, sizeof(uint16_t));
        pos += sizeof(uint16_t);

        (void)memcpy_s(&fuzzInput->dstPort, sizeof(uint16_t), Data + pos, sizeof(uint16_t));

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