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
#include <stdbool.h>
#include "nlstk_log.h"
#include "securec.h"
#include "sdf_mem.h"
#include "nlstk_cfgdb.h"
#include "hadm_link_manager.h"
#include "hadm_parser_iq.h"
#include "hadm_config_dli.h"
#include "hadm_config_cm.h"
#include "hadm_sm.h"

#define HADM_SOUNDING_ENABLE 0   // 开启HADM测量
#define HADM_SOUNDING_DISABLE 1  // 关闭HADM测量
#define HADM_SOUNDING_PAUSE 2    // 暂停HADM测量

// 下面的宏定义值的来源，均为产品进行定义，如果后续要变更，和终端侧对齐
#define HDAM_SOUNDING_TS_MAX_DIFF 10  // HADM双端IQ测量值时间戳最大差值
#define HADM_CAL_OFFSET_DEFAULT 300   // 单位cm
#define HADM_CAL_OFFSET_MIN 1
#define HADM_CAL_OFFSET_MAX 900
#define HADM_CAL_TOF_OFFSET_DEFAULT 1385  // 单位m
#define HADM_CAL_TOF_OFFSET_MAX 3000      // 单位m

typedef void (*HadmSoundingStateMachine)(SLE_Addr_S *addr, void *param);

HadmSoundingCbk_S g_hadmReportCbk = { 0 };
SLE_Addr_S g_soundingAddrs = { 0 };

#define HADM_REPORT_CONTROL_RESULT(addr, op, result)                                               \
    do {                                                                                           \
        if (g_hadmReportCbk.controlResultCbk != NULL) {                                            \
            NLSTK_LOG_INFO("[HADM]report the op: %d, result: %d to service, adds is %s", op, result, \
                         GET_ENC_ADDR(addr));                                                      \
            g_hadmReportCbk.controlResultCbk(addr, op, result);                                    \
        }                                                                                          \
    } while (0)

#define HADM_REPORT_IQ_RESULT(addr, iqData)                                                  \
    do {                                                                                     \
        if (g_hadmReportCbk.reportIqDataCbk != NULL) {                                       \
            NLSTK_LOG_INFO("[HADM]report Iq data to service, adds is %s", GET_ENC_ADDR(addr)); \
            g_hadmReportCbk.reportIqDataCbk(addr, iqData);                                   \
        }                                                                                    \
    } while (0)

static void HadmProcUserStartBeforeSoundingReady(SLE_Addr_S *addr, void *param);
static void HadmProcUserStopBeforeSoundingEnable(SLE_Addr_S *addr, void *param);
static void HadmSoundingStateMachineDefaultException(SLE_Addr_S *addr, void *param);
static void HadmProcCmLinkDownEventBeforeSoundingReady(SLE_Addr_S *addr, void *param);
static void HadmProcFeatruesEventInIdle(SLE_Addr_S *addr, void *param);
static void HadmProcUserStartWhenSoundingReady(SLE_Addr_S *addr, void *param);
static void HadmProcUpdateConnParamEvent(SLE_Addr_S *addr, void *param);
static void HadmProcDuplicateUserHandle(SLE_Addr_S *addr, void *param);
static void HadmProcUserStopWhileStarting(SLE_Addr_S *addr, void *param);
static void HadmProcCmLinkDownEventWhileStartSounding(SLE_Addr_S *addr, void *param);
static void HadmProcRemoteCsEventInReadRemoteCs(SLE_Addr_S *addr, void *param);
static void HadmProcConfigResultEventInConfigSounding(SLE_Addr_S *addr, void *param);
static void HadmProcEnableResultEventInEnableSounding(SLE_Addr_S *addr, void *param);
static void HadmProcUserStopInSoundingState(SLE_Addr_S *addr, void *param);
static void HadmProcCmLinkDownEventWhileStopSounding(SLE_Addr_S *addr, void *param);
static void HadmProcEnableResultEventInDisableSounding(SLE_Addr_S *addr, void *param);

