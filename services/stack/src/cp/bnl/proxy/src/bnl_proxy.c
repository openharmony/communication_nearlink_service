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

#include "nlstk_log.h"
#include "sdf_mem.h"
#include "cm.h"
#include "cm_api.h"
#include "dtap.h"
#include "dtap_tcid.h"
#include "nlstk_bnl_type_ext.h"
#include "bnl_ext_func_wrapper.h"
#include "bnl_proxy.h"

#define NET_BUF_MAX_SIZE 2048

static void BNL_ConnectDevice(const SLE_Addr_S *addr)
{
    CM_DirectConnAddrParam_S connParam = {.addr = *addr, .frameType = CM_CONN_PARAM_FRAME_TYPE_4};
    uint32_t ret = CM_DirectConnectAdd(CM_MODULE_BNL, &connParam);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[BNL] collab connect device failed,ret=%u", ret);
    }
}

static void BNL_DisConnectDevice(const SLE_Addr_S *addr)
{
    uint32_t ret = CM_DirectConnectRemove(CM_MODULE_BNL, addr, CM_DISC_REASON_REMOTE_USER_TERMINATED);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[BNL] collab disconnect device failed,ret=%u", ret);
    }
}

static void BNL_SendMsg(const uint8_t *cmdVal, uint32_t cmdLen)
{
    NLSTK_CHECK_RETURN_VOID(cmdVal, "[BNL] cmd value is null");
    NLSTK_CHECK_RETURN_VOID(cmdLen > sizeof(NLSTK_BnlSendMsg_S), "[BNL] cmd value len is error");
    NLSTK_BnlSendMsg_S* cmd = (NLSTK_BnlSendMsg_S*)cmdVal;
    NLSTK_CHECK_RETURN_VOID(cmd->len <= NET_BUF_MAX_SIZE && cmd->len == cmdLen - sizeof(NLSTK_BnlSendMsg_S),
        "[BNL] cmd len invalid");
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(cmd->len);
    NLSTK_CHECK_RETURN_VOID(buff != NULL, "[BNL] alloc buff failed");
    uint8_t *buf = SDF_BuffAppend(buff, cmd->len);
    if (buf == NULL) {
        NLSTK_LOG_ERROR("[BNL] buff append fail");
        SDF_BuffFree(buff);
        return;
    }
    (void)memcpy_s(buf, cmd->len, cmd->val, cmd->len);
    DTAP_Data_S data = {
        .pi = cmd->pi,
        .lcid = cmd->lcid,
        .tcid = cmd->tcid,
        .buff = buff,
    };
    NLSTK_LOG_INFO("[BNL] send msg len: %u", cmd->len);
    uint32_t ret = DTAP_DataSend(&data);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[BNL] collab send msg failed,ret=%u", ret);
        SDF_BuffFree(buff);
    }
}

static void BNL_LogicLinkCbk(CM_LogicLinkState_S *state)
{
    NLSTK_BnlLogicLinkState_S linkState = {
        .addr = state->addr,
        .advHandle = state->advHandle,
        .connCompleteType = state->connCompleteType,
        .discReason = state->discReason,
        .lcid = state->lcid,
        .result = state->result,
        .role = state->role,
    };
    BNL_ProxyLinkStateChange(&linkState);
}

static int BNL_RecvDtapMsg(DTAP_Data_Info_S *info, SDF_Buff_S *buff)
{
    NLSTK_CHECK_RETURN(info != NULL && buff != NULL, NLSTK_ERRCODE_POINTER_NULL, "[BNL] recv info or buff is null");
    NLSTK_CHECK_RETURN(SDF_DataLenGet(buff) <= NET_BUF_MAX_SIZE && SDF_DataLenGet(buff) > 0,
        NLSTK_ERRCODE_FAIL, "[BNL] recv datalen is invalid");
    uint8_t *dataBuf = SDF_DataOffset(buff);
    uint16_t dataSize = (uint16_t)SDF_DataLenGet(buff);
    NLSTK_BnlSendMsg_S *msg = (NLSTK_BnlSendMsg_S *)SDF_MemZalloc(sizeof(NLSTK_BnlSendMsg_S) + dataSize);
    NLSTK_CHECK_RETURN(msg != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[BNL] alloc msg failed");
    msg->lcid = info->lcid;
    msg->pi = info->pi;
    msg->tcid = info->tcid;
    msg->len = dataSize;
    (void)memcpy_s(msg->val, dataSize, dataBuf, dataSize);
    BNL_ProxyRecvMsg(msg);
    SDF_MemFree(msg);
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t BNL_Init(void)
{
    NLSTK_LOG_INFO("[BNL] bnl init");
    NLSTK_BnlProxyFunc_S func = {0};
    func.connectDeviceFunc = BNL_ConnectDevice;
    func.disConnectDeviceFunc = BNL_DisConnectDevice;
    func.sendMsgFunc = BNL_SendMsg;
    uint32_t ret = BNL_ProxyInit(&func);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[BNL] proxy init failed, ret=%u", ret);
        return ret;
    }
    CM_LogicLinkCbks_S cmLinkCbk = {0};
    cmLinkCbk.moduleId = CM_MODULE_BNL;
    cmLinkCbk.logicLinkCbk = BNL_LogicLinkCbk;
    ret = CM_RegLogicLinkListener(&cmLinkCbk);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[BNL] reg link cbk failed, ret=%u", ret);
        BNL_ProxyDeInit();
        return ret;
    }
    ret = DTAP_RegisterProtoRecvCbk(DTAP_PI_WNP, BNL_RecvDtapMsg);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[BNL] reg data cbk failed, ret=%u", ret);
        BNL_ProxyDeInit();
        CM_UnRegLogicLinkListener(CM_MODULE_BNL);
        return ret;
    }
    ret = DTAP_RegisterProtoRecvCbk(DTAP_PI_WAP, BNL_RecvDtapMsg);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[BNL] reg data cbk failed, ret=%u", ret);
        BNL_ProxyDeInit();
        CM_UnRegLogicLinkListener(CM_MODULE_BNL);
        (void)DTAP_UnregisterProtoRecvCbk(DTAP_PI_WNP);
        return ret;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

void BNL_DeInit(void)
{
    BNL_ProxyDeInit();
    CM_UnRegLogicLinkListener(CM_MODULE_BNL);
    (void)DTAP_UnregisterProtoRecvCbk(DTAP_PI_WNP);
    (void)DTAP_UnregisterProtoRecvCbk(DTAP_PI_WAP);
}