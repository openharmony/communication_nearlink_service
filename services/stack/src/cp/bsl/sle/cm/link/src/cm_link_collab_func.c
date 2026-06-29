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

#include "cm_link_collab_func.h"
#include "cm_errno.h"
#include "securec.h"

static CM_LinkCollabFunc_S g_linkCollabFunc = {};

uint32_t CM_LinkCollabRegFunc(const CM_LinkCollabFunc_S *funcs)
{
    if (funcs == NULL) {
        return CM_INVALID_PARAM_ERR;
    }
    g_linkCollabFunc = *funcs;
    return CM_SUCCESS;
}

void CM_LinkCollabUnRegFunc(void)
{
    (void)memset_s(&g_linkCollabFunc, sizeof(CM_LinkCollabFunc_S), 0, sizeof(CM_LinkCollabFunc_S));
}

bool CM_IsNeedLinkCollabReq(void)
{
    return (g_linkCollabFunc.isNeedLinkCollabReq != NULL) && (g_linkCollabFunc.isNeedLinkCollabReq());
}

bool CM_StartLinkCollabReq(uint8_t connInitiateType, const SLE_Addr_S *directConnAddr)
{
    return (g_linkCollabFunc.startLinkCollabReq != NULL) &&
        (g_linkCollabFunc.startLinkCollabReq(connInitiateType, directConnAddr));
}

bool CM_NotifyLinkCollabResult(void)
{
    return (g_linkCollabFunc.notifyLinkCollabResult != NULL) && (g_linkCollabFunc.notifyLinkCollabResult());
}