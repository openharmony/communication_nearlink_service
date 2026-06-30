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
#include "stack_cfgdb_mock.h"

using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
void *g_stackCfgdbMock;
 
CfgdbMock::CfgdbMock()
{
    g_stackCfgdbMock = reinterpret_cast<void *>(this);
}
 
CfgdbMock::~CfgdbMock()
{
    g_stackCfgdbMock = nullptr;
}
 
static CfgdbMockInterface *CfgdbMock()
{
    return reinterpret_cast<CfgdbMockInterface *>(g_stackCfgdbMock);
}
 
extern "C" {

uint32_t NLSTK_CfgdbGetPublicAddress(SLE_Addr_S *addr)
{
    return CfgdbMock()->NLSTK_CfgdbGetPublicAddress(addr);
}

}
} // namespace OHOS