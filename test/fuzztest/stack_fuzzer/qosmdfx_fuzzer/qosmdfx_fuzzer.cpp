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


#include "qosmdfx_fuzzer.h"

#include "fuzzer/FuzzedDataProvider.h"
#include "qosm_uevent.h"
#include "sdf_mem.h"
#include "sdf_evc.h"
#include "sdf_thread.h"
#include "sdf_errno_base.h"
#include "qosm_audio_dfx.h"
#include "cm_icb_def.h"

char* g_fuzzDataStr = nullptr;
size_t g_fuzzDataSize = 0;

extern "C" void QOSM_UeventInit(QOSM_DspDataProcessCb process)
{
    process(g_fuzzDataStr);
    return;
}

extern "C" void QOSM_UeventDeinit(void)
{
    return;
}

extern "C" bool QOSM_ExecuteBitrateChangeDecision(uint16_t connHandle, uint16_t qosIndex, uint16_t reportedDirection,
    uint16_t reportedQosLevel)
{
    (void)connHandle;
    (void)qosIndex;
    (void)reportedDirection;
    (void)reportedQosLevel;
    return false;
}
 
extern "C" void QOSM_PrintQualityReportParam(CM_ICBQuality *param)
{
    (void)param;
}

static void QOSM_AudioDfxDspStatusCbMock(bool isOn)
{
    (void)isOn;
}

static void QOSM_AudioDfxFlowCtrlCbMock(uint16_t connHandle, bool enterFlowCtrl)
{
    (void)connHandle;
    (void)enterFlowCtrl;
}

namespace OHOS {
    void FuzzDtapGetFrameCtx(const uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        size_t minLen = sizeof(QOSM_AudioDfxInfo) + 1;
        if (size < minLen) {
            return;
        }
        g_fuzzDataSize = size;

        size_t strDataSize = provider.ConsumeIntegralInRange<size_t>(0, size);
        std::vector<uint8_t> strData = provider.ConsumeBytes<uint8_t>(strDataSize);
        strDataSize = strData.size();
        if (strDataSize == 0) {
            return;
        }
        char* fuzzDataStr = static_cast<char*>(std::malloc(strDataSize));
        if (fuzzDataStr == nullptr) {
            return;
        }
        (void)memcpy_s(fuzzDataStr, strDataSize, strData.data(), strDataSize);
        fuzzDataStr[strDataSize - 1] = '\0';
        g_fuzzDataStr = fuzzDataStr;
        std::vector<uint8_t> infoData = provider.ConsumeRemainingBytes<uint8_t>();
        QOSM_AudioDfxInfo info = {0};
        (void)memcpy_s(&info, sizeof(QOSM_AudioDfxInfo), infoData.data(), infoData.size());
        info.flowCtrlCb = QOSM_AudioDfxFlowCtrlCbMock;
        info.dspStatusCb = QOSM_AudioDfxDspStatusCbMock;
        QOSM_AudioDfxStart(&info);
        QOSM_AudioDfxStop();
        free(fuzzDataStr);
        g_fuzzDataStr = nullptr;
    }

    /* DFX 直调组：与上方 FuzzDtapGetFrameCtx 的业务状态机覆盖（Start/Stop）分别统计，
     * 直调 Update/QualityReport/GetDspStatus 等纯参数入口。
     * 注：QOSM_ExecuteBitrateChangeDecision/QOSM_UeventInit 为本文件既有桩——DFX 比特率
     * 变更决策与 uevent netlink 非真实链路，故单列直调组而不混入业务状态机序列。 */
    void FuzzQosmAudioDfxDirectCalls(const uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        struct QOSM_AudioDfxConn conn = {0};
        conn.connHandle = fdp.ConsumeIntegral<uint16_t>();
        conn.lcid = fdp.ConsumeIntegral<uint16_t>();
        for (size_t i = 0; i < QOSM_ADDR_LEN; i++) {
            conn.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        bool connected = fdp.ConsumeBool();
        uint32_t bitrate = fdp.ConsumeIntegral<uint32_t>();
        uint8_t frameType = fdp.ConsumeIntegral<uint8_t>();
        uint16_t powerHandle = fdp.ConsumeIntegral<uint16_t>();
        uint8_t powerLevel = fdp.ConsumeIntegral<uint8_t>();
        uint16_t txFlushed = fdp.ConsumeIntegral<uint16_t>();
        uint16_t ackrate = fdp.ConsumeIntegral<uint16_t>();
        QOSM_AudioDfxUpdateConn(&conn, connected);
        QOSM_AudioDfxUpdateBitrate(bitrate);
        QOSM_AudioDfxUpdateFrameType(frameType);
        QOSM_AudioDfxUpdatePowerLevel(powerHandle, powerLevel);
        QOSM_AudioDfxQualityReport(txFlushed, ackrate);
        bool started = false;
        (void)QOSM_AudioDfxGetDspStatus(&started);
    }
}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    OHOS::FuzzDtapGetFrameCtx(data, size);
    OHOS::FuzzQosmAudioDfxDirectCalls(data, size);
    return 0;
}
