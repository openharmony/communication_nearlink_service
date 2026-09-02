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

#include "qosm_trans_channel_tuning.h"
#include "securec.h"
#include "common_ext_func_wrapper.h"
#include "cp_errno_base.h"
#include "cp_worker.h"
#include "sdf_addr.h"
#include "sdf_dlist.h"
#include "sdf_map.h"
#include "sdf_mem.h"
#include "sdf_struct.h"
#include "sdf_stm.h"
#include "sdf_func.h"
#include "sdf_timer.h"
#include "sle_logic_link_mgr.h"
#include "cm_def.h"
#include "cm_errno.h"
#include "cm_dyn_trans_channel_api.h"
#include "qosm_errno.h"
#include "qosm_log.h"

#define QOSM_SLE_CONN_NORMAL_SPEED_INTERAL 0x64      // 链路调度最小间隔 100 * 0.125 = 12.5ms
#define QOSM_SLE_CONN_HIGH_SPEED_INTERAL 0x24        // 链路调度最小间隔 36 * 0.125ms = 4.5 ms
#define QOSM_SLE_NORMAL_MCS 6
#define QOSM_SLE_HIGH_MCS   8

#define QOSM_SLE_CONN_EVENT_IFS 125             // 链路调度间隔, 125us
#define QOSM_SLE_CONN_TIME_UNIT 4               // 系统调度时隙, 125us
#define QOSM_SLE_CONN_SUPERVISION_TIMEOUT 1000  // 单位: 10ms, 超时时间10s

#define QOSM_TUNING_CONN_UPDATE_TIMEOUT_MSEC 11000U  // 11s (DLI层超时时延为10s)
#define QOSM_TUNING_SET_PHY_TIMEOUT_MSEC 11000U      // 11s (DLI层超时时延为10s)
#define QOSM_TUNING_SET_MCS_TIMEOUT_MSEC 3000U      // 3s (没有空中对端交互，命令下发成功即可)
#define QOSM_TUNING_TIMER_NO_USED_HANDLE (-1)

enum {
    QOSM_SLE_PHY_TYPE_1M = 0x0,  // 1M PHY
    QOSM_SLE_PHY_TYPE_4M = 0x2,  // 4M PHY
};

enum {
    QOSM_SLE_PILOT_DENSITY_4_TO_1 = 0x0,   // 导频密度为4:1
    QOSM_SLE_PILOT_DENSITY_16_TO_1 = 0x2,  // 导频密度为16:1
    QOSM_SLE_PILOT_DENSITY_NO_SET = 0x3,   // 无需设置导频密度
};

typedef enum {
    QOSM_TC_TUNING_STATE_START,
    QOSM_TC_TUNING_STATE_WAIT_CONN_UPDATE_PARAMS_RSP,
    QOSM_TC_TUNING_STATE_WAIT_SET_PHY_RSP,
    QOSM_TC_TUNING_STATE_WAIT_SET_MCS_RSP,
    QOSM_TC_TUNING_STATE_COMPLETE,
    QOSM_TC_TUNING_STATE_FAILED,
    QOSM_TC_TUNING_STATE_MAX,
} QOSM_TcTuningState_E;

typedef enum {
    QOSM_TC_TUNING_EVENT_START,
    QOSM_TC_TUNING_EVENT_CONN_UPDATE_PARAMS_RSP,
    QOSM_TC_TUNING_EVENT_SET_PHY_RSP,
    QOSM_TC_TUNING_EVENT_SET_MCS_RSP,
    QOSM_TC_TUNING_EVENT_TIMEOUT,
    QOSM_TC_TUNING_EVENT_MAX,
} QOSM_TcTuningEvent_E;

typedef struct {
    uint16_t interval;
} QOSM_SleConnParams_S;

typedef struct {
    uint8_t format;
    uint8_t phy;
    uint8_t pilotDensity;
} QOSM_SlePhyParams_S;

typedef struct {
    uint8_t mcs;
} QOSM_SleMcsParams_S;

typedef struct {
    QOSM_SleConnParams_S connParams;
    QOSM_SlePhyParams_S phyParams;
    QOSM_SleMcsParams_S mcsParams;
} QOSM_SleLogicLinkParams_S;

typedef struct {
    StateMachine stm;
    QOSM_TcTuningCtx_S ctx;
    int connUpdateTimerHandle;   // 连接参数更新定时器
    int setPhyTimerHandle;       // 设置PHY定时器
    int setMcsTimerHandle;       // 设置MCS定时器
    bool needReleaseChannel;
} QOSM_TcTuningStm_S;

static QOSM_TcTuningStm_S *g_tcTuningStm = NULL;  // 动态传输通道参数更新

