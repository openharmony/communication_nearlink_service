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

/* DliHw_fuzzer: Z-1 硬件 DLI 族（6 个 0 覆盖文件）进程内直调 harness。
 *
 * 覆盖目标（源码直接编入本 target，参照 dli_fuzzer 模式）：
 *   services/hardware/src/SleDliSnoop.cpp        Snoop 启停、抓帧、匿名化、敏感指令注册
 *   services/hardware/src/SleDliLayerAdapter.cpp 帧收发适配层（SleHalInit、Send、Close）
 *   services/hardware/src/SleDliCallbacks.cpp    HDI 回调注册体手动触发
 *   services/stack/src/dli/event/src/dli_factory_event.c  工厂事件 4 回调
 *   services/stack/src/nai/slem/src/sle_dli_layer.c       slem 层注册 6 接口
 *   services/stack/src/adapter/src/bnl_ext_func_wrapper.c BNL 扩展 4 代理
 *
 * 纪律：
 *   - 不调 SleDliThreadUtil::InitThread、DestroyQueue（构造已 ENABLED；DestroyQueue 会
 *     置 DISABLED 导致后续轮次 PostTask 静默丢弃，等价 hidhost 历史挂死源，不学）。
 *   - 不调 SleReset()：产品实现为 kill(getpid(), SIGKILL)，进程自杀红线。
 *   - 栈侧不需要 DM mock（不触设备管理链路）。
 *
 * FDP 消费序（首版固定，尾部扩展须追加）：
 *   shutSel, typeSel, isReceived, junk, newTimeNeeded,
 *   cmdNum, cmdOpcodes[cmdNum], evtNum, evtOpcodes[evtNum],
 *   wrapCmdNum, wrapEvtNum, regNullSel, sendPktData, halInitSel, initStatusSel,
 *   cbkTypeSel, cbkNullSel, evtOpcode, linkState 七字段, addr 七字段,
 *   msgPi, msgLcid, msgTcid, msgLen
 */

#include "DliHw_fuzzer.h"

#include <cstdlib>
#include <vector>
#include "fuzzer/FuzzedDataProvider.h"
#include "SleDliSnoop.h"
#include "SleDliLayerAdapter.h"
#include "SleDliCallbacks.h"
/* sle_dli_layer.h / slem_dli_callback.h 均无 extern "C" 保护（纯 C 头），
 * C++ TU 必须包一层，否则按 C++ 名字修饰声明、链接不到 sle_dli_layer.c 的 C 符号 */
extern "C" {
#include "sle_dli_layer.h"
#include "slem_dli_callback.h"
}
#include "dli_factory_event.h"
#include "bnl_ext_func_wrapper.h"
#include "bnl_reg_ext_func.h"

namespace {
constexpr size_t MAX_JUNK_LEN = 64;
constexpr useconds_t SNOOP_DRAIN_US = 50000; /* 50ms 收敛 ffrt 队列异步任务（基线做法） */
constexpr uint8_t MAX_REG_OPCODES = 4;

/* DliSnoopCapture 类型表：CMD/EVENT/ACB/ICB/未知 各分支（匿名化 switch 分发依据） */
const uint8_t SNOOP_TYPE_TABLE[] = {0xA1, 0xA2, 0xA3, 0xA4, 0x00};

/* ---- 登记用空回调（回调体只求可安全调用，不做语义校验） ---- */
void FuzzInitializationComplete(SleInitStatus status)
{
    (void)status;
}

void FuzzDliPacketReceived(SlePacketType type, const SlePacket *packet)
{
    (void)type;
    (void)packet;
}

void FuzzDliFailure(void) {}

void FuzzBnlLinkStateChange(const NLSTK_BnlLogicLinkState_S *state)
{
    (void)state;
}

void FuzzBnlRecvMsg(const NLSTK_BnlSendMsg_S *msg)
{
    (void)msg;
}

void FuzzBnlInit(const NLSTK_BnlProxyFunc_S *func)
{
    (void)func;
}

void FuzzBnlDeInit(void) {}

void FuzzConnectDevice(const SLE_Addr_S *addr)
{
    (void)addr;
}

void FuzzDisConnectDevice(const SLE_Addr_S *addr)
{
    (void)addr;
}

void FuzzSendMsg(const uint8_t *cmdVal, uint32_t cmdLen)
{
    (void)cmdVal;
    (void)cmdLen;
}

/* 敏感码流确定模板：保证黑名单命中/未命中两分支每轮可达（buffer 布局对齐
 * SleDliSnoop.cpp 匿名化解析偏移：send=[type|opcodeLE]，recv=[eventLE|lenLE|cmdLE]） */
std::vector<uint8_t> MakePayload(const uint8_t *tpl, size_t tplLen, const std::vector<uint8_t> &junk)
{
    std::vector<uint8_t> payload(tpl, tpl + tplLen);
    payload.insert(payload.end(), junk.begin(), junk.end());
    return payload;
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);

