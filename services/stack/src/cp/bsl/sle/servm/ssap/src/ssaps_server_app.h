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
#ifndef SSAPS_SERVER_APP_H
#define SSAPS_SERVER_APP_H

#include <stdint.h>
#include "ssap_common.h"
#include "nlstk_ssap_app_link.h"
#include "nlstk_ssap_app_server.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NLSTK_SSAP_SERVER_APP_MAX_NUM 64
#define SSAP_SERVER_APP_INVALID_APPID (-1)

typedef struct {
    int32_t appId;
    NLSTK_SsapAppServerCb_S cb;
} SsapRegServerAppParam_S;

typedef struct {
    int32_t appId;
    uint16_t handle;
    uint16_t reserved;
} SsapServerRemoveAppServiceParam_S;

typedef struct {
    int32_t appId;
    NLSTK_SsapUuid_S uuid;
    bool isServiceExist;
} SsapServerCheckServiceExistParam_S;

typedef struct {
    int32_t appId;
    uint16_t handle;
    uint16_t reserved;
    NLSTK_VariableData_S *value;
} SsapServerUpdatePropertyParam_S;

/**
 * @brief 注册Ssap服务端的App，该函数属于内部调用，外部不可调用
 *
 * @param param 用户传入的基于App的回调函数
 */
void SsapServerRegApp(void *param);

void SsapServerRegAppAsyn(void *param);

/**
 * @brief 释放一个注册的Ssap服务端的App，该函数属于内部调用，外部不可调用
 *
 * @param param 待释放的App的ID
 */
void SsapServerDeregisterApplication(void *param);

/**
 * @brief 根据UUID查询App的服务，该函数属于内部调用，外部不可调用
 *
 * @param param 查询的UUID
 */
void SsapServerCheckServiceExistByUuid(void *param);

/**
 * @brief 删除一个注册的Ssap服务端的App的服务，该函数属于内部调用，外部不可调用
 *
 * @param param 待删除的服务的信息
 */
void SsapServerRemoveAppService(void *param);

/**
 * @brief 删除一个注册的Ssap服务端的App上的所有服务，该函数属于内部调用，外部不可调用
 *
 * @param param 待删除的服务的信息
 */
void SsapServerCleanAppService(void *param);

void SsapServerGetAppServiceNums(void *param);

void SsapServerGetAppServices(void *param);

void SsapServerLinkStateNofity(SLE_Addr_S *addr, NLSTK_SsapConnectLinkState_E state, NLSTK_Errcode_E errCode,
                               int32_t reason);

void SsapServerAppAddServiceExceptionCallBack(SsapAsyncProcessResult_S *result);
void SsapServerAppStartServiceCallBack(SSAP_Service_S *service);
void SsapServerAppWriteValueCallback(SSAP_BufferedOperation_S *operation);
void SsapServerAppReadValueCallback(SSAP_BufferedOperation_S *operation);
void SsapServerUpdateValueResultCallback(SSAP_BufferedOperation_S *operation);
void SsapServerAppCallMethodCallback(SSAP_BufferedOperation_S *methodCall);
void SsapServerAppMtuExchangeCallBack(SSAP_MtuInfo_S *mtuInfo);
void SsapServerAppIndicateCfmValueCallback(SSAP_ValuePkt_S *valuePkt);

void SsapServerCleanApp(void *param);
void SsapServerAppCleanUpNotify(void);
void SsapServerAppDeinit(void);

#ifdef __cplusplus
}
#endif
#endif