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
 * The manager of icb link, which is run in cp thread
 *
 ***************************************************************************/

#ifndef CM_ICB_MGR_H
#define CM_ICB_MGR_H

#include <stdint.h>
#include "cm_icb_def.h"
#include "dli_cmd_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CM_MAX_CHANNEL_COUNT 16 /* 一组同步链路最多支持16个通道 */
#define CM_MAX_LABEL_COUNT 16

typedef struct {
    uint16_t lcid;
    uint16_t subrate;       /* 单位为10ms */
} CM_SetACBSubrateInnerParam;

typedef uint32_t (*CM_InnerSetACBSubratePtr)(const CM_SetACBSubrateInnerParam *param);

/**
 * @brief  注册设置异步链路subrate的函数指针，用于解耦icb与link模块
 * @param  [in] func: 设置异步链路subrate的函数指针
 */
void CM_ICBMgrSetInnerSetACBSubrate(CM_InnerSetACBSubratePtr func);

typedef struct {
    uint16_t connHandle;
    uint16_t lcid;
} CM_GetLcidCtx;

/**
 * @brief  同步链路管理模块初始化
 * @param  [in]  无
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrInit(void);

/**
 * @brief  同步链路管理模块去初始化
 * @param  [in]  无
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrDeinit(void);

/**
 * @brief  同步链路管理模块使能
 * @param  [in]  无
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrEnable(void);

/**
 * @brief  同步链路管理模块去初始化
 * @param  [in]  无
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrDisable(void);

/**
 * @brief  注册同步链路回调函数
 * @param  [in]  < cb > 回调函数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrRegisterCb(const CM_ICBCallback *cb);

/**
 * @brief  取消注册同步链路回调函数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrUnregisterCb(void);

/**
 * @brief  设置同步链路参数
 * @param  [in]  < param > 同步链路参数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrSetParam(DLI_ICGParam *param);

/**
 * @brief  设置同步链路test参数
 * @param  [in]  < param > 同步链路test参数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrSetTestParam(DLI_ICGTestParam *param, bool mcast, bool supportAutorate);

/**
 * @brief  删除同步链路参数
 * @param  [in]  < param > 同步链路参数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrRemoveParam(CM_ICGRemovedParam *param);

/**
 * @brief  设置同步链路label
 * @param  [in]  < param > 同步链路label参数
 * @param  [in]  < supportSubrate > true表示支持subrate
 * @param  [in]  < supportAutorate > true表示支持码率自适应
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICGMgrSetLabel(DLI_ICGLabelParam *param, bool mcast, bool supportSubrate, bool supportAutorate);

/**
 * @brief  添加同步链路
 * @param  [in]  < param > 同步链路建链参数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrAddConnection(DLI_ICBConnectionParam *param, bool mcast, bool supportAutorate);

/**
 * @brief  删除同步链路
 * @param  [in]  < param > 同步链路断链参数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrDelConnection(DLI_ICBConnectionParam *param);

/**
 * @brief  更新同步链路参数
 * @param  [in]  < param > 同步链路参数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICGMgrUpdateParam(DLI_ICGUpdatedParam *param, bool mcast);

/**
 * @brief  配置同步链路数据路径以及编解码参数
 * @param  [in]  < param > 同步链路数据路径以及编解码参数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrSetupDataPath(DLI_SetupICBDataPathParam *param);

/**
 * @brief  移除同步链路数据路径以及编解码参数
 * @param  [in]  < param > 同步链路数据路径以及编解码参数
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrRemoveDataPath(DLI_RemoveICBDataPathParam *param);

/**
 * @brief  监听频段切换事件
 * @param  [in]  < listener > 频段切换监听者
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrListenFreqBandSwitchEvent(CM_FreqBandListener listener);

/**
 * @brief  取消监听频段切换事件
 * @param  [in]  < listener > 频段切换监听者
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ICBMgrUnlistenFreqBandSwitchEvent(CM_FreqBandListener listener);

/**
 * @brief  查询同步链路对应的异步链路id
 * @param  [in]  < arg > 同步链路id，异步链路id
 */
void CM_GetLcidByConnHandleInner(void *arg);

#ifdef __cplusplus
}
#endif

#endif