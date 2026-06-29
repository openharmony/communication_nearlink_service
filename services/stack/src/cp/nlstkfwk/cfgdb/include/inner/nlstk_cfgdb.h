/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef NLSTK_CFGDB_H
#define NLSTK_CFGDB_H

#include <stdint.h>
#include "dli_event_struct.h"
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t status;
    uint8_t caps[SLE_MEASURE_LEN];
} __attribute__((packed)) CfgdbLocalCsCaps_S;

typedef struct {
    uint8_t encryptAlgo;
    uint8_t intgProtectAlgo;
    uint8_t keyDerivAlgo;
    uint8_t keyNegoAlgo;
} __attribute__((packed)) CfgdbAlgoCaps_S;

typedef enum {
    CFGDB_FIND_SERVICE_STRUCTURE = 0,
    CFGDB_READ_MULTI_HANDLES,
    CFGDB_START_PLAYING_MERGE,
    CFGDB_QOS_ID_CONFIG = 6,
} CfgdbManufacturerAbility_E;

uint32_t CfgdbReadCommConfigValue(void);

uint32_t CfgdbInit(void);

void CfgdbDeinit(void);

uint8_t CfgdbGetLocalVersion(void);

uint16_t CfgdbGetMaxAdvDataLen(void);

uint8_t CfgdbGetMaxAdvNodesNum(void);

uint32_t CfgdbReadAlgoCaps(CfgdbAlgoCaps_S *caps);

uint32_t CfgdbReadLocalCsCaps(CfgdbLocalCsCaps_S *caps);

bool CfgdbGetManufacturerSupport(SLE_Addr_S *addr, CfgdbManufacturerAbility_E ability);

#ifdef __cplusplus
}
#endif
#endif