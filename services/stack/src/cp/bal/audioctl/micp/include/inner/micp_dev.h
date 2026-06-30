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
#ifndef MICP_DEV_H
#define MICP_DEV_H

#include "sdf_addr.h"
#include "nlstk_micp_client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    SLE_Addr_S addr;
    int32_t appId;
    uint8_t micState;
    uint16_t switchHandle;
    NLSTK_MicpConnectState_E state;
} MicpDevice_S;

void MicpAddDevice(SLE_Addr_S *addr);

void MicpDeleteDevice(SLE_Addr_S *addr);

MicpDevice_S *MicpFindDeviceByAddr(SLE_Addr_S *addr);

MicpDevice_S *MicpFindDeviceByApp(int32_t appId);

uint32_t MicpDevInit(void);

void MicpDevDisable(void);

void MicpDevDeinit(void);

#ifdef __cplusplus
}
#endif
#endif