static const char *g_tcTuningStateName[QOSM_TC_TUNING_STATE_MAX] = {
    [QOSM_TC_TUNING_STATE_START] = "TuningStartState",
    [QOSM_TC_TUNING_STATE_WAIT_CONN_UPDATE_PARAMS_RSP] = "TuningWaitConnUpdateParamsRspState",
    [QOSM_TC_TUNING_STATE_WAIT_SET_PHY_RSP] = "TuningWaitSetPhyRspState",
    [QOSM_TC_TUNING_STATE_WAIT_SET_MCS_RSP] = "TuningWaitSetMcsRspState",
    [QOSM_TC_TUNING_STATE_COMPLETE] = "TuningCompleteState",
    [QOSM_TC_TUNING_STATE_FAILED] = "TuningFailedState",
};

static QOSM_SleLogicLinkParams_S g_sleLogicLinkParams[QOSM_TRANS_CHANNEL_SLQI_MAX] = {
    {
        { QOSM_SLE_CONN_NORMAL_SPEED_INTERAL },
        { QOSM_SLE_RADIO_FRAME_TYPE_2, QOSM_SLE_PHY_TYPE_1M, QOSM_SLE_PILOT_DENSITY_16_TO_1 },
        { QOSM_SLE_NORMAL_MCS }
    },
    {
        { QOSM_SLE_CONN_HIGH_SPEED_INTERAL },
        { QOSM_SLE_RADIO_FRAME_TYPE_2, QOSM_SLE_PHY_TYPE_4M, QOSM_SLE_PILOT_DENSITY_NO_SET },
        { QOSM_SLE_HIGH_MCS }
    },
};

static QOSM_TcTuningCbks_S g_tcTuningCbks = { 0 };

static void QOSM_TcTuningCtxInit(QOSM_TcTuningCtx_S *ctx);
static void QOSM_TcTuningStmDtor(QOSM_TcTuningStm_S *stm);
static State *QOSM_TcTuningStartState(StateMachine *stm);
static State *QOSM_TcTuningWaitConnUpdateParamsRspState(StateMachine *stm);
static State *QOSM_TcTuningWaitSetPhyRspState(StateMachine *stm);
static State *QOSM_TcTuningWaitSetMcsRspState(StateMachine *stm);
static State *QOSM_TcTuningCompleteState(StateMachine *stm);
static State *QOSM_TcTuningFailedState(StateMachine *stm);
static bool QOSM_FilterRspAndCheckActive(uint16_t lcid, const char* stmStateName);
static QOSM_LogicLink_S *QOSM_TuningLogicLinkFind(uint16_t lcid);
static void QOSM_TuningDecreaseChannelSize(uint16_t lcid);

static void QOSM_TcTuningCtxInit(QOSM_TcTuningCtx_S *ctx)
{
    if (ctx != NULL) {
        (void)memset_s(ctx, sizeof(QOSM_TcTuningCtx_S), 0, sizeof(QOSM_TcTuningCtx_S));
    }
}

static void QOSM_TransChannelUpdateTimeout(void *arg)
{
    QOSM_TcTuningStm_S *tuningStm = (QOSM_TcTuningStm_S *)arg;
    if (tuningStm == NULL || tuningStm->stm.current_ == NULL) {
        return;
    }
    if (!QOSM_FilterRspAndCheckActive(tuningStm->ctx.lcid, tuningStm->stm.current_->name_)) {
        return;
    }
    STM_MFUNC(&tuningStm->stm, ProcessMessage, (Message) {.what = QOSM_TC_TUNING_EVENT_TIMEOUT});
}

static void QOSM_ConnUpdateTimeout(void *arg)
{
    QOSM_TcTuningStm_S *tuningStm = (QOSM_TcTuningStm_S *)arg;
    if (tuningStm == NULL) {
        return;
    }
    QOSM_LOGE("[STM]tuning conn update timeout, lcid: %hu", tuningStm->ctx.lcid);
    QOSM_TransChannelUpdateTimeout(arg);
}

static void QOSM_SetPhyTimeout(void *arg)
{
    QOSM_TcTuningStm_S *tuningStm = (QOSM_TcTuningStm_S *)arg;
    if (tuningStm == NULL) {
        return;
    }
    QOSM_LOGE("[STM]tuning set phy timeout, lcid: %hu", tuningStm->ctx.lcid);
    QOSM_TransChannelUpdateTimeout(arg);
}

static void QOSM_SetMcsTimeout(void *arg)
{
    QOSM_TcTuningStm_S *tuningStm = (QOSM_TcTuningStm_S *)arg;
    if (tuningStm == NULL) {
        return;
    }
    QOSM_LOGE("[STM]tuning set mcs timeout, lcid: %hu", tuningStm->ctx.lcid);
    QOSM_TransChannelUpdateTimeout(arg);
}

static void QOSM_TcStartConnUpdateTimer(StateMachine *stm)
{
    QOSM_TcTuningStm_S *tuningStm = (QOSM_TcTuningStm_S *)stm;
    SDF_TimerParam param = {
        .expires = QOSM_TUNING_CONN_UPDATE_TIMEOUT_MSEC,
        .period = false,
        .callback = QOSM_ConnUpdateTimeout,
        .args = stm,
    };
    (void)CP_TimerAdd(&tuningStm->connUpdateTimerHandle, &param);
    QOSM_LOGI("[STM]start conn update timer, timerId:%d", tuningStm->connUpdateTimerHandle);
}

