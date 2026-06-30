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
/*
 * @file HADM_SM.h
 * @brief HADM状态机管理模块，负责HADM模块的状态迁移处理
 * @details 该模块提供以下功能：
 *          - 状态机事件触发函数
 *          - 状态机初始化及回调函数注册
 *          - 处理DLI上报的Sounding状态事件
 */

#ifndef HADM_SM_H
#define HADM_SM_H

#include "hadm_api.h"
#include "hadm_parser_iq.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */
/*  事件枚举类型定义                                                        */
/* ------------------------------------------------------------------------ */

/**
 * @brief HADM状态机事件枚举类型
 * @details 定义了HADM状态机中可能触发的各种事件
 */
typedef enum {
    USER_START_SOUNDING_EVENT = 0, /**< 用户启动声呐事件 */
    USER_STOP_SOUNDING_EVENT,      /**< 用户停止声呐事件 */

    CM_REPORT_LINK_STATE_CONNECTED,    /**< 链路连接状态报告 - 已连接 */
    CM_REPORT_LINK_STATE_DISCONNECTED, /**< 链路连接状态报告 - 已断开 */
    CM_REPORT_FEATURES_EVENT,          /**< 特性报告事件 */
    CM_REPORT_UPDATE_CONN_PARAM_EVENT, /**< 更新连接参数事件 */

    DLI_REPORT_REMOTE_MEASURE_EVENT,       /**< DLI上报远程参数事件 */
    DLI_REPORT_CONFIG_RESULT_EVENT,   /**< DLI配置结果报告事件 */
    DLI_REPORT_SOUNDING_RESULT_EVENT, /**< DLI上报Sounding结果事件 */

    HADM_MAX_EVENT_TYPE /**< 事件类型最大值，用于边界检查 */
} HadmStateMachineEvent_E;

/* ------------------------------------------------------------------------ */
/*  函数声明                                                                */
/* ------------------------------------------------------------------------ */
typedef struct {
    uint16_t status;
    HadmRemoteCsParam_S csCaps;
} HadmDliReportRemoteCsParam_S;

/**
 * @brief 触发HADM状态机
 * @param [in] SLE_Addr_S *addr 链路地址信息指针
 * @param [in] uint32_t event 事件类型
 * @param [in] void *param 事件参数（根据事件类型不同而变化）
 * @return uint32_t 状态码，0表示成功，非0表示失败
 * @details 根据输入的事件类型和参数，触发对应的状态机处理逻辑，进行状态迁移
 */
uint32_t HadmTriggerStateMachine(SLE_Addr_S *addr, uint32_t event, void *param);

/**
 * @brief 触发HADM状态机
 * @param [in] SLE_Addr_S *addr 链路地址信息指针
 * @param [in] uint32_t event 事件类型
 * @param [in] void *param 事件参数（根据事件类型不同而变化）
 * @return uint32_t 状态码，0表示成功，非0表示失败
 * @details 根据输入的事件类型和参数，触发对应的状态机处理逻辑，进行状态迁移
 */
uint32_t HadmTriggerStateMachineByLcid(uint16_t lcid, uint32_t event, void *param);

/**
 * @brief 初始化HADM状态机
 * @param [in] HadmSoundingCbk_S *hadmCbk HADM事件回调函数结构体指针
 * @details 初始化HADM状态机，注册回调函数，以便在状态迁移时调用相应的处理逻辑
 */
void HadmInitStateMachine(HadmSoundingCbk_S *hadmCbk);

/**
 * @brief 处理DLI上报的Sounding状态事件
 * @param [in] void *param 事件参数
 * @details 处理DLI上报的Sounding状态事件，此事件属于“过路”逻辑，HADM不需要做任何处理，直接上报到Service即可
 */
void HadmReportSoundingStateFromDli(HadmSoundingStateInfo_S *param);

uint32_t HadmReportSoundingIqInfoFromDli(uint16_t lcid, HadmIqInfoFromDli_S *iqInfo);

#ifdef __cplusplus
}
#endif
#endif /* HADM_SM_H */