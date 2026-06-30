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
#ifndef ACTM_API_TYPE_H
#define ACTM_API_TYPE_H

#include <stdbool.h>
#include "sdf_addr.h"
#include "actm_l2hc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ACTM_AVAILABLE_BITRATE_MAX 8

typedef enum {
    NLSTK_ACTM_SOURCE_POINT = 0b01,
    NLSTK_ACTM_SINK_POINT = 0b10,
    NLSTK_ACTM_ALL_POINT = 0b11,
} NLSTK_ActmPointType_E;

typedef enum {
    NLSTK_ACTM_UNICAST = 0x0,
    NLSTK_ACTM_MULTICAST,
} NLSTK_ActmCommType_E;

typedef enum {
    NLSTK_ACTM_STREAM_STOP = 0,
    NLSTK_ACTM_STREAM_TRANS,
} NLSTK_ActmStreamOpcode_E;

typedef enum {
    NLSTK_ACTM_EVENT_READ = 0,
    NLSTK_ACTM_EVENT_OPEN,
    NLSTK_ACTM_EVENT_CONFIG,
    NLSTK_ACTM_EVENT_TRANS,
    NLSTK_ACTM_EVENT_STOP,
    NLSTK_ACTM_EVENT_RELEASE,
    NLSTK_ACTM_EVENT_DISCONNECTED,
    NLSTK_ACTM_EVENT_BITRATE,
    NLSTK_ACTM_EVENT_CREATE_STREAM,
    NLSTK_ACTM_EVENT_DELETE_STREAM,
    NLSTK_ACTM_EVENT_UPDATE_LABLEID,
} NLSTK_ActmEventType_E;

typedef enum {
    NLSTK_ACTM_SUCCESS = 0,
    NLSTK_ACTM_OP_ERROR = 0X80,
    NLSTK_ACTM_SSAP_ERROR,
    NLSTK_ACTM_NOT_FIND_DEVICE,
    NLSTK_ACTM_NO_ENOUGH_POINT,
    NLSTK_ACTM_QOSM_ERROR,
    NLSTK_ACTM_CREATE_STREAM_FAIL,
    NLSTK_ACTM_NOT_FIND_STREAM,
} NLSTK_ActmConfigResult_E;

typedef enum {
    NLSTK_ACTM_DIRECTION_UNCONFIG = 0b00,
    NLSTK_ACTM_DIRECTION_DOWN = 0b01,
    NLSTK_ACTM_DIRECTION_UP = 0b10,
    NLSTK_ACTM_DIRECTION_BOTH = 0b11,
} NLSTK_ActmDirection_E;

typedef struct {
    uint8_t version;
    uint8_t rateConf;
    uint8_t depthConf;
    uint8_t channelConf;
    uint8_t frameConf;
    uint8_t bpsConf;
    uint64_t bpsRange;
} NLSTK_L2HCConfig_S;

typedef struct {
    uint8_t codecId;    // 编解码器标识, 0x0: PCM(暂不支持), 0x1: L2HC
    uint16_t companyId; // 厂商标识
    uint16_t vendorId;  // 厂商编解码器标识
    union {
        NLSTK_L2HCConfig_S l2hc;
    };
} NLSTK_ActmCodecConfig_S;

typedef struct {
    uint8_t comm;  /* 通信方式, 0x0: 单播, 0x1: 数据组播 */
    uint8_t trans; /* 透传方式, 0x0: 不透传, 0x2: SLE透传 */
    uint8_t qosId; /* qos参数索引 */
} NLSTK_ActmChannelConfig_S;

typedef struct {
    uint8_t pointType; // 访问点类型，参考NLSTK_ActmPointType_E
    NLSTK_ActmCodecConfig_S codec;     // 编解码器参数
    NLSTK_ActmChannelConfig_S channel; // 数据通道要求
    uint32_t streamType; // 音频流类型
    uint8_t duration;    // 音频流持续时长，0x01: 长时间，0x02: 短时间
    uint8_t mediaId;     // 媒体实例标识
    uint16_t src;        // 源端口号，仅透传方式为不透传时配置
    uint16_t dst;        // 目标端口号，仅透传方式为不透传时配置
} NLSTK_ActmConfig_S;

typedef struct {
    uint8_t codecId;    // 编解码器标识, 0x0: PCM(暂不支持), 0x1: L2HC
    uint16_t companyId; // 厂商标识
    uint16_t vendorId;  // 厂商编解码器标识
    union {
        NLSTK_L2HCParam_S l2hc;
    };
} NLSTK_ActmCodecParam_S;

typedef struct {
    uint8_t codecNum;
    NLSTK_ActmCodecParam_S *codec;
    uint8_t comm; /* 通信方式, 0x0: 单播, 0x1: 数据组播 */
    uint32_t supportType; // 支持的音频流类型
} NLSTK_ActmAbility_S;

typedef struct {
    uint8_t pointType; // 访问点类型，参考NLSTK_ActmPointType_E
    NLSTK_ActmAbility_S ability; // 音频能力，参考NLSTK_ActmAbility_S
    uint32_t acceptType; // 可接收的音频流类型
} NLSTK_ActmProp_S;

