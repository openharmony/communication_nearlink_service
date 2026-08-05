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

#include "actm_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "actm_api.h"
#include "actm_api_type.h"
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

static void ActmEventCbkImpl(SLE_Addr_S *addr, uint8_t eventType, uint8_t result, void *param)
{
    (void)addr;
    (void)eventType;
    (void)result;
    (void)param;
}

static void ActmPropCbkImpl(SLE_Addr_S *addr, uint8_t num, NLSTK_ActmProp_S *prop)
{
    (void)addr;
    (void)num;
    (void)prop;
}

static void ActmBitrateCbkImpl(NLSTK_ActmBitrateChange_S *bitrate)
{
    (void)bitrate;
}

static void ActmLocationCbkImpl(SLE_Addr_S *addr, bool isLeft)
{
    (void)addr;
    (void)isLeft;
}

static void ActmStreamTypeCbkImpl(SLE_Addr_S *addr, uint32_t availableStreamType)
{
    (void)addr;
    (void)availableStreamType;
}

static void ActmCallBitUpDownCbkImpl(NLSTK_ActmAutoRateSendMsg_S *upDownParam)
{
    (void)upDownParam;
}

static void FillAddr(FuzzedDataProvider &fdp, SLE_Addr_S &addr)
{
    addr.type = fdp.ConsumeIntegral<uint8_t>();
    for (int i = 0; i < SLE_ADDR_LEN; i++) {
        addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
    }
}

static void FillActmConfig(FuzzedDataProvider &fdp, NLSTK_ActmConfig_S &config)
{
    config.pointType = fdp.ConsumeIntegral<uint8_t>();
    config.codec.codecId = fdp.ConsumeIntegral<uint8_t>();
    config.codec.companyId = fdp.ConsumeIntegral<uint16_t>();
    config.codec.vendorId = fdp.ConsumeIntegral<uint16_t>();
    config.codec.l2hc.version = fdp.ConsumeIntegral<uint8_t>();
    config.codec.l2hc.rateConf = fdp.ConsumeIntegral<uint8_t>();
    config.codec.l2hc.depthConf = fdp.ConsumeIntegral<uint8_t>();
    config.codec.l2hc.channelConf = fdp.ConsumeIntegral<uint8_t>();
    config.codec.l2hc.frameConf = fdp.ConsumeIntegral<uint8_t>();
    config.codec.l2hc.bpsConf = fdp.ConsumeIntegral<uint8_t>();
    config.codec.l2hc.bpsRange = fdp.ConsumeIntegral<uint64_t>();
    config.channel.comm = fdp.ConsumeIntegral<uint8_t>();
    config.channel.trans = fdp.ConsumeIntegral<uint8_t>();
    config.channel.qosId = fdp.ConsumeIntegral<uint8_t>();
    config.streamType = fdp.ConsumeIntegral<uint32_t>();
    config.duration = fdp.ConsumeIntegral<uint8_t>();
    config.mediaId = fdp.ConsumeIntegral<uint8_t>();
    config.src = fdp.ConsumeIntegral<uint16_t>();
    config.dst = fdp.ConsumeIntegral<uint16_t>();
}

void FuzzActmCreateStream(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    NLSTK_ActmStreamParam_S param = {0};
    FillAddr(fdp, addr);
    param.pointType = fdp.ConsumeIntegral<uint8_t>();
    param.commType = fdp.ConsumeIntegral<uint8_t>();
    (void)NLSTK_ActmCreateStream(&addr, &param);
}

