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
#ifndef ICCE_TYPE_H
#define ICCE_TYPE_H

#include <stdint.h>
#include "sdf_addr.h"
#include "nlstk_icce_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ICCE_SERVICE_UUID  0x060D
#define ICCE_PORT_UUID 0x104C
#define ICCE_UUID_FIFTEENTH_BYTE 14
#define ICCE_UUID_SIXTEENTH_BYTE 15
#define ICCE_DESC_CLIENT_CONFIG_LEN 2
#define ICCE_PORT_LEN 4
#define ICCE_INVALID_PORT (-1)
#define ICCE_INVALID_APPID (-1)

typedef struct {
    int32_t *port;
    SLE_Addr_S *addr;
} IcceReadInfo_S;

typedef struct {
    SLE_Addr_S addr;
    int32_t appId;
    uint16_t icceServiceHandle;
    uint16_t portHandle;
    uint8_t state;
    NLSTK_IcceServiceInfo_S devicesInfo;
} IcceDevice_S;

#ifdef __cplusplus
}
#endif
#endif