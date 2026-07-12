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
 * this file contains the CM Inner API definitions
 *
 ***************************************************************************/

#ifndef CM_INNER_API_H
#define CM_INNER_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t lcid;
    uint16_t subrate;       /* 单位为10ms */
} CM_SetACBSubrateInnerParam;

/**
 * @brief  设置异步链路的subrate，内部接口，运行在CP线程
 * @param  [in] param : acb subrate参数
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_InnerSetACBSubrate(const CM_SetACBSubrateInnerParam *param);

/**
 * @brief  获取对端设备的私有特性，内部接口，运行在CP线程
 * @param  [in] lcid : 异步链路唯一标识
 * @param  [in] featureBit : 特性位图
 * @return true：特性支持，false : 特性不支持
 */
bool CM_InnerGetRemotePrivateFeature(uint16_t lcid, uint16_t featureBit);

#ifdef __cplusplus
}
#endif

#endif