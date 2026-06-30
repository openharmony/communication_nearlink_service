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

#include "stack_dli_event_mock.h"

using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
void *g_stackDliEventMock;
 
DliEventMock::DliEventMock()
{
    g_stackDliEventMock = reinterpret_cast<void *>(this);
}

DliEventMock::~DliEventMock()
{
    g_stackDliEventMock = nullptr;
}
 
static DliEventMockInterface *DliEventMock()
{
    return reinterpret_cast<DliEventMockInterface *>(g_stackDliEventMock);
}
 
extern "C" {
uint32_t DLI_RegNOCPEventCbk(DLI_RegModuleType module, DLI_NOCPEventCbk cbk)
{
    return DliEventMock()->DLI_RegNOCPEventCbk(module, cbk);
}

uint32_t DLI_UnregNOCPEventCbk(DLI_RegModuleType module)
{
    return DliEventMock()->DLI_UnregNOCPEventCbk(module);
}
}
} // namespace OHOS