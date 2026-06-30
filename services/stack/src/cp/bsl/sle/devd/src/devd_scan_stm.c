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

#include "securec.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "sdf_addr.h"
#include "nlstk_devd_api.h"
#include "cp_worker.h"
#include "devd_scan_type.h"
#include "devd_scan_stm.h"
#include "devd_scan_util.h"
#include "devd_scan_filter.h"
#include "nlstk_stm_collab_ext.h"

const char *g_scanStateName[NLSTK_DEVD_STATE_USER_DEF_END] = {
    [DEVD_STATE_STOPPED] = "ScanStoppedState",
    [DEVD_STATE_STARTING] = "ScanStartingState",
    [DEVD_STATE_STARTED] = "ScanStartedState",
    [DEVD_STATE_STOPPING] = "ScanStoppingState",
};

static NLSTK_DevdCollabScanFunc_S g_devCollabFunc = {};

static State *CreateStoppedState(StateMachine *stm);
static State *CreateStartingState(StateMachine *stm);
static State *CreateStartedState(StateMachine *stm);
static State *CreateStoppingState(StateMachine *stm);

NLSTK_Errcode_E DevdSetStmStateName(uint8_t devdScanState, const char *scanStateName)
{
    if (devdScanState < NLSTK_DEVD_STATE_USER_DEF_START || devdScanState >= NLSTK_DEVD_STATE_USER_DEF_END ||
        scanStateName == NULL) {
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    g_scanStateName[devdScanState] = scanStateName;
    return NLSTK_ERRCODE_SUCCESS;
}

const char *DevdGetStmStateName(uint8_t devdScanState)
{
    if (devdScanState < NLSTK_DEVD_STATE_STOPPED || devdScanState >= NLSTK_DEVD_STATE_USER_DEF_END) {
        return "";
    }
    return g_scanStateName[devdScanState];
}

NLSTK_Errcode_E DevdCollabRegFunc(const NLSTK_DevdCollabScanFunc_S *cbks)
{
    if (cbks == NULL) {
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    g_devCollabFunc = *cbks;
    return NLSTK_ERRCODE_SUCCESS;
}

void DevdCollabUnRegFunc(void)
{
    (void)memset_s(&g_devCollabFunc, sizeof(NLSTK_DevdCollabScanFunc_S), 0, sizeof(NLSTK_DevdCollabScanFunc_S));
}

const NLSTK_DevdCollabScanFunc_S *DevdGetCollabScanFunc(void)
{
    return &g_devCollabFunc;
}

static inline void ResetScanSetting(NLSTK_DevdScanSettingInner_S *setting)
{
    if (setting == NULL) {
        return;
    }
    (void)memset_s(setting, sizeof(NLSTK_DevdScanSettingInner_S), 0, sizeof(NLSTK_DevdScanSettingInner_S));
    setting->interval = UINT16_MAX;
}

static NLSTK_DevdScanManager_S *DevdScanManagerCtor(void)
{
    NLSTK_DevdScanManager_S *scanManager = (NLSTK_DevdScanManager_S *)SDF_MemZalloc(sizeof(NLSTK_DevdScanManager_S));
    NLSTK_CHECK_RETURN(scanManager != NULL, NULL, "[DEVDS] scanManager malloc fail");
    SDF_Traits traits = {
        .dtor = DevdFreeDevdScanner,
    };
    scanManager->scanners = SDF_CreateVector(traits);
    scanManager->internalTimer = NLSTK_DEVD_TIMER_NO_USED_HANDLE;
    scanManager->durationTimer = NLSTK_DEVD_TIMER_NO_USED_HANDLE;
    ResetScanSetting(&scanManager->currentScanSetting);
    ResetScanSetting(&scanManager->inflightScanSetting);
    ResetScanSetting(&scanManager->currentFrame4ScanSetting);
    ResetScanSetting(&scanManager->inflightFrame4ScanSetting);
    if (!scanManager->scanners) {
        NLSTK_LOG_ERROR("[DEVDS] scanManager->scanner malloc fail");
        SDF_MemFree(scanManager);
        return NULL;
    }
    return scanManager;
}

static void DevdScanManagerDtor(NLSTK_DevdScanManager_S *scanManager)
{
    if (!scanManager) {
        return;
    }
    if (scanManager->internalTimer != NLSTK_DEVD_TIMER_NO_USED_HANDLE) {
        CP_TimerDel(scanManager->internalTimer);
    }
    if (scanManager->durationTimer != NLSTK_DEVD_TIMER_NO_USED_HANDLE) {
        CP_TimerDel(scanManager->durationTimer);
    }
    SDF_DestroyVector(scanManager->scanners);
    SDF_MemFree(scanManager);
}

void DevdStateMachineDtor(NLSTK_DevdStateMachine_S *stm)
{
    if (g_devCollabFunc.collabScanStmDeInit != NULL) {
        g_devCollabFunc.collabScanStmDeInit();
    }
    if (stm != NULL) {
        StateMachineSoftBaseDtor((StateMachine *)stm);
        DevdScanManagerDtor(stm->scanManager);
        SDF_MemFree(stm);
    }
}

NLSTK_DevdStateMachine_S *DevdStateMachineCtor(void)
{
    NLSTK_DevdScanManager_S *scanManager = DevdScanManagerCtor();
    NLSTK_DevdStateMachine_S *stm = (NLSTK_DevdStateMachine_S *)SDF_MemZalloc(sizeof(NLSTK_DevdStateMachine_S));
    if (scanManager == NULL || stm == NULL || !StateMachineSoftBaseCtor((StateMachine *)stm)) {
        DevdScanManagerDtor(scanManager);
        DevdStateMachineDtor(stm);
        return NULL;
    }
    stm->scanManager = scanManager;

    if (g_devCollabFunc.collabScanStmInit != NULL &&
        g_devCollabFunc.collabScanStmInit((StateMachine *)stm) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_WARN("[COLLAB] collab stm scan not init complete");
    }

    State *stopped = CreateStoppedState((StateMachine *)stm);
    State *starting = CreateStartingState((StateMachine *)stm);
    State *started = CreateStartedState((StateMachine *)stm);
    State *stopping = CreateStoppingState((StateMachine *)stm);
    State *startCollab = g_devCollabFunc.createStartCollabState != NULL ?
        g_devCollabFunc.createStartCollabState((StateMachine *)stm) : NULL;
    State *stopCollab = g_devCollabFunc.createStopCollabState != NULL ?
        g_devCollabFunc.createStopCollabState((StateMachine *)stm) : NULL;

    if ((stopped == NULL || starting == NULL || started == NULL || stopping == NULL) ||
        ((startCollab != NULL && stopCollab == NULL) || (startCollab == NULL && stopCollab != NULL))) {
        StateDtor(stopCollab);
        StateDtor(startCollab);
        StateDtor(stopping);
        StateDtor(started);
        StateDtor(starting);
        StateDtor(stopped);
        DevdStateMachineDtor(stm);
        return NULL;
    }

    STM_MFUNC(stm, EmplaceNewState, stopped);
    STM_MFUNC(stm, EmplaceNewState, starting);
    STM_MFUNC(stm, EmplaceNewState, started);
    STM_MFUNC(stm, EmplaceNewState, stopping);

    if (startCollab != NULL) {
        STM_MFUNC(stm, EmplaceNewState, startCollab);
    }
    if (stopCollab != NULL) {
        STM_MFUNC(stm, EmplaceNewState, stopCollab);
    }

    STM_MFUNC(stm, Transition, g_scanStateName[DEVD_STATE_STOPPED]);
    return stm;
}

/*
 * +------------------+---------------+
 * | Scan State Machine | Stopped State |
 * +------------------+---------------+
 */

static void StoppedEntry(State *state)
{
    NLSTK_LOG_INFO("[DEVDS] state machine enter stopped state");
    if (g_devCollabFunc.notifyScanCollabResult != NULL) {
        g_devCollabFunc.notifyScanCollabResult(state, true);
    }
    // 进入Stopped状态，需要清理inflightSetting
    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(state->stm_, scanManager);
    ResetScanSetting(&manager->inflightScanSetting);
    ResetScanSetting(&manager->inflightFrame4ScanSetting);
    // 进入Stopped状态，需要清理currentScanSetting
    ResetScanSetting(&manager->currentScanSetting);
    ResetScanSetting(&manager->currentFrame4ScanSetting);
}

static void StoppedExit(State *state)
{
    SDF_UNUSED(state);
    NLSTK_LOG_DEBUG("[DEVDS] state machine exit stopped state");
}

static bool ScanSettingsEqual(NLSTK_DevdScanSettingInner_S *left, NLSTK_DevdScanSettingInner_S *right)
{
    return (left->window == right->window && left->interval == right->interval);
}

static void StoppedDispatch(State *state, Message msg)
{
    NLSTK_LOG_INFO("[DEVDS] recv event %d in %s", msg.what, g_scanStateName[DEVD_STATE_STOPPED]);
    switch (msg.what) {
        case DEVD_POTIENTIAL_UPDATE: {
            NLSTK_DevdScanManager_S *manager = DEVD_STM_M(state->stm_, scanManager);
            NLSTK_DevdScanSettingInner_S *targetFrame1Ptr = FetchFrame1TargetScanSettings(manager->scanners);
            NLSTK_DevdScanSettingInner_S *targetFrame4Ptr = FetchFrame4TargetScanSettings(manager->scanners);
            NLSTK_LOG_INFO("[DEVDS] SleScanStoppedState POTIENTIAL_UPDATE to SLE_SCAN_STARTING enter, "
                "frame 1 current window %hu interval %hu, target window %hu interval %hu, "
                "frame 4 current window %hu interval %hu, target window %hu interval %hu",
                manager->currentScanSetting.window, manager->currentScanSetting.interval,
                targetFrame1Ptr ? targetFrame1Ptr->window : 0, targetFrame1Ptr ? targetFrame1Ptr->interval : UINT16_MAX,
                manager->currentFrame4ScanSetting.window,
                manager->currentFrame4ScanSetting.interval, targetFrame4Ptr ? targetFrame4Ptr->window : 0,
                targetFrame4Ptr ? targetFrame4Ptr->interval : UINT16_MAX);
            if ((targetFrame1Ptr == NULL || targetFrame1Ptr->window == 0) &&
                (targetFrame4Ptr == NULL || targetFrame4Ptr->window == 0)) {
                break;
            }
            NLSTK_DevdScanSettingInner_S targetFrame1Settings = { .interval = UINT16_MAX };
            NLSTK_DevdScanSettingInner_S targetFrame4Settings = { .interval = UINT16_MAX };
            if (targetFrame1Ptr != NULL && targetFrame1Ptr->window != 0) {
                (void)memcpy_s(&targetFrame1Settings, sizeof(NLSTK_DevdScanSettingInner_S), targetFrame1Ptr,
                    sizeof(NLSTK_DevdScanSettingInner_S));
            }
            if (targetFrame4Ptr != NULL && targetFrame4Ptr->window != 0) {
                (void)memcpy_s(&targetFrame4Settings, sizeof(NLSTK_DevdScanSettingInner_S), targetFrame4Ptr,
                    sizeof(NLSTK_DevdScanSettingInner_S));
            }
            if (g_devCollabFunc.isNeedStartScanCollabReqAndTransState != NULL &&
                g_devCollabFunc.isNeedStartScanCollabReqAndTransState(state)) {
                break;
            }
            if (!ScanSettingsEqual(&targetFrame1Settings, &manager->currentScanSetting)) {
                (void)memcpy_s(&manager->inflightScanSetting, sizeof(NLSTK_DevdScanSettingInner_S),
                    &targetFrame1Settings, sizeof(NLSTK_DevdScanSettingInner_S));
            }
            if (!ScanSettingsEqual(&targetFrame4Settings, &manager->currentFrame4ScanSetting)) {
                (void)memcpy_s(&manager->inflightFrame4ScanSetting, sizeof(NLSTK_DevdScanSettingInner_S),
                    &targetFrame4Settings, sizeof(NLSTK_DevdScanSettingInner_S));
            }
            state->Transition(state, g_scanStateName[DEVD_STATE_STARTING]);
            break;
        }
        default:
            break;
    }
}

State *CreateStoppedState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_scanStateName[DEVD_STATE_STOPPED]);
    NLSTK_CHECK_RETURN(state != NULL, NULL, "[DEVDS] create stopped state failed");
    state->Entry = StoppedEntry;
    state->Exit = StoppedExit;
    state->Dispatch = StoppedDispatch;
    return state;
}

