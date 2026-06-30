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
#include <stdint.h>
#include "nlstk_log.h"
#include "ssap_link_state.h"
#include "ssapc_app_link_sm.h"
// ssap客户端链路状态机函数，用于处理各种状态下的链路变化事件
// reason参数专门用于接管CM接口上报的原因值
typedef void (*SsapAppClientLinkStateMachine)(int32_t appId, uint8_t reason);

static void SsapAppClientUserConnectInIdleState(int32_t appId, uint8_t reason);
static void SsapAppClientUserDisconnectInIdleState(int32_t appId, uint8_t reason);
static void SsapAppClientLinkConnectedInIdleState(int32_t appId, uint8_t reason);
static void SsapAppClientLinkDisconnectedInIdleState(int32_t appId, uint8_t reason);

static void SsapAppClientUserConnectInConnectingState(int32_t appId, uint8_t reason);
static void SsapAppClientUserDisconnectInConnectingState(int32_t appId, uint8_t reason);
static void SsapAppClientLinkConnectedInConnectingState(int32_t appId, uint8_t reason);
static void SsapAppClientLinkDisconnectedInConnectingState(int32_t appId, uint8_t reason);

static void SsapAppClientUserConnectInConnectedState(int32_t appId, uint8_t reason);
static void SsapAppClientUserDisconnectInConnectedState(int32_t appId, uint8_t reason);
static void SsapAppClientLinkConnectedInConnectedState(int32_t appId, uint8_t reason);
static void SsapAppClientLinkDisconnectedInConnectedState(int32_t appId, uint8_t reason);

static void SsapAppClientUserConnectInDisconnectingState(int32_t appId, uint8_t reason);
static void SsapAppClientUserDisconnectInDisconnectingState(int32_t appId, uint8_t reason);
static void SsapAppClientLinkConnectedInDisconnectingState(int32_t appId, uint8_t reason);
static void SsapAppClientLinkDisconnectedInDisconnectingState(int32_t appId, uint8_t reason);

static void SsapAppClientUserConnectInDisconnectedState(int32_t appId, uint8_t reason);
static void SsapAppClientUserDisconnectInDisconnectedState(int32_t appId, uint8_t reason);
static void SsapAppClientLinkConnectedInDisconnectedState(int32_t appId, uint8_t reason);
static void SsapAppClientLinkDisconnectedInDisconnectedState(int32_t appId, uint8_t reason);

