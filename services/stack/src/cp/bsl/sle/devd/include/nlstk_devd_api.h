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
 * @file         nlstk_devd_api.h
 * @brief        设备发现对外api
 */
#ifndef NLSTK_DEVD_API_H
#define NLSTK_DEVD_API_H

#include <stdint.h>
#include "nlstk_devd_def.h"
#include "nlstk_devd_def_ext.h"
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 设置广播数据
 * @param setAdvData 广播数据设置结构体指针
 * @return 返回错误码
 */
NLSTK_Errcode_E NLSTK_DevdSetAdvData(NLSTK_DevdSetAdvData_S *setAdvData);

/**
 * @brief 开启广播入口
 * @param advParams 广播参数设置结构体指针
 * @return 返回错误码
 */
NLSTK_Errcode_E NLSTK_DevdStartAdv(NLSTK_DevdSetAdvParams_S *advParams);

/**
 * @brief 启用广播功能
 * @param setEnable 广播功能启用结构体指针
 * @return 返回错误码
 */
NLSTK_Errcode_E NLSTK_DevdEnableAdv(NLSTK_DevdSetAdvEnable_S *setEnable);

/**
 * @brief 设置发送功率
 * @param setTxPower 发送功率参数结构体指针
 * @return 返回错误码
 */
NLSTK_Errcode_E NLSTK_DevdSetTxPower(NLSTK_DevdSetTxPower_S *setTxPower);

/**
 * @brief 移除广播设置
 * @param setAdvHandle 广播设置句柄
 * @return 返回错误码
 */
NLSTK_Errcode_E NLSTK_DevdRemoveAdv(uint8_t *setAdvHandle);

/**
 * @brief 创建广播句柄
 * @param cbk 广播节点回调
 * @return 返回句柄值，当返回为0xFF时，表示创建句柄失败
 */
uint8_t NLSTK_DevdCreateAdvHandle(NLSTK_DevdAdvEventCbk cbk);

/**
 * @brief 开启扫描入口
 * @param sleScanParams 扫描参数设置结构体指针
 * @return 返回错误码
 */
NLSTK_Errcode_E NLSTK_DevdSleStartScan(NLSTK_DevdSleScanParams_S *sleScanParams);

/**
 * @brief 启用扫描功能
 * @param sleScanEnable 扫描启用结构体指针
 * @return 返回错误码
 */
NLSTK_Errcode_E NLSTK_DevdSleEnableScan(NLSTK_DevdSleScanEnable_S *sleScanEnable);

/**
 * @brief 注册扫描事件回调
 * @param scanEventCbk 扫描事件回调结构体指针
 * @return 返回错误码
 */
NLSTK_Errcode_E NLSTK_DevdRegScanEventCbk(NLSTK_DevdSleScanExterCbk_S *scanEventCbk);

#ifdef __cplusplus
}
#endif

#endif