HadmSoundingStateMachine g_hadmSoundingStateMachine[HADM_SOUNDING_STATE_INVALID][HADM_MAX_EVENT_TYPE] = {
    [HADM_SOUNDING_STATE_IDLE] = {
        [USER_START_SOUNDING_EVENT] = HadmProcUserStartBeforeSoundingReady,
        [USER_STOP_SOUNDING_EVENT] = HadmProcUserStopBeforeSoundingEnable,
        [CM_REPORT_LINK_STATE_CONNECTED] = HadmSoundingStateMachineDefaultException,        // 不存在此场景，默认异常处理
        [CM_REPORT_LINK_STATE_DISCONNECTED] = HadmProcCmLinkDownEventBeforeSoundingReady,
        [CM_REPORT_FEATURES_EVENT] = HadmProcFeatruesEventInIdle,
        [CM_REPORT_UPDATE_CONN_PARAM_EVENT] = HadmSoundingStateMachineDefaultException,     // 不存在此场景，默认异常处理
        [DLI_REPORT_REMOTE_MEASURE_EVENT] = HadmProcRemoteCsEventInReadRemoteCs,
        [DLI_REPORT_CONFIG_RESULT_EVENT] = HadmSoundingStateMachineDefaultException,        // 不存在此场景，默认异常处理
        [DLI_REPORT_SOUNDING_RESULT_EVENT] = HadmSoundingStateMachineDefaultException,      // 不存在此场景，默认异常处理
    },

    [HADM_SOUNDING_STATE_SOUNDING_READY] = {
        [USER_START_SOUNDING_EVENT] = HadmProcUserStartWhenSoundingReady,
        [USER_STOP_SOUNDING_EVENT] = HadmProcUserStopBeforeSoundingEnable,
        [CM_REPORT_LINK_STATE_CONNECTED] = HadmSoundingStateMachineDefaultException,
        [CM_REPORT_LINK_STATE_DISCONNECTED] = HadmProcCmLinkDownEventBeforeSoundingReady,
        [CM_REPORT_FEATURES_EVENT] = HadmSoundingStateMachineDefaultException,              // 不存在此场景，默认异常处理
        [CM_REPORT_UPDATE_CONN_PARAM_EVENT] = HadmSoundingStateMachineDefaultException,     // 不存在此场景，默认异常处理
        [DLI_REPORT_REMOTE_MEASURE_EVENT] = HadmSoundingStateMachineDefaultException,            // 不存在此场景，默认异常处理
        [DLI_REPORT_CONFIG_RESULT_EVENT] = HadmSoundingStateMachineDefaultException,        // 不存在此场景，默认异常处理
        [DLI_REPORT_SOUNDING_RESULT_EVENT] = HadmSoundingStateMachineDefaultException,      // 不存在此场景，默认异常处理
    },

    [HADM_SOUNDING_STATE_CONFIG_CONNECTION] = {
        [USER_START_SOUNDING_EVENT] = HadmProcDuplicateUserHandle,
        [USER_STOP_SOUNDING_EVENT] = HadmProcUserStopWhileStarting,
        [CM_REPORT_LINK_STATE_CONNECTED] = HadmSoundingStateMachineDefaultException,        // 不存在此场景，默认异常处理
        [CM_REPORT_LINK_STATE_DISCONNECTED] = HadmProcCmLinkDownEventWhileStartSounding,
        [CM_REPORT_FEATURES_EVENT] = HadmSoundingStateMachineDefaultException,              // 不存在此场景，默认异常处理
        [CM_REPORT_UPDATE_CONN_PARAM_EVENT] = HadmProcUpdateConnParamEvent,
        [DLI_REPORT_REMOTE_MEASURE_EVENT] = HadmSoundingStateMachineDefaultException,            // 不存在此场景，默认异常处理
        [DLI_REPORT_CONFIG_RESULT_EVENT] = HadmSoundingStateMachineDefaultException,        // 不存在此场景，默认异常处理
        [DLI_REPORT_SOUNDING_RESULT_EVENT] = HadmSoundingStateMachineDefaultException,      // 不存在此场景，默认异常处理
    },

    [HADM_SOUNDING_STATE_CONFIG_SOUNDING] = {
        [USER_START_SOUNDING_EVENT] = HadmProcDuplicateUserHandle,
        [USER_STOP_SOUNDING_EVENT] = HadmProcUserStopWhileStarting,                         // 这里缓存停止操作
        [CM_REPORT_LINK_STATE_CONNECTED] = HadmSoundingStateMachineDefaultException,
        [CM_REPORT_LINK_STATE_DISCONNECTED] = HadmProcCmLinkDownEventWhileStartSounding,
        [CM_REPORT_FEATURES_EVENT] = HadmSoundingStateMachineDefaultException,              // 不存在此场景，默认异常处理
        [CM_REPORT_UPDATE_CONN_PARAM_EVENT] = HadmSoundingStateMachineDefaultException,     // 不存在此场景，默认异常处理
        [DLI_REPORT_REMOTE_MEASURE_EVENT] = HadmSoundingStateMachineDefaultException,            // 不存在此场景，默认异常处理
        [DLI_REPORT_CONFIG_RESULT_EVENT] = HadmProcConfigResultEventInConfigSounding,        // 不存在此场景，默认异常处理
        [DLI_REPORT_SOUNDING_RESULT_EVENT] = HadmSoundingStateMachineDefaultException,      // 不存在此场景，默认异常处理
    },

    [HADM_SOUNDING_STATE_ENABLE_SOUNDING] = {
        [USER_START_SOUNDING_EVENT] = HadmProcDuplicateUserHandle,
        [USER_STOP_SOUNDING_EVENT] = HadmProcUserStopWhileStarting,                         // 这里缓存停止操作
        [CM_REPORT_LINK_STATE_CONNECTED] = HadmSoundingStateMachineDefaultException,
        [CM_REPORT_LINK_STATE_DISCONNECTED] = HadmProcCmLinkDownEventWhileStartSounding,
        [CM_REPORT_FEATURES_EVENT] = HadmSoundingStateMachineDefaultException,              // 不存在此场景，默认异常处理
        [CM_REPORT_UPDATE_CONN_PARAM_EVENT] = HadmSoundingStateMachineDefaultException,     // 不存在此场景，默认异常处理
        [DLI_REPORT_REMOTE_MEASURE_EVENT] = HadmSoundingStateMachineDefaultException,            // 不存在此场景，默认异常处理
        [DLI_REPORT_CONFIG_RESULT_EVENT] = HadmSoundingStateMachineDefaultException,        // 不存在此场景，默认异常处理
        [DLI_REPORT_SOUNDING_RESULT_EVENT] = HadmProcEnableResultEventInEnableSounding,
    },

    [HADM_SOUNDING_STATE_SOUNDING] = {
        [USER_START_SOUNDING_EVENT] = HadmProcDuplicateUserHandle,
        [USER_STOP_SOUNDING_EVENT] = HadmProcUserStopInSoundingState,
        [CM_REPORT_LINK_STATE_CONNECTED] = HadmSoundingStateMachineDefaultException,
        [CM_REPORT_LINK_STATE_DISCONNECTED] = HadmProcCmLinkDownEventWhileStartSounding,
        [CM_REPORT_FEATURES_EVENT] = HadmSoundingStateMachineDefaultException,              // 不存在此场景，默认异常处理
        [CM_REPORT_UPDATE_CONN_PARAM_EVENT] = HadmSoundingStateMachineDefaultException,     // 不存在此场景，默认异常处理
        [DLI_REPORT_REMOTE_MEASURE_EVENT] = HadmSoundingStateMachineDefaultException,            // 不存在此场景，默认异常处理
        [DLI_REPORT_CONFIG_RESULT_EVENT] = HadmSoundingStateMachineDefaultException,        // 不存在此场景，默认异常处理
        [DLI_REPORT_SOUNDING_RESULT_EVENT] = HadmSoundingStateMachineDefaultException,
    },

    [HADM_SOUNDING_STATE_DISABLE_SOUNDING] = {
        [USER_START_SOUNDING_EVENT] = HadmProcUserStartBeforeSoundingReady,
        [USER_STOP_SOUNDING_EVENT] = HadmProcDuplicateUserHandle,
        [CM_REPORT_LINK_STATE_CONNECTED] = HadmSoundingStateMachineDefaultException,
        [CM_REPORT_LINK_STATE_DISCONNECTED] = HadmProcCmLinkDownEventWhileStopSounding,
        [CM_REPORT_FEATURES_EVENT] = HadmSoundingStateMachineDefaultException,              // 不存在此场景，默认异常处理
        [CM_REPORT_UPDATE_CONN_PARAM_EVENT] = HadmSoundingStateMachineDefaultException,     // 不存在此场景，默认异常处理
        [DLI_REPORT_REMOTE_MEASURE_EVENT] = HadmSoundingStateMachineDefaultException,            // 不存在此场景，默认异常处理
        [DLI_REPORT_CONFIG_RESULT_EVENT] = HadmSoundingStateMachineDefaultException,        // 不存在此场景，默认异常处理
        [DLI_REPORT_SOUNDING_RESULT_EVENT] = HadmProcEnableResultEventInDisableSounding,
    }
};

