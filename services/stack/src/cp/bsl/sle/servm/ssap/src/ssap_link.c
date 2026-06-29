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
#include <string.h>
#include "securec.h"
#include "sdf_mem.h"
#include "cpfwk_log.h"
#include "cp_worker.h"
#include "ssap_pkt.h"
#include "ssap_utils.h"
#include "ssap_common.h"
#include "ssap_link.h"

SDF_DListHead_S g_SsapLinkList;

static uint8_t g_SsapRecvOpcodeMap[SSAP_CODE_MAX] = {
    0,
    0,                               // SSAP_ERROR_RSP
    0,                               // SSAP_EXCHANGE_INFO_REQ
    SSAP_EXCHANGE_INFO_REQ,          // SSAP_EXCHANGE_INFO_RSP
    0,                               // SSAP_FIND_STRUCTURE_REQ
    SSAP_FIND_STRUCTURE_REQ,         // SSAP_FIND_STRUCTURE_RSP
    0,                               // SSAP_FIND_STRUCTURE_BY_UUID_REQ
    SSAP_FIND_STRUCTURE_BY_UUID_REQ, // SSAP_FIND_STRUCTURE_BY_UUID_RSP
    0,                               // SSAP_READ_REQ
    SSAP_READ_REQ,                   // SSAP_READ_RSP
    0,                               // SSAP_READ_BY_UUID_REQ
    SSAP_READ_BY_UUID_REQ,           // SSAP_READ_BY_UUID_RSP
    0,                               // SSAP_WRITE_CMD
    0,                               // SSAP_WRITE_REQ
    SSAP_WRITE_REQ,                  // SSAP_WRITE_RSP
    0,                               // SSAP_VALUE_NTF
    0,                               // SSAP_VALUE_IND
    SSAP_VALUE_IND,                  // SSAP_VALUE_ACK
    0,                               // SSAP_CALL_METHOD_CMD
    0,                               // SSAP_CALL_METHOD_REQ
    SSAP_CALL_METHOD_REQ             // SSAP_CALL_METHOD_RSP
};

uint32_t SSAP_LinkInit(void)
{
    SDF_DListHeadInit(&g_SsapLinkList);
    return SSAP_STACK_SUCCESS;
}

void SSAP_LinkDeInit(void)
{
    SSAP_LinkNode_S *linkNode = NULL;
    SSAP_LinkNode_S *tmpNode = NULL;
    SDF_DListElmSafeForeach(linkNode, tmpNode, &g_SsapLinkList, entry) {
        SSAP_DeleteSsapLinkByAddr(&(linkNode->link->addr));
    }
    SDF_DListHeadInit(&g_SsapLinkList);
}

