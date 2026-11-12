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
#include "cm_concurrent_conn_fuzzer.h"
#include <cstddef>
#include <type_traits>
#include "fuzzer/FuzzedDataProvider.h"
#include "cm.h"
#include "securec.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "sdf_timer.h"
#include "sdf_worker.h"
#include "cm_errno.h"
#include "cm_trans_channel_api.h"
#include "cm_logic_link_api.h"
#include "cm_api.h"
// 取消时序动作用到 CM_ConnectEstablishReq/CM_ConnectCancelReq（声明在 internal 头，与 cm_link_fuzzer 一致）
#include "cm_inner_api.h"
#include "cm_log.h"
#include "cm_util.h"
#include "cm_fuzzer_util.h"
// DLI 相关头文件
#include "dli_event_struct.h"
#include "dli_errno.h"

extern "C" uint32_t CP_TimerAdd(int *handle, SDF_TimerParam *param)
{
    return 0;
}

extern "C" void CP_TimerDel(int handle)
{
}

namespace OHOS {
    void FuzzCmCCDirectConnAddApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint8_t moduleId = 0;
        CM_DirectConnAddrParam_S param = {};
        moduleId = provider.ConsumeIntegral<uint8_t>();
        param.addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        param.frameType = provider.ConsumeIntegral<uint8_t>();
        uint32_t ret = CM_DirectConnectAdd(moduleId, &param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_DirectConnectAdd failed, ret:%d", ret);
            return;
        }
    }

    void FuzzCmCCDirectConnRmvApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint8_t moduleId = 0;
        uint8_t discReason = 0;
        CM_DirectConnAddrParam_S param = {};
        moduleId = provider.ConsumeIntegral<uint8_t>();
        param.addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        discReason = provider.ConsumeIntegral<uint8_t>();
        CM_DirectConnectAdd(moduleId, &param);
        moduleId = (moduleId % 2 == 0 ? 0x10 : 0x21);
        CM_DirectConnectRemove(moduleId, &param.addr, discReason);
    }

    void FuzzCmCCBgConnAddApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        const uint8_t testMaxAddrArrCount = 255;
        uint8_t moduleId = 0;
        uint8_t addrArrCount = 0;
        moduleId = provider.ConsumeIntegral<uint8_t>();
        addrArrCount = provider.ConsumeIntegral<uint8_t>() % testMaxAddrArrCount;
        CM_BgConnAddrParam_S *addrArr = (CM_BgConnAddrParam_S *)SDF_MemZalloc(addrArrCount * sizeof(CM_BgConnAddrParam_S));
        CM_CHECK_RETURN(addrArr != NULL, "malloc failed");
        for (uint8_t i = 0; i < addrArrCount; i++) {
            (addrArr + i)->addr.type = provider.ConsumeIntegral<uint8_t>();
            for (uint8_t j = 0; j < SLE_ADDR_LEN; j++) {
                (addrArr + i)->addr.addr[j] = provider.ConsumeIntegral<uint8_t>();
            }
            (addrArr + i)->isBypass = provider.ConsumeBool();
        }
        uint32_t ret = CM_BackgroundConnectAdd(moduleId, addrArrCount, addrArr);
        SDF_MemFree(addrArr);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_BackgroundConnectAdd failed, ret:%d", ret);
            return;
        }
    }

    void FuzzCmCCBgConnRmvApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint8_t moduleId = 0;
        SLE_Addr_S addr = {0};
        moduleId = provider.ConsumeIntegral<uint8_t>();
        addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        uint32_t ret = CM_BackgroundConnectRemove(moduleId, &addr);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_BackgroundConnectRemove failed, ret:%d", ret);
            return;
        }
    }

    void FuzzCmCCBgConnClearApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint8_t moduleId = 0;
        moduleId = provider.ConsumeIntegral<uint8_t>();
        uint32_t ret = CM_BackgroundConnectClear(moduleId);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_BackgroundConnectClear failed, ret:%d", ret);
            return;
        }
    }

    /* ==== PR-10 并发连接交错事件序列（cm_concurrent_conn 深路径）====
     * 现有 Direct/BgConn 入口用全随机 moduleId/frameType，几乎全部被参数校验拦截在
     * CM_DirectConnectAdd/CM_BackgroundConnectAdd 浅层。此处在单输入内以合法参数构造
     * 多设备交错时序：设备数 1..3、步数、每步动作与目标设备全 FDP 驱动，动作取自
     * 8 项域表 {直连增/直连删/背景增/背景删/背景清空/连接完成事件/断开事件/取消时序}，
     * 单线程顺序执行（无跨输入真并发/状态依赖，PR#345 红队 P1 教训）。原三设备固定
     * 交错脚本语义整体下沉种子 seed_cc_interleave。 */
    const uint8_t CM_FUZZER_CC_DEV_MAX = 3;    // 设备数上限（FDP 1..3）
    const uint8_t CM_FUZZER_CC_STEP_MAX = 12;  // 步数上限（FDP 1..12）
    const uint8_t CM_FUZZER_CC_ACTION_MAX = 8; // 动作域表大小

    static void CmFuzzerCCGenAddr(SLE_Addr_S *addr, uint8_t seq)
    {
        addr->type = 0;
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr->addr[i] = 0;
        }
        addr->addr[0] = (uint8_t)(0x71 + seq); // 非全零（过 CM_IsEmptyAddr 校验）且各设备互异
        addr->addr[1] = 0x3C;
        addr->addr[2] = 0xB7;
    }

    void FuzzCmCCInterleavedSeqApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        static const uint8_t legalModules[] = { CM_MODULE_ADPT, CM_MODULE_SSAP, CM_MODULE_BNL };
        uint8_t modA = legalModules[provider.ConsumeIntegral<uint8_t>() %
            (sizeof(legalModules) / sizeof(legalModules[0]))];
        uint8_t devCnt = (uint8_t)(provider.ConsumeIntegral<uint8_t>() % CM_FUZZER_CC_DEV_MAX + 1);
        SLE_Addr_S addrs[CM_FUZZER_CC_DEV_MAX] = {};
        // 完成事件句柄按设备配对记忆（仅本输入局部，断开事件复用最近完成句柄覆盖"断开在连链路"深分支）
        uint16_t handles[CM_FUZZER_CC_DEV_MAX] = {0};
        for (uint8_t d = 0; d < devCnt; d++) {
            CmFuzzerCCGenAddr(&addrs[d], (uint8_t)(d + 1));
        }
        uint8_t steps = (uint8_t)(provider.ConsumeIntegral<uint8_t>() % CM_FUZZER_CC_STEP_MAX + 1);
        for (uint8_t s = 0; s < steps; s++) {
            uint8_t act = provider.ConsumeIntegral<uint8_t>() % CM_FUZZER_CC_ACTION_MAX;
            switch (act) {
                case 0: {
                    // 直连添加：DoingConnDirectInsert → timer(mock) → StartAllowListConnReq →
                    // 全0地址白名单建链；map 已有节点时先取消旧建链再重启（重复添加走重启分支）
                    uint8_t dev = provider.ConsumeIntegral<uint8_t>() % devCnt;
                    // frameType 仅 TYPE_1(0)/TYPE_4(3) 合法（CM_DirectConnectAdd 校验）
                    uint8_t frame = (provider.ConsumeBool() ? CM_CONN_PARAM_FRAME_TYPE_1
                                                            : CM_CONN_PARAM_FRAME_TYPE_4);
                    CM_DirectConnAddrParam_S param = {};
                    (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), &addrs[dev], sizeof(SLE_Addr_S));
                    param.frameType = frame;
                    (void)CM_DirectConnectAdd(modA, &param);
                    break;
                }
                case 1: {
                    // 直连移除：已连接态走 CM_ConnectReleaseReq 分支；未连接走 DoingConnDirectRemove
                    //（各模块取消通知 + map 清理 + StartRmvAllowListConnReq）
                    uint8_t dev = provider.ConsumeIntegral<uint8_t>() % devCnt;
                    (void)CM_DirectConnectRemove(modA, &addrs[dev],
                        provider.ConsumeBool() ? CM_DISC_REASON_REMOTE_USER_TERMINATED
                                               : CM_DISC_REASON_CANCEL_PAIR);
                    break;
                }
                case 2: {
                    // 背景添加（moduleId 仅 ADPT 合法；count 1..16）；同地址重复添加走
                    // CM_DoingConnBgFind 已存在 → isBypass 刷新分支
                    uint8_t dev = provider.ConsumeIntegral<uint8_t>() % devCnt;
                    CM_BgConnAddrParam_S bgArr[1] = {};
                    (void)memcpy_s(&bgArr[0].addr, sizeof(SLE_Addr_S), &addrs[dev], sizeof(SLE_Addr_S));
                    bgArr[0].isBypass = provider.ConsumeBool();
                    (void)CM_BackgroundConnectAdd(CM_MODULE_ADPT, 1, bgArr);
                    break;
                }
                case 3: {
                    // 背景移除（CM_DoingConnBgRemove）
                    uint8_t dev = provider.ConsumeIntegral<uint8_t>() % devCnt;
                    (void)CM_BackgroundConnectRemove(CM_MODULE_ADPT, &addrs[dev]);
                    break;
                }
                case 4:
                    // 背景清空（CM_DoingConnBgClear + 空表遍历分支；模块级操作无目标设备）
                    (void)CM_BackgroundConnectClear(CM_MODULE_ADPT);
                    break;
                case 5: {
                    // 连接完成事件：白名单 CONNECTING 全0节点地址覆写 → CONNECTED → 固定通道激活 →
                    // CM_ConcurrentConnDoingComplete 清该设备后 map 仍有其他设备 → 重新发起白名单建链
                    uint8_t dev = provider.ConsumeIntegral<uint8_t>() % devCnt;
                    handles[dev] = (uint16_t)(provider.ConsumeIntegral<uint8_t>() + 1);
                    CmFuzzerSleConnectCompleteEvtEx(handles[dev], &addrs[dev], 0, CM_T_NODE,
                        CM_CONN_COMPLETE_SCAN);
                    break;
                }
                case 6: {
                    // 断开事件（CM_SleDisconnectProc → CM_ReleaseFixedTransChannel + 监听通报）；
                    // 句柄优先复用本设备最近完成事件句柄，未完成过则 FDP 生成（无链路防御分支）
                    uint8_t dev = provider.ConsumeIntegral<uint8_t>() % devCnt;
                    uint16_t h = (handles[dev] != 0) ? handles[dev]
                        : (uint16_t)(provider.ConsumeIntegral<uint8_t>() + 1);
                    CmFuzzerSleDisconnectEvtEx(h, CM_DISC_REASON_REMOTE_USER_TERMINATED, 0);
                    break;
                }
                default: {
                    // 取消连接时序：新白名单建链 → CM_ConnectCancelReq（删 CONNECTING）→ 取消完成事件
                    //（result 覆写/正常两分支：先取消后事件、先事件后取消两种时序均覆盖）
                    uint8_t dev = provider.ConsumeIntegral<uint8_t>() % devCnt;
                    CM_ConnectParamReq_S cancelParam = {0};
                    (void)memcpy_s(&cancelParam.addr, sizeof(SLE_Addr_S), &addrs[dev], sizeof(SLE_Addr_S));
                    if (CM_ConnectEstablishReq(&cancelParam) == CM_SUCCESS) {
                        (void)CM_ConnectCancelReq();
                        uint8_t cancelResult = 0;
                        CmFuzzerGenericCmEvt(DLI_CBK_CONNECT_CANCEL, &cancelResult, sizeof(cancelResult), 0, 0);
                        if (CM_ConnectEstablishReq(&cancelParam) == CM_SUCCESS) {
                            CmFuzzerGenericCmEvt(DLI_CBK_CONNECT_CANCEL, &cancelResult,
                                sizeof(cancelResult), 0, 0);
                            (void)CM_ConnectCancelReq();
                        }
                    } else {
                        (void)CM_ConnectCancelReq(); // 无 CONNECTING 节点防御分支
                    }
                    break;
                }
            }
        }
    }
}

