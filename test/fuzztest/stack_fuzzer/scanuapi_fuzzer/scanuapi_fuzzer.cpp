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
 
#include "scanuapi_fuzzer.h"
#include "securec.h"
#include "nai_log.h"
#include "sdf_mem.h"
#include "nlstk_devd_def.h"
#include "nlstk_devd_api.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"
#include "cm_api.h"
#include "devd_scan.h"
#include "sdf_evc.h"
#include "sdf_thread.h"
 
#define TEST_DEFAULT_NUM 30
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
    FUZZ_ONEHUNDREDTWENTYEIGHT = 128,
    FUZZ_TWOHUNDREDFIFTYSIX    = 256,
    FUZZ_NUM_END
} FUZZ_NUM_E;

int SleSendDliPacket(const SlePkt *packet)
{
    return 0;
}

namespace OHOS {

    void FuzzEnableScan(const uint8_t* fuzzData, size_t size)
    {
        if (size < sizeof(NLSTK_DevdSleScanEnable_S)) {
            return;
        }

        NLSTK_DevdSleScanEnable_S *scanEnable = (NLSTK_DevdSleScanEnable_S *)malloc(sizeof(NLSTK_DevdSleScanEnable_S));
        if (scanEnable == nullptr) {
            NAI_LOG_INFO("Failed to allocate memory for scan_enable\n");
            return;
        }

        int ii = 0;
        scanEnable->scanEnable = fuzzData[ii++];
        scanEnable->scanFilterDuplicates = fuzzData[ii++];

        NLSTK_DevdSleEnableScan(scanEnable);
        free(scanEnable);
    }

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
        if (size < sizeof(NLSTK_DevdSleScanExterCbk_S)) {
            return;
        }
        NLSTK_DevdSleScanExterCbk_S cbks;
        cbks.scanCbk = scanCallback;
        cbks.reportCbk = reportCallback;

        NLSTK_DevdRegScanEventCbk(&cbks);
    }

    void FuzzScanApi(const uint8_t *fuzzData, size_t size)
    {
        FuzzEnableScan(fuzzData, size);
        FuzzScanRegCbk(fuzzData, size);
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    (void)SDF_ThreadInit(TEST_DEFAULT_NUM);
    (void)SDF_EvcInit();
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