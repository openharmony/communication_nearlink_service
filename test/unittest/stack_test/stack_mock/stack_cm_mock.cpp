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

#include "stack_cm_mock.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
void *g_stackCmMock;

CmMock::CmMock()
{
    g_stackCmMock = reinterpret_cast<void *>(this);
}

CmMock::~CmMock()
{
    g_stackCmMock = nullptr;
}

static CmMockInterface *CmMock()
{
    return reinterpret_cast<CmMockInterface *>(g_stackCmMock);
}

extern "C" {

uint32_t CM_RegLogicLinkListener(CM_LogicLinkCbks_S *cbks)
{
    return CmMock()->CM_RegLogicLinkListener(cbks);
}

uint32_t CM_UnRegLogicLinkListener(uint8_t moduleId)
{
    return CmMock()->CM_UnRegLogicLinkListener(moduleId);
}

uint32_t CM_RegTransChannelListener(CM_TransChannelCbk cbk)
{
    return CmMock()->CM_RegTransChannelListener(cbk);
}

void CM_UnRegTransChannelListener(void)
{
    return CmMock()->CM_UnRegTransChannelListener();
}

uint32_t CM_GetLogicLinkConnectedSize(void)
{
    return CmMock()->CM_GetLogicLinkConnectedSize();
}

uint32_t CM_GetLogicLinkByAddr(SLE_Addr_S *addr, CM_LogicLink_S *logicLink)
{
    return CmMock()->CM_GetLogicLinkByAddr(addr, logicLink);
}

uint32_t CM_ConnectUpdateParamReq(CM_ConnectUpdateParamReq_S *param)
{
    return 0;
}

uint32_t CM_GetLogicLinkByLcid(uint16_t lcid, CM_LogicLink_S *logicLink)
{
    return CmMock()->CM_GetLogicLinkByLcid(lcid, logicLink);
}

void CM_ClearAcceptFilterList(void)
{
    return;
}

uint32_t CM_AddDeviceToAcceptFilterList(SLE_Addr_S *addr, bool isBypass)
{
    return 0;
}

uint32_t CM_ConnectSetParamReq(CM_ConnectSetParamReq_S *param)
{
    return 0;
}

uint32_t CM_ConnectEstablishReq(CM_ConnectParamReq_S *param)
{
    return 0;
}

uint32_t CM_ConnectCancelReq(void)
{
    return 0;
}

uint32_t CM_ConnectReleaseReq(CM_DisconnectParamReq_S *param)
{
    return 0;
}

uint32_t CM_SetLogicLinkDeviceType(uint16_t lcid, uint8_t deviceType)
{
    return 0;
}

}
} // namespace OHOS