static void CM_FuzzerChannelState(CM_TransChannelStateList_S *state)
{
}

static void CM_FuzzerSSAP_CMLogicLinkCbk(CM_LogicLinkState_S *param)
{
}

static void CM_FuzzerAdapt_CMLogicLinkCbk(CM_LogicLinkState_S *param)
{
}

static void CM_FuzzerHADM_CMReadRemoteCsCbk(CM_LogicLinkRemoteFeatures_S *param)
{
}

static void CM_FuzzerConnRemoteParamUpdateReqCbk(CM_ConnectRemoteUpdateParamReq_S *param)
{
    if (param == nullptr) {
        CM_LOGE("param is nullptr");
        return;
    }
    CM_LOGI("enter");
}

static void CM_FuzzerConnUpdateParamReqCbk(CM_ConnectUpdateParamRsp_S *param)
{
    CM_LOGI("enter");
    if (param == nullptr) {
        CM_LOGE("param is nullptr");
        return;
    }
}

static void CM_FuzzerConnSetPhyReqCbk(CM_SetPhyRsp_S *param)
{
    CM_LOGI("enter");
    if (param == nullptr) {
        CM_LOGE("param is nullptr");
        return;
    }
}

static void CM_FuzzerReadRmtFeatVerCbk(CM_ReadRemoteFeatureVersionRsp_S *param)
{
    CM_LOGI("enter");
    if (param == nullptr) {
        CM_LOGE("param is nullptr");
        return;
    }
}

