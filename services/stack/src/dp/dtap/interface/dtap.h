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
 * this file contains dtap module definition and relatd APIs.
 *
 * dtap: data transmission and adaptation，数据传输与适配
 *
 ***************************************************************************/

#ifndef DTAP_H
#define DTAP_H

#include <stdint.h>

#include "sdf_buff.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DTAP_MAX_PAYLOAD_LEN 32768U // 32K

enum {
    DTAP_PI_NONE = 0,  // 保留
    DTAP_PI_IPV4,      // IPv4
    DTAP_PI_IPV6,      // IPv6
    DTAP_PI_LWCLTP,    // Light Weight Connectionless-mode Transport Protocol
    DTAP_PI_LWCTP,     // Light Weight Connection-mode Transport Protocol
    DTAP_PI_WNP,       // Wireless Network Protocol(WNP， 无线网络协议)
    DTAP_PI_WAP,       // Wireless Adjacency protocol(WAP，无线邻居协议)

    DTAP_PI_MAX,
};

typedef enum {
    DTAP_PRIORITY_FRAGMENT = 0, // 必须为0，用于数组下标表示，每个lcid下只有一个fragmentChannel
    DTAP_PRIORITY_CMD,
    DTAP_PRIORITY_HIGH,
    DTAP_PRIORITY_NORMAL,
    DTAP_PRIORITY_MAX
} DTAP_ChannelPriority;

typedef struct DTAP_Data {
    uint8_t pi;         // 上层协议指示
    uint16_t lcid;      // 逻辑链路handle
    uint8_t tcid;       // 传输通道tcid
    SDF_Buff_S *buff;   // 待发送数据
} DTAP_Data_S;

typedef struct DTAP_Data_Info {
    uint8_t pi;         // 上层协议指示
    uint16_t lcid;      // 逻辑链路handle
    uint8_t tcid;       // 本端传输通道tcid
} DTAP_Data_Info_S;

/**
 * @brief  初始化dtap模块
 * @param  无
 * @return DTAP_SUCCESS: 成功
 */
uint32_t DTAP_Init(void);

/**
 * @brief  去初始化dtap模块
 * @param  无
 * @return 无
 */
void DTAP_DeInit(void);

/**
 * @brief  DTAP数据发送接口，发送成功无须释放buff，发送失败需要释放buff
 * @param  [in] data 待发送数据
 * @return DTAP_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DTAP_DataSend(DTAP_Data_S *data);

/**
 * @brief  DTAP数据接收接口
 * @param  [in] lcid 接收数据的lcid
 * @param  [in] buf 数据接收buf
 * @return 无
 */
void DTAP_DataRecv(uint16_t lcid, SDF_Buff_S *buf);

/**
 * @brief DTAP数据接收回调函数，无须释放buff
 * @param [in] info 报文信息，见DTAP_Data_Info_S
 * @param [in] buff 数据缓冲区
 */
typedef int (*DTAP_DataRecvCb)(DTAP_Data_Info_S *info, SDF_Buff_S *buff);

/**
 * @brief  注册DTAP固定传输通道数据接收回调函数，需保证在协议栈初始化中且在DTAP模块初始化后调用
 * @param  [in] tcid
 * @param  [in] cb 数据接收回调函数
 * @return DTAP_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DTAP_RegisterDataRecvCb(uint8_t tcid, DTAP_DataRecvCb cb);

/**
 * @brief  取消注册DTAP固定传输通道数据接收回调函数，需保证在协议栈反初始化中且在DTAP模块反初始化前调用
 * @param  [in] tcid
 * @return DTAP_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DTAP_UnregisterDataRecvCb(uint8_t tcid);

/**
 * @brief  注册DTAP上层协议数据接收回调函数
 * @param  [in] pi 上层协议指示
 * @param  [in] cb 数据接收回调函数
 * @return DTAP_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DTAP_RegisterProtoRecvCbk(uint8_t pi, DTAP_DataRecvCb cbk);

/**
 * @brief  取消注册DTAP上层协议数据接收回调函数
 * @param  [in] pi 上层协议指示
 * @return DTAP_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DTAP_UnregisterProtoRecvCbk(uint8_t pi);

/**
 * @brief 传输层发送数据的回调函数
 *
 * @param [in] lcid 逻辑通道Id
 * @param [in] tcid 传输通道Id
 * @param [in] status 传输通道状态
 */
typedef void (*DTAP_TransChannelStatusChangeCbk)(uint16_t lcid, uint16_t tcid, uint8_t status);

typedef struct DTAP_Data_Send_Cbks_S {
    DTAP_TransChannelStatusChangeCbk transChannelStatusChangeCbk;  // 发送数据的回调函数
} DTAP_Data_Send_Cbks_S;

void DTAP_UnRegisterDataSendCbks(void);
void DTAP_RegisterDataSendCbks(const DTAP_Data_Send_Cbks_S *cbks);
void DTAP_ChannelSetStatus(uint16_t lcid, uint8_t tcid, uint16_t result);

#ifdef __cplusplus
}
#endif
#endif