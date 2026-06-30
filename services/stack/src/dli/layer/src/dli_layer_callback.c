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

#include "dli_layer_callback.h"

#include "securec.h"

#include "dli_log.h"
#include "dli_errno.h"
#include "dli.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DLI_LayerCallback {
    DLI_WriteFileHandler writeFile;
    DLI_RecvEventHandler recvEvent;
    DLI_Callback dliCbk;
};

static struct DLI_LayerCallback g_callback = {
    .writeFile = NULL,
    .recvEvent = NULL,
    .dliCbk.dftReportKill = NULL,
    .dliCbk.postOtherThread = NULL,
    .dliCbk.recvAcbHandler = NULL,
    .dliCbk.hadmProcessCsCaps = NULL,
};

static DLI_DataNumChangecbk g_dataNumChangeCbk = NULL;

void DLI_SetWriteFileCallback(DLI_WriteFileHandler handler)
{
    g_callback.writeFile = handler;
    DLI_LOGI("DLI_SetWriteFileCallback %s", handler == NULL ? "disable" : "enable");
}

void DLI_SetRecvEventCallback(DLI_RecvEventHandler handler)
{
    g_callback.recvEvent = handler;
    DLI_LOGI("DLI_SetRecvEventCallback %s", handler == NULL ? "disable" : "enable");
}

uint32_t DLI_SetCallback(const DLI_Callback *callback)
{
    DLI_LOGI("DLI_SetCallback %s", callback == NULL ? "disable" : "enable");
    if (callback == NULL) {
        (void)memset_s(&g_callback.dliCbk, sizeof(DLI_Callback), 0, sizeof(DLI_Callback));
        return 0;
    }
    if (callback->postOtherThread == NULL ||
        callback->postOtherBlockedThread == NULL ||
        callback->recvAcbHandler == NULL) {
        DLI_LOGE("params is null");
        return DLI_STACK_PARAMS_ERRNO;
    }

    g_callback.dliCbk = *callback;
    return 0;
}

void DLI_FileWriteHandler(uint16_t type, const uint8_t *data, uint32_t len, int result)
{
    if (g_callback.writeFile == NULL) {
        return;
    }
    g_callback.writeFile(type, data, len, result);
}

void DLI_AcbRecvHander(uint16_t lcid, SDF_Buff_S *buf)
{
    DLI_CHECK_RETURN(g_callback.dliCbk.recvAcbHandler, "recvAcb is null");
    g_callback.dliCbk.recvAcbHandler(lcid, buf);
}

void DLI_EventRecvHandler(uint16_t event, void *context, const uint8_t *data, uint32_t len)
{
    DLI_CHECK_RETURN(g_callback.recvEvent, "recvEvent is null");
    g_callback.recvEvent(event, context, data, len);
}

void DLI_DftReportKill(uint16_t switchType)
{
    if (g_callback.dliCbk.dftReportKill == NULL) {
        return;
    }

    g_callback.dliCbk.dftReportKill(switchType);
}

DLI_Callback *DLI_GetCallback(void)
{
    return &g_callback.dliCbk;
}

void DLI_DataNumChangeRegister(DLI_DataNumChangecbk cbk)
{
    g_dataNumChangeCbk = cbk;
    DLI_LOGI("register callback success");
}

void DLI_DataNumChange(DLI_DataType type, uint16_t dataNum)
{
    if (g_dataNumChangeCbk != NULL) {
        g_dataNumChangeCbk(type, dataNum);
    }
}

#ifdef __cplusplus
}
#endif