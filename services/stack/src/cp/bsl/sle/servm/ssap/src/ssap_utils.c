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
#include <string.h>
#include "securec.h"
#include "cpfwk_log.h"
#include "ssap_pkt.h"
#include "ssap_utils.h"

#define SSAP_ENC_LOG_UUID_INDEX_0 0
#define SSAP_ENC_LOG_UUID_INDEX_1 1
#define SSAP_ENC_LOG_UUID_INDEX_14 14
#define SSAP_ENC_LOG_UUID_INDEX_15 15

static uint8_t g_ssapStdBaseUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

struct SSAP_EncryptedUuidString SSAP_GetEncryptUuid(const NLSTK_SsapUuid_S *uuid)
{
    struct SSAP_EncryptedUuidString res = {0};
    (void)sprintf_s(res.buf, SSAP_ENC_UUID_STR_LEN, "%02x%02x****-****-****-****-********%02x%02x",
        uuid->uuid[SSAP_ENC_LOG_UUID_INDEX_0], uuid->uuid[SSAP_ENC_LOG_UUID_INDEX_1],
        uuid->uuid[SSAP_ENC_LOG_UUID_INDEX_14], uuid->uuid[SSAP_ENC_LOG_UUID_INDEX_15]);
    return res;
}


bool SSAP_IsUuidEqual(NLSTK_SsapUuid_S *uuid, NLSTK_SsapUuid_S *target)
{
    if (uuid == NULL && target == NULL) {
        return true;
    }
    if (uuid == NULL || target == NULL) {
        return false;
    }
    return memcmp(uuid->uuid, target->uuid, SSAP_UUID128_LEN) == 0;
}

uint8_t SSAP_GetOpcodeType(uint8_t opcode)
{
    if (opcode < SSAP_ERROR_RSP || opcode >= SSAP_CODE_MAX) {
        return SSAP_TRANS_INVALID;
    }
    static uint8_t ssapOpcodeType[SSAP_CODE_MAX - 1] = {
        SSAP_TRANS_RSP,  // SSAP_ERROR_RSP
        SSAP_TRANS_REQ,  // SSAP_EXCHANGE_INFO_REQ
        SSAP_TRANS_RSP,  // SSAP_EXCHANGE_INFO_RSP
        SSAP_TRANS_REQ,  // SSAP_FIND_STRUCTURE_REQ
        SSAP_TRANS_RSP,  // SSAP_FIND_STRUCTURE_RSP
        SSAP_TRANS_REQ,  // SSAP_FIND_STRUCTURE_BY_UUID_REQ
        SSAP_TRANS_RSP,  // SSAP_FIND_STRUCTURE_BY_UUID_RSP
        SSAP_TRANS_REQ,  // SSAP_READ_REQ
        SSAP_TRANS_RSP,  // SSAP_READ_RSP
        SSAP_TRANS_REQ,  // SSAP_READ_BY_UUID_REQ
        SSAP_TRANS_RSP,  // SSAP_READ_BY_UUID_RSP
        SSAP_TRANS_CMD,  // SSAP_WRITE_CMD
        SSAP_TRANS_REQ,  // SSAP_WRITE_REQ
        SSAP_TRANS_RSP,  // SSAP_WRITE_RSP
        SSAP_TRANS_NOTI, // SSAP_VALUE_NTF
        SSAP_TRANS_IND,  // SSAP_VALUE_IND
        SSAP_TRANS_ACK,  // SSAP_VALUE_ACK
        SSAP_TRANS_CMD,  // SSAP_CALL_METHOD_CMD
        SSAP_TRANS_REQ,  // SSAP_CALL_METHOD_REQ
        SSAP_TRANS_RSP   // SSAP_CALL_METHOD_RSP
    };
    return ssapOpcodeType[opcode - 1];
}

static inline void SSAP_SwapBuffer(uint8_t *destBuf, uint32_t destBufLen, uint8_t *srcBuf, uint32_t srcBufLen)
{
    if (destBufLen < srcBufLen) {
        return;
    }
    for (uint32_t i = 0; i < srcBufLen; i++) {
        destBuf[srcBufLen - 1 - i] = srcBuf[i];
    }
}

