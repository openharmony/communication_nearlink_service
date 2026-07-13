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
 * this file defines signalings of transport channel and related operations.
 *
 ***************************************************************************/

#ifndef CM_SIGNALING_TRANS_CHANNEL_H
#define CM_SIGNALING_TRANS_CHANNEL_H

#include <stdbool.h>
#include <stdint.h>

#include "cm_signaling_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t reorderTimeout;           /* 重排序定时器超时时间 */
    uint16_t crcInit;                  /* crc种子 */
    uint16_t retransTimeout;           /* 重传定时器超时时间 */
    uint16_t rspTimeout;               /* 应答定时器超时时间 */
    uint8_t txWindow;                  /* 发送队列的长度 */
    uint8_t maxTxThreshold;            /* 最大重传次数 */
} CM_SignalingTransModeReliableConfig_S;

typedef struct {
    uint8_t mode; /* 传输模式。0：基础模式，1:透传模式，2：流模式，3：可靠模式，其它：保留 */
    uint16_t mtu; /* 基础服务层可以处理的最大SDU大小 */
    uint16_t mps; /* 基础服务层可以处理的最大PDU大小 */
} CM_SignalingTransModeCommonConfig_S;

typedef struct {
    uint16_t reorderTimeout;   /* 重排序定时器超时时间 */
    uint16_t crcInit;          /* crc种子 */
    uint16_t flushTimeout;     /* 丢弃定时器超时时间 */
} CM_SignalingTransModeStreamConfig_S;

typedef struct {
    uint16_t srcPort; /* 源端口 */
    uint16_t dstPort; /* 目的端口 */
    uint16_t aid;     /* 基础应用层业务标识AID */
} CM_SignalingPortConfig_S;

typedef struct {
    CM_SignalingTransModeCommonConfig_S commonConfig; /* 传输模式通用配置 */
    union {
        CM_SignalingTransModeStreamConfig_S streamConfig;  /* 流模式配置 */
        CM_SignalingTransModeReliableConfig_S reliableConfig; /* 可靠传输模式配置 */
    };
} CM_SignalingTransModeConfig_S;

typedef struct {
    CM_SignalingPortConfig_S *portConfig; /* 可选，端口、业务配置 */
} CM_SignalingTransChanEstablishExtension_S;

typedef struct {
    uint8_t slqiNum;                         /* 星闪QoS参数索引数目 */
    uint8_t slqi[CM_SIGNALING_SLQI_MAX_NUM]; /* 星闪QoS参数索引 */
} CM_SignalingPreferredSlqiList_S;

typedef struct {
    uint8_t srcTcid;                                     /* 本端传输通道标识 */
    bool exclusive;                                      /* 是否创建独享传输通道 */
    bool measure;                                        /* 是否开启传输通道测量 */
    CM_SignalingTransModeConfig_S transModeConfig;       /* 传输通道配置信息 */
    CM_SignalingPreferredSlqiList_S slqiList;            /* 期望的SLQI列表 */
    CM_SignalingTransChanEstablishExtension_S extension; /* 扩展字段 */
} CM_SignalingTransChanEstablishReq_S;

typedef struct {
    uint8_t srcTcid;         /* 本端传输通道标识 */
    uint8_t dstTcid;         /* 对端传输通道标识 */
    uint8_t result;          /* 传输通道建立的结果，见CM_TransChanEstablishRspResult_E */
    CM_LcConfig_S *lcConfig; /* 可选，逻辑链路配置信息 */
} CM_SignalingTransChanEstablishRsp_S;

typedef struct {
    uint8_t srcTcid; /* 本端传输通道标识 */
    uint8_t dstTcid; /* 对端传输通道标识 */
} CM_SignalingTransChanReleaseReq_S;

typedef struct {
    uint8_t srcTcid; /* 本端传输通道标识 */
    uint8_t dstTcid; /* 对端传输通道标识 */
    uint8_t result;  /* 传输通道释放的结果，见CM_TransChanReleaseRspResult_E */
} CM_SignalingTransChanReleaseRsp_S;

/**
 * @brief 传输通道建立请求回调函数
 * @param lcid 逻辑链路标识
 * @param reqId 请求信令标识
 * @param rsp 传输通道建立的响应信息
 * @return 无
 */
typedef void (*CM_SignalingTransChanEstablishReqCbk)(uint16_t lcid, uint8_t reqId,
                                                     CM_SignalingTransChanEstablishReq_S *req);

/**
 * @brief 传输通道建立响应回调函数
 * @param lcid 逻辑链路标识
 * @param rsp 传输通道建立的响应信息
 * @return 无
 */
