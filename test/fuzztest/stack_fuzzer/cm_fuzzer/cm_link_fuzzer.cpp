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
#include "cm_link_fuzzer.h"
#include <cstddef>
#include <cstdlib>
#include <type_traits>
#include "securec.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "sdf_worker.h"
#include "cm_errno.h"
#include "cm_trans_channel_api.h"
#include "cm_dyn_trans_channel_api.h"
#include "cm_logic_link_api.h"
#include "cm_api.h"
#include "cm_inner_api.h"
#include "cm_log.h"
#include "cm_util.h"
#include "cm_fuzzer_util.h"
#include "cm_signaling_struct.h"
#include "cm_signaling_internal.h"
// DLI 相关头文件
#include "dli_event_struct.h"
#include "dli_errno.h"

typedef struct {
    uint8_t status;
    uint16_t connHandle;         /*!< 连接句柄 */
} DLI_RemConnParamReqReplyReturnParam;

namespace OHOS {
    void CM_ConnectReadLocalFeaturesFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        DLI_ReadLocalFeatsEvt fevt = { 0 };
        for (uint8_t i = 0; i < sizeof(fevt.feats) / sizeof(fevt.feats[0]); i++) {
            fevt.feats[i] = provider.ConsumeIntegral<uint8_t>();
        }
        CmFuzzerSleCompleteEvt(DLI_READ_LOCAL_SUPPORT_FEATS, DLI_CBK_READ_LOCAL_FEATURE, fevt);
    }

    void CM_ConnectEstablishReqFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_ConnectParamReq_S param = {0};
        param.version = provider.ConsumeIntegral<uint8_t>();
        param.localIndex = provider.ConsumeIntegral<uint16_t>();
        param.addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        uint32_t ret = CM_ConnectEstablishReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectEstablishReq failed, ret:%d", ret);
            return;
        }
        uint16_t handle = provider.ConsumeIntegral<uint16_t>();
        // 修复地址源缺陷：完成事件与请求携带同一对端地址，覆盖正常直连分支
        // （SleLogicLinkSetAddrWhenActiveConnDone 中 GetByAddr 命中 CONNECTING 节点）
        CmFuzzerSleConnectCompleteEvtWithAddr(handle, &param.addr, 0);
        ret = CM_ConnectEstablishReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectEstablishReq failed, ret:%d", ret);
            return;
        }
        // 第二次完成事件同址重复上报，覆盖"该地址已完成连接"的异常分支；handle 独立注入
        uint16_t handleSecond = provider.ConsumeIntegral<uint16_t>();
        CmFuzzerSleConnectCompleteEvtWithAddr(handleSecond, &param.addr, 0);
    }

    // 保留旧路径：完成事件地址与请求地址不同，覆盖 SleLogicLinkSetAddrWhenActiveConnDone
    // 的白名单兜底分支（GetByStatus(CONNECTING) + 地址覆写）。
    // 注意：先消费一个保留字节使本函数的地址取值与 CM_ConnectEstablishReqFuzz 错开——后者先执行且
    // 总是把同模式地址的链路置为 CONNECTED，若地址相同本函数将只能覆盖"无连接中节点"的兜底子分支
    void CM_ConnectWhitelistBranchFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint8_t reserved = provider.ConsumeIntegral<uint8_t>();
        (void)reserved;
        CM_ConnectParamReq_S param = {0};
        param.version = provider.ConsumeIntegral<uint8_t>();
        param.localIndex = provider.ConsumeIntegral<uint16_t>();
        param.addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        uint32_t ret = CM_ConnectEstablishReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectEstablishReq failed, ret:%d", ret);
            return;
        }
        uint16_t handle = provider.ConsumeIntegral<uint16_t>();
        CmFuzzerSleConnectCompleteEvt(handle);
        ret = CM_ConnectEstablishReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectEstablishReq failed, ret:%d", ret);
            return;
        }
        CmFuzzerSleConnectCompleteEvt(handle);
    }

    void CM_ConnectReadRemoteFeturesAndVersionFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        DLI_ReadRemoteVersionEvt evt = { 0 };
        evt.status = provider.ConsumeIntegral<uint8_t>();
        evt.connHandle = provider.ConsumeIntegral<uint16_t>();
        evt.version = provider.ConsumeIntegral<uint8_t>();
        evt.companyIdentifier = provider.ConsumeIntegral<uint16_t>();
        evt.subversion = provider.ConsumeIntegral<uint16_t>();
        CmFuzzerReadRemoteVersionCompleteEvt(&evt);
        DLI_ReadRemoteFeatsEvt fevt = { 0 };
        fevt.status = provider.ConsumeIntegral<uint8_t>();
        fevt.connHandle = provider.ConsumeIntegral<uint16_t>();
        for (uint32_t i = 0; i < (sizeof(fevt.feats) / sizeof(fevt.feats[0])); i++) {
            fevt.feats[i] = provider.ConsumeIntegral<uint8_t>();
        }
        CmFuzzerReadRemoteFeaturesCompleteEvt(&fevt);
        DLI_DataLenChangeEvt devt = { 0 };
        devt.connHandle = provider.ConsumeIntegral<uint16_t>();
        devt.maxTxOctets = provider.ConsumeIntegral<uint16_t>();
        devt.maxRxOctets = provider.ConsumeIntegral<uint16_t>();
        CmFuzzerSleCompleteEvt(DLI_SET_DATA_LEN, DLI_CBK_SET_DATA_LEN, devt);
    }

    void CM_ConnectSetParamReqFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_ConnectSetParamReq_S param = {0};
        param.bitFrameType = provider.ConsumeIntegral<uint8_t>();
        param.enableFilterPolicy = provider.ConsumeIntegral<uint8_t>();
        param.initiatingPhys = provider.ConsumeIntegral<uint8_t>();
        param.gtNegotiate = provider.ConsumeIntegral<uint8_t>();
        param.scanInterval = provider.ConsumeIntegral<uint16_t>();
        param.scanWindow = provider.ConsumeIntegral<uint16_t>();
        param.minInterval = provider.ConsumeIntegral<uint16_t>();
        param.maxInterval = provider.ConsumeIntegral<uint16_t>();
        param.timeout = provider.ConsumeIntegral<uint16_t>();
        uint32_t ret = CM_ConnectSetParamReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectSetParamReq failed, ret:%d", ret);
            return;
        }
    }

    void CM_ConnectRemoteUpdateParamReqFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        DLI_RemoteConnParamReqEvt revt = { 0 };
        revt.connHandle = provider.ConsumeIntegral<uint16_t>();
        revt.connIntervalMin = provider.ConsumeIntegral<uint16_t>();
        revt.connIntervalMax = provider.ConsumeIntegral<uint16_t>();
        revt.txRxInterval = provider.ConsumeIntegral<uint8_t>();
        revt.eventInterval = provider.ConsumeIntegral<uint16_t>();
        revt.maxLatency = provider.ConsumeIntegral<uint16_t>();
        revt.supervisionTimeout = provider.ConsumeIntegral<uint16_t>();
        revt.systemTimeUnit = provider.ConsumeIntegral<uint8_t>();
        revt.txRxFlag = provider.ConsumeIntegral<uint8_t>();
        CmFuzzerSleConnectRemoteParamUpdateReqEvt(&revt);
    }

    typedef struct {
        uint8_t status;
        uint16_t connHandle;         /*!< 连接句柄 */
    } DLI_RemoteConnParamReqReplyReturnParam;

    void CM_ConnectRemoteConnectionParamRequestsReplyFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        DLI_RemoteConnParamReqReplyReturnParam revt = { 0 };
        revt.status = provider.ConsumeIntegral<uint8_t>();
        revt.connHandle = provider.ConsumeIntegral<uint16_t>();
        CmFuzzerSleCompleteEvt(DLI_CONNECTION_PARAM_REQ_REPLY, DLI_CBK_REMOTE_CONNECT_PARAM_REQ_REPLY, revt);
    }

    void CM_ConnectUpdateParamReqFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_ConnectUpdateParamReq_S param = {0};
        param.version = provider.ConsumeIntegral<uint8_t>();
        param.localIndex = provider.ConsumeIntegral<uint16_t>();
        param.addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        param.intervalMin = provider.ConsumeIntegral<uint16_t>();
        param.intervalMax = provider.ConsumeIntegral<uint16_t>();
        param.txRxInterval = provider.ConsumeIntegral<uint8_t>();
        param.eventInterval = provider.ConsumeIntegral<uint16_t>();
        param.maxLatency = provider.ConsumeIntegral<uint16_t>();
        param.supervisionTimeout = provider.ConsumeIntegral<uint16_t>();
        param.systemTimeUnit = provider.ConsumeIntegral<uint8_t>();
        param.txRxFlag = provider.ConsumeIntegral<uint8_t>();
        uint32_t ret = CM_ConnectUpdateParamReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectSetParamReq failed, ret:%d", ret);
            return;
        }
        DLI_ConnectionUpdateCmpEvt evt = { 0 };
        evt.status = provider.ConsumeIntegral<uint8_t>();
        evt.connHandle = provider.ConsumeIntegral<uint16_t>();
        evt.connInterval = provider.ConsumeIntegral<uint16_t>();
        evt.txRxInterval = provider.ConsumeIntegral<uint8_t>();
        evt.eventInterval = provider.ConsumeIntegral<uint16_t>();
        evt.maxLatency = provider.ConsumeIntegral<uint16_t>();
        evt.supervisionTimeout = provider.ConsumeIntegral<uint16_t>();
        evt.systemTimeUnit = provider.ConsumeIntegral<uint8_t>();
        evt.txRxFlag = provider.ConsumeIntegral<uint8_t>();
        CmFuzzerSleConnectParamUpdateEvt(&evt);
    }

    void CM_ConnectReleaseReqFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_DisconnectParamReq_S param = {};
        param.addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        SLE_Addr_S addr = {0};
        (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        param.discReason = provider.ConsumeIntegral<uint8_t>();
        uint32_t ret = CM_ConnectReleaseReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectReleaseReq failed, ret:%d", ret);
            return;
        }
        uint16_t handle = provider.ConsumeIntegral<uint16_t>();
        uint16_t reason = provider.ConsumeIntegral<uint16_t>();
        CmFuzzerSleDisconnectEvt(handle, reason);
    }

    void CM_ConnectCancelFuzz(uint8_t *data, size_t size)
    {
        (void)data;
        (void)size;
        uint32_t ret = CM_ConnectCancelReq();
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_ConnectCancelReq failed, ret:%d", ret);
        std::nullptr_t evt = nullptr;
        CmFuzzerSleCompleteEvt(DLI_CREATE_CONNECTION_CANCEL, DLI_CBK_CONNECT_CANCEL, evt);
    }

    void CM_ClearAcceptFilterListFuzz(uint8_t *data, size_t size)
    {
        (void)data;
        (void)size;
        CM_ClearAcceptFilterList();
        std::nullptr_t evt = nullptr;
        CmFuzzerSleCompleteEvt(DLI_CLEAR_ACCESS_FILTER_LIST, DLI_CBK_CLEAR_ACCESS_FLT_LIST, evt);
    }

    void CM_ReadAcceptFilterListSizeFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_ReadAcceptFilterListSize();
        DLI_ReadBufSizeEvt evt = { 0 };
        evt.acbTxDataLen = provider.ConsumeIntegral<uint16_t>();
        evt.acbTxDataNum = provider.ConsumeIntegral<uint8_t>();
        evt.icbTxDataLen = provider.ConsumeIntegral<uint16_t>();
        evt.icbTxDataNum = provider.ConsumeIntegral<uint8_t>();
        CmFuzzerSleCompleteEvt(DLI_READ_ACCESS_FILTER_LIST_SIZE, DLI_CBK_READ_ACCESS_FLT_LIST_SIZE, evt);
    }

    void CM_AddDeviceToAcceptFilterListFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        SLE_Addr_S addr = {0};
        addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        uint32_t ret = CM_AddDeviceToAcceptFilterList(&addr, false);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_AddDeviceToAcceptFilterList failed, ret:%d", ret);
        std::nullptr_t evt = nullptr;
        CmFuzzerSleCompleteEvt(DLI_ADD_DEVICE_TO_ACCESS_FILTER_LIST, DLI_CBK_ADD_DEV_TO_ACCESS_FLT_LIST, evt);
    }

    void CM_RemoveDeviceFromAcceptFilterListFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        SLE_Addr_S addr = {0};
        addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        uint32_t ret = CM_RemoveDeviceFromAcceptFilterList(&addr);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_RemoveDeviceFromAcceptFilterList failed, ret:%d", ret);
        std::nullptr_t evt = nullptr;
        CmFuzzerSleCompleteEvt(DLI_REMOVE_DEVICE_FROM_ACCESS_FILTER_LIST, DLI_CBK_RMV_DEV_FROM_ACCESS_FLT_LIST, evt);
    }

    void CM_SetPhyFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_SetPhyReq_S param = {0};
        param.lcid = provider.ConsumeIntegral<uint16_t>();
        param.txFormat = provider.ConsumeIntegral<uint8_t>();
        param.rxFormat = provider.ConsumeIntegral<uint8_t>();
        param.txPhy = provider.ConsumeIntegral<uint8_t>();
        param.rxPhy = provider.ConsumeIntegral<uint8_t>();
        param.txPilotDensity = provider.ConsumeIntegral<uint8_t>();
        param.rxPilotDensity = provider.ConsumeIntegral<uint8_t>();
        param.gFeedback = provider.ConsumeIntegral<uint8_t>();
        param.tFeedback = provider.ConsumeIntegral<uint8_t>();
        uint32_t ret = CM_SetPhy(&param);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_SetPhy failed, ret:%d", ret);
        DLI_SetPhyEvt evt = { 0 };
        evt.status = provider.ConsumeIntegral<uint8_t>();
        evt.connHandle = provider.ConsumeIntegral<uint16_t>();
        evt.txFormat = provider.ConsumeIntegral<uint8_t>();
        evt.rxFormat = provider.ConsumeIntegral<uint8_t>();
        evt.txPhy = provider.ConsumeIntegral<uint8_t>();
        evt.rxPhy = provider.ConsumeIntegral<uint8_t>();
        evt.txPilotDensity = provider.ConsumeIntegral<uint8_t>();
        evt.rxPilotDensity = provider.ConsumeIntegral<uint8_t>();
        evt.gFeedback = provider.ConsumeIntegral<uint8_t>();
        evt.tFeedback = provider.ConsumeIntegral<uint8_t>();
        CmFuzzerSetPhyEvt(&evt);
    }

    void CM_SetHostChannelClassificationFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_SetChannelMapReq_S param = {0};
        for (uint32_t i = 0; i < (sizeof(param.channelMap) / sizeof(param.channelMap[0])); i++) {
            param.channelMap[i] = provider.ConsumeIntegral<uint8_t>();
        }
        uint32_t ret = CM_SetHostChannelClassification(&param);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_SetHostChannelClassification failed, ret:%d", ret);
        std::nullptr_t evt = nullptr;
        CmFuzzerSleCompleteEvt(DLI_SET_HOST_CHANNEL_CLASSIFICATION, DLI_CBK_SET_HOST_CHANNEL_CLASSIFICATION, evt);
    }

    void CM_GetLogicLinkByAddrFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        SLE_Addr_S addr = { 0 };
        CM_LogicLink_S logicLink = { 0 };
        addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        uint32_t ret = CM_GetLogicLinkByAddr(&addr, &logicLink);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_GetLogicLinkByAddr failed, ret:%d", ret);
    }

    void CM_GetLogicLinkByLcidFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t lcid = 0;
        CM_LogicLink_S logicLink = { 0 };
        lcid = provider.ConsumeIntegral<uint16_t>();
        uint32_t ret = CM_GetLogicLinkByLcid(lcid, &logicLink);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_GetLogicLinkByLcid failed, ret:%d", ret);
    }

    void CM_SetLogicLinkDeviceTypeFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t lcid = 0;
        uint8_t deviceType = 0;
        lcid = provider.ConsumeIntegral<uint16_t>();
        deviceType = provider.ConsumeIntegral<uint8_t>();
        CM_SetLogicLinkDeviceType(lcid, deviceType);
    }

    void CM_SetACBSubrateFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_SetACBSubrateInnerParam param = {0};
        param.lcid = provider.ConsumeIntegral<uint16_t>();
        param.subrate = provider.ConsumeIntegral<uint16_t>();
        CM_InnerSetACBSubrate(&param);
    }

    static uint32_t FuzzEnableConnHighPowerImpl(DLI_EnableConnHighPowerParam *param)
    {
        (void)param;
        return CM_SUCCESS;
    }

    static uint32_t FuzzSetACBSubrateImpl(DLI_ACBSubrateParam *dliParam)
    {
        (void)dliParam;
        return CM_SUCCESS;
    }

    // 一致性校验：违反内部不变量时 abort（对齐 ICB_EVT_CHECK 风格，不用 assert 以避免 NDEBUG 失效）
    void CmFuzzerTrap(const char *msg)
    {
        CM_LOGE("CM_LINK_FUZZ_CHECK abort: %s", msg);
        abort();
    }
    #define CM_LINK_FUZZ_CHECK(cond, msg) \
        do { \
            if (!(cond)) { \
                CmFuzzerTrap(msg); \
            } \
        } while (0)

    /* 环境前置降级（PR#337 设备 ASAN 轮用户裁定）：真机/无芯片环境 InitStack 中途回退，
     * 半初始化态下建链/信令链路等前置不满足属环境可达性问题，不是产品缺陷——记录在案 +
     * 跳过本轮（置位后本目标后续输入直接跳过，对应验收锚点记 BLOCKED）。
     * 探测通过后（完整环境）其余断言保持 abort，"合法输入下产品行为真的错了"仍可炸出。 */
    static bool g_cmLinkEnvDegraded = false;
    // 供 FuzzCmDynTransInit 短路查询：降级为永久（真机无芯片不随输入恢复），避免每输入重试刷日志
    bool CmFuzzerLinkEnvDegraded(void)
    {
        return g_cmLinkEnvDegraded;
    }
    #define CM_LINK_FUZZ_REQUIRE(cond, msg) \
        do { \
            if (!(cond)) { \
                CM_LOGE("cm link env degraded, skip: %s", msg); \
                g_cmLinkEnvDegraded = true; \
                return; \
            } \
        } while (0)

    // 同址建链 helper：请求与完成事件携带同一对端地址，建链成功后链路 lcid == handle。
    // ponytail: addr/handle/version/localIndex 消费顺序固定（FDP 从尾部取整型），供种子文件按字节对齐
    static uint32_t CmFuzzerEstablishConnectedLink(FuzzedDataProvider &provider, SLE_Addr_S *addr,
        uint16_t *handle, uint8_t *version, uint16_t *localIndex)
    {
        addr->type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr->addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        *handle = provider.ConsumeIntegral<uint16_t>();
        *version = provider.ConsumeIntegral<uint8_t>();
        *localIndex = provider.ConsumeIntegral<uint16_t>();
        CM_ConnectParamReq_S param = {0};
        param.version = *version;
        param.localIndex = *localIndex;
        (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
        uint32_t ret = CM_ConnectEstablishReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectEstablishReq failed, ret:%d", ret);
            return ret;
        }
        CmFuzzerSleConnectCompleteEvtWithAddr(*handle, addr, 0);
        return CM_SUCCESS;
    }

    // 建链后用真实 lcid 依次触发连接运维操作并回注完成事件（SetPhy/ConnectUpdate/SetMcs 的
    // 事件 connHandle 必须 == lcid，否则产品侧按链路不存在处理）
    void CM_ConnectActiveOpsFuzz(uint8_t *data, size_t size)
    {
        if (g_cmLinkEnvDegraded) {
            return; /* 环境已降级：本轮记 BLOCKED 跳过 */
        }
        FuzzedDataProvider provider(data, size);
        SLE_Addr_S addr = {0};
        uint16_t handle = 0;
        uint8_t version = 0;
        uint16_t localIndex = 0;
        uint32_t ret = CmFuzzerEstablishConnectedLink(provider, &addr, &handle, &version, &localIndex);
        /* 环境探测段：真机回退态建链/查询链路不可达属环境问题，降级跳过并记录 */
        CM_LINK_FUZZ_REQUIRE(ret == CM_SUCCESS, "active ops establish link failed (env)");
        CM_LogicLink_S logicLink = { 0 };
        ret = CM_GetLogicLinkByAddr(&addr, &logicLink);
        CM_LINK_FUZZ_REQUIRE((ret == CM_SUCCESS) && (logicLink.lcid == handle), "active ops lcid mismatch (env)");
        uint16_t lcid = logicLink.lcid;

        CM_SetPhyReq_S phyParam = {0};
        phyParam.lcid = lcid;
        phyParam.txFormat = provider.ConsumeIntegral<uint8_t>();
        phyParam.rxFormat = provider.ConsumeIntegral<uint8_t>();
        phyParam.txPhy = provider.ConsumeIntegral<uint8_t>();
        phyParam.rxPhy = provider.ConsumeIntegral<uint8_t>();
        phyParam.txPilotDensity = provider.ConsumeIntegral<uint8_t>();
        phyParam.rxPilotDensity = provider.ConsumeIntegral<uint8_t>();
        phyParam.gFeedback = provider.ConsumeIntegral<uint8_t>();
        phyParam.tFeedback = provider.ConsumeIntegral<uint8_t>();
        ret = CM_SetPhy(&phyParam);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_SetPhy failed, ret:%d", ret);
        if (ret == CM_SUCCESS) {
            DLI_SetPhyEvt phyEvt = { 0 };
            phyEvt.status = 0;
            phyEvt.connHandle = lcid;
            phyEvt.txFormat = phyParam.txFormat;
            phyEvt.rxFormat = phyParam.rxFormat;
            phyEvt.txPhy = phyParam.txPhy;
            phyEvt.rxPhy = phyParam.rxPhy;
            phyEvt.txPilotDensity = phyParam.txPilotDensity;
            phyEvt.rxPilotDensity = phyParam.rxPilotDensity;
            phyEvt.gFeedback = phyParam.gFeedback;
            phyEvt.tFeedback = phyParam.tFeedback;
            CmFuzzerSetPhyEvt(&phyEvt);
        }

        CM_ConnectUpdateParamReq_S updParam = {0};
        updParam.version = version;
        updParam.localIndex = localIndex;
        (void)memcpy_s(&updParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        updParam.intervalMin = provider.ConsumeIntegral<uint16_t>();
        updParam.intervalMax = provider.ConsumeIntegral<uint16_t>();
        updParam.txRxInterval = provider.ConsumeIntegral<uint8_t>();
        updParam.eventInterval = provider.ConsumeIntegral<uint16_t>();
        updParam.maxLatency = provider.ConsumeIntegral<uint16_t>();
        updParam.supervisionTimeout = provider.ConsumeIntegral<uint16_t>();
        updParam.systemTimeUnit = provider.ConsumeIntegral<uint8_t>();
        updParam.txRxFlag = provider.ConsumeIntegral<uint8_t>();
        ret = CM_ConnectUpdateParamReq(&updParam);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_ConnectUpdateParamReq failed, ret:%d", ret);
        if (ret == CM_SUCCESS) {
            DLI_ConnectionUpdateCmpEvt updEvt = { 0 };
            updEvt.status = 0;
            updEvt.connHandle = lcid;
            updEvt.connInterval = updParam.intervalMax;
            updEvt.txRxInterval = updParam.txRxInterval;
            updEvt.eventInterval = updParam.eventInterval;
            updEvt.maxLatency = updParam.maxLatency;
            updEvt.supervisionTimeout = updParam.supervisionTimeout;
            updEvt.systemTimeUnit = updParam.systemTimeUnit;
            updEvt.txRxFlag = updParam.txRxFlag;
            CmFuzzerSleConnectParamUpdateEvt(&updEvt);
        }

        CM_SetMcsReq_S mcsParam = {0};
        mcsParam.lcid = lcid;
        mcsParam.mcs = provider.ConsumeIntegral<uint8_t>() % CM_MCS_MAX;
        ret = CM_SetMcs(&mcsParam);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_SetMcs failed, ret:%d", ret);
        if (ret == CM_SUCCESS) {
            CmFuzzerSetMcsEvt(lcid, provider.ConsumeIntegral<uint8_t>());
        }

        // 高功率/ACBSubrate/RSSI：lcid 与 addr 均为真实建链值
        CM_EnableConnHighPowerReq_S powerParam = {0};
        powerParam.lcid = lcid;
        powerParam.enable = provider.ConsumeBool();
        powerParam.powerLevel = provider.ConsumeIntegral<uint8_t>();
        (void)CM_EnableRealConnHighPower(&powerParam, FuzzEnableConnHighPowerImpl);

        CM_SetACBSubrateParam subrateParam = {};
        (void)memcpy_s(&subrateParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        subrateParam.subrate = provider.ConsumeIntegral<uint16_t>();
        subrateParam.onlySubrate = provider.ConsumeBool();
        subrateParam.subrateMax = provider.ConsumeIntegral<uint16_t>();
        subrateParam.maxLatency = provider.ConsumeIntegral<uint16_t>();
        subrateParam.continuationNum = provider.ConsumeIntegral<uint16_t>();
        subrateParam.supervisionTimeout = provider.ConsumeIntegral<uint16_t>();
        (void)CM_SetRealACBSubrate(&subrateParam, FuzzSetACBSubrateImpl);

        CM_ReadRemoteRssiReq_S rssiParam = {0};
        rssiParam.lcid = lcid;
        (void)CM_GetRssi(&rssiParam);
    }

    static void FuzzDynTransEstablishRspCbkImpl(const CM_DynTransChanEstablishParamRsp_S *param)
    {
        (void)param;
    }

    static void FuzzDynTransReleaseRspCbkImpl(const CM_DynTransChanReleaseParamRsp_S *param)
    {
        (void)param;
    }

    static void FuzzDynTransStatusIndicationCbkImpl(const CM_DynTransChanStatusIndicationRsp_S *param)
    {
        (void)param;
    }

    static bool FuzzDynTransEstablishedCheckCbkImpl(const CM_DynTransChanEstablishedCheckParam_S *param)
    {
        (void)param;
        return true;
    }

    void CM_EnableRealConnHighPowerFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_EnableConnHighPowerReq_S param = {0};
        param.lcid = provider.ConsumeIntegral<uint16_t>();
        param.enable = provider.ConsumeBool();
        param.powerLevel = provider.ConsumeIntegral<uint8_t>();
        (void)CM_EnableRealConnHighPower(&param, FuzzEnableConnHighPowerImpl);
    }

    void CM_SetRealACBSubrateFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_SetACBSubrateParam param = {};
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        param.subrate = provider.ConsumeIntegral<uint16_t>();
        param.onlySubrate = provider.ConsumeBool();
        param.subrateMax = provider.ConsumeIntegral<uint16_t>();
        param.maxLatency = provider.ConsumeIntegral<uint16_t>();
        param.continuationNum = provider.ConsumeIntegral<uint16_t>();
        param.supervisionTimeout = provider.ConsumeIntegral<uint16_t>();
        (void)CM_SetRealACBSubrate(&param, FuzzSetACBSubrateImpl);
    }

    void CM_GetRssiFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_ReadRemoteRssiReq_S param = {0};
        param.lcid = provider.ConsumeIntegral<uint16_t>();
        (void)CM_GetRssi(&param);
    }

    void CM_GetLcidByConnHandleFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t connHandle = provider.ConsumeIntegral<uint16_t>();
        (void)CM_GetLcidByConnHandle(connHandle);
    }

    void CM_RegDynTransChannelCbksFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        (void)provider;
        CM_DynTransChannelCbks_S cbks = {0};
        cbks.establishRspCbk = FuzzDynTransEstablishRspCbkImpl;
        cbks.releaseRspCbk = FuzzDynTransReleaseRspCbkImpl;
        cbks.statusIndicationCbk = FuzzDynTransStatusIndicationCbkImpl;
        cbks.establishedCheckCbk = FuzzDynTransEstablishedCheckCbkImpl;
        (void)CM_RegDynTransChannelCbks(&cbks);
    }

    void CM_DynTransChannelEstablishReqFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_DynTransChannelEstablishParamReq_S param = {0};
        param.version = provider.ConsumeIntegral<uint8_t>();
        param.localIndex = provider.ConsumeIntegral<uint16_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        param.srcPort = provider.ConsumeIntegral<uint16_t>();
        param.dstPort = provider.ConsumeIntegral<uint16_t>();
        param.slqi = provider.ConsumeIntegral<uint8_t>();
        param.extension = nullptr;
        param.expectedTransportMode = provider.ConsumeIntegral<uint8_t>();
        param.exclusive = provider.ConsumeBool();
        param.frameType = static_cast<CM_TransConnFrameType_E>(
            provider.ConsumeIntegral<uint8_t>() % CM_RADIO_FRAME_TYPE_END);
        (void)CM_DynTransChannelEstablishReq(&param);
    }

    // ---- dyntrans 建链闭环（PR-3 批次 B 主交付）----
    // 信令数据回注使用的 SLE 通用管理传输通道 tcid（cm_signaling_manage.c 中 CM_MANAGE_DEFAULT_TCID=0x02，
    // 该宏未导出头文件，此处按协议值定义）
    const uint8_t CM_FUZZER_SIG_MANAGE_TCID = 0x02;
    // 被动建立请求信令 data 固定长度：ReqPkt(3) + CommonConfig(5) + StreamConfig(6) + SlqiList(2) = 16
    const uint16_t CM_FUZZER_PASSIVE_REQ_LEN = 16;
    // 闭环场景变体数（成功/失败复用/错关联/重复响应/被动/断连晚到）
    const uint8_t CM_FUZZER_DYN_TRANS_VARIANT_MAX = 6;

    // ponytail: 捕获回调只保存最新一条信令（单线程无 identifier 竞争；多条连续发送时以最后一条为准）
    typedef struct {
        bool valid;
        uint16_t lcid;
        uint8_t tcid;
        uint8_t code;
        uint8_t identifier;
        uint8_t pkt[32];
        uint16_t len;
    } CmFuzzerCapSig_S;
    static CmFuzzerCapSig_S g_capSig = {};

    // 计数回调与最近一次结果记录
    static uint32_t g_dynEstCnt = 0;
    static uint32_t g_dynRelCnt = 0;
    static uint32_t g_dynIndCnt = 0;
    static uint8_t g_dynLastEstResult = 0;
    static uint8_t g_dynLastEstSrcTcid = 0;
    static uint8_t g_dynLastEstDstTcid = 0;
    static uint8_t g_dynLastRelResult = 0;
    static bool g_dynLastIndAdded = false;
    static uint8_t g_dynLastIndSrcTcid = 0;
    static uint8_t g_dynLastIndDstTcid = 0;
    static bool g_dynEstCheckRet = true;

    // CM 信令发送捕获回调（产品由 dtap 注册，cm_fuzzer 未编 dtap）。产品语义：发送方移交 buff 所有权，
    // 捕获后必须 SDF_BuffFree 防泄漏
    uint32_t CmFuzzerSignalingCaptureCb(uint8_t pi, uint8_t tcid, uint16_t lcid, SDF_Buff_S *buff)
    {
        (void)pi;
        if (buff == NULL) {
            return CM_SUCCESS;
        }
        uint64_t dataLen = SDF_DataLenGet(buff);
        if (dataLen >= sizeof(CM_SignalingHead_S)) {
            CM_SignalingHead_S *head = (CM_SignalingHead_S *)SDF_DataOffset(buff);
            g_capSig.valid = true;
            g_capSig.lcid = lcid;
            g_capSig.tcid = tcid;
            g_capSig.code = head->code;
            g_capSig.identifier = head->identifier;
            g_capSig.len = (uint16_t)(dataLen - sizeof(CM_SignalingHead_S));
            uint16_t copyLen = (g_capSig.len < sizeof(g_capSig.pkt)) ? g_capSig.len : (uint16_t)sizeof(g_capSig.pkt);
            (void)memcpy_s(g_capSig.pkt, sizeof(g_capSig.pkt), head->data, copyLen);
        }
        SDF_BuffFree(buff);
        return CM_SUCCESS;
    }

    void CmFuzzerDynEstRspCbk(const CM_DynTransChanEstablishParamRsp_S *param)
    {
        if (param == nullptr) {
            return;
        }
        g_dynEstCnt++;
        g_dynLastEstResult = param->result;
        g_dynLastEstSrcTcid = param->srcTcid;
        g_dynLastEstDstTcid = param->dstTcid;
    }

    void CmFuzzerDynRelRspCbk(const CM_DynTransChanReleaseParamRsp_S *param)
    {
        if (param == nullptr) {
            return;
        }
        g_dynRelCnt++;
        g_dynLastRelResult = param->result;
    }

    void CmFuzzerDynStatusIndCbk(const CM_DynTransChanStatusIndicationRsp_S *param)
    {
        if (param == nullptr) {
            return;
        }
        g_dynIndCnt++;
        g_dynLastIndAdded = param->added;
        g_dynLastIndSrcTcid = param->srcTcid;
        g_dynLastIndDstTcid = param->dstTcid;
    }

    bool CmFuzzerDynEstCheckCbk(const CM_DynTransChanEstablishedCheckParam_S *param)
    {
        (void)param;
        return g_dynEstCheckRet;
    }

    // 复位 dyntrans 计数与捕获状态（每 input 窗口收尾调用，保证下一 input 计数从 0 开始）
    void CmFuzzerResetDynTransState(void)
    {
        g_dynEstCnt = 0;
        g_dynRelCnt = 0;
        g_dynIndCnt = 0;
        g_capSig.valid = false;
    }

    // 构造对端动态 tcid：落在单播动态区间 [0x80,0xDF] 且不等于本端 tcid
    static uint8_t CmFuzzerGenPeerTcid(uint8_t localTcid)
    {
        return (uint8_t)(CM_TCID_UC_BEGIN + ((localTcid + 0x37U) & 0x3FU));
    }

    // 回注一条信令：用真实 CM_CreateSignalingBuff 构造，经真实接收入口 CM_RecvSignalingData 处理。
    // CM_RecvSignalingData 不释放 buff，由本函数释放
    static int CmFuzzerInjectSignaling(uint8_t code, uint8_t identifier, uint16_t lcid, void *pkt, uint16_t pktLen)
    {
        SDF_Buff_S *buff = CM_CreateSignalingBuff(code, identifier, (uint8_t *)pkt, pktLen);
        if (buff == NULL) {
            return CM_FAIL;
        }
        DTAP_Data_Info_S info = { 0 };
        info.pi = 0;
        info.lcid = lcid;
        info.tcid = CM_FUZZER_SIG_MANAGE_TCID;
        int ret = CM_RecvSignalingData(&info, buff);
        SDF_BuffFree(buff);
        return ret;
    }

    // 发起一次主动建立：请求成功 + 已发出 TC_CONNECT_REQ 信令（环境探测后的正向流程锚点，
    // 真机回退态不可达时降级跳过并记录）；成功返回 true 并带出 identifier 与本端 localTcid
    // （CM_TransChanEstablishReqPkt_S.srcTcid 为 data 首字段）
    static bool CmFuzzerDynTransEstablishOnce(const CM_DynTransChannelEstablishParamReq_S *estReq,
        uint8_t *identifier, uint8_t *localTcid)
    {
        g_capSig.valid = false;
        uint32_t ret = CM_DynTransChannelEstablishReq(estReq);
        if ((ret != CM_SUCCESS) || !g_capSig.valid || (g_capSig.code != TC_CONNECT_REQ)) {
            CM_LOGE("dyn trans establish env degraded, ret:%u, capValid:%d, capCode:0x%02x",
                ret, (int)g_capSig.valid, g_capSig.code);
            g_cmLinkEnvDegraded = true;
            return false;
        }
        *identifier = g_capSig.identifier;
        *localTcid = g_capSig.pkt[0];
        return true;
    }

    // 构造建立响应并回注（srcTcid 回显请求方即本端 tcid，dstTcid 为对端动态 tcid）
    static int CmFuzzerInjectEstablishRsp(uint8_t identifier, uint16_t lcid, uint8_t localTcid, uint8_t peerTcid,
        uint8_t result)
    {
        CM_TransChanEstablishRspPkt_S rspPkt = {0};
        rspPkt.srcTcid = localTcid;
        rspPkt.dstTcid = peerTcid;
        rspPkt.result = result;
        rspPkt.lc = 0;
        return CmFuzzerInjectSignaling(TC_CONNECT_RSP, identifier, lcid, &rspPkt, sizeof(rspPkt));
    }

    // 主动释放闭环：ReleaseReq + 校验 TC_DISCONNECT_REQ + 回注释放成功响应 + releaseRspCbk 命中。
    // 正向流程锚点：真机回退态不可达时降级返回 false（调用方记录跳过）
    static bool CmFuzzerDynTransReleaseClosure(const CM_DynTransChannelReleaseParamReq_S *relReq, uint16_t lcid,
        uint8_t localTcid, uint8_t peerTcid)
    {
        uint32_t relBefore = g_dynRelCnt;
        uint32_t ret = CM_DynTransChannelReleaseReq(relReq);
        if ((ret != CM_SUCCESS) || !g_capSig.valid || (g_capSig.code != TC_DISCONNECT_REQ)) {
            CM_LOGE("dyn trans release env degraded, ret:%u, capValid:%d, capCode:0x%02x",
                ret, (int)g_capSig.valid, g_capSig.code);
            g_cmLinkEnvDegraded = true;
            return false;
        }
        uint8_t relIdentifier = g_capSig.identifier;
        CM_TransChanReleaseRspPkt_S relRspPkt = {0};
        relRspPkt.srcTcid = localTcid;
        relRspPkt.dstTcid = peerTcid;
        relRspPkt.result = CM_RESULT_RELEASE_SUCCESS;
        int recvRet = CmFuzzerInjectSignaling(TC_DISCONNECT_RSP, relIdentifier, lcid, &relRspPkt,
            sizeof(relRspPkt));
        if ((recvRet != CM_SUCCESS) || (g_dynRelCnt != relBefore + 1) ||
            (g_dynLastRelResult != CM_DYN_TRANS_CHAN_RELEASE_SUCCESS)) {
            CM_LOGE("dyn trans release rsp env degraded, recvRet:%d, relCnt:%u, relResult:%u",
                recvRet, g_dynRelCnt, g_dynLastRelResult);
            g_cmLinkEnvDegraded = true;
            return false;
        }
        return true;
    }

    // dyntrans 全生命周期闭环 fuzz：按变体走主动建立/失败复用/错关联/重复响应/被动建立/断连晚到路径。
    // 正常业务拒绝（参数非法等）不算违规不 trap；仅 trap 内部不变量被破坏的场景
    void CM_DynTransLifecycleFuzz(uint8_t *data, size_t size)
    {
        if (g_cmLinkEnvDegraded) {
            return; /* 环境已降级：本轮记 BLOCKED 跳过 */
        }
        FuzzedDataProvider provider(data, size);
        uint8_t variant = provider.ConsumeIntegral<uint8_t>() % CM_FUZZER_DYN_TRANS_VARIANT_MAX;
        g_dynEstCheckRet = provider.ConsumeBool();
        uint8_t slqi = provider.ConsumeIntegral<uint8_t>() % CM_TRANS_CHANNEL_SLQI_MAX;
        uint16_t dstPort = provider.ConsumeIntegral<uint16_t>();
        uint16_t srcPort = provider.ConsumeIntegral<uint16_t>();
        uint8_t passiveId = provider.ConsumeIntegral<uint8_t>();
        SLE_Addr_S addr = {0};
        uint16_t handle = 0;
        uint8_t version = 0;
        uint16_t localIndex = 0;
        uint32_t ret = CmFuzzerEstablishConnectedLink(provider, &addr, &handle, &version, &localIndex);
        CM_LINK_FUZZ_REQUIRE(ret == CM_SUCCESS, "dyn trans establish link failed (env)");
        CM_LogicLink_S logicLink = { 0 };
        ret = CM_GetLogicLinkByAddr(&addr, &logicLink);
        CM_LINK_FUZZ_REQUIRE(ret == CM_SUCCESS, "dyn trans get logic link failed (env)");
        uint16_t lcid = logicLink.lcid;

        // 主动建立请求统一合法参数（STREAM 模式）
        CM_DynTransChannelEstablishParamReq_S estReq = {0};
        estReq.version = version;
        estReq.localIndex = localIndex;
        (void)memcpy_s(&estReq.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        estReq.srcPort = srcPort;
        estReq.dstPort = dstPort;
        estReq.transModeConfig.transMode = CM_TRANS_MODE_STREAM;
        estReq.transModeConfig.mtu = 0x200;
        estReq.transModeConfig.mps = 0x80;
        estReq.transModeConfig.streamMode.reorderTimeout = 0x100;
        estReq.transModeConfig.streamMode.crcInit = 0x1234;
        estReq.transModeConfig.streamMode.flushTimeout = 0x100;
        estReq.slqi = slqi;
        estReq.extension = nullptr;
        estReq.expectedTransportMode = CM_ACCESS_TRANS_MODE_UNICAST;
        estReq.exclusive = true;
        estReq.frameType = CM_SLE_RADIO_FRAME_TYPE_1;

        CM_DynTransChannelReleaseParamReq_S relReq = {0};
        relReq.version = version;
        relReq.localIndex = localIndex;
        (void)memcpy_s(&relReq.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        relReq.extension = nullptr;

        if (variant == 0) {
            // 成功闭环：建立成功 -> 主动释放 -> 断连（观察 statusIndication/release 路径，不 trap）
            uint8_t identifier = 0;
            uint8_t localTcid = 0;
            CM_LINK_FUZZ_REQUIRE(CmFuzzerDynTransEstablishOnce(&estReq, &identifier, &localTcid),
                "dyn trans establish once (env)");
            uint8_t peerTcid = CmFuzzerGenPeerTcid(localTcid);
            uint32_t estBefore = g_dynEstCnt;
            int recvRet = CmFuzzerInjectEstablishRsp(identifier, lcid, localTcid, peerTcid,
                CM_RESULT_ESTABLISH_SUCCESS);
            CM_LINK_FUZZ_CHECK(recvRet == CM_SUCCESS, "variant0 recv establish rsp failed");
            CM_LINK_FUZZ_CHECK((g_dynEstCnt == estBefore + 1) &&
                (g_dynLastEstResult == CM_DYN_TRANS_CHAN_ESTABLISH_SUCCESS) &&
                (g_dynLastEstSrcTcid == localTcid) && (g_dynLastEstDstTcid == peerTcid),
                "variant0 establish rsp cbk mismatch");
            relReq.srcTcid = localTcid;
            relReq.dstTcid = peerTcid;
            CM_LINK_FUZZ_REQUIRE(CmFuzzerDynTransReleaseClosure(&relReq, lcid, localTcid, peerTcid),
                "dyn trans release closure (env)");
            CmFuzzerSleDisconnectEvt(lcid, CM_DISC_REASON_REMOTE_USER_TERMINATED);
        } else if (variant == 1) {
            // 失败响应后 tcid 回收复用（红队 R5/R6），随后走释放闭环
            uint8_t identifier = 0;
            uint8_t localTcid = 0;
            CM_LINK_FUZZ_REQUIRE(CmFuzzerDynTransEstablishOnce(&estReq, &identifier, &localTcid),
                "dyn trans establish once (env)");
            uint8_t peerTcid = CmFuzzerGenPeerTcid(localTcid);
            uint32_t estBefore = g_dynEstCnt;
            int recvRet = CmFuzzerInjectEstablishRsp(identifier, lcid, localTcid, peerTcid,
                CM_RESULT_INSUFFICIENT_RESOURCE);
            CM_LINK_FUZZ_CHECK(recvRet == CM_SUCCESS, "variant1 recv fail rsp failed");
            CM_LINK_FUZZ_CHECK((g_dynEstCnt == estBefore + 1) &&
                (g_dynLastEstResult == CM_DYN_TRANS_CHAN_ESTABLISH_FAIL) &&
                (g_dynLastEstSrcTcid == localTcid), "variant1 fail rsp cbk mismatch");
            uint8_t identifier2 = 0;
            uint8_t localTcid2 = 0;
            CM_LINK_FUZZ_REQUIRE(CmFuzzerDynTransEstablishOnce(&estReq, &identifier2, &localTcid2),
                "dyn trans establish reuse (env)");
            // 池内唯一在飞的 tcid 已回收，立即重建应复用同一 localTcid
            CM_LINK_FUZZ_CHECK(localTcid2 == localTcid, "variant1 tcid not recycled");
            relReq.srcTcid = localTcid2;
            relReq.dstTcid = peerTcid;
            CM_LINK_FUZZ_REQUIRE(CmFuzzerDynTransReleaseClosure(&relReq, lcid, localTcid2, peerTcid),
                "dyn trans release closure reuse (env)");
            CmFuzzerSleDisconnectEvt(lcid, CM_DISC_REASON_REMOTE_USER_TERMINATED);
        } else if (variant == 2) {
            // 错关联（红队 R1）：错 identifier/错 lcid 的响应必须被拒绝且不回调，再用正确值走完闭环
            uint8_t identifier = 0;
            uint8_t localTcid = 0;
            CM_LINK_FUZZ_REQUIRE(CmFuzzerDynTransEstablishOnce(&estReq, &identifier, &localTcid),
                "dyn trans establish once (env)");
            uint8_t peerTcid = CmFuzzerGenPeerTcid(localTcid);
            uint32_t estBefore = g_dynEstCnt;
            int recvRet = CmFuzzerInjectEstablishRsp((uint8_t)(identifier + 1), lcid, localTcid, peerTcid,
                CM_RESULT_ESTABLISH_SUCCESS);
            CM_LINK_FUZZ_CHECK(recvRet != CM_SUCCESS, "variant2 wrong identifier accepted");
            recvRet = CmFuzzerInjectEstablishRsp(identifier, (uint16_t)(lcid + 1), localTcid, peerTcid,
                CM_RESULT_ESTABLISH_SUCCESS);
            CM_LINK_FUZZ_CHECK(recvRet != CM_SUCCESS, "variant2 wrong lcid accepted");
            CM_LINK_FUZZ_CHECK(g_dynEstCnt == estBefore, "variant2 cbk triggered by mismatched rsp");
            recvRet = CmFuzzerInjectEstablishRsp(identifier, lcid, localTcid, peerTcid,
                CM_RESULT_ESTABLISH_SUCCESS);
            CM_LINK_FUZZ_CHECK(recvRet == CM_SUCCESS, "variant2 correct rsp rejected");
            CM_LINK_FUZZ_CHECK((g_dynEstCnt == estBefore + 1) &&
                (g_dynLastEstResult == CM_DYN_TRANS_CHAN_ESTABLISH_SUCCESS),
                "variant2 correct rsp cbk mismatch");
            relReq.srcTcid = localTcid;
            relReq.dstTcid = peerTcid;
            CM_LINK_FUZZ_REQUIRE(CmFuzzerDynTransReleaseClosure(&relReq, lcid, localTcid, peerTcid),
                "dyn trans release closure (env)");
            CmFuzzerSleDisconnectEvt(lcid, CM_DISC_REASON_REMOTE_USER_TERMINATED);
        } else if (variant == 3) {
            // 重复响应（红队 R2）：成功闭环后同 identifier 的 RSP 再投一次必须被拒绝且计数不变
            uint8_t identifier = 0;
            uint8_t localTcid = 0;
            CM_LINK_FUZZ_REQUIRE(CmFuzzerDynTransEstablishOnce(&estReq, &identifier, &localTcid),
                "dyn trans establish once (env)");
            uint8_t peerTcid = CmFuzzerGenPeerTcid(localTcid);
            int recvRet = CmFuzzerInjectEstablishRsp(identifier, lcid, localTcid, peerTcid,
                CM_RESULT_ESTABLISH_SUCCESS);
            CM_LINK_FUZZ_CHECK(recvRet == CM_SUCCESS, "variant3 first rsp failed");
            uint32_t estBefore = g_dynEstCnt;
            recvRet = CmFuzzerInjectEstablishRsp(identifier, lcid, localTcid, peerTcid,
                CM_RESULT_ESTABLISH_SUCCESS);
            CM_LINK_FUZZ_CHECK(recvRet != CM_SUCCESS, "variant3 duplicated rsp accepted");
            CM_LINK_FUZZ_CHECK(g_dynEstCnt == estBefore, "variant3 duplicated rsp re-triggered cbk");
            relReq.srcTcid = localTcid;
            relReq.dstTcid = peerTcid;
            CM_LINK_FUZZ_REQUIRE(CmFuzzerDynTransReleaseClosure(&relReq, lcid, localTcid, peerTcid),
                "dyn trans release closure (env)");
            CmFuzzerSleDisconnectEvt(lcid, CM_DISC_REASON_REMOTE_USER_TERMINATED);
        } else if (variant == 4) {
            // 被动路径：手工打包对端 TC_CONNECT_REQ（STREAM 合法参数）经真实接收入口注入
            uint8_t peerTcid = CmFuzzerGenPeerTcid(CM_TCID_UC_BEGIN);
            uint8_t reqPkt[CM_FUZZER_PASSIVE_REQ_LEN] = {0};
            reqPkt[0] = peerTcid;             // srcTcid：对端动态 tcid
            reqPkt[1] = 0;                    // optionOffset=0，无扩展字段
            reqPkt[2] = 0x01;                 // exclusive=1, measure=0
            reqPkt[3] = CM_TRANS_MODE_STREAM; // transMode=2
            reqPkt[4] = 0x00;                 // mtu=0x0200 小端
            reqPkt[5] = 0x02;
            reqPkt[6] = 0x80;                 // mps=0x0080 小端
            reqPkt[7] = 0x00;
            reqPkt[8] = 0x00;                 // flushTimeout=0x0100 小端
            reqPkt[9] = 0x01;
            reqPkt[10] = 0x00;                // reorderTimeout=0x0100 小端
            reqPkt[11] = 0x01;
            reqPkt[12] = 0x34;                // crcInit=0x1234 小端
            reqPkt[13] = 0x12;
            reqPkt[14] = 0x01;                // slqiNum=1（CM_SLQI_SURPPORT_NUM）
            reqPkt[15] = slqi;
            g_capSig.valid = false;
            uint32_t indBefore = g_dynIndCnt;
            int recvRet = CmFuzzerInjectSignaling(TC_CONNECT_REQ, passiveId, lcid, reqPkt, sizeof(reqPkt));
            CM_LINK_FUZZ_CHECK(recvRet == CM_SUCCESS, "variant4 passive req rejected");
            if (g_dynEstCheckRet) {
                // 被动创建成功：statusIndication(added=true) + 本端回 TC_CONNECT_RSP（dstTcid=本端分配 tcid）
                CM_LINK_FUZZ_CHECK((g_dynIndCnt == indBefore + 1) && g_dynLastIndAdded,
                    "variant4 passive added indication missing");
                CM_LINK_FUZZ_CHECK((g_dynLastIndSrcTcid != peerTcid) && (g_dynLastIndDstTcid == peerTcid),
                    "variant4 passive indication tcid mismatch");
                CM_LINK_FUZZ_CHECK(g_capSig.valid && (g_capSig.code == TC_CONNECT_RSP),
                    "variant4 passive rsp not sent");
                uint8_t localTcid = g_capSig.pkt[1]; // RSP.dstTcid 为本端分配 tcid
                CM_LINK_FUZZ_CHECK((g_capSig.pkt[0] == peerTcid) &&
                    (g_capSig.pkt[2] == CM_RESULT_ESTABLISH_SUCCESS) &&
                    (g_dynLastIndSrcTcid == localTcid), "variant4 passive rsp content mismatch");
                // 被动释放：对端发 TC_DISCONNECT_REQ，本端回 statusIndication(added=false) + TC_DISCONNECT_RSP
                uint32_t indBefore2 = g_dynIndCnt;
                CM_TransChanReleaseReqPkt_S discReq = {0};
                discReq.srcTcid = peerTcid;
                discReq.dstTcid = localTcid;
                g_capSig.valid = false;
                recvRet = CmFuzzerInjectSignaling(TC_DISCONNECT_REQ, passiveId, lcid, &discReq, sizeof(discReq));
                CM_LINK_FUZZ_CHECK(recvRet == CM_SUCCESS, "variant4 passive release rejected");
                CM_LINK_FUZZ_CHECK((g_dynIndCnt == indBefore2 + 1) && !g_dynLastIndAdded,
                    "variant4 passive released indication missing");
                CM_LINK_FUZZ_CHECK(g_capSig.valid && (g_capSig.code == TC_DISCONNECT_RSP),
                    "variant4 release rsp not sent");
                CM_LINK_FUZZ_CHECK((g_capSig.pkt[0] == peerTcid) && (g_capSig.pkt[1] == localTcid) &&
                    (g_capSig.pkt[2] == CM_RESULT_RELEASE_SUCCESS), "variant4 release rsp content mismatch");
            } else {
                // 被动拒绝：本端回 RSP(result=INSUFFICIENT_RESOURCE) 且不创建通道（无 statusIndication）
                CM_LINK_FUZZ_CHECK(g_dynIndCnt == indBefore, "variant4 rejected req still added channel");
                CM_LINK_FUZZ_CHECK(g_capSig.valid && (g_capSig.code == TC_CONNECT_RSP),
                    "variant4 reject rsp not sent");
                CM_LINK_FUZZ_CHECK((g_capSig.pkt[0] == peerTcid) &&
                    (g_capSig.pkt[2] == CM_RESULT_INSUFFICIENT_RESOURCE),
                    "variant4 reject rsp content mismatch");
            }
            CmFuzzerSleDisconnectEvt(lcid, CM_DISC_REASON_REMOTE_USER_TERMINATED);
        } else {
            // 取消/失败（红队 R2/R6/R3）：失败响应 -> 重建 -> 断连触发 ACTIVATING 态失败回调 -> 晚到 RSP 拒绝
            uint8_t identifier = 0;
            uint8_t localTcid = 0;
            CM_LINK_FUZZ_REQUIRE(CmFuzzerDynTransEstablishOnce(&estReq, &identifier, &localTcid),
                "dyn trans establish once (env)");
            uint8_t peerTcid = CmFuzzerGenPeerTcid(localTcid);
            int recvRet = CmFuzzerInjectEstablishRsp(identifier, lcid, localTcid, peerTcid,
                CM_RESULT_INSUFFICIENT_RESOURCE);
            CM_LINK_FUZZ_CHECK(recvRet == CM_SUCCESS, "variant5 fail rsp rejected");
            uint8_t identifier2 = 0;
            uint8_t localTcid2 = 0;
            CM_LINK_FUZZ_REQUIRE(CmFuzzerDynTransEstablishOnce(&estReq, &identifier2, &localTcid2),
                "dyn trans establish reuse (env)");
            uint32_t estBefore = g_dynEstCnt;
            CmFuzzerSleDisconnectEvt(lcid, CM_DISC_REASON_REMOTE_USER_TERMINATED);
            // 断连触发 ReleaseAllNode：ACTIVATING 态通道经 establishRspCbk 上报失败
            CM_LINK_FUZZ_CHECK((g_dynEstCnt == estBefore + 1) &&
                (g_dynLastEstResult == CM_DYN_TRANS_CHAN_ESTABLISH_FAIL),
                "variant5 disconnect establish fail cbk missing");
            // 断连后晚到的旧 RSP：缓存已按 lcid 清理，必须拒绝且计数不变（红队 R3）
            uint32_t estBefore2 = g_dynEstCnt;
            recvRet = CmFuzzerInjectEstablishRsp(identifier2, lcid, localTcid2, peerTcid,
                CM_RESULT_ESTABLISH_SUCCESS);
            CM_LINK_FUZZ_CHECK(recvRet != CM_SUCCESS, "variant5 late rsp accepted");
            CM_LINK_FUZZ_CHECK(g_dynEstCnt == estBefore2, "variant5 late rsp re-triggered cbk");
        }
    }

    void CM_SetRemoteFeatureFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        NLSTK_RemoteFeatureInfo_S info = {0};
        info.lcid = provider.ConsumeIntegral<uint16_t>();
        for (int i = 0; i < NLSTK_REMOTE_FEATURE_LEN; i++) {
            info.features[i] = provider.ConsumeIntegral<uint8_t>();
        }
        (void)CM_SetRemoteFeature(&info);
    }

    void FuzzCmLinkSupplementApi(uint8_t *data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        CM_EnableRealConnHighPowerFuzz(data, size);
        CM_SetRealACBSubrateFuzz(data, size);
        CM_GetRssiFuzz(data, size);
        CM_GetLcidByConnHandleFuzz(data, size);
        CM_RegDynTransChannelCbksFuzz(data, size);
        CM_DynTransChannelEstablishReqFuzz(data, size);
        CM_SetRemoteFeatureFuzz(data, size);
    }

    void FuzzCmApi(uint8_t *data, size_t size)
    {
        CM_ConnectReadLocalFeaturesFuzz(data, size);
        CM_ConnectEstablishReqFuzz(data, size);
        CM_ConnectWhitelistBranchFuzz(data, size);
        CM_ConnectReadRemoteFeturesAndVersionFuzz(data, size);
        CM_GetLogicLinkByAddrFuzz(data, size);
        CM_GetLogicLinkByLcidFuzz(data, size);
        CM_ConnectSetParamReqFuzz(data, size);
        CM_ConnectUpdateParamReqFuzz(data, size);
        CM_ConnectRemoteUpdateParamReqFuzz(data, size);
        CM_ConnectRemoteConnectionParamRequestsReplyFuzz(data, size);
        CM_ConnectReleaseReqFuzz(data, size);
        CM_ConnectCancelFuzz(data, size);
        CM_ReadAcceptFilterListSizeFuzz(data, size);
        CM_ClearAcceptFilterListFuzz(data, size);
        CM_AddDeviceToAcceptFilterListFuzz(data, size);
        CM_RemoveDeviceFromAcceptFilterListFuzz(data, size);
        CM_SetPhyFuzz(data, size);
        CM_SetHostChannelClassificationFuzz(data, size);
        CM_GetLogicLinkByAddrFuzz(data, size);
        CM_GetLogicLinkByLcidFuzz(data, size);
        CM_SetLogicLinkDeviceTypeFuzz(data, size);
        CM_SetACBSubrateFuzz(data, size);
    }
}

