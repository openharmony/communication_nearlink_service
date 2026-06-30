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
#ifndef SSAPS_SERVER_H
#define SSAPS_SERVER_H

#include <stdint.h>
#include "ssap_type.h"
#include "ssap_pkt.h"
#include "ssap_link.h"
#include "sdf_buff.h"
#include "ssap_link.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct SSAP_ReadResultItem {
    uint16_t handle;
    uint8_t errorCode;
    uint8_t dataType;
    SSAP_LengthValue_S *value;
} SSAP_ReadResultItem_S;

typedef struct SSAP_WriteOriginData {
    uint8_t dataType;
    SSAP_LengthValue_S *value;
} SSAP_WriteOriginData_S;

/**
 * @brief 写入结果项结构体
 * @details 用于存储多写请求的处理结果，包含句柄、错误码、originData有效项数量及原始数据数组
 * @note  originData数组中每个元素通过dataType字段标识类型，数组下标与dataType值无关，
 *        由originDataCount记录有效项数量。遍历时应使用originDataCount而非DESC_TYPE_MAX
 */
typedef struct SSAP_WriteResultItem {
    uint16_t handle;                                 /**< 属性句柄 */
    uint8_t errorCode;                               /**< 错误码，SSAP_ERRCODE_SUCCESS表示成功 */
    uint8_t originDataCount;                         /**< originData有效项数量 */
    SSAP_WriteOriginData_S originData[DESC_TYPE_MAX];
} SSAP_WriteResultItem_S;

/**
 * @brief  接收到的MTU信息交换请求报文处理
 */
void SSAPS_ExchangeInfoReqHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

void SSAPS_FindReqHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  接收到的读取数据请求报文处理
 */
void SSAPS_ReadReqHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  接收到的通过uuid读取数据请求报文
 */
void SSAPS_ReadByUuidReqHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  接收到的带响应写入数据请求报文处理
 */
void SSAPS_WriteReqHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  接收到的无响应写入数据报文处理
 */
void SSAPS_WriteCmdHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  接受到的指示确认报文处理
 */
void SSAPS_ValueAckHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  接受到的无响应方法调用报文处理
 */
void SSAPS_MethodCmdHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  接受到的有响应方法调用报文处理
 */
void SSAPS_MethodReqHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  发送服务数据属性或事件变化通知（无确认）
 */
void SSAPS_ValueNtf(SSAP_Link_S *link, void *arg);

/**
 * @brief  发送服务数据属性或事件变化指示（有确认）
 */
void SSAPS_ValueInd(SSAP_Link_S *link, void *arg);

/**
 * @brief  服务端通过句柄更新本地数据
 */
void SSAPS_UpdateItemValueByHandle(void *arg);

/**
 * @brief 用户返回授权结果
 */
void SSAPS_SendUserResponse(void *arg);

/**
 * @brief 用户返回方法调用结果
 */
void SSAPS_SendMethodCallRes(void *arg);

void SSAPS_SendWriteReqRsp(SSAP_Link_S *link, uint8_t errorCode, SSAP_BufferedOperation_S *operation);

uint8_t SSAPS_UpdatePropertyValue(SSAP_Property_S *property, uint8_t type, SSAP_LengthValue_S *value,
    SLE_Addr_S *addr, bool isRmt);

SSAP_Property_S *SSAPS_GetPropertyByHandle(uint16_t handle);

void SSAPS_PushOperationPenddingVector(SSAP_BufferedOperation_S *operation);

SSAP_BufferedOperation_S *SSAPS_PopOperationPenddingVector(uint16_t requestId, size_t *index);

bool SSAPS_GetPermissionAndOperation(SSAP_Property_S *property, uint8_t *permissions,
    uint32_t *opIndication, uint8_t type);

SSAP_LengthValue_S *SSAPS_GetPropertyValue(SSAP_Link_S *link, SSAP_Property_S *property, uint8_t type,
    uint8_t *errorCode);

#ifdef __cplusplus
}
#endif

#endif