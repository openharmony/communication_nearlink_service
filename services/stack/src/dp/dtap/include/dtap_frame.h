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
 * this file defines transmission frame format and related apis.
 *
 ***************************************************************************/

#ifndef DTAP_FRAME_H
#define DTAP_FRAME_H

#include <stdbool.h>
#include <stdint.h>

#include "dtap.h"
#include "dtap_channel.h"
#include "sdf_buff.h"
#include "sdf_dlist.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    DTAP_FRAME_BASIC = 0b0000,         // 基础帧
    DTAP_FRAME_SIMPLEX_AGGR = 0b0001,  // 单向聚合帧
    DTAP_FRAME_SIMPLEX_FRAG = 0b0010,  // 单向分片帧
    DTAP_FRAME_DUPLEX_AGGR = 0b0011,   // 双向聚合帧
    DTAP_FRAME_DUPLEX_FRAG = 0b0100,   // 双向分片帧
    DTAP_FRAME_ACK = 0b0101,           // 应答帧
    DTAP_FRAME_MEAS_REQ = 0b0110,      // 测量请求帧
    DTAP_FRAME_MEAS_RSP = 0b0111,      // 测量应答帧

    DTAP_FRAME_MAX,  // 帧类型数目
};

enum {
    DTAP_SAR_UNSEG = 0b00,
    DTAP_SAR_FIRST = 0b01,
    DTAP_SAR_MID = 0b10,
    DTAP_SAR_LAST = 0b11,
};

#define DTAP_MAX_FRAGMENT_NUM 128

enum {
    DTAP_FRAME_O_BIT = 0b0001,
    DTAP_FRAME_C_BIT = 0b0010,
    DTAP_FRAME_P_BIT = 0b0100,
    DTAP_FRAME_F_BIT = 0b1000,
};

#define DTAP_FRAME_SEQ_INIT         (0xFFFF)
#define DTAP_FRAME_SEQ_MAX          (0x3FFF)
#define DTAP_FRAME_SEQ_SIGN_MASK    (0x2000)

#define DTAP_BASIC_FRAME_BITS_SHIFT 4

typedef struct DTAP_FrameCtx {
    uint8_t (*getFrameType)(void);                              // 获取帧类型
    uint8_t (*getHeaderLen)(void);                              // 获取帧头部固定长度
    uint32_t (*parseFrame)(const struct DTAP_FrameCtx *ctx);    // 解析帧
    bool (*checkFrameHeader)(const struct DTAP_FrameCtx *ctx);  // 校验帧头部
    uint32_t (*buildFrame)(const struct DTAP_FrameCtx *ctx, uint8_t tcid, uint16_t crcInit); // 构建帧
} DTAP_FrameCtx_S;

typedef struct DTAP_Frame {
    SDF_DListEntry_S entry; // 当前暂定使用链表
    SDF_Buff_S *buff;       // 帧数据
    void *header;           // 帧头
    void *extension;        // 扩展字段
    uint8_t *payload;       // 数据信息
    uint8_t bits;           // ocpf指示位集合
    uint8_t pi;             // 上层协议标识
    union {
        struct {
            uint16_t reqSeq;    // 期待接收下一个PDU的TxSeq
            bool sBit;          // S=false为正常应答， S=true为异常应答
            uint32_t res;       // 保证联合体字节对齐
        } ack;
        struct {
            uint8_t reTxCnt;    // 重传次数
            uint8_t sar;        // 分片状态
            uint16_t txSeq;     // 发送PDU的序列号
            uint16_t reqSeq;    // 期待接收下一个PDU的TxSeq
            uint8_t res;        // 保证联合体字节对齐
        } enhance;
    };
    uint8_t headerLen;      // 帧头长度
    uint16_t extensionLen;  // 扩展字段长度
    uint16_t payloadLen;    // 帧负载长度
    DTAP_FrameCtx_S ctx;    // 帧上下文
} DTAP_Frame_S;

typedef struct DTAP_BasicHeader {
    uint8_t tcid;           // 传输通道标识
    union {
        struct {
            uint8_t frameType : 4;  // 帧类型
            uint8_t optionBit : 1;  // option bit, 取值为1表示帧结构中携带扩展字段；取值为0表示帧结构中无扩展字段
            uint8_t crcBit : 1;  // crc16 bit, 取值为1表示帧结构末尾携带2字节CRC校验值；取值为0表示帧结构中无CRC校验值
            uint8_t pBit : 1;  // 当p比特为1，则需要对端立即对该数据帧回复，否则为0（增强帧中使用）
            uint8_t fBit : 1;  // 指示针对p比特的立即回复（增强帧中使用）
        };
        uint8_t typeBits;  // 帧类型和ocpf指示位集合
    };
    uint16_t length;   // 帧长度, 指示此字段后的字节总数
} __attribute__((packed)) DTAP_BasicHeader_S;

