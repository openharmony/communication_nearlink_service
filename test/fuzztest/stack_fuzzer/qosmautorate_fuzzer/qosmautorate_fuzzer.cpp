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

#include "qosmautorate_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "qosm_autorate.h"
#include "nai_log.h"
#include "nlstk_init_api.h"
#include "sdf_evc.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "securec.h"

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

static void ParamChangedCbkImpl(const QOSM_ParamCb *param)
{
    (void)param;
}

static void ConnChangedCbkImpl(const QOSM_ConnParamCb *param)
{
    (void)param;
}

static void DataPathChangedCbkImpl(const QOSM_DataPathParamCb *param)
{
    (void)param;
}

static void BitrateChangedCbkImpl(const QOSM_BitrateParamCb *param, uint8_t paramCnt)
{
    (void)param;
    (void)paramCnt;
}

static void FreqBandChangedCbkImpl(const QOSM_FrequencyBandParamCb *param)
{
    (void)param;
}

static void HighPowerModeChangedCbkImpl(const QOSM_HighPowerModeParamCb *param)
{
    (void)param;
}

static void AutoRateSendMsgCbkImpl(const QOSM_AutoRateSendMsgCb *param)
{
    (void)param;
}

static QOSM_QosIndex ConsumeQosIndex(FuzzedDataProvider &fdp)
{
    return static_cast<QOSM_QosIndex>(fdp.ConsumeIntegral<uint8_t>() % QOSM_QOSINDEX_MAX);
}

