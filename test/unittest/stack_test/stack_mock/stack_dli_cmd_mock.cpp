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

#include <gtest/gtest.h>
#include <securec.h>

#include "stack_dli_cmd_mock.h"

#include "dli_def.h"
#include "dli_cmd_struct.h"

using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
void *g_stackDliCmdMock;
 
DliCmdMock::DliCmdMock()
{
    g_stackDliCmdMock = reinterpret_cast<void *>(this);
}
 
DliCmdMock::~DliCmdMock()
{
    g_stackDliCmdMock = nullptr;
}
 
static DliCmdMockInterface *DliCmdMock()
{
    return reinterpret_cast<DliCmdMockInterface *>(g_stackDliCmdMock);
}
 
extern "C" {

uint32_t DLI_GetPublicAddress(DLI_PublicAddrParam *param)
{
    return 0;
}

uint32_t DLI_ReadLocalFeatures(void)
{
    return DliCmdMock()->DLI_ReadLocalFeatures();
}

uint32_t DLI_ReadLocalVersion(void)
{
    return DliCmdMock()->DLI_ReadLocalVersion();
}

uint32_t DLI_ReadMaximumAdvDataLen(void)
{
    return DliCmdMock()->DLI_ReadMaximumAdvDataLen();
}

uint32_t DLI_ReadAdvSetsNum(void)
{
    return DliCmdMock()->DLI_ReadAdvSetsNum();
}

uint32_t DLI_ReadSupportCryptoAlgo(void)
{
    return DliCmdMock()->DLI_ReadSupportCryptoAlgo();
}

void DLI_SetLocalFeatures(DLI_LocalFeatures_S *features)
{

}

uint32_t DLI_ReadLocalMeasureCaps(void)
{
    return 0;
}

bool DLI_IsSupportNewDisMeasure(void)
{
    return false;
}

}
}