static void QOSM_TcStopConnUpdateTimer(StateMachine *stm)
{
    QOSM_TcTuningStm_S *tuningStm = (QOSM_TcTuningStm_S *)stm;
    if (tuningStm->connUpdateTimerHandle != QOSM_TUNING_TIMER_NO_USED_HANDLE) {
        CP_TimerDel(tuningStm->connUpdateTimerHandle);
        QOSM_LOGI("[STM]stop conn update timer, timerId:%d", tuningStm->connUpdateTimerHandle);
        tuningStm->connUpdateTimerHandle = QOSM_TUNING_TIMER_NO_USED_HANDLE;
    }
}

static void QOSM_TcStartSetPhyTimer(StateMachine *stm)
{
    QOSM_TcTuningStm_S *tuningStm = (QOSM_TcTuningStm_S *)stm;
    SDF_TimerParam param = {
        .expires = QOSM_TUNING_SET_PHY_TIMEOUT_MSEC,
        .period = false,
        .callback = QOSM_SetPhyTimeout,
        .args = stm,
    };
    (void)CP_TimerAdd(&tuningStm->setPhyTimerHandle, &param);
    QOSM_LOGI("[STM]start set phy timer, timerId:%d", tuningStm->setPhyTimerHandle);
}

static void QOSM_TcStopSetPhyTimer(StateMachine *stm)
{
    QOSM_TcTuningStm_S *tuningStm = (QOSM_TcTuningStm_S *)stm;
    if (tuningStm->setPhyTimerHandle != QOSM_TUNING_TIMER_NO_USED_HANDLE) {
        CP_TimerDel(tuningStm->setPhyTimerHandle);
        QOSM_LOGI("[STM]stop set phy timer, timerId:%d", tuningStm->setPhyTimerHandle);
        tuningStm->setPhyTimerHandle = QOSM_TUNING_TIMER_NO_USED_HANDLE;
    }
}

static void QOSM_TcStartSetMcsTimer(StateMachine *stm)
{
    QOSM_TcTuningStm_S *tuningStm = (QOSM_TcTuningStm_S *)stm;
    SDF_TimerParam param = {
        .expires = QOSM_TUNING_SET_MCS_TIMEOUT_MSEC,
        .period = false,
        .callback = QOSM_SetMcsTimeout,
        .args = stm,
    };
    (void)CP_TimerAdd(&tuningStm->setMcsTimerHandle, &param);
    QOSM_LOGI("[STM]start set mcs timer, timerId:%d", tuningStm->setMcsTimerHandle);
}

static void QOSM_TcStopSetMcsTimer(StateMachine *stm)
{
    QOSM_TcTuningStm_S *tuningStm = (QOSM_TcTuningStm_S *)stm;
    if (tuningStm->setMcsTimerHandle != QOSM_TUNING_TIMER_NO_USED_HANDLE) {
        CP_TimerDel(tuningStm->setMcsTimerHandle);
        QOSM_LOGI("[STM]stop set mcs timer, timerId:%d", tuningStm->setMcsTimerHandle);
        tuningStm->setMcsTimerHandle = QOSM_TUNING_TIMER_NO_USED_HANDLE;
    }
}

static void QOSM_TcStopAllTimers(StateMachine *stm)
{
    QOSM_TcStopConnUpdateTimer(stm);
    QOSM_TcStopSetPhyTimer(stm);
    QOSM_TcStopSetMcsTimer(stm);
}

static void QOSM_TcTuningStmDtor(QOSM_TcTuningStm_S *stm)
{
    if (stm == NULL) {
        return;
    }
    QOSM_TcStopAllTimers((StateMachine *)stm);
    StateMachineSoftBaseDtor((StateMachine *)stm);
    SDF_MemFree(stm);
}