SSAP_Link_S *SSAP_CreateSsapLinkWithInitReq(SLE_Addr_S *addr, uint16_t lcid, SendCb sendFunc, bool hasInitReqTask)
{
    CP_CHECK_LOG_RETURN(addr != NULL && sendFunc != NULL, NULL, "[SSAP] param is null");
    SSAP_Link_S *link = (SSAP_Link_S *)SDF_MemZalloc(sizeof(SSAP_Link_S));
    CP_CHECK_LOG_RETURN(link != NULL, NULL, "[SSAP] link mem alloc failed");
    SDF_DListHeadInit(&link->paramList);
    (void)memcpy_s(&link->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    link->lcid = lcid;
    link->mtu = SSAP_GetServerMtu();
    link->version = SSAP_EXCHANGE_VERSION;
    link->status = SSAP_LINK_IDLE;
    link->sendFunc = sendFunc;
    link->hasInitReqTask = hasInitReqTask;
    link->timerHandle = SSAP_TIMER_NO_USED_HANDLE;
    SSAP_LinkNode_S *linkNode = (SSAP_LinkNode_S *)SDF_MemZalloc(sizeof(SSAP_LinkNode_S));
    if (linkNode == NULL) {
        SDF_MemFree(link);
        return NULL;
    }
    SDF_DListEntryInit(&linkNode->entry);
    linkNode->link = link;
    SDF_DListElmTailInsert(&g_SsapLinkList, linkNode, entry);
    CP_LOG_INFO("[SSAP] ssap create link, addr = %s, lcid = %d", GET_ENC_ADDR(&link->addr), link->lcid);

    return link;
}

SSAP_Link_S *SSAP_CreateSsapLink(SLE_Addr_S *addr, uint16_t lcid, SendCb sendFunc)
{
    return SSAP_CreateSsapLinkWithInitReq(addr, lcid, sendFunc, false);
}

SSAP_TaskParam_S *SSAP_AllocTaskParam(SSAP_TaskParam_S *taskParam)
{
    CP_CHECK_LOG_RETURN(taskParam != NULL, NULL, "[SSAP] taskParam is null");
    SSAP_TaskParam_S *param = (SSAP_TaskParam_S *)SDF_MemZalloc(sizeof(SSAP_TaskParam_S));
    CP_CHECK_LOG_RETURN(param != NULL, NULL, "[SSAP] malloc param is null");
    param->appId = taskParam->appId;
    param->arg = taskParam->arg;
    param->freeFunc = taskParam->freeFunc;
    param->func = taskParam->func;
    param->timeout = taskParam->timeout;
    param->valid = taskParam->valid;
    param->appCallback = taskParam->appCallback;
    return param;
}

static void SSAP_FreeTaskParams(SSAP_TaskParam_S *param)
{
    CP_CHECK_LOG_RETURN_VOID(param != NULL, "[SSAP] param is null");
    if (param->arg != NULL && param->freeFunc != NULL) {
        param->freeFunc(param->arg);
    }
    SDF_MemFree(param);
}

void SSAP_DeleteSsapLinkByAddr(SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN_VOID(addr != NULL, "[SSAP] addr is null");
    SSAP_LinkNode_S *linkNode = NULL;
    SSAP_LinkNode_S *tmpLinkNode = NULL;
    SDF_DListElmForeach(tmpLinkNode, &g_SsapLinkList, entry) {
        SLE_Addr_S *tmpAddr = &(tmpLinkNode->link->addr);
        if (memcmp(addr->addr, tmpAddr->addr, SLE_ADDR_LEN) == 0) {
            linkNode = tmpLinkNode;
            break;
        }
    }
    CP_CHECK_LOGD_RETURN_VOID(linkNode != NULL, "[SSAP] not find link");
    SDF_DListElmDel(&g_SsapLinkList, linkNode, entry);
    SSAP_Link_S *link = linkNode->link;
    CP_LOG_INFO("[SSAP] ssap delete link, addr = %s, lcid = %d", GET_ENC_ADDR(&link->addr), link->lcid);
    SSAP_DelTimer(link);
    SSAP_ParamNode_S *taskParamNode = NULL;
    SSAP_ParamNode_S *tmpTaskParamNode = NULL;
    SDF_DListElmSafeForeach(taskParamNode, tmpTaskParamNode, &link->paramList, entry) {
        SSAP_FreeTaskParams(taskParamNode->param);
        SDF_MemFree(taskParamNode);
    }
    if (link->curTask.param != NULL) {
        SSAP_FreeTaskParams(link->curTask.param);
    }
    SDF_BuffFree(link->curTask.buff);
    SDF_MemFree(link);
    SDF_MemFree(linkNode);
}

SSAP_Link_S *SSAP_FindSsapLinkByAddr(SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN(!SDF_DListIsEmpty(&g_SsapLinkList), NULL, "[SSAP] ssap link list is empty");
    SSAP_Link_S *link = NULL;
    SSAP_LinkNode_S *linkNode = NULL;
    SDF_DListElmForeach(linkNode, &g_SsapLinkList, entry) {
        SLE_Addr_S *tmpAddr = &(linkNode->link->addr);
        if (memcmp(addr->addr, tmpAddr->addr, SLE_ADDR_LEN) == 0) {
            link = linkNode->link;
            break;
        }
    }

    return link;
}

SSAP_Link_S *SSAP_FindSsapLinkByLcid(uint16_t lcid)
{
    CP_CHECK_LOG_RETURN(!SDF_DListIsEmpty(&g_SsapLinkList), NULL, "[SSAP] ssap link list is empty");
    SSAP_Link_S *link = NULL;
    SSAP_LinkNode_S *linkNode = NULL;
    SDF_DListElmForeach(linkNode, &g_SsapLinkList, entry) {
        if (linkNode->link->lcid == lcid) {
            link = linkNode->link;
            break;
        }
    }

    return link;
}

void SSAP_LinkAddCurrentTask(SSAP_Link_S *link, SSAP_TaskParam_S *param)
{
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] link is null");
    CP_CHECK_LOG_RETURN_VOID(param != NULL, "[SSAP] param node is null, when add current task to link");
    // 做异常处理，先将原来的任务删除掉；
    if (link->curTask.param != NULL) {
        CP_LOG_ERROR("[SSAP] link add current task has old param, appid = %d", link->curTask.param->appId);
        SSAP_FreeTaskParams(link->curTask.param);
        link->curTask.param = NULL;
        SDF_BuffFree(link->curTask.buff);
        link->curTask.buff = NULL;
        SSAP_DelTimer(link);
    }
    link->curTask.param = param;
}

