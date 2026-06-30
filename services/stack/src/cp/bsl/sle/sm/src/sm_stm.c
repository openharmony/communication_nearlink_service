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
/*
 * STM: state machine.
 * 本文件实现了配对流程状态机.
 * SmStateMachine_S 继承自 SDF_StateMachine.
 * 参考 sdf_stm.c 以获得更多信息.
 *
 * 共有六个状态,
 *      + INIT - 配对流程尚未开始.
 *      + NEGO - 根据协议决策配对时使用的密码学算法, 鉴权方式等.
 *      + AUTH - 根据指定鉴权方式验证公钥真实性.
 *      + ENCP - 使能加密能力.
 *      + FULL - 已经配对状态.
 *      + REMV - 执行配对删除.
 * 以 XxxxDispatch() 命名的函数描述了状态机的转移行为,
 * 例如 InitDispatch()
 *
 * 以 XxxxEntry() 和 XxxxExit() 命名的函数描述了进入和退出状态时的行为.
 */

#include <stddef.h>
#include <stdbool.h>
#include "sdf_mem.h"
#include "nlstk_log.h"
#include "cp_worker.h"
#include "cm.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "dli_cmd.h"
#include "dli_errno.h"

#include "sm.h"
#include "sm_errcode.h"
#include "sm_slink.h"
#include "sm_nego.h"
#include "sm_auth.h"
#include "sm_encp.h"
#include "sm_dft.h"
#include "sm_stm.h"

const char *g_smStateName[] = {
    [SM_STATE_INIT] = "SLINK_INIT",
    [SM_STATE_NEGO] = "NEGOTIATING",
    [SM_STATE_AUTH] = "AUTHENTICATING",
    [SM_STATE_ENCP] = "SECU_CTRL_PROC",
    [SM_STATE_MISS] = "KEY_MISSING",
    [SM_STATE_FULL] = "SLINK_FULL",
    [SM_STATE_REMV] = "SLINK_REMOVE",
};

static State *CreateInitState(StateMachine *stm);
static State *CreateNegoState(StateMachine *stm);
static State *CreateAuthState(StateMachine *stm);
static State *CreateEncpState(StateMachine *stm);
static State *CreateMissState(StateMachine *stm);
static State *CreateFullState(StateMachine *stm);
static State *CreateRemvState(StateMachine *stm);

void SmStateMachineDtor(SmStateMachine_S *stm)
{
    if (stm != NULL) {
        StateMachineSoftBaseDtor((StateMachine *)stm);
        SDF_MemFree(stm);
    }
}

SmStateMachine_S *SmStateMachineCtor(SmSLink_S *slink)
{
    SmStateMachine_S *stm = SDF_MemZalloc(sizeof(SmStateMachine_S)); // SmStateMachineDtor
    if (stm == NULL || !StateMachineSoftBaseCtor((StateMachine *)stm)) {
        SmStateMachineDtor(stm);
        return NULL;
    }
    stm->slink = slink;

    State *init = CreateInitState((StateMachine *)stm);
    State *nego = CreateNegoState((StateMachine *)stm);
    State *auth = CreateAuthState((StateMachine *)stm);
    State *encp = CreateEncpState((StateMachine *)stm);
    State *miss = CreateMissState((StateMachine *)stm);
    State *full = CreateFullState((StateMachine *)stm);
    State *fail = CreateRemvState((StateMachine *)stm);

    if (init == NULL || nego == NULL || auth == NULL || encp == NULL || miss == NULL || full == NULL || fail == NULL) {
        goto FAIL_LABEL;
    }

    bool ret = true;
    ret = ret && STM_MFUNC(stm, EmplaceNewState, init);
    ret = ret && STM_MFUNC(stm, EmplaceNewState, nego);
    ret = ret && STM_MFUNC(stm, EmplaceNewState, auth);
    ret = ret && STM_MFUNC(stm, EmplaceNewState, encp);
    ret = ret && STM_MFUNC(stm, EmplaceNewState, miss);
    ret = ret && STM_MFUNC(stm, EmplaceNewState, full);
    ret = ret && STM_MFUNC(stm, EmplaceNewState, fail);
    if (!ret) {
        goto FAIL_LABEL;
    }

    STM_MFUNC(stm, Transition, g_smStateName[SM_STATE_INIT]);
    return stm;

FAIL_LABEL:
    StateDtor(init);
    StateDtor(nego);
    StateDtor(auth);
    StateDtor(encp);
    StateDtor(miss);
    StateDtor(full);
    StateDtor(fail);
    SmStateMachineDtor(stm);
    return NULL;
}