typedef struct {
    uint8_t qosIndex;
    uint8_t codecId;
    uint8_t version;
    uint8_t frame; // 帧长ms
    uint8_t bitSamp; // 位宽
    uint8_t channelMode; // 通道类型
    uint8_t linkCnt; // 链路数量
    uint8_t sca; // 睡眠时钟精度
    uint8_t packing; // 当前仅支持交叉
    uint8_t framing; // 当前仅支持未切分
    uint8_t ft; // G到T方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF]
    uint8_t rtn; // G到T每一个IMB数据PDU重传次数
    uint8_t nse; // 0x01-0x1F 在IMG中每一个IMB每个间隔内的子事件个数
    uint8_t bn; // G到T方向上每一个IMB每个间隔内传输新ICB数据包的个数
    uint8_t phy; // 0:1M, 1:2M, 2:4M
    uint8_t mcs; // G到T方向上使用的调制方式和polar编码
    uint8_t pilot; // 0x00-0x07 标识Polar编码时插导频pilot的比例，代表[数据:导频]=[2^ratio:1]
    uint8_t frameType; // 无线电帧类型
    uint16_t companyId; // 厂商标识
    uint16_t vendorId;  // 厂商编解码器标识
    uint16_t bps;  // 码率kbps
    uint16_t gHandle;    // 组播组句柄
    uint16_t connHandle; // 链路句柄
    uint16_t sduInterval; // sdu间隔
    uint16_t maxSdu; // sdu最大长度
    uint16_t maxPdu; // pdu最大长度
    uint16_t bufNum; // 同步链路buffer数
    uint16_t maxLatency; // 最大传输延迟，ms
    uint16_t icbInterval; // 连续两个IMB Anchor Point的间隔时间，取值范围[0x0014,0x3E80]，时间 = N * 0.25ms，时间范围[5ms,4s]
    uint32_t rate; // 采样率Hz
} NLSTK_ActmQosmInfo_S;

typedef struct {
    uint16_t groupId;
    uint8_t direction;
    uint8_t labelId;
    uint16_t downBitrate;
    uint16_t upBitrate;
    uint8_t qosIndex;
    uint8_t qosLevel;
    uint8_t dutyCycle;
    uint8_t availableBitratesCnt;
    uint16_t availableBitrates[ACTM_AVAILABLE_BITRATE_MAX];
} NLSTK_ActmBitrateChange_S;

#define NLSTK_MAX_LINK_CNT 16
typedef struct {
    uint8_t qosId;
    uint8_t linkCnt;
    SLE_Addr_S addr[NLSTK_MAX_LINK_CNT];
    uint8_t labelId;
    uint8_t qosIndex;
    uint8_t direction;
    uint16_t downwardBitrate;
    uint16_t upwardBitrate;
    uint8_t msgType;
    uint32_t result;
} NLSTK_ActmAutoRateSendMsg_S;

typedef struct {
    uint8_t streamId;
    uint8_t pointType;
    uint8_t commType;
} NLSTK_ActmStreamInfo_S;

typedef struct {
    uint8_t pointType;
    uint8_t commType;
} NLSTK_ActmStreamParam_S;

typedef struct {
    uint8_t qosId;
    uint8_t labelId;
    uint8_t qosIndex;
    uint8_t msgType;
    uint32_t result;
} NLSTK_ActmAutoRateRecvMsg_S;

typedef void (*NLSTK_ActmEventCbk)(SLE_Addr_S *addr, uint8_t eventType, uint8_t result, void *param);

typedef void (*NLSTK_ActmPropCbk)(SLE_Addr_S *addr, uint8_t num, NLSTK_ActmProp_S *prop);

typedef void (*NLSTK_ActmBitrateCbk)(NLSTK_ActmBitrateChange_S *bitrate);

typedef void (*NLSTK_ActmLocationCbk)(SLE_Addr_S *addr, bool isLeft);

typedef void (*NLSTK_ActmStreamTypeCbk)(SLE_Addr_S *addr, uint32_t availableStreamType);

typedef void (*NLSTK_ActmCallBitUpDownCbk)(NLSTK_ActmAutoRateSendMsg_S *upDownParam);

typedef struct {
    NLSTK_ActmEventCbk eventCbk;
    NLSTK_ActmPropCbk propCbk;
    NLSTK_ActmBitrateCbk bitCbk;
    NLSTK_ActmLocationCbk locationCbk;
    NLSTK_ActmStreamTypeCbk streamTypeCbk;
    NLSTK_ActmCallBitUpDownCbk callBitUpDownCbk;
} NLSTK_ActmCbk_S;

#define NLSTK_GROUP_KEY_LEN 16
typedef struct {
    uint8_t cryptAlgo;
    uint64_t giv;
    uint8_t groupKey[NLSTK_GROUP_KEY_LEN];
} NLSTK_ActmImgEncpParam_S;

typedef struct {
    uint32_t groupId;
    uint8_t streamId;
    NLSTK_ActmConfig_S *srcConfig;
    NLSTK_ActmConfig_S *sinkConfig;
    bool isImg;
    NLSTK_ActmImgEncpParam_S encp;
} NLSTK_ActmConfigParam_S;

typedef struct {
    uint8_t streamId;
} NLSTK_ActmOpenParam_S;

typedef struct {
    uint8_t streamId;
    uint8_t op;
} NLSTK_ActmChangeParam_S;

typedef struct {
    uint8_t streamId;
} NLSTK_ActmReleaseParam_S;

typedef struct {
    uint8_t streamId;
    uint64_t bitrate;
} NLSTK_ActmBitrateParam_S;

#ifdef __cplusplus
}
#endif
#endif