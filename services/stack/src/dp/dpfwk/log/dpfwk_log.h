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
#ifndef DPFWK_LOG_H
#define DPFWK_LOG_H

#include "sdf_log_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CM_TAG "CM"
#define MM_TAG "MM"
#define QOSM_TAG "QOSM"
#define DTAP_TAG "DTAP"
#define TP_TAG "TRANSPORT"
#define THREAD_TAG "THREAD"

#define DPFWK_LOGF(module, fmt, args...) SDF_PUB_LOG_FATAL(module, fmt, ##args)
#define DPFWK_LOGE(module, fmt, args...) SDF_PUB_LOG_ERROR(module, fmt, ##args)
#define DPFWK_LOGW(module, fmt, args...) SDF_PUB_LOG_WARN(module, fmt, ##args)
#define DPFWK_LOGI(module, fmt, args...) SDF_PUB_LOG_INFO(module, fmt, ##args)
#define DPFWK_LOGD(module, fmt, args...) SDF_PUB_LOG_DEBUG(module, fmt, ##args)

#define CM_LOGF(fmt, args...) DPFWK_LOGF(CM_TAG, fmt, ##args)
#define CM_LOGE(fmt, args...) DPFWK_LOGE(CM_TAG, fmt, ##args)
#define CM_LOGW(fmt, args...) DPFWK_LOGW(CM_TAG, fmt, ##args)
#define CM_LOGI(fmt, args...) DPFWK_LOGI(CM_TAG, fmt, ##args)
#define CM_LOGD(fmt, args...) DPFWK_LOGD(CM_TAG, fmt, ##args)

#define DTAP_LOGF(fmt, args...) DPFWK_LOGF(DTAP_TAG, fmt, ##args)
#define DTAP_LOGE(fmt, args...) DPFWK_LOGE(DTAP_TAG, fmt, ##args)
#define DTAP_LOGW(fmt, args...) DPFWK_LOGW(DTAP_TAG, fmt, ##args)
#define DTAP_LOGI(fmt, args...) DPFWK_LOGI(DTAP_TAG, fmt, ##args)
#define DTAP_LOGD(fmt, args...) DPFWK_LOGD(DTAP_TAG, fmt, ##args)

#define MM_LOGF(fmt, args...) DPFWK_LOGF(MM_TAG, fmt, ##args)
#define MM_LOGE(fmt, args...) DPFWK_LOGE(MM_TAG, fmt, ##args)
#define MM_LOGW(fmt, args...) DPFWK_LOGW(MM_TAG, fmt, ##args)
#define MM_LOGI(fmt, args...) DPFWK_LOGI(MM_TAG, fmt, ##args)
#define MM_LOGD(fmt, args...) DPFWK_LOGD(MM_TAG, fmt, ##args)

#define QOSM_LOGF(fmt, args...) DPFWK_LOGF(QOSM_TAG, fmt, ##args)
#define QOSM_LOGE(fmt, args...) DPFWK_LOGE(QOSM_TAG, fmt, ##args)
#define QOSM_LOGW(fmt, args...) DPFWK_LOGW(QOSM_TAG, fmt, ##args)
#define QOSM_LOGI(fmt, args...) DPFWK_LOGI(QOSM_TAG, fmt, ##args)
#define QOSM_LOGD(fmt, args...) DPFWK_LOGD(QOSM_TAG, fmt, ##args)

#define TP_LOGF(fmt, args...) DPFWK_LOGF(TP_TAG, fmt, ##args)
#define TP_LOGE(fmt, args...) DPFWK_LOGE(TP_TAG, fmt, ##args)
#define TP_LOGW(fmt, args...) DPFWK_LOGW(TP_TAG, fmt, ##args)
#define TP_LOGI(fmt, args...) DPFWK_LOGI(TP_TAG, fmt, ##args)
#define TP_LOGD(fmt, args...) DPFWK_LOGD(TP_TAG, fmt, ##args)

#define CHECK_AND_RETURN_RET_LOG(module, cond, ret, fmt, ...) \
    do {                                                           \
        if (!(cond)) {                                             \
            DPFWK_LOGE(module, fmt, ##__VA_ARGS__);                \
            return ret;                                            \
        }                                                          \
    } while (0)

#define CHECK_AND_RETURN_LOG(module, cond, fmt, ...) \
    do {                                                  \
        if (!(cond)) {                                    \
            DPFWK_LOGE(module, fmt, ##__VA_ARGS__);       \
            return;                                       \
        }                                                 \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif  // DPFWK_LOG_H