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
#include "cm_dyn_trans_channel_api.h"
#include "cm_errno.h"
#include "cm_inner_api.h"
#include "cm_logic_link_api.h"
#include "cm_dli_adapter.h"
#include "cm_signaling_internal.h"
#include "cm_signaling_struct.h"
#include "sle_access_dli.h"
#include "cm_util.h"
#include "cp_worker.h"
#include "dli.h"
#include "dli_callback.h"
#include "dli_errno.h"
#include "dli_event.h"
#include "dtap_tcid.h"
#include "qosm_autorate.h"
#include "qosm.h"
#include "qosm_log.h"
#include "qosm_trans_channel.h"
#include "nai_log.h"
#include "nlstk_init_api.h"
#include "nlstk_schedule.h"
#include "sdf_buff.h"
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
    /* 规则013修正：uint8 全域 cast 会以非法值(0/10..255)为主，改为合法档位 1..9 为主
     * （QOSM_QOSINDEX_MAX=10，档位 8 保留未用，仍可命中）；非法值路径在调用入口单独保留。 */
    return static_cast<QOSM_QosIndex>(1 + fdp.ConsumeIntegral<uint8_t>() % (QOSM_QOSINDEX_MAX - 1));
}

void FuzzQosmSetParam(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    QOSM_AutoRateParam param = {};
    param.qosId = fdp.ConsumeIntegral<uint8_t>();
    param.qosIndex = ConsumeQosIndex(fdp);
    if (fdp.ConsumeBool()) {
        /* 保留非法参数路径：uint8 全域（含 0/10..255 越界档位） */
        param.qosIndex = static_cast<QOSM_QosIndex>(fdp.ConsumeIntegral<uint8_t>());
    }
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
    uint8_t codecConfigData[8] = {};
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
    QOSM_AutoRateRecvMsgParam param = {};
    param.qosId = fdp.ConsumeIntegral<uint8_t>();
    param.qosIndex = ConsumeQosIndex(fdp);
    param.labelId = fdp.ConsumeIntegral<uint8_t>();
    param.msgType = fdp.ConsumeIntegral<uint8_t>();
    param.result = fdp.ConsumeIntegral<uint32_t>();
    (void)QOSM_RecvAutoRateMsg(&param);
}

/* 观测层：statusCbk 计数版（不改变业务行为），记录命中次数与最后一次响应参数，
 * 供 tuning 闭环验收断言使用 */
static uint32_t g_qosStatusCbkHits = 0;
static uint32_t g_qosEstCompleteHits = 0;  // status==ESTABLISHED 的完成类回调计数
static QOSM_TransChannelRspParams_S g_qosLastRsp = {};
static bool g_qosHaveLastRsp = false;

static void TransChannelStatusCbkImpl(const QOSM_TransChannelRspParams_S *respParams)
{
    if (respParams == nullptr) {
        return;
    }
    g_qosStatusCbkHits++;
    if (respParams->status == QOSM_TRANS_CHANNEL_ESTABLISHED) {
        g_qosEstCompleteHits++;
    }
    g_qosLastRsp = *respParams;
    g_qosHaveLastRsp = true;
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
    QOSM_TransChannelCbks_S cbks = {};
    cbks.statusCbk = TransChannelStatusCbkImpl;
    cbks.establishedCheck = TransChannelEstablishedCheckCbkImpl;
    (void)QOSM_TransChannelCbksRegister(&cbks);
}

/* ============ PR-3 批次C：tuning 状态机闭环（DLI 真实分发入口直调） ============ */

/* 约束违反才报告失败：abort（libFuzzer 记为 crash），不用 assert（NDEBUG 下会被编译掉） */
#define QOSM_TUNING_CHECK(cond) \
    do {                        \
        if (!(cond)) {          \
            abort();            \
        }                       \
    } while (0)

