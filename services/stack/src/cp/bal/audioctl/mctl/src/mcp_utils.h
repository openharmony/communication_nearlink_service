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
#ifndef MCP_UTILS_H
#define MCP_UTILS_H

#include "ssap_type.h"
#include "mcp_type.h"
#include "nlstk_mcp_media_server.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PARSE_TO_UINT8(u8, p) do {                                  \
        (u8) = ((uint8_t)(*(p)));                                   \
        (p) += 1;                                                   \
} while (0)

#define PARSE_TO_UINT16(u16, p) do {                                    \
        (u16) = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8));   \
        (p) += 2;                                                       \
} while (0)

#define STREAM_WRITE(p, data, len) do {                             \
        (void)memcpy_s((p), (len), (data), (len));                  \
        (p) += (len);                                               \
} while (0)

NLSTK_SsapUuid_S McpConvertUuidToStru(uint16_t uuid);

uint16_t McpConvertUuidTo16Bits(NLSTK_SsapUuid_S uuidStru);

void McpFreeMediaInfo(void *ptr);

void McpFreeUpdatePropertyParam(void *ptr);

void McpFreeSetStreamVolumeParam(void *ptr);

uint32_t McpCopyMediaInfo(NLSTK_McpMediaInfo_S *dest, NLSTK_McpMediaInfo_S *src);

uint16_t McpGetPropertyUuidByType(NLSTK_McpPropertyType_E propertyType);

NLSTK_McpPropertyType_E McpGetPropertyTypeByUuid(uint16_t uuid);

NLSTK_VariableData_S *McpMediaValueConvert(void *data, NLSTK_McpPropertyType_E type);

#ifdef __cplusplus
}
#endif

#endif