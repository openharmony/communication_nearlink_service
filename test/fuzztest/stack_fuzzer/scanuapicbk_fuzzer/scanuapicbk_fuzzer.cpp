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
 
#include "scanuapicbk_fuzzer.h"
#include "securec.h"
#include "nai_log.h"
#include "sdf_mem.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"
#include "cm_api.h"
#include "devd_scan.h"
#include "dli_cmd_struct.h"
#include "devd_cbk.h"
#include "sdf_evc.h"
#include "sdf_thread.h"
#include "devd_local.h"
#include "dli_event_struct.h"

#define TEST_DEFAULT_NUM 10
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

    static void ReportCbk(NLSTK_DevdAdvReportInfo_S *report)
    {
        (void)report;
    }

    void FuzzSetScanParamCbk(const uint8_t* fuzzData, size_t size)
    {
        if (size == 0) {
            return;
        }
        NLSTK_DevdSleScanExterCbk_S *param = (NLSTK_DevdSleScanExterCbk_S *)SDF_MemZalloc(
            sizeof(NLSTK_DevdSleScanExterCbk_S));
        if (param == NULL) {
            return;
        }
        param->reportCbk = ReportCbk;
        DevdRegScanEventCbk(param);

        DLI_ExecuteCmdRetParam *cfm_par = (DLI_ExecuteCmdRetParam *)SDF_MemZalloc(sizeof(DLI_ExecuteCmdRetParam));
        if (cfm_par == NULL) {
            SDF_MemFree(param);
            return;
        }

        size_t advReportSize = sizeof(DLI_AdvReportEvt) + size;
        cfm_par->eventParameter = SDF_MemZalloc(advReportSize);
        if (cfm_par->eventParameter == NULL) {
            SDF_MemFree(cfm_par);
            SDF_MemFree(param);
            return;
        }

        cfm_par->size = advReportSize;

        struct DLI_AdvReportEvt *par = (struct DLI_AdvReportEvt *)cfm_par->eventParameter;
        par->dataLength = size;

        if (size > 0) {
            memcpy_s(par->data, size, fuzzData, size);
        }

        // 调用回调函数
        DevdSleSetScanParamCbk(NULL, 0, cfm_par);
        DevdSleEnableScanCbk(NULL, 0, cfm_par);
        DevdSleScanReportCbkCheck(cfm_par);
        DevdSleScanReportCbk(NULL, 0, cfm_par);

        // 释放分配的内存
        SDF_MemFree(cfm_par->eventParameter);
        SDF_MemFree(cfm_par);
        SDF_MemFree(param);
    }

    void FuzzScanApi(const uint8_t *fuzzData, size_t size)
    {
        FuzzSetScanParamCbk(fuzzData, size);
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