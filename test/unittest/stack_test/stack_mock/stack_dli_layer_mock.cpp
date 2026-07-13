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
 
#include "stack_dli_layer_mock.h"

using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
void *g_stackDliLayerMock;
 
DliLayerMock::DliLayerMock()
{
    g_stackDliLayerMock = reinterpret_cast<void *>(this);
}

DliLayerMock::~DliLayerMock()
{
    g_stackDliLayerMock = nullptr;
}

static DliLayerMockInterface *DliLayerMock()
{
    return reinterpret_cast<DliLayerMockInterface *>(g_stackDliLayerMock);
}

extern "C" {
uint32_t DLI_GetDataFragmentNums(SDF_Buff_S *buf)
{
    return DliLayerMock()->DLI_GetDataFragmentNums(buf);
}

uint32_t DLI_GetFragmentMaxLen(void)
{
    return DliLayerMock()->DLI_GetFragmentMaxLen();
}

uint32_t DLI_SplitData(DLI_DataStru *data, SDF_Buff_S *fragmentBuf[], uint32_t fragmentCnt)
{
    return DliLayerMock()->DLI_SplitData(data, fragmentBuf, fragmentCnt);
}

uint32_t DLI_DataSend(DLI_DataStru *data)
{
    return DliLayerMock()->DLI_DataSend(data);
}

void DLI_PostNextTask(DLI_TaskType type)
{
    return DliLayerMock()->DLI_PostNextTask(type);
}

uint32_t DLI_CmdSend(DLI_CmdStru *cmd)
{
    return DliLayerMock()->DLI_CmdSend(cmd);
}
}
} // namespace OHOS