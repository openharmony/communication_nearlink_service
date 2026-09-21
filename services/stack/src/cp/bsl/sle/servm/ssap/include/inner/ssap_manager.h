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
#ifndef SSAP_MANAGER_H
#define SSAP_MANAGER_H

#include "dtap.h"
#include "ssap_type.h"
#include "ssap_pkt.h"
#include "ssap_link.h"

#ifdef __cplusplus
extern "C" {
#endif

void SSAP_ProcessRequestTask(SSAP_Link_S *link, SSAP_TaskParam_S *param, bool delay);

void SSAP_ProcessHighPriorityRequestTask(SSAP_Link_S *link, SSAP_TaskParam_S *param, bool delay);

void SSAP_ProcessNormalTask(SSAP_Link_S *link, SSAP_ProcessTaskFunc func, void *arg, SSAP_TaskArgFreeFunc freeFunc);

int SSAP_Recv(DTAP_Data_Info_S *info, SDF_Buff_S *buff);

void SSAP_TimeoutCbk(void *arg);

uint32_t SSAP_Init(void);

void SSAP_DeInit(void);

/**
 * @brief  错误处理报文响应
 */
void SSAP_PduErrorRsp(SSAP_Link_S *link, uint8_t op, uint16_t errorHandle, uint8_t errorCode);

/**
 * @brief  分包组包状态
 */
typedef enum SSAP_ReassemStatus {
    SSAP_REASSEM_WAIT = 0,      // 分片已缓存，等待后续分片
    SSAP_REASSEM_COMPLETE,      // 组包完成
    SSAP_REASSEM_DROP,          // 分片异常，已丢弃
    SSAP_REASSEM_CANCEL,        // 写入类分片被取消，缓存已清空
} SSAP_ReassemStatus_E;

/**
 * @brief  判断报文是否为SSAP分片报文（各信令处理函数入口调用）
 */
bool SSAP_IsFragPkt(SDF_Buff_S *buff);

/**
 * @brief  各信令处理函数的分片组包工具，组包完成后通过complete输出完整报文
 */
SSAP_ReassemStatus_E SSAP_ReassemFragment(SSAP_Link_S *link, SDF_Buff_S *buff, SDF_Buff_S **complete);

/**
 * @brief  链路发送回调：报文超MTU且对端支持分包时分包发送，否则单包发送（所有信令统一发送入口）
 */
void SSAP_Send(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode);

/**
 * @brief  分片接收组包完成后的完整报文处理回调（各信令handler自身，完整报文走非分片分支）
 */
typedef void (*SSAP_FragProcFunc)(SSAP_Link_S *link, SDF_Buff_S *complete);

/**
 * @brief  取消写入回调（WRITE类分片被取消时调用，可为NULL）
 */
typedef void (*SSAP_FragCancelFunc)(SSAP_Link_S *link);

/**
 * @brief  分片接收统一处理入口：分片报文组包完成后调用proc处理完整报文
 *         响应/确认类报文组包完成后自动清理链路任务并继续执行排队任务
 * @param [in] proc       组包完成后的完整报文处理回调
 * @param [in] cancelProc 取消写入回调（WRITE类分片被取消时调用，可为NULL）
 * @return true=报文已按分片处理（含WAIT/DROP/CANCEL）；false=非分片报文，调用方按单包处理
 */
bool SSAP_HandleFragRecv(SSAP_Link_S *link, SDF_Buff_S *sdfBuff, SSAP_FragProcFunc proc,
    SSAP_FragCancelFunc cancelProc);

/**
 * @brief  分包发送实现（由SSAP_Send统一调用，亦可单独测试）：报文超MTU时按BEGIN/MID/END分片发送
 */
bool SSAP_SendFragPkt(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode);

/**
 * @brief  分片响应/确认报文组包完成后的任务清理，由各响应处理函数在组包完成后调用
 */
void SSAP_HandleRspTaskComplete(SSAP_Link_S *link);

#ifdef __cplusplus
}
#endif
#endif