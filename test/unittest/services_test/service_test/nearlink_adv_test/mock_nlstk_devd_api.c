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
#include <string.h>

#define MAX_VALID_HANDLES 8

static uint8_t g_handle = 0;
static uint8_t g_valid_handles[MAX_VALID_HANDLES];
static int g_valid_handle_count = 0;

static int IsValidHandle(uint8_t handle)
{
    for (int i = 0; i < g_valid_handle_count; i++) {
        if (g_valid_handles[i] == handle) {
            return 1;
        }
    }
    return 0;
}

uint8_t NLSTK_DevdCreateAdvHandle(NLSTK_DevdAdvEventCbk cbk)
{
    uint8_t handle = g_handle;
    g_handle++;
    if (g_valid_handle_count < MAX_VALID_HANDLES) {
        g_valid_handles[g_valid_handle_count++] = handle;
    }
    return handle;
}

NLSTK_Errcode_E NLSTK_DevdRemoveAdv(uint8_t *setAdvHandle)
{
    if (*setAdvHandle == 0xFF) {
        g_handle = 0;
        g_valid_handle_count = 0;
    }
    for (int i = 0; i < g_valid_handle_count; i++) {
        if (g_valid_handles[i] == *setAdvHandle) {
            g_valid_handles[i] = g_valid_handles[g_valid_handle_count - 1];
            g_valid_handle_count--;
            break;
        }
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdStartAdv(NLSTK_DevdSetAdvParams_S *advParams)
{
    if (advParams == NULL) {
        return NLSTK_ERRCODE_POINTER_NULL;
    }
    if (!IsValidHandle(advParams->param.advHandle)) {
        return NLSTK_ERRCODE_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdSetAdvData(NLSTK_DevdSetAdvData_S *setAdvData)
{
    if (setAdvData == NULL) {
        return NLSTK_ERRCODE_POINTER_NULL;
    }
    if (!IsValidHandle(setAdvData->advHandle)) {
        return NLSTK_ERRCODE_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdEnableAdv(NLSTK_DevdSetAdvEnable_S *setEnable)
{
    if (setEnable == NULL) {
        return NLSTK_ERRCODE_POINTER_NULL;
    }
    if (!IsValidHandle(setEnable->advHandle)) {
        return NLSTK_ERRCODE_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdSetTxPower(NLSTK_DevdSetTxPower_S *setTxPower)
{
    return NLSTK_ERRCODE_SUCCESS;
}