static void HadmUpdateSoundingAddrs(SLE_Addr_S *addr)
{
    if (addr == NULL) {
        memset_s(&g_soundingAddrs, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
    } else {
        (void)memcpy_s(&g_soundingAddrs, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    }
    NLSTK_LOG_INFO("g_soundingAddrs update to %s.", GET_ENC_ADDR(&g_soundingAddrs));
}

static void HadmSoundingStateMachineDefaultException(SLE_Addr_S *addr, void *param)
{
    SDF_UNUSED(param);
    HadmSoundingState_E currentState = HadmGetSoundingStateByAddr(addr);
    NLSTK_LOG_ERROR("[HADM]the exception event happen, current state: %d, addr: %s", currentState, GET_ENC_ADDR(addr));

    NLSTK_CHECK_RETURN_VOID(currentState < HADM_SOUNDING_STATE_INVALID,
        "[HADM] sounding state is invalid. currentState: %d", currentState);
    // 在这里重新触发一遍状态机，即使是重复操作事件，在状态机中也会进行缓存处理，这里只是保证在异常场景下，状态机能够正常工作；
    HadmUserOperate_E cacheOp = HadmGetUserOperate(addr);
    if (cacheOp != HADM_USER_INVALID_OPERATE) {
        (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);
        uint32_t event = (cacheOp == HADM_USER_START_SOUNDING ? USER_START_SOUNDING_EVENT : USER_STOP_SOUNDING_EVENT);
        g_hadmSoundingStateMachine[currentState][event](addr, NULL);
    }
    return;
}

static void HadmProcUserStartBeforeSoundingReady(SLE_Addr_S *addr, void *param)
{
    SDF_UNUSED(param);
    // 在测距预准备完成之前，收到用户下发的测距启动命令，先缓存起来，等到测距预准备完成后再发出，此时不需要调整状态；
    HadmUserOperate_E cacheOp = HadmGetUserOperate(addr);
    if (cacheOp != HADM_USER_START_SOUNDING) {
        // 直接更新缓存的操作为启动测距；原则上此处不会失败，因为只有当linkcb不存在时，此处调用才会返回失败；
        // 但是在状态机的入口处已经判定了linkcb存在，所以原则下面的HadmCacheUserOperate一定成功；
        if (HadmCacheUserOperate(addr, HADM_USER_START_SOUNDING) != NLSTK_ERRCODE_SUCCESS) {
            // 如果测距缓存失败，则通知service，启动测距失败；
            NLSTK_LOG_ERROR("[HADM]cache user start sounding fail, addr: %s", GET_ENC_ADDR(addr));
            HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING, NLSTK_ERRCODE_FAIL);
            return;
        }
    }
}

static void HadmProcUserStopBeforeSoundingEnable(SLE_Addr_S *addr, void *param)
{
    SDF_UNUSED(param);
    // 在此状态下还未下发DLI操作，因此不需要缓存
    // 这里不判断返回值，是因为在状态机的入口处已经判定了linkcb存在，所以原则上此处调用一定成功；
    // 通过HADM_USER_INVALID_OPERATE，清理掉缓存的操作，然后上报停止测距成功的事件；
    (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);
    // 上报停止测距成功
    HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_STOP_SOUNDING, NLSTK_ERRCODE_SUCCESS);
    return;
}

static void HadmProcUserStopWhileStarting(SLE_Addr_S *addr, void *param)
{
    SDF_UNUSED(param);
    // 这里不判断返回值，是因为在状态机的入口处已经判定了linkcb存在，所以原则上此处调用一定成功；
    // 通过HADM_USER_INVALID_OPERATE，清理掉缓存的操作，然后上报停止测距成功的事件；
    (void)HadmCacheUserOperate(addr, HADM_USER_STOP_SOUNDING);
    return;
}

static void HadmProcCmLinkDownEventBeforeSoundingReady(SLE_Addr_S *addr, void *param)
{
    SDF_UNUSED(param);
    // 通过HADM_USER_INVALID_OPERATE，清理掉缓存的操作，然后上报停止测距成功的事件；
    HadmUserOperate_E cacheOp = HadmGetUserOperate(addr);
    // 这里不需要处理其它的缓存操作，因为此时不可能出现stop的缓存操作，即使存在说明是异常逻辑，断链之后会直接清理掉资源；
    if (cacheOp == HADM_USER_START_SOUNDING) {
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING, NLSTK_HADM_ERRCODE_LINK_EXCEPTION);
    } else if (cacheOp == HADM_USER_STOP_SOUNDING) {
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_STOP_SOUNDING, NLSTK_ERRCODE_SUCCESS);
    }
    (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);
    // 将状态调整成idle
    HadmSetSoundingState(addr, HADM_SOUNDING_STATE_IDLE);
    return;
}

