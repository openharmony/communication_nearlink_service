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

#ifndef SCAN_API_H
#define SCAN_API_H

#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  多路扫描初始化
 */
NLSTK_Errcode_E DevdScanInit(void);

/**
 * @brief  多路扫描去初始化
 */
void DevdScanDeInit(void);

/**
 * @brief  注册扫描模块
 */
void DevdRegScanModule(void *arg);

/**
 * @brief  解注册扫描模块
 */
void DevdDeregScanModule(void *arg);

/**
 * @brief  添加扫描应用
 */
void DevdAllocScannerId(void *arg);

/**
 * @brief  删除扫描应用
 */
void DevdRemoveScannerId(void *arg);

/**
 * @brief  扫描应用开启扫描
 */
void DevdStartScan(void *arg);

/**
 * @brief  扫描应用停止扫描
 */
void DevdStopScan(void *arg);

/**
 * @brief  停止所有扫描
 */
void DevdStopAllScan(void *arg);

#ifdef __cplusplus
}
#endif

#endif