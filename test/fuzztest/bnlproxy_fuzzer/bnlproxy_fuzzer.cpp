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

/* bnl_proxy.c（services/stack/src/cp/bnl/proxy/src/bnl_proxy.c）直调 harness。
 * 架构：bnl_proxy.c 经 bnl_ext_func_wrapper（BNL_ProxyInit/RecvMsg/LinkStateChange/DeInit，
 * 由 dlsym 的平台插件 .so 提供）注册自身回调；裸进程无该插件，ext func 列表为空。
 * 本 harness 通过公共访问器 BNL_GetExtFuncList() 注入进程内假 ext func（捕获
 * bnl_proxy.c 注册的 connect/disconnect/sendmsg 指针），CM 边界按 dtap_fuzzer
 * 既有桩范式截获（CM_RegLogicLinkListener 捕获 logicLinkCbk 供点火），DTAP 编真源，
 * 经真实 DTAP_DataRecv 分发触达 BNL_RecvDtapMsg。 */

#include <cstdint>
#include <thread>
#include <vector>

#include "fuzzer/FuzzedDataProvider.h"
#include "bnlproxy_fuzzer.h"
#include "dtap_errno.h"
#include "cm_dli_adapter.h"
#include "cm_errno.h"
#include "cm_trans_channel_api.h"
#include "cm_dyn_trans_channel_api.h"
#include "cm_trans_channel_mgr.h"
#include "cp_worker.h"
#include "dli_errno.h"
#include "dli_event.h"
#include "dli_layer.h"
#include "dli_layer_callback.h"
#include "dli_layer_config.h"
#include "dli_layer_stru.h"
#include "dli_layer_utils.h"
#include "dpfwk_log.h"
#include "dtap.h"
#include "dtap_frame.h"
#include "dtap_scheduler.h"
#include "dtap_tcid.h"
#include "dtap_trans.h"
#include "bnl_proxy.h"
#include "bnl_ext_func_wrapper.h"
#include "bnl_reg_ext_func.h"
#include "securec.h"
#include "sdf_addr.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_schedule.h"
#include <stdlib.h>

#define TEST_DEFAULT_NUM 30
#define TEST_CRC16_INIT 0x5555
#define TEST_DEFAULT_ACB_NUM 100
#define TEST_DEFAULT_ACB_LEN 1500
#define BNL_NET_BUF_MAX_SIZE 2048 // 与 bnl_proxy.c 的 NET_BUF_MAX_SIZE 对齐

/* ---------------- CM/DLI 边界桩（dtap_fuzzer 既有范式） ---------------- */

CM_TransChannelCbk g_bnlFuzzTransChannelCbk = nullptr;

extern "C" uint32_t CM_GetTransChannelByLcid(uint16_t lcid, CM_TransChan_S *transChan)
{
    (void)lcid;
    (void)transChan;
    return CM_SUCCESS;
}

extern "C" uint32_t CM_GetTransChannelBySrcTcid(uint16_t lcid, uint8_t srcTcid, CM_TransChan_S *transChan)
{
    (void)lcid;
    (void)srcTcid;
    (void)transChan;
    return CM_SUCCESS;
}

/* 捕获逻辑链路监听（bnl_proxy.c 的 BNL_LogicLinkCbk 经此注册，供 harness 点火） */
static CM_LogicLinkCbks_S g_bnlFuzzLogicLinkCbks = {0};
extern "C" uint32_t CM_RegLogicLinkListener(CM_LogicLinkCbks_S *cbks)
{
    if (cbks != nullptr) {
        g_bnlFuzzLogicLinkCbks = *cbks;
    }
    return CM_SUCCESS;
}

extern "C" uint32_t CM_GetLogicLinkConnectedSize(void)
{
    return 0;
}

extern "C" uint32_t DLI_DataSend(DLI_DataStru *data)
{
    if (data == nullptr) {
        return DLI_STACK_PARAMS_ERRNO;
    }
    SDF_BuffFree(data->buf);
    SDF_MemFree(data);
    return 0;
}

extern "C" DLI_DataStru *DLI_DefaultDataStruCreate(uint16_t lcid, uint16_t type,
                                                   uint8_t ts, uint8_t prio, SDF_Buff_S *buf)
{
    DLI_DataStru *dliData = (DLI_DataStru *)SDF_MemZalloc(sizeof(DLI_DataStru));
    if (dliData == nullptr) {
        return nullptr;
    }
    dliData->lcid = lcid;
    dliData->type = type;
    dliData->ts = ts;
    dliData->prio = prio;
    dliData->buf = buf;
    return dliData;
}

extern "C" void CM_UnRegTransChannelListener(void)
{
    return;
}

extern "C" uint32_t CM_SignalingInit(void)
{
    return 0;
}