static void HadmProcCmLinkDownEventWhileStartSounding(SLE_Addr_S *addr, void *param)
{
    SDF_UNUSED(param);
    // 通过HADM_USER_INVALID_OPERATE，清理掉缓存的操作，然后上报停止测距成功的事件；
    HadmUserOperate_E cacheOp = HadmGetUserOperate(addr);
    // 在启动测距过程中，如果缓存中有停止测距，则上报停止测距成功，否则上报启动测距失败；然后将状态调整成idle
    if (cacheOp == HADM_USER_STOP_SOUNDING) {
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_STOP_SOUNDING, NLSTK_ERRCODE_SUCCESS);
    } else {
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING, NLSTK_HADM_ERRCODE_LINK_EXCEPTION);
    }
    (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);
    // 将状态调整成idle
    HadmSetSoundingState(addr, HADM_SOUNDING_STATE_IDLE);
    HadmUpdateSoundingAddrs(NULL);  // 在停止测距的时候，更新当前测距的地址，将测距地址刷新为空；
    return;
}

static void HadmProcCmLinkDownEventWhileStopSounding(SLE_Addr_S *addr, void *param)
{
    SDF_UNUSED(param);
    // 在启动探测的过程中(HADM_SOUNDING_STATE_DISABLE_SOUNDING)，如果底层链路故障，直接上报停止测距成功，然后将状态调整成idle
    // 通过HADM_USER_INVALID_OPERATE，清理掉缓存的操作，然后上报停止测距成功的事件；
    HadmUserOperate_E cacheOp = HadmGetUserOperate(addr);
    // 在停止测距过程中，如果缓存中有启动测距，上报启动测距失败，则上报停止测距成功；然后将状态调整成idle
    if (cacheOp == HADM_USER_START_SOUNDING) {
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING, NLSTK_HADM_ERRCODE_LINK_EXCEPTION);
    } else {
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_STOP_SOUNDING, NLSTK_ERRCODE_SUCCESS);
    }
    // 清理下缓存，这里不需要考虑缓存中有启动测距的动作，因为在停止测距的时候，不允许有新的启动测距的缓存操作；
    (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);
    // 将状态调整成idle
    HadmSetSoundingState(addr, HADM_SOUNDING_STATE_IDLE);
    return;
}

static void HadmProcFeatruesEventInIdle(SLE_Addr_S *addr, void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[HADM] Input param nullptr when triger state machine.");
    bool supportSounding = *(((bool *)param));
    HadmUserOperate_E cacheOp = HadmGetUserOperate(addr);
    if (supportSounding) {
        // 在状态机启动之前判断lcid的有效性，此处不需要再额外进行判断；
        (void)HadmCacheRemoteFeatures(addr, HADM_PEER_SUPPORT_SOUNDING_YES);
        // 触发读取对端CS特性
        uint16_t lcid = HadmGetLcid(addr);
        NLSTK_LOG_INFO("[HADM]read remote sounding caps, addr: %s", GET_ENC_ADDR(addr));
        uint32_t ret = HadmReadRemoteMeasureCaps(lcid);
        if (ret != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[HADM]read remote sounding caps fail, addr: %s", GET_ENC_ADDR(addr));
            // 如果用户下发了测距，需要上报失败
            if (cacheOp == HADM_USER_START_SOUNDING) {
                (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);
                HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING, NLSTK_ERRCODE_TASK_FAIL);
            }
        }
        return;
    } else {
        (void)HadmCacheRemoteFeatures(addr, HADM_PEER_SUPPORT_SOUNDING_NO);
        // 这里不需要处理其它的缓存操作，因为此时不可能出现stop的缓存操作，即使存在说明是异常逻辑，断链之后会直接清理掉资源；
        if (cacheOp == HADM_USER_START_SOUNDING) {
            // 当下发DLI_CONFIG_SOUNDING_CAPS命令失败时，不调整状态，继续维持idle,但是清理掉缓存的操作；
            (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);
            // 上报启动失败
            HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING, NLSTK_HADM_ERRCODE_PEER_NOT_SUPPORT_SOUNDING);
        }
        return;
    }
}

static void HadmProcRemoteCsEventInReadRemoteCs(SLE_Addr_S *addr, void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[HADM] Input param nullptr when triger state machine.");
    HadmUserOperate_E cacheOp = HadmGetUserOperate(addr);

    HadmDliReportRemoteCsParam_S *args = (HadmDliReportRemoteCsParam_S *)param;
    if (args->status != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM]dli report read remote channel sounding caps fail, addr: %s", GET_ENC_ADDR(addr));
        // remote cs读取失败，认为对端不支持测距
        (void)HadmCacheRemoteFeatures(addr, HADM_PEER_SUPPORT_SOUNDING_NO);
        // 这里不需要处理其它的缓存操作，因为此时不可能出现stop的缓存操作，即使存在说明是异常逻辑，断链之后会直接清理掉资源；
        if (cacheOp == HADM_USER_START_SOUNDING) {
            // 清理掉缓存的操作；
            (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);
            // 上报启动失败
            HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING, NLSTK_HADM_ERRCODE_PEER_NOT_SUPPORT_SOUNDING);
        }
        return;
    }
    // 这里原则上已经启动了状态，不会出现失败的问题
    // 缓存对端的CS信息，后续使用
    (void)HadmCacheLinkRemoteCs(addr, &(args->csCaps));
    // 切换到SOUNDING_READY状态
    (void)HadmSetSoundingState(addr, HADM_SOUNDING_STATE_SOUNDING_READY);
    // 这里不需要处理其它的缓存操作，因为此时不可能出现stop的缓存操作，即使存在说明是异常逻辑，断链之后会直接清理掉资源；
    if (cacheOp == HADM_USER_START_SOUNDING) {
        // 当下发DLI_CONFIG_SOUNDING_CAPS命令失败时，不调整状态，继续维持idle,但是清理掉缓存的操作；
        (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);
        // 触发状态机
        g_hadmSoundingStateMachine[HADM_SOUNDING_STATE_SOUNDING_READY][USER_START_SOUNDING_EVENT](addr, NULL);
    }
}

