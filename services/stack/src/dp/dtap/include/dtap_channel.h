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
 * this file defines dtap channel and relatd APIs.
 *
 ***************************************************************************/

#ifndef DTAP_CHANNEL_H
#define DTAP_CHANNEL_H

#include <stdint.h>

#include "dtap.h"
#include "sdf_dlist.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_TIMER_HANDLE (-1)
#define DTAP_STREAM_FLUSH_TIMEOUT 1000  /* ms */
#define DTAP_STREAM_REORDER_TIMEOUT 1000  /* ms */

typedef enum {
    DTAP_FRAME_UNICAST = 0,
    DTAP_FRAME_MULTICAST,
    DTAP_FRAME_BROADCAST,
} DTAP_CAST_MODE;

typedef struct DTAP_Basic_Channel {
    bool isReady;                                     /* 上层是否准备好接收报文 */
    SDF_DListHead_S cacheRxBuffs;                     /* 缓存报文 */
} DTAP_Basic_Channel_S;

typedef struct DTAP_TxWindow {
    uint8_t size;               /* 窗口大小 */
    uint16_t nextTxSeq;         /* 下一包待发送新传PDU的TxSeq */
    uint16_t expectedAckSeq;    /* 已发送PDU中首包等待ack的PDU的TxSeq */
    SDF_DListHead_S txList;     /* 发送队列，暂定使用链表 */
} DTAP_TxWindow_S;

typedef struct DTAP_RxWindow {
    uint8_t size;               /* 窗口大小 */
    uint16_t bufferSeq;         /* 接收队列的下边界（包含） */
    uint16_t expectedTxSeq;     /* 接收队列中第一个未被接收的PDU的TxSeq */
    uint16_t maxExpectedTxSeq;  /* 接收队列中正确接收的PDU中的最大序列号加1 */
    SDF_DListHead_S rxList;     /* 接收队列 */
} DTAP_RxWindow_S;

typedef struct DTAP_Stream_Channel {
    uint16_t flushTimeout;                            /* 丢弃超时时间 */
    uint16_t reorderTimeout;                          /* 重排序超时时间 */
    uint16_t crcInit;                                 /* crc初始值 */
    uint16_t reorderTxSeq;                            /* 重排定时器txSeq */
    uint16_t expectedSeq;                             /* 第一个未收到的序列号 */
    uint16_t maxExpectedSeq;                          /* 最大的已收到的序列号 + 1 */
    uint16_t frameTxSeq;                              /* 记录发送新传pdu的TxSeq */
    int reorderTimerHandle;                           /* 重排定时器 */
    SDF_DListHead_S txList;                           /* 发送队列 */
    SDF_DListHead_S rxList;                           /* 接收队列 */
    int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *); /* 接收回调 */
} DTAP_Stream_Channel_S;

typedef struct DTAP_ReliableChannel {
    uint16_t reorderTimeout;    /* 重排序定时器超时时间，单位为毫秒（ms） */
    uint16_t crcInit;           /* crc生成种子 */
    uint16_t retransTimeout;    /* 重传定时器超时时间，单位为毫秒（ms） */
    uint16_t rspTimeout;        /* 响应定时器超时时间，单位为毫秒（ms） */
    uint8_t maxTxThreshold;     /* 最大发送次数 */
    int reorderTimer;           /* 重排序定时器 */
    int retransTimer;           /* 重传定时器 */
    int rspTimer;               /* 响应定时器 */
    bool isRecvFrame;           /* 收到对端数据为true，否则为false */
    bool isRxWindowFull;        /* 接收队列已满为true，存在空余为false */
    uint16_t reTxSeq;           /* 重传序列号，标识当前需要重传的PDU的TxSeq */
    DTAP_TxWindow_S txWindow;   /* 发送队列 */
    uint8_t nackCnt;            /* 异常应答发送次数 */
    DTAP_RxWindow_S rxWindow;   /* 接收队列 */
    SDF_Buff_S *frags;          /* 分片报文 */
    uint8_t cachePi;            /* 缓存的pi */
    SDF_Buff_S *cacheBuff;      /* 缓存的分片报文 */
    uint8_t sendPollingCounter; /* 发送轮询计数器 */
    bool isTxWindowFull;        /* 发送队列已满为true，存在空余为false */
    bool isDestroyMsgSent;      /* 是否给对端发送过删除通道信令 */
} DTAP_ReliableChannel_S;

typedef struct DTAP_Channel {
    SDF_DListHead_S pktList;  /* 发包队列，节点的生命周期由dtap_scheduler管理 */
    SDF_DListEntry_S schedEntry;  /* 调度节点 */
    SDF_DListEntry_S entry;   /* 链表节点 */
    DTAP_ChannelPriority priority;
    uint8_t type;             /* 逻辑链路类型：详见DLI_DATATYPE */
    uint16_t lcid;            /* 逻辑链路标识 */
    uint8_t srcTcid;          /* 本设备传输通道标识 */
    uint8_t dstTcid;          /* 对端设备传输通道标识 */
    uint8_t mode;             /* 传输模式，参见DTAP模块定义 */
    uint16_t mtu;             /* MTU，Maximum Transmission Unit，最大传输单元 */
    uint16_t mps;             /* MPS，Maximum Payload Size，最大载荷大小 */
    DTAP_CAST_MODE castMode;  /* 将接入层传输模式转为单播、组播、广播，用于帧的组装和解析 */
    void *attr;               /* 可靠/流模式传输通道属性 */
} DTAP_Channel_S;

DTAP_Channel_S *DTAP_ChannelSearch(uint16_t lcid, uint8_t tcid);
uint32_t DTAP_ChannelInit(void);
void DTAP_ChannelDeInit(void);

#ifdef __cplusplus
}
#endif

#endif