/*
 * +------------------+---------------+
 * | Scan State Machine | Starting State |
 * +------------------+---------------+
 */

static void InternalTimeout(void *arg)
{
    StateMachine *stm = (StateMachine *)arg;
    STM_MFUNC(stm, ProcessMessage, (Message) {.what = DEVD_INTERNAL_TIMEOUT});
}

static bool StartScanTimer(State *state, time_t expires)
{
    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(state->stm_, scanManager);
    NLSTK_CHECK_RETURN(manager->internalTimer == NLSTK_DEVD_TIMER_NO_USED_HANDLE, false, "[DEVDS] timer is in use");
    SDF_TimerParam param = {
        .expires = expires,
        .period = false,
        .callback = InternalTimeout,
        .args = state->stm_,
    };
    uint32_t ret = CP_TimerAdd(&manager->internalTimer, &param);
    if (ret != SDF_OK) {
        NLSTK_LOG_ERROR("create timer failed, ret: 0x%08x", ret);
        return false;
    }
    NLSTK_LOG_INFO("timer add, handle: %d", manager->internalTimer);
    return true;
}

static void StopScanTimer(State *state)
{
    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(state->stm_, scanManager);
    if (manager->internalTimer != NLSTK_DEVD_TIMER_NO_USED_HANDLE) {
        CP_TimerDel(manager->internalTimer);
        NLSTK_LOG_INFO("timer delete, handle: %d", manager->internalTimer);
        manager->internalTimer = NLSTK_DEVD_TIMER_NO_USED_HANDLE;
    }
}

