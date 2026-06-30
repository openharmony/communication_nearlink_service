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
#ifndef NLSTK_PORT_CLINET_H
#define NLSTK_PORT_CLINET_H

#include <stdint.h>
#include "sdf_addr.h"
#include "nlstk_port_def.h"
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 注册<<应用交互端口信息管理>>服务profile客户端回调
 *
 * 该函数用于注册<<应用交互端口信息管理>>服务profile客户端回调函数，之后profile可通过回调通知上层处理操作或事件。
 *
 * @param[in] clientCallback 回调函数结构体
 *
 * @return uint32_t
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E NLSTK_PortClientRegCbk(NLSTK_PortClientCallBack_S *clientCallback);

/**
 * @brief 解注册<<应用交互端口信息管理>>服务profile客户端回调
 *
 * 该函数用于解注册<<应用交互端口信息管理>>服务profile客户端回调函数。
 *
 * @return uint32_t
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E NLSTK_PortClientDeregCbk(void);

/**
 * @brief 连接<<应用交互端口信息管理>>服务设备
 *
 * 该函数用于对<<应用交互端口信息管理>>服务设备进行PORT profile连接。
 *
 * PORT profile连接包含CM链路的连接、读取初始需要的属性、以及订阅客户端属性配置描述符等。
 *
 * @param[in] addr 对端设备地址
 * @param[in] connParam 连接参数
 *
 * @return uint32_t
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E NLSTK_PortConnect(const SLE_Addr_S *addr, const NLSTK_ConnParam_S *connParam);

/**
 * @brief 断开<<应用交互端口信息管理>>服务设备连接
 *
 * 该函数用于对<<应用交互端口信息管理>>服务设备进行PORT profile断连。
 *
 * @param[in] addr 对端设备地址
 *
 * @return uint32_t
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E NLSTK_PortDisconnect(const SLE_Addr_S *addr);

/**
 * @brief 获取<<应用交互端口信息管理>>服务设备连接状态
 *
 * 该函数用于获取<<应用交互端口信息管理>>服务设备连接状态，为同步接口
 *
 * @param[in] addr 对端设备地址
 * @param[out] state 设备连接状态
 *
 * @return uint32_t
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E NLSTK_PortGetConnectState(const SLE_Addr_S *addr, int *state);

/**
 * @brief 获取<<应用交互端口信息管理>>服务设备端口号
 *
 * 该函数用于通过UUID获取<<应用交互端口信息管理>>服务设备的对应端口号。
 *
 * 读取属性的方式由模块内部具体实现决定。
 *
 * @param[in] addr 对端设备地址
 * @param[in] uuid 对端设备端口号对应的UUID
 * @param[out] portId 对端设备端口号
 *
 * @return uint32_t
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E NLSTK_PortGetDevicePortIdByUuid(const SLE_Addr_S *addr, NLSTK_SsapUuid_S *uuid, uint16_t *portId);

/**
 * @brief 获取<<应用交互端口信息管理>>服务连接设备数量
 *
 * 该函数用于获取<<应用交互端口信息管理>>服务连接设备的数量
 *
 * @param[in] addr 对端设备地址
 * @param[out] portConnectNum 连接设备数量
 *
 * @return uint32_t
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E NLSTK_PortGetConnectDeviceNum(const SLE_Addr_S *addr, int *portConnectNum);

#ifdef __cplusplus
}
#endif

#endif