static bool QOSM_TcTuningStmInit(QOSM_TcTuningStm_S *stm)
{
    if (stm == NULL) {
        return false;
    }
    if (!StateMachineSoftBaseCtor((StateMachine *)stm)) {
        return false;
    }
    stm->connUpdateTimerHandle = QOSM_TUNING_TIMER_NO_USED_HANDLE;
    stm->setPhyTimerHandle = QOSM_TUNING_TIMER_NO_USED_HANDLE;
    stm->setMcsTimerHandle = QOSM_TUNING_TIMER_NO_USED_HANDLE;
    stm->needReleaseChannel = false;
    QOSM_TcTuningCtxInit(&stm->ctx);

    State *start = QOSM_TcTuningStartState((StateMachine *)stm);
    State *waitConn = QOSM_TcTuningWaitConnUpdateParamsRspState((StateMachine *)stm);
    State *waitPhy = QOSM_TcTuningWaitSetPhyRspState((StateMachine *)stm);
    State *waitMcs = QOSM_TcTuningWaitSetMcsRspState((StateMachine *)stm);
    State *complete = QOSM_TcTuningCompleteState((StateMachine *)stm);
    State *failed = QOSM_TcTuningFailedState((StateMachine *)stm);

    if (start == NULL || waitConn == NULL || waitPhy == NULL || waitMcs == NULL || complete == NULL || failed == NULL) {
        StateDtor(failed);
        StateDtor(complete);
        StateDtor(waitMcs);
        StateDtor(waitPhy);
        StateDtor(waitConn);
        StateDtor(start);
        StateMachineSoftBaseDtor((StateMachine *)stm);
        return false;
    }

    bool ret = true;
    ret = ret && STM_MFUNC(stm, EmplaceNewState, start);
    ret = ret && STM_MFUNC(stm, EmplaceNewState, waitConn);
    ret = ret && STM_MFUNC(stm, EmplaceNewState, waitPhy);
    ret = ret && STM_MFUNC(stm, EmplaceNewState, waitMcs);
    ret = ret && STM_MFUNC(stm, EmplaceNewState, complete);
    ret = ret && STM_MFUNC(stm, EmplaceNewState, failed);
    if (!ret) {
        StateDtor(failed);
        StateDtor(complete);
        StateDtor(waitMcs);
        StateDtor(waitPhy);
        StateDtor(waitConn);
        StateDtor(start);
        StateMachineSoftBaseDtor((StateMachine *)stm);
        return false;
    }

    STM_MFUNC(stm, Transition, g_tcTuningStateName[QOSM_TC_TUNING_STATE_START]);
    return true;
}

static QOSM_TcTuningStm_S *QOSM_TransChannelTuningStmCtor(void)
{
    QOSM_TcTuningStm_S *stm = (QOSM_TcTuningStm_S *)SDF_MemZalloc(
        sizeof(QOSM_TcTuningStm_S));
    if (stm == NULL) {
        return NULL;
    }
    if (!QOSM_TcTuningStmInit(stm)) {
        SDF_MemFree(stm);
        return NULL;
    }
    return stm;
}

static void QOSM_TcTuningStmFree(QOSM_TcTuningStm_S *stm)
{
    if (g_tcTuningStm != NULL && stm == g_tcTuningStm) {
        QOSM_TcTuningStmDtor(stm);
        g_tcTuningStm = NULL;
    }
}

static void QOSM_TcTuningCompleteReport(QOSM_TcTuningStm_S *stm)
{
    if (stm == NULL) {
        return;
    }
    if (stm->needReleaseChannel && stm->ctx.rspParams.tcid != CM_TRANS_INVALID_TCID) {
        QOSM_LOGW("[STM]tuning params timeout, need release trans channel, tcid: %hhu", stm->ctx.rspParams.tcid);
        CM_DynTransChannelReleaseParamReq_S releaseReq = {
            .version = 0,
            .localIndex = 0,
            .srcTcid = stm->ctx.rspParams.tcid,
            .dstTcid = 0,
        };
        (void)memcpy_s(&releaseReq.addr, sizeof(SLE_Addr_S), &stm->ctx.addr, sizeof(SLE_Addr_S));
        uint32_t ret = CM_DynTransChannelReleaseReq(&releaseReq);
        if (ret == CM_SUCCESS) {
            SleLogicLink_S *link = SleLogicLinkGetByAddr(&stm->ctx.addr);
            if (link != NULL) {
                QOSM_TuningDecreaseChannelSize(link->lcid);
            }
            QOSM_LOGI("[STM]release trans channel success, tcid: %hhu", stm->ctx.rspParams.tcid);
        } else {
            QOSM_LOGE("[STM]release trans channel failed, tcid: %hhu, ret: %08x", stm->ctx.rspParams.tcid, ret);
        }
    }
    QOSM_TransChannelRspParams_S *rsp = &stm->ctx.rspParams;
    if (g_tcTuningCbks.statusCbk != NULL) {
        QOSM_LOGI("[STM]tuning complete, call statusCbk, lcid: %hhu, tcid: %hhu, status: %hhu, slqi: %hhu",
            rsp->lcid, rsp->tcid, rsp->status, rsp->slqi);
        g_tcTuningCbks.statusCbk(rsp);
    }
}

static void QOSM_TcTuningIdleEntry(State *state)
{
    QOSM_LOGD("[STM]tuning enter start state");
}

static void QOSM_TcTuningIdleExit(State *state)
{
    (void)state;
    QOSM_LOGD("[STM]tuning exit start state");
}

static QOSM_LogicLink_S *QOSM_TuningLogicLinkFind(uint16_t lcid)
{
    return (g_tcTuningCbks.logicLinkFindCbk != NULL) ? g_tcTuningCbks.logicLinkFindCbk(lcid) : NULL;
}