static NLSTK_DevdSleScanParams_S *BuildScanParams(NLSTK_DevdScanManager_S *manager)
{
    uint8_t phyCnt = (manager->inflightScanSetting.window != 0) + (manager->inflightFrame4ScanSetting.window != 0);
    uint8_t frameType = 0;
    if (phyCnt == DEVD_SCAN_PHY_NUM_2) {
        frameType = DEVD_SCAN_FRAME_TYPE_1 | DEVD_SCAN_FRAME_TYPE_4;
    } else if (manager->inflightScanSetting.window != 0) {
        frameType = DEVD_SCAN_FRAME_TYPE_1;
    } else if (manager->inflightFrame4ScanSetting.window != 0) {
        frameType = DEVD_SCAN_FRAME_TYPE_4;
    }
    NLSTK_DevdSleScanParams_S *scanParams = (NLSTK_DevdSleScanParams_S *)SDF_MemZalloc(
        sizeof(NLSTK_DevdSleScanParams_S) + phyCnt * sizeof(NLSTK_DevdSleScanParamsNoPhy_S));
    NLSTK_CHECK_RETURN(scanParams != NULL, NULL, "[DEVDS] scanParams malloc fail");
    scanParams->localAddrType = PUBLIC_ADDRESS;
    scanParams->scanFilterPolicy = SCAN_FLT_BASIC_NONE;
    scanParams->frameType = frameType;
    uint8_t index = 0;
    if (manager->inflightScanSetting.window != 0) {
        scanParams->params[index].scanType = SCAN_TYPE_ACTIVE;
        scanParams->params[index].scanInterval = manager->inflightScanSetting.interval;
        scanParams->params[index].scanWindow = manager->inflightScanSetting.window;
        index++;
    }
    if (manager->inflightFrame4ScanSetting.window != 0) {
        scanParams->params[index].scanType = SCAN_TYPE_PASSIVE;
        scanParams->params[index].scanInterval = manager->inflightFrame4ScanSetting.interval;
        scanParams->params[index].scanWindow = manager->inflightFrame4ScanSetting.window;
    }
    return scanParams;
}

