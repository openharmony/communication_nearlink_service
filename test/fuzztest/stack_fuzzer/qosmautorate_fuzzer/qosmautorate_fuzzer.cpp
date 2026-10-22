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
#include "cm_api.h"
#include "qosm_autorate.h"
#include "qosm.h"
#include "qosm_trans_channel.h"
#include "nai_log.h"
#include "nlstk_init_api.h"
#include "nlstk_schedule.h"
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
    return static_cast<QOSM_QosIndex>(fdp.ConsumeIntegral<uint8_t>());
}

void FuzzQosmSetParam(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    QOSM_AutoRateParam param = {};
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
    (void)QOSM_AutoRateSetAutorateParam(&param);
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
    QOSM_ConnParam link = {};
    link.connHandle = fdp.ConsumeIntegral<uint16_t>();
    link.lcid = fdp.ConsumeIntegral<uint16_t>();
    QOSM_AutoRateConnParam param = {};
    param.qosId = fdp.ConsumeIntegral<uint8_t>();
    param.linkCnt = 1;
    param.link = &link;
    (void)QOSM_AutoRateAddConnection(&param);
    (void)QOSM_AutoRateDeleteConnection(&param);
}

void FuzzQosmAddDataPath(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    QOSM_AutoRateDataPath param = {};
    param.qosId = fdp.ConsumeIntegral<uint8_t>();
    param.connHandle = fdp.ConsumeIntegral<uint16_t>();
    param.direction = fdp.ConsumeIntegral<uint8_t>();
    param.pathId = static_cast<QOSM_PathId>(fdp.ConsumeIntegral<uint8_t>());
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
    QOSM_AutoRateDeletedDataPath param = {};
    param.qosId = fdp.ConsumeIntegral<uint8_t>();
    param.connHandle = fdp.ConsumeIntegral<uint16_t>();
    param.direction = fdp.ConsumeIntegral<uint8_t>();
    (void)QOSM_AutoRateDeleteDataPath(&param);
}

void FuzzQosmEarphoneFeedback(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    QOSM_AutoRateEarphoneFeedbackParam param = {};
    param.supportedBitrateCnt = fdp.ConsumeIntegral<uint8_t>() % QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT;
    for (uint32_t i = 0; i < param.supportedBitrateCnt; i++) {
        param.supportedBitrate[i] = fdp.ConsumeIntegral<uint16_t>();
    }
    (void)QOSM_AutoRateSetEarphoneFeedback(&param);
}

void FuzzQosmCoexistSuggestion(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    QOSM_AutoRateCoexistSuggestionParam param = {};
    param.maxBitrate = fdp.ConsumeIntegral<uint16_t>();
    param.dutyCycle = fdp.ConsumeIntegral<uint8_t>();
    (void)QOSM_AutoRateSetCoexistSuggestion(&param);
}

