/****************************************************************************
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
****************************************************************************/

/****************************************************************************
 *
 * this file contains the CM signaling struct definitions
 *
 ***************************************************************************/

#ifndef CM_SIGNALING_STRUCT_H
#define CM_SIGNALING_STRUCT_H

#include <stdint.h>
#include "sdf_addr.h"
#include "sdf_buff.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CM_CAP_ENABLE 1
#define CM_CAP_DISABLE 0
#define CM_CAP_NUM 6

#define CM_SIGNALING_SLQI_MAX_NUM 8U

/**
 * @brief 传输模式比特位信息
 */
typedef struct {
    uint16_t reliableMode : 1;
    uint16_t flowMode : 1;
    uint16_t byPassMode : 1;
    uint16_t reverse : 13;
} CM_TransMode;

typedef struct {
    uint32_t relayCap : 1;       /* 1bit支持中继能力查询 */
    uint32_t transMode : 1;      /* 1bit支持传输模式查询 */
    uint32_t measurementCap : 1; /* 1bit支持测量能力查询 */
    uint32_t accessSle : 1;      /* 1bit支持SLE接入能力查询 */
    uint32_t accessSlb : 1;      /* 1bit支持SLB接入能力查询 */
    uint32_t mtu : 1;            /* 1bit支持最大Mtu查询 */
    uint32_t mps : 1;            /* 1bit支持最大MPS查询 */
    uint32_t version : 1;        /* 1bit支持版本信息查询 */
    uint32_t wnd : 1;            /* 1bit支持可靠模式传输通道接收队列长度查询 */
    uint32_t reverse : 23;       /* 23bit预留 */
} CM_CapabilityBitmap_S;

typedef struct {
    CM_CapabilityBitmap_S capBitMap;
    CM_TransMode transMode;        /* 一个位图，对应位置1表示支持的传输模式 */
    uint16_t mtu;                  /* 支持的最大Mtu */
    uint16_t mps;                  /* 支持的最大MPS查询 */
    uint16_t version;              /* 支持的版本信息 */
    uint8_t wnd;                   /* 支持的可靠模式传输通道接收队列长度 */
    uint8_t accessSle[CM_CAP_NUM]; /* 支持的SLE接入能力 */
    uint8_t accessSlb[CM_CAP_NUM]; /* 支持的SLB接入能力 */
} CM_CapabilityValue_S;

typedef enum : uint8_t {
    CM_RESULT_ESTABLISH_SUCCESS = 0, /* 传输通道建立成功 */
    CM_RESULT_INSUFFICIENT_RESOURCE, /* 无资源 */
    CM_RESULT_UNSUPPORTED_MTU_SIZE,  /* MTU大小不支持 */
    CM_RESULT_UNSUPPORTED_MPS_SIZE,  /* MPS大小不支持 */
    CM_RESULT_UNEXPECTED_SRC_TCID,   /* srcTcid不符合预期 */
    CM_RESULT_ESTABLISH_TIMEOUT,     /* 传输通道建立超时 */
} CM_TransChanEstablishRspResult_E;

typedef enum : uint8_t {
    CM_RESULT_RELEASE_SUCCESS = 0, /* 传输通道释放成功 */
    CM_RESULT_RELEASE_TIMEOUT,     /* 传输通道释放超时 */
} CM_TransChanReleaseRspResult_E;

typedef struct {
    uint16_t flushTimeout;   /* 丢弃定时器超时时间，单位为毫秒（ms） */
    uint16_t reorderTimeout; /* 重排序定时器超时时间，单位为毫秒（ms） */
    uint16_t crcInit;        /* crc种子 */
} __attribute__((packed)) CM_TransModeStreamConfig_S;

typedef struct {
    uint8_t txWindow;        /* 发送队列的长度，单位为PDU个数  */
    uint8_t maxTxThreshold;  /* 最大重传输次数 */
    uint16_t retransTimeout; /* 重传定时器超时时间，单位为毫秒（ms） */
    uint16_t rspTimeout;     /* 应答定时器超时时间，单位为毫秒（ms） */
    uint16_t reorderTimeout; /* 重排序定时器超时时间，单位为毫秒（ms） */
    uint16_t crcInit;        /* crc种子 */
} __attribute__((packed)) CM_TransModeReliableConfig_S;

