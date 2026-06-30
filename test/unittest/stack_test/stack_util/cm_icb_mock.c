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

#include "cm_icb_api.h"
#include "cm_icb_init.h"
#include "cm_errno.h"

uint32_t CM_ICBInit(void)
{
    return CM_SUCCESS;
}

uint32_t CM_ICBDeinit(void)
{
    return CM_SUCCESS;
}

bool CM_ICBIsInited(void)
{
    return true;
}

uint32_t CM_ICBEnable(void)
{
    return CM_SUCCESS;
}

uint32_t CM_ICBDisable(void)
{
    return CM_SUCCESS;
}

uint32_t CM_ListenFreqBandSwitchEvent(CM_FreqBandListener listener)
{
    return CM_SUCCESS;
}

uint32_t CM_UnlistenFreqBandSwitchEvent(CM_FreqBandListener listener)
{
    return CM_SUCCESS;
}