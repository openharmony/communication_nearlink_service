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
#include <stdint.h>
#include <stdbool.h>
#include "nlstk_log.h"
#include "securec.h"
#include "cm_def.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "ssap_link_state.h"

#define NLSTK_SSAP_MAX_NUM_OF_LINK  128       // 最大链路数量，目前最大支持128个星闪对端

typedef struct {
    SLE_Addr_S addr;
    NLSTK_SsapConnectLinkState_E actualLinkState;    // 实际链路状态, 由CM通知上报
    bool usedFlag;
} SsapcAppLinkStateManager_S;

SsapcAppLinkStateManager_S g_linkState[NLSTK_SSAP_MAX_NUM_OF_LINK];
bool g_ssapClientCleanUp = false;
bool g_ssapServerCleanUp = false;

static SsapcAppLinkStateManager_S *SsapAllocLinkManagerCb(void)
{
    for (int32_t i = 0; i < NLSTK_SSAP_MAX_NUM_OF_LINK; i++) {
        if (g_linkState[i].usedFlag == true) {
            continue;
        }
        g_linkState[i].usedFlag = true;
        return &g_linkState[i];
    }
    NLSTK_LOG_ERROR("no link state manager cb can be allocated");
    return NULL;
}

static void SsapFreeLinkManagerCb(SsapcAppLinkStateManager_S *linkState)
{
    NLSTK_CHECK_RETURN_VOID(linkState != NULL, "param is null when free link state manager cb");
    linkState->usedFlag = false;
    (void)memset_s(linkState, sizeof(SsapcAppLinkStateManager_S), 0, sizeof(SsapcAppLinkStateManager_S));
    return;
}

static SsapcAppLinkStateManager_S *SsapFindLinkManagerCbByAddr(const SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NULL, "param is null when free link state manager cb");
    for (int32_t i = 0; i < NLSTK_SSAP_MAX_NUM_OF_LINK; i++) {
        if (g_linkState[i].usedFlag == false) {
            continue;
        }
        if (SDF_CompareSleAddr(&g_linkState[i].addr, addr) != 0) {
            continue;
        }
        return &g_linkState[i];
    }
    return NULL;
}

static void SsapGetCmDirectConnParam(CM_DirectConnAddrParam_S *param, const SLE_Addr_S *addr,
    const NLSTK_ConnParam_S *connParam)
{
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    param->frameType = connParam->frameType;
}

static NLSTK_SsapConnectLinkState_E SsapLinkHandleTryConnect(SsapcAppLinkStateManager_S *linkState,
    const NLSTK_ConnParam_S *connParam)
{
    NLSTK_SsapConnectLinkState_E state = SSAP_CONNECT_STATE_IDLE;
    NLSTK_LOG_DEBUG("call SsapLinkHandleTryConnect connect the link, state %d, addr: %s", linkState->actualLinkState,
                 GET_ENC_ADDR(&(linkState->addr)));
    CM_DirectConnAddrParam_S connAddrParam = {};
    switch (linkState->actualLinkState) {
        case SSAP_CONNECT_STATE_IDLE:
            SsapGetCmDirectConnParam(&connAddrParam, &(linkState->addr), connParam);
            if (CM_DirectConnectAdd(CM_MODULE_SSAP, &connAddrParam) != CM_SUCCESS) {
                state = SSAP_CONNECT_STATE_DISCONNECTED;  // 当调用CM接口失败的时候，返回建链失败
                break;
            }
            state = SSAP_CONNECT_STATE_CONNECTING;
            break;

        case SSAP_CONNECT_STATE_CONNECTING:
            // 正在建链，不能再次发起建链
            state = SSAP_CONNECT_STATE_CONNECTING;
            break;

        case SSAP_CONNECT_STATE_CONNECTED:
            // 正在建链，不能再次发起建链
            state = SSAP_CONNECT_STATE_CONNECTED;
            break;

        case SSAP_CONNECT_STATE_DISCONNECTING:
            state = SSAP_CONNECT_STATE_DISCONNECTING;
            break;

        case SSAP_CONNECT_STATE_DISCONNECTED:
            SsapGetCmDirectConnParam(&connAddrParam, &(linkState->addr), connParam);
            if (CM_DirectConnectAdd(CM_MODULE_SSAP, &connAddrParam) != CM_SUCCESS) {
                state = SSAP_CONNECT_STATE_DISCONNECTED;  // 当调用CM接口失败的时候，返回建链失败
                break;
            }
            state = SSAP_CONNECT_STATE_CONNECTING;
            break;
        default:
            break;
    }
    linkState->actualLinkState = state;
    if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
        SsapFreeLinkManagerCb(linkState);
    }
    return state;
}

