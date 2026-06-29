
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
 * this file contains access manager defines
 *
 ***************************************************************************/

#ifndef CM_ACCESS_MGR_H
#define CM_ACCESS_MGR_H

#include <stdint.h>
#include "sdf_addr.h"
#include "dli.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t opIndex;
    uint32_t size;
    void     *eventParameter;
} CM_ExecuteCmdPar_S;

typedef struct {
    uint16_t lcid;
} CM_ConnectExceptionRsp_S;

/**
 * @brief  星闪读取对端版本结果上报
 */
typedef struct {
    uint16_t lcid;
    uint8_t version;
    uint16_t companyId;
    uint16_t subversion;
    SLE_Addr_S addr;
    uint8_t role;        /* 链路角色：G或者T, 参考CM_NodeType_E定义 */
    uint8_t status;      /* 链路状态, 参见 CM_ConnectLinkState_E */
} CM_ReadRemoteVersionRsp_S;

typedef enum {
    SLE_ACCESS_CBK_CONNECT = 0,
    SLE_ACCESS_CBK_CONNECT_CANCEL,
    SLE_ACCESS_CBK_DISCONNECT,
    SLE_ACCESS_CBK_CONNECT_REMOTE_UPDATE_REQ,
    SLE_ACCESS_CBK_CONNECT_UPDATE,
    SLE_ACCESS_CBK_READ_REMOTE_VERSION,
    SLE_ACCESS_CBK_READ_REMOTE_FEATURE,
    SLE_ACCESS_CBK_READ_REMOTE_FEATURE_AND_VERSION,
    SLE_ACCESS_CBK_SET_DATA_LEN,
    SLE_ACCESS_CBK_SET_PHY,
    SLE_ACCESS_CBK_READ_LOCAL_FEATURE,
    SLE_ACCESS_CBK_ENABLE_CONN_HIGH_POWER,
    SLE_ACCESS_CBK_SET_PEER_DEV_TYPE,
    SLE_ACCESS_CBK_SET_RX_DATA_FILTER,
    SLE_ACCESS_CBK_CONNECT_EXCEPTION,
    SLE_ACCESS_CBK_SET_SUBRATE,
    SLE_ACCESS_CBK_REQ_SUBRATE,
    SLE_ACCESS_CBK_READ_ACCEPT_FLT_LIST_SIZE,
    SLE_ACCESS_CBK_READ_REMOTE_RSSI,
    SLE_ACCESS_CBK_MAX
} SleAccessLinkCbkType_E;

typedef void (*CM_ExeCmdCbk)(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par);

/**
 * @brief  SLE接入适配模块注册命令执行完成回调处理函数
 * @param  [in]  < type > 事件类型, 参见SleAccessLinkCbkType_E定义
 * @param  [in]  < cbk > 回调函数
 */
void CM_AccessRegCbk(uint8_t type, CM_ExeCmdCbk cbk);

/**
 * @brief  SLE接入适配模块取消注册所有命令执行完成回调处理函数
 * @param  [in]  < void > 无
 */
void CM_AccessUnRegAllCbk(void);

/**
 * @brief  SLE接入适配模块获取命令执行完成回调处理函数
 * @param  [in]  < type > 事件类型, 参见SleAccessLinkCbkType_E定义
 * @param  [out] < cbk > 回调函数
 */
CM_ExeCmdCbk CM_AccessGetCbk(uint8_t type);

/**
 * @brief  SLE接入适配模块注册底层DLI事件回调函数
 * @param  [in]  < table > 底层DLI事件回调函数列表
 * @param  [in]  < size > 底层DLI事件回调函数列表大小
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_AccessRegDliCbks(const DLI_CbkLineStru *table, uint32_t size);

/**
 * @brief  SLE接入适配模块去注册底层DLI事件回调函数
 * @param  [in]  < table > 底层DLI事件回调函数列表
 * @param  [in]  < size > 底层DLI事件回调函数列表大小
 */
void CM_AccessUnRegDliCbk(const DLI_CbkLineStru *table, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif