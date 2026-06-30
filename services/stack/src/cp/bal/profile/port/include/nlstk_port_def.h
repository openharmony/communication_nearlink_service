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
#ifndef NLSTK_PORT_DEF_H
#define NLSTK_PORT_DEF_H

#include <stdint.h>
#include "sdf_addr.h"
#include "ssap_type.h"
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

// <<应用交互端口信息管理>>服务profile的连接状态
typedef enum {
    PORT_CONNECTING,
    PORT_CONNECTED,
    PORT_DISCONNECTING,
    PORT_DISCONNECTED,
} NLSTK_PortConnectState_E;

// <<应用交互端口信息管理>>属性属性的类型
typedef enum {
    PORT_NL_STD_PORT_INFO,
    PORT_PRIVATE_PORT_INFO,
} PortPropertyType_E;

/**
 * @brief profile连接状态变化回调函数
 *
 * 该回调函数用于通知上层PORT连接状态变化。
 *
 * 若上层调用PortConnect时已连接或PortDisconnect已断连，则会触发该回调将最新的连接状态上报
 *
 * @param[in] addr 对端地址，标识对端设备地址
 * @param[in] state 连接状态，标识对端设备连接状态
 */
typedef void (*PortConnectStateChangeCbk)(SLE_Addr_S *addr, NLSTK_PortConnectState_E curState,
    NLSTK_PortConnectState_E preState, NLSTK_Errcode_E ret);

// NLSTK_PortClientCallBack_S结构体中的函数指针，用来设置各个操作或事件的回调函数
typedef struct {
    PortConnectStateChangeCbk connectStateChangeCbk;
} NLSTK_PortClientCallBack_S;

#ifdef __cplusplus
}
#endif

#endif