void FuzzQosmSetParam(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    QOSM_AutoRateParam param = {0};
    param.qosId = fdp.ConsumeIntegral<uint8_t>();
    param.qosIndex = ConsumeQosIndex(fdp);
    param.supportedBitrateCnt = fdp.ConsumeIntegral<uint8_t>() % QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT;
    for (uint32_t i = 0; i < param.supportedBitrateCnt; i++) {
        param.supportedBitrate[i] = fdp.ConsumeIntegral<uint16_t>();
    }
    param.bitrate = fdp.ConsumeIntegral<uint16_t>();
    param.linkCnt = fdp.ConsumeIntegral<uint8_t>() % QOSM_AUTORATE_MAX_LINK_CNT;
    param.lcidCnt = fdp.ConsumeIntegral<uint8_t>() % QOSM_AUTORATE_MAX_LINK_CNT;
    for (int i = 0; i < param.lcidCnt; i++) {
        param.lcid[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    (void)QOSM_AutoRateSetParam(&param);
    (void)QOSM_AutoRateSetTestParam(&param);
}

void FuzzQosmRemoveParam(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    uint8_t qosId = fdp.ConsumeIntegral<uint8_t>();
    (void)QOSM_AutoRateRemoveParam(qosId);
}

void FuzzQosmAddConnection(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    QOSM_ConnParam link = {0};
    link.connHandle = fdp.ConsumeIntegral<uint16_t>();
    link.lcid = fdp.ConsumeIntegral<uint16_t>();
    QOSM_AutoRateConnParam param = {0};
    param.qosId = fdp.ConsumeIntegral<uint8_t>();
    param.linkCnt = 1;
    param.link = &link;
    (void)QOSM_AutoRateAddConnection(&param);
    (void)QOSM_AutoRateDeleteConnection(&param);
}

void FuzzQosmAddDataPath(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    QOSM_AutoRateDataPath param = {0};
    param.qosId = fdp.ConsumeIntegral<uint8_t>();
    param.connHandle = fdp.ConsumeIntegral<uint16_t>();
    param.direction = fdp.ConsumeIntegral<uint8_t>();
    param.pathId = static_cast<QOSM_PathId>(fdp.ConsumeIntegral<uint8_t>() % 3);
    param.codec.codecId = fdp.ConsumeIntegral<uint8_t>();
    param.codec.vendorId = fdp.ConsumeIntegral<uint16_t>();
    param.codec.vendorCodecId = fdp.ConsumeIntegral<uint16_t>();
    param.controllerDelay = fdp.ConsumeIntegral<uint32_t>();
    uint8_t codecConfigLen = fdp.ConsumeIntegral<uint8_t>() % 8;
    uint8_t codecConfigData[8] = {0};
    for (uint8_t i = 0; i < codecConfigLen; i++) {
        codecConfigData[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    param.codecConfigLen = codecConfigLen;
    param.codecConfigData = codecConfigData;
    (void)QOSM_AutoRateAddDataPath(&param);
}

void FuzzQosmDeleteDataPath(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    QOSM_AutoRateDeletedDataPath param = {0};
    param.qosId = fdp.ConsumeIntegral<uint8_t>();
    param.connHandle = fdp.ConsumeIntegral<uint16_t>();
    param.direction = fdp.ConsumeIntegral<uint8_t>();
    (void)QOSM_AutoRateDeleteDataPath(&param);
}

void FuzzQosmEarphoneFeedback(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    QOSM_AutoRateEarphoneFeedbackParam param = {0};
    param.supportedBitrateCnt = fdp.ConsumeIntegral<uint8_t>() % QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT;
    for (uint32_t i = 0; i < param.supportedBitrateCnt; i++) {
        param.supportedBitrate[i] = fdp.ConsumeIntegral<uint16_t>();
    }
    (void)QOSM_AutoRateSetEarphoneFeedback(&param);
}

void FuzzQosmCoexistSuggestion(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    QOSM_AutoRateCoexistSuggestionParam param = {0};
    param.maxBitrate = fdp.ConsumeIntegral<uint16_t>();
    param.dutyCycle = fdp.ConsumeIntegral<uint8_t>();
    (void)QOSM_AutoRateSetCoexistSuggestion(&param);
}

void FuzzQosmGetIcbParam(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    QOSM_QosIndex qosIndex = ConsumeQosIndex(fdp);
    QOSM_ICBParam param = {0};
    (void)QOSM_AutoRateGetICGG2TParam(qosIndex, &param);
    (void)QOSM_AutoRateGetICGT2GParam(qosIndex, &param);
}

void FuzzQosmRecvAutoRateMsg(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    QOSM_AutoRateRecvMsgParam param = {0};
    param.qosId = fdp.ConsumeIntegral<uint8_t>();
    param.qosIndex = ConsumeQosIndex(fdp);
    param.labelId = fdp.ConsumeIntegral<uint8_t>();
    param.msgType = fdp.ConsumeIntegral<uint8_t>();
    param.result = fdp.ConsumeIntegral<uint32_t>();
    (void)QOSM_RecvAutoRateMsg(&param);
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
    QOSM_AutoRateCallback callback = {0};
    callback.paramChangedCbk = ParamChangedCbkImpl;
    callback.connChangedCbk = ConnChangedCbkImpl;
    callback.dataPathChangedCbk = DataPathChangedCbkImpl;
    callback.bitrateChangedCbk = BitrateChangedCbkImpl;
    callback.frequencyBandChangedCbk = FreqBandChangedCbkImpl;
    callback.highPowerModeChangedCbk = HighPowerModeChangedCbkImpl;
    callback.callBitrateUpDownCbk = AutoRateSendMsgCbkImpl;
    (void)QOSM_AutoRateRegisterCallback(&callback);
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
    OHOS::FuzzQosmSetParam(fuzzData, size);
    OHOS::FuzzQosmRemoveParam(fuzzData, size);
    OHOS::FuzzQosmAddConnection(fuzzData, size);
    OHOS::FuzzQosmAddDataPath(fuzzData, size);
    OHOS::FuzzQosmDeleteDataPath(fuzzData, size);
    OHOS::FuzzQosmEarphoneFeedback(fuzzData, size);
    OHOS::FuzzQosmCoexistSuggestion(fuzzData, size);
    OHOS::FuzzQosmGetIcbParam(fuzzData, size);
    OHOS::FuzzQosmRecvAutoRateMsg(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}
