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

#include "cm_api.h"
#include <stdint.h>
#include <stddef.h>
#include "securec.h"
#include "sdf_mem.h"
#include "cm_inner_api.h"
#include "cp_worker.h"
#include "cm_dli_adapter.h"
#include "cm_errno.h"
#include "cm_log.h"
#include "cm_util.h"
#include "cm_logic_link_api.h"
#include "cm_trans_channel_mgr.h"
#include "cm_exter_cbks_mgr.h"
#include "cm_event_core.h"
#include "cm_logic_link_listener_mgr.h"
#include "cm_icb_init.h"
#include "cm_icb_mgr.h"
#include "common_ext_func_wrapper.h"
#include "dli_ext_func_wrapper.h"
#include "dli_errno.h"
#include "dli_cmd.h"
#include "dli_cmd_struct.h"
#include "dli_reg_ext_func.h"
#include "sle_access_dli.h"
#include "sle_logic_link_mgr.h"
#include "sle_connect_param.h"
#include "cm_concurrent_conn.h"
#include "byte_codec.h"

#define CM_INIT_AND_DEINIT_TIMEOUT_MS 3000

static volatile bool g_cmIsInited = false;

static bool CM_IsRemotePrivateFeatureSupport(SleLogicLink_S *link, uint16_t featureBit);

static void CM_FreeCommonReqParamData(void *arg)
{
    if (arg == NULL) {
        return;
    }
    SDF_MemFree(arg);
}

static void CM_InitInner(void *param)
{
    CM_LOGI("CM_InitInner enter");
    (void)param;
    if (g_cmIsInited) {
        CM_LOGI("CM has inited");
        return;
    }
    CM_DliAdapterInit();
    SleAccessRegCbks();
    SleLogicLinkInit();
    SleDliConnectParamInit();
    CM_TransChannelMgrInit();
    CM_EventCoreInit();
    CM_ConcurrentConnInit();
    CM_ICBMgrSetInnerSetACBSubrate(CM_InnerSetACBSubrate);
    CM_ICBInit();
    g_cmIsInited = true;
    CM_LOGI("CM_InitInner success");
}

uint32_t CM_Init(void)
{
    CM_LOGI("CM_Init enter");
    uint32_t ret = CP_PostTaskBlocked(CM_InitInner, NULL, NULL, CM_INIT_AND_DEINIT_TIMEOUT_MS);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTask CM_InitInner failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    CM_LOGI("CM_Init success");
    return CM_SUCCESS;
}

static void CM_UnRegConnectCbksInner(void *param)
{
    (void)param;
    CM_CHECK_RETURN(g_cmIsInited, "CM has not inited");
    CM_UnRegExterCbks();
}

static void CM_DeInitInner(void *param)
{
    CM_LOGI("CM_DeInitInner enter");
    (void)param;
    CM_CHECK_RETURN(g_cmIsInited, "CM has not inited");
    CM_ICBDeinit();
    CM_ICBMgrSetInnerSetACBSubrate(NULL);
    CM_ConcurrentConnDeInit();
    CM_EventCoreDeInit();
    SleAccessUnRegCbks();
    CM_UnRegConnectCbksInner(NULL);
    for (uint8_t i = 0; i < CM_MODULE_ID_MAX; i++) {
        CM_UnRegLogicLinkListener(i);
    }
    CM_UnRegTransChannelListener();
    CM_TransChannelMgrDeInit();
    SleLogicLinkDeInit();
    CM_DliAdapterDeinit();
    g_cmIsInited = false;
    CM_LOGI("CM_DeInitInner success");
}

void CM_DeInit(void)
{
    CM_LOGI("CM_DeInit enter");
    uint32_t ret = CP_PostTaskBlocked(CM_DeInitInner, NULL, NULL, CM_INIT_AND_DEINIT_TIMEOUT_MS);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTask CM_DeInitInner failed, ret:0x%08x", ret);
        return;
    }
    CM_LOGI("CM_DeInit success");
}

static void CM_SetLocalPrivateFeature(void)
{
    DLI_LocalPrivateFeatures features = {};
    if (DLI_GetExtFuncList()->setLocalPrivateFeatures == NULL) {
        CM_LOGI("setLocalPrivateFeatures not support");
        return;
    }
    features.bitNumber = SLE_LOCAL_SUBRATE_FEATURE;
    features.bitValue = 1;
    uint32_t ret = DLI_GetExtFuncList()->setLocalPrivateFeatures(&features);
    CM_LOGI("bitNumber: %u, bitValue: %u, ret: %u", features.bitNumber, features.bitValue, ret);

    features.bitNumber = SLE_LOCAL_AUTORATE_FEATURE;
    features.bitValue = 1;
    ret = DLI_GetExtFuncList()->setLocalPrivateFeatures(&features);
    CM_LOGI("bitNumber: %u, bitValue: %u, ret: %u", features.bitNumber, features.bitValue, ret);
}

static void CM_EnableACBSubrate(void)
{
    DLI_ACBEnableSubrateParam subrateParam = {};
    if (DLI_GetExtFuncList()->acbEnableSubrate == NULL) {
        CM_LOGI("acbEnableSubrate not support");
        return;
    }

    subrateParam.subrateMin = SLE_ACB_SUBRATE_MIN;
    subrateParam.subrateMax = SLE_ACB_SUBRATE_MAX;
    subrateParam.maxLatency = SLE_ACB_MAX_LATENCY;
    subrateParam.continuationNum = SLE_ACB_CONTINUATION_NUM;
    subrateParam.supervisionTimeout = SLE_ACB_SUPERVISION_TIMEOUT_MAX;
    uint32_t ret = DLI_GetExtFuncList()->acbEnableSubrate(&subrateParam);
    CM_LOGI("acbEnableSubrate ret: %u", ret);
}

static void CM_EnableInner(void *param)
{
    CM_SetLocalPrivateFeature();
    CM_EnableACBSubrate();
    CM_ICBEnable();
}

uint32_t CM_Enable(void)
{
    uint32_t ret = CP_PostTask(CM_EnableInner, NULL, NULL);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTask failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    CM_LOGI("CM_Enable success");
    return CM_SUCCESS;
}

static void CM_DisableInner(void *param)
{
    CM_ICBDisable();
}

void CM_Disable(void)
{
    uint32_t ret = CP_PostTask(CM_DisableInner, NULL, NULL);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTask failed, ret:0x%08x", ret);
        return;
    }
    CM_LOGI("CM_Disable success");
}

static void CM_RegConnectCbksInner(void *param)
{
    CM_CHECK_RETURN(g_cmIsInited, "CM has not inited");
    CM_ConnectCbks_S *cbks = (CM_ConnectCbks_S *)param;
    CM_CHECK_RETURN((cbks != NULL), "cbks param is null");
    CM_RegExterCbks(cbks);
}

uint32_t CM_RegConnectCbks(CM_ConnectCbks_S *cbks)
{
    CM_CHECK_RETURN_RET((cbks != NULL), CM_INVALID_PARAM_ERR, "cbks param is null");
    CM_CHECK_RETURN_RET((cbks->connRemoteUpdateParamReqCbk != NULL) && (cbks->connUpdateParamCbk != NULL) &&
        (cbks->readRemoteFeatureVersionCbk != NULL) && (cbks->setPhyCbk != NULL),
        CM_INVALID_PARAM_ERR, "cbks member param is null");
    CM_ConnectCbks_S *req = (CM_ConnectCbks_S *)SDF_MemZalloc(sizeof(CM_ConnectCbks_S));
    CM_CHECK_RETURN_RET(req != NULL, CM_MEM_ERR, "mem zalloc error");
    (void)memcpy_s(req, sizeof(CM_ConnectCbks_S), cbks, sizeof(CM_ConnectCbks_S));
    uint32_t ret = CP_PostTaskBlocked(CM_RegConnectCbksInner, req, CM_FreeCommonReqParamData,
        CM_INIT_AND_DEINIT_TIMEOUT_MS);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTask CM_RegConnectCbksInner failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    CM_LOGI("CM_RegConnectCbks success");
    return CM_SUCCESS;
}

void CM_UnRegConnectCbks(void)
{
    uint32_t ret = CP_PostTaskBlocked(CM_UnRegConnectCbksInner, NULL, NULL, CM_INIT_AND_DEINIT_TIMEOUT_MS);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTask CM_UnRegConnectCbksInner failed, ret:0x%08x", ret);
        return;
    }
    CM_LOGI("CM_UnRegConnectCbks success");
}

uint32_t CM_RegLogicLinkListener(CM_LogicLinkCbks_S *cbks)
{
    CM_CHECK_RETURN_RET(g_cmIsInited, CM_NOT_INITED, "CM has not inited");
    CM_CHECK_RETURN_RET((cbks != NULL && cbks->moduleId < CM_MODULE_ID_MAX), CM_INVALID_PARAM_ERR, "param is null");
    CM_LOGI("reg enter, moduleId:%hhu", cbks->moduleId);
    return CM_RegLogicLinkCbks(cbks);
}

uint32_t CM_UnRegLogicLinkListener(uint8_t moduleId)
{
    CM_CHECK_RETURN_RET(g_cmIsInited, CM_NOT_INITED, "CM has not inited");
    CM_CHECK_RETURN_RET((moduleId < CM_MODULE_ID_MAX), CM_INVALID_PARAM_ERR, "param is invalid");
    CM_LOGI("unreg enter, moduleId:%hhu", moduleId);
    return CM_UnRegLogicLinkCbks(moduleId);
}

uint32_t CM_RegTransChannelListener(CM_TransChannelCbk cbk)
{
    CM_CHECK_RETURN_RET(g_cmIsInited, CM_NOT_INITED, "CM has not inited");
    CM_CHECK_RETURN_RET(cbk != NULL, CM_INVALID_PARAM_ERR, "param is null");
    return CM_RegTransChannelCbk(cbk);
}

void CM_UnRegTransChannelListener(void)
{
    CM_CHECK_RETURN(g_cmIsInited, "CM has not inited");
    CM_UnRegTransChannelCbk();
}

static void CM_NotifyConnectionReqCanceled(CM_ConnectParamReq_S *param, uint8_t discReason)
{
    // 连接请求异常，直接通知用户，不走芯片命令处理
    CM_LogicLinkState_S rsp = { 0 };
    rsp.role = CM_G_NODE;  // 建链失败，默认无效值
    rsp.lcid = CM_INVALID_LCID;
    (void)memcpy_s(&rsp.addr, sizeof(rsp.addr), &param->addr, sizeof(param->addr));
    rsp.result = CM_LINK_STATE_DISCONNECTED;
    rsp.discReason = discReason;
    CM_LOGI("notify connection req canceled rsp, addr:%s, discReason:0x%02x", GET_ENC_ADDR(&param->addr), discReason);
    CM_NotifyLogicLinkCbks(&rsp);
}

static void CM_NotifyConnectionExisted(CM_ConnectParamReq_S *param, SleLogicLink_S *link)
{
    // 连接已经存在，直接通知用户，不走芯片命令处理
    CM_LogicLinkState_S rsp = { 0 };
    rsp.role = link->role;
    rsp.lcid = link->lcid;
    (void)memcpy_s(&rsp.addr, sizeof(rsp.addr), &link->rmtAddr, sizeof(link->rmtAddr));
    rsp.result = link->status;
    rsp.discReason = DLI_SUCCESS;
    rsp.connCompleteType = link->connCompleteType;
    rsp.advHandle = link->advHandle;
    CM_LOGI("notify connection existed rsp, addr:%s, lcid:0x%04x, result:0x%02x",
        GET_ENC_ADDR(&param->addr), rsp.lcid, rsp.result);
    CM_NotifyLogicLinkCbks(&rsp);
}