/*
 * +------------------+---------------+
 * | SM State Machine | SM Init State |
 * +------------------+---------------+
 */

static void InitEntry(State *state)
{
    NLSTK_LOG_INFO("[SM] State machine enter init state.");
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    slink->curStateIndex = SM_STATE_INIT;
}

static void InitHandlePassiveStart(State *state, SmSLink_S *slink)
{
    if (SmSLinkBindLogicLink(slink)) {
        state->Transition(state, g_smStateName[SM_STATE_NEGO]);
        return;
    }
    state->Transition(state, g_smStateName[SM_STATE_REMV]);
}

static void InitHandleActiveStart(State *state, SmSLink_S *slink)
{
    if (SmSLinkBindLogicLink(slink)) {
        state->Transition(state, g_smStateName[SM_STATE_NEGO]);
        SmNegoStart(slink);
        return;
    }
    state->Transition(state, g_smStateName[SM_STATE_REMV]);
}

static void InitHandleEncpParamReqReply(State *state, SmSLink_S *slink)
{
    SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_KEY_MISSING);
    NLSTK_LOG_ERROR("[SM] Pin or key missing since local device in %s state.", g_smStateName[SM_STATE_INIT]);
    SmSLinkAuthCbk(slink, SM_KEY_MISSING);
    if (SmSLinkBindLogicLink(slink)) {
        uint32_t ret = DLI_EncryptionParamReqNegativeReply(&(DLI_ConnHandleStru) { .connHandle = slink->lcid });
        if (ret != DLI_SUCCESS) {
            NLSTK_LOG_ERROR("[SM] Send encryption negative reply command failure.");
            state->Transition(state, g_smStateName[SM_STATE_REMV]);
        }
    }
}

static void InitDispatch(State *state, Message msg)
{
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    switch (msg.what) {
        case SM_PASSIVE_START: {
            InitHandlePassiveStart(state, slink);
            break;
        }
        case SM_ACTIVE_START: {
            InitHandleActiveStart(state, slink);
            break;
        }
        case SM_RECOVER_START: {
            state->Transition(state, g_smStateName[SM_STATE_ENCP]);
            break;
        }
        case SM_INTERNAL_ERROR: {
            NLSTK_LOG_ERROR("[SM] Error occurred in %s state.", g_smStateName[SM_STATE_INIT]);
            state->Transition(state, g_smStateName[SM_STATE_REMV]);
            break;
        }
        case SM_REMOVE_PAIR: {
            state->Transition(state, g_smStateName[SM_STATE_REMV]);
            break;
        }
        case SM_ENCP_PARAM_REQ_REPLY: {
            InitHandleEncpParamReqReply(state, slink);
            break;
        }
        default: {
            NLSTK_LOG_ERROR("[SM] Unexpected event %d in %s state", msg.what, g_smStateName[SM_STATE_INIT]);
            break;
        }
    }
}

State *CreateInitState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_smStateName[SM_STATE_INIT]);
    NLSTK_CHECK_RETURN(state != NULL, NULL, "[SM] create init state failed");
    state->Entry = InitEntry;
    state->Dispatch = InitDispatch;
    return state;
}

/*
 * +------------------+---------------+
 * | SM State Machine | SM Nego State |
 * +------------------+---------------+
 */

