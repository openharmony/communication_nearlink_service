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
#ifndef HIBOX_PROCESS
#define HIBOX_PROCESS

#include "hibox_def.h"
#include "hibox_local.h"

#define HIBOX_STRING_TO_STREAM(p, src, count) do { \
    uint32_t _index; \
    for (_index = 0; _index < (count); _index++) { \
        *(p)++ = *(((uint8_t*)(src)) + _index); \
    } \
} while (0)

#define STREAM_TO_INT16(i16, p) \
    {                                           \
        uint16_t it16 = ((uint16_t)(*((p)+1)) << 8) | ((uint16_t)(*(p))); \
        (i16) = (int16_t)it16;                 \
        (p) += 2;                               \
    }
#define STREAM_TO_UINT8(u8, p) \
    {                            \
        (u8) = (uint8_t)(*(p));    \
        (p) += 1;                  \
    }
#define STREAM_TO_UINT16(u16, p)                                  \
    {                                                               \
        (u16) = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); \
        (p) += 2;                                                     \
    }
#define STREAM_TO_ARRAY(a, p, len)                                   \
    {                                                                  \
        int ijk;                                                         \
        for (ijk = 0; ijk < (len); ijk++) ((uint8_t*)(a))[ijk] = *(p)++; \
    }

#define STREAM_TO_UINT32(u32, p)                                      \
    {                                                                   \
        (u32) = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + \
                 ((((uint32_t)(*((p) + 2)))) << 16) +                     \
                 ((((uint32_t)(*((p) + 3)))) << 24));                     \
        (p) += 4;                                                         \
    }

#define UINT16_TO_STREAM(p, u16)    \
    {                                 \
        *(p)++ = (uint8_t)(u16);        \
        *(p)++ = (uint8_t)((u16) >> 8); \
    }

#define UINT8_TO_STREAM(p, u8) \
    { *(p)++ = (uint8_t)(u8); }

#define HIBOX_HEADER_OFFSET_SIZE 0
#define SERVICE_ID_OFFSET_SIZE 1
#define COMMAND_ID_OFFSET_SIZE 2
#define HIBOX_UPPER_DATA_MAX_LEN 30 // restrict the echo len from upper
#define HIBOX_HEAD_FIELD_LEN 3 // Header(1) + Service_id(1) + Command_id(1)
#define HIBOX_TLV_HEADER_LEN 3 // TLV header length. 3 = type(1) + length(2)
#define HIBOX_REQ_FIXED_HEADER 0x3F
#define HIBOX_RSP_FIXED_HEADER 0xF3
#define HIBOX_DATA_HEADER_LEN 2 // Service_id(1) + Command_id(1)
#define HIBOX_TLV_MIC_LEN 2 // length of 'MIC' field
#define HIBOX_MIN_PAYLOAD_LEN  (HIBOX_HEAD_FIELD_LEN + HIBOX_TLV_MIC_LEN)
#define HIBOX_ADAPTION_LEN 4 // Header(1) + Service_id(1) + length of 'MIC' field
#define HIBOX_HEADER_SERVICEID_LEN 2 // Header(1) + Service_id(1)
#define HIBOX_ERROR_MSG_LEN 4 // type(1) + length(2) + value(1)
#define VENDOR_ECHO_BUFFER_MAX 64
#define HIBOX_RSP_TYPE_RESULT 0x7F
#define HIBOX_COMMON_MSG_VALUE_LEN 1
#define HIBOX_ERROR_MSG_TLV_LEN (HIBOX_TLV_HEADER_LEN + HIBOX_COMMON_MSG_VALUE_LEN)
#define MAX_SERVICE_NUM 10
#define HIBOX_RSP_TIMEOUT_MS (3 * 1000) // 3 seconds
#define CRC16_POLY 0x8005
#define HIBOX_INVALID_DATA 0xFF
#define MAX_PARSE_DATA_LEN 0x400
#define PARSED_ATA_HEADER 3
#define TLV_BUF_LEN 640
#define HIBOX_SOURCE_DATA_HEADER_LEN 3 // msgType(1) + dataLen(2)

#define ADDR_1ST_INDEX 0
#define ADDR_2ND_INDEX 1
#define ADDR_3RD_INDEX 2
#define ADDR_5TH_INDEX 4
#define ADDR_6TH_INDEX 5
#define ADDR0(addr) ((addr)[ADDR_1ST_INDEX])
#define ADDR1(addr) ((addr)[ADDR_2ND_INDEX])
#define ADDR2(addr) ((addr)[ADDR_3RD_INDEX])
#define ADDR4(addr) ((addr)[ADDR_5TH_INDEX])
#define ADDR5(addr) ((addr)[ADDR_6TH_INDEX])

/* Hibox service ID */
enum HiboxSID {
    SID_SEVICE_MGMT = 0x01,
    SID_SCENE_MGMT,
    SID_HITWS_CTRL,
    SID_DTS_MGMT,
    SID_PAIR_CONN,
    SID_ICARRY_MGMT,
    SID_AUTO_PAIR,
    SID_AUDIO_MGMT = 0x08,
    SID_DEVICE_FOUND = 0x09,
    SID_SERVICE_UHD = 0x0A,
};

#pragma pack(1)
/* echo msg, stack report to hibox, hibox report to service */
typedef struct {
    uint8_t echoType;
    uint8_t sid;
    uint8_t cmdId;
    uint16_t tlvLen;
    uint8_t addr[HIBOX_ADDR_LEN];
    uint8_t transport;
    uint8_t id;
    uint8_t tlv[0];
} HiboxIndMsgArg;

typedef struct {
    uint8_t sid;
    uint8_t cmdId;
    uint16_t tlvLen;
    uint8_t addr[HIBOX_ADDR_LEN];
    uint8_t transport;
    uint8_t id;
    uint8_t tlv[0];
} HiboxSendMsgArg;

typedef struct {
    uint8_t dataType;
    uint8_t sid;
    uint8_t cmdId;
    uint16_t parseReqLen;
    uint16_t parseRspLen;
} HiboxDataTypeTable;
#pragma pack()

#endif
