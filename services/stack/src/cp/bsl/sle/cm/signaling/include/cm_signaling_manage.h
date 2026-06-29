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
 * this file contains the CM connect signaling management
 *
 ***************************************************************************/

#ifndef CM_SIGNALING_MANAGE_H
#define CM_SIGNALING_MANAGE_H

#include <stdint.h>
#include "cm_signaling_struct.h"
#include "cm_signaling_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CM_DECODE2BYTE(_ptr) (uint16_t)(*(uint8_t *)(_ptr) | (*(uint8_t *)((_ptr) + 1) << 8))
#define CM_ENCODE2BYTE(_ptr, value) \
do { \
    *((uint8_t *)(_ptr) + 1) = (uint8_t)((uint16_t)(value) >> 8); \
    *(uint8_t *)(_ptr) = (uint8_t)(value); \
} while (0)

#define CM_DECODE4BYTE(_ptr) \
    (uint32_t)(*(uint8_t *)(_ptr) | (*(uint8_t *)((_ptr) + 1) << 8) \
    | (*(uint8_t *)((_ptr) + 2) << 16) | (*(uint8_t *)((_ptr) + 3) << 24))

#define CM_ENCODE4BYTE(_ptr, value) \
do { \
    *(uint8_t *)((_ptr) + 3) = (uint8_t)((value) >> 24); \
    *(uint8_t *)((_ptr) + 2) = (uint8_t)((value) >> 16); \
    *(uint8_t *)((_ptr) + 1) = (uint8_t)((value) >> 8); \
    *(uint8_t *)(_ptr) = (uint8_t)(value); \
} while (0)

// 信令超时回调函数
typedef void (*CM_SignalingTimeoutCbk)(void *args);

// 注册信令发送函数
void CM_SignalingRegisterCbk(CM_SendSignalingDataCbk sendFunc);
// 获取信令处理函数
CM_SignalingHandle CM_SignalingGetManagerHandler(uint8_t code);
// 获取信令id
uint8_t CM_GetIdentifier(void);
// 插入信令缓存
uint32_t CM_SignalingCacheInsert(uint16_t lcid, uint8_t id, uint8_t code, void *args, CM_SignalingTimeoutCbk cbk);
// 移除信令缓存
void CM_SignalingCacheRemove(uint8_t id, uint8_t code);
// 移除lcid对应的所有信令缓存
void CM_SignalingCacheClearByLcid(uint16_t lcid);
// 初始化信令缓存
uint32_t CM_SignalingCacheInit(void);
// 释放信令缓存
void CM_SignalingCacheDeinit(void);
#ifdef __cplusplus
}
#endif
#endif // CM_SIGNALING_MANAGER_H