static void StartingEntry(State *state)
{
    NLSTK_LOG_INFO("[DEVDS] state machine enter starting state");
    if (!StartScanTimer(state, DEVD_BUSY_TIMEOUT_MSEC)) {
        NLSTK_LOG_ERROR("[DEVDS] start scan timer failed");
        // 原则上这里不会有异常，即使起定时器失败也不做异常处理
    }
    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(state->stm_, scanManager);
    NLSTK_DevdSleScanParams_S *scanParams = BuildScanParams(manager);
    if (scanParams == NULL) {
        for (uint8_t i = 0; i < DEVD_MAX_MODULE_ID; i++) {
            if (manager->scanModule[i].used && manager->scanModule[i].scanCbk.onStartOrStopEvent) {
                manager->scanModule[i].scanCbk.onStartOrStopEvent(NLSTK_ERRCODE_FAIL, true);
            }
        }
        state->Transition(state, g_scanStateName[DEVD_STATE_STOPPED]);
        return;
    }
    NLSTK_LOG_INFO("[DEVDS] start scan... frame type 1 interval %hu window %hu, frame type 4 interval %hu window %hu",
                   manager->inflightScanSetting.interval, manager->inflightScanSetting.window,
                   manager->inflightFrame4ScanSetting.interval, manager->inflightFrame4ScanSetting.window);
    if (manager->inflightScanSetting.window == 0 && manager->inflightFrame4ScanSetting.window == 0) {
        NLSTK_LOG_ERROR("[DEVDS] settings window is all empty!!!");
        SDF_MemFree(scanParams);
        // 进入启动扫描超时流程
        return;
    }
    NLSTK_Errcode_E ret = NLSTK_DevdSleStartScan(scanParams);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DEVDS] start scan fail");
        for (uint8_t i = 0; i < DEVD_MAX_MODULE_ID; i++) {
            if (manager->scanModule[i].used && manager->scanModule[i].scanCbk.onStartOrStopEvent) {
                manager->scanModule[i].scanCbk.onStartOrStopEvent(NLSTK_ERRCODE_FAIL, true);
            }
        }
        state->Transition(state, g_scanStateName[DEVD_STATE_STOPPED]);
    }
    SDF_MemFree(scanParams);
}

