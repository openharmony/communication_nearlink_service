/**
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "cm_dli_adapter.h"
#include "cm_errno.h"
#include "cm_icb_inner_api.h"
#include "cm_log.h"
#include "dli.h"
#include "dli_errno.h"
#include "dli_layer.h"
#include "dli_opcode.h"
#include "sle_logic_link_mgr.h"

static CM_DliCbk g_dliCbk[CM_DLI_ADAPTER_MODULE_MAX][CM_DLI_ADAPTER_TYPE_MAX] = {};
static bool g_dliAdapterInited = false;

static void CM_DLIConnectCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    for (uint32_t i = 0; i < CM_DLI_ADAPTER_MODULE_MAX; i++) {
        if (g_dliCbk[i][CM_DLI_ADAPTER_CONNECT] != NULL) {
            g_dliCbk[i][CM_DLI_ADAPTER_CONNECT](context, status, cmdRes);
        }
    }
}

static void CM_DLIDisconnectCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    for (uint32_t i = 0; i < CM_DLI_ADAPTER_MODULE_MAX; i++) {
        if (g_dliCbk[i][CM_DLI_ADAPTER_DISCONNECT] != NULL) {
            g_dliCbk[i][CM_DLI_ADAPTER_DISCONNECT](context, status, cmdRes);
        }
    }
}

static const struct DLI_CbkLineStru g_dliCbks[] = {
    {DLI_CBK_CONNECT, CM_DLIConnectCbk},                     // acb connect evt
    {DLI_CBK_DISCONNECT, CM_DLIDisconnectCbk},               // acb and icb disconnect evt
};

uint32_t CM_DliAdapterInit(void)
{
    if (g_dliAdapterInited) {
        CM_LOGI("dli adapter has been inited");
        return CM_SUCCESS;
    }
    uint32_t ret = DLI_CmdCbkReg(CM_COMMON, NULL, 0,
        g_dliCbks, sizeof(g_dliCbks) / sizeof(struct DLI_CbkLineStru));
    if (ret != DLI_SUCCESS) {
        CM_LOGE("dli register cbk failed, ret=%u", ret);
        return CM_FAIL;
    }
    g_dliAdapterInited = true;
    CM_LOGI("dli adapter init success");
    return CM_SUCCESS;
}

uint32_t CM_DliAdapterDeinit(void)
{
    if (!g_dliAdapterInited) {
        CM_LOGI("dli adapter has been deinited");
        return CM_SUCCESS;
    }
    DLI_CmdCbkUnReg(CM_COMMON, NULL, 0, g_dliCbks, sizeof(g_dliCbks) / sizeof(struct DLI_CbkLineStru));
    g_dliAdapterInited = false;
    CM_LOGI("dli adapter deinit success");
    return CM_SUCCESS;
}

uint32_t CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE module, CM_DLI_ADAPTER_TYPE type, CM_DliCbk cbk)
{
    if (!g_dliAdapterInited) {
        CM_LOGE("dli adapter has not been inited");
        return CM_NOT_INITED;
    }
    if (module < CM_DLI_ADAPTER_CM || module >= CM_DLI_ADAPTER_MODULE_MAX ||
        type < CM_DLI_ADAPTER_CONNECT || type >= CM_DLI_ADAPTER_TYPE_MAX || cbk == NULL) {
        CM_LOGE("invalid module %d or invalid type %d or cbk is null", module, type);
        return CM_INVALID_PARAM_ERR;
    }
    g_dliCbk[module][type] = cbk;
    return CM_SUCCESS;
}

uint32_t CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE module, CM_DLI_ADAPTER_TYPE type)
{
    if (!g_dliAdapterInited) {
        CM_LOGE("dli adapter has not been inited");
        return CM_NOT_INITED;
    }
    if (module < CM_DLI_ADAPTER_CM || module >= CM_DLI_ADAPTER_MODULE_MAX ||
        type < CM_DLI_ADAPTER_CONNECT || type >= CM_DLI_ADAPTER_TYPE_MAX) {
        CM_LOGE("invalid module %d or invalid type %d", module, type);
        return CM_INVALID_PARAM_ERR;
    }
    g_dliCbk[module][type] = NULL;
    return CM_SUCCESS;
}