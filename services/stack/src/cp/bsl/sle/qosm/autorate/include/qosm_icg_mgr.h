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
 * this file contains icb qos manager: listen controller icb qos event,
 * and output max bitrate by rssi, ack rate and packet interval.
 *
 ***************************************************************************/

#ifndef QOSM_ICG_MGR_H
#define QOSM_ICG_MGR_H

#include <stdint.h>
#include "qosm.h"
#include "qosm_autorate_def.h"
#include "qosm_table_mgr.h"
#include "qosm_icg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  使能QOSM ICG MGR模块
 * @return void
 */
void QOSM_ICGMgrEnable(void);

/**
 * @brief  关闭QOSM ICG MGR模块
 * @return void
 */
void QOSM_ICGMgrDisable(void);

/**
 * @brief  注册回调函数
 * @param  [in]  < callback > 回调函数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_ICGMgrRegisterCallback(const QOSM_AutoRateCallback *callback);

/**
 * @brief  注销回调函数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_ICGMgrUnregisterCallback(void);

/**
 * @brief  设置同步链路参数
 * @param  [in]  < param > 创建参数，类型为QOSM_ICGMgrParam
 * @return void
 */
void QOSM_ICGMgrSetParam(void *param);

/**
 * @brief  设置同步链路参数
 * @param  [in]  < param > 创建参数，类型为QOSM_ICGMgrParam
 * @return void
 */
void QOSM_ICGMgrSetTestParam(void *param);

/**
 * @brief  删除同步链路参数
 * @param  [in]  < id > 同步链路唯一标识，类型为uint16_t
 * @return void
 */
void QOSM_ICGMgrRemoveParam(void *id);

/**
 * @brief  设置label，建立同步链路。约束：
 *         1、不允许重复建链
 *         2、以group为单位上报建链结果，一个group底下的链路建链都成功才成功，其余情况均为失败
 * @param  [in]  < param > 创建参数，类型为QOSM_AutoRateConnectionParam
 * @return void
 */
void QOSM_ICGMgrAddConnection(void *param);

/**
 * @brief  断开同步链路（controller会清理label）
 * @param  [in]  < param > 创建参数，类型为QOSM_AutoRateConnectionParam
 * @return void
 */
void QOSM_ICGMgrDeleteConnection(void *param);

/**
 * @brief  设置同步链路data path参数
 * @param  [in]  < param > 创建参数，类型为QOSM_AutoRateDataPath
 * @return void
 */
void QOSM_ICGMgrAddDataPath(void *param);

/**
 * @brief  删除同步链路data path参数
 * @param  [in]  < param > 创建参数，类型为QOSM_AutoRateDeletedDataPath
 * @return void
 */
void QOSM_ICGMgrDeleteDataPath(void *param);

/**
 * @brief  参数变化通知
 * @param  [in]  < param > 参数，类型为QosMgrNotifyParam
 * @return void
 */
void QOSM_ICGMgrParamNotify(void *param);

/**
 * @brief  获取当前全局的占空比
 * @return 全局的占空比
 */
bool QOSM_Is5GFreqBand(void);

QOSM_ICGInfo *QOSM_FindQosIcbInfoByQosId(uint8_t qosId);
QOSM_ICGInfo *QOSM_FindQosIcbInfoByConnHandle(uint16_t connHandle, uint8_t *linkIndex);
QOSM_ICGInfo *QOSM_FindQosIcbInfoByIcgId(uint16_t icgId);

uint8_t QOSM_GetRealLabelId(QOSM_ICGInfo *icgInfo, struct QosLevelLabel *level);
bool QOSM_GetLabelId(QOSM_ICGInfo *icgInfo, QOSM_LinkParam *qosParam, uint8_t *labelId);

void QOSM_ExecuteDelayConnTask(QOSM_ICGInfo *icgInfo);

bool QOSM_IsICBConnected(const struct QosICBInfo *link);
bool QOSM_HasICBConnecting(const QOSM_ICGInfo *icgInfo);

void QOSM_ICGMgrIterate(void (*func)(QOSM_ICGInfo *icgInfo, void *data), void *data);

#ifdef __cplusplus
}
#endif
#endif