void FuzzQosmGetIcbParam(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    QOSM_QosIndex qosIndex = ConsumeQosIndex(fdp);
    QOSM_ICBParam param = {};
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

static void TransChannelStatusCbkImpl(const QOSM_TransChannelRspParams_S *respParams)
{
    (void)respParams;
}

static bool TransChannelEstablishedCheckCbkImpl(uint16_t srcPort)
{
    (void)srcPort;
    return true;
}

/* 单输入内一致的 qosId/connHandle 全生命周期：
 * SetParam → AddConnection → AddDataPath → 反馈/建议/消息 → DeleteDataPath
 * → DeleteConnection → RemoveParam。原 driver 各函数独立取参且 Add 后立即
 * Delete，状态无法在操作间保持。 */
void FuzzQosmConsistentLifecycle(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    uint8_t qosId = fdp.ConsumeIntegral<uint8_t>();
    uint16_t connHandle = fdp.ConsumeIntegral<uint16_t>();
    uint16_t lcid = fdp.ConsumeIntegral<uint16_t>();
    QOSM_QosIndex qosIndex = ConsumeQosIndex(fdp);
    uint8_t direction = fdp.ConsumeIntegral<uint8_t>();
    QOSM_PathId pathId = static_cast<QOSM_PathId>(fdp.ConsumeIntegral<uint8_t>());
    uint16_t bitrate = fdp.ConsumeIntegral<uint16_t>();
    uint8_t msgType = fdp.ConsumeIntegral<uint8_t>();
    uint8_t labelId = fdp.ConsumeIntegral<uint8_t>();
    uint32_t result = fdp.ConsumeIntegral<uint32_t>();
    uint8_t invalidQosId = fdp.ConsumeIntegral<uint8_t>();

    QOSM_AutoRateParam setParam = {};
    setParam.qosId = qosId;
    setParam.qosIndex = qosIndex;
    setParam.supportedBitrateCnt = 1;
    setParam.supportedBitrate[0] = bitrate;
    setParam.bitrate = bitrate;
    setParam.linkCnt = 1;
    setParam.lcidCnt = 1;
    setParam.lcid[0] = static_cast<uint8_t>(lcid);
    (void)QOSM_AutoRateSetParam(&setParam);
    (void)QOSM_AutoRateSetAutorateParam(&setParam);

    QOSM_ConnParam link = {};
    link.connHandle = connHandle;
    link.lcid = lcid;
    QOSM_AutoRateConnParam connParam = {};
    connParam.qosId = qosId;
    connParam.linkCnt = 1;
    connParam.link = &link;
    (void)QOSM_AutoRateAddConnection(&connParam);

    QOSM_AutoRateDataPath dataPath = {};
    dataPath.qosId = qosId;
    dataPath.connHandle = connHandle;
    dataPath.direction = direction;
    dataPath.pathId = pathId;
    dataPath.codecConfigLen = 0;
    dataPath.codecConfigData = nullptr;
    (void)QOSM_AutoRateAddDataPath(&dataPath);

    QOSM_AutoRateEarphoneFeedbackParam feedback = {};
    feedback.supportedBitrateCnt = 1;
    feedback.supportedBitrate[0] = bitrate;
    (void)QOSM_AutoRateSetEarphoneFeedback(&feedback);

    QOSM_AutoRateCoexistSuggestionParam suggestion = {};
    suggestion.maxBitrate = bitrate;
    suggestion.dutyCycle = direction;
    (void)QOSM_AutoRateSetCoexistSuggestion(&suggestion);

    QOSM_AutoRateRecvMsgParam msg = {};
    msg.qosId = qosId;
    msg.qosIndex = qosIndex;
    msg.labelId = labelId;
    msg.msgType = msgType;
    msg.result = result;
    (void)QOSM_RecvAutoRateMsg(&msg);

    QOSM_AutoRateDeletedDataPath delPath = {};
    delPath.qosId = qosId;
    delPath.connHandle = connHandle;
    delPath.direction = direction;
    (void)QOSM_AutoRateDeleteDataPath(&delPath);

    (void)QOSM_AutoRateDeleteConnection(&connParam);
    (void)QOSM_AutoRateRemoveParam(qosId);

    /* 保留非法路径：未建立 qoS 的操作 */
    (void)QOSM_AutoRateRemoveParam(invalidQosId);
}

void FuzzQosmTransChannelCbksRegister(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    (void)fdp;
    QOSM_TransChannelCbks_S cbks = {0};
    cbks.statusCbk = TransChannelStatusCbkImpl;
    cbks.establishedCheck = TransChannelEstablishedCheckCbkImpl;
    (void)QOSM_TransChannelCbksRegister(&cbks);
}

}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    /* ext 插件依赖真实芯片，fuzzer 进程内 InitStack 中途回滚（ScheduleDisable→
     * g_schedule=NULL）。QOSM_Init/Enable 内部经 CP_PostTask 投递，必须先重建
     * 调度器（红蓝对抗实证）；CM 回退解锁 RegisterCallback 路径的
     * CM_ICBRegisterCbk（CM_ICBIsInited 守卫）。
     * ponytail: 超时路径（qosm_autorate_timeout_process.c:150 CP_TimerAdd，
     * 1~5s 定时）走真实调度器，经 SetParam→档位变更流程后可触发（机制真实，
     * 非每输入必达），故不做 CP_TimerAdd 桩（shared_stack_source 已含
     * cp_worker.c，本地强符号桩会重复定义）；若未来需要输入字节级确定性
     * 触发，再改显式源列表排除 cp_worker.c。 */
    bool stackInited = NLSTK_InitStack() == NLSTK_ERRCODE_SUCCESS;
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    if (stackInited) {
        (void)NLSTK_EnableStack();
    } else {
        (void)ScheduleEnable();
        (void)CM_Init();
        (void)CM_Enable();
        (void)QOSM_Init();
        (void)QOSM_Enable();
    }
    QOSM_AutoRateCallback callback = {};
    callback.paramChangedCbk = OHOS::ParamChangedCbkImpl;
    callback.connChangedCbk = OHOS::ConnChangedCbkImpl;
    callback.dataPathChangedCbk = OHOS::DataPathChangedCbkImpl;
    callback.bitrateChangedCbk = OHOS::BitrateChangedCbkImpl;
    callback.frequencyBandChangedCbk = OHOS::FreqBandChangedCbkImpl;
    callback.highPowerModeChangedCbk = OHOS::HighPowerModeChangedCbkImpl;
    callback.callBitrateUpDownCbk = OHOS::AutoRateSendMsgCbkImpl;
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
    OHOS::FuzzQosmConsistentLifecycle(fuzzData, size);
    OHOS::FuzzQosmSetParam(fuzzData, size);
    OHOS::FuzzQosmRemoveParam(fuzzData, size);
    OHOS::FuzzQosmAddConnection(fuzzData, size);
    OHOS::FuzzQosmAddDataPath(fuzzData, size);
    OHOS::FuzzQosmDeleteDataPath(fuzzData, size);
    OHOS::FuzzQosmEarphoneFeedback(fuzzData, size);
    OHOS::FuzzQosmCoexistSuggestion(fuzzData, size);
    OHOS::FuzzQosmGetIcbParam(fuzzData, size);
    OHOS::FuzzQosmRecvAutoRateMsg(fuzzData, size);
    OHOS::FuzzQosmTransChannelCbksRegister(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}