static void QOSM_TuningDecreaseChannelSize(uint16_t lcid)
{
    if (g_tcTuningCbks.decreaseChannelSizeCbk != NULL) {
        g_tcTuningCbks.decreaseChannelSizeCbk(lcid);
    }
}

static void QOSM_TcTuningIdleDispatch(State *state, Message msg)
{
    QOSM_LOGD("[STM]tuning start recv event: %d", msg.what);
    switch (msg.what) {
        case QOSM_TC_TUNING_EVENT_START: {
            QOSM_TcTuningStm_S *stm = (QOSM_TcTuningStm_S *)state->stm_;
            QOSM_LogicLink_S *logicLink = QOSM_TuningLogicLinkFind(stm->ctx.lcid);
            if (logicLink == NULL) {
                QOSM_LOGE("[STM]logic link not found, lcid: %hu", stm->ctx.lcid);
                state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_FAILED]);
                break;
            }
            QOSM_SleLogicLinkParams_S *linkParams = &g_sleLogicLinkParams[stm->ctx.slqi];
            CM_ConnectUpdateParamReq_S connUpdateReq = {
                .version = 0,
                .localIndex = 0,
                .intervalMin = linkParams->connParams.interval,
                .intervalMax = linkParams->connParams.interval,
                .txRxInterval = QOSM_SLE_CONN_EVENT_IFS,
                .eventInterval = QOSM_SLE_CONN_EVENT_IFS,
                .maxLatency = 0,
                .supervisionTimeout = QOSM_SLE_CONN_SUPERVISION_TIMEOUT,
                .systemTimeUnit = QOSM_SLE_CONN_TIME_UNIT,
                .txRxFlag = 0,
            };
            (void)memcpy_s(&connUpdateReq.addr, sizeof(SLE_Addr_S), &logicLink->addr, sizeof(SLE_Addr_S));
            uint32_t ret = CM_ConnectUpdateParamReq(&connUpdateReq);
            if (ret != CM_SUCCESS) {
                QOSM_LOGE("[STM]tuning conn update params req failed, ret: %08x", ret);
                stm->ctx.rspParams.status = QOSM_TRANS_CHANNEL_ESTABLISH_FAIL;
                state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_FAILED]);
                break;
            }
            QOSM_LOGI("[STM]tuning conn update params req success, lcid: %hu", logicLink->lcid);
            state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_WAIT_CONN_UPDATE_PARAMS_RSP]);
            break;
        }
        default: {
            QOSM_LOGE("[STM]unexpected event %d in start state", msg.what);
            break;
        }
    }
}

static State *QOSM_TcTuningStartState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_tcTuningStateName[QOSM_TC_TUNING_STATE_START]);
    QOSM_CHECK_RETURN_NULL(state != NULL, "[STM]create start state failed");
    state->Entry = QOSM_TcTuningIdleEntry;
    state->Exit = QOSM_TcTuningIdleExit;
    state->Dispatch = QOSM_TcTuningIdleDispatch;
    return state;
}

static void QOSM_TcWaitConnUpdateParamsRspEntry(State *state)
{
    QOSM_LOGI("[STM]tuning enter wait conn update params rsp state");
    QOSM_TcStartConnUpdateTimer(state->stm_);
}

static void QOSM_TcWaitConnUpdateParamsRspExit(State *state)
{
    QOSM_LOGD("[STM]tuning exit wait conn update params rsp state");
    QOSM_TcStopConnUpdateTimer(state->stm_);
}

static void QOSM_TcTuningWaitConnUpdateParamsRspDispatch(State *state, Message msg)
{
    QOSM_LOGI("[STM]wait conn params update rsp recv event: %d", msg.what);
    switch (msg.what) {
        case QOSM_TC_TUNING_EVENT_CONN_UPDATE_PARAMS_RSP: {
            uint8_t result = (uint8_t)(uintptr_t)msg.extData;
            QOSM_TcTuningStm_S *stm = (QOSM_TcTuningStm_S *)state->stm_;
            if (result != CM_SUCCESS) {
                state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_FAILED]);
                break;
            }
            QOSM_LogicLink_S *logicLink = QOSM_TuningLogicLinkFind(stm->ctx.lcid);
            if (logicLink == NULL) {
                QOSM_LOGE("[STM]logic link not found, lcid:%hu", stm->ctx.lcid);
                state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_FAILED]);
                break;
            }
            QOSM_SleLogicLinkParams_S *linkParams = &g_sleLogicLinkParams[stm->ctx.slqi];
            CM_SetPhyReq_S setPhyReq = {
                .lcid = logicLink->lcid,
                .txFormat = linkParams->phyParams.format,
                .rxFormat = linkParams->phyParams.format,
                .txPhy = linkParams->phyParams.phy,
                .rxPhy = linkParams->phyParams.phy,
                .txPilotDensity = linkParams->phyParams.pilotDensity,
                .rxPilotDensity = linkParams->phyParams.pilotDensity,
                .gFeedback = 0,
                .tFeedback = 0,
            };
            uint32_t ret = CM_SetPhy(&setPhyReq);
            if (ret != CM_SUCCESS) {
                QOSM_LOGE("[STM]tuning set phy req failed, ret:%08x", ret);
                state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_FAILED]);
                break;
            }
            QOSM_LOGI("[STM]tuning set phy req success, lcid: %hu", logicLink->lcid);
            state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_WAIT_SET_PHY_RSP]);
            break;
        }
        case QOSM_TC_TUNING_EVENT_TIMEOUT: {
            QOSM_LOGE("[STM]tuning wait conn update params rsp timeout");
            state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_FAILED]);
            break;
        }
        default: {
            QOSM_LOGE("[STM]unexpected event %d in wait conn update params rsp state", msg.what);
            break;
        }
    }
}