static void StartingExit(State *state)
{
    NLSTK_LOG_DEBUG("[DEVDS] state machine exit starting state");
    StopScanTimer(state);
}

static void StartingDispatch(State *state, Message msg)
{
    SDF_UNUSED(state);
    NLSTK_LOG_INFO("[DEVDS] recv event %d in %s", msg.what, g_scanStateName[DEVD_STATE_STARTING]);
    switch (msg.what) {
        case DEVD_START_OK: {
            NLSTK_DevdScanManager_S *manager = DEVD_STM_M(state->stm_, scanManager);
            (void)memcpy_s(&manager->currentScanSetting, sizeof(NLSTK_DevdScanSettingInner_S),
                &manager->inflightScanSetting, sizeof(NLSTK_DevdScanSettingInner_S));
            (void)memcpy_s(&manager->currentFrame4ScanSetting, sizeof(NLSTK_DevdScanSettingInner_S),
                           &manager->inflightFrame4ScanSetting, sizeof(NLSTK_DevdScanSettingInner_S));
            for (uint8_t i = 0; i < DEVD_MAX_MODULE_ID; i++) {
                if (manager->scanModule[i].used && manager->scanModule[i].scanCbk.onStartOrStopEvent) {
                    manager->scanModule[i].scanCbk.onStartOrStopEvent(NLSTK_ERRCODE_SUCCESS, true);
                }
            }
            state->Transition(state, g_scanStateName[DEVD_STATE_STARTED]);
            STM_MFUNC(state->stm_, ProcessMessage, (Message) { .what = DEVD_POTIENTIAL_UPDATE });
            break;
        }
        case DEVD_START_ERR:
        case DEVD_INTERNAL_TIMEOUT: {
            NLSTK_LOG_ERROR("[DEVDS] start error");
            NLSTK_DevdScanManager_S *manager = DEVD_STM_M(state->stm_, scanManager);
            NLSTK_Errcode_E ret = msg.what == DEVD_START_ERR ? NLSTK_ERRCODE_FAIL : NLSTK_ERRCODE_TIMEOUT;
            for (uint8_t i = 0; i < DEVD_MAX_MODULE_ID; i++) {
                if (manager->scanModule[i].used && manager->scanModule[i].scanCbk.onStartOrStopEvent) {
                    manager->scanModule[i].scanCbk.onStartOrStopEvent(ret, true);
                }
            }
            state->Transition(state, g_scanStateName[DEVD_STATE_STOPPED]);
            break;
        }
        default:
            break;
    }
}

State *CreateStartingState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_scanStateName[DEVD_STATE_STARTING]);
    NLSTK_CHECK_RETURN(state != NULL, NULL, "[DEVDS] create starting state failed");
    state->Entry = StartingEntry;
    state->Exit = StartingExit;
    state->Dispatch = StartingDispatch;
    return state;
}

/*
 * +------------------+---------------+
 * | Scan State Machine | Started State |
 * +------------------+---------------+
 */

