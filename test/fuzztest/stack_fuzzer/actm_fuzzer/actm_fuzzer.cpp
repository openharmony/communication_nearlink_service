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
#include <atomic>
#include <chrono>
#include <thread>

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

/* 单输入内状态一致性：创建成功后的 streamId 由事件回调异步交付，
 * 在此捕获供同一次输入内的后续 Config/Start/Open/Change/Release 复用。 */
static std::atomic<uint8_t> g_lastEvent { UCHAR_MAX };
static std::atomic<uint8_t> g_lastResult { UCHAR_MAX };
static std::atomic<uint8_t> g_streamId { 0 };

static constexpr int WAIT_EVENT_TIMEOUT_MS = 100;

static void ActmEventCbkImpl(SLE_Addr_S *addr, uint8_t eventType, uint8_t result, void *param)
{
    (void)addr;
    if (eventType == NLSTK_ACTM_EVENT_CREATE_STREAM && result == NLSTK_ACTM_SUCCESS && param != nullptr) {
        g_streamId.store(reinterpret_cast<NLSTK_ActmStreamInfo_S *>(param)->streamId);
    }
    g_lastResult.store(result);
    g_lastEvent.store(eventType);
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

/* 调度器为单线程 FIFO，任务按投递序执行；事件回调在任务内同步触发，
 * 等待目标事件即等价于等待该步任务完成（有界超时，超时走未命中分支）。 */
static bool WaitActmEvent(uint8_t event)
{
    for (int waited = 0; waited < WAIT_EVENT_TIMEOUT_MS; waited++) {
        if (g_lastEvent.load() == event) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return g_lastEvent.load() == event;
}

static void FillConfigParam(NLSTK_ActmConfigParam_S &param, uint8_t streamId, uint8_t pointType,
    NLSTK_ActmConfig_S &srcConfig, NLSTK_ActmConfig_S &sinkConfig)
{
    param.streamId = streamId;
    param.srcConfig = ((pointType & NLSTK_ACTM_SOURCE_POINT) != 0) ? &srcConfig : nullptr;
    param.sinkConfig = ((pointType & NLSTK_ACTM_SINK_POINT) != 0) ? &sinkConfig : nullptr;
    param.isImg = false;
}

void FuzzActmConsistentSequence(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    /* pointType ∈ {SOURCE, SINK, SOURCE|SINK}，保证 Config 派生点类型与创建时一致 */
    uint8_t pointType = static_cast<uint8_t>(1 + fdp.ConsumeIntegral<uint8_t>() % 3);
    NLSTK_ActmStreamParam_S createParam = {0};
    createParam.pointType = pointType;
    createParam.commType = fdp.ConsumeIntegral<uint8_t>();
    NLSTK_ActmConfig_S srcConfig;
    NLSTK_ActmConfig_S sinkConfig;
    (void)memset_s(&srcConfig, sizeof(srcConfig), 0, sizeof(srcConfig));
    (void)memset_s(&sinkConfig, sizeof(sinkConfig), 0, sizeof(sinkConfig));
    FillActmConfig(fdp, srcConfig);
    FillActmConfig(fdp, sinkConfig);
    uint8_t changeOp = static_cast<uint8_t>(fdp.ConsumeIntegral<uint8_t>() % 2); /* STOP/TRANS */
    uint8_t direction = fdp.ConsumeIntegral<uint8_t>() % 2;
    uint8_t autorateQosIndex = fdp.ConsumeIntegral<uint8_t>();
    uint8_t autorateLabelId = fdp.ConsumeIntegral<uint8_t>();
    uint8_t autorateMsgType = fdp.ConsumeIntegral<uint8_t>();
    uint32_t autorateResult = fdp.ConsumeIntegral<uint32_t>();
    uint64_t bitrate = fdp.ConsumeIntegral<uint64_t>();
    uint8_t invalidStreamId = fdp.ConsumeIntegral<uint8_t>();
    uint8_t invalidChangeOp = static_cast<uint8_t>(2 + fdp.ConsumeIntegral<uint8_t>() % 2); /* API 外非法 op */

    /* 步骤 0：ReadRemoteProp 内部 ActmCreateRemoteDevice 建立设备（FIFO 先于后续任务） */
    (void)NLSTK_ActmReadRemoteProp(&addr);

    /* 步骤 1：创建流并等待回调交付 streamId */
    g_lastEvent.store(UCHAR_MAX);
    (void)NLSTK_ActmCreateStream(&addr, &createParam);
    uint8_t streamId = 0;
    if (WaitActmEvent(NLSTK_ACTM_EVENT_CREATE_STREAM) && g_lastResult.load() == NLSTK_ACTM_SUCCESS) {
        streamId = g_streamId.load();
    }

    /* 步骤 2~8：同一 addr/streamId/pointType 贯穿配置、传输、开流、变更、速率 */
    NLSTK_ActmConfigParam_S configParam = {0};
    FillConfigParam(configParam, streamId, pointType, srcConfig, sinkConfig);
    g_lastEvent.store(UCHAR_MAX);
    (void)NLSTK_ActmConfigAudioStream(&addr, &configParam);
    (void)WaitActmEvent(NLSTK_ACTM_EVENT_CONFIG);

    g_lastEvent.store(UCHAR_MAX);
    (void)NLSTK_ActmStartAudioStream(&addr, &configParam);
    (void)WaitActmEvent(NLSTK_ACTM_EVENT_TRANS);

    NLSTK_ActmOpenParam_S openParam = {0};
    openParam.streamId = streamId;
    g_lastEvent.store(UCHAR_MAX);
    (void)NLSTK_ActmOpenAudioStream(&addr, &openParam);
    (void)WaitActmEvent(NLSTK_ACTM_EVENT_OPEN);

    NLSTK_ActmChangeParam_S changeParam = {0};
    changeParam.streamId = streamId;
    changeParam.op = changeOp;
    g_lastEvent.store(UCHAR_MAX);
    (void)NLSTK_ActmChangeAudioStream(&addr, &changeParam);
    (void)WaitActmEvent(changeOp == NLSTK_ACTM_STREAM_TRANS ? NLSTK_ACTM_EVENT_TRANS : NLSTK_ACTM_EVENT_STOP);

    NLSTK_ActmBitrateParam_S bitrateParam = {0};
    bitrateParam.streamId = streamId;
    bitrateParam.bitrate = bitrate;
    (void)NLSTK_ActmUpdateBitrate(&addr, &bitrateParam);

    NLSTK_ActmAutoRateRecvMsg_S autorateParam = {0};
    autorateParam.qosIndex = autorateQosIndex;
    autorateParam.labelId = autorateLabelId;
    autorateParam.msgType = autorateMsgType;
    autorateParam.result = autorateResult;
    (void)NLSTK_ActmRecvAutoRateMsg(&addr, &autorateParam);

    (void)NLSTK_ActmSetDirection(&addr, direction);

    /* 步骤 9：确定性清理（释放→删除→断开） */
    NLSTK_ActmReleaseParam_S releaseParam = {0};
    releaseParam.streamId = streamId;
    g_lastEvent.store(UCHAR_MAX);
    (void)NLSTK_ActmReleaseAudioStream(&addr, &releaseParam);
    (void)WaitActmEvent(NLSTK_ACTM_EVENT_RELEASE);

    g_lastEvent.store(UCHAR_MAX);
    (void)NLSTK_ActmDeleteStream(&addr, streamId);
    (void)WaitActmEvent(NLSTK_ACTM_EVENT_DELETE_STREAM);

    g_lastEvent.store(UCHAR_MAX);
    (void)NLSTK_ActmDisconnect(&addr);
    (void)WaitActmEvent(NLSTK_ACTM_EVENT_DISCONNECTED);

    /* 保留非法参数路径：未知 streamId 与 API 外 op 值 */
    NLSTK_ActmReleaseParam_S badRelease = {0};
    badRelease.streamId = invalidStreamId;
    (void)NLSTK_ActmReleaseAudioStream(&addr, &badRelease);
    NLSTK_ActmChangeParam_S badChange = {0};
    badChange.streamId = streamId;
    badChange.op = invalidChangeOp;
    (void)NLSTK_ActmChangeAudioStream(&addr, &badChange);
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
    OHOS::FuzzActmConsistentSequence(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}
