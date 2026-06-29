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
#ifndef MCP_VOLUME_DEV_H
#define MCP_VOLUME_DEV_H

#include "sdf_addr.h"
#include "mcp_type.h"

#ifdef __cplusplus
extern "C" {
#endif

bool McpVolumeStartTimer(McpVolumeDevice_S *dev);

void McpVolumeDelTimer(McpVolumeDevice_S *dev);

bool McpStreamVolumeStartTimer(McpVolumeDevice_S *dev);

void McpStreamVolumeDelTimer(McpVolumeDevice_S *dev);

uint32_t McpVolumeDevInit(void);

void McpVolumeDevDeInit(void);

McpVolumeDevice_S *McpVolumeFindDeviceByAddr(SLE_Addr_S *addr);

McpVolumeDevice_S *McpVolumeFindDeviceByAppId(int32_t appId);

uint32_t McpVolumeAddDevice(int32_t appId, SLE_Addr_S *addr);

uint32_t McpVolumeRemoveDevice(int32_t appId);

#ifdef __cplusplus
}
#endif

#endif