void FuzzActmConfigAudioStream(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    NLSTK_ActmConfig_S srcConfig;
    NLSTK_ActmConfig_S sinkConfig;
    (void)memset_s(&srcConfig, sizeof(srcConfig), 0, sizeof(srcConfig));
    (void)memset_s(&sinkConfig, sizeof(sinkConfig), 0, sizeof(sinkConfig));
    FillActmConfig(fdp, srcConfig);
    FillActmConfig(fdp, sinkConfig);
    NLSTK_ActmConfigParam_S param = {0};
    param.groupId = fdp.ConsumeIntegral<uint32_t>();
    param.streamId = fdp.ConsumeIntegral<uint8_t>();
    param.srcConfig = &srcConfig;
    param.sinkConfig = &sinkConfig;
    param.isImg = false;
    param.encp.cryptAlgo = fdp.ConsumeIntegral<uint8_t>();
    param.encp.giv = fdp.ConsumeIntegral<uint64_t>();
    for (int i = 0; i < NLSTK_GROUP_KEY_LEN; i++) {
        param.encp.groupKey[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    (void)NLSTK_ActmConfigAudioStream(&addr, &param);
}

void FuzzActmStartAudioStream(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    NLSTK_ActmConfigParam_S param = {0};
    param.groupId = fdp.ConsumeIntegral<uint32_t>();
    param.streamId = fdp.ConsumeIntegral<uint8_t>();
    param.srcConfig = nullptr;
    param.sinkConfig = nullptr;
    param.isImg = false;
    (void)NLSTK_ActmStartAudioStream(&addr, &param);
}

void FuzzActmOpenAudioStream(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    NLSTK_ActmOpenParam_S param = {0};
    FillAddr(fdp, addr);
    param.streamId = fdp.ConsumeIntegral<uint8_t>();
    (void)NLSTK_ActmOpenAudioStream(&addr, &param);
}

void FuzzActmChangeAudioStream(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    NLSTK_ActmChangeParam_S param = {0};
    FillAddr(fdp, addr);
    param.streamId = fdp.ConsumeIntegral<uint8_t>();
    param.op = fdp.ConsumeIntegral<uint8_t>();
    (void)NLSTK_ActmChangeAudioStream(&addr, &param);
}

void FuzzActmReleaseAudioStream(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    NLSTK_ActmReleaseParam_S param = {0};
    FillAddr(fdp, addr);
    param.streamId = fdp.ConsumeIntegral<uint8_t>();
    (void)NLSTK_ActmReleaseAudioStream(&addr, &param);
}

void FuzzActmSetDirection(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    uint8_t direction = fdp.ConsumeIntegral<uint8_t>();
    (void)NLSTK_ActmSetDirection(&addr, direction);
}

void FuzzActmUpdateBitrate(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    NLSTK_ActmBitrateParam_S param = {0};
    FillAddr(fdp, addr);
    param.streamId = fdp.ConsumeIntegral<uint8_t>();
    param.bitrate = fdp.ConsumeIntegral<uint64_t>();
    (void)NLSTK_ActmUpdateBitrate(&addr, &param);
}

void FuzzActmRecvAutoRateMsg(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    NLSTK_ActmAutoRateRecvMsg_S param = {0};
    FillAddr(fdp, addr);
    param.qosId = fdp.ConsumeIntegral<uint8_t>();
    param.labelId = fdp.ConsumeIntegral<uint8_t>();
    param.qosIndex = fdp.ConsumeIntegral<uint8_t>();
    param.msgType = fdp.ConsumeIntegral<uint8_t>();
    param.result = fdp.ConsumeIntegral<uint32_t>();
    (void)NLSTK_ActmRecvAutoRateMsg(&addr, &param);
}

void FuzzActmDisconnectAndRead(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    uint8_t streamId = fdp.ConsumeIntegral<uint8_t>();
    (void)NLSTK_ActmReadRemoteProp(&addr);
    (void)NLSTK_ActmDeleteStream(&addr, streamId);
    (void)NLSTK_ActmDisconnect(&addr);
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
    NLSTK_ActmCbk_S cbk = {0};
    cbk.eventCbk = OHOS::ActmEventCbkImpl;
    cbk.propCbk = OHOS::ActmPropCbkImpl;
    cbk.bitCbk = OHOS::ActmBitrateCbkImpl;
    cbk.locationCbk = OHOS::ActmLocationCbkImpl;
    cbk.streamTypeCbk = OHOS::ActmStreamTypeCbkImpl;
    cbk.callBitUpDownCbk = OHOS::ActmCallBitUpDownCbkImpl;
    (void)NLSTK_ActmRegisterCallback(&cbk);
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
    OHOS::FuzzActmCreateStream(fuzzData, size);
    OHOS::FuzzActmConfigAudioStream(fuzzData, size);
    OHOS::FuzzActmStartAudioStream(fuzzData, size);
    OHOS::FuzzActmOpenAudioStream(fuzzData, size);
    OHOS::FuzzActmChangeAudioStream(fuzzData, size);
    OHOS::FuzzActmReleaseAudioStream(fuzzData, size);
    OHOS::FuzzActmSetDirection(fuzzData, size);
    OHOS::FuzzActmUpdateBitrate(fuzzData, size);
    OHOS::FuzzActmRecvAutoRateMsg(fuzzData, size);
    OHOS::FuzzActmDisconnectAndRead(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}
