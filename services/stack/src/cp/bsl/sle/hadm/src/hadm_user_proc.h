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
 */
/*
 * @file HADM_User_Proc.h
 * @brief HADM用户处理模块
 * @details 该模块负责处理用户对HADM功能的控制请求，包括声呐功能的启停和状态查询
 */

#ifndef HADM_USER_PROC_H
#define HADM_USER_PROC_H

#include "hadm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HADM_MAX_PARALLEL_SOUNDING_NUM 1

/* ------------------------------------------------------------------------ */
/*  结构体定义                                                              */
/* ------------------------------------------------------------------------ */

/**
 * @brief 声呐使能参数结构体
 * @details 定义了声呐功能的使能参数，包括链路地址和使能状态
 */
typedef struct {
    SLE_Addr_S addr; /**< 链路地址信息 */
    uint8_t enable;  /**< 声呐功能使能状态 */
} HadmSoundingEnableParam_S;

/**
 * @brief 声呐使能参数结构体
 * @details 定义了声呐功能的使能参数，包括链路地址和使能状态
 */
typedef struct {
    SLE_Addr_S addr;                       /**< 链路地址信息 */
    HadmConnectionParam_S connectionParam; /**< 连接参数信息 */
    HadmSoundingParam_S soundingParam;     /**< 声呐参数信息 */
    NLSTK_Errcode_E startResult;             /**< 声呐功能使能状态 */
} HadmUserStartSoundingParam_S;

/**
 * @brief 声呐状态参数结构体
 * @details 定义了声呐功能的状态参数，包括链路地址和当前状态
 */
typedef struct {
    SLE_Addr_S addr; /**< 链路地址信息 */
    uint8_t state;   /**< 声呐功能当前状态 */
} HadmSoundingStateParam_S;

typedef struct {
    SLE_Addr_S addr[HADM_MAX_PARALLEL_SOUNDING_NUM]; /**< 链路地址信息 */
    uint8_t soundingAddrNum;                         /**< 正在测距的地址个数 */
} HadmGetSoundingAddrParam_S;

/* ------------------------------------------------------------------------ */
/*  函数声明                                                                */
/* ------------------------------------------------------------------------ */

void HadmServiceRegisterCallBack(void *param);

/**
 * @brief 处理用户启动声呐功能请求
 * @param [in] void *param 用户请求参数指针
 * @details 根据输入的参数，启动指定链路的声呐功能
 */
void HadmUserStartSounding(void *param);

/**
 * @brief 处理用户停止声呐功能请求
 * @param [in] void *param 用户请求参数指针
 * @details 根据输入的参数，停止指定链路的声呐功能
 */
void HadmUserStopSounding(void *param);

/**
 * @brief 处理用户查询声呐状态请求
 * @param [in] void *param 用户请求参数指针
 * @details 根据输入的参数，查询并返回指定链路的声呐状态
 */
void HadmUserGetSoundingState(void *param);

/**
 * @brief 处理用户查询声呐状态请求
 * @param [in] void *param 用户请求参数指针
 * @details 根据输入的参数，查询正在测距的链路数量和地址
 */
void HadmUserGetSoundingAddrInfo(void *param);

#ifdef __cplusplus
}
#endif
#endif /* HADM_USER_PROC_H */