typedef void (*CM_SignalingTransChanEstablishRspCbk)(uint16_t lcid, CM_SignalingTransChanEstablishRsp_S *rsp);

/**
 * @brief 传输通道释放请求回调函数
 * @param lcid 逻辑链路标识
 * @param reqId 请求信令标识
 * @param rsp 传输通道释放的响应信息
 * @return 无
 */
typedef void (*CM_SignalingTransChanReleaseReqCbk)(uint16_t lcid, uint8_t reqId,
                                                   CM_SignalingTransChanReleaseReq_S *req);

/**
 * @brief 传输通道释放响应回调函数
 * @param lcid 逻辑链路标识
 * @param rsp 传输通道释放的响应信息
 * @return 无
 */
typedef void (*CM_SignalingTransChanReleaseRspCbk)(uint16_t lcid, CM_SignalingTransChanReleaseRsp_S *rsp);

typedef struct {
    CM_SignalingTransChanEstablishReqCbk establishReqCbk;   // 收到传输通道建立请求回调
    CM_SignalingTransChanEstablishRspCbk establishRspCbk;   // 收到传输通道建立响应回调
    CM_SignalingTransChanReleaseReqCbk releaseReqCbk;   // 收到传输通道释放请求回调
    CM_SignalingTransChanReleaseRspCbk releaseRspCbk;   // 收到传输通道释放响应回调
} CM_SignalingTransChanCbks_S;

/**
 * @brief 注册传输通道信令回调函数
 * @param cbks 回调函数结构体指针
 * @return 返回0表示成功，其他值表示失败
 */
uint32_t CM_SignalingTransChanCbksRegister(const CM_SignalingTransChanCbks_S *cbks);

/**
 * @brief 取消注册传输通道信令回调函数
 */
void CM_SignalingTransChanCbksUnregister(void);

/**
 * @brief 发送传输通道建立请求信令
 * @param lcid 逻辑链路标识
 * @param req 请求结构体指针
 * @return 返回0表示成功，其他值表示失败
 */
uint32_t CM_SignalingTransChanEstablishReqSend(uint16_t lcid, CM_SignalingTransChanEstablishReq_S *req);

/**
 * @brief 发送传输通道建立响应信令
 * @param lcid 逻辑链路标识
 * @param reqId 对应的请求信令标识
 * @param rsp 响应结构体指针
 * @return 返回0表示成功，其他值表示失败
 */
uint32_t CM_SignalingTransChanEstablishRspSend(uint16_t lcid, uint8_t reqId, CM_SignalingTransChanEstablishRsp_S *rsp);

/**
 * @brief 发送传输通道释放请求信令
 * @param lcid 逻辑链路标识
 * @param req 请求结构体指针
 * @return 返回0表示成功，其他值表示失败
 */
uint32_t CM_SignalingTransChanReleaseReqSend(uint16_t lcid, CM_SignalingTransChanReleaseReq_S *req);

/**
 * @brief 发送传输通道释放响应信令
 * @param lcid 逻辑链路标识
 * @param reqId 对应的请求信令标识
 * @param rsp 响应结构体指针
 * @return 返回0表示成功，其他值表示失败
 */
uint32_t CM_SignalingTransChanReleaseRspSend(uint16_t lcid, uint8_t reqId, CM_SignalingTransChanReleaseRsp_S *rsp);

/**
 * @brief 处理传输通道建立请求信令
 * @param lcid 逻辑通道标识符
 * @param pkt 信令结构体指针
 * @return 返回0表示成功，其他值表示失败
 */
uint32_t CM_SignalingTransChanEstablishReqProc(uint16_t lcid, CM_SignalingHead_S *pkt);

/**
 * @brief 处理传输通道建立响应信令
 * @param lcid 逻辑通道标识符
 * @param pkt 信令结构体指针
 * @return 返回0表示成功，其他值表示失败
 */
uint32_t CM_SignalingTransChanEstablishRspProc(uint16_t lcid, CM_SignalingHead_S *pkt);

/**
 * @brief 处理传输通道释放请求信令
 * @param lcid 逻辑通道标识符
 * @param pkt 信令结构体指针
 * @return 返回0表示成功，其他值表示失败
 */
uint32_t CM_SignalingTransChanReleaseReqProc(uint16_t lcid, CM_SignalingHead_S *pkt);

/**
 * @brief 处理传输通道释放请求信令
 * @param lcid 逻辑通道标识符
 * @param pkt 信令结构体指针
 * @return 返回0表示成功，其他值表示失败
 */
uint32_t CM_SignalingTransChanReleaseRspProc(uint16_t lcid, CM_SignalingHead_S *pkt);

#ifdef __cplusplus
}
#endif
#endif  // CM_SIGNALING_TRANS_CHANNEL_H