typedef struct {
    uint8_t transMode : 4; /* 传输模式。0：基础模式，1:透传模式，2：流模式，3：可靠模式，其它：保留 */
    uint8_t rfu : 4; /* 预留 */
    uint16_t
        mtu; /* 基础服务层可以处理的最大SDU大小，发送端上层下发超过MTU的SDU无法处理，
                     接收端收到发送端传来的超过MTU的SDU也无法处理。*/
    uint16_t
        mps; /* 基础服务层可以处理的最大PDU大小，发送端基础服务层处理之后的PDU不应超过该大小，
                     接收端星闪接入层传给基础服务层的PDU不应超过该大小 */
} __attribute__((packed)) CM_TransModeCommonConfig_S;

typedef struct {
    uint8_t slqiNum : 4; /* 用于指示preferredSlqiList对应的SLQI列表数量，取值范围1~8，0为无效值，
                            取值0时此报文无效丢弃，其他取值保留。 */
    uint8_t rfu : 4;     /* 预留 */
    uint8_t slqi[0];     /* 星闪QoS参数索引 */
} __attribute__((packed)) CM_PreferredSlqiList_S;

typedef struct {
    uint16_t pc : 1;   /* 表示帧结构中有无扩展字段port config，见CM_PortConfig_S */
    uint16_t mc : 1;   /* 表示帧结构中有无扩展字段multi lc config */
    uint16_t lc : 1;   /* 表示帧结构中有无扩展字段lc config，见CM_LcConfig_S */
    uint16_t rfu : 13; /* 预留 */
} __attribute__((packed)) CM_TransChanEstablishReqExt_S;

typedef struct {
    uint16_t dstPort; /* 目的端口 */
    uint16_t srcPort; /* 源端口 */
    uint16_t aid;     /* 基础应用层业务标识AID */
} __attribute__((packed)) CM_PortConfig_S;

typedef struct {
    uint8_t multiLcMode; /* 用于指示融合模式 */
    uint8_t primaryPath; /* 用于分流传输和冗余传输，指示数据传输的主逻辑链路 */
    uint8_t
        splitThreshold; /* 分流传输场景，数据传输与适配功能单元里对应传输通道缓存的数据包大小超过splitThreshold，则开始分流传输 */
} __attribute__((packed)) CM_MultiLcConfig_S;

typedef struct {
    uint16_t lcid;              /* 逻辑链路标识 */
    uint8_t addr[SLE_ADDR_LEN]; /* 对端星闪设备的媒体接入层标识 */
} __attribute__((packed)) CM_LcInfo_S;

typedef struct {
    uint8_t lcNum; /* 逻辑链路数量，当lcNum取值大于1时指示该传输通道可以在多个逻辑链路上进行传输，取值范围0~8 */
    CM_LcInfo_S lcInfos[0]; /* 传输通道映射的逻辑链路信息列表 */
} __attribute__((packed)) CM_LcConfig_S;

typedef struct {
    uint8_t srcTcid;      /* 本端星闪设备期望建立的传输通道在本端的传输通道标识 */
    uint8_t optionOffset; /* 用于指示扩展字段相对信令data起始位置的偏移值 */
    uint8_t
        exclusive : 1; /* 指示要求为当前业务数据建立一对一独享的传输通道，取值为0时，指示为当前业务建立共享传输通道 */
    uint8_t
        measure : 1; /* 指示srcTcid对应传输通道性能测量不使能，取值为1，指示使能该srcTcid对应传输通道性能测量，默认不使能 */
    uint8_t rfu : 6; /* 预留 */
} __attribute__((packed)) CM_TransChanEstablishReqPkt_S;

typedef struct {
    uint8_t srcTcid; /* 本端星闪设备请求释放的传输通道在本端的传输通道标识 */
    uint8_t dstTcid; /* 本端星闪设备请求释放的传输通道在对端的传输通道标识 */
    uint8_t result;  /* 传输通道建立的结果，见CM_TransChanEstablishRspResult_E */
    uint8_t lc : 1;  /* 表示帧结构中无扩展字段lcConfig，取值为1，表示有扩展字段lcConfig */
    uint8_t rfu : 7; /* 预留 */
} __attribute__((packed)) CM_TransChanEstablishRspPkt_S;

typedef struct {
    uint8_t srcTcid; /* 本端星闪设备请求释放的传输通道在本端的传输通道标识 */
    uint8_t dstTcid; /* 本端星闪设备请求释放的传输通道在对端的传输通道标识 */
} __attribute__((packed)) CM_TransChanReleaseReqPkt_S;

