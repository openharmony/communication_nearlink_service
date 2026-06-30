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
#include "hadmevtreport_fuzzer.h"
#include "nai_log.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"
#include "cm_api.h"
#include "devd_scan.h"
#include "devd_tbl.h"
#include "sdf_evc.h"
#include "sdf_thread.h"
#include "cpfwk_log.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "dli_errno.h"
#include "dli_log.h"
#include "dli_cmd.h"
#include "dli_cmd_struct.h"
#include "sdf_mem.h"
#include "sdf_addr.h"
#include "securec.h"
#include "dli_hadm_event.h"
#include "stub.h"
#include "hadm_parser_iq.h"
#include "hadm_listen_dli.h"
#include "hadm_link_manager.h"

typedef enum {
    FUZZ_ZERO,
    FUZZ_ONE,
    FUZZ_TWO,
    FUZZ_THREE,
    FUZZ_FOUR,
    FUZZ_NUM_END
} FUZZ_NUM;

namespace OHOS {

    void FuzzEvtReportSetCsEnable(const uint8_t* fuzzData, size_t size)
    {
        if (size < sizeof(DLI_SetMeasureEnableParam)) {
            return;
        }
        size_t pos = 0;

        DLI_SetMeasureEnableParam fuzzInput = {0};
        (void)memcpy_s(&fuzzInput.connHandle, sizeof(fuzzInput.connHandle), fuzzData, sizeof(fuzzInput.connHandle));
        pos += sizeof(fuzzInput.connHandle);
        (void)memcpy_s(&fuzzInput.enable, sizeof(fuzzInput.enable), fuzzData + pos, sizeof(fuzzInput.enable));
        DLI_SetMeasureEnable(&fuzzInput);
    }

    void FuzzEvtReportReadLocalCsCaps(void)
    {
        DLI_ReadLocalMeasureCaps();
    }

    void FuzzEvtReportSetCsConfig(const uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(DLI_SetMeasureConfigParam)) {
            return;
        }

