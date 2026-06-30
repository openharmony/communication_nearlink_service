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

#ifndef SDF_LOG_H
#define SDF_LOG_H

#include "sdf_log_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NAI_LOG_MODULE "NAI"

#define NAI_LOG_FATAL(fmt, args...)     SDF_PUB_LOG_FATAL(NAI_LOG_MODULE, fmt, ##args)
#define NAI_LOG_ERROR(fmt, args...)     SDF_PUB_LOG_ERROR(NAI_LOG_MODULE, fmt, ##args)
#define NAI_LOG_WARN(fmt, args...)      SDF_PUB_LOG_WARN(NAI_LOG_MODULE, fmt, ##args)
#define NAI_LOG_INFO(fmt, args...)      SDF_PUB_LOG_INFO(NAI_LOG_MODULE, fmt, ##args)
#define NAI_LOG_DEBUG(fmt, args...)     SDF_PUB_LOG_DEBUG(NAI_LOG_MODULE, fmt, ##args)

#define NAI_CHECK_LOG_RETURN(checkCond, errorCode, fmt, args...)        \
    do {                                                                \
        if (!(checkCond)) {                                             \
            NAI_LOG_ERROR(fmt, ##args);                                 \
            return errorCode;                                           \
        }                                                               \
    } while (0)

#define NAI_CHECK_LOG_RETURN_VOID(checkCond, fmt, args...)              \
    do {                                                                \
        if (!(checkCond)) {                                             \
            NAI_LOG_ERROR(fmt, ##args);                                 \
            return;                                                     \
        }                                                               \
    } while (0)


#ifdef __cplusplus
}
#endif

#endif /* SDF_LOG_H */
