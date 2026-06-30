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
#ifndef SSAP_COMMON_H
#define SSAP_COMMON_H

#include "ssap_type.h"
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t appId;
    NLSTK_Errcode_E errorCode;
} SsapAsyncProcessResult_S;

/**
 * @brief 服务发现完成事件回调参数
 */
typedef struct {
    uint8_t opCode;                             // 消息控制码
    uint16_t lcid;                              // 连接句柄
    SLE_Addr_S addr;                            // 对端地址
    uint8_t type;                               // 查找类型
    NLSTK_SsapUuid_S uuid;                        // UUID标识, 缺省0:发现所有服务
    uint8_t errCode;
    uint16_t preFindHandle;
} SSAP_DiscoveryComplete_S;

/**
 * @brief 信息交换完成事件回调参数
 */
typedef struct {
    SLE_Addr_S addr;                            // 对端地址
    uint16_t mtu;                               // 请求mtu
    uint16_t version;                           // 星闪协议版本号
    uint8_t errCode;                            // 错误码
} SSAP_ExchangeComplete_S;

/**
 * @brief 通过uuid读取完成事件回调参数
 */
typedef struct {
    SLE_Addr_S addr;                            // 对端地址
    NLSTK_SsapUuid_S uuid;                           // uuid
    uint8_t dataType;                           // 数据类型
    uint16_t beginHandle;                       // 查找的起始句柄
    uint16_t endHandle;                         // 查找的结束句柄
    uint8_t errCode;                            // 错误码
    SDF_Vector_S *readVals;
} SSAP_ReadByUuidComplete_S;

/**
 * @brief 通过handle读取多个条目完成事件回调参数
 */
typedef struct {
    SLE_Addr_S addr;                            // 对端地址
    uint8_t errCode;                            // 错误码
    SDF_Vector_S *readVals;
} SSAP_ReadByHandleComplete_S;

/**
 * @brief 方法调用结果回调参数
 */
typedef struct SSAP_MethodResult {
    SLE_Addr_S addr;
    uint16_t handle;
    uint8_t errorCode;
    SSAP_LengthValue_S value;
} SSAP_MethodResult_S;

typedef struct SSAP_ServerExchangeInfo {
    uint16_t mtuSize;
    uint16_t version;
} SSAP_ServerExchangeInfo_S;

void SSAP_SetServerExchangeInfo(void *arg);
uint16_t SSAP_GetServerMtu(void);

#ifdef __cplusplus
}
#endif
#endif