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
#ifndef CCP_TYPE_H
#define CCP_TYPE_H

#include "sdf_vector.h"
#include "nlstk_public_define.h"
#include "nlstk_ccp_ccs_server.h"
#include "nlstk_ccp_vas_server.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CCP_CCS_INVALID_UUID 0
#define CCP_INVALID_APP_ID (-1)
#define CCP_CCS_REQUIRED_PROPERTY_NUM 8
#define CCP_CCS_METHOD_NUM 1
#define CCP_DESC_CLIENT_CONFIG_LEN 2
#define CCP_READ_PERMISSION 0x10
#define CCP_ICON_TYPE_LEN 1
#define CCP_CALLIN_OUT_LEN 5
#define CCP_CALL_COUNT_LEN 1
#define CCP_CALL_STATUS_LEN 4
#define CCP_CCS_INVALID_OPCODE 0
#define CCP_CCS_METHOD_REQ_MIN_LEN 2
#define CCP_CCS_METHOD_RES_ERROR_LEN 2
#define CCP_CCS_METHOD_RES_STM_ERROR_LEN 4
#define CCP_CCS_METHOD_RES_LEN 3
#define CCP_VAS_PROPERTY_NUM 1
#define CCP_VAS_METHOD_NUM 1
#define CCP_VAS_METHOD_REQ_LEN 1
#define CCP_VAS_METHOD_RES_LEN 2
#define CCP_CCS_MAX_DATA_LEN 0xFFFF
#define CCP_CCS_MAX_CALL_COUNT 0xFF

// <<通话管理>>服务通用唯一标识
typedef enum {
    CCP_CALL_CONTROL_SERVICE_UUID = 0x0611,
    CCP_CALL_CONTROL_COMMON_SERVICE_UUID = 0x0612,
    CCP_CCS_INSTANCE_NAME_UUID = 0x1057,
    CCP_CCS_INSTANCE_ICON_UUID = 0x1058,
    CCP_CCS_FEATURE_STATUS_UUID = 0x1059,
    CCP_CCS_PROTOCOL_SUPPORT_UUID = 0x105A,
    CCP_CCS_CALLIN_OUT_INFO_UUID = 0x105B,
    CCP_CCS_CALL_STATUS_UUID = 0x105C,
    CCP_CCS_CALL_TERMINATION_UUID = 0x105D,
    CCP_CCS_MEDIA_INSTANCE_ID_UUID = 0x106F,
    CCP_CCS_NETWORK_SELECTION_UUID = 0x105E,
    CCP_CCS_CALL_REQ_SUPPORT_UUID = 0x105F,
    CCP_CCS_CALL_CONTROL_POINT_UUID = 0x1060,
} CcpCallControlServiceUuid_E;

// <<语音助手>>服务通用唯一标识
typedef enum {
    CCP_VOICE_ASSISTANT_SERVICE_UUID = 0x0613,
    CCP_VOICE_ASSISTANT_STATE_UUID = 0x1061,
    CCP_VOICE_ASSISTANT_CONTROL_UUID = 0x1062,
} CcpVoiceAssistantServiceUuid_E;

// 通话控制点操作码
typedef enum {
    CCP_CALLOUT_OPCODE = 0x01,
    CCP_ANSWER_OPCODE = 0x02,
    CCP_HANGUP_OPCODE = 0x03,
    CCP_HOLD_OPCODE = 0x04,
    CCP_RESUME_OPCODE = 0x05,
    CCP_MERGE_OPCODE = 0x06,
    CCP_GET_CALLINFO_OPCODE = 0x10
} CcpCallControlOpCode_E;

typedef struct {
    uint16_t handle;
    uint16_t uuid;
    NLSTK_CcpCcsPropertyType_E type;
    NLSTK_VariableData_S value;   // 当前不保存具体值，暂不使用
} CcpCcsPropertyCache_S;

typedef struct {
    uint16_t handle;
    uint16_t uuid;
} CcpCcsMethodCache_S;

typedef struct {
    uint16_t srvHandle;
    uint16_t endHandle;
    uint16_t uuid;
    uint16_t callControlHandle;
    SDF_Vector_S *properties;
} CcpCallControlServiceCache_S;

typedef struct {
    bool used;
    uint8_t instanceId;
    int32_t appId;
    NLSTK_CcpCallControlInfo_S *baseInfo;
    CcpCallControlServiceCache_S serviceCache;
    NLSTK_CcpCallControlPoint_S callControl;
    NLSTK_CcpStartCallControlServiceInst startCcsInst;
    NLSTK_CcpCallControlServiceAuthorize authorize;
    SDF_Vector_S *callReqQue;
    SDF_Vector_S *callStatusVec;
} CcpCallControlService_S;

typedef struct {
    uint32_t requestId;
    int32_t instanceId;
    uint8_t errorCode;
} CcpCallControlResultParam_S;

typedef struct {
    uint32_t requestId;
    int32_t instanceId;
    NLSTK_CcpCcsPropertyType_E type;
    uint8_t errorCode;
} CcpCcsAuthorizeResultParam_S;

typedef struct {
    int32_t instanceId;
    NLSTK_CcpCcsPropertyType_E type;
    NLSTK_VariableData_S *value;
} CcpUpdateCcsPropertyParam_S;

typedef struct {
    uint16_t requestId;
    uint8_t opCode;
    uint8_t callId;
} CcpCcsMethodCallRequest_S;

typedef struct {
    uint8_t callId;
    uint8_t networkId;
    uint8_t callStatus;
    uint8_t callFlag;
} CcpCcsCallStatus_S;

typedef struct {
    uint32_t requestId;
    uint8_t opCode;
    uint8_t errorCode;
} CcpVasControlResultParam_S;

typedef struct {
    uint32_t requestId;
    uint8_t errorCode;
} CcpVasAuthorizeResultParam_S;

typedef struct {
    bool used;
    int32_t appId;
    uint8_t state;
    uint8_t stateRight;
    uint16_t srvHandle;
    uint16_t stateHandle;
    uint16_t controlHandle;
    NLSTK_CcpVasStartService startServiceCbk;
    NLSTK_CcpVasStateAuthorize authorizeCbk;
    NLSTK_CcpVasControlPoint_S controlPoint;
} CcpVasService_S;

#ifdef __cplusplus
}
#endif

#endif