void SSAP_LinkSetTask(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    CP_CHECK_LOG_RETURN_VOID(buff != NULL, "[SSAP] buff is null");
    link->curTask.opcode = opcode;
    link->curTask.buff = buff;
    link->status = SSAP_LINK_BUSY;
    CP_LOG_DEBUG("[SSAP] ssap set task, lcid = %d, opcode = 0x%x", link->lcid, opcode);
}

SDF_Buff_S *SSAP_GetLastBuff(SSAP_Link_S *link)
{
    CP_CHECK_LOG_RETURN(link != NULL, NULL, "[SSAP] link is null");
    return link->curTask.buff;
}

void SSAP_LinkClearCurrentTask(SSAP_Link_S *link)
{
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] link is null");
    CP_LOG_DEBUG("[SSAP] ssap task complete, lcid = %d", link->lcid);
    SDF_BuffFree(link->curTask.buff);
    link->curTask.buff = NULL;
    SSAP_FreeTaskParams(link->curTask.param);
    link->curTask.param = NULL;
    SSAP_DelTimer(link);

    link->status = SSAP_LINK_IDLE;
}

void SSAP_AddTaskParamToLink(SSAP_Link_S *link, SSAP_TaskParam_S *param)
{
    SSAP_ParamNode_S *paramNode = (SSAP_ParamNode_S *)SDF_MemZalloc(sizeof(SSAP_ParamNode_S));
    if (paramNode == NULL) {
        CP_LOG_ERROR("[SSAP] task node mem alloc failed");
        SSAP_FreeTaskParams(param);
        return;
    }
    paramNode->param = param;
    SDF_DListEntryInit(&paramNode->entry);
    SDF_DListElmTailInsert(&link->paramList, paramNode, entry);
}

void SSAP_AddHighPriorityTaskParamToLink(SSAP_Link_S *link, SSAP_TaskParam_S *param)
{
    SSAP_ParamNode_S *paramNode = (SSAP_ParamNode_S *)SDF_MemZalloc(sizeof(SSAP_ParamNode_S));
    if (paramNode == NULL) {
        CP_LOG_ERROR("[SSAP] task node mem alloc failed");
        SSAP_FreeTaskParams(param);
        return;
    }
    paramNode->param = param;
    SDF_DListEntryInit(&paramNode->entry);
    SDF_DListElmHeadInsert(&link->paramList, paramNode, entry);
}

SSAP_TaskParam_S *SSAP_LinkGetFirstTaskParam(SSAP_Link_S *link)
{
    CP_CHECK_LOG_RETURN(link != NULL, NULL, "[SSAP] link is null");
    CP_CHECK_LOG_RETURN(!SDF_DListIsEmpty(&link->paramList), NULL, "[SSAP] ssap link task list is empty");
    SSAP_ParamNode_S *paramNode = (SSAP_ParamNode_S *)SDF_DListFirst(&link->paramList);
    SDF_DListElmHeadDel(&link->paramList, paramNode, entry);
    SSAP_TaskParam_S *param = paramNode->param;
    SDF_MemFree(paramNode);
    return param;
}

void SSAP_ExcuteProcessTask(SSAP_Link_S *link)
{
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] link is null");
    CP_CHECK_LOGD_RETURN_VOID(link->status == SSAP_LINK_IDLE, "[SSAP] ssap link is busy, lcid = %d", link->lcid);
    while (link->status == SSAP_LINK_IDLE && !SDF_DListIsEmpty(&link->paramList)) {
        SSAP_TaskParam_S *param = SSAP_LinkGetFirstTaskParam(link);
        CP_CHECK_LOG_RETURN_VOID(param != NULL, "[SSAP] link has no next param");
        SSAP_LinkAddCurrentTask(link, param);
        param->func(link, param->arg);
    }
}

