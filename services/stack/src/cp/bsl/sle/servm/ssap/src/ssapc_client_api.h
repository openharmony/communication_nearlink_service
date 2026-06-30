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
#ifndef SSAPC_CLIENT_API_H
#define SSAPC_CLIENT_API_H

#include "ssap_link.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void SSAP_ExchangeInfoReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback,
    SSAP_ExchangeInfoReqInfo_S *exchangeInfoReqInfo);

void SSAP_FindReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback, SSAP_ParamFind_S *findParam);

void SSAP_FindPriorityReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback, SSAP_ParamFind_S *findParam);

void SSAP_FindByUuidReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback,
    SSAP_ParamFindByUuid_S *findParam);

void SSAP_FindByUuidPriorityReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback,
    SSAP_ParamFindByUuid_S *findParam);
/**
 * @brief  客户端通过句柄和数据类型读取服务端数据
 */
void SSAP_ReadReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback, SSAP_ReadReqInfo_S *readReqInfo);

/**
 * @brief  客户端通过句柄和数据类型读取多个服务端数据
 */
void SSAP_ReadReqProps(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback,
    SSAP_ReadPropsInfo_S *readPropsInfo);

/**
 * @brief  客户端通过UUID和数据类型读取服务端数据
 */
void SSAP_ReadByUuidReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback,
    SSAP_ReadByUuidReqInfo_S *readByUuidReqInfo);

/**
 * @brief  发送无需响应的写入报文
 */
void SSAP_WriteCmd(SSAP_WriteCmdInfo_S *writeCmdInfo);

/**
 * @brief  客户端有响应写入服务端数据
 */
void SSAP_WriteReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback, SSAP_WriteReqInfo_S *writeReqInfo);

/**
 * @brief  数据指示报文确认
 */
void SSAP_ValueAck(SSAP_ValueAckInfo_S *valueAck);

/**
 * @brief  请求调用服务端方法(有响应)
 */
void SSAP_CallMethodReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback,
    SSAP_CallMethodReqInfo_S *callMethodInfo);

/**
 * @brief  请求调用服务端方法(无响应)
 */
void SSAP_CallMethodCmd(SSAP_CallMethodCmdInfo_S *callMethodInfo);

#ifdef __cplusplus
}
#endif

#endif