typedef struct {
    uint8_t srcTcid; /* 本端星闪设备请求释放的传输通道在本端的传输通道标识 */
    uint8_t dstTcid; /* 本端星闪设备请求释放的传输通道在对端的传输通道标识 */
    uint8_t result; /* 传输通道连接释放结果，0：释放成功，其它：预留，见CM_TransChanReleaseRspResult_E */
} __attribute__((packed)) CM_TransChanReleaseRspPkt_S;

// 基础服务层通用控制信令code值及其含义
typedef enum {
    ERROR_RSP = 0x00,     /* 异常响应 */
    CAPABILITY_REQ = 0x1, /* 连接管理能力查询请求 */
    CAPABILITY_RSP = 0x2, /* 连接管理能力查询响应 */

    TC_CONNECT_REQ = 0x10,    /* 传输通道建立请求 */
    TC_CONNECT_RSP = 0x11,    /* 传输通道建立响应 */
    TC_DISCONNECT_REQ = 0x12, /* 传输通道释放请求 */
    TC_DISCONNECT_RSP = 0x13, /* 传输通道释放响应 */
    TC_RECONFIG_REQ = 0x14,   /* 传输通道重配请求 */
    TC_RECONFIG_RSP = 0x15,   /* 传输通道重配响应 */

    RELAY_TC_CONNECT_REQ = 0x20,    /* 中继业务通道建立请求 */
    RELAY_TC_CONNECT_RSP = 0x21,    /* 中继业务通道建立响应 */
    RELAY_TC_DISCONNECT_REQ = 0x22, /* 中继业务通道释放请求 */
    RELAY_TC_DISCONNECT_RSP = 0x23, /* 中继业务通道释放响应 */
    RELAY_TC_RECONFIG_REQ = 0x24,   /* 中继业务通道重配请求 */
    RELAY_TC_RECONFIG_RSP = 0x25,   /* 中继业务通道重配响应 */

    QOS_ESTABLISH_REQ = 0x30, /* Qos建立请求 */
    QOS_ESTABLISH_RSP = 0x31, /* Qos建立响应 */
    QOS_MEAS_REQ = 0x32,      /* Qos测量请求 */
    QOS_MEAS_RSP = 0x33,      /* Qos测量响应 */
    QOS_RECONFIG_REQ = 0x34,  /* Qos重配请求 */
    QOS_RECONFIG_RSP = 0x35,  /* Qos重配响应 */
    QOS_RELEASE_REQ = 0x34,   /* Qos释放请求 */
    QOS_RELEASE_RSP = 0x35,   /* Qos释放响应 */

    MEASURE_STARTUP_REQ = 0x40,         /* 测量实例建立请求 */
    MEASURE_STARTUP_RSP = 0x41,         /* 测量实例建立响应 */
    MEASURE_STARTUP_RSP_CONFIRM = 0x42, /* 测量实例建立回复确认 */
    MEASURE_RELEASE_REQ = 0x43,         /* 测量实例释放请求 */
    MEASURE_RELEASE_RSP = 0x44,         /* 测量实例释放回复 */
    MEASURE_NOTICE = 0x45,              /* 测量实例结果反馈 */
    MEASURE_NOTICE_CONFIRM = 0x46,      /* 测量实例结果反馈应答 */
} CM_SignalingCode_S;

// 基础服务层通用控制信令head
typedef struct {
    uint8_t code;
    uint8_t identifier;
    uint16_t length;
    uint8_t data[0];
} CM_SignalingHead_S;

/**
 * @brief 连接管理能力
 */
enum {
    CM_SIGNAL_RELAY_CAPABILITY = 0,
    CM_SIGNAL_TRANS_MODE,
    CM_SIGNAL_MEASURE_CAPABILITY,
    CM_SIGNAL_ACCESS_SLE,
    CM_SIGNAL_ACCESS_SLB,
    CM_SIGNAL_MTU,
    CM_SIGNAL_MPS,
    CM_SIGNAL_VERSION,
    CM_SIGNAL_WND,
};

typedef uint32_t (*CM_SignalingHandle)(uint16_t lcid, CM_SignalingHead_S *pkt);

#define CM_GET_SIGNALING_HEAD(__buff) ((CM_SignalingHead_S *)SDF_DataOffset((__buff)))
SDF_Buff_S *CM_CreateSignalingBuff(uint8_t code, uint8_t identifier,
    uint8_t *data, uint16_t length);
CM_SignalingHead_S *CM_ParseSignalingBuff(SDF_Buff_S *buf);
uint32_t CM_SendBuffToDtap(uint16_t lcid, SDF_Buff_S *buff);
#ifdef __cplusplus
}
#endif
#endif // CM_SIGNALING_STRUCT_H