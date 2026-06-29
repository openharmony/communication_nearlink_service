/**
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

#include "bnl_ext_func_wrapper.h"

#include <stdio.h>
#include "nlstk_public_define_ext.h"
#include "bnl_reg_ext_func.h"
#include "adapter_log.h"

void BNL_ProxyLinkStateChange(NLSTK_BnlLogicLinkState_S *state)
{
    BNL_ExtFuncList *funcList = BNL_GetExtFuncList();
    if (funcList == NULL || funcList->linkStateChange == NULL) {
        ADAPTER_LOGW("get func bnl link state change failed");
        return;
    }

    funcList->linkStateChange(state);
}

void BNL_ProxyRecvMsg(NLSTK_BnlSendMsg_S *msg)
{
    BNL_ExtFuncList *funcList = BNL_GetExtFuncList();
    if (funcList == NULL || funcList->recvMsg == NULL) {
        ADAPTER_LOGW("get func bnl recv msg failed");
        return;
    }

    funcList->recvMsg(msg);
}

uint32_t BNL_ProxyInit(NLSTK_BnlProxyFunc_S *func)
{
    BNL_ExtFuncList *funcList = BNL_GetExtFuncList();
    if (funcList == NULL || funcList->init == NULL) {
        ADAPTER_LOGW("get func bnl init failed");
        return NLSTK_ERRCODE_FAIL;
    }

    funcList->init(func);
    return NLSTK_ERRCODE_SUCCESS;
}

void BNL_ProxyDeInit(void)
{
    BNL_ExtFuncList *funcList = BNL_GetExtFuncList();
    if (funcList == NULL || funcList->deInit == NULL) {
        ADAPTER_LOGW("get func bnl deinit failed");
        return;
    }

    funcList->deInit();
}