void SsapClientLinkStateMachineCall(int32_t appId, NLSTK_SsapClientLinkChangeEvent_E event, uint8_t reason)
{
    if (event >= SSAP_LINK_CHANGE_EVENT_BUTT) {
        return;
    }
    NLSTK_SsapConnectLinkState_E state = SsapcAppGetLinkState(appId);
    if (state >= SSAP_CONNECT_STATE_BUTT) {
        return;
    }
    NLSTK_LOG_DEBUG("app id(%d) start the link state machine, state is %d, event is %d", appId, state, event);
    static SsapAppClientLinkStateMachine linkStateMachine[SSAP_CONNECT_STATE_BUTT][SSAP_LINK_CHANGE_EVENT_BUTT] = {
        [SSAP_CONNECT_STATE_IDLE] = {
            [SSAP_USER_CONNECT] = SsapAppClientUserConnectInIdleState,
            [SSAP_USER_DISCONNECT] = SsapAppClientUserDisconnectInIdleState,
            [SSAP_LOGIC_LINK_CONNECTED] = SsapAppClientLinkConnectedInIdleState,
            [SSAP_LOGIC_LINK_DISCONNECTED] = SsapAppClientLinkDisconnectedInIdleState,
        },
        [SSAP_CONNECT_STATE_CONNECTING] = {
            [SSAP_USER_CONNECT] = SsapAppClientUserConnectInConnectingState,
            [SSAP_USER_DISCONNECT] = SsapAppClientUserDisconnectInConnectingState,
            [SSAP_LOGIC_LINK_CONNECTED] = SsapAppClientLinkConnectedInConnectingState,
            [SSAP_LOGIC_LINK_DISCONNECTED] = SsapAppClientLinkDisconnectedInConnectingState,
        },
        [SSAP_CONNECT_STATE_CONNECTED] = {
            [SSAP_USER_CONNECT] = SsapAppClientUserConnectInConnectedState,
            [SSAP_USER_DISCONNECT] = SsapAppClientUserDisconnectInConnectedState,
            [SSAP_LOGIC_LINK_CONNECTED] = SsapAppClientLinkConnectedInConnectedState,
            [SSAP_LOGIC_LINK_DISCONNECTED] = SsapAppClientLinkDisconnectedInConnectedState,
        },
        [SSAP_CONNECT_STATE_DISCONNECTING] = {
            [SSAP_USER_CONNECT] = SsapAppClientUserConnectInDisconnectingState,
            [SSAP_USER_DISCONNECT] = SsapAppClientUserDisconnectInDisconnectingState,
            [SSAP_LOGIC_LINK_CONNECTED] = SsapAppClientLinkConnectedInDisconnectingState,
            [SSAP_LOGIC_LINK_DISCONNECTED] = SsapAppClientLinkDisconnectedInDisconnectingState,
        },
        [SSAP_CONNECT_STATE_DISCONNECTED] = {
            [SSAP_USER_CONNECT] = SsapAppClientUserConnectInDisconnectedState,
            [SSAP_USER_DISCONNECT] = SsapAppClientUserDisconnectInDisconnectedState,
            [SSAP_LOGIC_LINK_CONNECTED] = SsapAppClientLinkConnectedInDisconnectedState,
            [SSAP_LOGIC_LINK_DISCONNECTED] = SsapAppClientLinkDisconnectedInDisconnectedState,
        },
    };
    linkStateMachine[state][event](appId, reason);
    return;
}


static bool SsapcAppClientDisconnectCmLinkCheck(int32_t appId)
{
    NLSTK_CHECK_RETURN(IsAppIdValid(appId), true, "[SSAPC_APP] appId is invalid in SsapClientConnect");

    SLE_Addr_S *addr = GetAddrByAppId(appId);
    NLSTK_CHECK_RETURN(addr != NULL, true, "[SSAPC_APP] appId is invalid in SsapClientConnect");
    int32_t appIdList[NLSTK_SSAP_CLIENT_APP_MAX_NUM] = {0};
    uint32_t clientAppIdNum = SsapcAppGetAppIdListByAddr(addr, appIdList, NLSTK_SSAP_CLIENT_APP_MAX_NUM);
    if (clientAppIdNum == 0) {
        // 不能进入到此分支，因为进入到此分支表示没有任何appId关键此地址
        NLSTK_LOG_ERROR("the addr in appid(%d) link is wrong, disconnect it", appId);
        return true;
    }

    for (uint32_t index = 0; index < clientAppIdNum; index++) {
        int32_t checkAppId = appIdList[index];
        // 检查时，如遇到当前正在操作的appId，则跳过
        if (appId == checkAppId) {
            continue;
        }

        // SSAP_CONNECT_STATE_DISCONNECTED 和 SSAP_CONNECT_STATE_IDLE 这两种状态都表示此时AppId未使用此链路
        // SSAP_CONNECT_STATE_CONNECTED/SSAP_CONNECT_STATE_CONNECTING 状态表示此地址还有其他的appId正在使用，不能进行断链操作
        // SSAP_CONNECT_STATE_DISCONNECTING 此状态，说明该appid正在进行断链操作，此时appId也可以尝试调用CM的接口进行断链操作
        if (SsapcAppGetLinkState(checkAppId) == SSAP_CONNECT_STATE_CONNECTED ||
            SsapcAppGetLinkState(checkAppId) == SSAP_CONNECT_STATE_CONNECTING) {
            return false;
        }
    }
    // 进入到此处，说明当前仅有本appId在使用此链路，可以断链
    return true;
}

