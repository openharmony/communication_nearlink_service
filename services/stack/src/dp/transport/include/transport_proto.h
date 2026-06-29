/**
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
*/

/*********************************************************************************************************
 *
 * this file defines basic header and option fields of transport protocol and related apis.
 *
 *********************************************************************************************************/

#ifndef TRANSPORT_PROTO_H
#define TRANSPORT_PROTO_H

#include <endian.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TRANS_PROTO_VERSION 0x01

#define TRANS_OPT_LEN_MULTYPLY 4

typedef struct TRANS_ProtoBasicHeader {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8_t optionLen : 5;  // 该字段的数值乘以4即为可选字段位图字段、可选字段和填充字段的总字节数量
    uint8_t version : 3;  // 协议版本号的数值为1
#else
    uint8_t version : 3;
    uint8_t optionLen : 5;
#endif
    uint16_t srcPort;  // 源端口号
    uint16_t dstPort;  // 目的端口号
} __attribute__((packed)) TRANS_ProtoBasicHeader_S;

#ifdef __cplusplus
}
#endif

#endif