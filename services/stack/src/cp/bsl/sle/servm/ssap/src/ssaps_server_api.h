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
#ifndef SSAPS_SERVER_API_H
#define SSAPS_SERVER_API_H

#include <stdint.h>
#include "ssap_common.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

uint32_t SSAP_ServerInit(void);
void SSAP_ServerDeInit(void);
void SSAP_CacheService(void *arg);
void SSAP_CacheProperty(void *arg);
void SSAP_CacheDescriptor(void *arg);
void SSAP_StartService(void *arg);
void SSAP_RemoveService(void *arg);

/**
 * @brief  发送服务数据属性或事件变化通知（无确认）
 */
void SSAP_ValueNtf(void *arg);

/**
 * @brief  发送服务数据属性或事件变化指示（有确认）
 */
void SSAP_ValueInd(void *arg);

/**
 * @brief  服务端通过句柄更新本地数据
 */
void SSAP_UpdateItemValueByHandle(void *arg);

/**
 * @brief 用户返回授权结果
 */
void SSAP_SendUserResponse(void *arg);

/**
 * @brief 用户返回方法调用结果
 */
void SSAP_SendMethodCallRes(void *arg);

#ifdef __cplusplus
}
#endif

#endif