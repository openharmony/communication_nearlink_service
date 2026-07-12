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
 * The icb inner api
 *
 ***************************************************************************/

#ifndef CM_ICB_INNER_API_H
#define CM_ICB_INNER_API_H

#include <stdint.h>
#include "cm_trans_channel_api.h"
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ICB_CONNECTED = 0,
    ICB_DISCONNECTED,
} ICBConnectionType;

/**
 * @brief  同步链路建链和断链事件回调函数
 * @param  [in]  < type > 事件类型，详见ICBConnectionType
 * @param  [in]  < trans > 详见CM_TransChan_S
 * @return 无
 */
typedef void (*CM_ICBConnectionStatusCbk)(ICBConnectionType type, CM_TransChan_S *trans);

/**
 * @brief  注册监听同步链路建链和断链事件
 * @param  [in]  < cbk > 事件回调函数
 * @return 注册成功返回CM_SUCCESS，失败返回错误码
 */
uint32_t CM_RegisterICBConnectionCbk(CM_ICBConnectionStatusCbk cbk);

/**
 * @brief  注销监听同步链路建链和断链事件
 * @return 注销成功返回CM_SUCCESS，失败返回错误码
 */
uint32_t CM_UnregisterICBConnectionCbk(void);

#ifdef __cplusplus
}
#endif

#endif