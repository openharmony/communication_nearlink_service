/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include "securec.h"
#include "sdf_struct.h"
#include "sdf_mem.h"
#include "dli.h"
#include "dli_cmd.h"
#include "stack_dli_stub.h"
#include "nlstk_log.h"
#include "cp_worker.h"

static DLI_ExecuteCmdCbk g_dliCmdCbk[DLI_CBK_MAX] = {0};

uint32_t TEST_DLI_CmdCbkReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    NLSTK_LOG_ERROR("[TLK] reg cmd cbk");
    for (uint32_t i = 0; i < size; i++) {
        uint16_t opcode = table[i].opcode;
        DLI_ExecuteCmdCbk func = table[i].func;
        if (func != NULL) {
            g_dliCmdCbk[opcode] = func;
        }
    }
    return 0;
}

uint32_t TEST_DLI_SetMeasureParam(DLI_SetMeasureConfigParam *param)
{
    return 0;
}

uint32_t TEST_DLI_SetMeasureEnable(DLI_SetMeasureEnableParam *param)
{
    return 0;
}

uint32_t TEST_DLI_ReadRemoteMeasureCaps(DLI_ReadRemoteMeasureCapsParam *param)
{
    return 0;
}

uint32_t TEST_DLI_EnableIMGEncryption(DLI_IMGEncryptParam *param)
{
    return 0;
}

void TEST_DLI_CmdCbkUnReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    memset_s(g_dliCmdCbk, sizeof(g_dliCmdCbk), 0, sizeof(g_dliCmdCbk));
    return;
}

void TEST_DLI_EventCbk(uint8_t opcode, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    if (g_dliCmdCbk[opcode] != NULL) {
        g_dliCmdCbk[opcode](NULL, status, cmdRes);
    }
}

static uint32_t TEST_PostTaskStubSuccess(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (cb != NULL) {
        cb(arg);
    }
    if (freeCb != NULL) {
        freeCb(arg);
    }
    return 0;
}
 
static uint32_t TEST_PostTaskBlockedStubSuccess(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    if (cb != NULL) {
        cb(arg);
    }
    if (freeCb != NULL) {
        freeCb(arg);
    }
    return 0;
}

static void TEST_DLI_RecvAcbHandler(uint16_t lcid, SDF_Buff_S *buf)
{
}

void TEST_DLI_Init()
{
    DLI_Callback dliCallback = {0};
    dliCallback.postOtherThread = CP_PostTask;
    dliCallback.postOtherBlockedThread = CP_PostTaskBlocked;
    dliCallback.dftReportKill = NULL;
    dliCallback.recvAcbHandler = TEST_DLI_RecvAcbHandler;
    DLI_SetCallback(&dliCallback);
    return;
}

void TEST_DLI_DeInit()
{
    return;
}
