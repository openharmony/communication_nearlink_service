/**
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

/*********************************************************************************************************
 *
 * this file contains light weight connectionless-mode transport protocol implementation and related apis.
 * CLTP: connectionless-mode transport protocol
 *
 *********************************************************************************************************/

#ifndef TRANSPORT_CLTP_H
#define TRANSPORT_CLTP_H

#include <stdint.h>

#include "sdf_buff.h"
#include "transport_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TRANS_CLTP_OPT_PAYLOAD_MASK 0x0001

/**
 * @brief 无连接的传输层报文格式定义，各字段的字节序采用大端序。
 *    0   1   2   3   4   5   6   7
 *  +---+---+---+---+---+---+---+---+
 *  |   version |   option length   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+
 *  !      src port（低位字节）      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+
 *  !      src port（高位字节）      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+
 *  !      dst port（低位字节）      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+
 *  !      dst port（高位字节）      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+
 *  !option fields bitmap（低位字节）|
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+
 *  !option fields bitmap（高位字节）|
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+
 *  !     option fields（变长）      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+
 *  !          pads（变长）          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+
 *  !          payload（变长）       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+
 */
typedef struct TRANS_CltpHeader {
    TRANS_ProtoBasicHeader_S basic;
    uint8_t option[0];
} __attribute__((packed)) TRANS_CltpHeader_S;

typedef struct TRANS_CltpHeaderOpts {
    uint16_t optionBitmap;
    uint16_t payloadLen;
} __attribute__((packed)) TRANS_CltpHeaderOpts_S;


static inline uint16_t TRANS_CltpHeaderSize(void)
{
    return sizeof(TRANS_CltpHeader_S) + sizeof(TRANS_CltpHeaderOpts_S);
}

int32_t TRANS_CltpPktProc(SDF_Buff_S *buff);
uint32_t TRANS_CltpHeaderBuild(uint16_t srcPort, uint16_t dstPort, SDF_Buff_S *buff);

#ifdef __cplusplus
}
#endif

#endif