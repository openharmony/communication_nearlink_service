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
#include "nlstk_log.h"
#include "ssapc_app.h"
#include "ssaps_server_app.h"
#include "ssapc_app_link_sm.h"
#include "nlstk_ssap_app_link.h"
#include "ssap_link_state.h"
#include "ssap_app_link.h"

void SsapClientConnect(void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[SSAPC_APP] param is null in SsapClientConnect");
    int32_t appId = *(int32_t *)param;
    NLSTK_LOG_INFO("appId %d call SsapClientConnect", appId);
    NLSTK_CHECK_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] appId is invalid in SsapClientConnect");
    if (SsapGetClientCleanUp() || SsapGetServerCleanUp()) {
        NLSTK_LOG_ERROR("ssap client is disable, can't connect");
        return;
    }
    SsapClientLinkStateMachineCall(appId, SSAP_USER_CONNECT, 0);
    return;
}

// 当链路状态是下述三种值的时候，Ssap客户端不需要通过回调通知service状态变更：
// SSAP_CONNECT_STATE_IDLE  : 这个表示service还没有调用connect，因此不需要通知
// SSAP_CONNECT_STATE_DISCONNECTED ： 这个表示service已经调用了disconnect，因此不需要通知
// 在其余状态下，若链路状态发生变化，均需要通知service调整状态
void SsapClientDisconnect(void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[SSAPC_APP] param is null in SsapClientDisconnect");
    int32_t appId = *(int32_t *)param;
    NLSTK_CHECK_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] appId is invalid in SsapClientDisconnect");
    NLSTK_LOG_INFO("appId %d call SsapClientDisconnect", appId);
    if (SsapGetClientCleanUp() || SsapGetServerCleanUp()) {
        NLSTK_LOG_ERROR("ssap client is disable, can't disconnect");
        return;
    }
    SsapClientLinkStateMachineCall(appId, SSAP_USER_DISCONNECT, 0);
    return;
}

void SsapTriggerLinkStateMachineChange(SLE_Addr_S *addr, NLSTK_SsapClientLinkChangeEvent_E event, uint8_t reason)
{
    // 服务端，则根据事件直接触发状态变更通知
    if (event == SSAP_LOGIC_LINK_CONNECTED) {
        SsapServerLinkStateNofity(addr, SSAP_CONNECT_STATE_CONNECTED, NLSTK_ERRCODE_SUCCESS, reason);
    } else {
        SsapServerLinkStateNofity(addr, SSAP_CONNECT_STATE_DISCONNECTED, NLSTK_ERRCODE_SUCCESS, reason);
    }

    // 获取所有关联此地址的客户端appId，并触发它们的链路状态机
    int32_t appIdList[NLSTK_SSAP_CLIENT_APP_MAX_NUM] = {0};
    uint32_t clientAppIdNum = SsapcAppGetAppIdListByAddr(addr, appIdList, NLSTK_SSAP_CLIENT_APP_MAX_NUM);
    if (clientAppIdNum == 0) {
        // 不能进入到此分支，因为进入到此分支表示没有任何appId关键此地址
        NLSTK_LOG_ERROR("no appId is related to this addr in SsapTriggerLinkStateMachineChange");
        return;
    }
    for (uint32_t index = 0; index < clientAppIdNum; index++) {
        SsapClientLinkStateMachineCall(appIdList[index], event, reason);
    }
}