static void NegoEntry(State *state)
{
    NLSTK_LOG_INFO("[SM] State machine enter negotiating state.");
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    slink->curStateIndex = SM_STATE_NEGO;
    slink->dispatcher = SmNegoPkgDispatcher;
    slink->expectOpCode = slink->role == SM_G_NODE ? SM_NEGO_PAIRING_START : SM_NEGO_PAIRING_REQUEST;
}

static void NegoHandleDisconn(State *state, SmSLink_S *slink)
{
    NLSTK_LOG_ERROR("[SM] Error occurred in %s state.", g_smStateName[SM_STATE_NEGO]);
    SmSLinkAuthCbk(slink, SM_LINK_DISCONNCTED);
    state->Transition(state, g_smStateName[SM_STATE_REMV]);
}

static void NegoHandleError(State *state, Message msg, SmSLink_S *slink)
{
    NLSTK_LOG_ERROR("[SM] Error occurred in %s state.", g_smStateName[SM_STATE_NEGO]);
    if (msg.extData != NULL) {
        uint8_t pkg = (uint8_t)(uintptr_t)msg.extData;
        SmSLinkAuthCbk(slink, pkg);
    }
    state->Transition(state, g_smStateName[SM_STATE_REMV]);
}


static void NegoHandleRemovePair(State *state, Message msg, SmSLink_S *slink)
{
    NLSTK_LOG_INFO("[SM] pair process aborted mannually by remove pair");
    if ((uintptr_t)msg.extData == SM_ERR_ACTIVE_CANCEL) {
        NLSTK_LOG_ERROR("[SM] Active cancel in %s state.", g_smStateName[SM_STATE_NEGO]);
        uint8_t pkg = SM_ERR_ACTIVE_CANCEL;
        SmSLinkAuthCbk(slink, SM_ERR_ACTIVE_CANCEL);
        bool ret = SmSendMessage(slink, SM_PAIR_FAIL_MESSAGE_OPCODE, &pkg, 1);
        NLSTK_CHECK_RETURN_VOID(ret, "[SM] Send Message failed.");
    }
    state->Transition(state, g_smStateName[SM_STATE_REMV]);
}

static void NegoHandleEncpParamReqReply(State *state, SmSLink_S *slink)
{
    NLSTK_LOG_ERROR("[SM] Pin or key missing since local device in %s state.", g_smStateName[SM_STATE_NEGO]);
    SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_KEY_MISSING);
    SmSLinkAuthCbk(slink, SM_KEY_MISSING);
    uint32_t ret = DLI_EncryptionParamReqNegativeReply(&(DLI_ConnHandleStru) { .connHandle = slink->lcid });
    if (ret != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Send encryption negative reply command failure.");
        state->Transition(state, g_smStateName[SM_STATE_REMV]);
    }
}

static void NegoDispatch(State *state, Message msg)
{
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    switch (msg.what) {
        case SM_NEGO_SUCCESS: {
            state->Transition(state, g_smStateName[SM_STATE_AUTH]);
            break;
        }
        case SM_LCHANNEL_DISCONN:
            NegoHandleDisconn(state, slink);
            break;
        case SM_INTERNAL_ERROR:
        case SM_EXTERNAL_ERROR:
        case SM_TIMEOUT: {
            NegoHandleError(state, msg, slink);
            break;
        }
        case SM_REMOVE_PAIR: {
            NegoHandleRemovePair(state, msg, slink);
            break;
        }
        case SM_ENCP_PARAM_REQ_REPLY: {
            NegoHandleEncpParamReqReply(state, slink);
            break;
        }
        case SM_PASSIVE_START: break; // ignored event
        default: {
            NLSTK_LOG_ERROR("[SM] Unexpected event %d in %s state", msg.what, g_smStateName[SM_STATE_NEGO]);
            break;
        }
    }
}

State *CreateNegoState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_smStateName[SM_STATE_NEGO]);
    NLSTK_CHECK_RETURN(state != NULL, NULL, "[SM] create nego state failed");
    state->Entry = NegoEntry;
    state->Dispatch = NegoDispatch;
    return state;
}

