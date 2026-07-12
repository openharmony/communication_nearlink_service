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
 * this file contains the CM log definitions
 *
 ***************************************************************************/

#ifndef CM_LOG_H
#define CM_LOG_H

#include "sdf_log_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CM_TAG "CM"

#define CM_LOGF(fmt, args...) SDF_PUB_LOG_FATAL(CM_TAG, fmt, ##args)
#define CM_LOGE(fmt, args...) SDF_PUB_LOG_ERROR(CM_TAG, fmt, ##args)
#define CM_LOGW(fmt, args...) SDF_PUB_LOG_WARN(CM_TAG, fmt, ##args)
#define CM_LOGI(fmt, args...) SDF_PUB_LOG_INFO(CM_TAG, fmt, ##args)
#define CM_LOGD(fmt, args...) SDF_PUB_LOG_DEBUG(CM_TAG, fmt, ##args)

#define CM_CHECK_RETURN_RET(cond, ret, fmt, ...)            \
    do {                                                    \
        if (!(cond)) {                                      \
            CM_LOGE(fmt, ##__VA_ARGS__);                    \
            return ret;                                     \
        }                                                   \
    } while (0)

#define CM_CHECK_RETURN(cond, fmt, ...)                     \
    do {                                                    \
        if (!(cond)) {                                      \
            CM_LOGE(fmt, ##__VA_ARGS__);                    \
            return;                                         \
        }                                                   \
    } while (0)

#define CM_CHECK_RETURN_NULL(cond, fmt, ...)             \
    do {                                                  \
        if (!(cond)) {                                    \
            CM_LOGE(fmt, ##__VA_ARGS__);                 \
            return NULL;                                  \
        }                                                 \
    } while (0)
#ifdef __cplusplus
}
#endif

#endif  // CM_LOG_H