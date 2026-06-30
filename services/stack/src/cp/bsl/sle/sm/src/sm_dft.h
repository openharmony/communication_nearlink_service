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
#ifndef SM_DFT_H
#define SM_DFT_H

#include <stdint.h>
#include "sdf_addr.h"
#include "sm_stm.h"
#include "sm_struct.h"
#include "nlstk_dft.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************************
                                    SM模块打点数据定义
*****************************************************************************************/

/**
 * @brief G节点协商打点数据
 */
typedef enum {
    SM_DFT_DEVICE_ADDR = 1,
    SM_DFT_G_NEGO_PEER_INFO = 3,
    SM_DFT_G_NEGO_START_PAIR_TIME,
    SM_DFT_G_NEGO_GEN_KEY_TIME,
    SM_DFT_G_NEGO_RECV_PAIR_START_TIME,
    SM_DFT_G_NEGO_SEND_PAIR_REQ_TIME,
    SM_DFT_G_NEGO_RECV_PAIR_RESP_TIME,
    SM_DFT_G_NEGO_SEND_PAIR_CFM_TIME,
    SM_DFT_G_NEGO_RECV_PAIR_INIT_TIME,
    SM_DFT_G_NEGO_ERR_CODE,
    SM_DFT_G_NEGO_RES,
} SmDftGNegoExcepParam_E;

/**
 * @brief T节点协商打点数据
 */
typedef enum {
    SM_DFT_T_NEGO_DEVICE_ADDR = 1,
    SM_DFT_T_NEGO_PEER_INFO = 3,
    SM_DFT_T_NEGO_START_PAIR_TIME,
    SM_DFT_T_NEGO_GEN_KEY_TIME,
    SM_DFT_T_NEGO_SEND_PAIR_START_TIME,
    SM_DFT_T_NEGO_RECV_PAIR_REQ_TIME,
    SM_DFT_T_NEGO_SEND_PAIR_RESP_TIME,
    SM_DFT_T_NEGO_RECV_PAIR_CFM_TIME,
    SM_DFT_T_NEGO_SEND_PAIR_INIT_TIME,
    SM_DFT_T_NEGO_ERR_CODE,
    SM_DFT_T_NEGO_RES,
    SM_DFT_T_NEGO_PARAM_BUTT,
} SmDftTNegoExcepParam_E;

/**
 * @brief G节点鉴权打点数据
 */
typedef enum {
    SM_DFT_G_AUTH_DEVICE_ADDR = 1,
    SM_DFT_G_AUTH_PEER_INFO = 3,
    SM_DFT_G_AUTH_METHOD,
    SM_DFT_G_AUTH_GEN_PASSCODE_TIME,
    SM_DFT_G_AUTH_RECV_CFM_TIME,
    SM_DFT_G_AUTH_SEND_RA_TIME,
    SM_DFT_G_AUTH_RECV_RB_TIME,
    SM_DFT_G_AUTH_USER_CFM_TIME,
    SM_DFT_G_AUTH_GEN_KEY_TIME,
    SM_DFT_G_AUTH_GEN_CFMKEY_TIME,
    SM_DFT_G_AUTH_SEND_CFMKEY_TIME,
    SM_DFT_G_AUTH_RECV_CFMKEY_TIME,
    SM_DFT_G_AUTH_ERR_CODE,
    SM_DFT_G_AUTH_RES,
} SmDftGAuthExcepParam_E;

/**
 * @brief T节点鉴权打点数据
 */
typedef enum {
    SM_DFT_T_AUTH_DEVICE_ADDR = 1,
    SM_DFT_T_AUTH_PEER_INFO = 3,
    SM_DFT_T_AUTH_METHOD,
    SM_DFT_T_AUTH_GEN_CFM_TIME,
    SM_DFT_T_AUTH_RECV_CFM_TIME,
    SM_DFT_T_AUTH_RECV_RA_TIME,
    SM_DFT_T_AUTH_SEND_CFM_TIME,
    SM_DFT_T_AUTH_SEND_RB_TIME,
    SM_DFT_T_AUTH_USER_CFM_TIME,
    SM_DFT_T_AUTH_GEN_KEY_TIME,
    SM_DFT_T_AUTH_GEN_CFMKEY_TIME,
    SM_DFT_T_AUTH_SEND_CFMKEY_TIME,
    SM_DFT_T_AUTH_RECV_CFMKEY_TIME,
    SM_DFT_T_AUTH_ERR_CODE,
    SM_DFT_T_AUTH_RES,
} SmDftTAuthExcepParam_E;