/* 环境前置降级（PR#337 设备 ASAN 轮用户裁定）：真机/无芯片环境 InitStack 中途回退，
 * DLI 注册表不全或命令层缺口属环境可达性问题，不是产品缺陷——记录在案 + 跳过本轮
 * （置位后本目标后续输入直接跳过，验收锚点记 BLOCKED）。探测通过后其余断言保持 abort。 */
static bool g_qosEnvDegraded = false;
#define QOSM_TUNING_REQUIRE(cond)                         \
    do {                                                \
        if (!(cond)) {                                  \
            QOSM_LOGE("qosm tuning env degraded, skip");\
            g_qosEnvDegraded = true;                    \
            return;                                     \
        }                                               \
    } while (0)

/* CM 信令发送捕获：拷贝信令头+载荷后释放 buff（取得所有权防泄漏），多次覆盖存最新。
 * 目标级替身：覆盖 dtap 的 DTAP_CMSignalingDataSendCbk 注册（真实数据面非本目标被测对象，
 * 见 LLVMFuzzerInitialize 记账）。 */
struct QosmFuzzCapSig_S {
    bool valid;
    uint16_t lcid;
    uint8_t code;
    uint8_t identifier;
    uint16_t len;
    uint8_t pkt[32];
};
static QosmFuzzCapSig_S g_qosCapSig = {};

static uint32_t QosmFuzzCaptureSendCbk(uint8_t pi, uint8_t tcid, uint16_t lcid, SDF_Buff_S *buff)
{
    (void)pi;
    (void)tcid;
    if (buff == nullptr) {
        return CM_SUCCESS;
    }
    CM_SignalingHead_S *head = (CM_SignalingHead_S *)SDF_DataOffset(buff);
    g_qosCapSig.valid = true;
    g_qosCapSig.lcid = lcid;
    g_qosCapSig.code = head->code;
    g_qosCapSig.identifier = head->identifier;
    uint16_t payloadLen = head->length;
    if (payloadLen > sizeof(g_qosCapSig.pkt)) {
        payloadLen = sizeof(g_qosCapSig.pkt);
    }
    for (uint16_t i = 0; i < payloadLen; i++) {
        g_qosCapSig.pkt[i] = head->data[i];
    }
    g_qosCapSig.len = payloadLen;
    SDF_BuffFree(buff);
    return CM_SUCCESS;
}

/* DLI 事件回注：真实注册分发入口直调（fuzz 线程）。context 需包一层 DLI_ManagerContext
 * （DLI_RunRegCbk 仅透传其 cbkContext 字段），evt 即 handler 收到的 cmdRes->eventParameter。 */
static void QosmFuzzInjectCmdCbk(uint16_t regOpcode, DLI_ConnCbkContext *cbkContext, void *evt, uint32_t size,
    uint16_t status)
{
    DLI_ManagerContext mgrCtx = {};
    mgrCtx.cbkContext = cbkContext;
    DLI_RunRegCbk(regOpcode, &mgrCtx, evt, size, regOpcode, status);
}

/* 信令回注：用真实 CM_CreateSignalingBuff 构造回包，经真实接收入口 CM_RecvSignalingData 分发；
 * identifier 取自捕获的请求信令（缓存匹配要求）。buff 用后由 driver 自行释放（收包入口不释放）。 */
static void QosmFuzzInjectSignalingRsp(uint16_t lcid, uint8_t rspCode, const uint8_t *pkt, uint16_t len)
{
    SDF_Buff_S *buff = CM_CreateSignalingBuff(rspCode, g_qosCapSig.identifier, (uint8_t *)pkt, len);
    QOSM_TUNING_CHECK(buff != nullptr);
    DTAP_Data_Info_S info = {};
    info.pi = 0;
    info.lcid = lcid;
    info.tcid = TCID_SLE_CMTC;
    (void)CM_RecvSignalingData(&info, buff);
    SDF_BuffFree(buff);
}

