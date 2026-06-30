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
#include "dli_event.h"

#include <stdbool.h>
#include "securec.h"

#include "dli.h"
#include "sdf_mem.h"
#include "sdf_dlist.h"

#include "dli.h"
#include "dli_log.h"
#include "dli_opcode.h"
#include "dli_errno.h"
#include "dli_thread.h"
#include "dli_layer.h"
#include "dli_layer_config.h"
#include "dli_layer_stru.h"
#include "dli_layer_callback.h"

#define DLI_OPCODE_BYTE_LENGTH 2

#define DLI_CP_BLOCK_TIMEOUT 3000 // ms

typedef struct {
    uint16_t cmdOpcode;
    uint32_t regOpcode;
} EvtRegOpcodeMappingTable;

// 后续可以继续增加
static EvtRegOpcodeMappingTable g_evtRegOpcodeMappingTable[] = {
    {DLI_CREATE_CONNECTION, DLI_CBK_CONNECT},
    {DLI_CREATE_CONNECTION_CANCEL, DLI_CBK_CONNECT_CANCEL},
    {DLI_DISCONNECT, DLI_CBK_DISCONNECT},
    {DLI_CONNECTION_UPDATE, DLI_CBK_CONNECT_UPDATE},
    {DLI_SET_SCAN_ENABLE, DLI_CBK_ENABLE_SCAN},

    {DLI_SET_IOG_PARAM, DLI_CBK_SET_IOG_PARAM},
    {DLI_SET_IOG_PARAM_TEST, DLI_CBK_SET_IOG_PARAM},
    {DLI_CREATE_IOB, DLI_CBK_IOB_ESTABLISHED},
    {DLI_ACCEPT_IOB_REQ, DLI_CBK_IOB_ESTABLISHED},
    {DLI_REMOVE_IOG_PARAM, DLI_CBK_REMOVE_ICG_PARAM},
    {DLI_REJECT_IOB_REQ, DLI_CBK_REJECT_IOB_REQ},

    {DLI_SET_IMG_PARAM, DLI_CBK_SET_IMG_PARAM},
    {DLI_SET_IMG_PARAM_TEST, DLI_CBK_SET_IMG_PARAM},
    {DLI_CREATE_IMB, DLI_CBK_IMB_ESTABLISHED},
    {DLI_ACCEPT_IMB_REQ, DLI_CBK_IMB_ESTABLISHED},
    {DLI_REMOVE_IMG_PARAM, DLI_CBK_REMOVE_ICG_PARAM},
    {DLI_REJECT_IMB_REQ, DLI_CBK_REJECT_IMB_REQ},

    {DLI_SETUP_ICB_DATA_PATH, DLI_CBK_ICB_SETUP_DATA_PATH},
    {DLI_REMOVE_ICB_DATA_PATH, DLI_CBK_ICB_REMOVE_DATA_PATH},
};

#define DLI_EVT_REG_OPCODE_MAPPING_TABLE_NUM (sizeof(g_evtRegOpcodeMappingTable) / sizeof(EvtRegOpcodeMappingTable))

static SDF_DListHead_S g_eventCbkListHead = {
    .list = {&(g_eventCbkListHead).list, &(g_eventCbkListHead).list}, .size = 0};
DLI_ExecuteCmdCbk g_cbkList[DLI_CBK_MAX] = {0};

static DLI_NOCPEventCbk g_eventNOCPCbk[DLI_REG_MODULE_MAX] = {0};

static void DLI_RunCmdCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGD("dli inner cbk evtOpcode: 0x%04X, len: %u", evtOpcode, len);
    DLI_InnerCbkNodeStru *node = NULL;
    DLI_InnerCbkNodeStru *tmp = NULL;
    DLI_InnerCbkLineStru line;

    SDF_DListElmSafeForeach(node, tmp, &g_eventCbkListHead, entry)
    {
        for (uint32_t i = 0; i < node->size; i++) {
            line = node->table[i];
            if (line.opcode != evtOpcode) {
                continue;
            }
            if (line.func != NULL) {
                DLI_LOGD("find opcode:%04X in table", evtOpcode);
                // DLI内部执行各个事件处理函数（包括完成事件和状态事件），最终回调到各个模块
                // len报文长度 arg报文内容 context发送命令时保存的上下文
                line.func(context, arg, len, evtOpcode);
                return;
            }
        }
    }
}