/**
 * @brief 处理用户在空闲状态下发起的链路连接请求
 * @details 根据appId获取链路地址，并调用SsapLinkHandleUserConnect进行连接操作。
 *          根据连接状态执行相应的处理：
 *          - 已连接（SSAP_CONNECT_STATE_CONNECTED）：设置链路状态为已连接，并通知链路状态。
 *          - 正在连接（SSAP_CONNECT_STATE_CONNECTING）：设置链路状态为正在连接。
 *          - 正在断开（SSAP_CONNECT_STATE_DISCONNECTING）：缓存此连接请求，等待断链完成后再处理。
 *          - 断开（SSAP_CONNECT_STATE_DISCONNECTED）：设置链路状态为断开，并通知链路状态和错误信息。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason CM回调中上报的连接原因值
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientUserConnectInIdleState(int32_t appId, uint8_t reason)
{
    NLSTK_CHECK_RETURN_VOID(IsAppIdValid(appId), "appId(%d) is invalid", appId);

    // 首先检查缓存的操作
    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op != SSAP_LINK_CHANGE_EVENT_BUTT) {
        // 在IDLE状态不应该有缓存的逻辑，因此这里要清理掉缓存的操作
        NLSTK_LOG_ERROR("the appid(%d) link state is idle, but there is a cache operation", appId);
        SsapcClearLinkOper(appId);
    }

    SLE_Addr_S *addr = GetAddrByAppId(appId);
    NLSTK_ConnParam_S *connParam = GetConnParamByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(addr != NULL && connParam != NULL, "addr in appId(%d) is invalid", appId);
    NLSTK_SsapConnectLinkState_E state = SsapLinkHandleUserConnect(addr, connParam);
    if (state == SSAP_CONNECT_STATE_CONNECTED) {
        // 底层链路已经建立，直接修改状态，并返回OK
        SsapcAppSetLinkState(appId, SSAP_CONNECT_STATE_CONNECTED);
        SsapcAppLinkStateNofity(appId, SSAP_CONNECT_STATE_CONNECTED, NLSTK_ERRCODE_SUCCESS, reason);
    } else if (state == SSAP_CONNECT_STATE_CONNECTING) {
        // 底层链路已经建立，直接修改状态，并返回OK
        SsapcAppSetLinkState(appId, SSAP_CONNECT_STATE_CONNECTING);
    } else if (state == SSAP_CONNECT_STATE_DISCONNECTING) {
        // 说明此时正在断链，要缓存此断链的操作，等待断链操作完成之后，再进行建链，此是不修改状态，缓存操作
        // 环境上已经存在appid1和addrA，appid1的业务已经完成正在发起断链；
        // 此时appid2发起了addrA的建链请求，但是由于当前正在断链，因此缓存此建链请求
        // CM完成了addrA的断链处理，上报了addrA的链路状态；此时针对appid2不上报断链故障，并触发appid2的缓存建链启动
        // appid2调用CM接口发起建链，等CM完成建链之后，上报建链成功或者建链失败的状态；
        SsapcCacheLinkOper(appId, SSAP_USER_CONNECT);
    } else if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
        // 表示调用建链接口失败了，此是需要上报给用户的错误
        // 底层链路已经建立，直接修改状态，并返回OK
        SsapcAppSetLinkState(appId, SSAP_CONNECT_STATE_DISCONNECTED);
        SsapcAppLinkStateNofity(appId, SSAP_CONNECT_STATE_DISCONNECTED, NLSTK_ERRCODE_CONN_FAIL, reason);
        return;
    }
    return;
}

/**
 * @brief 处理用户在空闲状态下发起的链路断开请求
 * @details 当应用程序之前没有调用过连接函数时，直接上报断链状态并返回，而不进行实际的断链操作。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 断开的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientUserDisconnectInIdleState(int32_t appId, uint8_t reason)
{
    // 此时表示此app之前并没有调用过 connect函数进行连接，这里直接上报一个断链的状态，并返回给用户，但是不修改状态；
    NLSTK_CHECK_RETURN_VOID(IsAppIdValid(appId), "appId(%d) is invalid", appId);
    SsapcAppLinkStateNofity(appId, SSAP_CONNECT_STATE_DISCONNECTED, NLSTK_ERRCODE_SUCCESS, reason);
}

/**
 * @brief 处理链路在空闲状态下连接完成的事件
 * @details 当链路连接完成时，根据是否有缓存的操作来决定是否重置链路状态或重新触发状态机。
 *          - 如果没有缓存操作，则说明链路无操作，不需要进行额外的操作。
 *          - 如果有缓存操作，清除缓存的操作，并上报链路故障。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 断开的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientLinkConnectedInIdleState(int32_t appId, uint8_t reason)
{
    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op != SSAP_LINK_CHANGE_EVENT_BUTT) {
        // 原则上代码不会进入此分支：缓存操作有如下几种场景：
        // 场景一： service调用appid进行建链，此时存在其他的appId正在断链，因此缓存建链操作；
        // 场景二： service调用appid进行建链，此时上一次本appId的断链操作还未完成，因此需要缓存；
        // 场景三： service调用appid进行断链，此时不存在其他的appId正使用此链路，且本appid上一次的建链操作还未完成，因此需要缓存；
        // 要求进入到此分支，则需要满足：
        // 1. 当前链路正在建链，因为此时收到的建链成功的事件； --->仅场景三满足此条件
        // 2. appId处于空闲状态  --->仅场景一满足此条件
        // 上述场景中，无任何场景能同时满足上述两个条件，因此代码不会进入此分支，
        // 但是为了韧性考虑还是添加了此分支，清理缓存，并上报链路故障；
        NLSTK_LOG_ERROR("appid(%s) link state exception", appId);
        SsapcClearLinkOper(appId);
        // 重新出发状态机，尝试下发缓存中的操作
        SsapcAppLinkStateNofity(appId, SSAP_CONNECT_STATE_DISCONNECTED, NLSTK_ERRCODE_SUCCESS, reason);
    }
    return;
}

/**
 * @brief 处理链路在空闲状态下断开的事件
 * @details 当链路断开时，根据是否有缓存的操作来决定是否重置链路状态或重新触发状态机。
 *          - 如果没有缓存操作，将链路状态设置为空闲状态。
 *          - 如果有缓存操作，清除缓存的操作，并重新触发状态机来处理缓存的操作。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 断开的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientLinkDisconnectedInIdleState(int32_t appId, uint8_t reason)
{
    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op != SSAP_LINK_CHANGE_EVENT_BUTT) { // SSAP_LINK_CHANGE_EVENT_BUTT表示没有缓存操作，说明当前service未使用这个链路
        // 缓存操作有如下几种场景：
        // 场景一： service调用appid进行建链，此时存在其他的appId正在断链，因此缓存建链操作；
        // 场景二： service调用appid进行建链，此时上一次本appId的断链操作还未完成，因此需要缓存；
        // 场景三： service调用appid进行断链，此时不存在其他的appId正使用此链路，且本appid上一次的建链操作还未完成，因此需要缓存；
        // 进入此分支的场景： 场景一，
        SsapcClearLinkOper(appId);
        // 重新出发状态机，尝试下发缓存中的操作
        SsapClientLinkStateMachineCall(appId, op, reason);
    }
    return;
}

/**
 * @brief 处理用户在链路正在连接状态下发起的连接请求
 * @details 当链路正在连接时，根据缓存的操作来决定是否清除缓存。
 *          - 如果没有缓存操作，直接返回。
 *          - 如果缓存的操作是断开或连接，清除缓存的操作。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 断开的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientUserConnectInConnectingState(int32_t appId, uint8_t reason)
{
    (void)reason;
    // 此时说明当前链路正在建链，需要查看是否有缓存的操作，可能存在断链的操作在缓存中，如下面的场景：
    // 场景二： service调用appid进行建链，此时上一次本appId的断链操作还未完成，因此需要缓存；
    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op == SSAP_LINK_CHANGE_EVENT_BUTT) {
        return;
    } else if (op == SSAP_USER_DISCONNECT || op == SSAP_USER_CONNECT) {
        // 当缓存操作中有SSAP_USER_DISCONNECT的时候，对应的appid操作场景如下：
        // step1: 建链 step2: 在建链未完成之前，下发断链 step3：在第一步的建链未完成之前，重新下发建链
        // SSAP会将step2和step3抵消，只保留step1的建链操作
        // 原则上，在connecting的状态下，缓存操作中不会有SSAP_USER_CONNECT，但如果存在的话，也做下清理操作；
        SsapcClearLinkOper(appId);  // 消除这个断链的操作；
        return;
    }
    return;
}

/**
 * @brief 处理用户在链路正在连接状态下发起的断开请求
 * @details 当链路正在连接时，根据缓存的操作来决定是否缓存断开操作。
 *          - 如果没有缓存操作，缓存断开操作。
 *          - 如果缓存的操作是连接，清除缓存的连接操作，缓存断开操作。
 *          - 如果已经缓存了断开操作，直接返回。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 断开的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientUserDisconnectInConnectingState(int32_t appId, uint8_t reason)
{
    (void)reason;
    // 场景三： service调用appid进行断链，但是此时appid正在进行建链操作，此时缓存断链操作，等建链完成之后，再处理断链操作；
    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op == SSAP_LINK_CHANGE_EVENT_BUTT) {
        SsapcCacheLinkOper(appId, SSAP_USER_DISCONNECT); // 缓存断链操作
    } else if (op == SSAP_USER_CONNECT) {
        // 原则上不会存在此场景，因为在connecting状态下，不会有SSAP_USER_CONNECT的操作，但为了韧性考虑，添加了此分支
        SsapcClearLinkOper(appId);
        SsapcCacheLinkOper(appId, SSAP_USER_DISCONNECT); // 缓存断链操作
    } else if (op == SSAP_USER_DISCONNECT) {
        return;    // 当前已经缓存了断链操作，不需要再次缓存
    }
    return;
}

/**
 * @brief 处理链路在连接状态下收到连接完成的事件
 * @details 当链路连接完成时，设置链路状态为已连接，并通知用户。然后，根据是否有缓存的操作来决定是否清除缓存的操作并重新触发状态机。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 断开的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientLinkConnectedInConnectingState(int32_t appId, uint8_t reason)
{
    // 正常场景，在用户调用connect之后，底层链路建立成功，这个时候需要将状态配置为Connected，上报状态变化给service
    SsapcAppSetLinkState(appId, SSAP_CONNECT_STATE_CONNECTED);
    SsapcAppLinkStateNofity(appId, SSAP_CONNECT_STATE_CONNECTED, NLSTK_ERRCODE_SUCCESS, reason);

    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op != SSAP_LINK_CHANGE_EVENT_BUTT) { // 表示没有缓存操作，说明当前service未使用这个链路
        SsapcClearLinkOper(appId);
        // 重新出发状态机，尝试下发缓存中的操作
        SsapClientLinkStateMachineCall(appId, op, reason);
    }
    return;
}

/**
 * @brief 处理链路在连接状态下断开的事件
 * @details service调用connect下发建链请求，但是CM模块建链失败，此时会上报一个断链的事件。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 断开的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientLinkDisconnectedInConnectingState(int32_t appId, uint8_t reason)
{
    // 建链失败，首先修改当前的状态，然后上报建链失败的状态给service
    SsapcAppSetLinkState(appId, SSAP_CONNECT_STATE_DISCONNECTED);
    SsapcAppLinkStateNofity(appId, SSAP_CONNECT_STATE_DISCONNECTED, NLSTK_ERRCODE_SUCCESS, reason);

    // 查询当前appId上是否有缓存相关的操作，如果有缓存则重新出发状态机，执行缓存的链路操作；
    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op != SSAP_LINK_CHANGE_EVENT_BUTT) { // 表示没有缓存操作，说明当前service未使用这个链路
        SsapcClearLinkOper(appId);
        // 重新出发状态机，尝试下发缓存中的操作
        SsapClientLinkStateMachineCall(appId, op, reason);
    }
    return;
}

/**
 * @brief 处理用户在链路已连接状态下发起的连接请求
 * @details 当链路状态为已连接时，直接通知用户链路已连接的状态，也不需要进行缓存操作；
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 断开的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientUserConnectInConnectedState(int32_t appId, uint8_t reason)
{
    // 当链路状态是connected状态时，直接上报一个已经连接的状态，并返回给用户，但是不修改状态；
    SsapcAppLinkStateNofity(appId, SSAP_CONNECT_STATE_CONNECTED, NLSTK_ERRCODE_SUCCESS, reason);
    return;
}

/**
 * @brief 处理用户在链路已连接状态下发起的断链请求
 * @details 当链路状态为已连接时，收到用户的断链请求，需要先检查是否可以对CM发起断链，然后再根据断链操作的返回值做
 *          额外的处理；
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 断开的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientUserDisconnectInConnectedState(int32_t appId, uint8_t reason)
{
    NLSTK_CHECK_RETURN_VOID(IsAppIdValid(appId), "appId(%d) is invalid", appId);
    // 首先检查缓存的操作
    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op != SSAP_LINK_CHANGE_EVENT_BUTT) {
        // 在Connected状态不应该有缓存的逻辑，因此这里要清理掉缓存的操作
        NLSTK_LOG_ERROR("the appid(%d) link state is connected, but there is a cache operation", appId);
        SsapcClearLinkOper(appId);
    }

    SLE_Addr_S *addr = GetAddrByAppId(appId);
    // 在Connected状态下，用户调用了disconnect，这个时候需要判断对端地址是否有其它的appid还在使用，如果没有的话，
    // 直接将状态配置为Disconnected，然后返回触发断链操作，否则将状态配置为Disconnecting，然后返回
    bool disconnectCM = SsapcAppClientDisconnectCmLinkCheck(appId);
    if (disconnectCM == true) {
        NLSTK_SsapConnectLinkState_E state = SsapLinkHandleUserDisconnect(addr);
        if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
            SsapcAppSetLinkState(appId, SSAP_CONNECT_STATE_DISCONNECTED);
            SsapcAppLinkStateNofity(appId, SSAP_CONNECT_STATE_DISCONNECTED, NLSTK_ERRCODE_SUCCESS, reason);
        } else if (state == SSAP_CONNECT_STATE_DISCONNECTING) {
            SsapcAppSetLinkState(appId, SSAP_CONNECT_STATE_DISCONNECTING);
        } else if (state == SSAP_CONNECT_STATE_CONNECTING) {
            // 原则上不存在此场景，因为在connected的状态下，底层CM不会处于Connecting状态
            // 因此直接上报故障，并将链路调整为故障状态
            SsapcClearLinkOper(appId);
            SsapcAppSetLinkState(appId, SSAP_CONNECT_STATE_DISCONNECTED);
            SsapcAppLinkStateNofity(appId, SSAP_CONNECT_STATE_DISCONNECTED, NLSTK_ERRCODE_CONN_FAIL, reason);
        }
    } else {
        // 不允许断链的情况下，直接上报一个断链状态，并返回给用户
        SsapcAppSetLinkState(appId, SSAP_CONNECT_STATE_DISCONNECTED);
        SsapcAppLinkStateNofity(appId, SSAP_CONNECT_STATE_DISCONNECTED, NLSTK_ERRCODE_SUCCESS, reason);
    }
}

/**
 * @brief 处理链路在已连接状态下再次连接的事件
 * @details 该场景理论上不存在，函数不做处理，直接返回。如果有缓存的操作，则清除缓存并重新触发状态机。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 连接的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientLinkConnectedInConnectedState(int32_t appId, uint8_t reason)
{
    // 进入此函数表示，链路已经建立的情况下，CM再次上报了一个建链成功的事件,原则上不存在此场景，链路已经建立的情况不应该收到
    // CM上报的建链成功的事件，如果存在，说明CM存在问题，这里直接返回；
    // 韧性处理，查询下当前appId上是否有缓存相关的操作，如果有缓存则重新出发状态机，执行缓存的链路操作；
    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op != SSAP_LINK_CHANGE_EVENT_BUTT) {
        SsapcClearLinkOper(appId);
        // 重新出发状态机，尝试下发缓存中的操作
        SsapClientLinkStateMachineCall(appId, op, reason);
    }
    return;
}

/**
 * @brief 处理链路在已连接状态下断开的事件
 * @details 当链路在已连接状态下断开时，设置链路状态为断开，并通知用户链路断开的状态。如果有缓存的操作，则清除缓存并重新触发状态机。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 断开的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientLinkDisconnectedInConnectedState(int32_t appId, uint8_t reason)
{
    // 链路发生了异常，直接上断链；
    SsapcAppSetLinkState(appId, SSAP_CONNECT_STATE_DISCONNECTED);
    SsapcAppLinkStateNofity(appId, SSAP_CONNECT_STATE_DISCONNECTED, NLSTK_ERRCODE_SUCCESS, reason);

    // 重新触发状态机，尝试下发缓存中的操作，韧性的保护操作；
    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op != SSAP_LINK_CHANGE_EVENT_BUTT) {
        SsapcClearLinkOper(appId);
        SsapClientLinkStateMachineCall(appId, op, reason);
    }
    return;
}

/**
 * @brief 处理用户在链路断开状态下发起的连接请求
 * @details 在链路断开状态下，用户发起连接请求时，根据缓存的操作决定是否缓存当前操作或清理之前的缓存。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 连接的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientUserConnectInDisconnectingState(int32_t appId, uint8_t reason)
{
    (void)reason;
    // 此状态下，缓存链路的操作，等待链路断开完成之后，再处理这个操作；
    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op == SSAP_LINK_CHANGE_EVENT_BUTT) {
        SsapcCacheLinkOper(appId, SSAP_USER_CONNECT);   // 缓存建链操作
        return;
    } else if (op == SSAP_USER_CONNECT) { // 之前已有缓存，不需要额外处理
        return;
    } else if (op == SSAP_USER_DISCONNECT) {
        // 原则上在disconnecting状态，不应该有disconnect的操作缓存，增加额外保护，直接清理掉当前缓存操作，然后再缓存建链
        SsapcClearLinkOper(appId);  // 清理掉之前的链路操作缓存，重新缓存当前操作
        SsapcCacheLinkOper(appId, SSAP_USER_CONNECT);   // 缓存建链操作
        return;
    }
    return;
}

/**
 * @brief 处理用户在链路断开状态下发起的断开请求
 * @details 在链路断开状态下，用户发起断开请求时，根据缓存的操作决定是否清理缓存。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 断开的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientUserDisconnectInDisconnectingState(int32_t appId, uint8_t reason)
{
    (void)reason;
    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op == SSAP_LINK_CHANGE_EVENT_BUTT) {
        // 当前正在断链时，再收到断链，这里不需要做额外的处理；
        return;
    } else if (op == SSAP_USER_CONNECT) {
        // 如果缓存中有connect操作，说明用户先断链，在链路断链还未完成下发了建链，然后再次下发断链
        // 此场景下直接低调掉缓存的connect操作；
        SsapcClearLinkOper(appId);
        return;
    } else if (op == SSAP_USER_DISCONNECT) {
        // 如果缓存中有disconnect操作，说明用户先断链，在链路断链还未完成下发了断链，然后再次下发断链
        // 原则上不存在此场景，直接清理掉此缓存操作；
        SsapcClearLinkOper(appId);
        return;
    }
    return;
}

/**
 * @brief 处理链路在断开状态下连接成功的事件
 * @details 该场景理论上不存在，属于异常逻辑。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 连接的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientLinkConnectedInDisconnectingState(int32_t appId, uint8_t reason)
{
    // 正在建链的场景下，突然收到了一个CM上报的建链成功的事件，这个情况理论上不应该存在
    // 在此时，增加保护，上报链路异常，同时清理缓存的操作；
    SsapcAppSetLinkState(appId, SSAP_CONNECT_STATE_DISCONNECTED);
    SsapcAppLinkStateNofity(appId, SSAP_CONNECT_STATE_DISCONNECTED, NLSTK_ERRCODE_CONN_FAIL, reason);
    SsapcClearLinkOper(appId);
    return;
}

/**
 * @brief 处理链路在断开状态下断开的事件
 * @details 当链路在断开状态下断开时，设置链路状态为断开，并通知用户链路断开的状态。如果有缓存的操作，则清除缓存并重新触发状态机。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 断开的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientLinkDisconnectedInDisconnectingState(int32_t appId, uint8_t reason)
{
    // 链路发生了异常，直接上断链；
    SsapcAppSetLinkState(appId, SSAP_CONNECT_STATE_DISCONNECTED);
    SsapcAppLinkStateNofity(appId, SSAP_CONNECT_STATE_DISCONNECTED, NLSTK_ERRCODE_SUCCESS, 0);

    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op != SSAP_LINK_CHANGE_EVENT_BUTT) { // 表示没有缓存操作，说明当前service未使用这个链路
        SsapcClearLinkOper(appId);
        // 重新出发状态机，尝试下发缓存中的操作
        SsapClientLinkStateMachineCall(appId, op, reason);
    }
    return;
}

/**
 * @brief 处理用户在链路断开状态下发起的连接请求
 * @details 在链路断开状态下，用户发起连接请求时，根据底层链路的状态设置相应的连接状态或缓存操作。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 连接的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientUserConnectInDisconnectedState(int32_t appId, uint8_t reason)
{
    // 处理逻辑和idle情况下一致
    SsapAppClientUserConnectInIdleState(appId, reason);
}

/**
 * @brief 处理用户在链路断开状态下发起的断开请求
 * @details 在链路断开状态下，用户发起断开请求时，直接上报断链成功的状态。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] reason 断开的原因，具体含义由调用者定义
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientUserDisconnectInDisconnectedState(int32_t appId, uint8_t reason)
{
    // 直接上报断链成功的状态
    SsapcAppLinkStateNofity(appId, SSAP_CONNECT_STATE_DISCONNECTED, NLSTK_ERRCODE_SUCCESS, reason);
}

/**
 * @brief 处理链路在断开状态下连接成功的事件
 * @details 在appid对应的链路处于断链的情况下，收到CM上报的建链成功的事件，可能场景如下：
 *          - 本appId对应的链路已经断开，此时其它的appId也关联了此链路，然后出发了建链操作，CM上报了建链成功的事件
 *          - 此时，针对本appId不需要进行额外的操作；
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientLinkConnectedInDisconnectedState(int32_t appId, uint8_t reason)
{
    // 韧性处理，原则此时链路中无缓存操作；
    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op != SSAP_LINK_CHANGE_EVENT_BUTT) { // 表示没有缓存操作，说明当前service未使用这个链路
        SsapcClearLinkOper(appId);
        // 重新出发状态机，尝试下发缓存中的操作
        SsapClientLinkStateMachineCall(appId, op, reason);
    }
}

/**
 * @brief 处理链路在断开状态下连接成功的事件
 * @details 在appid对应的链路处于断链的情况下，收到CM上报的建链成功的事件，可能场景如下：
 *          - 本appId对应的链路已经断开，此时其它的appId也关联了此链路，其他的appId针对链路操作时，出发本appId的状态机
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @return void 无返回值
 * @note appId必须有效，否则函数直接返回
 */
static void SsapAppClientLinkDisconnectedInDisconnectedState(int32_t appId, uint8_t reason)
{
    // 有其它的appId正在断链，直接忽略；
    NLSTK_SsapClientLinkChangeEvent_E op = SsapcGetLinkOper(appId);
    if (op != SSAP_LINK_CHANGE_EVENT_BUTT) { // 表示没有缓存操作，说明当前service未使用这个链路
        SsapcClearLinkOper(appId);
        // 重新出发状态机，尝试下发缓存中的操作
        SsapClientLinkStateMachineCall(appId, op, reason);
    }
}