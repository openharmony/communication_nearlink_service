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
 * this file contains the QOSM log definitions
 *
 ***************************************************************************/

#ifndef QOSM_LOG_H
#define QOSM_LOG_H

#include "sdf_log_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QOSM_TAG "QOSM"

#define QOSM_LOGF(fmt, args...) SDF_PUB_LOG_FATAL(QOSM_TAG, fmt, ##args)
#define QOSM_LOGE(fmt, args...) SDF_PUB_LOG_ERROR(QOSM_TAG, fmt, ##args)
#define QOSM_LOGW(fmt, args...) SDF_PUB_LOG_WARN(QOSM_TAG, fmt, ##args)
#define QOSM_LOGI(fmt, args...) SDF_PUB_LOG_INFO(QOSM_TAG, fmt, ##args)
#define QOSM_LOGD(fmt, args...) SDF_PUB_LOG_DEBUG(QOSM_TAG, fmt, ##args)

#define QOSM_CHECK_RETURN_RET(cond, ret, fmt, ...)            \
    do {                                                    \
        if (!(cond)) {                                      \
            QOSM_LOGE(fmt, ##__VA_ARGS__);                    \
            return ret;                                     \
        }                                                   \
    } while (0)

#define QOSM_CHECK_RETURN(cond, fmt, ...)                     \
    do {                                                    \
        if (!(cond)) {                                      \
            QOSM_LOGE(fmt, ##__VA_ARGS__);                    \
            return;                                         \
        }                                                   \
    } while (0)

#define QOSM_CHECK_RETURN_NULL(cond, fmt, ...)             \
    do {                                                  \
        if (!(cond)) {                                    \
            QOSM_LOGE(fmt, ##__VA_ARGS__);                 \
            return NULL;                                  \
        }                                                 \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif  // QOSM_LOG_H