static void StartedEntry(State *state)
{
    SDF_UNUSED(state);
    NLSTK_LOG_INFO("[DEVDS] state machine enter started state");
    if (g_devCollabFunc.notifyScanCollabResult != NULL) {
        g_devCollabFunc.notifyScanCollabResult(state, false);
    }
}

static void StartedExit(State *state)
{
    SDF_UNUSED(state);
    NLSTK_LOG_DEBUG("[DEVDS] state machine exit started state");
}

static bool IsFrame1CurrentSameAsTargetSettings(const NLSTK_DevdScanManager_S *manager,
    const NLSTK_DevdScanSettingInner_S *target)
{
    return (target->window == manager->currentScanSetting.window &&
            target->interval == manager->currentScanSetting.interval);
}

static bool IsFrame4CurrentSameAsTargetSettings(const NLSTK_DevdScanManager_S *manager,
    const NLSTK_DevdScanSettingInner_S *target)
{
    return (target->window == manager->currentFrame4ScanSetting.window &&
            target->interval == manager->currentFrame4ScanSetting.interval);
}

static void StartedDispatch(State *state, Message msg)
{
    SDF_UNUSED(state);
    NLSTK_LOG_INFO("[DEVDS] recv event %d in %s", msg.what, g_scanStateName[DEVD_STATE_STARTED]);
    switch (msg.what) {
        case DEVD_POTIENTIAL_UPDATE: {
            NLSTK_DevdScanManager_S *manager = DEVD_STM_M(state->stm_, scanManager);
            NLSTK_DevdScanSettingInner_S *target = FetchFrame1TargetScanSettings(manager->scanners);
            NLSTK_DevdScanSettingInner_S *targetFrame4 = FetchFrame4TargetScanSettings(manager->scanners);
            // 当前已经没有扫描应用，停止扫描
            if (target == NULL || targetFrame4 == NULL) {
                NLSTK_LOG_INFO("[DEVDS] there is no scanner, stop scan");
                state->Transition(state, g_scanStateName[DEVD_STATE_STOPPING]);
                break;
            }
            NLSTK_LOG_INFO("[DEVDS] started scan frame 1 current window %hu interval %hu, target window %hu "
                "interval %hu; frame 4 current window %hu interval %hu, target window %hu interval %hu",
                manager->currentScanSetting.window, manager->currentScanSetting.interval, target->window,
                target->interval, manager->currentFrame4ScanSetting.window,
                manager->currentFrame4ScanSetting.interval, targetFrame4->window, targetFrame4->interval);
            // 扫描应用的帧1/帧4的窗口都为0，不能只停止扫描，需要考虑参数变更，需要重启扫描
            // 合并出的最优扫描参数：扫描窗口和间隔与当前一致，不做处理
            if (IsFrame1CurrentSameAsTargetSettings(manager, target) &&
                IsFrame4CurrentSameAsTargetSettings(manager, targetFrame4)) {
                // 当前处于启动状态时，若最新AP侧和协同侧计算结果参数没有发生变化时，
                // 1. 协同侧参数可能处于启动扫描状态或者停止扫描状态，
                // 2. AP侧参数已重新变化，需要查看协同侧是否已经停止扫描
                if (g_devCollabFunc.isNeedTryStopScanCollab == NULL ||
                    !g_devCollabFunc.isNeedTryStopScanCollab(state)) {
                    break;
                }
            }
            if (g_devCollabFunc.isNeedStopScanCollabReqAndTransState != NULL &&
                g_devCollabFunc.isNeedStopScanCollabReqAndTransState(state)) {
                break;
            }
            if (!IsFrame1CurrentSameAsTargetSettings(manager, target)) {
                ResetScanSetting(&manager->inflightScanSetting);
            }
            if (!IsFrame4CurrentSameAsTargetSettings(manager, targetFrame4)) {
                ResetScanSetting(&manager->inflightFrame4ScanSetting);
            }
            // 合并出的最优扫描参数发送了变化，停止扫描，后续由状态机触发重新开启扫描
            state->Transition(state, g_scanStateName[DEVD_STATE_STOPPING]);
            break;
        }
        default:
            break;
    }
}

State *CreateStartedState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_scanStateName[DEVD_STATE_STARTED]);
    NLSTK_CHECK_RETURN(state != NULL, NULL, "[DEVDS] create started state failed");
    state->Entry = StartedEntry;
    state->Exit = StartedExit;
    state->Dispatch = StartedDispatch;
    return state;
}

