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
#ifndef NLSTK_MCP_VOLUME_H
#define NLSTK_MCP_VOLUME_H

#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  音量控制客户端使能
 */
uint32_t McpVolumeEnable(void);

/**
 * @brief  音量控制客户端去使能
 */
void McpVolumeDisable(void);

/**
 * @brief  通知音频流访问点信息
 */
void McpVolumeNotifyAccessPoint(SLE_Addr_S *addr, uint8_t mediaPoint, uint8_t callPoint);

#ifdef __cplusplus
}
#endif

#endif