static State *QOSM_TcTuningWaitConnUpdateParamsRspState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_tcTuningStateName[QOSM_TC_TUNING_STATE_WAIT_CONN_UPDATE_PARAMS_RSP]);
    QOSM_CHECK_RETURN_NULL(state != NULL, "[STM]create wait conn update params rsp state failed");
    state->Entry = QOSM_TcWaitConnUpdateParamsRspEntry;
    state->Exit = QOSM_TcWaitConnUpdateParamsRspExit;
    state->Dispatch = QOSM_TcTuningWaitConnUpdateParamsRspDispatch;
    return state;
}

static void QOSM_TcTuningWaitSetPhyRspEntry(State *state)
{
    QOSM_LOGD("[STM]enter wait set phy rsp state");
    QOSM_TcStartSetPhyTimer(state->stm_);
}

static void QOSM_TcTuningWaitSetPhyRspExit(State *state)
{
    QOSM_LOGD("[STM]exit wait set phy rsp state");
    QOSM_TcStopSetPhyTimer(state->stm_);
}

static void QOSM_TcTuningWaitSetPhyRspStateDispatch(State *state, Message msg)
{
    QOSM_LOGD("[STM]Wait set phy rsp recv event: %d", msg.what);
    switch (msg.what) {
        case QOSM_TC_TUNING_EVENT_SET_PHY_RSP: {
            uint8_t status = (uint8_t)(uintptr_t)msg.extData;
            QOSM_TcTuningStm_S *stm = (QOSM_TcTuningStm_S *)state->stm_;
            if (status != CM_SUCCESS) {
                QOSM_LOGE("[STM]tuning set phy rsp failed, status: %hhu", status);
                state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_FAILED]);
            } else {
                QOSM_LOGI("[STM]tuning set phy rsp success, lcid: %hu, slqi: %hhu", stm->ctx.lcid, stm->ctx.slqi);
                QOSM_SleLogicLinkParams_S *linkParams = &g_sleLogicLinkParams[stm->ctx.slqi];
                CM_SetMcsReq_S mcsReq = { .lcid = stm->ctx.lcid, .mcs = linkParams->mcsParams.mcs };
                uint32_t ret = CM_SetMcs(&mcsReq);
                if (ret != CM_SUCCESS) {
                    QOSM_LOGE("[STM]tuning set mcs req failed, ret: %08x", ret);
                    state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_FAILED]);
                } else {
                    QOSM_LOGI("[STM]tuning set mcs req success, lcid: %hu", stm->ctx.lcid);
                    state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_WAIT_SET_MCS_RSP]);
                }
            }
            break;
        }
        case QOSM_TC_TUNING_EVENT_TIMEOUT: {
            QOSM_LOGE("[STM]tuning wait set phy rsp timeout");
            state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_FAILED]);
            break;
        }
        default: {
            QOSM_LOGE("[STM]unexpected event %d in wait set phy rsp state", msg.what);
            break;
        }
    }
}

static State *QOSM_TcTuningWaitSetPhyRspState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_tcTuningStateName[QOSM_TC_TUNING_STATE_WAIT_SET_PHY_RSP]);
    QOSM_CHECK_RETURN_NULL(state != NULL, "[STM]create wait set phy rsp state failed");
    state->Entry = QOSM_TcTuningWaitSetPhyRspEntry;
    state->Exit = QOSM_TcTuningWaitSetPhyRspExit;
    state->Dispatch = QOSM_TcTuningWaitSetPhyRspStateDispatch;
    return state;
}

static void QOSM_TcTuningWaitSetMcsRspEntry(State *state)
{
    QOSM_LOGD("[STM]enter wait set mcs rsp state");
    QOSM_TcStartSetMcsTimer(state->stm_);
}

static void QOSM_TcTuningWaitSetMcsRspExit(State *state)
{
    QOSM_LOGD("[STM]exit wait set mcs rsp state");
    QOSM_TcStopSetMcsTimer(state->stm_);
}

