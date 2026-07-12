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
 * 连接管理模块通过白名单连接机制管理多用户(模块)多操作并发的星闪连接
 * 1，支持多设备并发背景连接
 * 2，支持多设备并发直接连接
 * 3，支持背景连接和直接连接并发
 * 4，支持多用户注册监听连接事件
 * 限制说明:
 *    1，并发连接最大设备数量取决于控制器白名单大小（协议栈启动时从控制器读取）
 *       1）当并发连接数量超过白名单大小后，某个添加进来地址可能被忽略添加到白名单列表中，即得不到连接请求和响应操作
 *    2，对于某用户的异常操作，未下发芯片操作前，则直接通知该用户，不再通知其他用户
 *    3，不同模块之间可以互相断连（即不做引用计数），即对于已经存在的连接，发起连接时，将断开该地址的连接
 *    4，不同模块之间可以互相取消连接中的请求（即不做引用计数），即对于已经其他模块发起中的连接，可以做取消操作
 *
 ***************************************************************************/

#ifndef CM_H
#define CM_H

#include <stdint.h>
#include <stdbool.h>
#include "cm_def.h"
#include "sdf_addr.h"
#include "cm_logic_link_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 添加背景连接设备地址，可能触发白名单变化。
 *        1) 接口支持批量添加。
 *        2) 背景连接请求不携带连接参数, 采用默认值。
 *        3) 背景连接成功或者其他失败，则通知用户连接状态变化
 * @param moduleId 连接管理用户模块标识，参见CM_ModuleId_E定义，该接口当前仅支持模块有: 服务层适配模块
 * @param addrArrCount 指示地址列表数量
 * @param addrArr 背景回连设备地址列表
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_BackgroundConnectAdd(uint8_t moduleId, uint8_t addrArrCount, const CM_BgConnAddrParam_S *addrArr);

/**
 * @brief 清除背景连接设备地址，可能触发白名单变化。
 *        1) 背景连接，只做对正在连接中的地址清除操作，不做主动断开已连接成功地址的操作，清除成功或者未清除，不需要通知用户连接状态变化
 * @param moduleId 连接管理用户模块标识，参见CM_ModuleId_E定义，该接口当前仅支持模块有: 服务层适配模块
 * @param addr 背景回连设备地址
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_BackgroundConnectRemove(uint8_t moduleId, const SLE_Addr_S *addr);

/**
 * @brief 清除所有背景连接设备地址
 *
 * @param moduleId 连接管理用户模块标识，参见CM_ModuleId_E定义，该接口当前仅支持模块有: 服务层适配模块
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_BackgroundConnectClear(uint8_t moduleId);

/**
 * @brief 创建直接连接，可能触发白名单变化。
 *        1) 直接连接不支持用户自定义连接参数，由协议栈内部平衡功耗与性能后确定参数。
 *        2）并发操作时，连接状态响应结果存在如下预期行为：
 *           2.1) 对于已删除的设备, 此时本端与对端设备断链已完成(或者断连中)，那么会触发一次DISCONNECTED事件后，再触发一次CONNECTED事件
 *           2.2) 对于已添加的设备，若此时本端与对端设备建链已完成，则忽略通知该用户CONNECTED事件
 *        3）主动连接超时时间为10s
 * @param moduleId 连接管理用户模块标识，参见CM_ModuleId_E定义，该接口当前仅支持模块有: 服务层适配模块和协议栈SSAP模块
 * @param param 目标设备地址参数信息
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_DirectConnectAdd(uint8_t moduleId, const CM_DirectConnAddrParam_S *param);

/**
 * @brief 清除直接连接设备地址，可能触发白名单变化。
 *        1) 并发操作时，连接状态响应结果存在如下预期行为：
 *           1.1）对于已添加的设备，删除时处在建链中，调用本接口后
    *          1.若此时本端与对端设备建链未完成，那么只会通知一次DISCONNECTED事件
    *          2.若此时本端与对端设备建链已完成，那么会通知一次CONNECTED事件后，再通知一次DISCONNECTED事件
 *           1.2）对于已删除的设备，若当前已处在断连中，则直接通知该用户DISCONNECTING事件，用户可忽略该事件，由上一次删除操作完成DISCONNECTED事件
 * @param moduleId 连接管理用户模块标识，参见CM_ModuleId_E定义，该接口当前仅支持模块有: 服务层适配模块和协议栈SSAP模块
 * @param addr 目标设备地址
 * @param discReason 断连原因，定义参见CM_DISC_REASON
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_DirectConnectRemove(uint8_t moduleId, const SLE_Addr_S *addr, uint8_t discReason);

#ifdef __cplusplus
}
#endif

#endif