/**
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "cm_signaling_internal.h"
#include "cm_log.h"
#include "cm_errno.h"
#include "cm_def.h"
#include "cm_logic_link_api.h"
#include "cm_signaling_cap.h"
#include "cm_signaling_manage.h"
#include "cm_signaling_version.h"
#include "cm_signaling_struct.h"
#include "sle_logic_link_mgr.h"

static void CM_SleAccessLinkEstablishQuery(uint16_t lcid)
{
    CM_CapabilityBitmap_S cap = {0};
    cap.relayCap = CM_CAP_ENABLE;
    cap.transMode = CM_CAP_ENABLE;
    cap.measurementCap = CM_CAP_ENABLE;
    cap.accessSlb = CM_CAP_ENABLE;
    cap.accessSle = CM_CAP_ENABLE;
    cap.version = CM_CAP_ENABLE;
    cap.mps = CM_CAP_ENABLE;
    cap.mtu = CM_CAP_ENABLE;
    cap.wnd = CM_CAP_ENABLE;
    if (CM_SendReqSignalingCapability(lcid, &cap) != 0) {
        CM_LOGE("lcid:0x%x CM_SendReqSignalingCapability failed", lcid);
        return;
    }
    CM_LOGI("lcid:0x%x CM_SendReqSignalingCapability", lcid);
}

static void CM_SignalingLogicLinkCbk(CM_LogicLinkState_S *state)
{
    CM_CHECK_RETURN(state != NULL, "param is null");
    if (state->result == CM_LINK_STATE_CONNECTED) {
        CM_SleAccessLinkEstablishQuery(state->lcid);
    } else if (state->result == CM_LINK_STATE_DISCONNECTED) {
        CM_SignalingCacheClearByLcid(state->lcid);
    }
}

uint32_t CM_SignalingInit(void)
{
    CM_LogicLinkCbks_S cmSignalingCbks = { 0 };
    cmSignalingCbks.moduleId = CM_MODULE_CM_SIGNALING;
    cmSignalingCbks.logicLinkCbk = CM_SignalingLogicLinkCbk;
    uint32_t ret = CM_RegLogicLinkListener(&cmSignalingCbks);
    CM_CHECK_RETURN_RET(ret == CM_SUCCESS, ret, "CM_RegLogicLinkListener failed, ret=0x%08x", ret);
    return CM_SignalingCacheInit();
}

void CM_SignalingDeInit(void)
{
    CM_SignalingCacheDeinit();
    CM_UnRegLogicLinkListener(CM_MODULE_CM_SIGNALING);
}

void CM_SetSendSignalingDataCbk(CM_SendSignalingDataCbk cbk)
{
    CM_SignalingRegisterCbk(cbk);
}

int CM_RecvSignalingData(DTAP_Data_Info_S *info, SDF_Buff_S *buff)
{
    if (info == NULL || buff == NULL || SDF_DataLenGet(buff) < sizeof(uint8_t)) {
        return CM_FAIL;
    }

    uint8_t code = *(SDF_DataOffset(buff));
    CM_LOGI("CM_DataRecv code 0x%02x, lcid 0x%04x dataLen %lu",
        code, info->lcid, SDF_BuffLenGet(buff));
    CM_SignalingHandle handle = CM_SignalingGetManagerHandler(code);
    if (handle == NULL) {
        return CM_FAIL;
    }
    CM_SignalingHead_S *head = CM_ParseSignalingBuff(buff);
    if (head == NULL) {
        return CM_FAIL;
    }
    handle(info->lcid, head);
    CM_SignalingCacheRemove(head->identifier, code);
    return CM_SUCCESS;
}

uint8_t CM_GetLogicLinkDeviceType(uint16_t lcid)
{
    return CM_GetDeviceLinkDeviceType(lcid);
}

uint32_t CM_GetLogicLinkCapInfo(CM_CapInfo_S *capInfo, const SLE_Addr_S *addr)
{
    if (capInfo == NULL) {
        return CM_INVALID_PARAM_ERR;
    }
    SleLogicLink_S* link = SleLogicLinkGetByAddr(addr);
    if (link == NULL) {
        CM_LOGE("sle logic link get by addr failed, addr:%s", GetEncryptAddr(addr).buf);
        return CM_NOT_FOUND;
    }
    capInfo->rxWnd = link->rxWnd;
    capInfo->supportTransMode = link->supportTransMode;
    return CM_SUCCESS;
}