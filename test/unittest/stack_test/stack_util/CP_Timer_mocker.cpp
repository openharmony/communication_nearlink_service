/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "sdf_worker.h"
#include "cp_worker.h"
#include "CP_Timer_mocker.h"

static bool g_execCallbackAtOnce = false;
static bool g_execTimerAddFailed = false;

extern "C" uint32_t CP_TimerAdd(int *handle, SDF_TimerParam *param)
{
    if (g_execTimerAddFailed) {
        return 1;
    }
    if (g_execCallbackAtOnce) {
        if (param->callback != NULL) {
            param->callback(param->args);
        }
    }
    return 0;
}

extern "C" void CP_TimerDel(int handle)
{
    return;
}

void CP_TimerSetExecCallbackAtOnce(bool atOnce)
{
    g_execCallbackAtOnce = atOnce;
}

void CP_TimerSetAddFailed(bool failed)
{
    g_execTimerAddFailed = failed;
}