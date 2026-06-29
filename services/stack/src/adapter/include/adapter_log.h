/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef ADAPTER_LOG_H
#define ADAPTER_LOG_H

#include "sdf_log_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADAPTER_LOG_MODULE "NAI"

#define ADAPTER_LOGF(fmt, args...)     SDF_PUB_LOG_FATAL(ADAPTER_LOG_MODULE, fmt, ##args)
#define ADAPTER_LOGE(fmt, args...)     SDF_PUB_LOG_ERROR(ADAPTER_LOG_MODULE, fmt, ##args)
#define ADAPTER_LOGW(fmt, args...)      SDF_PUB_LOG_WARN(ADAPTER_LOG_MODULE, fmt, ##args)
#define ADAPTER_LOGI(fmt, args...)      SDF_PUB_LOG_INFO(ADAPTER_LOG_MODULE, fmt, ##args)
#define ADAPTER_LOGD(fmt, args...)     SDF_PUB_LOG_DEBUG(ADAPTER_LOG_MODULE, fmt, ##args)

#ifdef __cplusplus
}
#endif

#endif /* ADAPTER_LOG_H */
