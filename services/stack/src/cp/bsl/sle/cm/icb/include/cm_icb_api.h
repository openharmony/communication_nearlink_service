/**
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
 * CM for isochronous link sync API definitions
 *
 ***************************************************************************/

#ifndef CM_ICB_API_H
#define CM_ICB_API_H

#include <stdint.h>
#include "cm_icb_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  注册同步链路回调函数
 * @param  [in]  < cb > 回调函数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBRegisterCbk(const CM_ICBCallback *cb);

/**
 * @brief  取消注册同步链路回调函数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBUnregisterCbk(void);

/**
 * @brief  设置同步链路参数
 * @param  [in]  < icgParam > 同步链路参数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICGSetParam(CM_ICGParam *icgParam);

/**
 * @brief  设置同步链路Test参数
 * @param  [in]  < icgParam > 同步链路Test参数
 * @param  [in]  < supportAutorate > true表示支持码率自适应
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICGSetTestParam(CM_ICGTestParam *icgParam, bool supportAutorate);

/**
 * @brief  删除同步链路参数
 * @param  [in]  < icgParam > 同步链路删除参数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICGRemoveParam(CM_ICGRemovedParam *icgParam);

/**
 * @brief  设置同步链路label
 * @param  [in]  < icgLabel > 同步链路label
 * @param  [in]  < supportSubrate > true表示支持subrate
 * @param  [in]  < supportAutorate > true表示支持码率自适应
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICGSetLabel(CM_ICGLabelParam *icgLabel, bool supportSubrate, bool supportAutorate);

/**
 * @brief  添加同步链路
 * @param  [in]  < connParam > 同步链路建链参数
 * @param  [in]  < supportAutorate > true表示支持码率自适应
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBAddConnection(CM_ICBConnectionParam *connParam, bool supportAutorate);

/**
 * @brief  删除同步链路
 * @param  [in]  < connParam > 同步链路断链参数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBDelConnection(CM_ICBConnectionParam *connParam);

/**
 * @brief  更新同步链路参数
 * @param  [in]  < icgParam > 同步链路需要更新的参数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICGUpdateParam(CM_ICGUpdatedParam *icgParam);

/**
 * @brief  配置同步链路数据路径以及编解码参数
 * @param  [in]  < dataPath > 同步链路数据路径以及编解码参数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBSetupDataPath(CM_ICBDataPath *dataPath);

/**
 * @brief  删除同步链路数据路径以及编解码参数
 * @param  [in]  < dataPath > 同步链路数据路径删除参数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBRemoveDataPath(CM_ICBRemovedDataPath *dataPath);

/**
 * @brief  监听频段切换事件
 * @param  [in]  < cbk > 频段切换监听者
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ListenFreqBandSwitchEvent(CM_FreqBandListener listener);

/**
 * @brief  取消监听频段切换事件
 * @param  [in]  < cbk > 频段切换监听者
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_UnlistenFreqBandSwitchEvent(CM_FreqBandListener listener);

#ifdef __cplusplus
}
#endif

#endif