/**
 * @brief SM打点鉴权类型
 */
typedef enum {
    SM_DFT_NOENTRY = 0,
    SM_DFT_NUMCMP,
    SM_DFT_PASSCODE,
    SM_DFT_OOB,
    SM_DFT_PSK,
    SM_DFT_PASSWORD,
} SmDftAuthMethod_E;

/**
 * @brief 加密打点数据
 */
typedef enum {
    SM_DFT_ENCP_DEVICE_ADDR = 1,
    SM_DFT_ENCP_PEER_INFO = 3,
    SM_DFT_ENCP_ENABLE_TIME,
    SM_DFT_ENCP_REQ_PARAM_TIME,
    SM_DFT_ENCP_ERR_CODE,
    SM_DFT_ENCP_RES,
} SmDftEncpExcepParam_E;

/**
 * @brief SM打点异常类型
 */
typedef enum {
    SM_DFT_SUCC = 0,
    SM_DFT_UNKNOWN_COMMAND,
    SM_DFT_UNKNOWN_CONNECTION_INDENTIFIER,
    SM_DFT_HARDWARE_FAILURE,
    SM_DFT_AUTHENTICATION_FAILURE,
    SM_DFT_KEY_MISSING,
    SM_DFT_MEMORY_CAPACITY_EXCEEDED,
    SM_DFT_CONNECTION_TIMEOUT,
    SM_DFT_CONNECTION_LIMIT_EXCEEDED,
    SM_DFT_SYNC_CONNECTION_LIMIT_EXCEEDED,
    SM_DFT_CONNECTION_ALREADY_EXISTS,
    SM_DFT_COMMAND_DISALLOWED,
    SM_DFT_UNACCEPTADLI_BDADDR,
    SM_DFT_COMMAND_TIMEOUT,
    SM_DFT_UNSUPPORTED_REMOTE_FEATURE,
    SM_DFT_INVALID_PARAMETERS,
    SM_DFT_REMOTE_USER_TERMINATED_CONNECTION,
    SM_DFT_CONNECTION_TERMINATED_BY_LOCAL_HOST,
    SM_DFT_ROLE_CHANGE_NOT_ALLOWED,
    SM_DFT_ENCRYPTION_MODE_NOT_ACCEPTAGLE,
    SM_DFT_LINK_KEY_CANNOT_BE_CHANGED,
    SM_DFT_INSTANT_PASSED,
    SM_DFT_CHANNEL_CLASSIFICATION_NOT_SUPPORTED,
    SM_DFT_INSUFFICIENT_SECURITY,
    SM_DFT_ROLE_SWITCH_FAILED,
    SM_DFT_CONTROLLER_BUSY,
    SM_DFT_ADVERTISING_TIMEOUT,
    SM_DFT_CONNECTION_TERMINATED_MIC_FAILURE,
    SM_DFT_CONNECTION_FAILED_TO_BE_ESTABLISHED,
    SM_DFT_CCA_REJECTED_BUT_ADJUST_USING_CLOCK_DRAGGING,
    SM_DFT_UNKNOWN_ADVERTISING_IDENTIFIER,
    SM_DFT_PACKET_TOO_LONG,
    SM_DFT_UNSPECIFIED_ERROR,
    SM_DFT_RET_MSG_IS_NULL = 0X100,
    SM_DFT_PKG_SIZE_ERR,
    SM_DFT_GEN_CFM_ERR,
    SM_DFT_GEN_CFMKEY_ERR,
    SM_DFT_GEN_LINKKEY_ERR,
    SM_DFT_GEN_SIX_DIGIT_ERR,
    SM_DFT_PASS_CODE_ERR,
    SM_DFT_CFM_CODE_ERR,
    SM_DFT_CFM_KEY_ERR,
    SM_DFT_TIMEOUT_ERR,
    SM_DFT_SEND_MSG_ERR,
} SmDftErrCode_E;

typedef enum {
    SM_DFT_RES_SUCC = 0,
    SM_DFT_RES_FAIL = 1,
} SmDftRes_E;


void SmDftCacheTimestamp(SLE_Addr_S *addr, NLSTK_DftEventId_E eventId, uint16_t paramId);
void SmDftCacheU16(SLE_Addr_S *addr, NLSTK_DftEventId_E eventId, uint16_t paramId, uint16_t param);
void SmDftReport(SLE_Addr_S *addr, SmState_E curState, SmNodeType_E nodeType, uint16_t errVal);

#ifdef __cplusplus
}
#endif

#endif /* SM_DFT_H */