static void CM_FuzzerChannelState(CM_TransChannelStateList_S *state)
{
}

static void CM_FuzzerSSAP_CMLogicLinkCbk(CM_LogicLinkState_S *param)
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

uint32_t FuzzCmLinkInit(void)
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
    CM_TransChannelCbk cbk = CM_FuzzerChannelState;
    ret = CM_RegTransChannelListener(cbk);
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_RegConnectCbks failed, ret:%d", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

void FuzzCmLinkApi(uint8_t *data, size_t size)
{
    OHOS::FuzzCmApi(static_cast<uint8_t *>(data), size);
    OHOS::FuzzCmLinkSupplementApi(static_cast<uint8_t *>(data), size);
}

// dyntrans 闭环窗口 Init：每 input 调用，与 FuzzCmDynTransDeInit 严格配对。
// 说明：FuzzCmLinkSupplementApi 已注册过 dyntrans 空回调且未反注册（g_cmIsRegistered 为 true），
// 此处先静默反注册复位守卫，再用计数回调注册；注册失败属接线缺陷而非业务拒绝，直接 trap
uint32_t FuzzCmDynTransInit(void)
{
    if (OHOS::CmFuzzerLinkEnvDegraded()) {
        return CM_SUCCESS; /* 环境已降级：跳过本窗口注册（DeInit 侧均有守卫，安全） */
    }
    /* 环境前置失败降级记录（不 abort）：真机回退态下信令初始化/dyntrans 注册可能不可达 */
    CM_UnRegDynTransChannelCbks();
    uint32_t ret = CM_SignalingInit();
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_SignalingInit failed, env degraded, ret:%d", ret);
        OHOS::g_cmLinkEnvDegraded = true; /* 命名空间限定：本函数在 OHOS 外 */
        return ret;
    }
    CM_DynTransChannelCbks_S cbks = {0};
    cbks.establishRspCbk = OHOS::CmFuzzerDynEstRspCbk;
    cbks.releaseRspCbk = OHOS::CmFuzzerDynRelRspCbk;
    cbks.statusIndicationCbk = OHOS::CmFuzzerDynStatusIndCbk;
    cbks.establishedCheckCbk = OHOS::CmFuzzerDynEstCheckCbk;
    ret = CM_RegDynTransChannelCbks(&cbks);
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_RegDynTransChannelCbks failed, env degraded, ret:%d", ret);
        OHOS::g_cmLinkEnvDegraded = true; /* 命名空间限定：本函数在 OHOS 外 */
        return ret;
    }
    return CM_SUCCESS;
}

// dyntrans 闭环窗口 DeInit：释放信令缓存（含定时器）+ 复位 dyntrans 注册与计数器。
// 顺序约束：CM_DeInit 会撤销全部逻辑链路监听但不复位 dyntrans 的 g_cmIsRegistered，必须每 input 配对
void FuzzCmDynTransDeInit(void)
{
    CM_SignalingDeInit();
    CM_UnRegDynTransChannelCbks();
    OHOS::CmFuzzerResetDynTransState();
}

void FuzzCmLinkDeInit(void)
{
    CM_DeInit();
}