static void QOSM_TcTuningWaitSetMcsRspStateDispatch(State *state, Message msg)
{
    QOSM_LOGD("[STM]Wait set mcs rsp recv event: %d", msg.what);
    switch (msg.what) {
        case QOSM_TC_TUNING_EVENT_SET_MCS_RSP: {
            uint8_t status = (uint8_t)(uintptr_t)msg.extData;
            QOSM_TcTuningStm_S *stm = (QOSM_TcTuningStm_S *)state->stm_;
            if (status != CM_SUCCESS) {
                QOSM_LOGE("[STM]tuning set mcs failed, status: %hhu", status);
                state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_FAILED]);
            } else {
                QOSM_LOGI("[STM]tuning set mcs success, lcid: %hu", stm->ctx.lcid);
                stm->ctx.rspParams.status = QOSM_TRANS_CHANNEL_ESTABLISHED;
                state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_COMPLETE]);
            }
            break;
        }
        case QOSM_TC_TUNING_EVENT_TIMEOUT: {
            QOSM_LOGE("[STM]wait set mcs rsp timeout");
            state->Transition(state, g_tcTuningStateName[QOSM_TC_TUNING_STATE_FAILED]);
            break;
        }
        default: {
            QOSM_LOGE("[STM]unexpected event %d in wait set mcs rsp state", msg.what);
            break;
        }
    }
}

static State *QOSM_TcTuningWaitSetMcsRspState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_tcTuningStateName[QOSM_TC_TUNING_STATE_WAIT_SET_MCS_RSP]);
    QOSM_CHECK_RETURN_NULL(state != NULL, "[STM]create wait set mcs rsp state failed");
    state->Entry = QOSM_TcTuningWaitSetMcsRspEntry;
    state->Exit = QOSM_TcTuningWaitSetMcsRspExit;
    state->Dispatch = QOSM_TcTuningWaitSetMcsRspStateDispatch;
    return state;
}

static void QOSM_TcTuningCompleteEntry(State *state)
{
    QOSM_LOGI("[STM]tuning enter complete state");
    QOSM_TcTuningStm_S *stm = (QOSM_TcTuningStm_S *)state->stm_;
    QOSM_TcStopAllTimers(state->stm_);
    stm->ctx.rspParams.status = QOSM_TRANS_CHANNEL_ESTABLISHED;
    // 执行成功后，需要将slqi值更新到链路信息里
    QOSM_LogicLink_S *logicLink = QOSM_TuningLogicLinkFind(stm->ctx.lcid);
    if (logicLink != NULL) {
        logicLink->slqi = stm->ctx.slqi;
    }
    QOSM_TcTuningCompleteReport(stm);
    QOSM_TcTuningStmFree(stm);
}

static void QOSM_TcTuningCompleteExit(State *state)
{
    (void)state;
    QOSM_LOGD("[STM]tuning exit complete state");
}

static void QOSM_TcTuningCompleteDispatch(State *state, Message msg)
{
    (void)state;
    (void)msg;
}

static State *QOSM_TcTuningCompleteState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_tcTuningStateName[QOSM_TC_TUNING_STATE_COMPLETE]);
    if (state == NULL) {
        return NULL;
    }
    state->Entry = QOSM_TcTuningCompleteEntry;
    state->Exit = QOSM_TcTuningCompleteExit;
    state->Dispatch = QOSM_TcTuningCompleteDispatch;
    return state;
}

static void QOSM_TcTuningFailedEntry(State *state)
{
    QOSM_LOGI("[STM]tuning enter failed state");
    QOSM_TcTuningStm_S *stm = (QOSM_TcTuningStm_S *)state->stm_;
    QOSM_TcStopAllTimers(state->stm_);
    stm->ctx.rspParams.status = QOSM_TRANS_CHANNEL_ESTABLISH_FAIL;
    if (stm->ctx.rspParams.tcid != CM_TRANS_INVALID_TCID) {
        stm->needReleaseChannel = true;
    }
    QOSM_TcTuningCompleteReport(stm);
    QOSM_TcTuningStmFree(stm);
}

static void QOSM_TcTuningFailedExit(State *state)
{
    (void)state;
    QOSM_LOGD("[STM]tuning exit failed state");
}

static void QOSM_TcTuningFailedDispatch(State *state, Message msg)
{
    (void)state;
    (void)msg;
}

static State *QOSM_TcTuningFailedState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_tcTuningStateName[QOSM_TC_TUNING_STATE_FAILED]);
    if (state == NULL) {
        return NULL;
    }
    state->Entry = QOSM_TcTuningFailedEntry;
    state->Exit = QOSM_TcTuningFailedExit;
    state->Dispatch = QOSM_TcTuningFailedDispatch;
    return state;
}

