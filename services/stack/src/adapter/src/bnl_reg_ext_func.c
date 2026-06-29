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

#include "bnl_reg_ext_func.h"

#include <dlfcn.h>
#include <stdio.h>

#include "securec.h"
#include "adapter_log.h"

static BNL_ExtFuncList g_funcList = {0};

void BNL_RegisterExtFunc(void *soHandle)
{
    if (soHandle == NULL) {
        ADAPTER_LOGW("null handle");
        return;
    }

    g_funcList.linkStateChange = (BnlProxyLinkStateChangePtr)dlsym(soHandle, "BNL_ProxyLinkStateChange");
    g_funcList.recvMsg = (BnlProxyRecvMsgPtr)dlsym(soHandle, "BNL_ProxyRecvMsg");
    g_funcList.init = (BnlProxyInitPtr)dlsym(soHandle, "BNL_ProxyInit");
    g_funcList.deInit = (BnlProxyDeInitPtr)dlsym(soHandle, "BNL_ProxyDeInit");

    ADAPTER_LOGI("bnl register ext func finished");
}

void BNL_DeregisterExtFunc(void)
{
    (void)memset_s(&g_funcList, sizeof(BNL_ExtFuncList), 0, sizeof(BNL_ExtFuncList));
    ADAPTER_LOGI("bnl deregister ext func finished");
}

BNL_ExtFuncList *BNL_GetExtFuncList(void)
{
    return &g_funcList;
}