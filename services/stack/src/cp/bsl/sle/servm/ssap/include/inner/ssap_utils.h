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
#ifndef SSAP_UTILS_H
#define SSAP_UTILS_H

#include <stdbool.h>
#include <ssap_type.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum SSAP_LoopControlType {
    LOOP_CONTINUE,
    LOOP_BREAK,
    LOOP_RET_FALSE,
    LOOP_RET_TRUE,
    LOOP_NORMAL_EXECUTION,
} SSAP_LoopControlType_E;

#define SSAP_NTOHL(x) ((((x) & 0x000000ffu) << 24) |    \
                      (((x) & 0x0000ff00u) << 8) |      \
                      (((x) & 0x00ff0000u) >> 8) |      \
                      (((x) & 0xff000000u) >> 24))

#define SSAP_HTONL(x) ((((x) & 0x000000ffu) << 24) |    \
                      (((x) & 0x0000ff00u) << 8) |      \
                      (((x) & 0x00ff0000u) >> 8) |      \
                      (((x) & 0xff000000u) >> 24))

#define SSAP_NTOHS(x) ((((x) & 0x00ff) << 8) |          \
                      (((x) & 0xff00) >> 8))

#define SSAP_HTONS(x) ((((x) & 0x00ff) << 8) |          \
                      (((x) & 0xff00) >> 8))

#define SSAP_UINT32_TO_BYTE_LITTLE(data, val)                               \
    do {                                                                    \
        *(uint8_t *)((data) + 3) = (uint8_t)((val) >> 24);                  \
        *(uint8_t *)((data) + 2) = (uint8_t)((val) >> 16);                  \
        *(uint8_t *)((data) + 1) = (uint8_t)((val) >> 8);                   \
        *(uint8_t *)(data) = (uint8_t)(val);                                \
    } while (0)

#define SSAP_UINT16_TO_BYTE_LITTLE(data, val)                               \
    do {                                                                    \
        *(uint8_t *)((data) + 1) = (uint8_t)((val) >> 8);                   \
        *(uint8_t *)(data) = (uint8_t)(val);                                \
    } while (0)

#define SSAP_BYTE_TO_UINT16_LITTLE(data)                                    \
    (uint16_t)(*(uint8_t *)(data) | (*(uint8_t *)((data) + 1) << 8))

#define SSAP_BYTE_TO_UINT32_LITTLE(data)                                    \
    (uint32_t)(*(uint8_t *)(data) | (*(uint8_t *)((data) + 1) << 8) |       \
    (*(uint8_t *)((data) + 2) << 16) | (*(uint8_t *)((data) + 3) << 24))

#define SSAP_ENC_UUID_STR_LEN 64

struct SSAP_EncryptedUuidString {
    char buf[SSAP_ENC_UUID_STR_LEN];
};

struct SSAP_EncryptedUuidString SSAP_GetEncryptUuid(const NLSTK_SsapUuid_S *uuid);
#define SSAP_GET_ENC_UUID(uuid) ((const char *)(SSAP_GetEncryptUuid(uuid).buf))

bool SSAP_IsUuidEqual(NLSTK_SsapUuid_S *uuid, NLSTK_SsapUuid_S *target);
void SSAP_NetToHostUuid(uint8_t *destBuf, uint32_t destBufLen, uint8_t *srcBuf, uint32_t srcBufLen);
void SSAP_HostToNetUuid(uint8_t *destBuf, uint32_t destBufLen, uint8_t *srcBuf, uint32_t srcBufLen);
bool SSAP_CheckUuidStd(NLSTK_SsapUuid_S *uuid);
void SSAP_GetUuidFromPktBuf(NLSTK_SsapUuid_S *uuid, uint8_t *srcBuf, uint32_t srcBufLen);
void SSAP_PutUuidToPktBuf(NLSTK_SsapUuid_S *uuid, uint8_t *destBuf, uint32_t destBufLen);

uint8_t SSAP_GetOpcodeType(uint8_t opcode);
uint8_t GetOperationLenByItemType(uint8_t itemType);
uint8_t GetStartEndHandleLenByItemType(uint8_t itemType);
uint8_t GetDescriptorCountLenByItemType(uint8_t itemType);

#ifdef __cplusplus
}
#endif

#endif