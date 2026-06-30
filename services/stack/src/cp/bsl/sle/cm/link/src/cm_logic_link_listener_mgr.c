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

#include "cm_logic_link_listener_mgr.h"
#include "securec.h"
#include "cm_errno.h"
#include "cm_log.h"
#include "cm_api.h"
#include "cm_concurrent_conn.h"
#include "cm_link_collab_func.h"

static CM_LogicLinkCbks_S g_logicLinkCbks[CM_MODULE_ID_MAX] = { 0 };

uint32_t CM_RegLogicLinkCbks(CM_LogicLinkCbks_S *cbks)
{
    for (uint8_t i = 0; i < (uint8_t)(sizeof(g_logicLinkCbks) / sizeof(CM_LogicLinkCbks_S)); i++) {
        if (cbks->moduleId == i) {
            g_logicLinkCbks[i] = *cbks;
            return CM_SUCCESS;
        }
    }
    CM_LOGE("CM_RegLogicLinkCbks failed, moduleId:%hhu", cbks->moduleId);
    return CM_FAIL;
}

void CM_ExecLogicLinkModuleCbks(uint8_t moduleId, CM_LogicLinkState_S *state)
{
    if (g_logicLinkCbks[moduleId].logicLinkCbk != NULL) {
        g_logicLinkCbks[moduleId].logicLinkCbk(state);
        CM_LOGI("moduleId:%hhu, logic link cbk end", g_logicLinkCbks[moduleId].moduleId);
    }
}

void CM_NotifyLogicLinkDtapCbks(CM_LogicLinkState_S *state)
{
    if ((state->result != CM_LINK_STATE_CONNECTED)) {
        return;
    }
    CM_LOGI("notify dtap logic link state changed, lcid:0x%04x, role:%hhu, addr:%s, discReason:0x%02x, "
            "result:0x%02x", state->lcid, state->role, GET_ENC_ADDR(&state->addr), state->discReason, state->result);
    CM_ExecLogicLinkModuleCbks(CM_MODULE_DTAP, state);
}

void CM_NotifyLogicLinkCbks(CM_LogicLinkState_S *state)
{
    CM_LOGI("notify logic link state changed, lcid:0x%04x, role:%hhu, addr:%s, discReason:0x%02x, result:0x%02x, "
        "connCompleteType:%hhu, advHandle:0x%02x", state->lcid, state->role, GET_ENC_ADDR(&state->addr),
        state->discReason, state->result, state->connCompleteType, state->advHandle);
    uint8_t count = (uint8_t)(sizeof(g_logicLinkCbks) / sizeof(CM_LogicLinkCbks_S));
    if (state->result == CM_LINK_STATE_CONNECTED) {
        for (uint8_t i = 0; i < count; i++) {
            if (i == CM_MODULE_DTAP) {
                continue;
            }
            CM_ExecLogicLinkModuleCbks(i, state);
        }
        // DTAP模块需要根据CM模块做链路创建确认上报，在收到对端数据前，若未读对端版本完成，需要先做缓存，即延迟上报。
        CM_NotifyLogicLinkDtapCbks(state);
    } else if (state->result == CM_LINK_STATE_DISCONNECTED) {
        // 逆序执行，跟连接时相反
        for (uint8_t i = count; i > 0; i--) {
            CM_ExecLogicLinkModuleCbks(i - 1, state);
        }
    }

    if ((state->connCompleteType == CM_CONN_COMPLETE_SCAN) &&
        ((state->result == CM_LINK_STATE_CONNECTED) || (state->result == CM_LINK_STATE_DISCONNECTED))) {
        CM_ConcurrentConnNotifyLinkCollabResult();
    }
}

void CM_ExecLogicLinkRemoteFeaturesCbks(CM_LogicLinkRemoteFeatures_S *param)
{
    for (uint8_t i = 0; i < (uint8_t)(sizeof(g_logicLinkCbks) / sizeof(CM_LogicLinkCbks_S)); i++) {
        if (g_logicLinkCbks[i].remoteFeaturesCbk != NULL) {
            g_logicLinkCbks[i].remoteFeaturesCbk(param);
            CM_LOGI("moduleId:%hhu, logic link remote features cbk end", g_logicLinkCbks[i].moduleId);
        }
    }
}

void CM_ExecLogicLinkConnUpdateParamCbks(CM_LogicLinkConnUpdateParam_S *param)
{
    CM_LOGI("sle exec logic link update param cbks start");
    for (uint8_t i = 0; i < (uint8_t)(sizeof(g_logicLinkCbks) / sizeof(CM_LogicLinkCbks_S)); i++) {
        if (g_logicLinkCbks[i].connUpdateParamCbk != NULL) {
            g_logicLinkCbks[i].connUpdateParamCbk(param);
            CM_LOGI("moduleId:%hhu, logic link connect update param cbk end", g_logicLinkCbks[i].moduleId);
        }
    }
}

uint32_t CM_UnRegLogicLinkCbks(uint16_t moduleId)
{
    g_logicLinkCbks[moduleId].moduleId = 0;
    g_logicLinkCbks[moduleId].logicLinkCbk = NULL;
    g_logicLinkCbks[moduleId].remoteFeaturesCbk = NULL;
    g_logicLinkCbks[moduleId].connUpdateParamCbk = NULL;
    return CM_SUCCESS;
}