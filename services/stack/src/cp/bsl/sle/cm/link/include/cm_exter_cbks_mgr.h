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

/****************************************************************************
 *
 * this file contains exter cbks manager
 *
 ***************************************************************************/

#ifndef CM_EXTER_CBKS_MGR_H
#define CM_EXTER_CBKS_MGR_H

#include <stdint.h>
#include "sdf_addr.h"
#include "cm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 连接管理模块回调事件
 */
typedef enum {
    CM_SLE_CBK_EVENT_CONNECT_INVALID = 0,                /* 无效事件 */
    CM_SLE_CBK_EVENT_CONNECT_CANCEL,                     /* 取消连接建立 */
    CM_SLE_CBK_EVENT_CONNECT_REMOTE_UPDATE_PARAM_REQ,    /* 对端连接参数更新请求 */
    CM_SLE_CBK_EVENT_CONNECT_PARAM_UPDATE,               /* 连接参数更新 */
    CM_SLE_CBK_EVENT_READ_REMOTE_FEATURE_VERSION,        /* 读取端特性版本 */
    CM_SLE_CBK_EVENT_SET_PHY,                            /* 设置PHY参数 */
    CM_SLE_CBK_EVENT_READ_LOCAL_FEATURE,                 /* 读取本端特性 */
    CM_SLE_CBK_EVENT_ENABLE_CONN_HIGH_POWER,             /* 使能连接高功率 */
    CM_SLE_CBK_EVENT_SET_PEER_DEV_TYPE,                  /* 设置对端设备类型 */
    CM_SLE_CBK_EVENT_SET_RX_DATA_FILTER,                 /* 设置RxDataFilter参数 */
    CM_SLE_CBK_EVENT_SET_SUBRATE,                        /* 设置subrate参数 */
    CM_SLE_CBK_EVENT_REQ_SUBRATE,                        /* 请求subrate参数 */
    CM_SLE_CBK_EVENT_READ_ACCEPT_FLT_LIST_SIZE,          /* 读取白名单列表大小 */
    CM_SLE_CBK_EVENT_READ_REMOTE_RSSI,                   /* 读取rssi */
    CM_SLE_CBK_EVENT_MAX
} CM_SleCbkEventType_S;

void CM_RegExterCbks(const CM_ConnectCbks_S *cbks);

void CM_UnRegExterCbks(void);

/**
 * @brief  连接管理模块调用上层事件回调处理函数
 * @param  [in]  < event > 回调事件类型, 参见CM_SleCbkEventType_S定义
 * @param  [in]  < param > 回调函数参数
 */
void CM_ExecuteEventCbk(uint8_t event, void *param);

#ifdef __cplusplus
}
#endif

#endif