static SleLogicLink_S *CM_EstablishLinkInit(CM_ConnectParamReq_S *param)
{
    SLE_Addr_S *addr = &param->addr;
    SleLogicLink_S *link = SleLogicLinkGetByAddr(addr);
    if (link != NULL) {
        // 对于是否正在连接或者已连接或者断开中，用户service层需要做严格的时序控制处理，
        // 即等待上一次连接回调后，再发起建链， 此处做容错性处理
        CM_LOGW("sle connection link has exist, lcid:0x%04x, status:%u", link->lcid, link->status);
        CM_NotifyConnectionExisted(param, link);
        return NULL;
    }
    link = SleLogicLinkGetByStatus(CM_LINK_STATE_CONNECTING);
    if (link != NULL) {
        CM_LOGE("already has a connecting device:%s, current connect failed", GET_ENC_ADDR(&link->rmtAddr));
        CM_NotifyConnectionReqCanceled(param, DLI_COMMAND_DISALLOWED);
        return NULL;
    }
    // 此设备没有连接过，新建设备节点信息
    link = SleLogicLinkAdd(addr);
    if (link == NULL) {
        CM_LOGE("sle connection create link create failed");
        CM_NotifyConnectionReqCanceled(param, DLI_MEMORY_CAPACITY_EXCEEDED);
        return NULL;
    }
    link->status = CM_LINK_STATE_CONNECTING;
    link->connCompleteType = CM_CONN_COMPLETE_SCAN;
    return link;
}

uint32_t CM_ConnectEstablishReq(CM_ConnectParamReq_S *param)
{
    CM_CHECK_RETURN_RET(g_cmIsInited, CM_NOT_INITED, "CM has not inited");
    CM_CHECK_RETURN_RET(param != NULL, CM_INVALID_PARAM_ERR, "param is null");

    CM_CheckAndFixAddrType(&param->addr);

    CM_LOGD("start, addr:%s", GET_ENC_ADDR(&param->addr));
    if (SleGetInitiatorFilterPolicy() != SLE_INITIATOR_FILTER_POLICY) {
        // 白名单连接，地址无用，清理为纯0地址
        (void)memset_s(&param->addr, sizeof(param->addr), 0x00, sizeof(param->addr));
    }
    SleLogicLink_S *link = CM_EstablishLinkInit(param);
    if (link == NULL) {
        // 已经响应建链结果，直接返回即可
        return CM_SUCCESS;
    }

    CM_AccessParamReq_S accessParam = { 0 };
    (void)memcpy_s(&accessParam.peerAddr, sizeof(SLE_Addr_S), &param->addr, sizeof(SLE_Addr_S));
    accessParam.version = param->version;
    accessParam.localIndex = param->localIndex;

    uint32_t ret = SleAccessLinkEstablishReq(&accessParam);
    if (ret != CM_SUCCESS) {
        CM_LOGE("failed");
        SleLogicLinkRemove(link);
        CM_NotifyConnectionReqCanceled(param, DLI_INVALID_PARAMETERS);
        return ret;
    }
    CM_LOGI("success");
    return CM_SUCCESS;
}

static void CM_NotifyReleasingReqCanceled(CM_DisconnectParamReq_S *param, uint8_t discReason)
{
    // 直接取消连接释放请求响应给用户，不做下发命令给芯片操作
    CM_LogicLinkState_S rsp = { 0 };
    rsp.role = CM_G_NODE;  // 建链失败，默认无效值
    rsp.lcid = CM_INVALID_LCID;
    (void)memcpy_s(&rsp.addr, sizeof(rsp.addr), &param->addr, sizeof(param->addr));
    rsp.result = CM_LINK_STATE_DISCONNECTED;
    rsp.discReason = discReason;
    CM_LOGI("notify cancel releasing req rsp, addr:%s, discReason:0x%02x", GET_ENC_ADDR(&param->addr), discReason);
    CM_NotifyLogicLinkCbks(&rsp);
}

uint32_t CM_ConnectReleaseReq(CM_DisconnectParamReq_S *param)
{
    CM_CHECK_RETURN_RET(g_cmIsInited, CM_NOT_INITED, "CM has not inited");
    CM_CHECK_RETURN_RET(param != NULL, CM_INVALID_PARAM_ERR, "param is null");
    CM_CHECK_RETURN_RET(((param->discReason == CM_DISC_REASON_REMOTE_USER_TERMINATED) ||
        (param->discReason == CM_DISC_REASON_CANCEL_PAIR) || (param->discReason == CM_DISC_REASON_COMMAND_TIMEOUT)),
        CM_INVALID_PARAM_ERR, "discReason:0x%02x is invalid", param->discReason);

    CM_CheckAndFixAddrType(&param->addr);

    SleLogicLink_S *link = NULL;
    CM_AccessParamReq_S accessParam = { 0 };
    link = SleLogicLinkGetByAddr(&param->addr);
    if (link == NULL) {
        CM_LOGE("sle connection release link failed, link is not exist, addr:%s", GET_ENC_ADDR(&param->addr));
        CM_NotifyReleasingReqCanceled(param, DLI_INVALID_PARAMETERS);
        return CM_FAIL;
    }
    if (link->status != CM_LINK_STATE_CONNECTED) {
        CM_LOGE("sle connection release link failed, link handle:0x%04x, status:%u", link->lcid, link->status);
        CM_NotifyReleasingReqCanceled(param, DLI_COMMAND_DISALLOWED);
        return CM_FAIL;
    }
    link->status = CM_LINK_STATE_DISCONNECTTING;
    accessParam.lcid = link->lcid;
    accessParam.version = CM_CONNECT_VERSION_1_0;
    accessParam.localIndex = CM_CONNECT_LOCAL_INDEX_0;
    accessParam.discReason = param->discReason;
    CM_LOGI("start, lcid:0x%04x, version:0x%02x, localIndex:0x%04x, discReason:0x%02x",
        accessParam.lcid, accessParam.version, accessParam.localIndex, accessParam.discReason);
    uint32_t ret = SleAccessLinkReleaseReq(&accessParam);
    if (ret != CM_SUCCESS) {
        link->status = CM_LINK_STATE_CONNECTED;
        CM_NotifyReleasingReqCanceled(param, DLI_INVALID_PARAMETERS);
    }
    return ret;
}

uint32_t CM_ConnectSetParamReq(CM_ConnectSetParamReq_S *param)
{
    CM_CHECK_RETURN_RET(g_cmIsInited, CM_NOT_INITED, "CM has not inited");
    CM_CHECK_RETURN_RET(param != NULL, CM_INVALID_PARAM_ERR, "param is null");

    CM_LOGI("bitFrameType:0x%02x, scanInterval:0x%04x, scanWindow:0x%04x, minInterval:0x%04x, "
        "maxInterval:0x%04x, timeout:0x%04x",
        param->bitFrameType, param->scanInterval, param->scanWindow, param->minInterval,
        param->maxInterval, param->timeout);
    SleSetDliConnectParam(param);
    return CM_SUCCESS;
}

static void CM_NotifyUpdateParamReqReqCanceled(CM_ConnectUpdateParamReq_S *param, uint8_t result)
{
    // 直接取消参数更新请求响应给用户，不做下发命令给芯片操作
    CM_ConnectUpdateParamRsp_S rsp = { 0 };
    rsp.localIndex = CM_CONNECT_LOCAL_INDEX_0;
    rsp.version = CM_CONNECT_VERSION_1_0;
    rsp.lcid = CM_INVALID_LCID;
    (void)memcpy_s(&rsp.addr, sizeof(rsp.addr), &param->addr, sizeof(param->addr));
    rsp.result = result;
    CM_LOGI("notify cancel update param req rsp, addr:%s, result:0x%02x", GET_ENC_ADDR(&param->addr), result);
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_CONNECT_PARAM_UPDATE, &rsp);
    CM_LogicLinkConnUpdateParam_S updateParamRsp = { 0 };
    updateParamRsp.result = result;
    updateParamRsp.lcid = CM_INVALID_LCID;
    (void)memcpy_s(&updateParamRsp.addr, sizeof(SLE_Addr_S), &param->addr, sizeof(SLE_Addr_S));
    CM_ExecLogicLinkConnUpdateParamCbks(&updateParamRsp);
}

static void CM_ConnectUpdateParamReqInner(void *arg)
{
    CM_LOGI("enter");
    CM_CHECK_RETURN(g_cmIsInited, "CM has not inited");
    CM_ConnectUpdateParamReq_S *param = (CM_ConnectUpdateParamReq_S *)arg;
    CM_CHECK_RETURN(param != NULL, "param is null");
    CM_CheckAndFixAddrType(&param->addr);
    SleLogicLink_S *link = NULL;
    link = SleLogicLinkGetByAddr(&param->addr);
    if (link == NULL) {
        CM_LOGE("sle logic link get by addr failed");
        CM_NotifyUpdateParamReqReqCanceled(param, DLI_INVALID_PARAMETERS);
        return;
    }
    DLI_ConnectionUpdateParam updateParam = { 0 };
    updateParam.connHandle = link->lcid;
    const uint16_t CM_CONN_TEMP_INTERVAL = 0x18;
    bool isSupport = COMMON_IsSupportSetMaxInterval();
    if (isSupport && (param->intervalMax < CM_CONN_TEMP_INTERVAL)) { // 芯片默认按max参数调度
        updateParam.connIntervalMin = CM_CONN_TEMP_INTERVAL;
        updateParam.connIntervalMax = CM_CONN_TEMP_INTERVAL;
        CM_LOGI("updated intervalMin and intervalMax to 0x%04x", CM_CONN_TEMP_INTERVAL);
    } else {
        updateParam.connIntervalMin = param->intervalMin;
        updateParam.connIntervalMax = param->intervalMax;
    }
    // 共存场景下，HID interval设置不能小于15ms
    uint16_t coexInterval = 0;
    if (CM_AdjustCoexAcbInterval(&link->rmtAddr, updateParam.connIntervalMin, &coexInterval)) {
        updateParam.connIntervalMin = coexInterval;
        updateParam.connIntervalMax = coexInterval;
    }
    updateParam.txRxInterval = param->txRxInterval;
    updateParam.eventInterval = param->eventInterval;
    updateParam.maxLatency = param->maxLatency;
    updateParam.supervisionTimeout = param->supervisionTimeout;
    updateParam.systemTimeUnit = param->systemTimeUnit;
    updateParam.txRxFlag = param->txRxFlag;
    uint32_t ret = DLI_UpdateConnectionParam(param->version, param->localIndex, &updateParam);
    if (ret != DLI_SUCCESS) {
        CM_LOGE("DLI_UpdateConnectionParam failed, ret:0x%08x", ret);
        CM_NotifyUpdateParamReqReqCanceled(param, DLI_INVALID_PARAMETERS);
        return;
    }
}