/* 捕获通道监听（dtap 通道建立/释放经此下发，供收包分发路径建通道） */
extern "C" uint32_t CM_RegTransChannelListener(CM_TransChannelCbk cbk)
{
    g_bnlFuzzTransChannelCbk = cbk;
    return 0;
}

extern "C" uint32_t CM_UnRegLogicLinkListener(uint8_t moduleId)
{
    (void)moduleId;
    return CM_SUCCESS;
}

extern "C" void CM_SignalingDeInit(void)
{
    return;
}

extern "C" void CM_SetSendSignalingDataCbk(uint32_t (*)(uint8_t pi, uint8_t tcid,
    uint16_t lcid, SDF_Buff_S *buff))
{
    return;
}

extern "C" int CM_RecvSignalingData(DTAP_Data_Info_S *info, SDF_Buff_S *buff)
{
    (void)info;
    (void)buff;
    return CM_SUCCESS;
}

extern "C" uint32_t DLI_ReadLocalMeasureCaps(void)
{
    return 0;
}

extern "C" SleTransLcid_S *CM_FindTransChannelByLocalTcid(uint16_t lcid, uint8_t tcid)
{
    (void)lcid;
    (void)tcid;
    return NULL;
}

extern "C" uint32_t CM_DynTransChannelReleaseReq(const CM_DynTransChannelReleaseParamReq_S *param)
{
    (void)param;
    return 0;
}

extern "C" uint16_t DLI_DataNumGet(DLI_DataType type)
{
    (void)type;
    return TEST_DEFAULT_ACB_NUM;
}

extern "C" uint16_t DLI_DataLenGet(DLI_DataType type)
{
    (void)type;
    return TEST_DEFAULT_ACB_LEN;
}

extern "C" void DLI_DataNumChangeRegister(DLI_DataNumChangecbk cbk)
{
    (void)cbk;
    return;
}

extern "C" uint32_t CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE module, CM_DLI_ADAPTER_TYPE type, CM_DliCbk cbk)
{
    (void)module;
    (void)type;
    (void)cbk;
    return 0;
}

extern "C" uint32_t CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE module, CM_DLI_ADAPTER_TYPE type)
{
    (void)module;
    (void)type;
    return 0;
}

extern "C" uint32_t DLI_GetDataFragmentNums(SDF_Buff_S *buf)
{
    (void)buf;
    return 1;
}

extern "C" uint32_t DLI_GetFragmentMaxLen(void)
{
    return 605;
}

extern "C" uint32_t DLI_SplitData(DLI_DataStru *data, SDF_Buff_S *fragmentBuf[], uint32_t fragmentCnt)
{
    (void)data;
    (void)fragmentBuf;
    (void)fragmentCnt;
    return 0;
}

/* bnl_proxy.c 引用的 CM 直连接口（桩截获即可覆盖 bnl_proxy.c 侧行；CM 侧由 cm_fuzzer 承担） */
extern "C" uint32_t CM_DirectConnectAdd(uint8_t moduleId, const CM_DirectConnAddrParam_S *param)
{
    (void)moduleId;
    (void)param;
    return CM_SUCCESS;
}

extern "C" uint32_t CM_DirectConnectRemove(uint8_t moduleId, const SLE_Addr_S *addr, uint8_t reason)
{
    (void)moduleId;
    (void)addr;
    (void)reason;
    return CM_SUCCESS;
}

