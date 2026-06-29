/*
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
 */

#ifndef SDF_EXCEP_H
#define SDF_EXCEP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum EXCEP_SsapErrorCode {
    EXCEP_SSAP_PDU_ERROR_RSP_RECV = 0x01,
    EXCEP_SSAP_EXCHANGE_INFO_REQ_SEND,
    EXCEP_SSAP_EXCHANGE_INFO_REQ_RECV,
    EXCEP_SSAP_EXCHANGE_INFO_RSP_SEND,
    EXCEP_SSAP_EXCHANGE_INFO_RSP_RECV,
    EXCEP_SSAP_INIT_FIND_REQ_SEND,
    EXCEP_SSAP_FIND_STRUCTURE_REQ_SEND,
    EXCEP_SSAP_FIND_STRUCTURE_REQ_RECV,
    EXCEP_SSAP_FIND_STRUCTURE_RSP_SEND,
    EXCEP_SSAP_FIND_STRUCTURE_RSP_RECV,
    EXCEP_SSAP_FIND_BY_UUID_REQ_SEND,
    EXCEP_SSAP_FIND_BY_UUID_REQ_RECV,
    EXCEP_SSAP_FIND_BY_UUID_RSP_SEND,
    EXCEP_SSAP_FIND_BY_UUID_RSP_RECV,
    EXCEP_SSAP_READ_REQ_RECV,
    EXCEP_SSAP_READ_RSP_RECV,
    EXCEP_SSAP_READ_BY_UUID_REQ_RECV,
    EXCEP_SSAP_READ_BY_UUID_RSP_RECV,
    EXCEP_SSAP_WRITE_REQ_RECV,
    EXCEP_SSAP_WRITE_RSP_RECV,
    EXCEP_SSAP_WRITE_CMD_RECV,
    EXCEP_SSAP_REQ_TIMEOUT,
} EXCEP_SsapSceneCode_E;

typedef enum EXCEP_SsapSubErrorCode {
    EXCEP_SSAP_INVALID_PDU = 0x01,         // PDU无效
    EXCEP_SSAP_UNSUPPORT_PDU,              // PDU不支持
    EXCEP_SSAP_UNKNOWN,                    // 未知错误
    EXCEP_SSAP_INVALID_HANDLE,             // 句柄无效
    EXCEP_SSAP_NO_RESOURCE,                // 资源不足
    EXCEP_SSAP_FORBID_READ,                // 禁止读取
    EXCEP_SSAP_FORBID_WRITE,               // 禁止写入
    EXCEP_SSAP_UNAUTHENTICATED,            // 客户端未认证
    EXCEP_SSAP_UNAUTHORIZED,               // 客户端未授权
    EXCEP_SSAP_UNENCRYPTED,                // 承载未加密
    EXCEP_SSAP_ITEM_INEXIST,               // 未找到条目
    EXCEP_SSAP_METHOD_ACCESS,              // 方法访问错误
    EXCEP_SSAP_DATA_TYPE,                  // 数据类型错误
    EXCEP_SSAP_DATA_LENGTH,                // 数据值长度错误
    EXCEP_SSAP_DATA_RANGE,                 // 值超出范围
    EXCEP_SSAP_SERVER_FRAG,                // 不支持分包
    EXCEP_SSAP_ITEM_OVER_LIMIT,            // 条目数量超限
    EXCEP_SSAP_PDU_TIMEOUT = 0xAF,         // PDU超时
    EXCEP_SSAP_PDU_DECODE_FAILED,          // PDU解析失败
} EXCEP_SsapSubSceneCode_E;

#ifdef __cplusplus
}
#endif

#endif /* SDF_EXCEP_H */