static void HadmProcUserStartWhenSoundingReady(SLE_Addr_S *addr, void *param)
{
    SDF_UNUSED(param);
    // 直接清理缓存，因为此时已经在sounding ready状态，不需要缓存操作，以当前的操作为准；
    (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);

    HadmConnectionParam_S currentParam = { 0 };
    uint32_t ret = HadmGetConnectionParam(addr, &currentParam);
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_ERRCODE_SUCCESS,
                              "[Hadm] fail to get connection param while in start, addr: %s, ret %d.",
                              GET_ENC_ADDR(addr), ret);
    ret = HadmSetConnectionParamToCm(addr, &currentParam);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM]set connection param fail, addr: %s", GET_ENC_ADDR(addr));
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING, NLSTK_HADM_ERRCODE_CONFIG_CM_FAIL);
        return;
    }
    (void)HadmSetSoundingState(addr, HADM_SOUNDING_STATE_CONFIG_CONNECTION);
    return;
}

static void HadmProcUpdateConnParamEvent(SLE_Addr_S *addr, void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[HADM] Input param nullptr when triger state machine.");
    bool ifUpdateSuccess = *((bool *)param);
    if (!ifUpdateSuccess) {
        NLSTK_LOG_ERROR("[HADM]set connection param fail, addr: %s", GET_ENC_ADDR(addr));
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING, NLSTK_HADM_ERRCODE_CONFIG_CM_FAIL);
        return;
    }

    uint16_t lcid = HadmGetLcid(addr);  // 这里不需要额外判断lcid的有效性，因为在状态机启动之前已经判断过；
    HadmSoundingParam_S soundingParam = { 0 };
    uint32_t ret = HadmGetSoundingParam(addr, &soundingParam);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM]get sounding soundingParam fail, addr: %s", GET_ENC_ADDR(addr));
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING, NLSTK_ERRCODE_SYS_ERROR);
        HadmSetSoundingState(addr, HADM_SOUNDING_STATE_SOUNDING_READY);
        return;
    }
    ret = HadmSetMeasureParam(lcid, &soundingParam);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM]post task to set dli channel sounding soundingParam fail, addr: %s", GET_ENC_ADDR(addr));
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING, NLSTK_ERRCODE_TASK_FAIL);
        HadmSetSoundingState(addr, HADM_SOUNDING_STATE_SOUNDING_READY);
        return;
    }
    (void)HadmSetSoundingState(addr, HADM_SOUNDING_STATE_CONFIG_SOUNDING);
    return;
}

static void HadmProcDuplicateUserHandle(SLE_Addr_S *addr, void *param)
{
    SDF_UNUSED(param);
    // 直接清理缓存，因为此时已经在sounding ready状态，不需要缓存操作，以当前的操作为准；
    (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);
}

static void HadmProcConfigResultEventInConfigSounding(SLE_Addr_S *addr, void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[HADM] Input param nullptr when triger state machine.");
    HadmUserOperate_E cacheOp = HadmGetUserOperate(addr);
    if (cacheOp == HADM_USER_STOP_SOUNDING) {
        (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_STOP_SOUNDING, NLSTK_ERRCODE_SUCCESS);
        HadmSetSoundingState(addr, HADM_SOUNDING_STATE_SOUNDING_READY);
        return;
    }
    uint16_t status = *(((uint16_t *)param));
    // DLI上报的配置结果，如果配置失败，需要添加相应的配置失败的处理
    if (status != NLSTK_ERRCODE_SUCCESS) {  // DLI上报了配置结果
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING,
                                   NLSTK_HADM_ERRCODE_LINK_EXCEPTION);  // 配置失败，启动测距失败
        HadmSetSoundingState(addr, HADM_SOUNDING_STATE_SOUNDING_READY);
        return;
    }

    uint16_t lcid = HadmGetLcid(addr);
    uint32_t ret = HadmSetMeasureEnable(lcid, HADM_SOUNDING_ENABLE);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING, NLSTK_ERRCODE_TASK_FAIL);
        HadmSetSoundingState(addr, HADM_SOUNDING_STATE_SOUNDING_READY);
        return;
    }
    HadmSetSoundingState(addr, HADM_SOUNDING_STATE_ENABLE_SOUNDING);
    return;
}

static void HadmProcEnableResultEventInEnableSounding(SLE_Addr_S *addr, void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[HADM] Input param nullptr when triger state machine.");
    // DLI上报的配置结果，需要缓存，以便在状态机中使用，如果失败，则上报启动测距失败
    HadmUserOperate_E cacheOp = HadmGetUserOperate(addr);
    uint16_t status = *(((uint16_t *)param));
    if (status != NLSTK_ERRCODE_SUCCESS) {
        if (cacheOp == HADM_USER_STOP_SOUNDING) {
            // 如果启动测距失败，而且缓存中有停止测距，则上报停止测距成功；
            HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_STOP_SOUNDING, NLSTK_ERRCODE_SUCCESS);
            HadmSetSoundingState(addr, HADM_SOUNDING_STATE_SOUNDING_READY);
            return;
        }
        // 配置失败，启动测距失败
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING, NLSTK_HADM_ERRCODE_LINK_EXCEPTION);
        HadmSetSoundingState(addr, HADM_SOUNDING_STATE_SOUNDING_READY);
        return;
    }

    HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_START_SOUNDING, NLSTK_ERRCODE_SUCCESS);
    HadmSetSoundingState(addr, HADM_SOUNDING_STATE_SOUNDING);
    HadmUpdateSoundingAddrs(addr);  // 在启动测距的时候，更新当前测距的地址
    // 如果缓存中有停止测距，则启动停止测距的流程，通过触发状态机实现；
    if (cacheOp == HADM_USER_STOP_SOUNDING) {
        (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);
        g_hadmSoundingStateMachine[HADM_SOUNDING_STATE_SOUNDING][USER_STOP_SOUNDING_EVENT](addr, NULL);
    }
    return;
}

