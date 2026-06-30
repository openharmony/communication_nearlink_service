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
#ifndef PORT_CLIENT_H
#define PORT_CLIENT_H

#include "port_type.h"
#include "nlstk_port_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PORT_STM_STATE_CHANGE(portInfoCache, newState)                                         \
    do {                                                                                       \
        NLSTK_LOG_INFO("[PORT] addr = %s, old state = %u, new state = %u",                       \
            GET_ENC_ADDR(&(portInfoCache)->addr), (portInfoCache)->stmState, (newState));      \
        (portInfoCache)->stmState = (newState);                                                   \
    } while (0)

/**
 * @brief 注册<<应用交互端口信息管理>>服务profile客户端回调
 */
void PortClientRegCbkInner(void *arg);

/**
 * @brief 解注册<<应用交互端口信息管理>>服务profile客户端回调
 */
void PortClientDeregCbkInner(void *arg);

/**
 * @brief 连接<<应用交互端口信息管理>>服务设备
 */
void PortConnectInner(void *arg);

/**
 * @brief 断开<<应用交互端口信息管理>>服务设备连接
 */
void PortDisconnectInner(void *arg);

/**
 * @brief 获取<<应用交互端口信息管理>>服务设备连接状态
 */
void PortGetConnectStateInner(void *arg);

/**
 * @brief 获取<<应用交互端口信息管理>>服务设备端口号
 */
void PortGetDevicePortIdByUuidInner(void *arg);

/**
 * @brief 获取<<应用交互端口信息管理>>服务连接设备数量
 */
void PortGetConnectDeviceNumInner(void *arg);

void PortStateChangeCbk(SLE_Addr_S *addr, NLSTK_PortConnectState_E curState, NLSTK_PortConnectState_E preState,
    NLSTK_Errcode_E ret);

PortInfoCache_S *PortFindInfoCacheByAddr(SLE_Addr_S *addr);

PortInfoCache_S *PortFindInfoCacheByAppId(int32_t appId);

void PortInfoCacheRemove(SLE_Addr_S *addr);

#ifdef __cplusplus
}
#endif

#endif