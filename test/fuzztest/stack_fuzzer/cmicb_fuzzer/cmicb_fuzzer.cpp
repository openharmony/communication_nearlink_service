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

#include "cmicb_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "cm_icb_api.h"
#include "nai_log.h"
#include "nlstk_init_api.h"
#include "sdf_evc.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "securec.h"

#define FUZZ_ICB_MAX_CNT 4
#define FUZZ_ICB_CODEC_CFG_LEN 8

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

static void IcbConnectionCbkImpl(CM_ICBConnection *connection)
{
    (void)connection;
}

static void IcbLabelReportCbkImpl(CM_ICBLabelReportParam *param)
{
    (void)param;
}

static void IcbQualityCbkImpl(CM_ICBQuality *quality)
{
    (void)quality;
}

static void FreqBandListenerImpl(CM_FreqBandSwitchParam *param)
{
    (void)param;
}

static CM_ICBType ConsumeIcbType(FuzzedDataProvider &fdp)
{
    return static_cast<CM_ICBType>(fdp.ConsumeIntegral<uint8_t>() % 2);
}

void FuzzIcgSetParam(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    CM_ICBParam icbParam[FUZZ_ICB_MAX_CNT];
    CM_ICBParam::CM_ICB icbInner[FUZZ_ICB_MAX_CNT][2];
    (void)memset_s(icbParam, sizeof(icbParam), 0, sizeof(icbParam));
    (void)memset_s(icbInner, sizeof(icbInner), 0, sizeof(icbInner));
    uint8_t paramCnt = fdp.ConsumeIntegral<uint8_t>() % FUZZ_ICB_MAX_CNT;
    for (uint8_t i = 0; i < paramCnt; i++) {
        icbParam[i].id = fdp.ConsumeIntegral<uint8_t>();
        uint8_t innerCnt = fdp.ConsumeIntegral<uint8_t>() % 2;
        for (uint8_t j = 0; j < innerCnt; j++) {
            icbInner[i][j].maxSduG2T = fdp.ConsumeIntegral<uint16_t>();
            icbInner[i][j].maxSduT2G = fdp.ConsumeIntegral<uint16_t>();
            icbInner[i][j].rtnG2T = fdp.ConsumeIntegral<uint8_t>();
            icbInner[i][j].rtnT2G = fdp.ConsumeIntegral<uint8_t>();
        }
        icbParam[i].param = icbInner[i];
    }
    CM_ICGParam icgParam = {};
    icgParam.type = ConsumeIcbType(fdp);
    icgParam.id = fdp.ConsumeIntegral<uint8_t>();
    icgParam.sduIntervalG2T = fdp.ConsumeIntegral<uint32_t>();
    icgParam.sduIntervalT2G = fdp.ConsumeIntegral<uint32_t>();
    icgParam.sca = fdp.ConsumeIntegral<uint8_t>();
    icgParam.packing = fdp.ConsumeIntegral<uint8_t>();
    icgParam.framing = fdp.ConsumeIntegral<uint8_t>();
    icgParam.maxLatencyG2T = fdp.ConsumeIntegral<uint16_t>();
    icgParam.maxLatencyT2G = fdp.ConsumeIntegral<uint16_t>();
    icgParam.icbCnt = paramCnt;
    icgParam.paramCnt = paramCnt;
    icgParam.icbParam = icbParam;
    (void)CM_ICGSetParam(&icgParam);
}