/* CP 队列排水栅栏：阻塞等待此前 CP_PostTask 投递的任务（cbks 注册、conn update inner 等）
 * 执行完成，消除 driver 与 worker 的竞态 */
static void QosmFuzzCpNop(void *arg)
{
    (void)arg;
}

static void QosmFuzzDrainCp(void)
{
    (void)CP_PostTaskBlocked(QosmFuzzCpNop, nullptr, nullptr, 3000);
}

/* tuning 全生命周期闭环（CM-02：单输入内 addr/handle/version 复用；R4：variant 单扰动）：
 * 建链（CM_ConnectEstablishReq + DLI_CBK_CONNECT/READ_REMOTE_VERSION 回注——CONNECTED 对监听
 * 的通知发生在读版本完成事件，T 角色直达；QOSM_LogicLinkAdd 与 dyntrans tcid 池激活均于此发生）
 * → 建通道（CM_DynTransChannelEstablishReq → 捕获 TC_CONNECT_REQ → 回注 TC_CONNECT_RSP；
 *   请求 slqi=HIGH 回注时回填 rsp.slqiList.slqi[0]，与链路默认 LOW 不同 → tuning 启动）
 * → 三步推进回注（CONNECT_UPDATE/SET_PHY/SET_MCS）→ 断链清理（DISCONNECT 回注）。
 * ponytail: 11s 超时路径无注入接缝（CP_TimerAdd 真实 timerfd），BLOCKED；connUpdate 失败的
 * 产品路径取 link->status 而非 DLI status（sle_access_dli.c SleAccessLinkParamUpdateRsp），
 * 故 variant 1 用 CM_ConnectReleaseReq 置 DISCONNECTING 后回注驱动 result≠0 → Failed。 */