uint8_t SSAP_CheckOpcode(SSAP_Link_S *link, uint8_t opcode)
{
    CP_CHECK_LOG_RETURN(opcode >= SSAP_ERROR_RSP && opcode < SSAP_CODE_MAX, SSAP_ERRCODE_UNSUPPORT_PDU,
        "[SSAP] invalid opcode");
    if (opcode == SSAP_ERROR_RSP) {
        if (link->status == SSAP_LINK_IDLE) {
            return SSAP_ERRCODE_INVALID_PDU;
        } else {
            return SSAP_ERRCODE_SUCCESS;
        }
    }
    uint8_t lastOpcode = g_SsapRecvOpcodeMap[opcode];
    if (lastOpcode == 0) {
        return SSAP_ERRCODE_SUCCESS;
    }
    // 空闲状态不应当处理rsp报文，因为没有发送req报文
    if (link->status == SSAP_LINK_IDLE) {
        return SSAP_ERRCODE_INVALID_PDU;
    }
    SSAP_Task_S task = link->curTask;
    if (task.opcode != lastOpcode) {
        return SSAP_ERRCODE_INVALID_PDU;
    }
    return SSAP_ERRCODE_SUCCESS;
}

SDF_DListHead_S *SSAP_GetSsapLinkList(void)
{
    return &g_SsapLinkList;
}

void SSAP_CleanLinkTaskByAppId(SSAP_Link_S *link, int32_t appId)
{
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] link is null");
    uint32_t cleanCount = 0;
    SSAP_ParamNode_S *taskParamNode = NULL;
    SSAP_ParamNode_S *tmpTaskParamNode = NULL;
    SDF_DListElmSafeForeach(taskParamNode, tmpTaskParamNode, &link->paramList, entry) {
        if (taskParamNode->param->appId == appId) {
            cleanCount++;
            CP_LOG_INFO("[SSAP] ssap clean task, addr = %s, lcid = %d, appid = %d",
                GET_ENC_ADDR(&link->addr), link->lcid, appId);
            SDF_DListElmDel(&link->paramList, taskParamNode, entry);
            SSAP_FreeTaskParams(taskParamNode->param);
            SDF_MemFree(taskParamNode);
        }
    }
    CP_LOG_INFO("[SSAP] ssap clean task, addr = %s, lcid = %d, appid = %d, count = %d", GET_ENC_ADDR(&link->addr),
        link->lcid, appId, cleanCount);
    if (link->curTask.param != NULL && link->curTask.param->appId == appId) {
        CP_LOG_INFO("[SSAP] ssap clean task set current task invalid");
        link->curTask.param->valid = false;
    }
}


void SSAP_CleanTaskByAppId(SLE_Addr_S *addr, int32_t appId)
{
    CP_CHECK_LOG_RETURN_VOID(addr != NULL, "[SSAP] addr is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(addr);
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] not find link");

    SSAP_CleanLinkTaskByAppId(link, appId);
}


bool SSAP_StartTimer(SSAP_Link_S *link, SDF_TimerCallback callback)
{
    CP_LOG_DEBUG("[SSAP] SSAP_StartTimer enter");
    CP_CHECK_LOG_RETURN(link->timerHandle == SSAP_TIMER_NO_USED_HANDLE, false, "[SSAP] timerHandle is in use.");
    SDF_TimerParam param = {
        .expires = SsapTaskGetTimeout(link),
        .period = false,
        .callback = callback,
        .args = link,
    };
    return CP_TimerAdd(&link->timerHandle, &param) == CP_OK;
}
 
void SSAP_DelTimer(SSAP_Link_S *link)
{
    CP_LOG_DEBUG("[SSAP] SSAP_DelTimer enter");
    if (link->timerHandle != SSAP_TIMER_NO_USED_HANDLE) {
        CP_TimerDel(link->timerHandle);
    }
    link->timerHandle = SSAP_TIMER_NO_USED_HANDLE;
}

void SsapTaskExecuteCallback(SSAP_Link_S *link, void *arg)
{
    CP_CHECK_LOG_RETURN_VOID(link && link->curTask.param && link->curTask.param->appCallback,
        "[SSAP] task app callback invalid");
    link->curTask.param->appCallback(link->curTask.param->appId, arg);
}

int32_t SsapTaskGetAppId(SSAP_Link_S *link)
{
    CP_CHECK_LOG_RETURN(link && link->curTask.param, SSAP_APP_INVALID_ID,
        "[SSAP] task app param invalid");
    return link->curTask.param->appId;
}

int64_t SsapTaskGetTimeout(SSAP_Link_S *link)
{
    CP_CHECK_LOG_RETURN(link && link->curTask.param, SSAP_INTERACTION_DEFAULT_TIMEOUT,
        "[SSAP] task app param invalid");
    return link->curTask.param->timeout;
}

bool SsapTaskValid(SSAP_Link_S *link)
{
    CP_CHECK_LOG_RETURN(link && link->curTask.param, false, "[SSAP] task app param invalid");
    return link->curTask.param->valid;
}