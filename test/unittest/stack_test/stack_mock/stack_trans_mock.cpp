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

#include <gtest/gtest.h>
#include <securec.h>

#include "stack_trans_mock.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
void *g_stackTransMock;

TransMock::TransMock()
{
    g_stackTransMock = reinterpret_cast<void *>(this);
}

TransMock::~TransMock()
{
    g_stackTransMock = nullptr;
}

static TransMockInterface *TransMock()
{
    return reinterpret_cast<TransMockInterface *>(g_stackTransMock);
}

extern "C" {

uint32_t TRANS_RegisterCbks(const TRANS_Cbks_S *cbks)
{
    return TransMock()->TRANS_RegisterCbks(cbks);
}

void TRANS_UnregisterCbks(void)
{
    return TransMock()->TRANS_UnregisterCbks();
}

uint32_t TRANS_SendData(TRANS_Addr_S *addr, const uint8_t *data, uint16_t dataLen)
{
    return TransMock()->TRANS_SendData(addr, data, dataLen);
}

void TRANS_ChannelSetStatus(SLE_Addr_S *addr, uint8_t tcid, uint8_t result)
{
    return TransMock()->TRANS_ChannelSetStatus(addr, tcid, result);
}

}

}