static void HadmProcEnableResultEventInDisableSounding(SLE_Addr_S *addr, void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[HADM] Input param nullptr when triger state machine.");

    uint16_t status = *(((uint16_t *)param));
    if (status != NLSTK_ERRCODE_SUCCESS) {
        // 如果DLI上报命令执行失败，则触发一个停止测距失败的回调；
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_STOP_SOUNDING, NLSTK_HADM_ERRCODE_LINK_EXCEPTION);
        NLSTK_LOG_ERROR("[HADM] DLI report the disable sounding fail");
    } else {
        // 如果DLI上报命令执行成功，则触发一个停止测距成功的回调；
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_STOP_SOUNDING, NLSTK_ERRCODE_SUCCESS);
    }
    // 无论DLI上报的命令是什么，在收到停止测距的逻辑之后，将HADM的状态切换成ready状态；
    // 由于目前无DLI的具体行为，HADM无法兼容DLI的异常处理逻辑；
    HadmSetSoundingState(addr, HADM_SOUNDING_STATE_SOUNDING_READY);
    // 如果缓存中有启动测距，则开始启动测距的流程，通过触发状态机实现；
    HadmUserOperate_E cacheOp = HadmGetUserOperate(addr);
    if (cacheOp == HADM_USER_START_SOUNDING) {
        // 如果service针对同一个地址调用 启动测距，停止测距，然后再调用启动测距，这个时候不会上报中间的停止测距成功
        // 因此这里不需要调用回调函数通知测距状态的上报
        (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);
        g_hadmSoundingStateMachine[HADM_SOUNDING_STATE_SOUNDING_READY][USER_START_SOUNDING_EVENT](addr, NULL);
        return;
    }
    return;
}

static void HadmProcUserStopInSoundingState(SLE_Addr_S *addr, void *param)
{
    SDF_UNUSED(param);
    // 为了可靠性，先清一遍缓存
    (void)HadmCacheUserOperate(addr, HADM_USER_INVALID_OPERATE);

    uint16_t lcid = HadmGetLcid(addr);
    uint32_t ret = HadmSetMeasureEnable(lcid, HADM_SOUNDING_DISABLE);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM]post task to disable channel sounding fail, addr: %s", GET_ENC_ADDR(addr));
        HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_STOP_SOUNDING, NLSTK_HADM_ERRCODE_LINK_EXCEPTION);
        return;  // 这里不调整状态，由service来处理这种停止测距失败的情况
    }
    HadmSetSoundingState(addr, HADM_SOUNDING_STATE_DISABLE_SOUNDING);
    HadmUpdateSoundingAddrs(NULL);  // 在停止测距的时候，更新当前测距的地址，将测距地址刷新为空；
    return;
}

uint32_t HadmTriggerStateMachine(SLE_Addr_S *addr, uint32_t event, void *param)
{
    // param 存在为空的场景，因此不再此处判断，在状态机中进行判断
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[HADM] Input addr nullptr when triger state machine.");
    HadmSoundingState_E state = HadmGetSoundingStateByAddr(addr);
    if (state == HADM_SOUNDING_STATE_INVALID) {
        NLSTK_LOG_ERROR("[HADM]get invalid state when triger state machine, addr: %s", GET_ENC_ADDR(addr));
        if (event == USER_STOP_SOUNDING_EVENT) { // 如果是用户下发的停止测距事件，则直接返回停止测距成功
            HADM_REPORT_CONTROL_RESULT(addr, HADM_USER_STOP_SOUNDING, NLSTK_ERRCODE_SUCCESS);
            return NLSTK_ERRCODE_SUCCESS;
        }
        return NLSTK_ERRCODE_FAIL;
    }
    if (event < USER_START_SOUNDING_EVENT || event > DLI_REPORT_SOUNDING_RESULT_EVENT ||
        state < HADM_SOUNDING_STATE_IDLE || state > HADM_SOUNDING_STATE_DISABLE_SOUNDING) {
        NLSTK_LOG_ERROR("[HADM]event or state out of range when trige state machine, addr: %s, event: %u, state: %u",
                      GET_ENC_ADDR(addr), event, state);
        return NLSTK_ERRCODE_SYS_ERROR;
    }
    g_hadmSoundingStateMachine[state][event](addr, param);
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t HadmTriggerStateMachineByLcid(uint16_t lcid, uint32_t event, void *param)
{
    SLE_Addr_S addr = { 0 };
    uint32_t ret = HadmGetAddrsByLcid(lcid, &addr);  // 这里不判断返回值，因为如果返回失败，addr为0，后续状态机中会处理
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("HADM]get addr by lcid fail when trige state machine, lcid: %u", lcid);
        return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
    }
    return HadmTriggerStateMachine(&addr, event, param);
}

/**
 * @brief  北向回调函数的存储
 * @return void
 */
void HadmInitStateMachine(HadmSoundingCbk_S *hadmCbk)
{
    NLSTK_CHECK_RETURN_VOID(hadmCbk != NULL, "[HADM] Register event callback nullptr.");
    g_hadmReportCbk.reportIqDataCbk = hadmCbk->reportIqDataCbk;
    g_hadmReportCbk.controlResultCbk = hadmCbk->controlResultCbk;
    g_hadmReportCbk.soundingStateReportCbk = hadmCbk->soundingStateReportCbk;
    NLSTK_LOG_INFO("[HADM] Register event callback succeed.");
    return;
}

void HadmReportSoundingStateFromDli(HadmSoundingStateInfo_S *param)
{
    if (g_hadmReportCbk.soundingStateReportCbk == NULL) {
        NLSTK_LOG_ERROR("[HADM]Service do not Register Sounding state report cbk.");
        return;
    }
    g_hadmReportCbk.soundingStateReportCbk(param);
    return;
}

