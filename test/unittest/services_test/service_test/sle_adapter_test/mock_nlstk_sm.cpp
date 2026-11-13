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

#include "nlstk_sm_api.h"

// 裸进程无星闪栈调度线程，SM 下行投递恒败；本桩只替栈下行投递返回成功，不改用例期望值
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

NLSTK_Errcode_E NLSTK_SmRemovePairing(SLE_Addr_S *addrIn)
{
    (void)addrIn;
    return NLSTK_ERRCODE_SUCCESS;
}
