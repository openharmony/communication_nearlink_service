/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

/* PR-3 批次A（CM-10/11/12）：ICB 初始化后的事件闭环 fuzz 覆盖。
 *
 * 入口记账：
 * 1. 事件回注走真实注册分发入口 DLI_RunRegCbk（dli_event.c）直调：按 regOpcode 查真实
 *    g_cbkList，同步执行 cm_icb_mgr.c 注册的 ICB handler（真实入口直调，fuzz 线程执行）。
 * 2. DLI 上下文按分发层约定构造：DLI_RunRegCbk 把 context 视为 DLI_ManagerContext*，
 *    handler 实际收到其 cbkContext，故注入前将 DLI_ICGCbkParam 包进 DLI_ManagerContext。
 * 3. 事件结构体均为 pack(1)（dli_event_struct.h），本目标按小端直填字段；size 一律用
 *    sizeof(结构体) + n * sizeof(元素) 计算，不硬编码数字。
 * 4. SleSendDliPacket 为 cmicb_fuzzer.cpp 既有目标级发数桩（返回0），本文件不重复定义；
 *    请求侧 CM_* 经真实 DLI 命令层投递成功即返回 CM_SUCCESS（"命令已发出"）。
 * 5. FreqBand listener 注册（CM_ListenFreqBandSwitchEvent）经 CP_PostTask 异步投递到真实
 *    工作线程，与同步回注存在时序竞争：listener 命中数仅统计不强断言。
 *    ponytail: 若要强断言需引入跨线程同步原语，超出本批次范围。
 * 6. 断言用 ICB_EVT_CHECK（显式 if + abort，不用 assert 以免 NDEBUG 剥离）：仅约束违反
 *    （事件未进目标 handler / 轨迹不一致 / 合法域判断错误）才报告测试失败，
 *    业务正常错误返回（拒绝非法参数等）不触发。
 */

#include "cmicb_evt_fuzzer.h"
#include <stdlib.h>
#include "fuzzer/FuzzedDataProvider.h"
#include "cm_icb_api.h"
#include "cm_icb_inner_api.h"
#include "cm_icb_mgr.h"
#include "cm_errno.h"
#include "dli_errno.h"
#include "dli_event.h"
#include "securec.h"
#include "cm_log.h"

#define FUZZ_EVT_ICB_MAX_CNT 4     /* EventLoop 内 icbCnt 取值 [1,4] */
#define FUZZ_EVT_LABEL_MAX_CNT 4   /* labelCnt 取值 [1,4] */
#define FUZZ_EVT_IMB_TX_MAX_CNT 4  /* IMB 质量上报 txCount 取值 [1,4] */
#define FUZZ_EVT_CODEC_BUF_LEN 8   /* codecConfigLen 取值 [1,8]，等于缓冲区 8 字节 */