static void HadmGetLocalOffSetInfo(uint16_t *localNvOffset, uint16_t *localTofOffset)
{
    CfgdbLocalCsCaps_S localCsCaps;
    *localNvOffset = HADM_CAL_OFFSET_DEFAULT;
    *localTofOffset = HADM_CAL_TOF_OFFSET_DEFAULT;
    // 当前这里不可能失败，此被调函数中仅存在入参是否空的判断，针对localcs会在启动的时候获取，如果获取失败启动会失败；
    uint32_t ret = CfgdbReadLocalCsCaps(&localCsCaps);
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_ERRCODE_SUCCESS, "[HADM] fail to get local cs caps when report iq data.");
    HadmRemoteCsParam_S *localCsCapsStruct = (HadmRemoteCsParam_S *)localCsCaps.caps;
    uint16_t nvOffset = localCsCapsStruct->phaseCaliOffsetCm;
    uint16_t tofOffset = localCsCapsStruct->tofCaliOffsetM;
    NLSTK_LOG_INFO("[HADM] get local offset info, nvOffset: %u, tofOffset: %u", nvOffset, tofOffset);
    if (nvOffset < HADM_CAL_OFFSET_MIN || nvOffset > HADM_CAL_OFFSET_MAX) {
        nvOffset = HADM_CAL_OFFSET_DEFAULT;
    }
    if (tofOffset > HADM_CAL_TOF_OFFSET_MAX) {
        tofOffset = HADM_CAL_TOF_OFFSET_DEFAULT;
    }
    *localNvOffset = nvOffset;
    *localTofOffset = tofOffset;
    return;
}

static void HadmGetRemoteOffsetInfo(SLE_Addr_S *addr, uint16_t *remoteNvOffSet, uint16_t *reomoteTofOffset)
{
    HadmRemoteCsParam_S remoteCsCaps = { 0 };
    *remoteNvOffSet = HADM_CAL_OFFSET_DEFAULT;
    *reomoteTofOffset = HADM_CAL_TOF_OFFSET_DEFAULT;
    uint32_t ret = HadmGetLinkRemoteCs(addr, &remoteCsCaps);
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_ERRCODE_SUCCESS, "[HADM] fail to get remote cs caps when report iq data.");
    uint16_t nvOffset = remoteCsCaps.phaseCaliOffsetCm;
    uint16_t tofOffset = remoteCsCaps.tofCaliOffsetM;
    NLSTK_LOG_INFO("[HADM]get remote offset info success, nvOffset: %u, tofOffset: %u", nvOffset, tofOffset);
    if (nvOffset < HADM_CAL_OFFSET_MIN || nvOffset > HADM_CAL_OFFSET_MAX) {
        nvOffset = HADM_CAL_OFFSET_DEFAULT;
    }
    if (tofOffset > HADM_CAL_TOF_OFFSET_MAX) {
        tofOffset = HADM_CAL_TOF_OFFSET_DEFAULT;
    }
    *remoteNvOffSet = nvOffset;
    *reomoteTofOffset = tofOffset;
    return;
}

static HadmSoundingIqData_S *HadmBuildIqDataToService(SLE_Addr_S *addr, HadmIqInfoFromDli_S *localIqInfo,
                                                      HadmIqInfoFromDli_S *remoteIqInfo)
{
    NLSTK_CHECK_RETURN(addr != NULL && localIqInfo != NULL && remoteIqInfo != NULL, NULL,
                         "[HADM] Input param nullptr.");
    NLSTK_CHECK_RETURN(
        localIqInfo->iqChnlNum == remoteIqInfo->iqChnlNum, NULL,
        "[HADM] local iq chnl num is not equal to remote iq chnl num when report iq data. local: %d, remote: %d",
        localIqInfo->iqChnlNum, remoteIqInfo->iqChnlNum);
    // 不对入参进行有效性检查，因为该函数仅一个地方使用，且属于内部函数，外部调用已经保证有效性，不做冗余检查
    HadmSoundingIqData_S *iqData = (HadmSoundingIqData_S *)SDF_MemZalloc(sizeof(HadmSoundingIqData_S));
    NLSTK_CHECK_RETURN(iqData != NULL, NULL, "[HADM] malloc mem for iqData fail, addrs:%s", GET_ENC_ADDR(addr));
    iqData->timeStampSn = localIqInfo->timeStampSn;
    iqData->dutRssi = localIqInfo->rssi;
    iqData->rtdRssi = remoteIqInfo->rssi;
    /*
    * 2. tof_result表示ToF(Time of Flight)的时间，为了保证芯片是0.1ns的精度，原始tof_result做了乘10的操作，
    * 所以换算的时候要乘以10^(-1), 还原回去。光速是3*10^8 m/s。因此距离可以计算为：
    * 距离 = tof_result * 10^(-9) * 3 * 10^8 * 10^(-1) m = tof_result * 0.03 m。
    * 根据本端和对端的tof_result，可以计算得出底层芯片上报的双端测距值，这个值可供slem算法参考
    * 双端测距值 = (本端tof_result + 对端tof_result) * 0.03 / 2 - tofCalib * 2
    * 其中 tofCalib的含义：Calibration value(校准值) of ToF.
    */
    iqData->dutTof = localIqInfo->tofResult * 3 / 100;  // 3/100是为了乘以光速，将时间转换成距离，详细说明见上面注释
    iqData->rtdTof = remoteIqInfo->tofResult * 3 / 100;  // 3/100是为了乘以光速，将时间转换成距离，详细说明见上面注释
    iqData->iqChnlNum = remoteIqInfo->iqChnlNum > localIqInfo->iqChnlNum ? localIqInfo->iqChnlNum :
                                                                          remoteIqInfo->iqChnlNum;
    iqData->iqData = (HadmReportIqData_S *)SDF_MemZalloc(sizeof(HadmReportIqData_S) * iqData->iqChnlNum);
    if (iqData->iqData == NULL) {
        NLSTK_LOG_ERROR("HADM]malloc mem for iqData->iqData fail, addrs:%s", GET_ENC_ADDR(addr));
        SDF_MemFree(iqData);
        return NULL;
    }
    for (uint32_t index = 0; index < localIqInfo->iqChnlNum && index < remoteIqInfo->iqChnlNum; index++) {
        HadmReportIqData_S *singleIqData = &(iqData->iqData[index]);
        HadmIqValue_S *localIqValue = &(localIqInfo->iqData[index]);
        HadmIqValue_S *remoteIqValue = &(remoteIqInfo->iqData[index]);
        singleIqData->dutIData = localIqValue->iData;
        singleIqData->dutQData = localIqValue->qData;
        singleIqData->rtdIData = remoteIqValue->iData;
        singleIqData->rtdQData = remoteIqValue->qData;
    }
    uint16_t localNvOffset = HADM_CAL_OFFSET_DEFAULT;
    uint16_t localTofOffset = HADM_CAL_TOF_OFFSET_DEFAULT;
    uint16_t remoteNvOffset = HADM_CAL_OFFSET_DEFAULT;
    uint16_t remoteTofOffset = HADM_CAL_TOF_OFFSET_DEFAULT;
    HadmGetLocalOffSetInfo(&localNvOffset, &localTofOffset);
    HadmGetRemoteOffsetInfo(addr, &remoteNvOffset, &remoteTofOffset);
    iqData->isMultiTone = (localIqInfo->isMultiTone || remoteIqInfo->isMultiTone) ? 1 : 0;
    iqData->dutIqBitLen = localIqInfo->iqBitLen;
    iqData->rtdIqBitLen = remoteIqInfo->iqBitLen;
    (void)memcpy_s(iqData->dutSlemChmap, sizeof(iqData->dutSlemChmap),
        localIqInfo->slemChmap.chmap, sizeof(localIqInfo->slemChmap.chmap));
    (void)memcpy_s(iqData->rtdSlemChmap, sizeof(iqData->rtdSlemChmap),
        remoteIqInfo->slemChmap.chmap, sizeof(remoteIqInfo->slemChmap.chmap));
    iqData->localNvOffset = localNvOffset;
    iqData->remoteNvOffset = remoteNvOffset;
    iqData->localTofOffset = localTofOffset;
    iqData->remoteTofOffset = remoteTofOffset;
    return iqData;
}

