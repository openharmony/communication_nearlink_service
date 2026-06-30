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
#include "icce_type.h"
#include "icce_utils.h"

static uint8_t g_ssapStdUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint16_t IcceConvertUuidTo16Bits(NLSTK_SsapUuid_S uuidStru)
{
    uint16_t uuid = 0;
    uuid |= (uint16_t)(uuidStru.uuid[ICCE_UUID_FIFTEENTH_BYTE]) << 0x08;
    uuid |= (uint16_t)(uuidStru.uuid[ICCE_UUID_SIXTEENTH_BYTE]);
    return uuid;
}

NLSTK_SsapUuid_S IcceConvertUuidToStru(uint16_t uuid)
{
    NLSTK_SsapUuid_S uuidStru;
    for (int i = 0; i < ICCE_UUID_FIFTEENTH_BYTE; i++) {
        uuidStru.uuid[i] = g_ssapStdUuid[i];
    }
    uuidStru.uuid[ICCE_UUID_FIFTEENTH_BYTE] = (uint8_t)((uuid & 0xFF00) >> 0x08);
    uuidStru.uuid[ICCE_UUID_SIXTEENTH_BYTE] = (uint8_t)(uuid & 0x00FF);
    return uuidStru;
}

bool IcceCompAppId(void *ptr, void *args)
{
    IcceDevice_S *dev = (IcceDevice_S *)ptr;
    int32_t appId = *(int32_t *)args;
    return dev->appId == appId;
}

bool IcceCompAddr(void *ptr, void *args)
{
    IcceDevice_S *dev = (IcceDevice_S *)ptr;
    SLE_Addr_S *addr = (SLE_Addr_S *)args;
    return (SDF_CompareSleAddr(&dev->addr, addr) == 0);
}