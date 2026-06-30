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
#include "cm_common_fuzzer.h"

#include "cm_dli_adapter.h"
#include "sdf_worker.h"
#include "common_ext_func_wrapper.h"

static void DliAdapterCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    return;
}

void FuzzCmDliAdapterInit(void)
{
    CM_DliAdapterInit();
}

void FuzzCmDliAdapterDeinit(void)
{
    CM_DliAdapterDeinit();
}

void FuzzCmRegisterDliAdapterCbk(void)
{
    CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_DISCONNECT, DliAdapterCbk);
}

void FuzzCmUnregisterDliAdapterCbk(void)
{
    CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_DISCONNECT);
}

void FuzzCmDliAdaptor(uint8_t *data, size_t size)
{
    FuzzCmDliAdapterInit();
    FuzzCmRegisterDliAdapterCbk();

    FuzzCmUnregisterDliAdapterCbk();
    FuzzCmDliAdapterDeinit();

    FuzzCmDliAdapterInit();
    FuzzCmRegisterDliAdapterCbk();
}