void FuzzIcgSetTestParam(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    CM_ICBTestParam icbParam[FUZZ_ICB_MAX_CNT];
    (void)memset_s(icbParam, sizeof(icbParam), 0, sizeof(icbParam));
    uint8_t icbCnt = fdp.ConsumeIntegral<uint8_t>() % FUZZ_ICB_MAX_CNT;
    for (uint8_t i = 0; i < icbCnt; i++) {
        icbParam[i].id = fdp.ConsumeIntegral<uint8_t>();
        icbParam[i].nse = fdp.ConsumeIntegral<uint8_t>();
        icbParam[i].maxSduG2T = fdp.ConsumeIntegral<uint16_t>();
        icbParam[i].maxSduT2G = fdp.ConsumeIntegral<uint16_t>();
        icbParam[i].maxPduG2T = fdp.ConsumeIntegral<uint16_t>();
        icbParam[i].maxPduT2G = fdp.ConsumeIntegral<uint16_t>();
        icbParam[i].frameG2T = fdp.ConsumeIntegral<uint8_t>();
        icbParam[i].frameT2G = fdp.ConsumeIntegral<uint8_t>();
        icbParam[i].phyG2T = fdp.ConsumeIntegral<uint8_t>();
        icbParam[i].phyT2G = fdp.ConsumeIntegral<uint8_t>();
        icbParam[i].mcsG2T = fdp.ConsumeIntegral<uint8_t>();
        icbParam[i].mcsT2G = fdp.ConsumeIntegral<uint8_t>();
        icbParam[i].pilotG2T = fdp.ConsumeIntegral<uint8_t>();
        icbParam[i].pilotT2G = fdp.ConsumeIntegral<uint8_t>();
        icbParam[i].bnG2T = fdp.ConsumeIntegral<uint8_t>();
        icbParam[i].bnT2G = fdp.ConsumeIntegral<uint8_t>();
    }
    CM_ICGTestParam icgParam = {};
    icgParam.type = ConsumeIcbType(fdp);
    icgParam.id = fdp.ConsumeIntegral<uint8_t>();
    icgParam.labelId = fdp.ConsumeIntegral<uint8_t>();
    icgParam.sduIntervalG2T = fdp.ConsumeIntegral<uint32_t>();
    icgParam.sduIntervalT2G = fdp.ConsumeIntegral<uint32_t>();
    icgParam.ftG2T = fdp.ConsumeIntegral<uint8_t>();
    icgParam.ftT2G = fdp.ConsumeIntegral<uint8_t>();
    icgParam.icbInterval = fdp.ConsumeIntegral<uint16_t>();
    icgParam.sca = fdp.ConsumeIntegral<uint8_t>();
    icgParam.packing = fdp.ConsumeIntegral<uint8_t>();
    icgParam.framing = fdp.ConsumeIntegral<uint8_t>();
    icgParam.icbCnt = icbCnt;
    icgParam.icbParam = icbParam;
    bool supportAutorate = fdp.ConsumeBool();
    (void)CM_ICGSetTestParam(&icgParam, supportAutorate);
}

void FuzzIcgRemoveParam(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    CM_ICGRemovedParam icgParam = {};
    icgParam.type = ConsumeIcbType(fdp);
    icgParam.id = fdp.ConsumeIntegral<uint8_t>();
    (void)CM_ICGRemoveParam(&icgParam);
}

void FuzzIcgSetLabel(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    CM_ICBChannel icb[FUZZ_ICB_MAX_CNT];
    (void)memset_s(icb, sizeof(icb), 0, sizeof(icb));
    uint8_t icbCnt = fdp.ConsumeIntegral<uint8_t>() % FUZZ_ICB_MAX_CNT;
    for (uint8_t i = 0; i < icbCnt; i++) {
        icb[i].connHandle = fdp.ConsumeIntegral<uint16_t>();
        icb[i].lcid = fdp.ConsumeIntegral<uint16_t>();
        icb[i].direction = fdp.ConsumeIntegral<uint8_t>();
        icb[i].labelId = fdp.ConsumeIntegral<uint8_t>();
    }
    CM_ICGLabelParam icgLabel = {};
    icgLabel.type = ConsumeIcbType(fdp);
    icgLabel.id = fdp.ConsumeIntegral<uint8_t>();
    icgLabel.icbCnt = icbCnt;
    icgLabel.icb = icb;
    bool supportSubrate = fdp.ConsumeBool();
    bool supportAutorate = fdp.ConsumeBool();
    (void)CM_ICGSetLabel(&icgLabel, supportSubrate, supportAutorate);
}

