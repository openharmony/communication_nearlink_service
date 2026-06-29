/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "cm_access_mgr.h"
#include "securec.h"
#include "cm_errno.h"
#include "cm_log.h"
#include "dli.h"
#include "dli_errno.h"

static CM_ExeCmdCbk g_cmAccessCbk[SLE_ACCESS_CBK_MAX] = { 0 };

void CM_AccessRegCbk(uint8_t type, CM_ExeCmdCbk cbk)
{
    CM_CHECK_RETURN(type < SLE_ACCESS_CBK_MAX, "access reg cbk type:%02x is invalid", type);
    g_cmAccessCbk[type] = cbk;
}

void CM_AccessUnRegAllCbk(void)
{
    for (uint32_t i = 0; i < SLE_ACCESS_CBK_MAX; i++) {
        g_cmAccessCbk[i] = NULL;
    }
}

CM_ExeCmdCbk CM_AccessGetCbk(uint8_t type)
{
    CM_CHECK_RETURN_RET(type < SLE_ACCESS_CBK_MAX, NULL, "access get cbk type:%02x, cmdCbk not found", type);
    return g_cmAccessCbk[type];
}

uint32_t CM_AccessRegDliCbks(const DLI_CbkLineStru *table, uint32_t size)
{
    uint32_t ret = DLI_CmdCbkReg(CM, NULL, 0, table, size);
    if (ret != DLI_SUCCESS) {
        CM_LOGE("DLI_CmdCbkReg ret:%u", ret);
        return CM_FAIL;
    }
    CM_LOGI("DLI_CmdCbkReg success");
    return CM_SUCCESS;
}

void CM_AccessUnRegDliCbk(const DLI_CbkLineStru *table, uint32_t size)
{
    DLI_CmdCbkUnReg(CM, NULL, 0, table, size);
    CM_LOGI("DLI_CmdCbkUnReg success");
}