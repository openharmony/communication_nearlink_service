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
#ifndef NLSTK_DIS_CLIENT_H
#define NLSTK_DIS_CLIENT_H

#include "nlstk_dis_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief
* 1) 将上层回调函数注册至协议栈
* @param[in] clientCallback service层注册的回调函数结构体（目前只有NLSTK_DisConnectStateChangeCbk）
*/
NLSTK_Errcode_E NLSTK_DisRegisterCallbBack(NLSTK_DisClientCbk_S *clientCallback);

/**
* @brief
* 1) 调用SSAP接口建链
* 2）服务发现 ssapFindByUuid
* 3）读取设备信息
* 4）连接状态变化回调，将连接状态通知给上层
* @param[in] addr 对端设备地址
*/
NLSTK_Errcode_E NLSTK_DisProfileConnect(SLE_Addr_S *addr);

/**
* @brief
* 1) 调用SSAP接口断链
* 2）连接状态变化回调，将连接状态通知给上层
* @param[in] addr 对端设备地址
*/
NLSTK_Errcode_E NLSTK_DisProfileDisconnect(SLE_Addr_S *addr);

/**
* @brief
* 1) 根据不同类型枚举同步读取设备缓存在本地的信息
* @param[in] addr 对端设备地址
* @param[in] type 对端设备信息类型
* @param[in] outData 对端设备信息具体内容
*/
NLSTK_Errcode_E NLSTK_DisReadInfo(SLE_Addr_S *addr, NLSTK_DisInfoType_E type, NLSTK_VariableData_S *outData);

/**
* @brief
* 1) 同步读取设备缓存在本地的所有信息
* @param[in] propInfo 对端所有属性信息
*/
NLSTK_Errcode_E NLSTK_DisReadAllInfo(NLSTK_DisAllPropInfo_S *propInfo);

/**
* @brief
* 1) 根据不同类型枚举同步读取设备缓存在本地的外观信息
* @param[in] addr 对端设备地址
* @param[in] appearance 对端设备外观信息
*/
NLSTK_Errcode_E NLSTK_DisReadAppearanceInfo(SLE_Addr_S *addr, uint32_t *appearance);

/**
* @brief
* 1) 根据不同类型枚举同步读取设备缓存在本地的外观信息
* @param[in] addr 对端设备地址
* @param[in] appearance 对端设备外观信息
*/
NLSTK_Errcode_E NLSTK_GetConnectedDeviceNum(uint8_t *num);

#ifdef __cplusplus
}
#endif
#endif /* NLSTK_DIS_CLIENT_H */