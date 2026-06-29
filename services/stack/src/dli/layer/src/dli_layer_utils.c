/*
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

#include "dli_layer_utils.h"

#include <stdint.h>
#include "securec.h"

#include "dli_errno.h"
#include "dli_log.h"
#include "sdf_mem.h"
#include "sdf_dlist.h"
#include "sdf_mutex.h"
#include "sdf_timer.h"
#include "dli_layer_stru.h"

#ifdef __cplusplus
extern "C" {
#endif

DLI_CmdStru *DLI_DefaultCmdStruCreate(uint16_t cmd, uint16_t event,
    uint8_t *par, uint16_t parLen)
{
    DLI_CHECK_RETURN_NULL(parLen <= DLI_MAX_LEN(sizeof(DLI_CmdStru)),
        "DLI_DefaultCmdStruCreate parLen is error");
    uint32_t len = (uint32_t)(sizeof(DLI_CmdStru) + DLI_HEADER) + parLen;
    DLI_CmdStru *cmdInfo = (DLI_CmdStru*)SDF_MemZalloc(len);
    DLI_CHECK_RETURN_NULL(cmdInfo != NULL, "DLI_DefaultCmdStruCreate SDF_MemZalloc failed");

    cmdInfo->cmd = cmd;
    cmdInfo->event = event;
    if (parLen != 0) {
        (void)memcpy_s(&(cmdInfo->par[DLI_HEADER]), parLen, par, parLen);
        cmdInfo->parLen = parLen;
    }
    return cmdInfo;
}

DLI_DataStru *DLI_DefaultDataStruCreate(uint16_t lcid, uint16_t type,
    uint8_t ts, uint8_t prio, SDF_Buff_S *buf)
{
    DLI_DataStru *dataInfo = (DLI_DataStru*)SDF_MemZalloc(sizeof(DLI_DataStru));
    DLI_CHECK_RETURN_NULL(dataInfo != NULL, "DLI_DefaultDataStruCreate SDF_MemZalloc failed");

    dataInfo->buf = buf;
    dataInfo->lcid = lcid;
    dataInfo->prio = prio;
    dataInfo->ts = ts;
    dataInfo->type = type;
    return dataInfo;
}

DLI_RecvDataNode *DLI_RecvDataNodeCreate(uint16_t lcid, uint16_t pb, SDF_Buff_S *buf)
{
    DLI_RecvDataNode *dataNode = (DLI_RecvDataNode *)SDF_MemZalloc(sizeof(DLI_RecvDataNode));
    DLI_CHECK_RETURN_NULL(dataNode != NULL, "DLI_RecvDataNodeCreate SDF_MemZalloc failed");

    dataNode->buf = buf;
    dataNode->lcid = lcid;
    dataNode->pb = pb;
    SDF_DListEntryInit(&dataNode->node);
    return dataNode;
}

DLI_CmdTxNode *DLI_CmdNodeCreate(DLI_CmdStru *cmd)
{
    DLI_CHECK_RETURN_NULL(cmd, "param cmd is null");
    DLI_CmdTxNode *cmdNode = (DLI_CmdTxNode*)SDF_MemZalloc(sizeof(DLI_CmdTxNode));
    DLI_CHECK_RETURN_NULL(cmdNode != NULL, "DLI_CmdNodeCreate SDF_MemZalloc failed");

    cmdNode->handle = -1;
    cmdNode->info = cmd;
    cmdNode->isSent = false;
    cmdNode->isRecvStatusEvt = false;
    SDF_DListEntryInit(&cmdNode->node);
    return cmdNode;
}

DLI_RecvDataNode *DLI_DataNodeFind(SDF_DListHead_S *head, uint16_t lcid)
{
    DLI_RecvDataNode *dataNode = NULL;
    DLI_RecvDataNode *tmp = NULL;

    SDF_DListElmSafeForeach(dataNode, tmp, head, node) {
        if (dataNode->lcid == lcid) {
            return dataNode;
        }
    }
    return NULL;
}

DLI_CmdTxNode *DLI_CmdNodeFind(SDF_DListHead_S *head, uint16_t event, uint16_t opcode)
{
    DLI_CmdTxNode *cmdNode = NULL;
    DLI_CmdTxNode *tmp = NULL;

    SDF_DListElmSafeForeach(cmdNode, tmp, head, node)
    {
        // 状态事件和完成事件的opcode=cmd=cmdNode->info->cmd，其他事件的opcode=0
        if ((opcode == 0 || cmdNode->info->cmd == opcode) && cmdNode->info->event == event) {
            return cmdNode;
        }
    }
    return NULL;
}

DLI_CmdTxNode *DLI_CmdNodeFindByOpcode(SDF_DListHead_S *head, uint16_t opcode)
{
    DLI_CmdTxNode *cmdNode = NULL;
    DLI_CmdTxNode *tmp = NULL;

    SDF_DListElmSafeForeach(cmdNode, tmp, head, node)
    {
        // 状态事件和完成事件的opcode=cmd=cmdNode->info->cmd，其他事件的opcode=0
        if (opcode == 0 || cmdNode->info->cmd == opcode) {
            return cmdNode;
        }
    }
    return NULL;
}

DLI_CmdTxNode *DLI_CmdNodeFindNotRecvStatus(SDF_DListHead_S *head, uint16_t opcode)
{
    DLI_CmdTxNode *cmdNode = NULL;
    DLI_CmdTxNode *tmp = NULL;

    SDF_DListElmSafeForeach(cmdNode, tmp, head, node)
    {
        // 节点插入时是尾插法，遍历时从头开始，所以匹配到的是第一个节点
        if ((opcode == 0 || cmdNode->info->cmd == opcode) && !cmdNode->isRecvStatusEvt) {
            return cmdNode;
        }
    }
    return NULL;
}

static int DLI_ReciveBufUpdate(SDF_Buff_S *buf, SDF_Buff_S *newBuf)
{
    uint64_t remainLen = SDF_BuffTailRoom(buf);
    uint8_t *p = SDF_BuffAppend(buf, SDF_DataLenGet(newBuf));
    if (p == NULL) {
        return 1;
    }

    if (memcpy_s(p, remainLen, SDF_DataOffset(newBuf), SDF_DataLenGet(newBuf)) != EOK) {
        DLI_LOGE("memcpy_s p failed");
        return -1;
    }
    return 0;
}

bool DLI_ReciveDataUpdate(DLI_RecvDataNode *node, uint16_t lcid,
    uint16_t pb, SDF_Buff_S *buf)
{
    DLI_CHECK_RETURN_RET(node != NULL && node->buf != NULL, false, "param node or node buf is null");
    int ret = DLI_ReciveBufUpdate(node->buf, buf);
    if (ret != 1) {
        return (ret == 0);
    }

    // 如果空间不够且pb为第中间分片，(2倍的buff)预留最后一次分片的空间,减少一次new
    uint64_t size = pb == DLI_MIDDLE_FRAGMENT ? (2 * SDF_DataLenGet(buf)) : SDF_DataLenGet(buf);
    size += SDF_DataLenGet(node->buf);
    DLI_CHECK_RETURN_RET(size <= DLI_MAX_TXRX_DATA_LEN, false, "size is too large");
    SDF_Buff_S *newBuf = SDF_BuffNew((uint32_t)size);
    DLI_CHECK_RETURN_RET(newBuf, false, "SDF_BuffNew buf failed");

    ret = DLI_ReciveBufUpdate(newBuf, node->buf);
    if (ret != 0) {
        SDF_BuffFree(newBuf);
        DLI_LOGE("DLI_ReciveBufUpdate buf failed");
        return false;
    }

    ret = DLI_ReciveBufUpdate(newBuf, buf);
    if (ret != 0) {
        SDF_BuffFree(newBuf);
        DLI_LOGE("DLI_ReciveBufUpdate data failed");
        return false;
    }

    node->lcid = lcid;
    node->pb = pb;
    SDF_BuffFree(node->buf);
    node->buf = newBuf;
    return true;
}

void DLI_RxDataDlistDestroy(SDF_DListHead_S *head)
{
    DLI_RecvDataNode *dataNode = NULL;
    DLI_RecvDataNode *tmp = NULL;

    SDF_DListElmSafeForeach(dataNode, tmp, head, node) {
        SDF_DListElmDel(head, dataNode, node);
        DLI_RecvDataNodeDestroy(dataNode);
    }
}

void DLI_CmdNodeDestroy(void *txNode)
{
    DLI_CmdTxNode *node = (DLI_CmdTxNode *)txNode;
    if (node == NULL) {
        return;
    }

    if (node->info != NULL) {
        if (node->info->contextFree != NULL) {
            node->info->contextFree(node->info->context);
            node->info->context = NULL;
        }
        SDF_MemFree(node->info);
        node->info = NULL;
    }
    SDF_MemFree(node);
}

void DLI_DataStruDestroy(DLI_DataStru *data)
{
    if (data == NULL) {
        return;
    }
    SDF_BuffFree(data->buf);
    SDF_MemFree(data);
}

void DLI_RecvDataNodeDestroy(DLI_RecvDataNode *data)
{
    DLI_CHECK_RETURN(data, "data is null");
    SDF_BuffFree(data->buf);
    data->buf = NULL;
    SDF_MemFree(data);
}

void DLI_CmdDlistDestroy(SDF_DListHead_S *head)
{
    DLI_CmdTxNode *cmdNode = NULL;
    DLI_CmdTxNode *tmp = NULL;

    SDF_DListElmSafeForeach(cmdNode, tmp, head, node) {
        SDF_DListElmDel(head, cmdNode, node);
        if (cmdNode->handle != -1) {
            SDF_TimerDel(cmdNode->handle);
        }
        DLI_CmdNodeDestroy((void *)cmdNode);
    }
}
#ifdef __cplusplus
}
#endif