/*
 * +------------------+---------------+
 * | SM State Machine | SM Auth State |
 * +------------------+---------------+
 */

static void AuthEntry(State *state)
{
    NLSTK_LOG_INFO("[SM] State machine enter authentication state.");
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    slink->curStateIndex = SM_STATE_AUTH;

    uint8_t authMethod = slink->negoParams.authMethod;
    slink->dispatcher = SmGetAuthPkgDispatcher(authMethod);

    SmAuthStartFunc_S authStartFunc = SmGetAuthStartFunc(authMethod);
    NLSTK_CHECK_RETURN_VOID(authStartFunc != NULL, "[SM] Auth algorithm %d not implement.", authMethod);
    authStartFunc(slink);
}

static void AuthDispatch(State *state, Message msg)
{
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    switch (msg.what) {
        case SM_AUTH_SUCCESS: {
            SmSLinkAuthCbk(slink, SM_PAIR_OK);
            state->Transition(state, g_smStateName[SM_STATE_ENCP]);
            break;
        }
        case SM_LCHANNEL_DISCONN: {
            NLSTK_LOG_ERROR("[SM] Error occurred in %s state.", g_smStateName[SM_STATE_AUTH]);
            SmSLinkAuthCbk(slink, SM_LINK_DISCONNCTED);
            state->Transition(state, g_smStateName[SM_STATE_REMV]);
            break;
        }
        case SM_INTERNAL_ERROR:
        case SM_EXTERNAL_ERROR:
        case SM_TIMEOUT: {
            NLSTK_LOG_ERROR("[SM] Error occurred in %s state.", g_smStateName[SM_STATE_AUTH]);
            if (msg.extData != NULL) {
                uint8_t pkg = (uint8_t)(uintptr_t)msg.extData;
                SmSLinkAuthCbk(slink, pkg);
            }
            state->Transition(state, g_smStateName[SM_STATE_REMV]);
            break;
        }
        case SM_REMOVE_PAIR: {
            NLSTK_LOG_INFO("[SM] pair process aborted mannually by remove pair");
            if ((uintptr_t)msg.extData == SM_ERR_ACTIVE_CANCEL) {
                NLSTK_LOG_ERROR("[SM] Active cancel in %s state.", g_smStateName[SM_STATE_AUTH]);
                uint8_t pkg = SM_ERR_ACTIVE_CANCEL;
                SmSLinkAuthCbk(slink, SM_ERR_ACTIVE_CANCEL);
                bool ret = SmSendMessage(slink, SM_PAIR_FAIL_MESSAGE_OPCODE, &pkg, 1);
                NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
            }
            state->Transition(state, g_smStateName[SM_STATE_REMV]);
            break;
        }
        case SM_PASSIVE_START: break; // ignored event
        default: {
            NLSTK_LOG_ERROR("[SM] Unexpected event %d in %s state", msg.what, g_smStateName[SM_STATE_AUTH]);
            break;
        }
    }
}

State *CreateAuthState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_smStateName[SM_STATE_AUTH]);
    NLSTK_CHECK_RETURN(state != NULL, NULL, "[SM] create auth state failed");
    state->Entry = AuthEntry;
    state->Dispatch = AuthDispatch;
    return state;
}

/*
 * +------------------+---------------+
 * | SM State Machine | SM Encp State |
 * +------------------+---------------+
 */

static void EncpEntry(State *state)
{
    NLSTK_LOG_DEBUG("[SM] State machine enter security control process state.");
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    slink->curStateIndex = SM_STATE_ENCP;
    if (slink->role == SM_G_NODE) {
        SmEncpEnable(slink);
    }
}

/* EncpDispatch Handle处理函数 */
static void EncpHandleActiveStart(State *state)
{
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    if (SmSLinkBindLogicLink(slink)) {
        SmEncpEnable(slink);
    }
}

