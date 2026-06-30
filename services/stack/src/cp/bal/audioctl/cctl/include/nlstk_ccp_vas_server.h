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
#ifndef NLSTK_CCP_VAS_SERVER_H
#define NLSTK_CCP_VAS_SERVER_H

#include <stdint.h>
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NLSTK_VAS_READ_AUTHEN 0x01    // 认证读取权限
#define NLSTK_VAS_READ_ENCRYPT 0x02   // 加密读取权限
#define NLSTK_VAS_READ_AUTHOR 0x04    // 授权读取权限

typedef enum {
    NLSTK_VAS_STATE_IDLE = 0x00,
    NLSTK_VAS_STATE_ACTIVATED = 0x01,
} NLSTK_CcpVasState_E;

typedef enum {
    NLSTK_VAS_CONTROL_SUCCESS = 0x00,
    NLSTK_VAS_CONTROL_FAIL = 0xFF,
} NLSTK_CcpVasControlResCode_E;

typedef enum {
    NLSTK_CCP_VAS_ACTIVATE = 0x01,
    NLSTK_CCP_VAS_TERMINATE = 0x02,
} NLSTK_CcpVasControlOpCode_E;

typedef void (*NLSTK_CcpVasStartService)(uint32_t errorCode);
typedef void (*NLSTK_CcpVasStateAuthorize)(SLE_Addr_S *addr, uint32_t requestId);

typedef void (*NLSTK_CcpVasActivate)(SLE_Addr_S *addr, uint32_t requestId);
typedef void (*NLSTK_CcpVasTerminate)(SLE_Addr_S *addr, uint32_t requestId);

typedef struct {
    NLSTK_CcpVasActivate activate;
    NLSTK_CcpVasTerminate terminate;
} NLSTK_CcpVasControlPoint_S;

/**
 * @brief 语音助手相关信息结构体
 */
typedef struct {
    uint8_t state;                              /**< 语音助手状态 */
    uint8_t stateRight;                         /**< 语音助手状态操作权限 */
    NLSTK_CcpVasStartService startService;        /**< 语音助手服务创建回调 */
    NLSTK_CcpVasStateAuthorize authorize;         /**< 语音助手授权回调 */
    NLSTK_CcpVasControlPoint_S vasControlPoint;   /**< 语音助手控制点 */
} NLSTK_CcpVasInfo_S;

/**
 * @brief 添加语音助手服务
 *
 * 该函数用于添加语音助手服务，并根据提供的语音助手信息进行初始化。
 *
 * @param[in] vasInfo 语音助手服务信息
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_CcpCreateVoiceAssistantService(NLSTK_CcpVasInfo_S *vasInfo);

/**
 * @brief 通知语音助手控制执行结果
 *
 * 该函数用于在业务处理完 NLSTK_CcpVasControlPoint_S 回调后，通知语音助手控制执行的结果；
 * 若方法调用涉及到语音助手状态的更新，应该先回复方法调用响应，然后再通过 NLSTK_CcpUpdateVasState 更新语音助手状态。
 *
 * @param[in] requestId 请求标识，用于标识具体的请求序号，该标识由 NLSTK_CcpVasControlPoint_S 回调函数传入
 * @param[in] opCode 执行的语音助手控制对应操作码，参见 NLSTK_CcpVasControlOpCode_E 枚举中定义
 * @param[in] errorCode 执行的语音助手控制操作结果，参见 NLSTK_CcpVasControlResCode_E 枚举中定义
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_CcpVasControlResult(uint32_t requestId, uint8_t opCode, uint8_t errorCode);

/**
 * @brief 返回读取语音助手状态属性授权结果
 *
 * 该函数用于用户处理完读取语音助手状态属性授权请求后返回授权结果，语音助手服务根据错误码决定后续操作。
 *
 * @param[in] requestId 请求标识，标识当前操作的唯一请求
 * @param[in] errorCode 错误码，决定后续处理逻辑
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_CcpVasStateAuthorizeResult(uint32_t requestId, uint8_t errorCode);

/**
 * @brief 更新语音助手状态属性
 *
 * 该函数用于用户更新语音助手状态的属性值，若通话控制端订阅了相应属性描述符，SSAP将发送属性变更通知给通话控制端。
 *
 * @param[in] state 语音助手状态，参见 NLSTK_CcpVasState_E 枚举中定义
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_CcpUpdateVasState(uint8_t state);

/**
 * @brief 删除语音助手服务
 *
 * 该函数用于删除语音助手服务，销毁相应的SSAP服务和缓存资源。
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_CcpDeleteVoiceAssistantService(void);

#ifdef __cplusplus
}
#endif
#endif