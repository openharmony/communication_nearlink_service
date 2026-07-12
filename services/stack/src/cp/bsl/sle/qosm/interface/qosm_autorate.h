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
 * this file contains qosm auto rate apis.
 *
 ***************************************************************************/

#ifndef QOSM_AUTORATE_H
#define QOSM_AUTORATE_H

#include <stdint.h>
#include "qosm_autorate_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  注册回调函数
 * @param  [in]  < callback > 回调函数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_AutoRateRegisterCallback(const QOSM_AutoRateCallback *callback);

/**
 * @brief  注销回调函数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_AutoRateUnregisterCallback(void);

/**
 * @brief  设置参数
 * @param  [in]  < param > 参数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_AutoRateSetParam(const QOSM_AutoRateParam *param);

/**
 * @brief  设置Test参数
 * @param  [in]  < param > 参数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_AutoRateSetTestParam(const QOSM_AutoRateParam *param);

/**
 * @brief  删除参数
 * @param  [in]  < qosId > qosId
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_AutoRateRemoveParam(uint8_t qosId);

/**
 * @brief  创建链路
 * @param  [in]  < param > 参数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_AutoRateAddConnection(const QOSM_AutoRateConnParam *param);

/**
 * @brief  销毁链路
 * @param  [in]  < param > 参数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_AutoRateDeleteConnection(const QOSM_AutoRateConnParam *param);

/**
 * @brief  设置上下行通路以及编解码器参数
 * @param  [in]  < param > 参数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_AutoRateAddDataPath(const QOSM_AutoRateDataPath *param);

/**
 * @brief  删除上下行通路以及编解码器参数
 * @param  [in]  < param > 参数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_AutoRateDeleteDataPath(const QOSM_AutoRateDeletedDataPath *param);

/**
 * @brief  接收耳机反馈参数
 * @param  [in]  < param > 耳机反馈参数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_AutoRateSetEarphoneFeedback(const QOSM_AutoRateEarphoneFeedbackParam *param);

/**
 * @brief  接收共存模块参数
 * @param  [in]  < param > 共存模块参数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_AutoRateSetCoexistSuggestion(const QOSM_AutoRateCoexistSuggestionParam *param);

/**
 * @brief  获取ICG G2T参数
 * @param  [in]  < qosIndex > qos码率自适应索引
 * @param  [out]  < param > ICG G2T参数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_AutoRateGetICGG2TParam(QOSM_QosIndex qosIndex, QOSM_ICBParam *param);

/**
 * @brief  获取ICG T2G参数
 * @param  [in]  < qosIndex > qos码率自适应索引
 * @param  [out]  < param > ICG T2G参数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_AutoRateGetICGT2GParam(QOSM_QosIndex qosIndex, QOSM_ICBParam *param);

/**
 * @brief  调用方设置组播升码率参数
 * @param  [in]  < param > 升码率参数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_RecvAutoRateMsg(const QOSM_AutoRateRecvMsgParam *param);

#ifdef __cplusplus
}
#endif
#endif