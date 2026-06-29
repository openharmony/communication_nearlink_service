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
#ifndef SSAPC_APP_H
#define SSAPC_APP_H

#include "sdf_addr.h"
#include "ssap_common.h"
#include "nlstk_public_define.h"
#include "nlstk_ssap_app_link.h"
#include "nlstk_ssap_app_client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SSAP_USER_CONNECT = 0,              // 用户调用 NLSTK_SsapClientConnect 触发建链请求
    SSAP_USER_DISCONNECT,               // 用户调用 NLSTK_SsapClientDisconnect 触发断链请求
    SSAP_LOGIC_LINK_CONNECTED,          // 连接管理模块的链路状态变化通知，上报链路建立成功
    SSAP_LOGIC_LINK_DISCONNECTED,       // 连接管理模块的链路状态变化通知，上报链路故障成功
    SSAP_LINK_CHANGE_EVENT_BUTT,
} NLSTK_SsapClientLinkChangeEvent_E;

typedef struct {
    int32_t appId;
    SLE_Addr_S addr;
    NLSTK_SsapAppClientCb_S *cb;
    int64_t timeout;

    // 关联用户的是否做了NLSTK_SsapClientConnect或者NLSTK_SsapClientDisConnect操作
    // 由于建链是异步逻辑，同时在收到CM侧上报的连接状态变化时，也需要做对应的管理
    NLSTK_SsapConnectLinkState_E linkState;
    // 如果CM链路正在操作时，这里需要缓存下次操作；
    // 例如当service调用此appId进行建链的时候，发现底层CM正在断链，此时要等CM完成上一次操作之后再进行下一步操作
    NLSTK_SsapClientLinkChangeEvent_E nextOperator;
    NLSTK_ConnParam_S connParam;
} SsapcAppRegParam_S;

typedef struct {
    int32_t appId;
    int64_t timeout;
} SsapcAppTimeoutParam_S;

typedef struct {
    uint16_t handle;
    NLSTK_SsapUuid_S uuid;
    uint8_t errorCode;                                 // 错误码
    SSAP_LengthValue_S value;
} SsapcAppReadProperty_S;

void SsapcAppRegister(void *param);
void SsapcAppRegisterAsync(void *param);
void SsapcAppDeregister(void *param);
void SsapcAppSetInteractionTimeout(void *param);
void SsapcAppDiscServ(void *param);
void SsapcAppDiscServByUuid(void *param);
void SsapcAppExchangeMtu(void *param);
void SsapcAppGetServ(void *param);
void SsapcAppGetServAsyn(void *param);
void SsapcAppRead(void *param);
void SsapcAppReadProps(void *param);
void SsapcAppReadByUuid(void *param);
void SsapcAppWriteCmd(void *param);
void SsapcAppWriteReq(void *param);
void SsapcAppCallMethodCmd(void *param);
void SsapcAppCallMethodReq(void *param);
void SsapcAppGetCpcd(void *param);
void SsapcAppSetCpcd(void *param);
void SsapcClientCleanApp(void *param);
void SsapcAppNotifyClientCleanUp(void);
void SsapcClientAppDeinit(void);

SLE_Addr_S* GetAddrByAppId(int32_t appId);
NLSTK_ConnParam_S* GetConnParamByAppId(int32_t appId);
bool IsAppIdValid(int32_t appId);

void SsapcAppSetLinkState(int32_t appId, NLSTK_SsapConnectLinkState_E newState);
NLSTK_SsapConnectLinkState_E SsapcAppGetLinkState(int32_t appId);
uint32_t SsapcAppGetAppIdListByAddr(SLE_Addr_S *addr, int32_t *appIdList, uint32_t number);
void SsapcAppLinkStateNofity(int32_t appId, NLSTK_SsapConnectLinkState_E state, NLSTK_Errcode_E errCode,
    int32_t reason);

NLSTK_SsapClientLinkChangeEvent_E SsapcGetLinkOper(int32_t appId);
void SsapcClearLinkOper(int32_t appId);
void SsapcCacheLinkOper(int32_t appId, NLSTK_SsapClientLinkChangeEvent_E oper);

void SsapcAppPropertyNtf(SSAP_ValuePkt_S *valuePkt);

void SsapcAppServChange(SLE_Addr_S *addr, uint16_t startHandle, uint16_t endHandle);

#ifdef __cplusplus
}
#endif
#endif