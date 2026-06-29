/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef NLSTK_BAS_CLIENT_H
#define NLSTK_BAS_CLIENT_H

#include "bas_client.h"
#include "sdf_mem.h"
#include "bas_def.h"
#include "nlstk_bas_def.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief
* 1) 将上层回调函数注册至协议栈
* @param[in] clientCallback service层注册的回调函数结构体（目前只有NLSTK_DisConnectStateChangeCbk）
*/
NLSTK_Errcode_E NLSTK_BasRegisterCallBack(BasClientCallBack_S *clientCallback);

/**
* @brief
* 1) 调用SSAP接口建链
* 2）服务发现 ssapFindByUuid
* 3）读取设备信息
* 4）连接状态变化回调，将连接状态通知给上层
* @param[in] addr 对端设备地址
*/
NLSTK_Errcode_E NLSTK_BasProfileConnect(SLE_Addr_S *addr);

/**
* @brief
* 1) 调用SSAP接口断链
* 2）连接状态变化回调，将连接状态通知给上层
* @param[in] addr 对端设备地址
*/
NLSTK_Errcode_E NLSTK_BasProfileDisconnect(SLE_Addr_S *addr);

/**
* @brief
* 1) 获取已连接BAS设备数量
* @param[in] num 已连接设备数量
*/
NLSTK_Errcode_E NLSTK_GetConnectedBasDeviceNum(uint8_t *num);

/**
* @brief
* 1) 调用底层接口获取设备的电量信息
* 2) 将电量信息通过指针返回给调用者
* @param[in] Addr 对端设备地址
* @param[out] batteryLevel 用于存储电量信息的指针
*/
NLSTK_Errcode_E NLSTK_GetBatteryLevel(SLE_Addr_S *addr);

#ifdef __cplusplus
}
#endif
#endif /* NLSTK_BAS_CLIENT_H */