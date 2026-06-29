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
 * @file         cpfwk_log.h
 * @brief        CPFWK LOG head file.
 */
#ifndef CPFWK_LOG_H
#define CPFWK_LOG_H

#include "sdf_log_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CP_LOG_FATAL(fmt, args...)    SDF_PUB_LOG_FATAL(SDF_LOG_COMP_CP, fmt, ##args)
#define CP_LOG_ERROR(fmt, args...)    SDF_PUB_LOG_ERROR(SDF_LOG_COMP_CP, fmt, ##args)
#define CP_LOG_WARN(fmt, args...)     SDF_PUB_LOG_WARN(SDF_LOG_COMP_CP, fmt, ##args)
#define CP_LOG_INFO(fmt, args...)     SDF_PUB_LOG_INFO(SDF_LOG_COMP_CP, fmt, ##args)
#define CP_LOG_DEBUG(fmt, args...)    SDF_PUB_LOG_DEBUG(SDF_LOG_COMP_CP, fmt, ##args)

#define CP_CHECK_LOG_RETURN(checkCond, errorCode, fmt, args...)         \
    do {                                                                \
        if (!(checkCond)) {                                             \
            CP_LOG_ERROR(fmt, ##args);                                  \
            return errorCode;                                           \
        }                                                               \
    } while (0)

#define CP_CHECK_LOG_RETURN_VOID(checkCond, fmt, args...)               \
    do {                                                                \
        if (!(checkCond)) {                                             \
            CP_LOG_ERROR(fmt, ##args);                                  \
            return;                                                     \
        }                                                               \
    } while (0)

#define CP_CHECK_LOGD_RETURN_VOID(checkCond, fmt, args...)               \
    do {                                                                \
        if (!(checkCond)) {                                             \
            CP_LOG_DEBUG(fmt, ##args);                                  \
            return;                                                     \
        }                                                               \
    } while (0)


#define CP_CHECK_LOG_RETURN_ERR(checkCond, fmt, args...)               \
    do {                                                                \
        if (!(checkCond)) {                                             \
            CP_LOG_ERROR(fmt, ##args);                                  \
            return ERROR;                                                     \
        }                                                               \
    } while (0)


#ifdef __cplusplus
}
#endif

#endif /* CPFWK_LOG_H */