void SSAP_NetToHostUuid(uint8_t *destBuf, uint32_t destBufLen, uint8_t *srcBuf, uint32_t srcBufLen)
{
    SSAP_SwapBuffer(destBuf, destBufLen, srcBuf, srcBufLen);
}

void SSAP_HostToNetUuid(uint8_t *destBuf, uint32_t destBufLen, uint8_t *srcBuf, uint32_t srcBufLen)
{
    SSAP_SwapBuffer(destBuf, destBufLen, srcBuf, srcBufLen);
}

bool SSAP_CheckUuidStd(NLSTK_SsapUuid_S *uuid)
{
    uint8_t checkLen = SSAP_UUID_STD_BASE_OFFSET;
    for (uint8_t i = 0; i < checkLen; i++) {
        if (uuid->uuid[i] != g_ssapStdBaseUuid[i]) {
            return false;
        }
    }
    return true;
}

void SSAP_GetUuidFromPktBuf(NLSTK_SsapUuid_S *uuid, uint8_t *srcBuf, uint32_t srcBufLen)
{
    if (srcBufLen < SSAP_UUID128_LEN) {
        (void)memcpy_s(uuid->uuid, SSAP_UUID128_LEN, g_ssapStdBaseUuid, SSAP_UUID128_LEN);
    }
    uint32_t offset = SSAP_UUID128_LEN - srcBufLen;
    SSAP_NetToHostUuid(uuid->uuid + offset, srcBufLen, srcBuf, srcBufLen);
}

void SSAP_PutUuidToPktBuf(NLSTK_SsapUuid_S *uuid, uint8_t *destBuf, uint32_t destBufLen)
{
    uint32_t uuidLen = SSAP_CheckUuidStd(uuid) ? SSAP_UUID16_LEN : SSAP_UUID128_LEN;
    if (destBufLen < uuidLen) {
        CP_LOG_ERROR("[SSAP] put uuid to pkt wrong size, need: %d, real: %d", uuidLen, destBufLen);
        return;
    }
    uint32_t offset = SSAP_UUID128_LEN - uuidLen;
    SSAP_HostToNetUuid(destBuf, uuidLen, uuid->uuid + offset, uuidLen);
}

uint8_t GetDescriptorCountLenByItemType(uint8_t itemType)
{
    // 只有服务、属性、方法、事件有描述符类型列表
    switch (itemType) {
        case ITEM_TYPE_STD_PRIMARY_SERVICE:
        case ITEM_TYPE_STD_SECONDARY_SERVICE:
        case ITEM_TYPE_STD_PROPERTY:
        case ITEM_TYPE_STD_METHOD:
        case ITEM_TYPE_STD_EVENT:
        case ITEM_TYPE_VENDOR_PRIMARY_SERVICE:
        case ITEM_TYPE_VENDOR_SECONDARY_SERVICE:
        case ITEM_TYPE_VENDOR_PROPERTY:
        case ITEM_TYPE_VENDOR_METHOD:
        case ITEM_TYPE_VENDOR_EVENT:
            return SSAP_FIND_DESCRIPTOR_COUNT_LEN;
        default:
            // ITEM_TYPE_STD_SERVICE_REFERENCE 
            // ITEM_TYPE_VENDOR_SERVICE_REFERENCE
            return 0;
    }
}

uint8_t GetStartEndHandleLenByItemType(uint8_t itemType)
{
    // 只有服务引用声明有起始句柄、结束句柄
    switch (itemType) {
        case ITEM_TYPE_STD_SERVICE_REFERENCE:
        case ITEM_TYPE_VENDOR_SERVICE_REFERENCE:
            return SSAP_HANDLE_LEN + SSAP_HANDLE_LEN;
        default:
            return 0;
    }
}

uint8_t GetOperationLenByItemType(uint8_t itemType)
{
    // 只有属性、方法、事件有操作指示
    switch (itemType) {
        case ITEM_TYPE_STD_PROPERTY:
        case ITEM_TYPE_STD_METHOD:
        case ITEM_TYPE_STD_EVENT:
        case ITEM_TYPE_VENDOR_PROPERTY:
        case ITEM_TYPE_VENDOR_METHOD:
        case ITEM_TYPE_VENDOR_EVENT:
            return SSAP_FIND_OPERATION_LEN;
        default:
            return 0;
    }
}