static uint16_t DLI_GetEvtOpcode(uint16_t event, uint16_t opcode)
{
    uint16_t evtOpcode = 0;
    if (event == DLI_CMD_COMPLETE_EVT || event == DLI_INVALID_EVT) {
        evtOpcode = opcode;
    } else {
        evtOpcode = event;
    }
    return evtOpcode;
}

static void DLI_NotifySendComplete(DLI_NumberOfCompletedPacketsEvt *ev)
{
    for (uint32_t i = 0; i < DLI_REG_MODULE_MAX; i++) {
        if (g_eventNOCPCbk[i] != NULL) {
            g_eventNOCPCbk[i](ev->connHandle, ev->numCompletedPackets);
        }
    }
}

void RecvEventHandler(uint16_t event, void *context, const uint8_t *data, uint32_t len)
{
    if ((event == DLI_CMD_COMPLETE_EVT && context == NULL) || data == NULL || len == 0) {
        DLI_LOGE("recv event handler arg is invalid, event = 0x%04X", event);
        return;
    }
    // 完成事件和状态事件的opcode=cmd，其他事件的opcode=0
    uint16_t opcode = 0;
    uint16_t evtOpcode = 0;

    if (len >= DLI_OPCODE_BYTE_LENGTH) {
        opcode = DLI_DECODE2BYTE(data);
    }

    if (event == DLI_CMD_COMPLETE_EVT && len >= sizeof(DLI_CommandComplete)) {
        uint8_t cmdNum = ((DLI_CommandComplete *)data)->numDliCommandPackets;
        DLI_CmdNumSet(cmdNum);
        DLI_LOGD("recv event = 0x%04X, len = %u, cmd complete cbk update cmd num:%hhu success.", event, len, cmdNum);
    }

    // 完成事件的evtOpcode=opcode=cmd，状态事件和其他事件的evtOpcode=event
    evtOpcode = DLI_GetEvtOpcode(event, opcode);
    DLI_LOGI("recv event = 0x%04X, len = %u, opcode = 0x%04X, evtOpcode = 0x%04X", event, len, opcode, evtOpcode);

    // 回调，context和data由layer释放
    DLI_RunCmdCbk(context, (void *)data, len, evtOpcode);

    // 开始发送下一个命令
    DLI_PostNextTask(DLI_CMD_TASK);
}

void DLI_NumberOfCompletedPacketsCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    (void)context;
    DLI_LOGD("number of completed packets cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(arg, "number of completed packets cbk param is null");
    DLI_CHECK_RETURN(len >= sizeof(DLI_NumberOfCompletedPacketsEvt),
        "check len=%u, minDataLen=%zu",
        len,
        sizeof(DLI_NumberOfCompletedPacketsEvt));
    DLI_NumberOfCompletedPacketsEvt *ev = (DLI_NumberOfCompletedPacketsEvt *)arg;
    DLI_LOGD("get dli number of completed packets: %hhu", ev->numCompletedPackets);
    DLI_NotifySendComplete(ev);
}

void DLI_CommandStatusCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGD("command status cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(arg, "command status cbk param is null");
    DLI_CHECK_RETURN(len >= sizeof(DLI_CommandStatus), "check len=%u, minDataLen=%zu", len, sizeof(DLI_CommandStatus));
    DLI_CommandStatus *in = (DLI_CommandStatus *)arg;

    if (len >= sizeof(DLI_CommandStatus)) {
        DLI_CmdNumSet(in->numDliCommandPackets);
        DLI_LOGD("cmd status cbk update cmd num:%hhu success.", in->numDliCommandPackets);
    }

    // 发送失败，进入DLI_CommandErrorCbk进行处理，解析status字段，再回调给各个模块
    if (in->status != DLI_SUCCESS) {
        DLI_CommandErrorStru param = {0};
        param.status = in->status;
        param.cmd = in->cmdOpcode;
        param.event = evtOpcode;

        DLI_RunCmdCbk(context, (void *)&param, (uint32_t)(sizeof(DLI_CommandErrorStru)), DLI_CMD_ERROR_EVT);
    }
}