    /* ---------- 固定顺序 FDP 消费 ---------- */
    uint8_t shutSel = fdp.ConsumeIntegral<uint8_t>() % 2; /* 本轮是否 SnoopShutDown */
    uint8_t typeSel = fdp.ConsumeIntegral<uint8_t>() % 5;
    uint8_t packetType = SNOOP_TYPE_TABLE[typeSel];
    bool isReceived = fdp.ConsumeBool();
    std::vector<uint8_t> junk = fdp.ConsumeBytes<uint8_t>(MAX_JUNK_LEN);
    bool newTimeNeeded = fdp.ConsumeBool();
    uint8_t cmdNum = fdp.ConsumeIntegral<uint8_t>() % (MAX_REG_OPCODES + 1);
    uint8_t evtNum = fdp.ConsumeIntegral<uint8_t>() % (MAX_REG_OPCODES + 1);
    uint16_t cmdOpcodes[MAX_REG_OPCODES] = {0};
    for (uint8_t i = 0; i < cmdNum; i++) {
        cmdOpcodes[i] = fdp.ConsumeIntegral<uint16_t>();
    }
    uint16_t evtOpcodes[MAX_REG_OPCODES] = {0};
    for (uint8_t i = 0; i < evtNum; i++) {
        evtOpcodes[i] = fdp.ConsumeIntegral<uint16_t>();
    }
    uint8_t wrapCmdNum = fdp.ConsumeIntegral<uint8_t>() % (MAX_REG_OPCODES + 1); /* C 包装专用计数 */
    uint8_t wrapEvtNum = fdp.ConsumeIntegral<uint8_t>() % (MAX_REG_OPCODES + 1);
    uint8_t regNullSel = fdp.ConsumeIntegral<uint8_t>() % 4; /* 0 正常,1 cmdNull,2 evtNull,3 全空 */
    std::vector<uint8_t> sendPktData = fdp.ConsumeBytes<uint8_t>(MAX_JUNK_LEN);
    uint8_t halInitSel = fdp.ConsumeIntegral<uint8_t>() % 2;
    uint8_t initStatusSel = fdp.ConsumeIntegral<uint8_t>() % 3;
    uint8_t cbkTypeSel = fdp.ConsumeIntegral<uint8_t>() % 5;
    uint8_t cbkNullSel = fdp.ConsumeIntegral<uint8_t>() % 3; /* 0 无init/1 无recv/2 全有 */
    uint16_t evtOpcode = fdp.ConsumeIntegral<uint16_t>();
    NLSTK_BnlLogicLinkState_S linkState = {};
    linkState.lcid = fdp.ConsumeIntegral<uint16_t>();
    linkState.role = fdp.ConsumeIntegral<uint8_t>() % 2;
    linkState.result = fdp.ConsumeIntegral<uint8_t>();
    linkState.discReason = fdp.ConsumeIntegral<uint8_t>();
    linkState.connCompleteType = fdp.ConsumeIntegral<uint8_t>() % 2;
    linkState.advHandle = fdp.ConsumeIntegral<uint8_t>();
    linkState.addr.type = fdp.ConsumeIntegral<uint8_t>() % 2;
    for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
        linkState.addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    uint8_t msgPi = fdp.ConsumeIntegral<uint8_t>();
    uint16_t msgLcid = fdp.ConsumeIntegral<uint16_t>();
    uint8_t msgTcid = fdp.ConsumeIntegral<uint8_t>();
    uint8_t msgLen = fdp.ConsumeIntegral<uint8_t>() % 32;

    /* ---------- A. SleDliSnoop：启停 + 敏感指令注册 + 抓帧 ---------- */
    SleDliSnoop::GetInstance().SnoopStartUp();
    usleep(SNOOP_DRAIN_US); /* 等 ffrt 队列翻转 isLogging_，否则本轮抓帧被静默丢弃 */
    switch (regNullSel) {
        case 1: /* cmdNum>0 + null 指针：入参校验分支 */
            SleDliSnoop::GetInstance().RegisterSensitiveOpcodes(nullptr, cmdNum + 1, evtOpcodes, evtNum);
            break;
        case 2: /* evtNum>0 + null 指针 */
            SleDliSnoop::GetInstance().RegisterSensitiveOpcodes(cmdOpcodes, cmdNum, nullptr, evtNum + 1);
            break;
        case 3: /* 全空 */
            SleDliSnoop::GetInstance().RegisterSensitiveOpcodes(nullptr, 0, nullptr, 0);
            break;
        default: /* 正常注册：影响本轮抓帧的 IsExtSensitiveOpcode 判定 */
            SleDliSnoop::GetInstance().RegisterSensitiveOpcodes(cmdOpcodes, cmdNum, evtOpcodes, evtNum);
            break;
    }
    SleDliSnoopRegisterSensitiveOpcodes(cmdOpcodes, wrapCmdNum, evtOpcodes, wrapEvtNum); /* C 包装直调 */

