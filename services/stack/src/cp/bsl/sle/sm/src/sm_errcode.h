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
#ifndef SM_ERRCODE_H
#define SM_ERRCODE_H

#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define SM_OK                           NLSTK_OK
// 模块内与模块间错误码
#define SM_SLINK_MAP_INIT_FAIL          1     /* SLINK容器初始化失败 */
#define SM_LOCAL_PARAMS_INIT_FAIL       2     /* 本端参数初始化失败 */
#define SM_REGISTER_DLI_CBK_FAIL        3     /* 注册DLI回调失败 */
#define SM_REGISTER_CM_LISTENER_FAIL    4     /* 注册CM监听失败 */

/**
 * @brief 配对失败原因，需要发送给对端
 */
typedef enum {
    SM_ERR_OK                               = 0x00,
    SM_ERR_AUTHENTICATION_REQUIREMENTS      = 0x01,             /* 设备的IO功能不满足认证要求 无法执行配对过程  */
    SM_ERR_PAIRING_NOT_SUPPORT              = 0x02,             /* 设备不支持配对 */
    SM_ERR_NUMBER_COMPARE                   = 0x03,             /* 指示数字比较中的确认值不匹配 */
    SM_ERR_PASSCODE                         = 0x04,             /* 接收到的确认码和本地计算的确认码不一致 */
    SM_ERR_PASSWORD_ENTRY                   = 0x05,             /* 接收到的确认码和本地计算的确认码不一致 */
    SM_ERR_OUT_OF_BAND                      = 0x06,             /* 带外数据不可用 */
    SM_ERR_PSK                              = 0x07,             /* 接收到的确认码和本地计算的确认码不一致 */
    SM_ERR_DHKEY_AUTHENTICODE_CHECK_FAILED  = 0x08,             /* 接收到的DH Key检查值与本地计算的值不匹配 */
    SM_ERR_INVALID_PARAMETERS               = 0x09,             /* 命令长度无效或参数超出指定范围 */
    SM_ERR_UNSPECIFIED_REASON               = 0x0A,             /* 由于未指定的原因，配对失败 */
    SM_ERR_REPEATED_ATTEMPTS                = 0x0B,             /* 不允许配对或身份鉴权，因为自上次配对失败以来经过的时间太短或失败次数过多 */
    SM_ERR_ACTIVE_CANCEL                    = 0x0C,             /* 主动取消导致配对中止 */
} SmErrcode_E;

#ifdef __cplusplus
}
#endif

#endif // SM_ERRCODE_H