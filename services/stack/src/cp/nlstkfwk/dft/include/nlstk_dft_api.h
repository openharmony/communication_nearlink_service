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
#ifndef NLSTK_DFT_API_H
#define NLSTK_DFT_API_H

#include <stdint.h>
#include "sdf_addr.h"
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DFT打点主事件类型
 */
typedef enum {
    // SM模块主事件类型
    NLSTK_DFT_EVENT_SM_G_NEGO_EXCEP = 17,
    NLSTK_DFT_EVENT_SM_T_NEGO_EXCEP,
    NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP,
    NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP,
    NLSTK_DFT_EVENT_SM_ENCP_EXCEP,
    // HADM模块主事件类型
    NLSTK_DFT_EVENT_HADM_EXCEP,
    NLSTK_DFT_EVENT_MAX,
} NLSTK_DftEventId_E;

#define NLSTK_DFT_PARAM_VALUE_STR_MAX_LEN 64

typedef struct {
    char str[NLSTK_DFT_PARAM_VALUE_STR_MAX_LEN];
} NLSTK_DftParamValueString_S;

typedef enum {
    NLSTK_DFT_PARAM_VALUE_TYPE_UINT16 = 0,
    NLSTK_DFT_PARAM_VALUE_TYPE_UINT32,
    NLSTK_DFT_PARAM_VALUE_TYPE_INT32,
    NLSTK_DFT_PARAM_VALUE_TYPE_STRING,
} NLSTK_DftParamValueType_E;

/**
 * @brief  DFT模块加密回调函数类型
 */
typedef void (*NLSTK_DftCache)(SLE_Addr_S *addr, uint16_t eventId, uint16_t paramId,
    NLSTK_DftParamValueType_E paramType, void *param);
typedef void (*NLSTK_DftReport)(SLE_Addr_S *addr, uint16_t eventId, uint16_t paramId, uint16_t res);

/**
 * @brief  DFT模块回调函数
 */
typedef struct {
    NLSTK_DftCache cacheCb;     /* 缓存打点数据函数指针 */
    NLSTK_DftReport reportCb;   /* 上报打点数据函数指针 */
} NLSTK_DftCallback_S;

/**
 * @brief 注册DFT打点回调函数
 *
 * 同步调用，返回注册结果
 *
 * @param callback 注册进来的cache和report回调函数
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_DftRegisterCallback(NLSTK_DftCallback_S *callback);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NLSTK_DFT_API_H */