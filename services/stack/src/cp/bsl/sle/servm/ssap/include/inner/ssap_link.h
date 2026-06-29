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
#ifndef SSAP_LINK_H
#define SSAP_LINK_H

#include "sdf_addr.h"
#include "sdf_dlist.h"
#include "sdf_buff.h"
#include "sdf_timer.h"
#include "ssap_pkt.h"
#include "ssap_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SSAP_TIMER_NO_USED_HANDLE (-1)
#define SSAP_TIMEOUT_TIME 30000   // msec

typedef struct SSAP_Link SSAP_Link_S;

typedef void (*SendCb)(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode);

typedef struct SSAP_LinkNode {
    SDF_DListEntry_S entry;
    SSAP_Link_S *link;
} SSAP_LinkNode_S;

typedef void (*SSAP_ProcessTaskFunc)(SSAP_Link_S *link, void *arg);

typedef void (*SSAP_TaskArgFreeFunc)(void *arg);

typedef void (*SsapTaskAppCallback)(int32_t appId, void *arg);

typedef struct SSAP_TaskParam {
    int32_t appId;
    void *arg;
    SSAP_TaskArgFreeFunc freeFunc;
    SSAP_ProcessTaskFunc func;
    int64_t timeout;
    bool valid;
    SsapTaskAppCallback appCallback;
} SSAP_TaskParam_S;

typedef struct SSAP_ParamNode {
    SDF_DListEntry_S entry;
    SSAP_TaskParam_S *param;
} SSAP_ParamNode_S;

/**
 * @brief  SSAP任务
 */
typedef struct SSAP_Task {
    uint8_t opcode;                     // 当前执行任务的opcode
    SDF_Buff_S *buff;                   // 当前执行任务的报文
    SSAP_TaskParam_S *param;            // 当前执行任务的参数
} SSAP_Task_S;

typedef enum SSAP_LinkStatus {
    SSAP_LINK_IDLE,
    SSAP_LINK_BUSY,
} SSAP_LinkStatus_E;

/**
 * @brief  SSAP链路实体，控制报文处理
 */
struct SSAP_Link {
    SDF_DListHead_S paramList;          // 有回复信令缓存列表，节点SSAP_ParamNode_S
    SSAP_Task_S curTask;                // 当前执行的任务
    uint8_t status;                     // 任务状态，对应SSAP_LinkStatus_E
    bool hasInitReqTask;                // 是否是初始任务，初始的find任务不需要处理rsp
    SLE_Addr_S addr;                    // 对端地址
    SendCb sendFunc;                    // 发包钩子
    uint16_t lcid;                      // 链路id
    uint16_t mtu;                       // ssap mtu
    uint16_t version;                   // 星闪version
    int timerHandle;
};

uint32_t SSAP_LinkInit(void);

void SSAP_LinkDeInit(void);

SSAP_Link_S *SSAP_CreateSsapLinkWithInitReq(SLE_Addr_S *addr, uint16_t lcid, SendCb sendFunc, bool hasInitReqTask);

SSAP_Link_S *SSAP_CreateSsapLink(SLE_Addr_S *addr, uint16_t lcid, SendCb sendFunc);

void SSAP_DeleteSsapLinkByAddr(SLE_Addr_S *addr);

void SSAP_CleanTaskByAppId(SLE_Addr_S *addr, int32_t appId);

SSAP_Link_S *SSAP_FindSsapLinkByAddr(SLE_Addr_S *addr);

SSAP_Link_S *SSAP_FindSsapLinkByLcid(uint16_t lcid);

void SSAP_LinkSetTask(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode);

SSAP_TaskParam_S *SSAP_AllocTaskParam(SSAP_TaskParam_S *taskParam);

SDF_Buff_S *SSAP_GetLastBuff(SSAP_Link_S *link);

void SSAP_AddTaskParamToLink(SSAP_Link_S *link, SSAP_TaskParam_S *param);

void SSAP_AddHighPriorityTaskParamToLink(SSAP_Link_S *link, SSAP_TaskParam_S *param);

SSAP_TaskParam_S *SSAP_LinkGetFirstTaskParam(SSAP_Link_S *link);

void SSAP_ExcuteProcessTask(SSAP_Link_S *link);

void SSAP_LinkClearCurrentTask(SSAP_Link_S *link);

uint8_t SSAP_CheckOpcode(SSAP_Link_S *link, uint8_t opcode);

SDF_DListHead_S *SSAP_GetSsapLinkList(void);

bool SSAP_StartTimer(SSAP_Link_S *link, SDF_TimerCallback callback);

void SSAP_DelTimer(SSAP_Link_S *link);

void SsapTaskExecuteCallback(SSAP_Link_S *link, void *arg);

int32_t SsapTaskGetAppId(SSAP_Link_S *link);

int64_t SsapTaskGetTimeout(SSAP_Link_S *link);

bool SsapTaskValid(SSAP_Link_S *link);

#ifdef __cplusplus
}
#endif
#endif