uint16_t DLI_GetopcodeByCmd(uint16_t cmd)
{
    for (uint32_t i = 0; i < DLI_EVT_REG_OPCODE_MAPPING_TABLE_NUM; i++) {
        if (g_evtRegOpcodeMappingTable[i].cmdOpcode != cmd) {
            continue;
        }
        return g_evtRegOpcodeMappingTable[i].regOpcode;
    }

    if (DLI_GetCallback()->getExtRegOpcode != NULL) {
        return DLI_GetCallback()->getExtRegOpcode(cmd);
    }
    return DLI_CBK_MAX;
}

void DLI_CommandErrorCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("command error cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(arg, "command error cbk param is null");
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_CommandErrorStru), "check len=%u, minDataLen=%zu", len, sizeof(DLI_CommandErrorStru));
    DLI_CommandErrorStru *param = (DLI_CommandErrorStru *)arg;
    DLI_LOGI("cmd: 0x%04X send error, status: 0x%04X", param->cmd, param->status);
    uint16_t regOpcode = DLI_GetopcodeByCmd(param->cmd);
    if (regOpcode == DLI_CBK_MAX) {
        DLI_LOGW("cmd: 0x%04X send error, regOpcode: 0x%04x is error", param->cmd, regOpcode);
        return;
    }

    DLI_RunRegCbk(regOpcode, context, arg, (uint32_t)(sizeof(DLI_CommandErrorStru)), evtOpcode, param->status);
}

void DLI_InitEventCbkList(void)
{
    SDF_DListHeadInit(&g_eventCbkListHead);
}

void DLI_DeInitEventCbkList(void)
{
    DLI_InnerCbkNodeStru *node = NULL;
    DLI_InnerCbkNodeStru *tmp = NULL;

    SDF_DListElmSafeForeach(node, tmp, &g_eventCbkListHead, entry)
    {
        SDF_DListElmDel(&g_eventCbkListHead, node, entry);
        SDF_MemFree((void *)node->table);
        node->table = NULL;
        SDF_MemFree(node);
        node = NULL;
    }
}

static void DLI_InnerEventCbkRegInner(void *param)
{
    DLI_InnerCbkNodeStru *node = (DLI_InnerCbkNodeStru *)param;
    SDF_DListElmTailInsert(&g_eventCbkListHead, node, entry);
}

uint32_t DLI_InnerEventCbkReg(const DLI_InnerCbkLineStru *table, const uint32_t size)
{
    if ((size == 0) || (table == NULL)) {
        return DLI_INVALID_PARAMETERS;
    }
    DLI_InnerCbkNodeStru *node = (DLI_InnerCbkNodeStru *)SDF_MemZalloc(sizeof(DLI_InnerCbkNodeStru));
    if (node == NULL) {
        return DLI_STACK_MEM_ERRNO;
    }
    size_t totalSize = size * sizeof(DLI_InnerCbkLineStru);
    node->table = (DLI_InnerCbkLineStru *)SDF_MemZalloc(totalSize);
    if (node->table == NULL) {
        SDF_MemFree(node);
        return DLI_STACK_MEM_ERRNO;
    }
    (void)memcpy_s((void *)node->table, totalSize, table, totalSize);
    node->size = size;
    uint32_t ret = DLI_PostOtherThread(DLI_InnerEventCbkRegInner, (void *)node, NULL);
    if (ret != 0) {
        SDF_MemFree((void *)node->table);
        SDF_MemFree(node);
        DLI_LOGE("DLI_PostOtherThread failed: %u", ret);
        return DLI_STACK_MEM_ERRNO;
    }
    return DLI_SUCCESS;
}

static void DLI_InnerEventCbkUnRegInner(void *param)
{
    const DLI_InnerCbkNodeStru *innerNode = (const DLI_InnerCbkNodeStru *)param;
    DLI_InnerCbkNodeStru *node = NULL;
    DLI_InnerCbkNodeStru *tmp = NULL;
    SDF_DListElmSafeForeach(node, tmp, &g_eventCbkListHead, entry)
    {
        if (node->size != innerNode->size) {
            continue;
        }
        if (memcmp(node->table, innerNode->table, node->size * sizeof(DLI_InnerCbkLineStru)) == 0) {
            DLI_LOGD("dli inner evt cbk unreg.");
            SDF_DListElmDel(&g_eventCbkListHead, node, entry);
            SDF_MemFree((void *)node->table);
            node->table = NULL;
            SDF_MemFree(node);
            node = NULL;
            break;
        }
    }
}

