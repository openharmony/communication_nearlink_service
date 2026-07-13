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
#ifndef SLE_SERVICE_FFRT_LOG_H
#define SLE_SERVICE_FFRT_LOG_H

#include "log_util.h"

#ifdef HILOGF
#undef HILOGF
#endif

#ifdef HILOGE
#undef HILOGE
#endif

#ifdef HILOGW
#undef HILOGW
#endif

#ifdef HILOGI
#undef HILOGI
#endif

#ifdef HILOGD
#undef HILOGD
#endif

extern int GetFfrtQueueId(void);

#define HILOGD(fmt, ...)  NL_HILOG(HILOG_DEBUG, "[%{public}d]" fmt, GetFfrtQueueId(), ##__VA_ARGS__)
#define HILOGI(fmt, ...)  NL_HILOG(HILOG_INFO, "[%{public}d]" fmt, GetFfrtQueueId(), ##__VA_ARGS__)
#define HILOGW(fmt, ...)  NL_HILOG(HILOG_WARN, "[%{public}d]" fmt, GetFfrtQueueId(), ##__VA_ARGS__)
#define HILOGE(fmt, ...)  NL_HILOG(HILOG_ERROR, "[%{public}d]" fmt, GetFfrtQueueId(), ##__VA_ARGS__)
#define HILOGF(fmt, ...)  NL_HILOG(HILOG_FATAL, "[%{public}d]" fmt, GetFfrtQueueId(), ##__VA_ARGS__)

#endif // SLE_SERVICE_FFRT_LOG_H