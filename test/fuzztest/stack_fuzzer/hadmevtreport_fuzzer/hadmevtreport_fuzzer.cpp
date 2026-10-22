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
#include "fuzzer/FuzzedDataProvider.h"
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
        FuzzedDataProvider fdp(fuzzData, size);
        DLI_SetMeasureEnableParam fuzzInput = {0};
        fuzzInput.connHandle = fdp.ConsumeIntegral<uint16_t>();
        fuzzInput.enable = fdp.ConsumeIntegral<uint8_t>();
        DLI_SetMeasureEnable(&fuzzInput);
    }

    void FuzzEvtReportReadLocalCsCaps(void)
    {
        DLI_ReadLocalMeasureCaps();
    }

    void FuzzEvtReportSetCsConfig(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        DLI_SetMeasureConfigParam fuzzInput = {0};
        fuzzInput.connHandle = fdp.ConsumeIntegral<uint16_t>();
        fuzzInput.configId = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.measureConfigDirect = fdp.ConsumeIntegral<uint32_t>();
        fuzzInput.occurrenceGroupPeriod = fdp.ConsumeIntegral<uint32_t>();
        fuzzInput.schedulingTimeslot = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.rttPhy = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.freqHoppingMode = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.fmFreq = fdp.ConsumeIntegral<uint16_t>();
        fuzzInput.sendDirect = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.antennaOrderConfig = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.firstAntennaTypeConfig = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.secondAntennaTypeConfig = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.eventsCount = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.bitWidth = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.pmInitAntCount = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.pmInitSignal2Tone = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.firstNodeInterval = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.pmReflAntCount = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.pmReflSignal2Tone = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.secondNodeInterval = fdp.ConsumeIntegral<uint8_t>();
        fuzzInput.channelBandwidth = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_MEASURE_PM_24G_BAND_LEN; i++) {
            fuzzInput.pm2400mBand[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        DLI_SetMeasureParam(&fuzzInput);
    }

    void FuzzEvtReportReadRemoteCsCaps(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        DLI_ExecuteCmdCbk EvtReportReadRemoteCsCaps = g_hadmCbkList[FUZZ_ZERO];
        uint16_t fuzzStatus = fdp.ConsumeIntegral<uint16_t>();

        DLI_ExecuteCmdRetParam *fuzzCmdRes = (DLI_ExecuteCmdRetParam *)SDF_MemAlloc(sizeof(DLI_ExecuteCmdRetParam));
        if (fuzzCmdRes == nullptr) {
            return;
        }
        fuzzCmdRes->cmdOpcode = fdp.ConsumeIntegral<uint16_t>();
        fuzzCmdRes->size = sizeof(DLI_ReadRemoteCsCapsEvt);
        DLI_ReadRemoteCsCapsEvt evt = {0};
        evt.status = fdp.ConsumeIntegral<uint8_t>();
        evt.connHandle = fdp.ConsumeIntegral<uint16_t>();
        for (uint8_t i = 0; i < SLE_MEASURE_LEN; i++) {
            evt.caps[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        fuzzCmdRes->eventParameter = &evt;

        EvtReportReadRemoteCsCaps(&fuzzStatus, fuzzStatus, fuzzCmdRes);
        SDF_MemFree(fuzzCmdRes);
    }

    void FuzzEvtReportSlemIQ(const uint8_t *fuzzData, size_t size)
    {
        /* 在原来基础上加了3个多音管道，每个管道3B，还有1B的指示符 */
        uint32_t fuzzLen = HADM_MEASURE_IQ_REPORT_DATA_LEN + 79 * 3 * 3 + 1;
        FuzzedDataProvider fdp(fuzzData, size);
        DLI_ExecuteCmdCbk EvtReportSlemIQ = g_hadmCbkList[FUZZ_THREE];
        uint16_t fuzzStatus = fdp.ConsumeIntegral<uint16_t>();

        DLI_ExecuteCmdRetParam *fuzzCmdRes = (DLI_ExecuteCmdRetParam *)SDF_MemAlloc(sizeof(DLI_ExecuteCmdRetParam));
        if (fuzzCmdRes == nullptr) {
            return;
        }
        fuzzCmdRes->cmdOpcode = fdp.ConsumeIntegral<uint16_t>();
        fuzzCmdRes->size = fuzzLen;
        uint32_t blobSize = fuzzLen + sizeof(DLI_CsIqReportEvt) + sizeof(HadmSlemChmap_S);
        fuzzCmdRes->eventParameter = (void *)SDF_MemAlloc(blobSize);
        if (fuzzCmdRes->eventParameter == nullptr) {
            SDF_MemFree(fuzzCmdRes);
            return;
        }
        uint8_t *blob = (uint8_t *)fuzzCmdRes->eventParameter;
        DLI_CsIqReportEvt *iqEvt = (DLI_CsIqReportEvt *)blob;
        iqEvt->status = fdp.ConsumeIntegral<uint8_t>();
        iqEvt->connHandle = fdp.ConsumeIntegral<uint16_t>();
        iqEvt->slemIdx = fdp.ConsumeIntegral<uint8_t>();
        uint16_t slemInfo = fdp.ConsumeIntegral<uint16_t>();
        (void)memcpy_s(&iqEvt->slemInfoType, sizeof(DLI_SlemInfoType), &slemInfo, sizeof(DLI_SlemInfoType));
        iqEvt->timestampSn = fdp.ConsumeIntegral<uint32_t>();
        for (uint32_t i = sizeof(DLI_CsIqReportEvt); i < blobSize; i++) {
            blob[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        EvtReportSlemIQ(&fuzzStatus, fuzzStatus, fuzzCmdRes);
        SDF_MemFree(fuzzCmdRes->eventParameter);
        SDF_MemFree(fuzzCmdRes);
    }

    void FuzzEvtReportMeasureState(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        DLI_ExecuteCmdCbk EvtReportMeasureState = g_hadmCbkList[FUZZ_FOUR];
        uint16_t fuzzStatus = fdp.ConsumeIntegral<uint16_t>();

        DLI_ExecuteCmdRetParam *fuzzCmdRes = (DLI_ExecuteCmdRetParam *)SDF_MemAlloc(sizeof(DLI_ExecuteCmdRetParam));
        if (fuzzCmdRes == nullptr) {
            return;
        }
        fuzzCmdRes->cmdOpcode = fdp.ConsumeIntegral<uint16_t>();
        fuzzCmdRes->size = sizeof(HadmSoundingStateInfo_S);
        HadmSoundingStateInfo_S evt = {0};
        evt.status = fdp.ConsumeIntegral<uint8_t>();
        evt.posMeasureSigConfigIdx = fdp.ConsumeIntegral<uint8_t>();
        evt.measureState = fdp.ConsumeIntegral<uint8_t>();
        fuzzCmdRes->eventParameter = &evt;

        EvtReportMeasureState(&fuzzStatus, fuzzStatus, fuzzCmdRes);
        SDF_MemFree(fuzzCmdRes);
    }

    void FuzzEvtReportSetMeasureConfigParam(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        DLI_ExecuteCmdCbk EvtReportSetMeasureConfigParam = g_hadmCbkList[FUZZ_ONE];
        uint16_t fuzzStatus = fdp.ConsumeIntegral<uint16_t>();

        DLI_ExecuteCmdRetParam *fuzzCmdRes = (DLI_ExecuteCmdRetParam *)SDF_MemAlloc(sizeof(DLI_ExecuteCmdRetParam));
        if (fuzzCmdRes == nullptr) {
            return;
        }
        fuzzCmdRes->cmdOpcode = fdp.ConsumeIntegral<uint16_t>();
        fuzzCmdRes->size = sizeof(DLI_CommandStatus);
        DLI_CommandStatus evt = {0};
        evt.cmdOpcode = fdp.ConsumeIntegral<uint16_t>();
        evt.numDliCommandPackets = fdp.ConsumeIntegral<uint8_t>();
        evt.status = fdp.ConsumeIntegral<uint8_t>();
        fuzzCmdRes->eventParameter = &evt;

        EvtReportSetMeasureConfigParam(&fuzzStatus, fuzzStatus, fuzzCmdRes);
        SDF_MemFree(fuzzCmdRes);
    }

    void FuzzEvtReportSetMeasureEn(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        DLI_ExecuteCmdCbk EvtReportSetMeasureEn = g_hadmCbkList[FUZZ_TWO];
        uint16_t fuzzStatus = fdp.ConsumeIntegral<uint16_t>();

        DLI_ExecuteCmdRetParam *fuzzCmdRes = (DLI_ExecuteCmdRetParam *)SDF_MemAlloc(sizeof(DLI_ExecuteCmdRetParam));
        if (fuzzCmdRes == nullptr) {
            return;
        }
        fuzzCmdRes->cmdOpcode = fdp.ConsumeIntegral<uint16_t>();
        fuzzCmdRes->size = sizeof(DLI_CommandStatus);
        DLI_CommandStatus evt = {0};
        evt.cmdOpcode = fdp.ConsumeIntegral<uint16_t>();
        evt.numDliCommandPackets = fdp.ConsumeIntegral<uint8_t>();
        evt.status = fdp.ConsumeIntegral<uint8_t>();
        fuzzCmdRes->eventParameter = &evt;

        EvtReportSetMeasureEn(&fuzzStatus, fuzzStatus, fuzzCmdRes);
        SDF_MemFree(fuzzCmdRes);
    }

    void FuzzEvt(const uint8_t *fuzzData, size_t size)
    {
        HadmRegDliCbk();
        HadmInitSoundingCbManager();
        FuzzEvtReportSetCsEnable(fuzzData, size);
        FuzzEvtReportReadLocalCsCaps();
        FuzzEvtReportSetCsConfig(fuzzData, size);
        FuzzEvtReportSetMeasureConfigParam(fuzzData, size);
        FuzzEvtReportSetMeasureEn(fuzzData, size);
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
    FuzzedDataProvider fdp(data, size);
    std::vector<uint8_t> fuzzVec = fdp.ConsumeBytes<uint8_t>(size);
    uint8_t *fuzzData = fuzzVec.data();
    size_t fuzzSize = fuzzVec.size();
    OHOS::FuzzEvt(fuzzData, fuzzSize);
    return 0;
}