static void EncpHandleSuccess(State *state)
{
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    SmSLinkEncpCbk(slink, SM_PAIR_OK);
    state->Transition(state, g_smStateName[SM_STATE_FULL]);
}

static void EncpHandleParamReqReply(State *state)
{
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    if (SmSLinkBindLogicLink(slink)) {
        SmEncpParamReqReplyProcess(slink);
    } else {
        NLSTK_LOG_ERROR("[SM] Encrypt param request reply bind logic link error.");
    }
}

static void EncpHandleInternalError(State *state, Message msg)
{
    NLSTK_LOG_ERROR("[SM] Error occurred in %s state.", g_smStateName[SM_STATE_ENCP]);
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    if (msg.extData != NULL) {
        uint8_t pkg = (uint8_t)(uintptr_t)msg.extData;
        SmSLinkEncpCbk(slink, pkg);
    }
}

static void EncpHandleEncpFail(State *state, Message msg)
{
    NLSTK_LOG_ERROR("[SM] Start to handle encryption fail.");
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    if ((uintptr_t)msg.extData == SM_ERR_STATUS_KEY_MISSING) {
        NLSTK_LOG_ERROR("[SM] Key missing during encryption");
        state->Transition(state, g_smStateName[SM_STATE_MISS]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_MISS_HANDLE });
    } else {
        SmSLinkEncpCbk(slink, (uint8_t)(uintptr_t)msg.extData);
    }
}

static void EncpHandleRemovePair(State *state)
{
    NLSTK_LOG_INFO("[SM] Remove paired slink");
    state->Transition(state, g_smStateName[SM_STATE_REMV]);
}

static void EncpDispatch(State *state, Message msg)
{
    switch (msg.what) {
        case SM_ACTIVE_START: {
            EncpHandleActiveStart(state);
            break;
        }
        case SM_RECOVER_START: break; // ignored event
        case SM_ENCP_SUCCESS: {
            EncpHandleSuccess(state);
            break;
        }
        case SM_ENCP_PARAM_REQ_REPLY: {
            EncpHandleParamReqReply(state);
            break;
        }
        case SM_INTERNAL_ERROR:
        case SM_EXTERNAL_ERROR: {
            EncpHandleInternalError(state, msg);
            break;
        }
        case SM_LCHANNEL_DISCONN: {
            SmSLink_S *slink = SM_STM_M(state->stm_, slink);
            SmSLinkEncpCbk(slink, SM_LINK_DISCONNCTED);
            break;
        }
        case SM_ENCP_FAIL: {
            EncpHandleEncpFail(state, msg);
            break;
        }
        case SM_REMOVE_PAIR: {
            EncpHandleRemovePair(state);
            break;
        }
        default: {
            NLSTK_LOG_ERROR("[SM] Unexpected event %d in %s state", msg.what, g_smStateName[SM_STATE_ENCP]);
            break;
        }
    }
}

State *CreateEncpState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_smStateName[SM_STATE_ENCP]);
    NLSTK_CHECK_RETURN(state != NULL, NULL, "[SM] create encp state failed");
    state->Entry = EncpEntry;
    state->Dispatch = EncpDispatch;
    return state;
}

/*
 * +------------------+---------------+
 * | SM State Machine | SM Miss State |
 * +------------------+---------------+
 */

static void MissEntry(State *state)
{
    NLSTK_LOG_INFO("[SM] State machine enter key missing state.");
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    SmSLinkEncpCbk(slink, SM_KEY_MISSING);
    slink->curStateIndex = SM_STATE_MISS;
}