uint32_t CM_ConnectUpdateParamReq(CM_ConnectUpdateParamReq_S *param)
{
    CM_CHECK_RETURN_RET(param != NULL, CM_INVALID_PARAM_ERR, "param is null");

    CM_ConnectUpdateParamReq_S *req = (CM_ConnectUpdateParamReq_S *)SDF_MemZalloc(sizeof(CM_ConnectUpdateParamReq_S));
    CM_CHECK_RETURN_RET(req != NULL, CM_MEM_ERR, "mem zalloc error");

    (void)memcpy_s(req, sizeof(CM_ConnectUpdateParamReq_S), param, sizeof(CM_ConnectUpdateParamReq_S));
    uint32_t ret = CP_PostTask((SDF_WorkCb)CM_ConnectUpdateParamReqInner, (void *)req,
        (SDF_FreeWorkArg)CM_FreeCommonReqParamData);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTask failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_ConnectCancelReq(void)
{
    CM_CHECK_RETURN_RET(g_cmIsInited, CM_NOT_INITED, "CM has not inited");

    SleLogicLink_S *link = SleLogicLinkGetByStatus(CM_LINK_STATE_CONNECTING);
    if (link == NULL) {
        CM_LOGI("CM_ConnectCancelReq no connecting link, ignore this req");
        return CM_SUCCESS;
    }

    SleLogicLinkRemove(link); // 取消连接之前，需要先删除连接中的设备, 用于后续重新发起白名单连接

    uint32_t ret = DLI_CancelCreateConnection();
    if (ret != DLI_SUCCESS) {
        CM_LOGE("DLI_CancelCreateConnection failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

// 统一在被调用点（NAI_EnableStack）切主线程, 此函数不切线程
void CM_ReadAcceptFilterListSize(void)
{
    CM_LOGI("enter");
    uint32_t ret = DLI_ReadAcceptFilterListSize();
    CM_CHECK_RETURN(ret == DLI_SUCCESS, "DLI_ReadAcceptFilterListSize failed, ret:0x%08x", ret);
}

void CM_ClearAcceptFilterList(void)
{
    CM_CHECK_RETURN(g_cmIsInited, "CM has not inited");

    uint32_t ret = DLI_ClearAcceptFilterList();
    if (ret != DLI_SUCCESS) {
        CM_LOGE("DLI_ClearAcceptFilterList failed, ret:0x%08x", ret);
    }
}

uint32_t CM_AddDeviceToAcceptFilterList(SLE_Addr_S *addr, bool isBypass)
{
    CM_CHECK_RETURN_RET(g_cmIsInited, CM_NOT_INITED, "CM has not inited");
    CM_CHECK_RETURN_RET(addr != NULL, CM_INVALID_PARAM_ERR, "param is null");

    CM_CheckAndFixAddrType(addr);

    SleLogicLink_S *sll = SleLogicLinkGetByAddr(addr);
    if (sll != NULL) {
        CM_LOGW("add a device addr:%s has in logic link, status:%u, possible invalid when allowlist connection",
            GET_ENC_ADDR(addr), sll->status);
    }
    uint32_t ret = DLI_SUCCESS;
    if (DLI_IsSupportConnBypassAdv()) {
        DLI_AddrExtStru addDevice = {0};
        (void)memcpy_s(addDevice.addr, SLE_ADDR_LEN, addr->addr, SLE_ADDR_LEN);
        addDevice.ext |= (uint8_t)isBypass;
        if (DLI_GetExtFuncList()->addDeviceToAcceptFilterListExt == NULL) {
            CM_LOGW("addDeviceToAcceptFilterListExt callback is null, use the default path");
            return DLI_AddDeviceToAcceptFilterList(addr);
        }
        CM_LOGD("add device to access filter list ext, addr: %s, ext:%hhu", GET_ENC_ADDR(addr), addDevice.ext);
        ret = DLI_GetExtFuncList()->addDeviceToAcceptFilterListExt(&addDevice);
    } else {
        ret = DLI_AddDeviceToAcceptFilterList(addr);
    }
    if (ret != DLI_SUCCESS) {
        CM_LOGE("dli add device to accept filter list failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_RemoveDeviceFromAcceptFilterList(SLE_Addr_S *addr)
{
    CM_CHECK_RETURN_RET(g_cmIsInited, CM_NOT_INITED, "CM has not inited");
    CM_CHECK_RETURN_RET(addr != NULL, CM_INVALID_PARAM_ERR, "param is null");

    CM_CheckAndFixAddrType(addr);

    uint32_t ret = DLI_RemoveDeviceFromAcceptFilterList(addr);
    if (ret != DLI_SUCCESS) {
        CM_LOGE("DLI_RemoveDeviceFromAcceptFilterList failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

static void CM_NotifySetPhyReqReqCanceled(CM_SetPhyReq_S *param, uint8_t result)
{
    // 直接取消设置Phy命令请求响应给用户，不做下发命令给芯片操作
    // 命令异常，除必要参数外，其他值无效，设为0即可
    CM_SetPhyRsp_S rsp = { 0 };
    rsp.status = result;
    rsp.lcid = param->lcid;
    CM_LOGI("notify cancel set phy param req rsp, lcid:0x%04x, result:0x%02x", param->lcid, result);
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_SET_PHY, &rsp);
}

static void CM_SetPhyInner(void *arg)
{
    CM_LOGI("enter");
    CM_CHECK_RETURN(g_cmIsInited, "CM has not inited");
    CM_SetPhyReq_S *param = (CM_SetPhyReq_S *)arg;
    CM_CHECK_RETURN(param != NULL, "param is null");

    SleLogicLink_S *sll = SleLogicLinkGetByLcid(param->lcid);
    if (sll == NULL) {
        CM_LOGE("SleLogicLinkGetByLcid failed, lcid:0x%04x", param->lcid);
        CM_NotifySetPhyReqReqCanceled(param, DLI_UNKNOWN_CONNECTION_IDENTIFIER);
        return;
    }

    DLI_SetPhyParam setParam = {0};
    setParam.connHandle = param->lcid;
    setParam.txFormat = param->txFormat;
    setParam.rxFormat = param->rxFormat;
    setParam.txPhy = param->txPhy;
    setParam.rxPhy = param->rxPhy;
    setParam.txPilotDensity = param->txPilotDensity;
    setParam.rxPilotDensity = param->rxPilotDensity;
    setParam.gFeedback = param->gFeedback;
    setParam.tFeedback = param->tFeedback;

    CM_LOGI("connHandle:0x%04X, txFormat:0x%02X, rxFormat:0x%02X, txPhy:0x%02X, "
             "rxPhy:0x%02X, txPilotDensity:0x%02X, rxPilotDensity:0x%02X, gFeedback:0x%02X, tFeedback:0x%02X",
        setParam.connHandle, setParam.txFormat, setParam.rxFormat, setParam.txPhy,
        setParam.rxPhy, setParam.txPilotDensity, setParam.rxPilotDensity, setParam.gFeedback, setParam.tFeedback);
    uint32_t ret = SleAccessSetPhy(&setParam);
    if (ret != DLI_SUCCESS) {
        CM_LOGE("SleAccessSetPhy failed, ret:0x%08x", ret);
        CM_NotifySetPhyReqReqCanceled(param, DLI_INVALID_PARAMETERS);
        return;
    }
}

uint32_t CM_SetPhy(CM_SetPhyReq_S *param)
{
    CM_LOGI("enter");
    CM_CHECK_RETURN_RET(param != NULL, CM_INVALID_PARAM_ERR, "param is null");

    CM_SetPhyReq_S *req = (CM_SetPhyReq_S *)SDF_MemZalloc(sizeof(CM_SetPhyReq_S));
    CM_CHECK_RETURN_RET(req != NULL, CM_MEM_ERR, "mem zalloc error");

    (void)memcpy_s(req, sizeof(CM_SetPhyReq_S), param, sizeof(CM_SetPhyReq_S));
    uint32_t ret = CP_PostTask((SDF_WorkCb)CM_SetPhyInner, (void *)req, (SDF_FreeWorkArg)CM_FreeCommonReqParamData);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTask failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

static void CM_SetHostChannelClassificationInner(void *arg)
{
    CM_LOGI("enter");
    CM_CHECK_RETURN(g_cmIsInited, "CM has not inited");
    CM_SetChannelMapReq_S *freq = (CM_SetChannelMapReq_S *)arg;
    CM_CHECK_RETURN(freq != NULL, "param is null");

    uint32_t ret = DLI_SetHostChannelClassification(freq->channelMap, CM_CHANNEL_MAP_LEN);
    CM_CHECK_RETURN(ret == DLI_SUCCESS, "DLI_SetHostChannelClassification failed, ret:0x%08x", ret);
}

uint32_t CM_SetHostChannelClassification(CM_SetChannelMapReq_S *param)
{
    CM_CHECK_RETURN_RET(param != NULL, CM_INVALID_PARAM_ERR, "param is null");

    CM_SetChannelMapReq_S *freq = SDF_MemZalloc(sizeof(CM_SetChannelMapReq_S));
    CM_CHECK_RETURN_RET(freq != NULL, CM_MEM_ERR, "mem zalloc error");

    (void)memcpy_s(freq, sizeof(CM_SetChannelMapReq_S), param, sizeof(CM_SetChannelMapReq_S));
    uint32_t ret = CP_PostTask((SDF_WorkCb)CM_SetHostChannelClassificationInner, (void *)freq,
        (SDF_FreeWorkArg)CM_FreeCommonReqParamData);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTask failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_GetLogicLinkByAddr(SLE_Addr_S *addr, CM_LogicLink_S *logicLink)
{
    CM_CHECK_RETURN_RET(g_cmIsInited, CM_NOT_INITED, "CM has not inited");
    CM_CHECK_RETURN_RET((addr != NULL && logicLink != NULL), CM_INVALID_PARAM_ERR, "param is null");

    CM_CheckAndFixAddrType(addr);
    SleLogicLink_S *sll = SleLogicLinkGetByAddr(addr);
    if (sll == NULL) {
        CM_LOGE("SleLogicLinkGetByAddr:%s failed", GET_ENC_ADDR(addr));
        return CM_NOT_FOUND;
    }
    logicLink->lcid = sll->lcid;
    logicLink->role = sll->role;
    (void)memcpy_s(&logicLink->addr, sizeof(SLE_Addr_S), &sll->rmtAddr, sizeof(SLE_Addr_S));
    return CM_SUCCESS;
}

uint32_t CM_GetLogicLinkByLcid(uint16_t lcid, CM_LogicLink_S *logicLink)
{
    CM_CHECK_RETURN_RET(g_cmIsInited, CM_NOT_INITED, "CM has not inited");
    CM_CHECK_RETURN_RET(logicLink != NULL, CM_INVALID_PARAM_ERR, "param is null");

    SleLogicLink_S *sll = SleLogicLinkGetByLcid(lcid);
    if (sll == NULL) {
        CM_LOGE("SleLogicLinkGetByLcid failed, lcid:0x%04x", lcid);
        return CM_NOT_FOUND;
    }
    logicLink->lcid = sll->lcid;
    logicLink->role = sll->role;
    (void)memcpy_s(&logicLink->addr, sizeof(SLE_Addr_S), &sll->rmtAddr, sizeof(SLE_Addr_S));
    return CM_SUCCESS;
}

uint32_t CM_GetLogicLinkConnectedSize(void)
{
    CM_CHECK_RETURN_RET(g_cmIsInited, 0, "CM has not inited");
    return SleLogicLinkGetConnectedSize();
}

uint32_t CM_SetLogicLinkDeviceType(uint16_t lcid, uint8_t deviceType)
{
    CM_CHECK_RETURN_RET(g_cmIsInited, CM_NOT_INITED, "CM has not inited");
    CM_CHECK_RETURN_RET(deviceType == CM_DEVTYPE_OLD, CM_INVALID_PARAM_ERR, "deviceType:%hhu is invalid", deviceType);
    SleLogicLink_S *sll = SleLogicLinkGetByLcid(lcid);
    if (sll == NULL) {
        CM_LOGE("SleLogicLinkGetByLcid failed, lcid:0x%04x", lcid);
        return CM_NOT_FOUND;
    }
    sll->devType = deviceType;
    CM_LOGI("lcid:0x%04x, deviceType:%hhu", lcid, deviceType);
    return CM_SUCCESS;
}

static void CM_NotifyEnableConnHighPowerReqCanceled(CM_EnableConnHighPowerReq_S *param, uint8_t result)
{
    CM_LOGI("notify cancel enable conn high power req rsp, lcid:0x%04x, result:0x%02x", param->lcid, result);
    // 连接请求异常，直接通知用户，不走芯片命令处理
    CM_EnableConnHighPowerRsp_S rsp = { 0 };
    rsp.lcid = param->lcid;
    rsp.status = DLI_UNKNOWN_CONNECTION_IDENTIFIER;
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_ENABLE_CONN_HIGH_POWER, &rsp);
}

static void CM_EnableRealConnHighPowerInner(CM_EnableConnHighPowerReq_S *inParam,
    uint32_t (*enableConnHighPower)(DLI_EnableConnHighPowerParam *param))
{
    CM_CHECK_RETURN(g_cmIsInited, "CM has not inited");
    SleLogicLink_S *sll = SleLogicLinkGetByLcid(inParam->lcid);
    if (sll == NULL) {
        CM_LOGE("SleLogicLinkGetByLcid failed, lcid:0x%04x", inParam->lcid);
        CM_NotifyEnableConnHighPowerReqCanceled(inParam, DLI_UNKNOWN_CONNECTION_IDENTIFIER);
        return;
    }

    DLI_EnableConnHighPowerParam param = {0};
    param.connHandle = inParam->lcid;
    param.enable = inParam->enable;
    param.powerLevel = inParam->powerLevel;
    CM_LOGI("connHandle:0x%04X, enable:%d, powerLevel:0x%02X", param.connHandle, param.enable, param.powerLevel);

    uint32_t ret = enableConnHighPower(&param);
    if (ret != DLI_SUCCESS) {
        CM_LOGE("DLI_EnableConnHighPower failed, ret:0x%08x", ret);
        CM_NotifyEnableConnHighPowerReqCanceled(inParam, DLI_COMMAND_DISALLOWED);
        return;
    }
}

uint32_t CM_EnableRealConnHighPower(CM_EnableConnHighPowerReq_S *inParam,
    uint32_t (*enableConnHighPower)(DLI_EnableConnHighPowerParam *param))
{
    CM_CHECK_RETURN_RET(inParam != NULL, CM_INVALID_PARAM_ERR, "inParam is null");
    CM_CHECK_RETURN_RET(enableConnHighPower != NULL, CM_INVALID_PARAM_ERR, "enableConnHighPower is null");
    CM_EnableRealConnHighPowerInner(inParam, enableConnHighPower);
    return CM_SUCCESS;
}

static void CM_NotifySubrateEvent(const SleLogicLink_S *link, const CM_SetACBSubrateParam *cmParam)
{
    CM_AcbSubrateCbParam_S subrateParam = {};
    if (link->subrate == 0) {
        subrateParam.subrate = cmParam->subrate;
        if (cmParam->onlySubrate) {
            subrateParam.subrateMax = SLE_ACB_SUBRATE_MAX;
            subrateParam.maxLatency = SLE_ACB_MAX_LATENCY;
            subrateParam.continuationNum = SLE_ACB_CONTINUATION_NUM;
            subrateParam.supervisionTimeout = SLE_ACB_SUPERVISION_TIMEOUT;
        } else {
            subrateParam.subrateMax = cmParam->subrateMax;
            subrateParam.maxLatency = cmParam->maxLatency;
            subrateParam.continuationNum = cmParam->continuationNum;
            subrateParam.supervisionTimeout = cmParam->supervisionTimeout;
        }
    } else {
        subrateParam.subrate = link->subrate;
        subrateParam.subrateMax = link->subrateMax;
        subrateParam.maxLatency = link->maxLatency;
        subrateParam.continuationNum = link->continuationNum;
        subrateParam.supervisionTimeout = link->supervisionTimeout;
    }
    (void)memcpy_s(&subrateParam.addr, sizeof(SLE_Addr_S), &link->rmtAddr, sizeof(SLE_Addr_S));
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_SET_SUBRATE, (void *)&subrateParam);
}

static void CM_SetRealACBSubrateInner(const CM_SetACBSubrateParam *cmParam,
    uint32_t (*setSubrate)(DLI_ACBSubrateParam *dliParam))
{
    CM_LOGD("enter");
    CM_CHECK_RETURN(g_cmIsInited, "CM has not inited");
    SleLogicLink_S *link = SleLogicLinkGetByAddr(&cmParam->addr);
    if (link == NULL) {
        CM_LOGE("logic link get addr fail, addr:%s", GET_ENC_ADDR(&cmParam->addr));
        return;
    }

    bool isSubrateSupported = CM_IsRemotePrivateFeatureSupport(link, CM_PRIVATE_FEATURES_BIT_SUBRATE);
    if (!isSubrateSupported) {
        CM_LOGI("subrate is not supported, req subrate is %hu, cur subrate is %hu, reject",
            cmParam->subrate, link->subrate);
        CM_NotifySubrateEvent(link, cmParam);
        return;
    }

    if (link->subrate == cmParam->subrate) {
        CM_LOGI("subrate is %u, no need to change, is updating %u", cmParam->subrate, link->isSubrateUpdating);
        if (link->isSubrateUpdating) {
            return;
        }
        CM_NotifySubrateEvent(link, cmParam);
        return;
    }

    DLI_ACBSubrateParam subrateParam = {};
    subrateParam.lcid = link->lcid;
    subrateParam.subrateMin = cmParam->subrate;
    if (cmParam->onlySubrate) {
        subrateParam.subrateMax = SLE_ACB_SUBRATE_MAX;
        subrateParam.maxLatency = SLE_ACB_MAX_LATENCY;
        subrateParam.continuationNum = SLE_ACB_CONTINUATION_NUM;
        subrateParam.supervisionTimeout = SLE_ACB_SUPERVISION_TIMEOUT;
    } else {
        subrateParam.subrateMax = cmParam->subrateMax;
        subrateParam.maxLatency = cmParam->maxLatency;
        subrateParam.continuationNum = cmParam->continuationNum;
        subrateParam.supervisionTimeout = cmParam->supervisionTimeout;
    }

    CM_LOGI("lcid:0x%04x, onlySubrate:%u, subrateMin:%u, subrateMax:%u, maxLatency:%u, continuationNum:%u, "
        "supervisionTimeout:%u", subrateParam.lcid, cmParam->onlySubrate, subrateParam.subrateMin,
        subrateParam.subrateMax, subrateParam.maxLatency, subrateParam.continuationNum,
        subrateParam.supervisionTimeout);
    uint32_t ret = setSubrate(&subrateParam);
    if (ret != DLI_SUCCESS) {
        CM_LOGE("DLI_ACBSetSubrate failed, ret:0x%08x", ret);
        CM_NotifySubrateEvent(link, cmParam);
        return;
    }

    link->isSubrateUpdating = true;
    link->subrate = cmParam->subrate;
}

uint32_t CM_SetRealACBSubrate(const CM_SetACBSubrateParam *param, uint32_t (*setSubrate)(DLI_ACBSubrateParam *dliParam))
{
    CM_CHECK_RETURN_RET(param != NULL, CM_INVALID_PARAM_ERR, "param is null");
    CM_CHECK_RETURN_RET(setSubrate != NULL, CM_INVALID_PARAM_ERR, "setSubrate is null");
    CM_SetRealACBSubrateInner(param, setSubrate);
    return CM_SUCCESS;
}

uint32_t CM_InnerSetACBSubrate(const CM_SetACBSubrateInnerParam *param)
{
    SleLogicLink_S *link = SleLogicLinkGetByLcid(param->lcid);
    if (link == NULL) {
        CM_LOGE("logic link get failed, lcid: %u", param->lcid);
        return CM_FAIL;
    }
    if (link->subrate == param->subrate) {
        CM_LOGI("lcid %u subrate is already %u, no need to change", param->lcid, param->subrate);
        return CM_SUCCESS;
    }

    DLI_ACBSubrateParam subrateParam = {};
    subrateParam.lcid = param->lcid;
    subrateParam.subrateMin = param->subrate;
    subrateParam.subrateMax = SLE_ACB_SUBRATE_MAX;
    subrateParam.maxLatency = SLE_ACB_MAX_LATENCY;
    subrateParam.continuationNum = SLE_ACB_CONTINUATION_NUM;
    subrateParam.supervisionTimeout = SLE_ACB_SUPERVISION_TIMEOUT;
    uint32_t ret = DLI_ACBSetSubrate(&subrateParam);
    if (ret != DLI_SUCCESS) {
        CM_LOGE("DLI_ACBSetSubrate failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    link->isSubrateUpdating = true;
    link->subrate = param->subrate;
    return CM_SUCCESS;
}

static void CM_GetRssiInner(void *param)
{
    CM_CHECK_RETURN(g_cmIsInited, "CM has not inited");

    CM_ReadRemoteRssiReq_S *cmParam = (CM_ReadRemoteRssiReq_S *)param;
    DLI_ConnHandleStru setParam = {0};
    setParam.connHandle = cmParam->lcid;
    uint32_t ret = DLI_ReadRemoteRssi(&setParam);
    CM_CHECK_RETURN(ret == DLI_SUCCESS, "CM_GetRssiInner failed, ret:0x%08x", ret);
}

uint32_t CM_GetRssi(const CM_ReadRemoteRssiReq_S *param)
{
    CM_CHECK_RETURN_RET(param != NULL, CM_INVALID_PARAM_ERR, "param is null");

    CM_ReadRemoteRssiReq_S *readRssiParam = (CM_ReadRemoteRssiReq_S *)SDF_MemZalloc(sizeof(CM_ReadRemoteRssiReq_S));
    CM_CHECK_RETURN_RET(readRssiParam != NULL, CM_MEM_ERR, "mem zalloc error");
    (void)memcpy_s(readRssiParam, sizeof(CM_ReadRemoteRssiReq_S), param, sizeof(CM_ReadRemoteRssiReq_S));

    uint32_t ret = CP_PostTask(CM_GetRssiInner, (void *)readRssiParam, SDF_MemFree);
    CM_CHECK_RETURN_RET(ret == CP_OK, CM_FAIL, "CP_PostTask failed, ret:0x%08x", ret);
    return CM_SUCCESS;
}

uint16_t CM_GetLcidByConnHandle(uint16_t connHandle)
{
    CM_LOGD("enter");
    CM_GetLcidCtx ctx = {0};
    ctx.connHandle = connHandle;
    uint32_t ret = CP_PostTaskBlocked(CM_GetLcidByConnHandleInner, (void *)&ctx, NULL, CM_INIT_AND_DEINIT_TIMEOUT_MS);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTaskBlocked failed, ret:0x%08x", ret);
        return CM_INVALID_LCID;
    }
    return ctx.lcid;
}

uint32_t CM_SetRemoteFeature(const NLSTK_RemoteFeatureInfo_S *info)
{
    CM_CHECK_RETURN_RET(info != NULL, CM_INVALID_PARAM_ERR, "info is null");
    SleLogicLink_S *link = SleLogicLinkGetByLcid(info->lcid);
    if (link == NULL) {
        CM_LOGE("logic link get lcid fail, lcid:0x%04x", info->lcid);
        return CM_INVALID_PARAM_ERR;
    }
    if (memcpy_s(link->remotePrivateFeature, sizeof(link->remotePrivateFeature),
        info->features, sizeof(info->features)) != EOK) {
        CM_LOGE("memcpy feature failed");
        return CM_MEM_ERR;
    }

    for (uint32_t i = 0; i < CM_PRIVATE_FEATURES_LEN; i++) {
        CM_LOGI("private feature[%u]=0x%02x", i, link->remotePrivateFeature[i]);
    }
    return CM_SUCCESS;
}

static bool CM_IsRemotePrivateFeatureSupport(SleLogicLink_S *link, uint16_t featureBit)
{
    uint16_t featureIndex = featureBit / BITS_OF_BYTE;
    if (featureIndex >= CM_PRIVATE_FEATURES_LEN) {
        CM_LOGE("featureIndex is invalid, index: %hu", featureIndex);
        return false;
    }

    return link->remotePrivateFeature[featureIndex] & (1 << (featureBit % BITS_OF_BYTE));
}

bool CM_InnerGetRemotePrivateFeature(uint16_t lcid, uint16_t featureBit)
{
    SleLogicLink_S *link = SleLogicLinkGetByLcid(lcid);
    if (link == NULL) {
        CM_LOGE("logic link get failed, lcid: %hu", lcid);
        return false;
    }

    return CM_IsRemotePrivateFeatureSupport(link, featureBit);
}

bool CM_AdjustCoexAcbInterval(SLE_Addr_S *addr, uint16_t incommingInterval, uint16_t *coexInterval)
{
    CM_CHECK_RETURN_RET(addr != NULL, false, "addr is null");
    CM_CHECK_RETURN_RET(coexInterval != NULL, false, "coexInterval is null");
    CM_HidCoexModeParam_S coexParam = { 0 };
    coexParam.eventType = CM_SLE_CBK_EVENT_GET_HID_COEX_INTERVAL;
    coexParam.addr = *addr;
    coexParam.incomingInterval = incommingInterval;
    coexParam.coexInterval = 0;
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_HID_COEX_MODE, &coexParam);
    if (coexParam.coexInterval != 0) {
        CM_LOGI("sle connection update in hid coex mode, addr: %s, incoming interval: %d, "
            "coex interval: %d", GET_ENC_ADDR(addr), incommingInterval, coexParam.coexInterval);
        *coexInterval = coexParam.coexInterval;
    }
    coexParam.eventType = CM_SLE_CBK_EVENT_HID_COEX_MODE_PARAM_UPDATE;
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_HID_COEX_MODE, &coexParam);
    return coexParam.coexInterval != 0;
}