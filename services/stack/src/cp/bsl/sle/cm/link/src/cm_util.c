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

#include "cm_util.h"
#include "cm_log.h"

uint32_t CM_PackVersionLocalIndex(uint8_t version, uint16_t localIndex)
{
    return (((uint32_t)(localIndex) << CM_SHIFT_BITS_16) | (version));
}

uint8_t CM_UnPackVersion(uint32_t localIndexVersion)
{
    return ((uint8_t)(((uintptr_t)(localIndexVersion)) & 0xFF));
}

uint16_t CM_UnPackLocalIndex(uint32_t localIndexVersion)
{
    return ((uint16_t)((((uintptr_t)(localIndexVersion)) >> CM_SHIFT_BITS_16) & 0xFFFF));
}

void CM_CheckAndFixAddrType(SLE_Addr_S *addr)
{
    /* 支持PUBLIC_ADDRESS(0)和RPA_UNRESOLV_ADDRESS(4)，其他类型设为默认0 */
    if (addr->type != PUBLIC_ADDRESS && addr->type != RPA_UNRESOLV_ADDRESS) {
        addr->type = PUBLIC_ADDRESS;
    } else {
        CM_LOGW("addr type[%hhu] is not suppported", addr->type);
    }
}