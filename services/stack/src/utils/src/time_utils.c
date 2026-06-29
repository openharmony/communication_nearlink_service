/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "time_utils.h"
#include <time.h>
#include "utils_log.h"

#define DP_TIME_UNIT_MS 1000
#define DP_TIME_UNIT_US 1000000
#define DP_TIME_UNIT_NS 1000000000

uint64_t DP_GetRealTimeMs(void)
{
    struct timespec time = {};
    if (clock_gettime(CLOCK_REALTIME, &time) != EOK) {
        UTILS_LOGE("get real times failed");
    }
    return time.tv_sec * DP_TIME_UNIT_MS + time.tv_nsec / DP_TIME_UNIT_US;
}

uint64_t DP_GetMonoTimeMs(void)
{
    struct timespec time = {};
    if (clock_gettime(CLOCK_MONOTONIC, &time) != EOK) {
        UTILS_LOGE("get mono times failed");
    }
    return time.tv_sec * DP_TIME_UNIT_MS + time.tv_nsec / DP_TIME_UNIT_US;
}