static NLSTK_SsapConnectLinkState_E SsapLinkHandleTryDisconnect(SsapcAppLinkStateManager_S *linkState)
{
    NLSTK_SsapConnectLinkState_E state = SSAP_CONNECT_STATE_IDLE;
    NLSTK_LOG_INFO("call SsapLinkHandleTryDisconnect connect the link, state %d, addr: %s", linkState->actualLinkState,
                 GET_ENC_ADDR(&(linkState->addr)));
    switch (linkState->actualLinkState) {
        case SSAP_CONNECT_STATE_IDLE:
            state = SSAP_CONNECT_STATE_DISCONNECTED;  // 此场景说明链路已经不存在了，所以直接返回断链状态；
            break;

        case SSAP_CONNECT_STATE_CONNECTING:
            // 正在建链，不能发起断链
            state = SSAP_CONNECT_STATE_CONNECTING;
            break;

        case SSAP_CONNECT_STATE_CONNECTED:
            if (CM_DirectConnectRemove(CM_MODULE_SSAP, &(linkState->addr), CM_DISC_REASON_REMOTE_USER_TERMINATED) !=
                CM_SUCCESS) {
                state = SSAP_CONNECT_STATE_DISCONNECTED;
                break;
            }
            state = SSAP_CONNECT_STATE_DISCONNECTING;
            break;

        case SSAP_CONNECT_STATE_DISCONNECTING:
            state = SSAP_CONNECT_STATE_DISCONNECTING;
            break;

        case SSAP_CONNECT_STATE_DISCONNECTED:
            state = SSAP_CONNECT_STATE_DISCONNECTED;
            break;
        default:
            break;
    }
    linkState->actualLinkState = state;
    if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
        SsapFreeLinkManagerCb(linkState);
    }
    return state;
}

void SsapLinkHandleRecordLinkStateFromCm(SLE_Addr_S *addr, NLSTK_SsapConnectLinkState_E state)
{
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "addr is null when record link state");
    SsapcAppLinkStateManager_S *linkState = SsapFindLinkManagerCbByAddr(addr);
    if (linkState == NULL) {
        // 当底层上报的是断链的时候，没有必要记录；
        if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
            return;
        } else if (state == SSAP_CONNECT_STATE_CONNECTED) {
            linkState = SsapAllocLinkManagerCb();
            NLSTK_CHECK_RETURN_VOID(linkState != NULL, "no link state manager cb can be allocated");
            linkState->actualLinkState = SSAP_CONNECT_STATE_CONNECTED;
            memcpy_s(&(linkState->addr), sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
        }
        return;
    } else {
        if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
            SsapFreeLinkManagerCb(linkState);
            return;
        } else if (state == SSAP_CONNECT_STATE_CONNECTED) {
            linkState->actualLinkState = SSAP_CONNECT_STATE_CONNECTED;
            return;
        }
    }
}

