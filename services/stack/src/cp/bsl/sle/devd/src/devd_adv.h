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
 * @file         devd_adv.h
 * @brief        sle广播接口
 */
#ifndef DEVD_ADV_H
#define DEVD_ADV_H

#include "devd_tbl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  设置广播参数和数据
 */
void DevdSetAdvParam(void *arg);

/**
 * @brief  更新广播数据
 */
void DevdSetAdvData(void *arg);

/**
 * @brief  使能广播
 */
void DevdEnableAdv(void *arg);

/**
 * @brief  设置发送功率
 */
void DevdSetTxPower(void *arg);

/**
 * @brief  移除广播
 */
void DevdRemoveAdv(void *arg);

/**
 * @brief  创建广播句柄(同时注册对应回调)
 */
uint8_t DevdCreateAdvHandle(NLSTK_DevdAdvEventCbk cbk);

#ifdef __cplusplus
}
#endif
#endif