uint32_t FuzzCmConcurrentConnInit(void)
{
    int32_t ret = CM_Init();
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_Init failed, ret:%d", ret);
        return CM_FAIL;
    }
    CM_LOGI("cm init success");
    CM_ConnectCbks_S cmCbks = { 0 };
    cmCbks.connRemoteUpdateParamReqCbk = CM_FuzzerConnRemoteParamUpdateReqCbk;
    cmCbks.connUpdateParamCbk = CM_FuzzerConnUpdateParamReqCbk;
    cmCbks.setPhyCbk = CM_FuzzerConnSetPhyReqCbk;
    cmCbks.readRemoteFeatureVersionCbk = CM_FuzzerReadRmtFeatVerCbk;
    ret = CM_RegConnectCbks(&cmCbks);
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_RegConnectCbks failed, ret:%d", ret);
        return CM_FAIL;
    }
    CM_LogicLinkCbks_S listener = { 0 };
    listener.moduleId = CM_MODULE_SSAP;
    listener.logicLinkCbk = CM_FuzzerSSAP_CMLogicLinkCbk;
    ret = CM_RegLogicLinkListener(&listener);
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_RegLogicLinkListener failed, ret:%d", ret);
        return CM_FAIL;
    }
    CM_LogicLinkCbks_S cmLinkCbk = {0};
    cmLinkCbk.moduleId = CM_MODULE_HADM;
    cmLinkCbk.remoteFeaturesCbk = CM_FuzzerHADM_CMReadRemoteCsCbk;
    ret = CM_RegLogicLinkListener(&cmLinkCbk);
    if (CM_RegLogicLinkListener(&cmLinkCbk) != CM_SUCCESS) {
        CM_LOGE("CM_RegLogicLinkListener failed, ret:%d", ret);
        return CM_FAIL;
    }
    CM_LogicLinkCbks_S cmAdptCbk = {0};
    cmAdptCbk.moduleId = CM_MODULE_ADPT;
    cmAdptCbk.logicLinkCbk = CM_FuzzerAdapt_CMLogicLinkCbk;
    ret = CM_RegLogicLinkListener(&cmAdptCbk);
    if (CM_RegLogicLinkListener(&cmAdptCbk) != CM_SUCCESS) {
        CM_LOGE("CM_RegLogicLinkListener failed, ret:%d", ret);
        return CM_FAIL;
    }
    CM_TransChannelCbk cbk = CM_FuzzerChannelState;
    ret = CM_RegTransChannelListener(cbk);
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_RegTransChannelListener failed, ret:%d", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

void FuzzCmConcurrentConnApi(uint8_t *data, size_t size)
{
    OHOS::FuzzCmCCDirectConnAddApi(static_cast<uint8_t *>(data), size);
    OHOS::FuzzCmCCDirectConnRmvApi(static_cast<uint8_t *>(data), size);
    OHOS::FuzzCmCCBgConnAddApi(static_cast<uint8_t *>(data), size);
    OHOS::FuzzCmCCBgConnRmvApi(static_cast<uint8_t *>(data), size);
    OHOS::FuzzCmCCBgConnClearApi(static_cast<uint8_t *>(data), size);
    // PR-10 尾向追加：合法参数多设备交错时序（不改头部已有消费顺序）
    OHOS::FuzzCmCCInterleavedSeqApi(static_cast<uint8_t *>(data), size);
}

void FuzzCmConcurrentConnDeInit(void)
{
    CM_DeInit();
}