static void MissDispatch(State *state, Message msg)
{
    switch (msg.what) {
        case SM_MISS_HANDLE: {
            SmSLink_S *slink = SM_STM_M(state->stm_, slink);
            if (SmSLinkBindLogicLink(slink)) {
                state->Transition(state, g_smStateName[SM_STATE_NEGO]);
                if (slink->role == SM_G_NODE) {
                    SmNegoStart(slink);
                }
            }
            break;
        }
        case SM_LCHANNEL_DISCONN: {
            NLSTK_LOG_ERROR("[SM] logic channel disconn");
            state->Transition(state, g_smStateName[SM_STATE_ENCP]);
            break;
        }
        case SM_REMOVE_PAIR: {
            NLSTK_LOG_INFO("[SM] Remove paired slink in %s state", g_smStateName[SM_STATE_MISS]);
            state->Transition(state, g_smStateName[SM_STATE_REMV]);
            break;
        }
        default: {
            NLSTK_LOG_ERROR("[SM] Unexpected event %d in %s state", msg.what, g_smStateName[SM_STATE_MISS]);
            break;
        }
    }
}

State *CreateMissState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_smStateName[SM_STATE_MISS]);
    NLSTK_CHECK_RETURN(state != NULL, NULL, "[SM] create miss state failed");
    state->Entry = MissEntry;
    state->Dispatch = MissDispatch;
    return state;
}

/*
 * +------------------+---------------+
 * | SM State Machine | SM Full State |
 * +------------------+---------------+
 */

static void FullEntry(State *state)
{
    NLSTK_LOG_DEBUG("[SM] state machine ends in full state.");
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    slink->curStateIndex = SM_STATE_FULL;
}

static void FullDispatch(State *state, Message msg)
{
    switch (msg.what) {
        case SM_LCHANNEL_DISCONN: {
            NLSTK_LOG_ERROR("[SM] logic channel disconn");
            state->Transition(state, g_smStateName[SM_STATE_ENCP]);
            break;
        }
        case SM_REMOVE_PAIR: {
            NLSTK_LOG_INFO("[SM] remove paired slink");
            state->Transition(state, g_smStateName[SM_STATE_REMV]);
            break;
        }
        case SM_ACTIVE_START: {
            NLSTK_LOG_INFO("[SM] recevie pair event while already paired");
            SmSLink_S *slink = SM_STM_M(state->stm_, slink);
            SmSLinkEncpCbk(slink, SM_PAIR_OK);
            break;
        }
        default: {
            NLSTK_LOG_ERROR("[SM] Unexpected event %d in %s state", msg.what, g_smStateName[SM_STATE_FULL]);
            break;
        }
    }
}

State *CreateFullState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_smStateName[SM_STATE_FULL]);
    NLSTK_CHECK_RETURN(state != NULL, NULL, "[SM] create full state failed");
    state->Entry = FullEntry;
    state->Dispatch = FullDispatch;
    return state;
}

/*
 * +------------------+---------------+
 * | SM State Machine | SM Remv State |
 * +------------------+---------------+
 */

static void RemvEntry(State *state)
{
    NLSTK_LOG_INFO("[SM] State machine enter remv state.");
    SmSLink_S *slink = SM_STM_M(state->stm_, slink);
    slink->curStateIndex = SM_STATE_REMV;
    slink->lcid = CM_INVALID_LCID;
    // why CM_ConnectReleaseReq.
    //     nearlink_service may call DisconnectAcb and CancelPairing in reverse order,
    //     and if CancelPairing occurs first, DisconnectAcb will fail.
    //     To address this, we add release action here. This is a temporary workaround.
    uint32_t ret = CM_DirectConnectRemove(CM_MODULE_ADPT, &slink->rmtAddr, CM_DISC_REASON_CANCEL_PAIR);
    if (ret != CM_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] direct connect remove failed, ret:0x%08x", ret);
        return;
    }
    SmExternalCbks(SM_CBK_EVENT_PAIRING_REMOVE, &(NLSTK_SmPairingRemove_S) {
        .lcid = slink->lcid, .addr = slink->rmtAddr, .removeStatus = SM_PAIR_OK});
    SmRemoveSLink((void *)&slink->rmtAddr);
}

State *CreateRemvState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_smStateName[SM_STATE_REMV]);
    NLSTK_CHECK_RETURN(state != NULL, NULL, "[SM] create remv state failed");
    state->Entry = RemvEntry;
    state->Dispatch = NULL;
    return state;
}