void FuzzIcbConnection(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    CM_ICBChannel channel[FUZZ_ICB_MAX_CNT];
    (void)memset_s(channel, sizeof(channel), 0, sizeof(channel));
    uint8_t channelCnt = fdp.ConsumeIntegral<uint8_t>() % FUZZ_ICB_MAX_CNT;
    for (uint8_t i = 0; i < channelCnt; i++) {
        channel[i].connHandle = fdp.ConsumeIntegral<uint16_t>();
        channel[i].lcid = fdp.ConsumeIntegral<uint16_t>();
        channel[i].direction = fdp.ConsumeIntegral<uint8_t>();
        channel[i].labelId = fdp.ConsumeIntegral<uint8_t>();
    }
    CM_ICBConnectionParam connParam = {};
    connParam.type = ConsumeIcbType(fdp);
    connParam.id = fdp.ConsumeIntegral<uint8_t>();
    connParam.labelId = fdp.ConsumeIntegral<uint8_t>();
    connParam.channelCnt = channelCnt;
    connParam.channel = channel;
    bool supportAutorate = fdp.ConsumeBool();
    (void)CM_ICBAddConnection(&connParam, supportAutorate);
    (void)CM_ICBDelConnection(&connParam);
}

void FuzzIcgUpdateParam(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    uint16_t connHandle[FUZZ_ICB_MAX_CNT] = {0};
    uint8_t icbCnt = fdp.ConsumeIntegral<uint8_t>() % FUZZ_ICB_MAX_CNT;
    for (uint8_t i = 0; i < icbCnt; i++) {
        connHandle[i] = fdp.ConsumeIntegral<uint16_t>();
    }
    CM_ICGUpdatedParam icgParam = {};
    icgParam.type = ConsumeIcbType(fdp);
    icgParam.id = fdp.ConsumeIntegral<uint8_t>();
    icgParam.labelId = fdp.ConsumeIntegral<uint8_t>();
    icgParam.icbCnt = icbCnt;
    icgParam.connHandle = connHandle;
    (void)CM_ICGUpdateParam(&icgParam);
}

void FuzzIcbSetupDataPath(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    uint8_t codecConfigData[FUZZ_ICB_CODEC_CFG_LEN] = {0};
    uint8_t codecConfigLen = fdp.ConsumeIntegral<uint8_t>() % FUZZ_ICB_CODEC_CFG_LEN;
    for (uint8_t i = 0; i < codecConfigLen; i++) {
        codecConfigData[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    CM_ICBDataPath dataPath = {0};
    dataPath.connHandle = fdp.ConsumeIntegral<uint16_t>();
    dataPath.direction = fdp.ConsumeIntegral<uint8_t>();
    dataPath.pathId = fdp.ConsumeIntegral<uint8_t>();
    dataPath.codec.codecId = fdp.ConsumeIntegral<uint8_t>();
    dataPath.codec.vendorId = fdp.ConsumeIntegral<uint16_t>();
    dataPath.codec.vendorCodecId = fdp.ConsumeIntegral<uint16_t>();
    dataPath.controllerDelay = fdp.ConsumeIntegral<uint32_t>();
    dataPath.codecConfigLen = codecConfigLen;
    dataPath.codecConfigData = codecConfigData;
    (void)CM_ICBSetupDataPath(&dataPath);
}

void FuzzIcbRemoveDataPath(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    CM_ICBRemovedDataPath dataPath = {0};
    dataPath.connHandle = fdp.ConsumeIntegral<uint16_t>();
    dataPath.direction = fdp.ConsumeIntegral<uint8_t>();
    (void)CM_ICBRemoveDataPath(&dataPath);
}

void FuzzFreqBandSwitchEvent(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    (void)fdp;
    (void)CM_ListenFreqBandSwitchEvent(FreqBandListenerImpl);
    (void)CM_UnlistenFreqBandSwitchEvent(FreqBandListenerImpl);
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
    CM_ICBCallback cb = {0};
    cb.connectionCbk = OHOS::IcbConnectionCbkImpl;
    cb.labelReportCbk = OHOS::IcbLabelReportCbkImpl;
    cb.qualityReportCbk = OHOS::IcbQualityCbkImpl;
    (void)CM_ICBRegisterCbk(&cb);
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
    OHOS::FuzzIcgSetParam(fuzzData, size);
    OHOS::FuzzIcgSetTestParam(fuzzData, size);
    OHOS::FuzzIcgRemoveParam(fuzzData, size);
    OHOS::FuzzIcgSetLabel(fuzzData, size);
    OHOS::FuzzIcbConnection(fuzzData, size);
    OHOS::FuzzIcgUpdateParam(fuzzData, size);
    OHOS::FuzzIcbSetupDataPath(fuzzData, size);
    OHOS::FuzzIcbRemoveDataPath(fuzzData, size);
    OHOS::FuzzFreqBandSwitchEvent(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}
