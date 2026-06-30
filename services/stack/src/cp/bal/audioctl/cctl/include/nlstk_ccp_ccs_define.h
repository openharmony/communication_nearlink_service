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
#ifndef NLSTK_CCP_CCS_DEFINE_H
#define NLSTK_CCP_CCS_DEFINE_H

#include <stdint.h>
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

// 通用的枚举，应该放到SSAP的头文件中
// 用于授权回调 NLSTK_McpAuthorize 表示客户端的具体操作
typedef enum {
    NLSTK_SSAP_PROPERTY_READ = 0x01,
    NLSTK_SSAP_PROPERTY_WRITE = 0x02,
} NLSTK_ServicePropertyOpType_E;

// 每个属性的操作权限，用来设置 NLSTK_CcpCallControlInfo_S 中的 propertyRights 字段
#define NLSTK_CCS_READ_AUTHEN 0x01    // 认证读取权限
#define NLSTK_CCS_READ_ENCRYPT 0x02   // 加密读取权限
#define NLSTK_CCS_READ_AUTHOR 0x04    // 授权读取权限

// 通话请求支持
// Bit0 0：不支持本地保持和本地恢复 1：支持本地保持和本地恢复
// Bit1 0：不支持合并 1：支持合并
#define NLSTK_CCP_CALL_REQUEST_SUPPORT_NOT 0x00
#define NLSTK_CCP_CALL_REQUEST_SUPPORT_LOCAL_HOLD_RESUME 0x01
#define NLSTK_CCP_CALL_REQUEST_SUPPORT_COMBINE 0x02

#define NLSTK_CCP_IN_BAND_RING_OFF 0x00    // 带内铃音关闭
#define NLSTK_CCP_IN_BAND_RING_ON 0x01     // 带内铃音开启

typedef enum {
    NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE = 0,
    NLSTK_CCP_CALL_CONTROL_SERVICE,
} NLSTK_CcpCcsType_E;

typedef enum {
    NLSTK_CCP_CCS_INSTANCE_NAME = 0,
    NLSTK_CCP_CCS_INSTANCE_ICON,
    NLSTK_CCP_CCS_FEATURE_STATUS,
    NLSTK_CCP_CCS_PROTOCOL_SUPPORT,
    NLSTK_CCP_CCS_CALLIN_OUT_INFO,
    NLSTK_CCP_CCS_CALL_STATUS,
    NLSTK_CCP_CCS_CALL_TERMINATION,
    NLSTK_CCP_CCS_MEDIA_INSTANCE_ID,
    NLSTK_CCP_CCS_NETWORK_SELECTION,
    NLSTK_CCP_CCS_CALL_REQ_SUPPORT,
    NLSTK_CCP_CCS_MAX_PROPERTY,
} NLSTK_CcpCcsPropertyType_E;

typedef enum {
    NLSTK_CCP_CCS_RING_OFF = 0,       // 带内铃声关闭
    NLSTK_CCP_CCS_RING_ON,            // 带内铃声开启
} NLSTK_CcpCcsFeatureStatus_E;

typedef enum {
    NLSTK_CCP_CCS_INCOMING_CALL = 0x01,       // 来电
    NLSTK_CCP_CCS_OUTGOING_CALL = 0x02,       // 呼出
    NLSTK_CCP_CCS_RINGING = 0x03,             // 振铃
    NLSTK_CCP_CCS_CONNECTED = 0x04,           // 接通
    NLSTK_CCP_CCS_LOCAL_HOLD = 0x05,          // 本地保持
    NLSTK_CCP_CCS_REMOTE_HOLD = 0x06,         // 远端保持
    NLSTK_CCP_CCS_BOTH_HOLD = 0x07            // 本地与远端均保持
} NLSTK_CcpCcsCallStatus_E;

typedef enum {
    NLSTK_CCP_CCS_CLIENT_ENDED_CALL = 0x01,   // 客户端结束呼叫
    NLSTK_CCP_CCS_SERVER_ENDED_CALL = 0x02,   // 服务端结束呼叫
    NLSTK_CCP_CCS_REMOTE_ENDED_CALL = 0x03,   // 远端结束呼叫
    NLSTK_CCP_CCS_LINE_BUSY = 0x04,           // 线路忙
    NLSTK_CCP_CCS_NO_ANSWER = 0x05,           // 无人接听
    NLSTK_CCP_CCS_NO_NETWORK_SERVICE = 0x06   // 无网络服务
} NLSTK_CcpCcsTerminateReason_E;

typedef enum {
    NLSTK_CCP_CCS_OPERATION_SUCCESS = 0x00,           // 操作成功
    NLSTK_CCP_CCS_REQUEST_TYPE_NOT_SUPPORTED = 0x01,  // 请求类型不支持
    NLSTK_CCP_CCS_CALL_ID_NOT_FOUND = 0x02,           // 通话标识不存在
    NLSTK_CCP_CCS_STATE_MACHINE_ERROR = 0x03,         // 状态机错误
    NLSTK_CCP_CCS_OPERATION_FAILED = 0xFF             // 操作失败
} NLSTK_CcpCcsCallControlRes_E;

typedef struct {
    uint8_t type;
    uint16_t len;
    uint8_t *icon;
} NLSTK_CcpInstanceIcon_S;

typedef struct {
    uint8_t callId;                         /**< 通话标识 */
    uint8_t networkId;                      /**< 网络标识 */
    uint8_t callFlag;                       /**< 通话标记 */
    NLSTK_VariableData_S userInfo;     /**< 用户信息 */
    NLSTK_VariableData_S userAlias;    /**< 用户别名 */
} NLSTK_CcpCallInOutInfo_S;

/**
 * @brief 通话控制相关数据结构体
 */
typedef struct {
    uint8_t callCount;   /**< 通话数量 */
    uint8_t *callId;     /**< 通话标识数组 */
    uint8_t *networkId;  /**< 网络标识数组 */
    uint8_t *callStatus; /**< 通话状态数组 */
    uint8_t *callFlag;   /**< 通话标记数组 */
} NLSTK_CcpCallStatues_S;

typedef struct {
    uint8_t callId;          /**< 通话标识 */
    uint8_t terminateReason; /**< 终止原因 */
} __attribute__((packed)) NLSTK_CcpCallTermination_S;

#ifdef __cplusplus
}
#endif
#endif