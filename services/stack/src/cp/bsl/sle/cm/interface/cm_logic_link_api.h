/****************************************************************************
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
****************************************************************************/

/****************************************************************************
 *
 * this file contains the CM for logic link sync API definitions
 *
 ***************************************************************************/

#ifndef CM_LOGIC_LINK_API_H
#define CM_LOGIC_LINK_API_H

#include "sdf_addr.h"
#include "cm.h"
#include "nlstk_api_type_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  根据模块注册逻辑链路变化等监听回调
 * @param  [in] < cbks > 回调接口函数
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_RegLogicLinkListener(CM_LogicLinkCbks_S *cbks);

/**
 * @brief  根据模块取消注册逻辑链路变化回调
 * @param [in] < moduleId > 模块标识，参见CM_ModuleId_E定义
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_UnRegLogicLinkListener(uint8_t moduleId);

/**
 * @brief  根据对端设备地址查找出逻辑链路信息
 * @param  [in] < addr > 对端设备地址
 * @param  [out] < logicLink > 逻辑链路信息
 * @return SUCCESS: 成功, NOT_FOUND: 找不到, OTHER: 失败
 */
uint32_t CM_GetLogicLinkByAddr(SLE_Addr_S *addr, CM_LogicLink_S *logicLink);

/**
 * @brief  根据LCID查找出逻辑链路信息
 * @param  [in] < lcid > 逻辑链路标识
 * @param  [out] < logicLink > 逻辑链路信息
 * @return SUCCESS: 成功, NOT_FOUND: 找不到, OTHER: 失败
 */
uint32_t CM_GetLogicLinkByLcid(uint16_t lcid, CM_LogicLink_S *logicLink);

/**
 * @brief 获取已完成连接的连接数
 * @return 已完成连接的连接数, 若未初始化, 则直接返回0
 */
uint32_t CM_GetLogicLinkConnectedSize(void);

/**
 * @brief  根据LCID设置逻辑链路设备类型, 备注: 当前主要使用者为SM模块，用于兼容老设备类型
 * @param  [in] < lcid > 逻辑链路标识
 * @param  [in] < deviceType > 设备类型, 参见CM_DeviceType_E定义
 * @return SUCCESS: 成功, NOT_FOUND: 找不到, OTHER: 失败
 */
uint32_t CM_SetLogicLinkDeviceType(uint16_t lcid, uint8_t deviceType);

/**
 * @brief  根据LCID查找出对端设备类型
 * @param  [in] < lcid > 逻辑链路标识
 * @return  CM_DeviceType_E 老设备，新设备或者三方设备
 */
uint8_t CM_GetLogicLinkDeviceType(uint16_t lcid);

/**
 * @brief  根据LCID设置对端的feature
 * @param  [in] < info > 对端feature相关的信息
 * @return  SUCCESS: 成功, NOT_FOUND: 找不到, OTHER: 失败
 */
uint32_t CM_SetRemoteFeature(const NLSTK_RemoteFeatureInfo_S *info);
#ifdef __cplusplus
}
#endif

#endif