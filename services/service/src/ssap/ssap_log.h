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
#ifndef SSAP_LOG_H
#define SSAP_LOG_H

#include "log_util.h"
#include "SleServiceFfrtLog.h"

#define SSAP_LOG_TAG "[SSAP]"

#define SSAP_LOGD(fmt, ...) \
    HILOGD(SSAP_LOG_TAG fmt, ##__VA_ARGS__)

#define SSAP_LOGI(fmt, ...) \
    HILOGI(SSAP_LOG_TAG fmt, ##__VA_ARGS__)

#define SSAP_LOGW(fmt, ...) \
    HILOGW(SSAP_LOG_TAG fmt, ##__VA_ARGS__)

#define SSAP_LOGE(fmt, ...) \
    HILOGE(SSAP_LOG_TAG fmt, ##__VA_ARGS__)

#define SSAP_LOGF(fmt, ...) \
    HILOGF(SSAP_LOG_TAG fmt, ##__VA_ARGS__)

#define ADDR_LOG(rawAddress) (GetEncryptAddr((rawAddress).GetAddress()).c_str())

#endif // SSAP_LOG_H