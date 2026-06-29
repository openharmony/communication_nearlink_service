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
 * this file contains the CM util functions
 *
 ***************************************************************************/

#ifndef CM_UTIL_H
#define CM_UTIL_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CM_SHIFT_BITS_16    16

/**
 * @brief 将version和local index压缩为uint32_t保存，高16bit为local index，低8bit为version
 */
uint32_t CM_PackVersionLocalIndex(uint8_t version, uint16_t localIndex);

/**
 * @brief 从uint32_t的低8bit解析version
 */
uint8_t CM_UnPackVersion(uint32_t localIndexVersion);

/**
 * @brief 从uint32_t的高16bit解析local_index
 */
uint16_t CM_UnPackLocalIndex(uint32_t localIndexVersion);

/*
 * @brief 检查并修正地址类型，支持PUBLIC_ADDRESS(0)和RPA_UNRESOLV_ADDRESS(4)，其他类型设为默认0
 */
void CM_CheckAndFixAddrType(SLE_Addr_S *addr);

/*
 * @brief 白名单主动连接时，超时或者取消连接，其地址为全0地址
 */
static inline bool CM_IsEmptyAddr(const SLE_Addr_S *addr)
{
    SLE_Addr_S emptyAddr = { 0 }; // 白名单连接，全0地址
    return (memcmp(&emptyAddr, addr, sizeof(SLE_Addr_S)) == 0);
}

#ifdef __cplusplus
}
#endif

#endif // CM_UTIL_H