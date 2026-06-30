/****************************************************************************
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
****************************************************************************/

/****************************************************************************
 *
 * this file contains the CM for dynamic trans channel tcid allocation definitions
 *
 ***************************************************************************/

#ifndef CM_DYN_TCID_H
#define CM_DYN_TCID_H

#include <stdint.h>
#include "cm_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 动态传输通道TCID分配器初始化
 */
void CM_DynTcidInit(void);

/**
 * @brief 动态传输通道TCID分配器去初始化
 */
void CM_DynTcidDeInit(void);

/**
 * @brief 动态传输通道TCID分配器根据lcid激活TCID池
 */
uint32_t CM_DynTcidActivatePool(uint16_t lcid);

/**
 * @brief 动态传输通道TCID分配器根据lcid销毁TCID池
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_DynTcidDestroyPool(uint16_t lcid);

/**
 * @brief 动态传输通道获取可用TCID
 * @param  [in] < lcid > 目标LCID
 * @param  [in] < transportMode > 星闪接入层数据传输方式，参见CM_AccessTransportMode_E
 * @return CM_TRANS_INVALID_TCID: 无效, 其他: 有效
 */
uint8_t CM_DynTcidAllocate(uint16_t lcid, uint8_t transportMode);

/**
 * @brief 动态传输通道释放目标TCID
 * @param  [in] < lcid > 目标LCID
 * @param  [in] < tcid > 目标TCID
 * @param  [in] < transportMode > 星闪接入层数据传输方式，参见CM_AccessTransportMode_E
 * @return 成功: CM_SUCCESS, 其他：失败
 */
uint32_t CM_DynTcidRelease(uint16_t lcid, uint8_t tcid, uint8_t transportMode);

#ifdef __cplusplus
}
#endif

#endif