/**
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

#ifndef NLSTK_SCAN_API_H
#define NLSTK_SCAN_API_H

#include <stdint.h>
#include "sdf_vector.h"
#include "sdf_addr.h"
#include "nlstk_public_define.h"
#include "nlstk_devd_def.h"
#include "nlstk_devd_scan_type_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 注册扫描模块, 同步接口
 *
 * 该函数用于注册一个扫描模块，后续可通过返回的模块标识注册扫描应用，同一模块下的不同扫描应用的扫描结果和事件由scanCbk统一上报。
 *
 * @param [out] moduleId 扫描模块标识
 * @param [in] scanCbk 扫描模块回调
 *
 * @return NLSTK_Errcode_E
 *
 */
NLSTK_Errcode_E NLSTK_DevdRegScanModule(uint8_t *moduleId, NLSTK_DevdScanCbk_S *scanCbk);

/**
 * @brief 解注册扫描模块, 同步接口
 *
 * 该函数用于解注册一个扫描模块，并清除该扫描模块下的所有扫描应用。
 *
 * @param [in] moduleId 扫描模块标识
 *
 * @return NLSTK_Errcode_E
 *
 */
void NLSTK_DevdDeregScanModule(uint8_t moduleId);

/**
 * @brief 添加扫描应用, 同步接口
 *
 * 该函数用于在扫描模块下添加一个扫描应用，后续该扫描应用可开启扫描。
 *
 * @param [in] moduleId 扫描模块标识
 * @param [out] scannerId 扫描应用标识
 *
 * @return NLSTK_Errcode_E
 *
 */
NLSTK_Errcode_E NLSTK_DevdAllocScannerId(uint8_t moduleId, uint32_t *scannerId);

/**
 * @brief 删除扫描应用, 同步接口
 *
 * 该函数用于通过扫描应用标识删除对应扫描应用实例，由于不同模块的扫描应用统一管理，只需要传入扫描应用标识。
 *
 * @param [in] scannerId 扫描应用标识
 *
 * @return void
 *
 */
void NLSTK_DevdRemoveScannerId(uint32_t scannerId);

/**
 * @brief 扫描应用开启扫描, 异步接口
 *
 * 该函数用于对扫描应用开启扫描，并传入扫描参数和过滤器配置，扫描启停事件和通过过滤的扫描结果会通过回调函数上报。
 *
 * @param [in] scannerId 扫描应用标识
 * @param [in] setting 扫描设置
 * @param [in] filters 过滤器数组
 * @param [in] filtersNum 过滤器数量
 *
 * @return NLSTK_Errcode_E
 *
 */
NLSTK_Errcode_E NLSTK_DevdStartScan(uint32_t scannerId, NLSTK_DevdScanSetting_S *setting,
    NLSTK_DevdScanFilter_S *filters, uint16_t filtersNum);

/**
 * @brief 扫描应用停止扫描, 异步接口
 *
 * 该函数用于对扫描应用停止扫描，扫描启停事件通过回调函数上报。
 *
 * @param [in] scannerId 扫描应用标识
 *
 * @return NLSTK_Errcode_E
 *
 */
NLSTK_Errcode_E NLSTK_DevdStopScan(uint32_t scannerId);

/**
 * @brief 停止所有扫描, 异步接口
 *
 * 该函数用于停止所有扫描应用的扫描，扫描启停事件通过回调函数上报。
 *
 * @return NLSTK_Errcode_E
 *
 */
NLSTK_Errcode_E NLSTK_DevdStopAllScan(void);

#ifdef __cplusplus
}
#endif

#endif