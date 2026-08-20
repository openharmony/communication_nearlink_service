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

/****************************************************************************
 *
 * this file defines dtap priority queue scheduler APIs.
 *
 ***************************************************************************/

#ifndef DTAP_SCHEDULER_H
#define DTAP_SCHEDULER_H

#include "dtap_channel.h"
#include "sdf_buff.h"

#ifdef __cplusplus
extern "C" {
#endif

// LCID buffer 节点：记录该链路的发送配额与排队/速率统计，供内部与测试用例只读查询使用。
typedef struct {
    SDF_DListEntry_S entry;
    uint16_t lcid;
    uint8_t quota;                // 该LCID的发送配额
    uint8_t lastQuota;            // 上次打印时的quota，用于判断是否变化，有变化才打印
    uint32_t sendNotAckPktCnt;    // 该LCID的发送但未确认的包数
    uint32_t queuedPktCnt;        // 已入队待调度包数，用于评估链路排队数据量
    uint64_t windowBytes;         // 当前速率计算窗口内已发送字节数
    uint64_t lastRateCalcTime;    // 上次速率计算时间（ms）
} DTAP_LcidBufferNode;

uint32_t DTAP_SchedulerInit(void);
uint32_t DTAP_SchedulerDeinit(void);
void DTAP_ChannelDown(uint16_t lcid, uint8_t srcTcid);
uint32_t DTAP_DataSendWithPriority(DTAP_Channel_S *transChan, SDF_Buff_S *buff);
DTAP_LcidBufferNode *DTAP_GetLcidBufferNode(uint16_t lcid);

#ifdef __cplusplus
}
#endif

#endif // DTAP_SCHEDULER_H