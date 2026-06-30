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

#include <gtest/gtest.h>
#include <securec.h>
 
#include "stack_dli_mock.h"

using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
void *g_stackDliMock;
 
DliMock::DliMock()
{
    g_stackDliMock = reinterpret_cast<void *>(this);
}
 
DliMock::~DliMock()
{
    g_stackDliMock = nullptr;
}
 
static DliMockInterface *DliMock()
{
    return reinterpret_cast<DliMockInterface *>(g_stackDliMock);
}
 
extern "C" {

uint32_t DLI_CmdCbkReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    return DliMock()->DLI_CmdCbkReg(module, innerTable, innerSize, table, size);
}

void DLI_CmdCbkUnReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    return DliMock()->DLI_CmdCbkUnReg(module, innerTable, innerSize, table, size);
}

uint32_t DLI_SetMeasureParam(DLI_SetMeasureConfigParam *param)
{
    return DliMock()->DLI_SetMeasureParam(param);
}

uint32_t DLI_SetMeasureEnable(DLI_SetMeasureEnableParam *param)
{
    return DliMock()->DLI_SetMeasureEnable(param);
}

uint32_t DLI_ReadRemoteMeasureCaps(DLI_ReadRemoteMeasureCapsParam *param)
{
    return DliMock()->DLI_ReadRemoteMeasureCaps(param);
}

uint32_t DLI_EnableIMGEncryption(DLI_IMGEncryptParam *param)
{
    return DliMock()->DLI_EnableIMGEncryption(param);
}

uint32_t DLI_EnableEncryption(DLI_EnableEncryptParam *param)
{
    return 0;
}

uint32_t DLI_SetControllerData(DLI_ControllerData *data)
{
    return 0;
}

uint32_t DLI_EncryptionParamReqNegativeReply(DLI_ConnHandleStru *param)
{
    return 0;
}

uint32_t DLI_EncryptionParamReqReply(DLI_EncryptReqReplyParam *param)
{
    return 0;
}

bool DLI_IsSupportNewDisMeasure()
{
    return false;
}
}
} // namespace OHOS