static void HadmFreeIqData(HadmSoundingIqData_S *iqData)
{
    if (iqData != NULL) {
        if (iqData->iqData != NULL) {
            SDF_MemFree(iqData->iqData);
            iqData->iqData = NULL;
        }
        SDF_MemFree(iqData);
    }
    return;
}

static void HadmReportSoundingIQResult(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN_VOID(g_hadmReportCbk.reportIqDataCbk != NULL,
                              "[HADM]Service do not Register Sounding Iq report cbk.");
    HadmIqInfoFromDli_S *localIqInfo = HadmGetLocalIqInfo(addr);
    HadmIqInfoFromDli_S *remoteIqInfo = HadmGetRemoteIqInfo(addr);
    if (localIqInfo == NULL || remoteIqInfo == NULL) {
        // 这里有可能是正常场景，例如仅上报了local的信息；
        NLSTK_LOG_INFO("[HADM]localIqInfo or remoteIqInfo is null when report iq data, addrs: %s", GET_ENC_ADDR(addr));
        return;
    }

    // 构造北向上报的IQ数据
    HadmSoundingIqData_S *iqData = HadmBuildIqDataToService(addr, localIqInfo, remoteIqInfo);
    NLSTK_CHECK_RETURN_VOID(iqData != NULL, "[HADM]malloc iq data fail when report iq data.");  // 构造失败，直接返回
    g_hadmReportCbk.reportIqDataCbk(addr, iqData);
    NLSTK_LOG_INFO("[HADM]report iq data to service, addrs: %s", GET_ENC_ADDR(addr));
    // 每轮上报完成之后，将缓存的IQ信息清空，以便下轮测距使用
    HadmCacheLocalIqInfo(addr, NULL);
    HadmCacheRemoteIqInfo(addr, NULL);
    HadmFreeIqData(iqData);
    return;
}

uint32_t HadmReportSoundingIqInfoFromDli(uint16_t lcid, HadmIqInfoFromDli_S *iqInfo)
{
    SDF_UNUSED(lcid);
    SLE_Addr_S *addr = &g_soundingAddrs;
    uint32_t ret = NLSTK_ERRCODE_SUCCESS;  // 这里默认为成功，因为即使失败，后续流程也不受影响

    HadmSoundingState_E state = HadmGetSoundingStateByAddr(addr);
    if (state == HADM_SOUNDING_STATE_INVALID) {
        NLSTK_LOG_ERROR("[HADM]get invalid state when trige state machine, addr: %s", GET_ENC_ADDR(addr));
        return NLSTK_ERRCODE_FAIL;
    } else if (state != HADM_SOUNDING_STATE_SOUNDING) {
        // 这里不返回，因为之前未下沉的代码中也没有这种逻辑的判断，因此仅打印日志；
        NLSTK_LOG_INFO("the state is wrong, when get the IQ data from DLI. addrs: %s, state: %d", GET_ENC_ADDR(addr),
            state);
    }

    if (iqInfo->localId == 0) {
        ret = HadmCacheLocalIqInfo(addr, iqInfo);
        NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret,
                             "[HADM]cache local iq info fail when report iq data, addrs: %s", GET_ENC_ADDR(addr));
    } else {
        HadmIqInfoFromDli_S *localIqInfo = HadmGetLocalIqInfo(addr);
        NLSTK_CHECK_RETURN(localIqInfo != NULL, NLSTK_ERRCODE_SYS_ERROR,
                             "[HADM]get local iq info fail when report iq data, addrs: %s", GET_ENC_ADDR(addr));
        uint32_t diff = (iqInfo->timeStampSn > localIqInfo->timeStampSn) ?
                            iqInfo->timeStampSn - localIqInfo->timeStampSn :
                            localIqInfo->timeStampSn - iqInfo->timeStampSn;
        NLSTK_CHECK_RETURN(diff <= HDAM_SOUNDING_TS_MAX_DIFF, NLSTK_ERRCODE_TASK_TIMEOUT,
                             "the timestampSn is bigger Than 10, local TimerStamp is %u, remote timerStamp is %u.",
                             localIqInfo->timeStampSn, iqInfo->timeStampSn);
        ret = HadmCacheRemoteIqInfo(addr, iqInfo);
        NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret,
                             "[HADM]cache remote iq info fail when report iq data, addrs: %s", GET_ENC_ADDR(addr));
    }
    HadmReportSoundingIQResult(addr);
    return NLSTK_ERRCODE_SUCCESS;
}