/*
 * +------------------+---------------+
 * | Scan State Machine | Stopping State |
 * +------------------+---------------+
 */

static void StoppingEntry(State *state)
{
    SDF_UNUSED(state);
    NLSTK_LOG_INFO("[DEVDS] state machine enter stopping state");
    if (!StartScanTimer(state, DEVD_BUSY_TIMEOUT_MSEC)) {
        NLSTK_LOG_ERROR("[DEVDS] start scan timer failed");
        // 原则上这里不会有异常，即使起定时器失败也不做异常处理
    }
    NLSTK_DevdSleScanEnable_S scanEnable = {0};
    if (NLSTK_DevdSleEnableScan(&scanEnable) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DEVDS] stop scan fail");
        state->Transition(state, g_scanStateName[DEVD_STATE_STARTED]);
    }
}

static void StoppingExit(State *state)
{
    SDF_UNUSED(state);
    NLSTK_LOG_DEBUG("[DEVDS] state machine exit stopping state");
    StopScanTimer(state);
}

static void StoppingDispatch(State *state, Message msg)
{
    SDF_UNUSED(state);
    NLSTK_LOG_INFO("[DEVDS] recv event %d in %s", msg.what, g_scanStateName[DEVD_STATE_STOPPING]);
    switch (msg.what) {
        case DEVD_STOP_OK: {
            NLSTK_DevdScanManager_S *manager = DEVD_STM_M(state->stm_, scanManager);
            for (uint8_t i = 0; i < DEVD_MAX_MODULE_ID; i++) {
                if (manager->scanModule[i].used && manager->scanModule[i].scanCbk.onStartOrStopEvent) {
                    manager->scanModule[i].scanCbk.onStartOrStopEvent(NLSTK_ERRCODE_SUCCESS, false);
                }
            }
            state->Transition(state, g_scanStateName[DEVD_STATE_STOPPED]);
            STM_MFUNC(state->stm_, ProcessMessage, (Message) { .what = DEVD_POTIENTIAL_UPDATE });
            break;
        }
        case DEVD_STOP_ERR: {
            NLSTK_LOG_ERROR("[DEVDS] stop error");
            NLSTK_DevdScanManager_S *manager = DEVD_STM_M(state->stm_, scanManager);
            for (uint8_t i = 0; i < DEVD_MAX_MODULE_ID; i++) {
                if (manager->scanModule[i].used && manager->scanModule[i].scanCbk.onStartOrStopEvent) {
                    manager->scanModule[i].scanCbk.onStartOrStopEvent(NLSTK_ERRCODE_FAIL, false);
                }
            }
            state->Transition(state, g_scanStateName[DEVD_STATE_STARTED]);
            break;
        }
        case DEVD_INTERNAL_TIMEOUT: {
            NLSTK_LOG_ERROR("[DEVDS] stop internal timeout");
            NLSTK_DevdScanManager_S *manager = DEVD_STM_M(state->stm_, scanManager);
            for (uint8_t i = 0; i < DEVD_MAX_MODULE_ID; i++) {
                if (manager->scanModule[i].used && manager->scanModule[i].scanCbk.onStartOrStopEvent) {
                    manager->scanModule[i].scanCbk.onStartOrStopEvent(NLSTK_ERRCODE_TIMEOUT, false);
                }
            }
            // 可能因为芯片已经下电后，未清理状态机，继续下发停止扫描指令，导致未收到指令回复超时
            // 此时需要尝试重新刷新扫描状态机，避免扫描参数未更新
            state->Transition(state, g_scanStateName[DEVD_STATE_STOPPED]);
            STM_MFUNC(state->stm_, ProcessMessage, (Message) { .what = DEVD_POTIENTIAL_UPDATE });
        } break;
        default:
            break;
    }
}

State *CreateStoppingState(StateMachine *stm)
{
    State *state = StateCtor(stm, g_scanStateName[DEVD_STATE_STOPPING]);
    NLSTK_CHECK_RETURN(state != NULL, NULL, "[DEVDS] create stopping state failed");
    state->Entry = StoppingEntry;
    state->Exit = StoppingExit;
    state->Dispatch = StoppingDispatch;
    return state;
}