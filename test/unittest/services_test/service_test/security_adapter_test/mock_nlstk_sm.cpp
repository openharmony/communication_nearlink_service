/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// 测试目的：裸进程无星闪栈调度线程，SM 下行接口（StartPairing/SetPassCode/SetConfirm）经
// SchedulePostTask 投递恒败（实测 hilog：nlstk_sm_api.c "post task failed"），安全适配层
// （SleAdapter→SleSecurity）无法到达被测逻辑。按 mic_test/mock_mic_stack.cpp 惯例在测试
// 二进制内提供成功桩：仅替栈下行投递，不改任何用例期望值。
#include "nlstk_sm_api.h"

NLSTK_Errcode_E NLSTK_SmStartPairing(SLE_Addr_S *addrIn)
{
    (void)addrIn;
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SmSetConfirm(SLE_Addr_S *addrIn)
{
    (void)addrIn;
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SmSetPassCode(NLSTK_SmPassCode_S *passCodeIn)
{
    (void)passCodeIn;
    return NLSTK_ERRCODE_SUCCESS;
}
