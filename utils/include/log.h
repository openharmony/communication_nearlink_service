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
#ifndef LOG_H
#define LOG_H

#ifndef LOG_DOMAIN
#define LOG_DOMAIN 0xD000150
#endif

#ifndef LOG_TAG
#define LOG_TAG "nearlink"
#endif

#include "securec.h"
#include "hilog/log.h"

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

#ifndef IS_NL_RELEASE_VERSION
#define NL_HILOG(log, fmt, args...)                                                                              \
    do {                                                                                                         \
        log(LOG_CORE, "[%{public}s(%{public}s:%{public}d)]" fmt, __FILE_NAME__, __FUNCTION__, __LINE__, ##args); \
    } while (0)
#else
#define NL_HILOG(log, fmt, args...)                                                                              \
    do {                                                                                                         \
        log(LOG_CORE, "[%{public}s:%{public}d]" fmt, __FUNCTION__, __LINE__, ##args);                            \
    } while (0)
#endif

#define HILOGD(fmt, ...) NL_HILOG(HILOG_DEBUG, fmt, ##__VA_ARGS__)
#define HILOGI(fmt, ...) NL_HILOG(HILOG_INFO, fmt, ##__VA_ARGS__)
#define HILOGW(fmt, ...) NL_HILOG(HILOG_WARN, fmt, ##__VA_ARGS__)
#define HILOGE(fmt, ...) NL_HILOG(HILOG_ERROR, fmt, ##__VA_ARGS__)
#define HILOGF(fmt, ...) NL_HILOG(HILOG_FATAL, fmt, ##__VA_ARGS__)

#ifdef LOG_DEBUG
#undef LOG_DEBUG
#endif

#ifdef LOG_INFO
#undef LOG_INFO
#endif

#ifdef LOG_WARN
#undef LOG_WARN
#endif

#ifdef LOG_ERROR
#undef LOG_ERROR
#endif

#ifdef LOG_FATAL
#undef LOG_FATAL
#endif

#define ALOGV(...) HILOGD(__VA_ARGS__)
#define ALOGD(...) HILOGD(__VA_ARGS__)
#define ALOGI(...) HILOGI(__VA_ARGS__)
#define ALOGW(...) HILOGW(__VA_ARGS__)
#define ALOGE(...) HILOGE(__VA_ARGS__)

#define LOG_VERBOSE(...) HILOGD(__VA_ARGS__)
#define LOG_DEBUG(...) HILOGD(__VA_ARGS__)
#define LOG_INFO(...) HILOGI(__VA_ARGS__)
#define LOG_WARN(...) HILOGW(__VA_ARGS__)
#define LOG_ERROR(...) HILOGE(__VA_ARGS__)

#ifndef LOG_EVENT_INT
#define LOG_EVENT_INT(tag, subTag) LOG_ERROR("ERROR tag num: 0x%x, opcode: %ld", tag, subTag)
#endif


#ifdef NL_CHECK_RETURN
#undef NL_CHECK_RETURN
#endif

#define NL_CHECK_RETURN(cond, fmt, ...)             \
    do {                                            \
        if (!(cond)) {                              \
            HILOGE(fmt, ##__VA_ARGS__);             \
            return;                                 \
        }                                           \
    } while (0)

#ifdef NL_CHECK_RETURN_RET
#undef NL_CHECK_RETURN_RET
#endif

#define NL_CHECK_RETURN_RET(cond, ret, fmt, ...)                    \
    do {                                                            \
        if (!(cond)) {                                              \
            HILOGE(fmt, ##__VA_ARGS__);                             \
            return ret;                                             \
        }                                                           \
    } while (0)

#ifdef NL_CHECK_RETURN_LOGD
#undef NL_CHECK_RETURN_LOGD
#endif

#define NL_CHECK_RETURN_LOGD(cond, fmt, ...)                        \
    do {                                                            \
        if (!(cond)) {                                              \
            HILOGD(fmt, ##__VA_ARGS__);                             \
            return;                                                 \
        }                                                           \
    } while (0)

#ifdef NL_CHECK_RETURN_LOGD_RET
#undef NL_CHECK_RETURN_LOGD_RET
#endif

#define NL_CHECK_RETURN_LOGD_RET(cond, ret, fmt, ...)               \
    do {                                                            \
        if (!(cond)) {                                              \
            HILOGD(fmt, ##__VA_ARGS__);                             \
            return ret;                                             \
        }                                                           \
    } while (0)

#endif