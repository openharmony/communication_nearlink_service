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

// 信令结构体
typedef struct CM_Signaling_S {
    uint8_t recvCode;           // 接收到的信令码，用于匹配信令类型，参见CM_SignalingCode_S
    uint8_t requestCode;        // 接收响应信令对应的请求信令码，如果recvCode为请求信令则requestCode为本身
    CM_SignalingHandle handle;  // 信令处理回调
} CM_Signaling_S;

// 信令超时回调函数
typedef void (*CM_SignalingTimeoutCbk)(void *args);

// 注册信令发送函数
void CM_SignalingRegisterCbk(CM_SendSignalingDataCbk sendFunc);
// 获取信令函数
const CM_Signaling_S *CM_SignalingGet(uint8_t code);
// 获取信令id，成功返回true并通过id返回值，失败（全部id被未决信令占用）返回false
bool CM_GetIdentifier(uint8_t *id);
// 插入信令缓存
uint32_t CM_SignalingCacheInsert(uint16_t lcid, uint8_t id, uint8_t code, void *args, CM_SignalingTimeoutCbk cbk);
// 移除信令缓存，返回true表示移除成功
bool CM_SignalingCacheRemove(uint16_t lcid, uint8_t id, uint8_t code);
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