/**
 * @brief 基础帧格式定义
 *   0               1               2               3
 *   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     tcid      | 0b0000|o|c|rfu|            length             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  ! pi(optional)  |             extension(variable)               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  !       payload(variable)       |        crc(optional)          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
typedef struct DTAP_BasicFrameHeader {
    DTAP_BasicHeader_S header;  // 基础头
    uint8_t pi[0];  // 协议标识：1字节，指示上层协议的类型。当TCID=0x01~0x1D时为固定传输通道标识，省略PI字段。
} __attribute__((packed)) DTAP_BasicFrameHeader_S;

/**
 * @brief 透传帧格式定义
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  +                                                               +
 *  !                         payload                               |
 *  +                                                               +
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */


/**
 * @brief 单向分片帧格式定义
 *   0               1               2               3
 *   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     tcid      | 0b0010|o|c|p|r|            length             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       pi      |          txSeq            |sar|extension(var) |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       payload(variable)       |              crc              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
typedef struct DTAP_SimplexFragFrameHeader {
    DTAP_BasicHeader_S header;  // 基础头，r位为保留字段
    uint8_t pi;                 // 协议标识：1字节，指示上层协议的类型
    uint16_t txSeq : 14;        // 发送帧序列号
    uint16_t sar : 2;           // 分片状态:
                                // 0b00：未分片，包含一个完整SDU
                                // 0b01：包含SDU第一个分片
                                // 0b10：包含SDU中间分片
                                // 0b11：包含SDU最后一个分片
    uint8_t extension[0];       // 扩展字段，当o=1时存在
} __attribute__((packed)) DTAP_SimplexFragFrameHeader_S;

/**
 * @brief 单向聚合帧格式定义
 *   0               1               2               3
 *   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     tcid      | 0b0001|o|c|p|r|            length             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       pi      |          txSeq            |rfu|extension(var) |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         extension(var)                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |        payload1 length        |       payload1(variable)      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |        payload2 length        |       payload2(variable)      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       payload2(variable)      |              crc              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
typedef struct DTAP_SimplexAggrFrameHeader {
    DTAP_BasicHeader_S header;  // 基础头，r位为保留字段
    uint8_t pi;                 // 协议标识：1字节，指示上层协议的类型
    uint16_t txSeq : 14;        // 发送帧序列号
    uint16_t rfu : 2;           // 保留字段
    uint8_t extension[0];       // 扩展字段，当o=1时存在
} __attribute__((packed)) DTAP_SimplexAggrFrameHeader_S;

/**
 * @brief 双向分片帧格式定义
 *   0               1               2               3
 *   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     tcid      | 0b0100|o|c|p|f|            length             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       pi      |          txSeq            |sar|   reqSeq      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | reqSeq    |rfu|              extension(variable)              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       payload(variable)       |              crc              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
typedef struct DTAP_DuplexFragFrameHeader {
    DTAP_BasicHeader_S header;  // 基础头
    uint8_t pi;                 // 协议标识：1字节，指示上层协议的类型
    uint16_t txSeq : 14;        // 发送帧序列号
    uint16_t sar : 2;           // 分片状态:
                                // 0b00：未分片，包含一个完整SDU
                                // 0b01：包含SDU第一个分片
                                // 0b10：包含SDU中间分片
                                // 0b11：包含SDU最后一个分片
    uint16_t reqSeq : 14;       // 期待接收下一个PDU的TxSeq
    uint16_t rfu : 2;           // 保留字段
    uint8_t extension[0];       // 扩展字段，当o=1时存在
} __attribute__((packed)) DTAP_DuplexFragFrameHeader_S;

/**
 * @brief 双向聚合帧格式定义
 *   0               1               2               3
 *   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     tcid      | 0b0011|o|c|p|f|            length             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       pi      |          txSeq            |rfu|   reqSeq      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | reqSeq    |rfu|              extension(variable)              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |        payload1 length        |       payload1(variable)      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |        payload2 length        |       payload2(variable)      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       payload2(variable)      |              crc              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
typedef struct DTAP_DuplexAggrFrameHeader {
    DTAP_BasicHeader_S header;  // 基础头
    uint8_t pi;                 // 协议标识：1字节，指示上层协议的类型
    uint16_t txSeq : 14;        // 发送帧序列号
    uint16_t rfu1 : 2;          // 保留字段
    uint16_t reqSeq : 14;       // 期待接收下一个PDU的TxSeq
    uint16_t rfu2 : 2;          // 保留字段
    uint8_t extension[0];       // 扩展字段，当o=1时存在
} __attribute__((packed)) DTAP_DuplexAggrFrameHeader_S;

/**
 * @brief 应答帧格式定义
 *   0               1               2               3
 *   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     tcid      | 0b0101|o|c|r|f|            length             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |            reqSeq         |s|r|      extension(variable)      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |      extension(variable)      |              crc              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
typedef struct DTAP_AckFrameHeader {
    DTAP_BasicHeader_S header;  // 基础头，p位为保留字段
    uint16_t reqSeq : 14;       // 期待接收下一个PDU的TxSeq
    uint16_t sBit : 1;          // S=0为正常应答， S=1为异常应答
    uint16_t rfu : 1;           // 保留字段
    uint8_t extension[0];       // 扩展字段，当o=1时存在
} __attribute__((packed)) DTAP_AckFrameHeader_S;

/**
 * @brief 聚合帧SDU定义
 */
