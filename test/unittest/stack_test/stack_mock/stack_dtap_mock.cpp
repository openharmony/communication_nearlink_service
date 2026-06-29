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
 
#include "stack_dtap_mock.h"

using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
void *g_stackDtapMock;
 
DtapMock::DtapMock()
{
    g_stackDtapMock = reinterpret_cast<void *>(this);
}
 
DtapMock::~DtapMock()
{
    g_stackDtapMock = nullptr;
}
 
static DtapMockInterface *DtapMock()
{
    return reinterpret_cast<DtapMockInterface *>(g_stackDtapMock);
}
 
extern "C" {
uint32_t DTAP_RegisterDataRecvCb(uint8_t tcid, DTAP_DataRecvCb cb)
{
    return DtapMock()->DTAP_RegisterDataRecvCb(tcid, cb);
}

uint32_t DTAP_UnregisterDataRecvCb(uint8_t tcid)
{
    return DtapMock()->DTAP_UnregisterDataRecvCb(tcid);
}

uint32_t DTAP_DataSend(DTAP_Data_S *data)
{
    return DtapMock()->DTAP_DataSend(data);
}
}
} // namespace OHOS