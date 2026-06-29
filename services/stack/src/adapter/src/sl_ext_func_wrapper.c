/**
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

#include "sl_ext_func_wrapper.h"

#include <stdio.h>
#include "sl_reg_ext_func.h"
#include "adapter_log.h"

uint32_t SL_HostInit(void)
{
    SL_ExtFuncList *funcList = SL_GetExtFuncList();
    if (funcList == NULL || funcList->hostInit == NULL) {
        ADAPTER_LOGW("get func hostInit failed");
        return 0;
    }

    return funcList->hostInit();
}

void SL_HostDeinit(void)
{
    SL_ExtFuncList *funcList = SL_GetExtFuncList();
    if (funcList == NULL || funcList->hostDeinit == NULL) {
        ADAPTER_LOGW("get func hostDeinit failed");
        return;
    }

    return funcList->hostDeinit();
}
