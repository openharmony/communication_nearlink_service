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
 * this file contains the CM connect signaling protocol version
 *
 ***************************************************************************/

#ifndef CM_SIGNALING_VERSION_H
#define CM_SIGNALING_VERSION_H

#include <stdint.h>
#include <stdbool.h>
#include "cm_signaling_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CM_CAP_VERSION1_1 0x0101
#define CM_CAP_DEFAULT_VERSION CM_CAP_VERSION1_1

void CM_SetLinkProtocolVersion(uint16_t connId, uint16_t protocolVersion);

void CM_SetLinkExchangeVersion(uint16_t connId, uint16_t exchangeVersion);

uint16_t CM_GetDeviceLinkDeviceType(uint16_t connId);

void CM_SetDeviceLinkDeviceType(uint16_t connId, bool hasVerBit);

void CM_SetLinkRxWindow(uint16_t connId, uint8_t rxWindow);

void CM_SetLinkTransMode(uint16_t connId, CM_TransMode mode);

void CM_SetLinkMtu(uint16_t connId, uint16_t mtu);

#ifdef __cplusplus
}
#endif
#endif // CM_SIGNALING_VERSION_H