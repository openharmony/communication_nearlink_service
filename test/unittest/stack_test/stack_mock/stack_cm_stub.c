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
#include "sdf_mem.h"
#include "stack_cm_stub.h"

#define CM_SUPPORT_MAX_MODULE_NUM 16

static CM_LogicLinkCbks_S *g_cmLogicLinkCbks = NULL;
static CM_TransChannelCbk g_cmTransChannelCbk = NULL;

uint32_t TEST_CM_RegLogicLinkListener(CM_LogicLinkCbks_S *logicLinkCbks)
{
    for (int i = 0; i < CM_SUPPORT_MAX_MODULE_NUM; i++) {
        CM_LogicLinkCbks_S *cbks = &g_cmLogicLinkCbks[i];
        if (cbks->moduleId == CM_MODULE_ID_MAX) {
            cbks->moduleId = logicLinkCbks->moduleId;
            cbks->logicLinkCbk = logicLinkCbks->logicLinkCbk;
            cbks->remoteFeaturesCbk = logicLinkCbks->remoteFeaturesCbk;
            cbks->connUpdateParamCbk = logicLinkCbks->connUpdateParamCbk;
            return 0;
        }
    }
    return 0;
}

uint32_t TEST_CM_UnRegLogicLinkListener(uint8_t moduleId)
{
    return 0;
}

uint32_t TEST_CM_RegTransChannelListener(CM_TransChannelCbk cbk)
{
    g_cmTransChannelCbk = cbk;
    return 0;
}

void TEST_CM_UnRegTransChannelListener(void)
{
    g_cmTransChannelCbk = NULL;
}

void TEST_CM_TransChannelDo(CM_TransChannelStateList_S *channelState)
{
    if (g_cmTransChannelCbk != NULL) {
        g_cmTransChannelCbk(channelState);
    }
}

uint32_t TEST_CM_GetLogicLinkByAddr(SLE_Addr_S *addr, CM_LogicLink_S *logicLink)
{
    if (logicLink == NULL) {
        return 1;
    }
    logicLink->lcid = 0;
    return 0;
}

void TEST_CM_CreateLogicLink(uint16_t moduleId, CM_LogicLinkState_S *logicLinkState)
{
    for (int i = 0; i < CM_SUPPORT_MAX_MODULE_NUM; i++) {
        CM_LogicLinkCbks_S *cbks = &g_cmLogicLinkCbks[i];
        if (cbks->moduleId == moduleId) {
            cbks->logicLinkCbk(logicLinkState);
        }
    }
    return;
}

void TEST_CM_RemoteFeatureCbk(uint16_t moduleId, CM_LogicLinkRemoteFeatures_S *param)
{
    for (int i = 0; i < CM_SUPPORT_MAX_MODULE_NUM; i++) {
        CM_LogicLinkCbks_S *cbks = &g_cmLogicLinkCbks[i];
        if (cbks->moduleId == moduleId) {
            cbks->remoteFeaturesCbk(param);
        }
    }
    return;
}

void TEST_CM_ConnUpdateCbk(uint16_t moduleId, CM_LogicLinkConnUpdateParam_S *param)
{
    for (int i = 0; i < CM_SUPPORT_MAX_MODULE_NUM; i++) {
        CM_LogicLinkCbks_S *cbks = &g_cmLogicLinkCbks[i];
        if (cbks->moduleId == moduleId) {
            cbks->connUpdateParamCbk(param);
        }
    }
    return;
}

void TEST_CM_Init()
{
    g_cmLogicLinkCbks = (CM_LogicLinkCbks_S *)SDF_MemZalloc(sizeof(CM_LogicLinkCbks_S) * CM_SUPPORT_MAX_MODULE_NUM);
    for (int i = 0; i < CM_SUPPORT_MAX_MODULE_NUM; i++) {
        CM_LogicLinkCbks_S *cbks = &g_cmLogicLinkCbks[i];
        cbks->moduleId = CM_MODULE_ID_MAX;
    }
    return;
}

void TEST_CM_DeInit()
{
    if (g_cmLogicLinkCbks != NULL) {
        SDF_MemFree(g_cmLogicLinkCbks);
        g_cmLogicLinkCbks =  NULL;
    }
    return;
}

uint32_t TEST_CM_GetLogicLinkConnectedSize(void)
{
    return 0;
}