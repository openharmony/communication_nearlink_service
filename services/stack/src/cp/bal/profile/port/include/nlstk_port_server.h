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
#ifndef NLSTK_PORT_SERVER_H
#define NLSTK_PORT_SERVER_H

#include "nlstk_port_def.h"
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 添加或更新端口信息
 *
 * 该函数用于通知本端设备新增或更新端口信息从而更新Port服务属性。
 *
 * 当Port服务未创建时，创建Port服务并添加端口信息属性。
 *
 * @param[in] uuid 端口号对应的通用唯一标识
 * @param[in] manufactureId 端口号对应的厂商标识
 * @param[in] portId 端口号
 *
 * @return uint32_t
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E NLSTK_PortAddByUuid(NLSTK_SsapUuid_S *uuid, uint16_t manufactureId, uint16_t portId);

/**
 * @brief 删除端口信息
 *
 * 该函数用于通知本端设备删除UUID对应的端口信息从而更新Port服务属性。
 *
 * @param[in] uuid 端口号对应的通用唯一标识
 *
 * @return uint32_t
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E NLSTK_PortDeleteByUuid(NLSTK_SsapUuid_S *uuid);

#ifdef __cplusplus
}
#endif
#endif