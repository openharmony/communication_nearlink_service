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
 
#include "scan_fuzzer.h"
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

typedef struct SlePkt {
    uint8_t *data;
    uint32_t size;
} SlePkt;

int SleSendDliPacket(const SlePkt *packet)
{
    return 0;
}

typedef enum {
    FUZZ_SIX     = 6,
    FUZZ_NUM_END
} FUZZ_NUM_E;

namespace OHOS {

    void FuzzSetScan(const uint8_t* fuzzData, size_t size)
    {
        if (size < sizeof(NLSTK_DevdSleScanParams_S) + FUZZ_SIX) {
            return;
        }
        NLSTK_DevdSleScanParams_S *sle_scan_params;
        int ii = 0;
        sle_scan_params = (NLSTK_DevdSleScanParams_S *)SDF_MemAlloc(
            sizeof(NLSTK_DevdSleScanParams_S) + 1 * sizeof(NLSTK_DevdSleScanParamsNoPhy_S));
        if (sle_scan_params == NULL) {
            NAI_LOG_ERROR("sle_scan_params new fail");
            return;
        }

        sle_scan_params->scanFilterPolicy = fuzzData[ii++];
        for (uint8_t idx = 0; idx < 1; idx++) {
            sle_scan_params->params[idx].scanType = fuzzData[ii++];
            // 读取 scanInterval（两个字节）
            sle_scan_params->params[idx].scanInterval = (uint16_t)(fuzzData[ii] | (fuzzData[ii+1] << 8));
            ii += 2;

            // 读取 scanWindow（两个字节）
            sle_scan_params->params[idx].scanWindow = (uint16_t)(fuzzData[ii] | (fuzzData[ii+1] << 8));
        }
        DevdSleSetScanParam(sle_scan_params);
        free(sle_scan_params);
    }

    void FuzzScanApi(const uint8_t *fuzzData, size_t size)
    {
        FuzzSetScan(fuzzData, size);
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    NLSTK_InitStack();
    NLSTK_EnableStack();
    (void)SDF_ThreadInit(10);
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
    memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzScanApi(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}