namespace OHOS {
namespace {
constexpr int BNL_FUZZ_DELAY_50_MS = 50;

/* ---------------- BNL 平台 ext func 进程内假实现 ----------------
 * 捕获 bnl_proxy.c 经 BNL_ProxyInit 注册的 NLSTK_BnlProxyFunc_S（内含其 static
 * BNL_ConnectDevice/BNL_DisConnectDevice/BNL_SendMsg 指针），供 harness 直接点火。 */
static NLSTK_BnlProxyFunc_S g_bnlProxyFuncs = {0};

static void FakeBnlExtInit(const NLSTK_BnlProxyFunc_S *func)
{
    if (func != nullptr) {
        g_bnlProxyFuncs = *func;
    }
}

static void FakeBnlExtDeInit(void)
{
    return;
}

static void FakeBnlLinkStateChange(const NLSTK_BnlLogicLinkState_S *state)
{
    (void)state;
    return;
}

static void FakeBnlRecvMsg(const NLSTK_BnlSendMsg_S *msg)
{
    (void)msg;
    return;
}

static SDF_Buff_S *CreateBnlBuff(const uint8_t *data, uint32_t size)
{
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(size);
    if (buff == nullptr) {
        return nullptr;
    }
    uint8_t *buffData = SDF_BuffAppend(buff, size);
    if (buffData == nullptr) {
        SDF_BuffFree(buff);
        return nullptr;
    }
    if (size > 0 && memcpy_s(buffData, size, data, size) != EOK) {
        SDF_BuffFree(buff);
        return nullptr;
    }
    return buff;
}

/* dtap_fuzzer 通道建立范式：经捕获的 CM 通道监听回调下发通道配置 */
static void OperateChannel(uint16_t lcid, uint8_t tcid, uint8_t mode, uint16_t result)
{
    if (g_bnlFuzzTransChannelCbk == nullptr) {
        return;
    }
    SDF_Traits traits = { .dtor = nullptr };
    SDF_Vector_S *channelVector = SDF_CreateVector(traits);
    if (channelVector == NULL) {
        return;
    }
    CM_TransChan_S chan = {0};
    chan.castMode = 0;
    chan.lcid = lcid;
    chan.srcTcid = tcid;
    chan.dstTcid = tcid;
    chan.config.transMode = mode;
    chan.config.mps = UINT16_MAX;
    chan.config.mtu = UINT16_MAX;
    SDF_VectorEmplaceBack(channelVector, &chan);
    CM_TransChannelStateList_S param = {0};
    param.channelVector = channelVector;
    param.result = result;
    g_bnlFuzzTransChannelCbk(&param);
    SDF_DestroyVector(channelVector);
    return;
}

/* BNL_SendMsg（sendMsgFunc）驱动（随机性整改）：len 全域由调用侧 FDP 传入（错配域
 * 交引擎自发命中：不一致/超上限/任意档），另保留一次与 payload 对齐的合法构造
 * （确定性准备，保证 DTAP_DataSend 深路径可达） */
static void DriveBnlSendMsg(const std::vector<uint8_t> &payload, uint8_t pi, uint32_t fdpLen)
{
    if (g_bnlProxyFuncs.sendMsgFunc == nullptr) {
        return;
    }
    uint32_t headerLen = sizeof(NLSTK_BnlSendMsg_S);
    std::vector<uint8_t> cmd(headerLen + payload.size(), 0);
    NLSTK_BnlSendMsg_S *msg = reinterpret_cast<NLSTK_BnlSendMsg_S *>(cmd.data());
    msg->pi = pi;
    msg->lcid = 0;
    msg->tcid = 0;
    if (!payload.empty()) {
        (void)memcpy_s(msg->val, payload.size(), payload.data(), payload.size());
    }
    /* 档1：len 全域（FDP）→ 合法/不一致/超上限由引擎与种子命中 */
    msg->len = fdpLen;
    g_bnlProxyFuncs.sendMsgFunc(cmd.data(), static_cast<uint32_t>(cmd.size()));
    /* 档2：合法构造（len 与 payload 对齐，真实报文形状），通道已建则深入 DTAP_DataSend */
    msg->len = static_cast<uint32_t>(payload.size());
    g_bnlProxyFuncs.sendMsgFunc(cmd.data(), static_cast<uint32_t>(cmd.size()));
}

/* BNL_RecvDtapMsg：真实 DTAP_DataRecv 分发（pi=WNP/WAP 已由 BNL_Init 注册） */
static void DriveBnlRecvDtapMsg(uint8_t pi, const std::vector<uint8_t> &payload)
{
    OperateChannel(0, 0, CM_TRANS_MODE_BASIC, CM_TRANS_CHANNEL_STATE_ACTIVATED);
    DTAP_RegisterBasicFrameCtx();
    DTAP_RegisterEnhanceFrameCtx();
    DTAP_Frame_S *frame = DTAP_CreateFrame(DTAP_FRAME_BASIC);
    if (frame == nullptr) {
        OperateChannel(0, 0, CM_TRANS_MODE_BASIC, CM_TRANS_CHANNEL_STATE_RELEASED);
        return;
    }
    frame->pi = pi;
    frame->buff = CreateBnlBuff(payload.data(), static_cast<uint32_t>(payload.size()));
    if (frame->buff == nullptr) {
        DTAP_DestroyFrame(frame);
        OperateChannel(0, 0, CM_TRANS_MODE_BASIC, CM_TRANS_CHANNEL_STATE_RELEASED);
        return;
    }
    if (frame->ctx.buildFrame(&frame->ctx, 0, TEST_CRC16_INIT) != DTAP_SUCCESS) {
        DTAP_DestroyFrame(frame);
        OperateChannel(0, 0, CM_TRANS_MODE_BASIC, CM_TRANS_CHANNEL_STATE_RELEASED);
        return;
    }
    frame->header = (uint8_t *)SDF_DataOffset(frame->buff);
    if (frame->ctx.parseFrame(&frame->ctx) != DTAP_SUCCESS) {
        DTAP_DestroyFrame(frame);
        OperateChannel(0, 0, CM_TRANS_MODE_BASIC, CM_TRANS_CHANNEL_STATE_RELEASED);
        return;
    }
    DTAP_DataRecv(0, frame->buff);
    DTAP_DestroyFrame(frame); /* 所有权语义同 dtap_fuzzer：分发不接管 buff，统一由 DestroyFrame 释放 */
    OperateChannel(0, 0, CM_TRANS_MODE_BASIC, CM_TRANS_CHANNEL_STATE_RELEASED);
    return;
}
} // namespace

void BnlProxyFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    SLE_Addr_S addr = {0};
    addr.type = fdp.ConsumeIntegral<uint8_t>();
    for (int i = 0; i < SLE_ADDR_LEN; i++) {
        addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    uint8_t pi = static_cast<uint8_t>(DTAP_PI_WNP + fdp.ConsumeIntegral<uint8_t>() % 2); /* WNP/WAP 两档 */
    std::vector<uint8_t> sendPayload = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegral<uint8_t>());
    std::vector<uint8_t> recvPayload = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegral<uint8_t>());

    DTAP_Init();
    /* 先建 basic 通道（lcid=0/tcid=0）：BNL_SendMsg 合法档可深入 DTAP_DataSend */
    OperateChannel(0, 0, CM_TRANS_MODE_BASIC, CM_TRANS_CHANNEL_STATE_ACTIVATED);

    /* BNL_Init 成功路径：假 ext init 已在 Initialize 注入 → BNL_ProxyInit 成功 →
     * CM_RegLogicLinkListener（桩捕获）→ DTAP_RegisterProtoRecvCbk(WNP/WAP) 真注册 */
    (void)BNL_Init();

    /* 连接/断开回调（BNL_ConnectDevice / BNL_DisConnectDevice） */
    if (g_bnlProxyFuncs.connectDeviceFunc != nullptr) {
        g_bnlProxyFuncs.connectDeviceFunc(&addr);
    }
    if (g_bnlProxyFuncs.disConnectDeviceFunc != nullptr) {
        g_bnlProxyFuncs.disConnectDeviceFunc(&addr);
    }

    /* 发送回调（BNL_SendMsg）：len 全域 FDP + 合法对齐构造 */
    uint32_t sendLen = fdp.ConsumeIntegral<uint32_t>();
    DriveBnlSendMsg(sendPayload, pi, sendLen);

    /* 链路状态回调（BNL_LogicLinkCbk）：经捕获的 logicLinkCbk 点火，逐字段 FDP */
    if (g_bnlFuzzLogicLinkCbks.logicLinkCbk != nullptr) {
        CM_LogicLinkState_S state = {0};
        state.lcid = fdp.ConsumeIntegral<uint16_t>();
        state.role = fdp.ConsumeIntegral<uint8_t>();
        state.result = fdp.ConsumeIntegral<uint8_t>();
        state.discReason = fdp.ConsumeIntegral<uint8_t>();
        state.connCompleteType = fdp.ConsumeIntegral<uint8_t>();
        state.advHandle = fdp.ConsumeIntegral<uint8_t>();
        state.addr = addr;
        g_bnlFuzzLogicLinkCbks.logicLinkCbk(&state);
    }

    /* 收包回调（BNL_RecvDtapMsg）：真实 DTAP 分发 */
    DriveBnlRecvDtapMsg(pi, recvPayload);

    BNL_DeInit();
    DTAP_DeInit();
}
} // namespace OHOS

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    SDF_ThreadInit(TEST_DEFAULT_NUM);
    SDF_EvcInit();
    ScheduleEnable();
    /* 注入进程内假 BNL ext func（裸进程无 libnearlink_stack_ext.z.so，
     * BNL_GetExtFuncList 为栈侧公共访问器，非产品码改动） */
    BNL_ExtFuncList *extFuncs = BNL_GetExtFuncList();
    if (extFuncs != NULL) {
        extFuncs->init = OHOS::FakeBnlExtInit;
        extFuncs->deInit = OHOS::FakeBnlExtDeInit;
        extFuncs->linkStateChange = OHOS::FakeBnlLinkStateChange;
        extFuncs->recvMsg = OHOS::FakeBnlRecvMsg;
    }
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    uint8_t *fuzzData = static_cast<uint8_t *>(malloc(size));
    if (fuzzData == nullptr) {
        return 0;
    }
    if (memcpy_s(fuzzData, size, data, size) != EOK) {
        free(fuzzData);
        return 0;
    }
    OHOS::BnlProxyFuzzTest(fuzzData, size);
    free(fuzzData);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::BNL_FUZZ_DELAY_50_MS));
    return 0;
}
