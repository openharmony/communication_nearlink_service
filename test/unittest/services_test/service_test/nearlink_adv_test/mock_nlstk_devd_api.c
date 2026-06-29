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
#include "nlstk_public_define_ext.h"

#include "nlstk_devd_api.h"

static uint8_t g_handle = 0;

uint8_t NLSTK_DevdCreateAdvHandle(NLSTK_DevdAdvEventCbk cbk)
{
    uint8_t handle = g_handle;
    g_handle++;
    return handle;
}

NLSTK_Errcode_E NLSTK_DevdRemoveAdv(uint8_t *setAdvHandle)
{
    if (*setAdvHandle == 0xFF) {
        g_handle = 0;
    }
    return NLSTK_ERRCODE_SUCCESS;
}