/****************************************************************************
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
****************************************************************************/

/****************************************************************************
 *
 * this file contains the CM connect signaling capability
 *
 ***************************************************************************/

#ifndef CM_SIGNALING_CAP_H
#define CM_SIGNALING_CAP_H

#include <stdint.h>
#include "cm_signaling_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

// 本地发起CAPABILITY_REQ
uint32_t CM_SendReqSignalingCapability(uint16_t lcid, CM_CapabilityBitmap_S *cap);

// 本端CAPABILITY_RSP对应的处理
uint32_t CM_ProcessRspSignalingCapability(uint16_t lcid, CM_SignalingHead_S *pkt);

// 本端对CAPABILITY_REQ的回复
uint32_t CM_ProcessReqSignalingCapability(uint16_t lcid, CM_SignalingHead_S *pkt);
#ifdef __cplusplus
}
#endif
#endif // CM_SIGNALING_CAP_H