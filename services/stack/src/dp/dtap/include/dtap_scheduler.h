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

uint32_t DTAP_SchedulerInit(void);
uint32_t DTAP_SchedulerDeinit(void);
void DTAP_ChannelDown(uint16_t lcid, uint8_t srcTcid);
uint32_t DTAP_DataSendWithPriority(DTAP_Channel_S *transChan, SDF_Buff_S *buff);

#ifdef __cplusplus
}
#endif

#endif // DTAP_SCHEDULER_H