typedef struct DTAP_AggregateSdu {
    uint16_t length;    // SDU长度指示
    uint8_t data[0];    // SDU数据载荷
} __attribute__((packed)) DTAP_AggregateSdu_S;

/**
 * @brief 扩展字段格式定义
 *    0   1   2   3   4   5   6   7
 *  +---+---+---+---+---+---+---+---+
 *  |     extension num (8 bit)     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+
 *  !     extension type (8 bit)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+
 *  !   extension length (8 bit)    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+
 *  !   extension value (variable)  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+
 */
typedef struct DTAP_Extension {
    uint8_t type;       // 扩展字段类型
    uint8_t length;     // 扩展字段长度
    uint8_t value[0];   // 扩展字段值
} __attribute__((packed)) DTAP_Extension_S;

typedef struct DTAP_ExtensionHeader {
    uint8_t num;                // 扩展字段数量
    DTAP_Extension_S data[0];   // 扩展字段内容
} DTAP_ExtensionHeader_S;

#define DTAP_CRC_LEN 2
#define DTAP_BASIC_HEADER_LEN (sizeof(DTAP_BasicHeader_S))
#define DTAP_SIMPLEX_FRAG_FRAME_HEADER_LEN (sizeof(DTAP_SimplexFragFrameHeader_S))
#define DTAP_SIMPLEX_AGGR_FRAME_HEADER_LEN (sizeof(DTAP_SimplexAggrFrameHeader_S))
#define DTAP_DUPLEX_FRAG_FRAME_HEADER_LEN (sizeof(DTAP_DuplexFragFrameHeader_S))
#define DTAP_DUPLEX_AGGR_FRAME_HEADER_LEN (sizeof(DTAP_DuplexAggrFrameHeader_S))
#define DTAP_ACK_FRAME_HEADER_LEN (sizeof(DTAP_AckFrameHeader_S))

void DTAP_RegisterBasicFrameCtx(void);
void DTAP_RegisterEnhanceFrameCtx(void);
void DTAP_RegisterFrameCtx(DTAP_FrameCtx_S *dtapFrameCtx);
DTAP_FrameCtx_S *DTAP_GetFrameCtx(uint8_t frameType);
uint32_t DTAP_ParseFrame(uint8_t frameType, SDF_Buff_S *buff, DTAP_Frame_S *dtapFrame);
uint32_t DTAP_SendFrame(DTAP_Channel_S *transChan, SDF_Buff_S *buff);
DTAP_Frame_S *DTAP_CreateFrame(uint8_t frameType);
void DTAP_DestroyFrame(DTAP_Frame_S *frame);
DTAP_Frame_S *DTAP_CopyFrame(const DTAP_Frame_S *srcFrame);
uint32_t DTAP_ParseExtension(DTAP_Frame_S *frame);
uint32_t DTAP_RecvAggregateFrame(DTAP_Frame_S *frame, DTAP_Data_Info_S *info,
    int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *));
uint32_t DTAP_RecvFragmentFrame(SDF_Buff_S **buffs, const DTAP_Frame_S *frame, DTAP_Data_Info_S *info,
    int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *));
void DTAP_FragmentFrame(SDF_Buff_S *buff, uint16_t mps, uint8_t frameType,
    DTAP_Frame_S *outFrames[DTAP_MAX_FRAGMENT_NUM], uint16_t *outCnt);
uint32_t DTAP_SetFrameBit(DTAP_BasicHeader_S *frameHeader, uint8_t bits);
uint32_t DTAP_ReCalculateCrcValue(uint16_t crcInit, DTAP_Frame_S *frame);

static inline uint16_t DTAP_GetFragmentFramesNum(uint16_t len, uint16_t mps)
{
    return mps == 0 ? 1 : (len + mps - 1) / mps;
}

static inline bool DTAP_CheckFrameBit(uint8_t bits, uint8_t bit)
{
    return (bits & bit) != 0;
}

#ifdef __cplusplus
}
#endif
#endif