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
 * this file contains  host stack proc for sle connect access dli defines
 *
 ***************************************************************************/

#ifndef SLE_ACCESS_DLI_H
#define SLE_ACCESS_DLI_H

#include <stdint.h>
#include "sdf_addr.h"
#include "dli_cmd_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  星闪接入层逻辑链路创建扩展
 */
typedef struct {
    uint8_t bitFrameType; /* 连接帧类型 */
} CM_AccessConnectParamReq_S;

/**
 * @brief  星闪接入层逻辑链路创建
 */
typedef struct {
    uint8_t version;      /* 原语版本号，当前标准版本设置为0 */
    uint16_t localIndex;  /* 本地索引，在连接管理功能单元和星闪接入层之间可以索引到一条信令 */
    uint8_t tcid;         /* 星闪设备本侧传输通道标识 */
    uint16_t lcid;        /* 星闪设备逻辑链路Id */
    SLE_Addr_S peerAddr;  /* 对端星闪设备的媒体接入层标识 */
    CM_AccessConnectParamReq_S connectParam; /* 星闪接入层逻辑链路创建扩展参数 */
    uint8_t discReason;   /* 断连原因 */
} CM_AccessParamReq_S;

/**
 * @brief  星闪接入层逻辑链路创建响应扩展
 */
typedef struct {
    uint8_t reserv; /* 创建响应扩展保留参数 */
} CM_AccessRspExt_S;

/**
 * @brief  星闪接入层逻辑链路创建响应
 */
typedef struct {
    uint8_t role;              /* 链路角色：G或者T */
    uint8_t version;           /* 原语版本号，当前标准版本设置为0 */
    uint16_t localIndex;       /* 本地索引，在连接管理功能单元和星闪接入层之间可以索引到一条信令 */
    SLE_Addr_S peerAddr;       /* 对端星闪设备的媒体接入层标识 */
    uint8_t result;            /* 创建结果 */
    uint8_t discReason;        /* 链路断链原因 */
    uint16_t lcid;             /* 星闪设备逻辑链路Id */
    uint8_t connCompleteType;  /* 连接完成类型，0：通过本端扫描建立的连接，1：通过本端广播建立的连接 */
    uint8_t advHandle;         /* 广播标识，若是通过本端广播建立的连接，则为对应启动广播标识，否则默认为0，无效值 */
    CM_AccessRspExt_S *reserv; /* 创建链路其他扩展参数 */
} CM_AccessRspParam_S;

void SleAccessRegCbks(void);

void SleAccessUnRegCbks(void);

/**
 * @brief SLE接入管理层建立SLE逻辑链路接口
 * @param [in] linkParam: GLE建立逻辑链路参数
 * @return SUCCESS: 成功, OTHERS: 失败
 */
uint32_t SleAccessLinkEstablishReq(CM_AccessParamReq_S *linkParam);

/**
 * @brief SLE接入管理层释放SLE逻辑链路接口
 * @param [in] linkParam:释放SLE逻辑链路参数
 * @return SUCCESS: 成功, OTHERS: 失败
 */
uint32_t SleAccessLinkReleaseReq(CM_AccessParamReq_S *linkParam);

/**
 * @brief SLE接入管理层读取对端特性
 * @param [in] handle:逻辑链路标识
 * @return SUCCESS: 成功, OTHERS: 失败
 */
uint32_t SleAccessReadRemoteFeatures(uint16_t handle);

/**
 * @brief SLE接入管理层读取对端版本
 * @param [in] handle:逻辑链路标识
 * @return SUCCESS: 成功, OTHERS: 失败
 */
uint32_t SleAccessReadRemoteVersion(uint16_t handle);

/**
 * @brief SLE接入管理层设置Phy
 * @param [in] param:设置Phy参数
 * @return SUCCESS: 成功, OTHERS: 失败
 */
uint32_t SleAccessSetPhy(DLI_SetPhyParam *param);
#ifdef __cplusplus
}
#endif

#endif