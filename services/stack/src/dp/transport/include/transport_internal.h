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
 * this file defines transport layer innner apis.
 *
 ***************************************************************************/

#ifndef TRANSPORT_INTERNAL_H
#define TRANSPORT_INTERNAL_H

#include <stdint.h>

#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TRANS_RESULT_SUCCESS = 0,

    TRANS_RESULT_INTERNAL_FAULT,
    TRANS_RESULT_LOGIC_LINK_NOT_FOUND,
    TRANS_RESULT_INVALID_DATA,
    TRANS_RESULT_CHANNEL_NOT_FOUND,
    TRANS_RESULT_EXCEED_MTU_ERR,
    TRANS_RESULT_TX_CACHE_FULL,
} TRANS_Result_E;   // 数据发送结果，和transport_result_t定义保持一致

typedef enum : uint8_t {
    TRANS_PROTO_CONNECTIONLESS = 0,  // 传输层无连接协议
    TRANS_PROTO_CONNECTION,          // 传输层面向连接协议
} TRANS_Protocol_E;

typedef struct TRANS_Addr {
    SLE_Addr_S devAddr;          // 对端星闪设备地址
    uint8_t tcid;                // 发送时/接收时均为本端tcid
    TRANS_Protocol_E proto;  // 传输层协议类型
    uint16_t srcPort;            // 传输层源端口号
    uint16_t dstPort;            // 传输层目的端口号
} TRANS_Addr_S;

/**
 * @brief 传输层接收数据的回调函数
 *
 * @param [in] addr 传输层地址结构体指针
 * @param [in] data 接收到的数据指针
 * @param [in] len 接收到的数据长度
 */
typedef int (*TRANS_RecvDataCbk)(const TRANS_Addr_S *addr, uint8_t *data, uint16_t len);

/**
 * @brief 传输层发送数据的回调函数
 *
 * @param [in] srcPort 本端portid
 * @param [in] result 数据发送结果，见TRANS_Result_t
 */
typedef void (*TRANS_SendDataCbk)(const SLE_Addr_S *addr, uint8_t tcid, uint16_t srcPort, uint8_t result);

typedef struct TRANS_Cbks_S {
    TRANS_RecvDataCbk recvDataCbk;  // 接收数据的回调函数
    TRANS_SendDataCbk sendDataCbk;  // 发送数据的回调函数
} TRANS_Cbks_S;

/**
 * @brief 注册传输层回调函数
 *
 * @param [in] cbks 传输层回调函数结构体指针
 * @return TRANS_SUCCESS：成功，其他值：失败
 */
uint32_t TRANS_RegisterCbks(const TRANS_Cbks_S *cbks);

/**
 * @brief 注销传输回调函数
 */
void TRANS_UnregisterCbks(void);

/**
 * @brief 传输层发送数据函数
 * @param [in] addr 传输层地址指针
 * @param [in] data 待发送数据指针
 * @param [in] dataLen 待发送数据的长度
 * @return TRANS_SUCCESS：成功，其他值：失败
 */
uint32_t TRANS_SendData(TRANS_Addr_S *addr, const uint8_t *data, uint16_t dataLen);

/**
 * 更改传输通道状态
 * @param addr 对端星闪设备地址
 * @param tcid 对端tcid
 * @param result socket状态
 */
void TRANS_ChannelSetStatus(SLE_Addr_S *addr, uint8_t tcid, uint8_t result);
#ifdef __cplusplus
}
#endif
#endif