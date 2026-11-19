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

#include "nlstk_ccp_ccs_server.h"
#include "mock_ccs_record.h"
#include "securec.h"
#include "log.h"

static struct MockCcsRecord g_ccsRecord = {0};

struct MockCcsRecord MockCcsGetRecord(void)
{
    return g_ccsRecord;
}

void MockCcsResetRecord(void)
{
    (void)memset_s(&g_ccsRecord, sizeof(g_ccsRecord), 0, sizeof(g_ccsRecord));
}

uint32_t NLSTK_CcpCreateCcsInstance(NLSTK_CcpCallControlInfo_S *baseInfo)
{
    HILOGI("[CcpStack Mocker] NLSTK_CcpCreateCcsInstance enter");
    g_ccsRecord.createInstanceCount++;
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_CcpDeleteCcsInstance(int32_t instanceId)
{
    HILOGI("[CcpStack Mocker] NLSTK_CcpDeleteCcsInstance enter");
    g_ccsRecord.deleteInstanceCount++;
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_CcpUpdateCcsProperty(int32_t instanceId, NLSTK_CcpCcsPropertyType_E property, void *value)
{
    HILOGI("[CcpStack Mocker] NLSTK_CcpUpdateCcsProperty enter, property = %{public}d", property);
    if (value == nullptr) {
        return NLSTK_ERRCODE_SUCCESS;
    }
    if (property == NLSTK_CCP_CCS_CALL_STATUS) {
        NLSTK_CcpCallStatues_S *callState = static_cast<NLSTK_CcpCallStatues_S *>(value);
        g_ccsRecord.callStateUpdateCount++;
        g_ccsRecord.lastCallCount = callState->callCount;
        if (callState->callCount > 0 && callState->callId != nullptr) {
            g_ccsRecord.lastCallFirstId = callState->callId[0];
        }
    } else if (property == NLSTK_CCP_CCS_CALLIN_OUT_INFO) {
        NLSTK_CcpCallInOutInfo_S *inOut = static_cast<NLSTK_CcpCallInOutInfo_S *>(value);
        g_ccsRecord.inOutUpdateCount++;
        g_ccsRecord.lastInOutCallFlag = inOut->callFlag;
        g_ccsRecord.lastInOutUserInfoLen = inOut->userInfo.len;
        g_ccsRecord.lastInOutUserAliasLen = inOut->userAlias.len;
        g_ccsRecord.lastInOutCallId = inOut->callId;
    } else if (property == NLSTK_CCP_CCS_CALL_TERMINATION) {
        g_ccsRecord.terminateUpdateCount++;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

void NLSTK_CcpCallControlResult(uint32_t requestId, int32_t instanceId, uint8_t errorCode)
{
    HILOGI("[CcpStack Mocker] NLSTK_CcpCallControlResult enter");
    g_ccsRecord.callControlResultCount++;
    g_ccsRecord.lastCallControlRequestId = requestId;
}

uint32_t NLSTK_CcpCcsAuthorizeResult(uint32_t requestId, int32_t instanceId, NLSTK_CcpCcsPropertyType_E property,
    uint8_t errorCode)
{
    HILOGI("[CcpStack Mocker] NLSTK_CcpCcsAuthorizeResult enter");
    g_ccsRecord.authorizeResultCount++;
    return NLSTK_ERRCODE_SUCCESS;
}