        DLI_SetMeasureConfigParam fuzzInput = {0};
        (void)memcpy_s(&fuzzInput, sizeof(DLI_SetMeasureConfigParam), fuzzData, sizeof(DLI_SetMeasureConfigParam));
        DLI_SetMeasureParam(&fuzzInput);
    }

    void FuzzEvtReportReadRemoteCsCaps(const uint8_t *fuzzData, size_t size)
    {
        DLI_ExecuteCmdCbk EvtReportReadRemoteCsCaps = g_hadmCbkList[FUZZ_ZERO];
        if (size < sizeof(uint16_t) + sizeof(DLI_ReadRemoteCsCapsEvt)) {
            return;
        }
        size_t pos = 0;

        uint16_t fuzzStatus = 0;

        DLI_ExecuteCmdRetParam *fuzzCmdRes = (DLI_ExecuteCmdRetParam *)SDF_MemAlloc(sizeof(DLI_ExecuteCmdRetParam));
        if (fuzzCmdRes == nullptr) {
            return;
        }
        (void)memcpy_s(&fuzzCmdRes->cmdOpcode, sizeof(fuzzCmdRes->cmdOpcode), fuzzData, sizeof(fuzzCmdRes->cmdOpcode));
        pos += sizeof(fuzzCmdRes->cmdOpcode);

        fuzzCmdRes->size = sizeof(DLI_ReadRemoteCsCapsEvt);

        fuzzCmdRes->eventParameter = (void *)SDF_MemAlloc(fuzzCmdRes->size);
        if (fuzzCmdRes->eventParameter == nullptr) {
            SDF_MemFree(fuzzCmdRes);
            return;
        }
        (void)memcpy_s(fuzzCmdRes->eventParameter, fuzzCmdRes->size, fuzzData + pos, fuzzCmdRes->size);

        EvtReportReadRemoteCsCaps(&fuzzStatus, fuzzStatus, fuzzCmdRes);
        SDF_MemFree(fuzzCmdRes->eventParameter);
        SDF_MemFree(fuzzCmdRes);
    }

    void FuzzEvtReportSlemIQ(const uint8_t *fuzzData, size_t size)
    {
        DLI_ExecuteCmdCbk EvtReportSlemIQ = g_hadmCbkList[FUZZ_THREE];
        if (size < sizeof(uint16_t) + HADM_MEASURE_IQ_REPORT_DATA_LEN +
            sizeof(DLI_CsIqReportEvt) + sizeof(HadmSlemChmap_S)) {
            return;
        }
        size_t pos = 0;

        uint16_t fuzzStatus = 0;

        DLI_ExecuteCmdRetParam *fuzzCmdRes = (DLI_ExecuteCmdRetParam *)SDF_MemAlloc(sizeof(DLI_ExecuteCmdRetParam));
        if (fuzzCmdRes == nullptr) {
            return;
        }
        (void)memcpy_s(&fuzzCmdRes->cmdOpcode, sizeof(fuzzCmdRes->cmdOpcode),
            fuzzData + pos, sizeof(fuzzCmdRes->cmdOpcode));
        pos += sizeof(fuzzCmdRes->cmdOpcode);

        fuzzCmdRes->size = HADM_MEASURE_IQ_REPORT_DATA_LEN;

        fuzzCmdRes->eventParameter = (void *)SDF_MemAlloc(fuzzCmdRes->size +
            sizeof(DLI_CsIqReportEvt) + sizeof(HadmSlemChmap_S));
        if (fuzzCmdRes->eventParameter == nullptr) {
            SDF_MemFree(fuzzCmdRes);
            return;
        }
        (void)memcpy_s(fuzzCmdRes->eventParameter, HADM_MEASURE_IQ_REPORT_DATA_LEN +
            sizeof(DLI_CsIqReportEvt) + sizeof(HadmSlemChmap_S), fuzzData + pos,
            HADM_MEASURE_IQ_REPORT_DATA_LEN + sizeof(DLI_CsIqReportEvt) + sizeof(HadmSlemChmap_S));

        EvtReportSlemIQ(&fuzzStatus, fuzzStatus, fuzzCmdRes);
        SDF_MemFree(fuzzCmdRes->eventParameter);
        SDF_MemFree(fuzzCmdRes);
    }

    void FuzzEvtReportMeasureState(const uint8_t *fuzzData, size_t size)
    {
        DLI_ExecuteCmdCbk EvtReportMeasureState = g_hadmCbkList[FUZZ_FOUR];
        if (size < sizeof(uint16_t) + sizeof(HadmSoundingStateInfo_S)) {
            return;
        }
        size_t pos = 0;

        uint16_t fuzzStatus = 0;

        DLI_ExecuteCmdRetParam *fuzzCmdRes = (DLI_ExecuteCmdRetParam *)SDF_MemAlloc(sizeof(DLI_ExecuteCmdRetParam));
        if (fuzzCmdRes == nullptr) {
            return;
        }

        (void)memcpy_s(&fuzzCmdRes->cmdOpcode, sizeof(fuzzCmdRes->cmdOpcode), fuzzData, sizeof(fuzzCmdRes->cmdOpcode));
        pos += sizeof(fuzzCmdRes->cmdOpcode);

        fuzzCmdRes->size = sizeof(HadmSoundingStateInfo_S);

        fuzzCmdRes->eventParameter = (void *)SDF_MemAlloc(fuzzCmdRes->size);
        if (fuzzCmdRes->eventParameter == nullptr) {
            SDF_MemFree(fuzzCmdRes);
            return;
        }
        (void)memcpy_s(fuzzCmdRes->eventParameter, fuzzCmdRes->size, fuzzData + pos, fuzzCmdRes->size);

        EvtReportMeasureState(&fuzzStatus, fuzzStatus, fuzzCmdRes);
        SDF_MemFree(fuzzCmdRes->eventParameter);
        SDF_MemFree(fuzzCmdRes);
    }

    void FuzzEvt(const uint8_t *fuzzData, size_t size)
    {
        HadmRegDliCbk();
        HadmInitSoundingCbManager();
        FuzzEvtReportSetCsEnable(fuzzData, size);
        FuzzEvtReportReadLocalCsCaps();
        FuzzEvtReportSetCsConfig(fuzzData, size);
        FuzzEvtReportReadRemoteCsCaps(fuzzData, size);
        FuzzEvtReportSlemIQ(fuzzData, size);
        FuzzEvtReportMeasureState(fuzzData, size);
        HadmDeInitLinkCbManager();
        HadmUnRegDliCbk();
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
    OHOS::FuzzEvt(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}