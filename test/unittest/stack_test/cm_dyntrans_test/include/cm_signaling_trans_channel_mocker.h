/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef CM_SIGNALING_TRANS_CHANNEL_MOCKER_H
#define CM_SIGNALING_TRANS_CHANNEL_MOCKER_H

#include <stdint.h>
#include "dli_cmd_struct.h"

CM_SignalingTransChanCbks_S* CM_GetSignalingTransChanCbks(void);

void CM_SetSignalingTransChanEstablishReqSend(bool failed);
void CM_SetSignalingTransChanEstablishRspSend(bool failed);
void CM_SetSignalingTransChanDestTcidInvalid(bool invalid);
void CM_SetSignalingTransChanEstablishRspSendResult(uint8_t result);
void CM_SetSignalingTransChanReleaseReqSend(bool failed);
void CM_SetSignalingTransChanReleaseRspSend(bool failed);

#endif // CM_SIGNALING_TRANS_CHANNEL_MOCKER_H