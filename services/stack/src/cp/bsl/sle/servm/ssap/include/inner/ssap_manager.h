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

#ifdef __cplusplus
}
#endif
#endif