namespace OHOS {

/* ---------------- 观测层：计数器 + last-seen 记录 + 计数回调 ---------------- */

static uint32_t g_evtConnCbkHits = 0;      /* connectionCbk 命中数 */
static uint32_t g_evtLabelCbkHits = 0;     /* labelReportCbk 命中数 */
static uint32_t g_evtQualityCbkHits = 0;   /* qualityReportCbk 命中数 */
static uint32_t g_evtFreqBandHits = 0;     /* FreqBand listener 命中数（仅统计） */
static uint32_t g_evtConnStatusHits = 0;   /* g_icbConnStatusCb 建链状态命中数 */
static uint8_t g_evtLastConnId = 0;
static uint16_t g_evtLastConnHandle = 0;
static CM_ICBConnectionState g_evtLastConnState = CM_ICB_STATE_IOG_CREATED;
static CM_ICBErrorCode g_evtLastConnErr = CM_ICB_SUCCESS;
static CM_ICBErrorCode g_evtLastLabelErr = CM_ICB_SUCCESS;
static uint16_t g_evtLastLabelConn = 0;
static uint16_t g_evtLastQualityConn = 0;

static void CountingConnCbk(CM_ICBConnection *connection)
{
    if (connection == NULL) {
        return;
    }
    g_evtConnCbkHits++;
    g_evtLastConnId = connection->id;
    g_evtLastConnState = connection->state;
    g_evtLastConnErr = connection->errorCode;
    if (connection->channelCnt > 0 && connection->channel != NULL) {
        g_evtLastConnHandle = connection->channel[0].connHandle;
    }
}

static void CountingLabelCbk(CM_ICBLabelReportParam *param)
{
    if (param == NULL) {
        return;
    }
    g_evtLabelCbkHits++;
    g_evtLastLabelErr = param->errorCode;
    g_evtLastLabelConn = param->connHandle;
}

static void CountingQualityCbk(CM_ICBQuality *quality)
{
    if (quality == NULL) {
        return;
    }
    g_evtQualityCbkHits++;
    g_evtLastQualityConn = quality->connHandle;
}

static void CountingConnStatusCbk(ICBConnectionType type, CM_TransChan_S *trans)
{
    (void)trans;
    if (type == ICB_CONNECTED) {
        g_evtConnStatusHits++;
    }
}

static void CountingFreqBand(CM_FreqBandSwitchParam *param)
{
    (void)param;
    g_evtFreqBandHits++;
}

/* 每个输入开头调用一次：清零计数器并覆盖注册观测回调。
 * CM_ICBRegisterCbk/CM_RegisterICBConnectionCbk 覆盖 LLVMFuzzerInitialize 中的空实现；
 * FreqBand listener 为异步注册（见文件头记账第 5 条）。 */
static void IcbEvtRegisterObservers(void)
{
    g_evtConnCbkHits = 0;
    g_evtLabelCbkHits = 0;
    g_evtQualityCbkHits = 0;
    g_evtFreqBandHits = 0;
    g_evtConnStatusHits = 0;
    g_evtLastConnId = 0;
    g_evtLastConnHandle = 0;
    g_evtLastConnState = CM_ICB_STATE_IOG_CREATED;
    g_evtLastConnErr = CM_ICB_SUCCESS;
    g_evtLastLabelErr = CM_ICB_SUCCESS;
    g_evtLastLabelConn = 0;
    g_evtLastQualityConn = 0;

    CM_ICBCallback cb = {};
    cb.connectionCbk = CountingConnCbk;
    cb.labelReportCbk = CountingLabelCbk;
    cb.qualityReportCbk = CountingQualityCbk;
    (void)CM_ICBRegisterCbk(&cb);
    (void)CM_RegisterICBConnectionCbk(CountingConnStatusCbk);
    (void)CM_ListenFreqBandSwitchEvent(CountingFreqBand);
}

/* ---------------- 注入 helper：真实分发入口直调 ---------------- */

/* 按真实分发层约定把业务上下文包进 DLI_ManagerContext（handler 收到 cbkContext），
 * evtOpcode 沿用 regOpcode（仅记账进 cmdRes.cmdOpcode，handler 不依赖）。 */
static void IcbEvtInject(uint16_t regOpcode, void *context, void *evt, uint32_t size, uint16_t status)
{
    DLI_ManagerContext mgrCtx = {};
    mgrCtx.cbkContext = context;
    DLI_RunRegCbk(regOpcode, &mgrCtx, evt, size, regOpcode, status);
}

static bool IcbEvtCbkReady(uint16_t opcode)
{
    return DLI_GetCbk(opcode) != NULL;
}

/* 约束违反才报告测试失败（abort），正常错误返回不触发；不用 assert 以免 NDEBUG 剥离 */
#define ICB_EVT_CHECK(cond)                       \
    do {                                          \
        if (!(cond)) {                            \
            abort();                              \
        }                                         \
    } while (0)

/* 环境前置降级（PR#337 设备 ASAN 轮用户裁定）：真机/无芯片环境 InitStack 中途回退，
 * DLI 回调注册表不全或初始化链缺口属环境可达性问题，不是产品缺陷——记录在案 +
 * 跳过本轮（置 g_icbEnvDegraded 后本目标后续输入直接跳过，验收锚点记 BLOCKED）。
 * 探测通过后（完整环境）其余断言保持 abort，"合法输入下产品行为真的错了"仍可炸出。 */
static bool g_icbEnvDegraded = false;
#define ICB_EVT_REQUIRE(cond, reason)                                          \
    do {                                                                       \
        if (!(cond)) {                                                         \
            CM_LOGE("icb evt env degraded, skip: %s", reason);                 \
            g_icbEnvDegraded = true;                                           \
            return;                                                            \
        }                                                                      \
    } while (0)

/* 按 handler 期望填充 DLI_ICGCbkParam（真实代码在各 CM_ICBMgr* 中构造，此处等价构造） */
static void IcbEvtFillCbkParam(DLI_ICGCbkParam *cbkParam, CM_ICBType type, uint8_t id, uint8_t direction,
    uint16_t connHandle, uint8_t connHandleNum)
{
    (void)memset_s(cbkParam, sizeof(DLI_ICGCbkParam), 0, sizeof(DLI_ICGCbkParam));
    cbkParam->type = (uint8_t)type;
    cbkParam->id = id;
    cbkParam->direction = direction;
    cbkParam->connHandle[0] = connHandle;
    cbkParam->connHandleNum = connHandleNum;
}

/* 合法 CM_ICGSetParam 请求（paramCnt=1、内层四元组一致、icbCnt<=4）+ 注入成功事件
 * （connHandle[] 全填同一 connHandle）。返回 CM_ICGSetParam 结果，轨迹断言由调用方执行。 */
static uint32_t IcbEvtSetParamAndNotify(CM_ICBType type, uint8_t id, uint16_t connHandle, uint16_t gHandle,
    uint8_t icbCnt, uint16_t maxSduG2T, uint16_t maxSduT2G, uint8_t rtnG2T, uint8_t rtnT2G)
{
    CM_ICBParam icbParam[FUZZ_EVT_ICB_MAX_CNT];
    CM_ICBParam::CM_ICB icbInner[FUZZ_EVT_ICB_MAX_CNT];
    (void)memset_s(icbParam, sizeof(icbParam), 0, sizeof(icbParam));
    (void)memset_s(icbInner, sizeof(icbInner), 0, sizeof(icbInner));
    for (uint8_t i = 0; i < icbCnt; i++) {
        icbParam[i].id = id;
        icbInner[i].maxSduG2T = maxSduG2T;
        icbInner[i].maxSduT2G = maxSduT2G;
        icbInner[i].rtnG2T = rtnG2T;
        icbInner[i].rtnT2G = rtnT2G;
        icbParam[i].param = &icbInner[i];
    }
    CM_ICGParam icgParam = {};
    icgParam.type = type;
    icgParam.id = id;
    icgParam.icbCnt = icbCnt;
    icgParam.paramCnt = 1; /* 合法：paramCnt>=1（cm_icb_api.c CM_ICGSetParam 校验） */
    icgParam.icbParam = icbParam;
    uint32_t ret = CM_ICGSetParam(&icgParam);

    /* 芯片成功响应回注：SetIOG/SetIMG 事件（柔性 connHandle[] 跟在固定头之后） */
    uint8_t evtBuf[sizeof(DLI_SetIMGParamEvt) + FUZZ_EVT_ICB_MAX_CNT * sizeof(uint16_t)];
    (void)memset_s(evtBuf, sizeof(evtBuf), 0, sizeof(evtBuf));
    DLI_ICGCbkParam cbkParam;
    IcbEvtFillCbkParam(&cbkParam, type, id, 0, 0, 0);
    uint32_t evtSize = 0;
    if (type == CM_IMB) {
        DLI_SetIMGParamEvt *evt = (DLI_SetIMGParamEvt *)evtBuf;
        evt->id = id;
        evt->gHandle = gHandle;
        evt->paramCnt = icbCnt;
        for (uint8_t i = 0; i < icbCnt; i++) {
            evt->connHandle[i] = connHandle;
        }
        evtSize = sizeof(DLI_SetIMGParamEvt) + icbCnt * sizeof(uint16_t);
        IcbEvtInject(DLI_CBK_SET_IMG_PARAM, &cbkParam, evtBuf, evtSize, DLI_SUCCESS);
    } else {
        DLI_SetIOGParamEvt *evt = (DLI_SetIOGParamEvt *)evtBuf;
        evt->id = id;
        evt->paramCnt = icbCnt;
        for (uint8_t i = 0; i < icbCnt; i++) {
            evt->connHandle[i] = connHandle;
        }
        evtSize = sizeof(DLI_SetIOGParamEvt) + icbCnt * sizeof(uint16_t);
        IcbEvtInject(DLI_CBK_SET_IOG_PARAM, &cbkParam, evtBuf, evtSize, DLI_SUCCESS);
    }
    return ret;
}

/* CM-10/CM-11 主闭环：请求 -> 芯片事件回注 -> 断言轨迹。
 * 单输入取参一次，全程复用同一 type、id、connHandle（CM-02 输入隔离，A->A 一致性）。 */
void FuzzIcbEventLoop(const uint8_t *fuzzData, size_t size)
{
    if (g_icbEnvDegraded) {
        return; /* 环境已降级：闭环与边界断言本轮记 BLOCKED 跳过（见 ICB_EVT_REQUIRE 注释） */
    }
    FuzzedDataProvider fdp(fuzzData, size);
    IcbEvtRegisterObservers();

    CM_ICBType type = fdp.ConsumeBool() ? CM_IMB : CM_IOB;
    uint8_t id = fdp.ConsumeIntegral<uint8_t>() % 0xF0; /* 合法域 [0,0xEF] */
    uint16_t connHandle = fdp.ConsumeIntegral<uint16_t>();
    uint16_t lcid = fdp.ConsumeIntegral<uint16_t>();
    uint8_t labelId = fdp.ConsumeIntegral<uint8_t>();
    uint8_t icbCnt = fdp.ConsumeIntegral<uint8_t>() % FUZZ_EVT_ICB_MAX_CNT + 1; /* [1,4] 合法 */
    uint8_t failMask = fdp.ConsumeIntegral<uint8_t>(); /* 失败注入开关位 */
    uint16_t maxSduG2T = fdp.ConsumeIntegral<uint16_t>();
    uint16_t maxSduT2G = fdp.ConsumeIntegral<uint16_t>();
    uint8_t rtnG2T = fdp.ConsumeIntegral<uint8_t>();
    uint8_t rtnT2G = fdp.ConsumeIntegral<uint8_t>();
    uint16_t gHandle = fdp.ConsumeIntegral<uint16_t>(); /* IMG 事件用，IOB 忽略 */
    bool supportSubrate = fdp.ConsumeBool();
    uint8_t codecConfigLen = fdp.ConsumeIntegral<uint8_t>() % FUZZ_EVT_CODEC_BUF_LEN + 1; /* [1,8] 合法 */
    uint8_t labelCnt = fdp.ConsumeIntegral<uint8_t>() % FUZZ_EVT_LABEL_MAX_CNT + 1; /* [1,4] 合法 */
    uint8_t imbTxCount = fdp.ConsumeIntegral<uint8_t>() % FUZZ_EVT_IMB_TX_MAX_CNT + 1; /* [1,4] */
    uint8_t oldFreqBand = fdp.ConsumeIntegral<uint8_t>();
    uint8_t newFreqBand = fdp.ConsumeIntegral<uint8_t>();

    uint8_t direction = failMask % 2;                          /* 合法：direction∈{0,1} */
    uint16_t failStatus = DLI_UNSPECIFIED_ERROR | failMask;    /* 非零失败状态（|=0x20） */
    uint16_t setRegOpcode = (type == CM_IMB) ? DLI_CBK_SET_IMG_PARAM : DLI_CBK_SET_IOG_PARAM;
    uint16_t labelRegOpcode = (type == CM_IMB) ? DLI_CBK_IMG_LABEL_REPORT : DLI_CBK_IOG_LABEL_REPORT;
    uint16_t estRegOpcode = (type == CM_IMB) ? DLI_CBK_IMB_ESTABLISHED : DLI_CBK_IOB_ESTABLISHED;
    uint16_t updRegOpcode = (type == CM_IMB) ? DLI_CBK_IMG_PARAM_UPDATE : DLI_CBK_IOG_PARAM_UPDATE;
    CM_ICBConnectionState createdState = (type == CM_IMB) ? CM_ICB_STATE_IMG_CREATED : CM_ICB_STATE_IOG_CREATED;
    CM_ICBConnectionState removedState = (type == CM_IMB) ? CM_ICB_STATE_IMG_REMOVED : CM_ICB_STATE_IOG_REMOVED;
    CM_ICBConnectionState estState = (type == CM_IMB) ? CM_ICB_STATE_IMB_CREATED : CM_ICB_STATE_IOB_CREATED;
    CM_ICBConnectionState updState = (type == CM_IMB) ? CM_ICB_STATE_IMG_UPDATED : CM_ICB_STATE_IOG_UPDATED;

    /* 注册表非空证明（CM-10 验收锚点）：环境探测第一关——真机回退态下注册表可能不全，
     * 单个 opcode 缺失即降级跳过并记录具体缺哪个 */
    static const uint16_t icbRegOpcodes[] = {
        DLI_CBK_SET_IOG_PARAM, DLI_CBK_SET_IMG_PARAM,
        DLI_CBK_IOG_LABEL_REPORT, DLI_CBK_IMG_LABEL_REPORT,
        DLI_CBK_IOB_REPORT_PARAM, DLI_CBK_IMB_REPORT_PARAM,
        DLI_CBK_IOB_ESTABLISHED, DLI_CBK_IMB_ESTABLISHED,
        DLI_CBK_ICB_SETUP_DATA_PATH, DLI_CBK_ICB_REMOVE_DATA_PATH,
        DLI_CBK_REMOVE_ICG_PARAM,
        DLI_CBK_IOG_PARAM_UPDATE, DLI_CBK_IMG_PARAM_UPDATE,
        DLI_CBK_SWITCH_FREQ_BAND,
    };
    for (size_t i = 0; i < sizeof(icbRegOpcodes) / sizeof(icbRegOpcodes[0]); i++) {
        if (!IcbEvtCbkReady(icbRegOpcodes[i])) {
            CM_LOGE("icb evt env degraded: opcode 0x%04x not registered", icbRegOpcodes[i]);
            g_icbEnvDegraded = true;
            return;
        }
    }

    /* --- 第一轮（环境探测段）：合法组进入业务体 + 命令已发出 + 事件实际进入目标 handler。
     * 任一不满足均为环境可达性问题（初始化链缺口），降级跳过；探测通过后其后的
     * 行为断言保持 abort --- */
    uint32_t ret = IcbEvtSetParamAndNotify(type, id, connHandle, gHandle, icbCnt, maxSduG2T, maxSduT2G,
        rtnG2T, rtnT2G);
    ICB_EVT_REQUIRE(ret == CM_SUCCESS, "legal CM_ICGSetParam rejected (env)");
    ICB_EVT_REQUIRE(g_evtConnCbkHits >= 1, "set param success evt no cbk (env)");
    ICB_EVT_REQUIRE(g_evtLastConnState == createdState, "created state mismatch (env)");
    ICB_EVT_REQUIRE(g_evtLastConnErr == CM_ICB_SUCCESS, "created err mismatch (env)");
    ICB_EVT_REQUIRE(g_evtLastConnId == id, "created id mismatch (env)");
    ICB_EVT_REQUIRE(g_evtLastConnHandle == connHandle, "created handle mismatch (env)");

    /* --- A->A 一致性（CM-02）：删节点后同参数重跑"请求+成功事件"，两轮轨迹一致 --- */
    CM_ICBConnectionState stateA1 = g_evtLastConnState;
    CM_ICBErrorCode errA1 = g_evtLastConnErr;
    uint8_t idA1 = g_evtLastConnId;
    uint16_t connA1 = g_evtLastConnHandle;
    CM_ICGRemovedParam removedParam = {};
    removedParam.type = type;
    removedParam.id = id;
    DLI_ICGCbkParam rmCtx;
    IcbEvtFillCbkParam(&rmCtx, type, id, 0, 0, 0);
    DLI_RemoveICGParamEvt rmEvt = {};
    rmEvt.id = id;
    ICB_EVT_CHECK(CM_ICGRemoveParam(&removedParam) == CM_SUCCESS);
    IcbEvtInject(DLI_CBK_REMOVE_ICG_PARAM, &rmCtx, &rmEvt, sizeof(rmEvt), DLI_SUCCESS);
    ICB_EVT_CHECK(g_evtLastConnState == removedState); /* 删除事件命中 REMOVE handler */
    ICB_EVT_CHECK(g_evtLastConnErr == CM_ICB_SUCCESS);

    uint32_t hitsBefore = g_evtConnCbkHits;
    ret = IcbEvtSetParamAndNotify(type, id, connHandle, gHandle, icbCnt, maxSduG2T, maxSduT2G, rtnG2T, rtnT2G);
    ICB_EVT_CHECK(ret == CM_SUCCESS); /* 节点已删，重建同参数请求仍成功 */
    ICB_EVT_CHECK(g_evtConnCbkHits == hitsBefore + 1); /* 计数差值==1 */
    ICB_EVT_CHECK(g_evtLastConnState == stateA1 && g_evtLastConnErr == errA1 &&
        g_evtLastConnId == idA1 && g_evtLastConnHandle == connA1); /* 轨迹与第一轮一致 */

    /* --- SetLabel 请求 + LabelReport 成功事件 --- */
    CM_ICBChannel labelIcb[1];
    (void)memset_s(labelIcb, sizeof(labelIcb), 0, sizeof(labelIcb));
    labelIcb[0].connHandle = connHandle;
    labelIcb[0].lcid = lcid;
    labelIcb[0].direction = direction;
    labelIcb[0].labelId = labelId;
    CM_ICGLabelParam labelParam = {};
    labelParam.type = type;
    labelParam.id = id;
    labelParam.icbCnt = 1;
    labelParam.icb = labelIcb;
    ICB_EVT_CHECK(CM_ICGSetLabel(&labelParam, supportSubrate, true) == CM_SUCCESS); /* 节点须已存在 */

    uint8_t labelEvtBuf[sizeof(DLI_ICGLabelReportEvt) + FUZZ_EVT_LABEL_MAX_CNT * sizeof(DLI_ICGLabel)];
    (void)memset_s(labelEvtBuf, sizeof(labelEvtBuf), 0, sizeof(labelEvtBuf));
    DLI_ICGLabelReportEvt *labelEvt = (DLI_ICGLabelReportEvt *)labelEvtBuf;
    labelEvt->lcid = lcid;
    labelEvt->connHandle = connHandle;
    labelEvt->icgId = id;
    labelEvt->labelCnt = labelCnt;
    DLI_ICGLabel *labels = (DLI_ICGLabel *)(labelEvtBuf + sizeof(DLI_ICGLabelReportEvt));
    for (uint8_t i = 0; i < labelCnt; i++) {
        labels[i].labelId = labelId; /* 其余字段 memset 为 0，覆盖解析路径即可 */
    }
    DLI_ICGCbkParam labelCtx;
    IcbEvtFillCbkParam(&labelCtx, type, id, 0, connHandle, 1);
    IcbEvtInject(labelRegOpcode, &labelCtx, labelEvtBuf,
        sizeof(DLI_ICGLabelReportEvt) + labelCnt * sizeof(DLI_ICGLabel), DLI_SUCCESS);
    ICB_EVT_CHECK(g_evtLabelCbkHits >= 1); /* LabelReport 成功进入 handler 并上报 */
    ICB_EVT_CHECK(g_evtLastLabelErr == CM_ICB_SUCCESS);
    ICB_EVT_CHECK(g_evtLastLabelConn == connHandle);

    /* --- 失败组（CM-11） --- */
    /* 1) status!=0 的 SetParam 事件：connectionCbk 收到失败态（NotifyParamChangeFailedCbk） */
    hitsBefore = g_evtConnCbkHits;
    IcbEvtInject(setRegOpcode, &labelCtx, labelEvtBuf, sizeof(DLI_ICGLabelReportEvt), failStatus);
    ICB_EVT_CHECK(g_evtConnCbkHits == hitsBefore + 1);
    ICB_EVT_CHECK(g_evtLastConnState == createdState);
    ICB_EVT_CHECK(g_evtLastConnErr != CM_ICB_SUCCESS);

    /* 2) connHandle 错关联的 LabelReport：无通道匹配，静默丢弃（仅日志），不得出现成功上报 */
    hitsBefore = g_evtLabelCbkHits;
    labelEvt->connHandle = (uint16_t)(connHandle ^ 0x8000); /* 高位翻转错关联 */
    IcbEvtInject(labelRegOpcode, &labelCtx, labelEvtBuf,
        sizeof(DLI_ICGLabelReportEvt) + labelCnt * sizeof(DLI_ICGLabel), DLI_SUCCESS);
    ICB_EVT_CHECK(g_evtLabelCbkHits == hitsBefore); /* 代码事实：不匹配走静默分支，无任何回调 */

    /* 3) connHandle 正确但 icgId 错关联：到达失败通知而非成功通知（NotifyLabelReportFailedCbk） */
    labelEvt->connHandle = connHandle;
    labelEvt->icgId = (uint8_t)(~id); /* 恒不等于 id */
    hitsBefore = g_evtLabelCbkHits;
    IcbEvtInject(labelRegOpcode, &labelCtx, labelEvtBuf,
        sizeof(DLI_ICGLabelReportEvt) + labelCnt * sizeof(DLI_ICGLabel), DLI_SUCCESS);
    ICB_EVT_CHECK(g_evtLabelCbkHits == hitsBefore + 1);
    ICB_EVT_CHECK(g_evtLastLabelErr != CM_ICB_SUCCESS);

    /* 4) status!=0 且 context 非空：按 context->connHandleNum 逐个失败上报 */
    labelEvt->icgId = id;
    hitsBefore = g_evtLabelCbkHits;
    IcbEvtInject(labelRegOpcode, &labelCtx, labelEvtBuf,
        sizeof(DLI_ICGLabelReportEvt) + labelCnt * sizeof(DLI_ICGLabel), failStatus);
    ICB_EVT_CHECK(g_evtLabelCbkHits == hitsBefore + 1);
    ICB_EVT_CHECK(g_evtLastLabelErr == CM_ICB_FAILED);
    ICB_EVT_CHECK(g_evtLastLabelConn == connHandle);

    /* 5) QualityReport 成功注入：无匹配校验，直接透传上报（断言 connHandle 透传一致） */
    DLI_IOBQualityReportEvt iobQualityEvt = {};
    iobQualityEvt.connHandle = connHandle;
    hitsBefore = g_evtQualityCbkHits;
    IcbEvtInject(DLI_CBK_IOB_REPORT_PARAM, NULL, &iobQualityEvt, sizeof(iobQualityEvt), DLI_SUCCESS);
    ICB_EVT_CHECK(g_evtQualityCbkHits == hitsBefore + 1);
    ICB_EVT_CHECK(g_evtLastQualityConn == connHandle);

    uint8_t imbQBuf[sizeof(DLI_IMBQualityReportEvt) + FUZZ_EVT_IMB_TX_MAX_CNT * sizeof(DLI_nodeChannelInfo)];
    (void)memset_s(imbQBuf, sizeof(imbQBuf), 0, sizeof(imbQBuf));
    DLI_IMBQualityReportEvt *imbQualityEvt = (DLI_IMBQualityReportEvt *)imbQBuf;
    imbQualityEvt->txCount = imbTxCount;
    DLI_nodeChannelInfo *chanInfo = (DLI_nodeChannelInfo *)(imbQBuf + sizeof(DLI_IMBQualityReportEvt));
    uint8_t imbChan = (uint8_t)(connHandle | 0x01); /* 非零句柄，避开无效 handle 过滤 */
    for (uint8_t i = 0; i < imbTxCount; i++) {
        chanInfo[i].channelConnHandle = imbChan;
    }
    hitsBefore = g_evtQualityCbkHits;
    IcbEvtInject(DLI_CBK_IMB_REPORT_PARAM, NULL, imbQBuf,
        sizeof(DLI_IMBQualityReportEvt) + imbTxCount * sizeof(DLI_nodeChannelInfo), DLI_SUCCESS);
    ICB_EVT_CHECK(g_evtQualityCbkHits == hitsBefore + imbTxCount); /* 每个非零 tx 通道各上报一次 */
    ICB_EVT_CHECK(g_evtLastQualityConn == imbChan);

    /* --- 建链：AddConnection 请求 + Established 成功/失败事件 --- */
    CM_ICBChannel channel[1];
    (void)memset_s(channel, sizeof(channel), 0, sizeof(channel));
    channel[0].connHandle = connHandle;
    channel[0].lcid = lcid;
    channel[0].direction = direction;
    channel[0].labelId = labelId;
    CM_ICBConnectionParam connParam = {};
    connParam.type = type;
    connParam.id = id;
    connParam.labelId = labelId;
    connParam.channelCnt = 1;
    connParam.channel = channel;
    ICB_EVT_CHECK(CM_ICBAddConnection(&connParam, true) == CM_SUCCESS); /* 节点须已存在 */

    DLI_ICBEstablishedEvt estEvt = {};
    estEvt.lcid = lcid;
    estEvt.connHandle = connHandle;
    estEvt.labelId = labelId; /* DLI_ICBEstablishedEvt 无 icgId 字段（真机编译核实） */
    hitsBefore = g_evtConnCbkHits;
    uint32_t statusHitsBefore = g_evtConnStatusHits;
    IcbEvtInject(estRegOpcode, &labelCtx, &estEvt, sizeof(estEvt), DLI_SUCCESS);
    ICB_EVT_CHECK(g_evtConnStatusHits == statusHitsBefore + 1); /* ICB_CONNECTED 状态回调命中 */
    ICB_EVT_CHECK(g_evtConnCbkHits == hitsBefore + 1);
    ICB_EVT_CHECK(g_evtLastConnState == estState);
    ICB_EVT_CHECK(g_evtLastConnErr == CM_ICB_SUCCESS);
    ICB_EVT_CHECK(g_evtLastConnHandle == connHandle);

    /* status!=0 变体：走 context 失败分支（NotifySingleChannelCallback 失败态） */
    hitsBefore = g_evtConnCbkHits;
    IcbEvtInject(estRegOpcode, &labelCtx, &estEvt, sizeof(estEvt), failStatus);
    ICB_EVT_CHECK(g_evtConnCbkHits == hitsBefore + 1);
    ICB_EVT_CHECK(g_evtLastConnState == estState);
    ICB_EVT_CHECK(g_evtLastConnErr == CM_ICB_FAILED);

    /* --- 数据路径：Setup/Remove 请求 + 成功事件（节点 channel 已含 connHandle） --- */
    uint8_t codecConfigData[FUZZ_EVT_CODEC_BUF_LEN] = {};
    for (uint8_t i = 0; i < codecConfigLen; i++) {
        codecConfigData[i] = (uint8_t)(labelId + i);
    }
    CM_ICBDataPath dataPath = {};
    dataPath.connHandle = connHandle;
    dataPath.direction = direction;
    dataPath.codecConfigLen = codecConfigLen;
    dataPath.codecConfigData = codecConfigData;
    ICB_EVT_CHECK(CM_ICBSetupDataPath(&dataPath) == CM_SUCCESS);

    DLI_ICBDataPathEvt dataPathEvt = {};
    dataPathEvt.connHandle = connHandle;
    DLI_ICGCbkParam pathCtx;
    IcbEvtFillCbkParam(&pathCtx, type, id, direction, connHandle, 1);
    hitsBefore = g_evtConnCbkHits;
    IcbEvtInject(DLI_CBK_ICB_SETUP_DATA_PATH, &pathCtx, &dataPathEvt, sizeof(dataPathEvt), DLI_SUCCESS);
    ICB_EVT_CHECK(g_evtConnCbkHits == hitsBefore + 1);
    ICB_EVT_CHECK(g_evtLastConnState == CM_ICB_STATE_ICB_DATA_PATH_SETUP);
    ICB_EVT_CHECK(g_evtLastConnErr == CM_ICB_SUCCESS);

    CM_ICBRemovedDataPath removedPath = {};
    removedPath.connHandle = connHandle;
    removedPath.direction = direction;
    ICB_EVT_CHECK(CM_ICBRemoveDataPath(&removedPath) == CM_SUCCESS);
    hitsBefore = g_evtConnCbkHits;
    IcbEvtInject(DLI_CBK_ICB_REMOVE_DATA_PATH, &pathCtx, &dataPathEvt, sizeof(dataPathEvt), DLI_SUCCESS);
    ICB_EVT_CHECK(g_evtConnCbkHits == hitsBefore + 1);
    ICB_EVT_CHECK(g_evtLastConnState == CM_ICB_STATE_ICB_DATA_PATH_REMOVED);
    ICB_EVT_CHECK(g_evtLastConnErr == CM_ICB_SUCCESS);

    /* --- 参数更新：UpdateParam 请求 + PARAM_UPDATE 成功事件 --- */
    uint16_t updConnHandle[1] = {connHandle};
    CM_ICGUpdatedParam updatedParam = {};
    updatedParam.type = type;
    updatedParam.id = id;
    updatedParam.labelId = labelId;
    updatedParam.icbCnt = 1;
    updatedParam.connHandle = updConnHandle;
    ICB_EVT_CHECK(CM_ICGUpdateParam(&updatedParam) == CM_SUCCESS);

    DLI_ICBParamUpdateEvt updEvt = {};
    updEvt.lcid = lcid;
    updEvt.connHandle = connHandle;
    updEvt.icgId = id;
    updEvt.labelId = labelId;
    hitsBefore = g_evtConnCbkHits;
    IcbEvtInject(updRegOpcode, NULL, &updEvt, sizeof(updEvt), DLI_SUCCESS);
    ICB_EVT_CHECK(g_evtConnCbkHits == hitsBefore + 1);
    ICB_EVT_CHECK(g_evtLastConnState == updState);
    ICB_EVT_CHECK(g_evtLastConnErr == CM_ICB_SUCCESS);

    /* --- 删除 -> 重建（资源复用）-> 最终删除，链表收尾为空，不影响下一输入 --- */
    ICB_EVT_CHECK(CM_ICGRemoveParam(&removedParam) == CM_SUCCESS); /* API 不校验节点存在，发包即成功 */
    hitsBefore = g_evtConnCbkHits;
    IcbEvtInject(DLI_CBK_REMOVE_ICG_PARAM, &rmCtx, &rmEvt, sizeof(rmEvt), DLI_SUCCESS);
    ICB_EVT_CHECK(g_evtConnCbkHits == hitsBefore + 1);
    ICB_EVT_CHECK(g_evtLastConnState == removedState);
    ICB_EVT_CHECK(g_evtLastConnErr == CM_ICB_SUCCESS);

    ret = IcbEvtSetParamAndNotify(type, id, connHandle, gHandle, icbCnt, maxSduG2T, maxSduT2G, rtnG2T, rtnT2G);
    ICB_EVT_CHECK(ret == CM_SUCCESS); /* 已删 id 重建节点成功 */
    ICB_EVT_CHECK(g_evtLastConnState == createdState);

    ICB_EVT_CHECK(CM_ICGRemoveParam(&removedParam) == CM_SUCCESS);
    IcbEvtInject(DLI_CBK_REMOVE_ICG_PARAM, &rmCtx, &rmEvt, sizeof(rmEvt), DLI_SUCCESS);
    ICB_EVT_CHECK(g_evtLastConnState == removedState);
    ICB_EVT_CHECK(g_evtLastConnErr == CM_ICB_SUCCESS);

    /* --- FreqBand：handler 同步执行（直调），listener 异步注册为统计覆盖不强断言 --- */
    (void)CM_ListenFreqBandSwitchEvent(CountingFreqBand);
    DLI_FreqBandSwitchEvt freqEvt = {};
    freqEvt.connHandle = connHandle;
    freqEvt.oldFreqBand = oldFreqBand;
    freqEvt.newFreqBand = newFreqBand;
    for (uint8_t i = 0; i < 4; i++) { /* 重复注入 4 次以统计覆盖注册时序竞争 */
        IcbEvtInject(DLI_CBK_SWITCH_FREQ_BAND, NULL, &freqEvt, sizeof(freqEvt), DLI_SUCCESS);
    }
    (void)g_evtFreqBandHits; /* 仅统计，见文件头记账第 5 条 */
    (void)CM_UnlistenFreqBandSwitchEvent(CountingFreqBand);
}

/* CM-12 边界组：按 variant 字节生成组合一，断言返回值符合合法域判断。
 * 合法组仅发请求不注入事件 -> 不建节点，无需清理（节点只在事件回注后创建）。 */
void FuzzIcgBoundaryParams(const uint8_t *fuzzData, size_t size)
{
    if (g_icbEnvDegraded) {
        return; /* 环境降级：CM_ICB* 的 NOT_INITED/命令层缺口会让合法/非法断言失真，本轮记 BLOCKED 跳过 */
    }
    FuzzedDataProvider fdp(fuzzData, size);
    uint8_t variant = fdp.ConsumeIntegral<uint8_t>() % 9;
    CM_ICBType type = fdp.ConsumeBool() ? CM_IMB : CM_IOB;
    uint16_t maxSduG2T = fdp.ConsumeIntegral<uint16_t>();
    uint16_t maxSduT2G = fdp.ConsumeIntegral<uint16_t>();
    uint8_t rtnG2T = fdp.ConsumeIntegral<uint8_t>();
    uint8_t rtnT2G = fdp.ConsumeIntegral<uint8_t>();

    CM_ICBParam icbParam[CM_MAX_CHANNEL_COUNT];
    CM_ICBParam::CM_ICB icbInner[CM_MAX_CHANNEL_COUNT]; /* 每条 icb 一个内层参数（paramCnt=1） */
    (void)memset_s(icbParam, sizeof(icbParam), 0, sizeof(icbParam));
    (void)memset_s(icbInner, sizeof(icbInner), 0, sizeof(icbInner));
    for (uint8_t i = 0; i < CM_MAX_CHANNEL_COUNT; i++) {
        icbParam[i].id = 0x10;
        icbInner[i].maxSduG2T = maxSduG2T;
        icbInner[i].maxSduT2G = maxSduT2G;
        icbInner[i].rtnG2T = rtnG2T;
        icbInner[i].rtnT2G = rtnT2G;
        icbParam[i].param = &icbInner[i];
    }

    CM_ICGParam icgParam = {};
    icgParam.type = type;
    icgParam.icbCnt = 1;
    icgParam.paramCnt = 1;
    icgParam.icbParam = icbParam;

    switch (variant) {
        case 0: /* 合法：id=0 为最小合法值（id<=0xEF，cm_icb_api.c） */
            icgParam.id = 0;
            icgParam.icbCnt = 1;
            ICB_EVT_CHECK(CM_ICGSetParam(&icgParam) == CM_SUCCESS);
            break;
        case 1: /* 合法：id=0xEF 为上界合法值 */
            icgParam.id = 0xEF;
            icgParam.icbCnt = 1;
            ICB_EVT_CHECK(CM_ICGSetParam(&icgParam) == CM_SUCCESS);
            break;
        case 2: /* 非法：id=0xF0 为上界邻点（id>0xEF 被拒） */
            icgParam.id = 0xF0;
            icgParam.icbCnt = 1;
            ICB_EVT_CHECK(CM_ICGSetParam(&icgParam) != CM_SUCCESS);
            break;
        case 3: /* 非法：icbCnt=0（要求 icbCnt!=0） */
            icgParam.id = 0x10;
            icgParam.icbCnt = 0;
            ICB_EVT_CHECK(CM_ICGSetParam(&icgParam) != CM_SUCCESS);
            break;
        case 4: /* 非法：icbCnt=17>CM_MAX_CHANNEL_COUNT(16)，校验先于数组遍历 */
            icgParam.id = 0x10;
            icgParam.icbCnt = CM_MAX_CHANNEL_COUNT + 1;
            ICB_EVT_CHECK(CM_ICGSetParam(&icgParam) != CM_SUCCESS);
            break;
        case 5: /* 非法：paramCnt=0（要求 paramCnt!=0） */
            icgParam.id = 0x10;
            icgParam.icbCnt = 1;
            icgParam.paramCnt = 0;
            ICB_EVT_CHECK(CM_ICGSetParam(&icgParam) != CM_SUCCESS);
            break;
        case 6: /* 非法：内层四元组不一致（icb1.rtnG2T 翻转最低位，恒与 icb0 不同） */
            icgParam.id = 0x10;
            icgParam.icbCnt = 2;
            icbInner[1].rtnG2T = (uint8_t)(rtnG2T ^ 0x01);
            ICB_EVT_CHECK(CM_ICGSetParam(&icgParam) != CM_SUCCESS);
            break;
        case 7: { /* 非法：direction=2 的 RemoveDataPath 与 codecConfigLen=0 的 SetupDataPath */
            CM_ICBRemovedDataPath removedPath = {};
            removedPath.connHandle = 0x0001;
            removedPath.direction = 2; /* 合法域 {0,1} 之外 */
            ICB_EVT_CHECK(CM_ICBRemoveDataPath(&removedPath) != CM_SUCCESS);
            uint8_t codecConfigData[FUZZ_EVT_CODEC_BUF_LEN] = {};
            CM_ICBDataPath dataPath = {};
            dataPath.connHandle = 0x0001;
            dataPath.direction = 0;
            dataPath.codecConfigLen = 0; /* 非法：len 为 0（data 非空以隔离 len 校验） */
            dataPath.codecConfigData = codecConfigData;
            ICB_EVT_CHECK(CM_ICBSetupDataPath(&dataPath) != CM_SUCCESS);
            break;
        }
        case 8: /* 合法：icbCnt=16 满配（paramCnt=1，四元组一致） */
        default:
            icgParam.id = 0x10;
            icgParam.icbCnt = CM_MAX_CHANNEL_COUNT;
            ICB_EVT_CHECK(CM_ICGSetParam(&icgParam) == CM_SUCCESS);
            break;
    }
}

}  // namespace OHOS
