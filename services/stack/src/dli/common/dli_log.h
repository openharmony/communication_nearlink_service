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
#ifndef DLI_LOG_H
#define DLI_LOG_H

#include "sdf_log_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DLI_TAG "DLI"

#define DLI_LOGF(fmt, args...) SDF_PUB_LOG_FATAL(DLI_TAG, fmt, ##args)
#define DLI_LOGE(fmt, args...) SDF_PUB_LOG_ERROR(DLI_TAG, fmt, ##args)
#define DLI_LOGW(fmt, args...) SDF_PUB_LOG_WARN(DLI_TAG, fmt, ##args)
#define DLI_LOGI(fmt, args...) SDF_PUB_LOG_INFO(DLI_TAG, fmt, ##args)
#define DLI_LOGD(fmt, args...) SDF_PUB_LOG_DEBUG(DLI_TAG, fmt, ##args)

#define DLI_CHECK_RETURN_RET(cond, ret, fmt, ...)         \
    do {                                                  \
        if (!(cond)) {                                    \
            DLI_LOGE(fmt, ##__VA_ARGS__);                 \
            return ret;                                   \
        }                                                 \
    } while (0)

#define DLI_CHECK_RETURN(cond, fmt, ...)                  \
    do {                                                  \
        if (!(cond)) {                                    \
            DLI_LOGE(fmt, ##__VA_ARGS__);                 \
            return;                                       \
        }                                                 \
    } while (0)

#define DLI_CHECK_RETURN_NULL(cond, fmt, ...)             \
    do {                                                  \
        if (!(cond)) {                                    \
            DLI_LOGE(fmt, ##__VA_ARGS__);                 \
            return NULL;                                  \
        }                                                 \
    } while (0)
#ifdef __cplusplus
}
#endif

#endif  // DPFWK_LOG_H