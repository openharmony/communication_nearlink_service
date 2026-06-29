/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef SSAPC_CLIENT_H
#define SSAPC_CLIENT_H

#include <stdint.h>
#include "ssap_type.h"
#include "ssap_pkt.h"
#include "ssap_link.h"
#include "sdf_buff.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void SSAPC_ExchangeInfoErrorHandle(SSAP_Link_S *link, uint8_t errCode);

void SSAPC_FindReqErrorHandle(SSAP_Link_S *link, uint8_t errCode);

void SSAPC_ValueErrorHandle(SSAP_Link_S *link, uint8_t errCode);

void SSAPC_ReadByUuidErrorHandle(SSAP_Link_S *link, uint8_t errCode);

void SSAPC_CallMethodErrorHandle(SSAP_Link_S *link, uint8_t errCode);

/**
 * @brief  接收到的MTU信息交换响应报文处理
 */
void SSAPC_ExchangeInfoRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

void SSAPC_FindRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  接收到的读取数据响应报文处理
 */
void SSAPC_ReadRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  接收到的通过UUID读取数据响应报文处理
 */
void SSAPC_ReadByUuidRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  接收到的写入数据响应报文处理
 */
void SSAPC_WriteRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  客户端接收到的数据变化通知处理
 */
void SSAPC_ValueNtfHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  客户端接收到的数据变化指示处理
 */
void SSAPC_ValueIndHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  客户端接收到的方法调用结果处理
 */
void SSAPC_CallMethodRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

/**
 * @brief  建链发送第一条报文，确保连接管理能力查询完成，收到的rsp不作处理
 */
void SSAPC_InitFindReq(SSAP_Link_S *link, void *arg);

/**
 * @brief  客户端Mtu信息交互
 */
void SSAPC_ExchangeInfoReq(SSAP_Link_S *link, void *arg);

void SSAPC_FindReq(SSAP_Link_S *link, void *arg);

void SSAPC_FindByUuidReq(SSAP_Link_S *link, void *arg);

/**
 * @brief  客户端通过句柄和数据类型读取服务端数据
 */
void SSAPC_ReadReq(SSAP_Link_S *link, void *arg);

/**
 * @brief  客户端通过句柄和数据类型读取多个服务端数据
 */
void SSAPC_ReadProps(SSAP_Link_S *link, void *arg);

/**
 * @brief  客户端通过UUID和数据类型读取服务端数据
 */
void SSAPC_ReadByUuidReq(SSAP_Link_S *link, void *arg);

/**
 * @brief  发送无需响应的写入报文
 */
void SSAPC_WriteCmd(SSAP_Link_S *link, void *arg);

/**
 * @brief  客户端有响应写入服务端数据
 */
void SSAPC_WriteReq(SSAP_Link_S *link, void *arg);

/**
 * @brief  数据指示报文确认
 */
void SSAPC_ValueAck(SSAP_Link_S *link, void *arg);

/**
 * @brief  请求调用服务端方法(有响应)
 */
void SSAPC_CallMethodReq(SSAP_Link_S *link, void *arg);

/**
 * @brief  请求调用服务端方法(无响应)
 */
void SSAPC_CallMethodCmd(SSAP_Link_S *link, void *arg);

#ifdef __cplusplus
}
#endif

#endif