NLSTK_SsapConnectLinkState_E SsapLinkHandleUserConnect(SLE_Addr_S *addr, const NLSTK_ConnParam_S *connParam)
{
    NLSTK_CHECK_RETURN(addr != NULL, SSAP_CONNECT_STATE_DISCONNECTED, "addr is null when try connect");
    NLSTK_LOG_DEBUG("try to connect the link addr %s", GET_ENC_ADDR(addr));
    SsapcAppLinkStateManager_S *linkState = SsapFindLinkManagerCbByAddr(addr);
    if (linkState == NULL) {
        linkState = SsapAllocLinkManagerCb();
        if (linkState == NULL) {
            NLSTK_LOG_ERROR("try to connect the link addr %s fail, there is no resourse", GET_ENC_ADDR(addr));
            return SSAP_CONNECT_STATE_DISCONNECTED;  // 当没有可用的链路状态管理回调的时候，返回建链失败
        }
        (void)memcpy_s(&(linkState->addr), sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    }
    return SsapLinkHandleTryConnect(linkState, connParam);
}

NLSTK_SsapConnectLinkState_E SsapLinkHandleUserDisconnect(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, SSAP_CONNECT_STATE_DISCONNECTED, "addr is null when try disconnect");
    SsapcAppLinkStateManager_S *linkState = SsapFindLinkManagerCbByAddr(addr);
    if (linkState == NULL) {
        return SSAP_CONNECT_STATE_DISCONNECTED;
    }
    return SsapLinkHandleTryDisconnect(linkState);
}

bool SsapGetClientCleanUp(void)
{
    return g_ssapClientCleanUp;
}

bool SsapGetServerCleanUp(void)
{
    return g_ssapServerCleanUp;
}

void SsapResetClientCleanUp(void)
{
    g_ssapClientCleanUp = false;
}

void SsapResetServerCleanUp(void)
{
    g_ssapServerCleanUp = false;
}


// 关闭星闪时，移除链路采用直接移除的方式，然后通过CM的回调通知上报链路状态清理资源，不需要额外判断链路状态
void SsapRemoveLink(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "addr is null when try disconnect");
    g_ssapClientCleanUp = true;
    NLSTK_LOG_INFO("ssap clientremove link, addr %s", GET_ENC_ADDR(addr));
    if (CM_DirectConnectRemove(CM_MODULE_SSAP, addr, CM_DISC_REASON_REMOTE_USER_TERMINATED) != CM_SUCCESS) {
        NLSTK_LOG_ERROR("CM _DirectConnectRemove fail, addr: %s", GET_ENC_ADDR(addr));
    }
}

bool SsapIsAllLinkCleanUp(void)
{
    int32_t i = 0;
    for (i = 0; i < NLSTK_SSAP_MAX_NUM_OF_LINK; i++) {
        if (g_linkState[i].usedFlag == false) {
            continue;
        }
        NLSTK_LOG_INFO("the addrs(%s) not in disconnected", GET_ENC_ADDR(&(g_linkState[i].addr)));
        break;
    }
    return i == NLSTK_SSAP_MAX_NUM_OF_LINK ? true : false;
}

void SsapRemoveAllLink(void)
{
    g_ssapServerCleanUp = true;
    for (int32_t i = 0; i < NLSTK_SSAP_MAX_NUM_OF_LINK; i++) {
        if (g_linkState[i].usedFlag == false) {
            continue;
        }
        NLSTK_LOG_INFO("ssap server remove link, addr %s", GET_ENC_ADDR(&(g_linkState[i].addr)));
        if (CM_DirectConnectRemove(CM_MODULE_SSAP, &(g_linkState[i].addr), CM_DISC_REASON_REMOTE_USER_TERMINATED)
            != CM_SUCCESS) {
            NLSTK_LOG_ERROR("CM _DirectConnectRemove fail, addr: %s", GET_ENC_ADDR(&(g_linkState[i].addr)));
        }
    }
}

void SsapLinkStateDeinit(void)
{
    (void)memset_s(g_linkState, sizeof(g_linkState), 0, sizeof(g_linkState));
    g_ssapClientCleanUp = false;
    g_ssapServerCleanUp = false;
}