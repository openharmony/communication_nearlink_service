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
#include "nlstk_public_define.h"
#include "hadm_link_manager.h"
#include "hadm_sm.h"
#include "hadm_ext_func_wrapper.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "hadm_listen_cm.h"

#define HADM_NEARLINK_FEATURE_INDEX 8         // 星闪查找特性在设备特性中的索引位置
#define HADM_NEARLINK_FEATURE_COMPETENCE 0x4  // 星闪查找特性对应的16进制值

static void HadmLinstenCmLinkReport(CM_LogicLinkState_S *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[SSAP] cm param is null");
    NLSTK_LOG_INFO("Hadm LinstenCmLinkReport cm link lcid: %d, addr: %s, result: %d", param->lcid,
                 GET_ENC_ADDR(&param->addr), param->result);
    if (param->result == CM_LINK_STATE_CONNECTED) {
        // 链路建立的时候，申请好linkCb，不触发状态机，状态机由底层上报远端支持能力触发
        HadmSoundCb_S *linkCb = HadmAllocLinkCb(&param->addr, param->lcid);
        if (linkCb == NULL) {
            NLSTK_LOG_ERROR("[HADM] alloc link cb fail");
            return;
        }
    } else if (param->result == CM_LINK_STATE_DISCONNECTED) {
        uint32_t ret = HadmTriggerStateMachine(&param->addr, CM_REPORT_LINK_STATE_DISCONNECTED, NULL);
        if (ret != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[HADM]triggers state machine failed, fail %d", ret);
        }
        HADM_ExtClearRemoteCsCaps(param->lcid);
        // 状态机触发完成之后，释放linkCb
        HadmFreeLinkCb(&param->addr);
    }
}

/**
 * @brief  CM监听注册函数的回调
 * @return void
 */
static void ReadRemoteFeatureCbk(CM_LogicLinkRemoteFeatures_S *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[HADM] CM Remote channel sounding callbacks nullptr.");
    bool isRemoteSupportSounding = false;
    if ((param->features[HADM_NEARLINK_FEATURE_INDEX] & HADM_NEARLINK_FEATURE_COMPETENCE) ==
        HADM_NEARLINK_FEATURE_COMPETENCE) {
        isRemoteSupportSounding = true;
    }
    NLSTK_LOG_INFO("[HADM]ReadRemoteFeatureCbk get remote cs CBK, isRemoteSupportSounding %d", isRemoteSupportSounding);
    HadmTriggerStateMachineByLcid(param->lcid, CM_REPORT_FEATURES_EVENT,
                                  &isRemoteSupportSounding);  // 触发状态机，进行状态的切换
}

/**
 * @brief  CM监听注册函数的回调
 * @return void
 */
static void HadmConnUpateParamCbk(CM_LogicLinkConnUpdateParam_S *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[HADM] CM connection update param callback nullptr.");
    NLSTK_LOG_INFO("[HADM]HadmConnUpateParamCbk result %d", param->result);
    bool ifUpdateSuccess = (param->result == 0);
    HadmTriggerStateMachineByLcid(param->lcid, CM_REPORT_UPDATE_CONN_PARAM_EVENT,
                                  &ifUpdateSuccess);  // 触发状态机，进行状态的切换
}

uint32_t HadmRegListenCmLink()
{
    CM_LogicLinkCbks_S cmLinkCbk = { 0 };
    cmLinkCbk.moduleId = CM_MODULE_HADM;
    cmLinkCbk.logicLinkCbk = HadmLinstenCmLinkReport;
    cmLinkCbk.remoteFeaturesCbk = ReadRemoteFeatureCbk;
    cmLinkCbk.connUpdateParamCbk = HadmConnUpateParamCbk;
    if (CM_RegLogicLinkListener(&cmLinkCbk) != CM_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM] Register CM logic link listener callback fail.");
        return NLSTK_HADM_ERRCODE_CALL_CM_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

void HadmUnregListenCmLink()
{
    (void)CM_UnRegLogicLinkListener(CM_MODULE_HADM);
    return;
}