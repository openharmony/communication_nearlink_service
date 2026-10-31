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
 
#include "scancbk_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "securec.h"
#include "nai_log.h"
#include "sdf_mem.h"
#include "nlstk_devd_def.h"
#include "nlstk_devd_api.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"
#include "cm_api.h"
#include "devd_scan.h"
#include "dli_cmd_struct.h"
#include "devd_cbk.h"
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
    FUZZ_ONEHUNDREDTWENTYEIGHT = 128,
    FUZZ_TWOHUNDREDFIFTYSIX = 256,
    FUZZ_NUM_END
} FUZZ_NUM_E;

int SleSendDliPacket(const SlePkt *packet)
{
    return 0;
}

namespace OHOS {

    void scanCallback(uint8_t event, uint8_t result)
    {
        NAI_LOG_INFO("Scan event: %u, result: %u\n", event, result);
    }

    void reportCallback(NLSTK_DevdAdvReportInfo_S *report)
    {
        NAI_LOG_INFO("Report received: addr_type: %u, addr: ", report->addrType);
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            NAI_LOG_INFO("%02x", report->addr[i]);
        }
        NAI_LOG_INFO("rssi: %u, data_length: %u\n", report->rssi, report->dataLength);
    }

    void FuzzScanRegCbk(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        NLSTK_DevdSleScanExterCbk_S cbks;
        cbks.scanCbk = scanCallback;
        cbks.reportCbk = reportCallback;

        uint8_t dataLength = fdp.ConsumeIntegral<uint8_t>() % FUZZ_SIX;
        NLSTK_DevdAdvReportInfo_S *report = (NLSTK_DevdAdvReportInfo_S *)SDF_MemZalloc(
            sizeof(NLSTK_DevdAdvReportInfo_S) + dataLength);
        if (report == NULL) {
            return;
        }
        report->addrType = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            report->addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        report->rssi = fdp.ConsumeIntegral<uint8_t>();
        report->dataLength = dataLength;
        for (uint8_t i = 0; i < dataLength; i++) {
            report->data[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        NLSTK_DevdAdvReportExtendParams_S *extendParams = &report->extendParams;
        extendParams->eventType = fdp.ConsumeIntegral<uint8_t>();
        extendParams->dataStatus = fdp.ConsumeIntegral<uint8_t>();
        extendParams->directAddrType = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            extendParams->directAddr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        extendParams->primPhy = fdp.ConsumeIntegral<uint8_t>();
        extendParams->secondPhy = fdp.ConsumeIntegral<uint8_t>();
        extendParams->secondMod = fdp.ConsumeIntegral<uint8_t>();
        extendParams->secondPilotRatio = fdp.ConsumeIntegral<uint8_t>();
        extendParams->secondPolar = fdp.ConsumeIntegral<uint8_t>();
        extendParams->primFrameType = fdp.ConsumeIntegral<uint8_t>();
        extendParams->secondFrameType = fdp.ConsumeIntegral<uint8_t>();

        NLSTK_DevdRegScanEventCbk(&cbks);
        SDF_MemFree(report);
    }

    void FuzzScanApi(const uint8_t *fuzzData, size_t size)
    {
        FuzzScanRegCbk(fuzzData, size);
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
    uint8_t *fuzzData = (uint8_t*)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzScanApi(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}