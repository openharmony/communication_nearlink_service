/**
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
 * @file         dli_layer_utils.h
 * @brief        dli一些节点处理
*/

#ifndef DLI_LAYER_UTILS_H
#define DLI_LAYER_UTILS_H

#include <stdint.h>
#include <stdbool.h>

#include "dli_layer_stru.h"
#include "sdf_evc.h"
#include "sdf_dlist.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DLI_MAX_LEN(sz) (UINT16_MAX - DLI_HEADER - (sz))

typedef struct DLI_CmdTxNode {
    SDF_DListEntry_S node;
    bool isSent;
    bool isRecvStatusEvt;
    int handle;
    DLI_CmdStru *info;
} DLI_CmdTxNode;

typedef struct DLI_RecvDataNode {
    SDF_DListEntry_S node;
    uint16_t lcid;
    uint8_t pb;
    SDF_Buff_S *buf;
} DLI_RecvDataNode;

// 默认context，timeoutCallback及contextFree都为空, opcode 为0
DLI_CmdStru *DLI_DefaultCmdStruCreate(uint16_t cmd, uint16_t event,
    uint8_t *par, uint16_t parLen);

DLI_DataStru *DLI_DefaultDataStruCreate(uint16_t lcid, uint16_t type,
    uint8_t ts, uint8_t prio, SDF_Buff_S *buf);

DLI_RecvDataNode *DLI_RecvDataNodeCreate(uint16_t lcid, uint16_t pb, SDF_Buff_S *buf);

DLI_CmdTxNode *DLI_CmdNodeCreate(DLI_CmdStru *cmd);

bool DLI_ReciveDataUpdate(DLI_RecvDataNode *node, uint16_t lcid,
    uint16_t pb, SDF_Buff_S *buf);

DLI_RecvDataNode *DLI_DataNodeFind(SDF_DListHead_S *head, uint16_t lcid);
DLI_CmdTxNode *DLI_CmdNodeFind(SDF_DListHead_S *head, uint16_t event, uint16_t opcode);
DLI_CmdTxNode *DLI_CmdNodeFindByOpcode(SDF_DListHead_S *head, uint16_t opcode);
DLI_CmdTxNode *DLI_CmdNodeFindNotRecvStatus(SDF_DListHead_S *head, uint16_t opcode);
void DLI_CmdNodeDestroy(void *txNode);
void DLI_DataStruDestroy(DLI_DataStru *data);
void DLI_RecvDataNodeDestroy(DLI_RecvDataNode *data);

void DLI_RxDataDlistDestroy(SDF_DListHead_S *head);
void DLI_CmdDlistDestroy(SDF_DListHead_S *head);
#ifdef __cplusplus
}
#endif
#endif // DLI_LAYER_UTILS_H