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

#include "sdf_worker.h"
#include "securec.h"
#include "cp_worker.h"
#include "CP_Timer_mocker.h"

static bool g_execCallbackAtOnce = false;
static bool g_execTimerAddFailed = false;
static bool g_execCallbackPostponed = false; // 延后超时测试
static SDF_TimerParam g_timerParam = { 0 };

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
    if (g_execCallbackPostponed) {
        g_timerParam = *param;
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

void CP_TimerSetExecCallbackPostponed(bool postponed)
{
    g_execCallbackPostponed = postponed;
    if (!g_execCallbackPostponed) {
        (void)memset_s(&g_timerParam, sizeof(SDF_TimerParam), 0, sizeof(SDF_TimerParam));
    }
}

void CP_TimerPostponedTimeout(void)
{
    if (g_execCallbackPostponed) {
        if (g_timerParam.callback != NULL) {
            g_timerParam.callback(g_timerParam.args);
        }
    }
}

void CP_TimerSetAddFailed(bool failed)
{
    g_execTimerAddFailed = failed;
}