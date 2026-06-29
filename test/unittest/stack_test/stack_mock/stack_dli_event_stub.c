/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "stack_dli_event_stub.h"
#include "securec.h"

#include "dli_errno.h"

static DLI_NOCPEventCbk g_testEventCbk[DLI_REG_MODULE_MAX] = {0};

void TEST_DliEventInit(void)
{
    (void)memset_s(g_testEventCbk, sizeof(g_testEventCbk), 0 , sizeof(g_testEventCbk));
}

void TEST_DliEventDeInit(void)
{
    (void)memset_s(g_testEventCbk, sizeof(g_testEventCbk), 0 , sizeof(g_testEventCbk));
}

uint32_t TEST_DLI_RegNOCPEventCbk(DLI_RegModuleType module, DLI_NOCPEventCbk cbk)
{
    if (module >= DLI_REG_MODULE_MAX || module < 0 || cbk == NULL) {
        return DLI_INVALID_PARAMETERS;
    }
    g_testEventCbk[module] = cbk;
    return DLI_SUCCESS;
}

uint32_t TEST_DLI_UnregNOCPEventCbk(DLI_RegModuleType module)
{
    if (module >= DLI_REG_MODULE_MAX || module < 0) {
        return DLI_INVALID_PARAMETERS;
    }
    g_testEventCbk[module] = NULL;
    return DLI_SUCCESS;
}

uint32_t TEST_NOCPEventDo(DLI_RegModuleType module, uint16_t connHandle, uint8_t numCompletedPackets)
{
    if (module >= DLI_REG_MODULE_MAX || module < 0 || g_testEventCbk[module] == NULL) {
        return DLI_INVALID_PARAMETERS;
    }
    g_testEventCbk[module](connHandle, numCompletedPackets);
    return DLI_SUCCESS;
}