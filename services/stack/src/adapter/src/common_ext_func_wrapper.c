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

#include "common_ext_func_wrapper.h"

#include <stdio.h>
#include "common_reg_ext_func.h"
#include "adapter_log.h"

bool COMMON_IsSupportConnHighSpeed(void)
{
    COMMON_ExtFuncList *funcList = COMMON_GetExtFuncList();
    if (funcList == NULL || funcList->isSupportConnHighSpeed == NULL) {
        ADAPTER_LOGW("get func isSupportConnHighSpeed failed");
        return false;
    }

    return funcList->isSupportConnHighSpeed();
}

bool COMMON_IsSupportQueryQosInfo(void)
{
    COMMON_ExtFuncList *funcList = COMMON_GetExtFuncList();
    if (funcList == NULL || funcList->isSupportQueryQosInfo == NULL) {
        ADAPTER_LOGW("get func isSupportQueryQosInfo failed");
        return false;
    }

    return funcList->isSupportQueryQosInfo();
}

bool COMMON_IsSupportSetMaxInterval(void)
{
    COMMON_ExtFuncList *funcList = COMMON_GetExtFuncList();
    if (funcList == NULL || funcList->isSupportSetMaxInterval == NULL) {
        ADAPTER_LOGW("get func isSupportSetMaxInterval failed");
        return false;
    }

    return funcList->isSupportSetMaxInterval();
}

bool COMMON_IsSupportConnFramePowerLevel(void)
{
    COMMON_ExtFuncList *funcList = COMMON_GetExtFuncList();
    if (funcList == NULL || funcList->isSupportConnFramePowerLevel == NULL) {
        ADAPTER_LOGW("get func isSupportConnFramePowerLevel failed");
        return false;
    }

    return funcList->isSupportConnFramePowerLevel();
}

bool COMMON_IsSupportScanFilter(void)
{
    COMMON_ExtFuncList *funcList = COMMON_GetExtFuncList();
    if (funcList == NULL || funcList->isSupportScanFilter == NULL) {
        ADAPTER_LOGW("get func isSupportScanFilter failed");
        return false;
    }

    return funcList->isSupportScanFilter();
}
