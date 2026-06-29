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

#ifndef NLSTK_DEVD_DEF_EXT_H
#define NLSTK_DEVD_DEF_EXT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEVD_SCAN_FILTER_ID_MAX 32              /*!< filter过滤策略最多32路 */
#define DEVD_INVALID_FILTER_ID 0xFF

typedef struct {
    uint8_t subCode; /*!< 子命令 */
    uint8_t dataLen; /*!< 数据长度 */
    uint8_t data[0]; /*!< 数据 */
} NLSTK_DevdSleScanFilter_S;

#ifdef __cplusplus
}
#endif

#endif // NLSTK_DEVD_DEF_EXT_H