void FuzzQosmTuningLifecycle(const uint8_t *fuzzData, size_t size)
{
    if (g_qosEnvDegraded) {
        return; /* 环境已降级：本轮记 BLOCKED 跳过 */
    }
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {};
    addr.type = fdp.ConsumeIntegral<uint8_t>();
    for (size_t i = 0; i < sizeof(addr.addr); i++) {
        addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    uint16_t handle = fdp.ConsumeIntegral<uint16_t>();
    uint8_t version = fdp.ConsumeIntegral<uint8_t>();
    uint16_t localIndex = fdp.ConsumeIntegral<uint16_t>();
    uint8_t variant = fdp.ConsumeIntegralInRange<uint8_t>(0, 6);

    /* 每输入观测清零；排水确保本输入 FuzzQosmTransChannelCbksRegister 投递的注册已落地 */
    g_qosStatusCbkHits = 0;
    g_qosEstCompleteHits = 0;
    g_qosHaveLastRsp = false;
    g_qosCapSig = {};
    QosmFuzzDrainCp();

    /* 注册表非空证明：四个回注 opcode 均有真实 handler */
    QOSM_TUNING_REQUIRE(DLI_GetCbk(DLI_CBK_CONNECT) != nullptr);
    QOSM_TUNING_REQUIRE(DLI_GetCbk(DLI_CBK_CONNECT_UPDATE) != nullptr);
    QOSM_TUNING_REQUIRE(DLI_GetCbk(DLI_CBK_SET_PHY) != nullptr);
    QOSM_TUNING_REQUIRE(DLI_GetCbk(DLI_CBK_SET_MCS) != nullptr);

    DLI_ConnCbkContext cbkCtx = {};
    cbkCtx.versionAndLocalIndex = CM_PackVersionLocalIndex(version, localIndex);
    cbkCtx.connHandle = handle;

    /* 步骤1 建链：真实建链请求（同步建 CONNECTING 节点）+ 连接完成事件回注（SCAN 主动完成、T 角色） */
    CM_ConnectParamReq_S connReq = {};
    connReq.version = version;
    connReq.localIndex = localIndex;
    connReq.addr = addr;
    QOSM_TUNING_REQUIRE(CM_ConnectEstablishReq(&connReq) == CM_SUCCESS);

    DLI_ConnectionCompleteEvt connEvt = {};
    connEvt.status = DLI_SUCCESS;
    connEvt.connHandle = handle;
    connEvt.role = CM_T_NODE;
    connEvt.peerAddressType = addr.type;
    for (size_t i = 0; i < sizeof(addr.addr); i++) {
        connEvt.peerAddress[i] = addr.addr[i];
    }
    connEvt.connCompleteType = CM_CONN_COMPLETE_SCAN;
    connEvt.connectionInterval = 0x24;
    connEvt.supervisionTimeout = 1000;
    QosmFuzzInjectCmdCbk(DLI_CBK_CONNECT, &cbkCtx, &connEvt, sizeof(connEvt), DLI_SUCCESS);

    DLI_ReadRemoteVersionEvt verEvt = {};
    verEvt.status = DLI_SUCCESS;
    verEvt.connHandle = handle;
    QosmFuzzInjectCmdCbk(DLI_CBK_READ_REMOTE_VERSION, &cbkCtx, &verEvt, sizeof(verEvt), DLI_SUCCESS);

    CM_LogicLink_S logicLink = {};
    QOSM_TUNING_REQUIRE(CM_GetLogicLinkByAddr(&addr, &logicLink) == CM_SUCCESS && logicLink.lcid == handle);

    /* 步骤2 建通道：STREAM/UNICAST/slqi=HIGH（≠链路默认 LOW → tuning 启动条件） */
    CM_DynTransChannelEstablishParamReq_S estReq = {};
    estReq.version = version;
    estReq.localIndex = localIndex;
    estReq.addr = addr;
    estReq.srcPort = 0x1001;
    estReq.dstPort = 0x1002;
    estReq.transModeConfig.transMode = CM_TRANS_MODE_STREAM;
    estReq.transModeConfig.mtu = 0x2000;
    estReq.transModeConfig.mps = 0x2000;
    estReq.transModeConfig.streamMode.flushTimeout = 0x100;
    estReq.transModeConfig.streamMode.reorderTimeout = 0x100;
    estReq.transModeConfig.streamMode.crcInit = 0x5555;
    estReq.slqi = QOSM_TRANS_CHANNEL_SLQI_HIGH;
    estReq.expectedTransportMode = CM_ACCESS_TRANS_MODE_UNICAST;
    estReq.exclusive = true;
    estReq.frameType = CM_SLE_RADIO_FRAME_TYPE_2;  // 帧4 会被 QOSM 排除 tuning，用帧2
    QOSM_TUNING_REQUIRE(CM_DynTransChannelEstablishReq(&estReq) == CM_SUCCESS);
    /* 捕获 TC_CONNECT_REQ（CONNECTED 通知触发的 CAPABILITY_REQ 会被本捕获覆盖，以最新为准） */
    QOSM_TUNING_REQUIRE(g_qosCapSig.valid && g_qosCapSig.code == TC_CONNECT_REQ && g_qosCapSig.len > 0);
    uint8_t localTcid = g_qosCapSig.pkt[0];  // CM_TransChanEstablishReqPkt_S 首字段 srcTcid
    uint8_t peerTcid = (localTcid == 0x81) ? 0x80 : 0x81;  // 对端 tcid 动态范围 [0x80,0xDF] 内任取 ≠本端
    CM_TransChanEstablishRspPkt_S estRsp = {};
    estRsp.srcTcid = localTcid;
    estRsp.dstTcid = peerTcid;
    estRsp.result = CM_RESULT_ESTABLISH_SUCCESS;
    estRsp.lc = 0;
    QosmFuzzInjectSignalingRsp(handle, TC_CONNECT_RSP, (const uint8_t *)&estRsp, sizeof(estRsp));
    /* 回注成功 → QOSM_TransChannelEstablishRspCbk → 通道入链表 → tuning 启动进入 WaitConnUpdate；
     * 此时无 statusCbk（g_qosStatusCbkHits 允许为 0），真正的验收在步骤3 */
    QosmFuzzDrainCp();

    DLI_ConnectionUpdateCmpEvt updEvt = {};
    DLI_SetPhyEvt phyEvt = {};
    switch (variant) {
        case 0: {  // 全成功：Idle→WaitConnUpdate→WaitSetPhy→WaitSetMcs→Complete
            updEvt.status = DLI_SUCCESS;
            updEvt.connHandle = handle;
            updEvt.connInterval = 0x24;
            updEvt.maxLatency = 0;
            updEvt.supervisionTimeout = 1000;
            QosmFuzzInjectCmdCbk(DLI_CBK_CONNECT_UPDATE, &cbkCtx, &updEvt, sizeof(updEvt), DLI_SUCCESS);
            phyEvt.status = DLI_SUCCESS;
            phyEvt.connHandle = handle;
            QosmFuzzInjectCmdCbk(DLI_CBK_SET_PHY, &cbkCtx, &phyEvt, sizeof(phyEvt), DLI_SUCCESS);
            QosmFuzzInjectCmdCbk(DLI_CBK_SET_MCS, &cbkCtx, nullptr, 0, DLI_SUCCESS);  // lcid 取 context
            QOSM_TUNING_CHECK(g_qosStatusCbkHits >= 1 && g_qosHaveLastRsp &&
                g_qosLastRsp.status == QOSM_TRANS_CHANNEL_ESTABLISHED && g_qosLastRsp.lcid == handle);
            break;
        }
        case 1: {  // connUpdate 失败：回注结果取 link->status，置 DISCONNECTING 后回注驱动 Failed
            CM_DisconnectParamReq_S relReq = {};
            relReq.addr = addr;
            relReq.discReason = CM_DISC_REASON_REMOTE_USER_TERMINATED;
            QOSM_TUNING_REQUIRE(CM_ConnectReleaseReq(&relReq) == CM_SUCCESS);
            updEvt.status = DLI_SUCCESS;
            updEvt.connHandle = handle;
            QosmFuzzInjectCmdCbk(DLI_CBK_CONNECT_UPDATE, &cbkCtx, &updEvt, sizeof(updEvt), DLI_SUCCESS);
            QOSM_TUNING_CHECK(g_qosStatusCbkHits >= 1 && g_qosHaveLastRsp &&
                g_qosLastRsp.status == QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
            break;
        }
        case 2: {  // setPhy 失败：connUpdate 成功 → SET_PHY status≠0（错误分支 evt 被忽略）
            updEvt.status = DLI_SUCCESS;
            updEvt.connHandle = handle;
            QosmFuzzInjectCmdCbk(DLI_CBK_CONNECT_UPDATE, &cbkCtx, &updEvt, sizeof(updEvt), DLI_SUCCESS);
            QosmFuzzInjectCmdCbk(DLI_CBK_SET_PHY, &cbkCtx, nullptr, 0, 1);
            QOSM_TUNING_CHECK(g_qosStatusCbkHits >= 1 && g_qosHaveLastRsp &&
                g_qosLastRsp.status == QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
            break;
        }
        case 3: {  // setMcs 失败：前两步成功 → SET_MCS status≠0
            updEvt.status = DLI_SUCCESS;
            updEvt.connHandle = handle;
            QosmFuzzInjectCmdCbk(DLI_CBK_CONNECT_UPDATE, &cbkCtx, &updEvt, sizeof(updEvt), DLI_SUCCESS);
            phyEvt.status = DLI_SUCCESS;
            phyEvt.connHandle = handle;
            QosmFuzzInjectCmdCbk(DLI_CBK_SET_PHY, &cbkCtx, &phyEvt, sizeof(phyEvt), DLI_SUCCESS);
            QosmFuzzInjectCmdCbk(DLI_CBK_SET_MCS, &cbkCtx, nullptr, 0, 1);
            QOSM_TUNING_CHECK(g_qosStatusCbkHits >= 1 && g_qosHaveLastRsp &&
                g_qosLastRsp.status == QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
            break;
        }
        case 4: {  // 乱序：先注入 SET_PHY（WaitConnUpdate 态被 FilterRspAndCheckActive 丢弃）→ 正常三步仍可完成
            phyEvt.status = DLI_SUCCESS;
            phyEvt.connHandle = handle;
            QosmFuzzInjectCmdCbk(DLI_CBK_SET_PHY, &cbkCtx, &phyEvt, sizeof(phyEvt), DLI_SUCCESS);
            updEvt.status = DLI_SUCCESS;
            updEvt.connHandle = handle;
            QosmFuzzInjectCmdCbk(DLI_CBK_CONNECT_UPDATE, &cbkCtx, &updEvt, sizeof(updEvt), DLI_SUCCESS);
            QosmFuzzInjectCmdCbk(DLI_CBK_SET_PHY, &cbkCtx, &phyEvt, sizeof(phyEvt), DLI_SUCCESS);
            QosmFuzzInjectCmdCbk(DLI_CBK_SET_MCS, &cbkCtx, nullptr, 0, DLI_SUCCESS);
            QOSM_TUNING_CHECK(g_qosEstCompleteHits == 1 && g_qosHaveLastRsp &&
                g_qosLastRsp.status == QOSM_TRANS_CHANNEL_ESTABLISHED && g_qosLastRsp.lcid == handle);
            break;
        }
        case 5: {  // 重复：connUpdate 成功后重复注入一次（WaitSetPhy 态被状态过滤）→ 仍只完成一次
            updEvt.status = DLI_SUCCESS;
            updEvt.connHandle = handle;
            QosmFuzzInjectCmdCbk(DLI_CBK_CONNECT_UPDATE, &cbkCtx, &updEvt, sizeof(updEvt), DLI_SUCCESS);
            QosmFuzzInjectCmdCbk(DLI_CBK_CONNECT_UPDATE, &cbkCtx, &updEvt, sizeof(updEvt), DLI_SUCCESS);
            phyEvt.status = DLI_SUCCESS;
            phyEvt.connHandle = handle;
            QosmFuzzInjectCmdCbk(DLI_CBK_SET_PHY, &cbkCtx, &phyEvt, sizeof(phyEvt), DLI_SUCCESS);
            QosmFuzzInjectCmdCbk(DLI_CBK_SET_MCS, &cbkCtx, nullptr, 0, DLI_SUCCESS);
            QOSM_TUNING_CHECK(g_qosStatusCbkHits == 1 && g_qosHaveLastRsp &&
                g_qosLastRsp.status == QOSM_TRANS_CHANNEL_ESTABLISHED && g_qosLastRsp.lcid == handle);
            break;
        }
        case 6: {  // 晚到（R3）：断链后再注入 connUpdate——晚到旧事件不得推动已停止的 stm
            DLI_DisconnectEvt discEvt = {};
            discEvt.status = DLI_SUCCESS;
            discEvt.connHandle = handle;
            discEvt.reason = CM_DISC_REASON_REMOTE_USER_TERMINATED;
            QosmFuzzInjectCmdCbk(DLI_CBK_DISCONNECT, &cbkCtx, &discEvt, sizeof(discEvt), DLI_SUCCESS);
            updEvt.status = DLI_SUCCESS;
            updEvt.connHandle = handle;
            QosmFuzzInjectCmdCbk(DLI_CBK_CONNECT_UPDATE, &cbkCtx, &updEvt, sizeof(updEvt), DLI_SUCCESS);
            QOSM_TUNING_CHECK(g_qosEstCompleteHits == 0);  // 未出现 ESTABLISHED 完成回调
            break;
        }
        default:
            break;
    }

    /* Failed 分支（variant 1/2/3）已自动发 CM_DynTransChannelReleaseReq：回注 RSP result=0 完成释放。
     * 放在断言之后——释放成功回调 statusCbk(RELEASED) 会覆盖 g_qosLastRsp。 */
    if (variant <= 3 && g_qosCapSig.valid && g_qosCapSig.code == TC_DISCONNECT_REQ) {
        CM_TransChanReleaseRspPkt_S relRsp = {};
        relRsp.srcTcid = localTcid;
        relRsp.dstTcid = peerTcid;
        relRsp.result = CM_RESULT_RELEASE_SUCCESS;
        QosmFuzzInjectSignalingRsp(handle, TC_DISCONNECT_RSP, (const uint8_t *)&relRsp, sizeof(relRsp));
    }

    /* 步骤4 清理：链路仍在则注入断链事件（释放 QOSM 链路/通道与 dyntrans tcid 池，兼清理信令缓存）；
     * 已删则处理器空转返回，天然幂等 */
    if (CM_GetLogicLinkByAddr(&addr, &logicLink) == CM_SUCCESS) {
        DLI_DisconnectEvt discEvt = {};
        discEvt.status = DLI_SUCCESS;
        discEvt.connHandle = handle;
        discEvt.reason = CM_DISC_REASON_REMOTE_USER_TERMINATED;
        QosmFuzzInjectCmdCbk(DLI_CBK_DISCONNECT, &cbkCtx, &discEvt, sizeof(discEvt), DLI_SUCCESS);
        QosmFuzzDrainCp();
    }
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
    if (!stackInited) {
        /* 回退链 DpDeinit 已拆掉 DLI 线程与 CM 信令缓存（产品由 NLSTK_InitStack 全链重建，
         * 上述回退只重建了 CM/QOSM）。按产品 DliCallbackInit/DLI_Init/CM_SignalingInit 顺序补齐，
         * 保证 DLI 命令受理（CM_ConnectEstablishReq/UpdateParamReq/SetPhy/SetMcs 返回成功）
         * 与 dyntrans 信令缓存可用。注意：回退时 CM_InitInner 里的 CM_DliAdapterInit/
         * SleAccessRegCbks 因 DLI 回调已被复位为 NULL 而静默失败（返回值被忽略），
         * g_cbkList 为空——DLI 回调重建后必须重注册，否则 DLI_GetCbk 全空、注入无分发。
         * ponytail: dli 队列重建后未做 layer enable（DLI_Enable 依赖 HAL 真实芯片信号量
         * 等待），命令入队后被 worker 丢弃（g_info.inited=false）——命令侧"真实 dli→
         * SleSendDliPacket 桩" 仅达入队层；响应侧全部由 driver 经 DLI_RunRegCbk 真实分发
         * 入口回注，不依赖发送侧，状态机行为不受影响。 */
        DLI_Callback dliCbk = {};
        dliCbk.postOtherThread = SchedulePostTask;
        dliCbk.postOtherBlockedThread = SchedulePostTaskBlocked;
        (void)DLI_SetCallback(&dliCbk);
        (void)DLI_Init();
        (void)CM_DliAdapterInit();  // 重注册 DLI_CBK_CONNECT/DISCONNECT 分发表
        SleAccessRegCbks();         // 重注册 CM 链路类 DLI 回调（CONNECT_UPDATE/SET_PHY/SET_MCS 等）
        (void)CM_SignalingInit();   // 信令缓存+CM 信令监听（产品侧由 DTAP_Init 完成）
    }
    /* 目标级替身：覆盖 dtap 注册的 DTAP_CMSignalingDataSendCbk，捕获 CM 信令供回注（真实数据面
     * 非本目标被测对象）。两种 init 分支下均需覆盖。 */
    CM_SetSendSignalingDataCbk(OHOS::QosmFuzzCaptureSendCbk);
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
    OHOS::FuzzQosmTuningLifecycle(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}
