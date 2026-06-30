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
*/

/****************************************************************************
 *
 * this file defines transmission mode and related apis.
 *
 ***************************************************************************/

#ifndef DTAP_TRANS_H
#define DTAP_TRANS_H

#include <stdbool.h>
#include <stdint.h>

#include "cm_trans_channel_api.h"
#include "dtap.h"
#include "dtap_channel.h"
#include "dtap_frame.h"
#include "sdf_buff.h"
#include "sdf_dlist.h"
#include "sdf_evc.h"
#include "sdf_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DTAP_TransMode {
    uint8_t (*getModeType)(void);                                            // 获取传输模式类型
    bool (*checkFrame)(const DTAP_Channel_S *transChan, DTAP_Frame_S *dtapFrame);   // 检查帧
    uint32_t (*recvFrame)(DTAP_Channel_S *transChan, DTAP_Frame_S *dtapFrame,
                          int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *buff));     // 处理接收帧
    uint32_t (*sendFrame)(DTAP_Channel_S *transChan, uint8_t pi, SDF_Buff_S *buff);  // 发送帧
    void (*transFrame)(DTAP_Channel_S *transChan, SDF_Buff_S *buff,
                       int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *));  // 透传帧
    void (*setTransChannelStatus)(DTAP_Channel_S *transChan, uint16_t result,
        int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *));  // 重新处理接收帧
} DTAP_TransMode_S;

void DTAP_TransChannelStatusChange(DTAP_Channel_S *channel, uint8_t result);
void DTAP_RegisterBasicTransMode(void);
void DTAP_RegisterTransparentMode(void);
void DTAP_RegisterStreamMode(void);
void DTAP_RegisterReliableTransMode(void);
void DTAP_RspTimerExpireProc(void *args);
void DTAP_RegisterTransMode(DTAP_TransMode_S *dtapTransMode);
DTAP_TransMode_S *DTAP_GetTransMode(uint8_t modeType);
bool DTAP_IsFrameSeqSmaller(uint16_t own, uint16_t other);
uint16_t DTAP_GetNextFrameSeq(uint16_t seq);
void FreeStreamList(SDF_DListEntry_S *entry);
void DTAP_DestroyBasicCacheFrame(SDF_DListEntry_S *entry);
void DTAP_RecvBasicFrameContinue(DTAP_Channel_S *transChan);
uint32_t DTAP_TransStartTimer(int *timer, uint16_t expire, bool isPeriodic,
    SDF_TimerCallback cbk, DTAP_Channel_S *channel);
void DTAP_TransStopTimer(int *timer);
uint32_t DTAP_TransRestartTimer(int *timer, uint16_t expire, bool isPeriodic,
    SDF_TimerCallback cbk, DTAP_Channel_S *channel);

#ifdef __cplusplus
}
#endif
#endif