static void DLI_InnerCbkNodeStruFree(void *param)
{
    DLI_InnerCbkNodeStru *node = (DLI_InnerCbkNodeStru *)param;
    if (node == NULL) {
        return;
    }
 
    SDF_MemFree((void *)node->table);
    SDF_MemFree(node);
}

void DLI_InnerEventCbkUnReg(const DLI_InnerCbkLineStru *table, const uint32_t size)
{
    if (table == NULL || size == 0) {
        return;
    }
 
    DLI_InnerCbkNodeStru *node = (DLI_InnerCbkNodeStru *)SDF_MemZalloc(sizeof(DLI_InnerCbkNodeStru));
    if (node == NULL) {
        DLI_LOGE("malloc node failed");
        return;
    }
    size_t totalSize = size * sizeof(DLI_InnerCbkLineStru);
    node->table = (DLI_InnerCbkLineStru *)SDF_MemZalloc(totalSize);
    if (node->table == NULL) {
        SDF_MemFree(node);
        DLI_LOGE("malloc table failed");
        return;
    }
    (void)memcpy_s((void *)node->table, totalSize, table, totalSize);
    node->size = size;
 
    (void)DLI_PostOtherThread(DLI_InnerEventCbkUnRegInner, (void *)node, DLI_InnerCbkNodeStruFree);
}

void DLI_AddCbks(const DLI_CbkLineStru *table, const uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        uint16_t opcode = table[i].opcode;
        if (opcode < DLI_CBK_SET_ADV_PARAMS || opcode >= DLI_CBK_MAX) {
            DLI_LOGE("opcode is invalid, opcode=0x%04X", opcode);
            continue;
        }
        DLI_ExecuteCmdCbk func = table[i].func;
        if (func == NULL) {
            DLI_LOGE("add cbk func is null, opcode=0x%04X", opcode);
            continue;
        }
        g_cbkList[opcode] = func;
    }
}

void DLI_RemoveCbks(const DLI_CbkLineStru *table, const uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        uint16_t opcode = table[i].opcode;
        if (opcode < DLI_CBK_SET_ADV_PARAMS || opcode >= DLI_CBK_MAX) {
            DLI_LOGE("opcode is invalid, opcode=0x%04X", opcode);
            continue;
        }
        g_cbkList[opcode] = NULL;
    }
}

DLI_ExecuteCmdCbk DLI_GetCbk(const uint16_t opcode)
{
    if (opcode < DLI_CBK_SET_ADV_PARAMS || opcode >= DLI_CBK_MAX) {
        DLI_LOGE("cbk func is null, opcode=0x%04X", opcode);
        return NULL;
    }
    return g_cbkList[opcode];
}

void DLI_RunRegCbk(uint16_t regOpcode, void *context, void *arg, uint32_t size, uint16_t evtOpcode, uint16_t status)
{
    // 根据各个模块注册的regOpcode，回调到各个模块注册的处理函数
    DLI_ExecuteCmdRetParam cmdRes = {0};
    DLI_ExecuteCmdCbk cbk = NULL;
    void *cbkContext = NULL;

    if (context != NULL) {
        DLI_ManagerContext *managerContext = (DLI_ManagerContext *)context;
        cbkContext = managerContext->cbkContext;
    }

    DLI_LOGD("RunRegCbk regOpcode = 0x%04X", regOpcode);

    cbk = DLI_GetCbk(regOpcode);
    if (cbk != NULL) {
        cmdRes.cmdOpcode = evtOpcode;
        cmdRes.eventParameter = arg;
        cmdRes.size = size;
        cbk(cbkContext, status, &cmdRes);
    }
}

uint32_t DLI_RegNOCPEventCbk(DLI_RegModuleType module, DLI_NOCPEventCbk cbk)
{
    if (module >= DLI_REG_MODULE_MAX || module < 0 || cbk == NULL) {
        return DLI_INVALID_PARAMETERS;
    }
    g_eventNOCPCbk[module] = cbk;
    return DLI_SUCCESS;
}

uint32_t DLI_UnregNOCPEventCbk(DLI_RegModuleType module)
{
    if (module >= DLI_REG_MODULE_MAX || module < 0) {
        return DLI_INVALID_PARAMETERS;
    }
    g_eventNOCPCbk[module] = NULL;
    return DLI_SUCCESS;
}