void QOSM_TcTuningLogicLinkConnUpdateParamCbk(CM_LogicLinkConnUpdateParam_S *param)
{
    if (param == NULL) {
        return;
    }
    if (!QOSM_FilterRspAndCheckActive(param->lcid,
        g_tcTuningStateName[QOSM_TC_TUNING_STATE_WAIT_CONN_UPDATE_PARAMS_RSP])) {
        return;
    }
    QOSM_LOGI("[STM]conn update param cbk, lcid: %hu, result: %hhu", param->lcid, param->result);
    STM_MFUNC(g_tcTuningStm, ProcessMessage, (Message) {
        .what = QOSM_TC_TUNING_EVENT_CONN_UPDATE_PARAMS_RSP,
        .extData = (void *)(uintptr_t)param->result
    });
}

void QOSM_TcTuningLogicLinkSetPhyCbk(CM_LogicLinkSetPhy_S *param)
{
    if (param == NULL) {
        return;
    }
    if (!QOSM_FilterRspAndCheckActive(param->lcid,
        g_tcTuningStateName[QOSM_TC_TUNING_STATE_WAIT_SET_PHY_RSP])) {
        return;
    }
    QOSM_LOGI("[STM]logic link set phy cbk, lcid: %hu, status: %hhu", param->lcid, param->status);
    STM_MFUNC(g_tcTuningStm, ProcessMessage, (Message) {
        .what = QOSM_TC_TUNING_EVENT_SET_PHY_RSP,
        .extData = (void *)(uintptr_t)param->status
    });
}

void QOSM_TcTuningLogicLinkSetMcsCbk(CM_LogicLinkSetMcs_S *param)
{
    if (param == NULL) {
        return;
    }
    // 由于set mcs cbk参数没有lcid，此处直接使用实际lcid校验通过即可
    if (!QOSM_FilterRspAndCheckActive(g_tcTuningStm->ctx.lcid,
        g_tcTuningStateName[QOSM_TC_TUNING_STATE_WAIT_SET_MCS_RSP])) {
        return;
    }
    QOSM_LOGI("[STM]logic link set mcs cbk, lcid: %hu, status: %hhu", g_tcTuningStm->ctx.lcid, param->status);
    STM_MFUNC(g_tcTuningStm, ProcessMessage, (Message) {
        .what = QOSM_TC_TUNING_EVENT_SET_MCS_RSP,
        .extData = (void *)(uintptr_t)param->status
    });
}

static bool QOSM_FilterRspAndCheckActive(uint16_t lcid, const char* stmStateName)
{
    if (g_tcTuningStm == NULL) {
        QOSM_LOGD("%s filtered, [STM]tuning stm is null, lcid:%hu", stmStateName, lcid);
        return false;
    }

    if (g_tcTuningStm->ctx.lcid != lcid) {
        QOSM_LOGD("[STM]%s filtered, lcid mismatch, expect:%hu, actual:%hu",
            stmStateName, g_tcTuningStm->ctx.lcid, lcid);
        return false;
    }
    StateMachine *stm = (StateMachine *)&g_tcTuningStm->stm;
    if (stm == NULL || stm->current_ == NULL || stm->current_->name_ == NULL) {
        QOSM_LOGD("[STM]%s filtered: stm state invalid, lcid: %hu", stmStateName, lcid);
        return false;
    }
    const char *stateName = stm->current_->name_;
    if (strcmp(stateName, stmStateName) != 0) {
        QOSM_LOGD("[STM]%s filtered: stm state not active, state: %s, lcid: %hu",
            stmStateName, stateName, lcid);
        return false;
    }
    return true;
}

uint32_t QOSM_TcTuningWithStm(const QOSM_TcTuningCtx_S *ctx)
{
    QOSM_CHECK_RETURN_RET(ctx != NULL, QOSM_NULL_PTR_ERR, "[STM]ctx is null");
    QOSM_LOGI("[STM]start to tuning logic link params with stm, lcid:%hu, slqi:%hhu.",
        ctx->lcid, ctx->slqi);
    if (g_tcTuningStm != NULL) {
        QOSM_LOGE("[STM]tuning stm is already running");
        return QOSM_POST_TASK_ERR;
    }

    g_tcTuningStm = QOSM_TransChannelTuningStmCtor();
    if (g_tcTuningStm == NULL) {
        QOSM_LOGE("[STM]create tuning stm failed");
        return QOSM_MALLOC_ERR;
    }

    g_tcTuningStm->ctx = *ctx;
    STM_MFUNC(g_tcTuningStm, ProcessMessage, (Message) {.what = QOSM_TC_TUNING_EVENT_START });
    return QOSM_SUCCESS;
}

void QOSM_TcTuningCbksRegister(const QOSM_TcTuningCbks_S *args)
{
    QOSM_CHECK_RETURN(args != NULL && args->statusCbk && args->logicLinkFindCbk && args->decreaseChannelSizeCbk,
        "[STM]args or args cbk is NULL.");
    g_tcTuningCbks = *(QOSM_TcTuningCbks_S *)args;
}

void QOSM_TcTuningCbksUnregister(void)
{
    (void)memset_s(&g_tcTuningCbks, sizeof(QOSM_TcTuningCbks_S), 0, sizeof(QOSM_TcTuningCbks_S));
}