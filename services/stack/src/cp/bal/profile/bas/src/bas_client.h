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
#ifndef BAS_CLIENT_H
#define BAS_CLIENT_H

#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  注册回调函数
 */
void BasRegClientCbk(void *arg);

/**
 * @brief  启用BAS功能
 * 
 * @param[out]  是否初始化成功
 */
NLSTK_Errcode_E BasEnable(void);

/**
 * @brief  禁用BAS功能
 */
void BasDisable(void);

/**
 * @brief  BAS建立连接
 */
void BasConnectTask(void *arg);

/**
 * @brief  BAS断开连接
 */
void BasDisconnectTask(void *arg);

/**
 * @brief  BAS计数连接设备
 */
void BasCountConnectedDevicesTask(void *arg);

/**
 * @brief  获取电池电量任务
 */
void BasGetBatteryLevelTask(void *arg);

#ifdef __cplusplus
}
#endif
#endif