    /* fuzz 型抓帧（send 收发两向 + 空载荷长度边界） */
    SleDliSnoop::GetInstance().DliSnoopCapture(packetType, junk, isReceived);
    SleDliSnoop::GetInstance().DliSnoopCapture(packetType, std::vector<uint8_t>(), !isReceived);

    /* 确定模板抓帧（随机性整改：静态表 + FDP 索引选择其一，不再每轮全量重放）：
     * 黑名单命中/未命中 × CMD/EVENT(status/complete/敏感)/ACB/ICB，7 槽全遍历交引擎/种子 */
    static const uint8_t cmdHit[] = {0xA1, 0x05, 0x04, 0x11, 0x22, 0x33, 0x44}; /* opcode 0x0405 命中 */
    static const uint8_t cmdMiss[] = {0xA1, 0x00, 0x10, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    static const uint8_t evtHit[] = {0x15, 0x00, 0x10, 0x00, 0xAA, 0xBB, 0xCC}; /* 0x0015 命中 */
    static const uint8_t evtStatus[] = {0x01, 0x00, 0x04, 0x00, 0x05, 0x04, 0xAA}; /* status 携带 0x0405 */
    static const uint8_t evtComplete[] = {0x02, 0x00, 0x04, 0x00, 0x00, 0x10, 0xBB}; /* complete 未命中 */
    static const uint8_t acbTpl[] = {0xA3, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    static const uint8_t icbTpl[] = {0xA4, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    static const struct {
        const uint8_t *data;
        size_t len;
    } CAP_TPL_TABLE[] = {
        {cmdHit, sizeof(cmdHit)},         {cmdMiss, sizeof(cmdMiss)},
        {evtHit, sizeof(evtHit)},         {evtStatus, sizeof(evtStatus)},
        {evtComplete, sizeof(evtComplete)}, {acbTpl, sizeof(acbTpl)},
        {icbTpl, sizeof(icbTpl)},
    };
    constexpr size_t CAP_TPL_COUNT = sizeof(CAP_TPL_TABLE) / sizeof(CAP_TPL_TABLE[0]);
    const auto &capTpl = CAP_TPL_TABLE[fdp.ConsumeIntegral<uint8_t>() % CAP_TPL_COUNT];
    std::vector<uint8_t> tplJunk = fdp.ConsumeBytes<uint8_t>(MAX_JUNK_LEN);
    std::vector<uint8_t> capPayload = MakePayload(capTpl.data, capTpl.len, tplJunk);
    /* send：data[0] 即类型；recv：类型由捕获任务补头。两向都驱动 */
    SleDliSnoop::GetInstance().DliSnoopCapture(capPayload[0], capPayload, false);
    SleDliSnoop::GetInstance().DliSnoopCapture(0xA2, capPayload, true);
    usleep(SNOOP_DRAIN_US); /* 队列内匿名化/落盘任务出队 */

    SleDliSnoop::GetInstance().CreateSnoopFile(newTimeNeeded);
    usleep(SNOOP_DRAIN_US);

    /* ---------- B. SleDliLayerAdapter：帧收发适配层 ---------- */
    SleDliCallbackFunc adapterCbks;
    adapterCbks.initializationComplete = FuzzInitializationComplete;
    adapterCbks.dliPacketReceived = FuzzDliPacketReceived;
    (void)SleHalInit(nullptr); /* 入参校验分支 */
    if (halInitSel == 1) {
        /* 环境依赖登记（PR#337 基线判例）：裸进程 ISleHciInterface::Get() 预期返回 null
         * 走 INITIALIZATION_ERROR；若宿主有 HDI 服务则继续走 death recipient 注册链。 */
        (void)SleHalInit(&adapterCbks);
    }
    SlePacket pkt = {};
    pkt.data = sendPktData.empty() ? nullptr : sendPktData.data();
    pkt.size = static_cast<uint32_t>(sendPktData.size());
    (void)SleSendDliPacket(nullptr); /* TRANSPORT_ERROR 分支 */
    (void)SleSendDliPacket(&pkt);    /* g_iSleDli 未初始化 → INITIALIZATION_ERROR 分支 */
    SleHalClose();

    /* ---------- C. SleDliCallbacks：回调注册体手动触发 ---------- */
    SleDliCallbackFunc cbkFunc = {};
    if (cbkNullSel != 0) {
        cbkFunc.initializationComplete = FuzzInitializationComplete;
    }
    if (cbkNullSel != 1) {
        cbkFunc.dliPacketReceived = FuzzDliPacketReceived;
    }
    SleDliCallbacks dliCbk(&cbkFunc);
    SleStatus initStatus = static_cast<SleStatus>(initStatusSel);
    (void)dliCbk.initializationComplete(initStatus);
    (void)dliCbk.hciPacketReceived(static_cast<uint32_t>(SNOOP_TYPE_TABLE[cbkTypeSel]), junk);
    (void)dliCbk.hciPacketReceived(static_cast<uint32_t>(packetType), std::vector<uint8_t>());

    /* ---------- D. sle_dli_layer.c：slem 层注册 6 接口 ---------- */
    (void)sle_dli_init();
    (void)sle_dli_enable();
    (void)sle_dli_disable();
    (void)sle_dli_cleanup();
    SlemCallbacks slemCbk = {};
    slemCbk.dliFailure = FuzzDliFailure;
    SleSetCallbackInstance(&slemCbk);
    (void)SleGetCallbackInstance();
    SleSetCallbackInstance(nullptr);
    (void)SleGetCallbackInstance();

    /* ---------- E. dli_factory_event.c：工厂事件 4 回调 ---------- */
    uint8_t factoryBuf[8] = {0};
    for (size_t i = 0; i < sizeof(factoryBuf); i++) {
        factoryBuf[i] = static_cast<uint8_t>(evtOpcode + i);
    }
    DLI_RfTxStartCbk(nullptr, factoryBuf, sizeof(factoryBuf), evtOpcode);
    DLI_RfRxStartCbk(&dliCbk, factoryBuf, sizeof(factoryBuf), evtOpcode); /* context 任意非空指针 */
    DLI_RfResetCbk(nullptr, nullptr, 0, 0);
    DLI_RfTrxEndCbk(nullptr, factoryBuf, sizeof(factoryBuf) - 1, evtOpcode);

    /* ---------- F. bnl_ext_func_wrapper.c：BNL 扩展 4 代理 ---------- */
    /* 未注册路径（g_funcList 全零 → WARN+return 分支；每轮由上轮尾部 Deregister 复位） */
    BNL_ProxyLinkStateChange(&linkState);
    BNL_ProxyRecvMsg(nullptr);
    (void)BNL_ProxyInit(nullptr);
    BNL_ProxyDeInit();

    /* 经公开 API BNL_GetExtFuncList 登记本进程回调 → 打通 4 个 call-through 分支 */
    BNL_ExtFuncList *funcList = BNL_GetExtFuncList();
    if (funcList != nullptr) {
        funcList->linkStateChange = FuzzBnlLinkStateChange;
        funcList->recvMsg = FuzzBnlRecvMsg;
        funcList->init = FuzzBnlInit;
        funcList->deInit = FuzzBnlDeInit;
    }
    NLSTK_BnlProxyFunc_S proxyFunc = {};
    proxyFunc.connectDeviceFunc = FuzzConnectDevice;
    proxyFunc.disConnectDeviceFunc = FuzzDisConnectDevice;
    proxyFunc.sendMsgFunc = FuzzSendMsg;
    (void)BNL_ProxyInit(&proxyFunc);
    BNL_ProxyLinkStateChange(&linkState);
    NLSTK_BnlSendMsg_S *msg =
        static_cast<NLSTK_BnlSendMsg_S *>(malloc(sizeof(NLSTK_BnlSendMsg_S) + msgLen));
    if (msg != nullptr) {
        msg->pi = msgPi;
        msg->lcid = msgLcid;
        msg->tcid = msgTcid;
        msg->len = msgLen;
        for (uint8_t i = 0; i < msgLen; i++) {
            msg->val[i] = static_cast<uint8_t>(i);
        }
        BNL_ProxyRecvMsg(msg);
        free(msg);
    }
    BNL_ProxyDeInit();
    BNL_DeregisterExtFunc(); /* 复位 g_funcList，保证下轮未注册路径可复现 */

    /* ---------- A'. 本轮可选停机（捕获任务已出队） ---------- */
    if (shutSel == 1) {
        SleDliSnoop::GetInstance().SnoopShutDown();
        usleep(SNOOP_DRAIN_US);
    }
    return 0;
}
