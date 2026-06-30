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
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "nlstk_schedule.h"
#include "securec.h"
#include "hadm_sm.h"
#include "hadm_link_manager.h"
#include "hadm_user_proc.h"

void HadmServiceRegisterCallBack(void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[Hadm] param is null in HadmServiceRegisterCallBack");
    HadmSoundingCbk_S *hadmCbksIn = (HadmSoundingCbk_S *)param;
    HadmInitStateMachine(hadmCbksIn);
    return;
}

static void HadmStartSoundingTask(void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[Hadm] param is null in HadmStartSoundingTask");
    SLE_Addr_S *addr = (SLE_Addr_S *)param;
    // 这里触发状态机，如果返回失败只需要打印日志即可，根据代码分析，此处不会出现返回失败的情况，如果后续有修改，需要重新分析
    uint32_t ret = HadmTriggerStateMachine(addr, USER_START_SOUNDING_EVENT, NULL);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("HadmTriggerStateMachine failed, addr: %s, ret: %d", GET_ENC_ADDR(addr), ret);
    }
    return;
}

static uint32_t HadmUserStartInvaidCheck(SLE_Addr_S *addr, HadmConnectionParam_S *param,
                                         HadmSoundingParam_S *soundParam)
{
    // 检查地址是否合法，如果不合法直接返回错误，也就是CM链路是否已经建立
    HadmSoundingState_E state = HadmGetSoundingStateByAddr(addr);
    if (state == HADM_SOUNDING_STATE_INVALID) {
        NLSTK_LOG_ERROR("[Hadm]addr not exsit, addr: %s", GET_ENC_ADDR(addr));
        return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
    }
    HadmPeerSupportSounding_E peerSupport = HADM_PEER_SUPPORT_SOUNDING_DEFALUT;
    // 检查对端是否支持测距，如果不支持测距则直接返回错误，对端不支持测距的时候，不会建立测距的链路，所以此处不检查链路状态
    if (HadmGetRemoteFeatures(addr, &peerSupport) == NLSTK_ERRCODE_SUCCESS) {
        // HADM_PEER_SUPPORT_SOUNDING_DEFALUT表示前置工作还没完成，此处不认为不支持测距，因此判断HADM_PEER_SUPPORT_SOUNDING_NO
        if (peerSupport == HADM_PEER_SUPPORT_SOUNDING_NO) {
            NLSTK_LOG_ERROR("[Hadm] peer not support sounding, addr: %s", GET_ENC_ADDR(addr));
            return NLSTK_HADM_ERRCODE_PEER_NOT_SUPPORT_SOUNDING;
        }
    } else {
        NLSTK_LOG_ERROR("[Hadm]get remote feature error, addr: %s", GET_ENC_ADDR(addr));
        return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
    }

    // 首先针对并行测距的规格进行校验，查看当前是否还有其它的测距存在，如果已经到达上限则直接返回失败
    SLE_Addr_S soundingAddr[HADM_MAX_PARALLEL_SOUNDING_NUM] = {0};
    uint32_t soundNumber = HadmGetSoundingNumAndAddr(soundingAddr, HADM_MAX_PARALLEL_SOUNDING_NUM);
    if (soundNumber >= HADM_MAX_PARALLEL_SOUNDING_NUM) {
        uint32_t i = 0;
        for (; i < soundNumber; i++) {
            if (SDF_CompareSleAddr(&soundingAddr[i], addr) == 0) {
                break;
            }
        }
        // 如果地址当前未在启动测距的流程，说明当前已经达到了规格，返回失败
        if (i == soundNumber) {
            NLSTK_LOG_ERROR("[Hadm] sounding number is already max, addr: %s", GET_ENC_ADDR(addr));
            return NLSTK_HADM_ERRCODE_MAX_PARALLEL_SOUNDING_NUM;
        }
    }

    // 在并行测距的规格满足之后，就看当前本地址的测距情况，避免使用不同的参数重复测距
    // 针对参数相同的重复测距，在状态机中进行拦截
    if (state == HADM_SOUNDING_STATE_IDLE || state == HADM_SOUNDING_STATE_SOUNDING_READY ||
        state == HADM_SOUNDING_STATE_DISABLE_SOUNDING) {
        // 当前处于空闲或者就绪状态，可以直接启动测距，而不需要更新参数，因为相关的参数还未下发配置
        return NLSTK_ERRCODE_SUCCESS;
    }

    // 如果在其它状态，则需要判断测距参数是否完全一样，如果一样则表示启动的是同一路测距，则直接返回成功
    // 否则先停止当前测距，再启动测距
    HadmConnectionParam_S currentParam;
    uint32_t ret = HadmGetConnectionParam(addr, &currentParam);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[Hadm] fail to get connection param, addr: %s",
                         GET_ENC_ADDR(addr));

    HadmSoundingParam_S currentSoundParam;
    ret = HadmGetSoundingParam(addr, &currentSoundParam);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[Hadm] fail to get sounding param, addr: %s",
                         GET_ENC_ADDR(addr));

    if (memcmp(&currentParam, param, sizeof(HadmConnectionParam_S)) != 0 ||
        memcmp(&currentSoundParam, soundParam, sizeof(HadmSoundingParam_S)) != 0) {
        NLSTK_LOG_ERROR("Hadm] sounding param is not same as current, addr: %s", GET_ENC_ADDR(addr));
        return NLSTK_HADM_ERRCODE_ADDR_ALREADY_IN_SOUNDING;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

void HadmUserStartSounding(void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[Hadm] param is null in HadmUserStartSounding");
    HadmUserStartSoundingParam_S *userStartSoundParam = (HadmUserStartSoundingParam_S *)param;
    userStartSoundParam->startResult = NLSTK_ERRCODE_MAX;

    HadmConnectionParam_S *connectParam = &(userStartSoundParam->connectionParam);
    HadmSoundingParam_S *soundParam = &(userStartSoundParam->soundingParam);
    SLE_Addr_S *addr = &(userStartSoundParam->addr);
    uint32_t ret = HadmUserStartInvaidCheck(addr, connectParam, soundParam);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        userStartSoundParam->startResult = ret;
        NLSTK_LOG_ERROR("[Hadm] HadmUserStartInvaidCheck failed, addr: %s, ret: %d", GET_ENC_ADDR(addr), ret);
        return;
    }
    ret = HadmCacheConnectionParam(addr, connectParam);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        userStartSoundParam->startResult = ret;
        NLSTK_LOG_ERROR("[Hadm] HadmCacheConnectionParam failed, addr: %s, ret: %d", GET_ENC_ADDR(addr), ret);
        return;
    }
    ret = HadmCacheSoundingParam(addr, soundParam);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        userStartSoundParam->startResult = ret;
        NLSTK_LOG_ERROR("[Hadm] HadmCacheSoundingParam failed, addr: %s, ret: %d", GET_ENC_ADDR(addr), ret);
        return;
    }
    SLE_Addr_S *startAddr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    if (startAddr == NULL) {
        userStartSoundParam->startResult = NLSTK_ERRCODE_MALLOC_FAIL;
        NLSTK_LOG_ERROR("[Hadm]mem alloc failed in HadmUserStartSounding, addr: %s", GET_ENC_ADDR(addr));
        return;
    }
    (void)memcpy_s(startAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));

    ret = SchedulePostTask(HadmStartSoundingTask, (void *)startAddr, SDF_MemFree);
    if (ret != NLSTK_OK) {
        userStartSoundParam->startResult = NLSTK_ERRCODE_TASK_FAIL;
        NLSTK_LOG_ERROR("[Hadm] fail to post task HadmPostStartSoundingTask, addr:%s", GET_ENC_ADDR(addr));
        return;
    }
    userStartSoundParam->startResult = NLSTK_ERRCODE_SUCCESS;
    return;
}

void HadmUserStopSounding(void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[Hadm] param is null in HadmUserStopSounding");
    SLE_Addr_S *addr = (SLE_Addr_S *)param;
    // 这里触发状态机，如果返回失败只需要打印日志即可，根据代码分析，此处不会出现返回失败的情况，如果后续有修改，需要重新分析
    uint32_t ret = HadmTriggerStateMachine(addr, USER_STOP_SOUNDING_EVENT, NULL);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("HadmTriggerStateMachine failed, addr: %s, ret: %d", GET_ENC_ADDR(addr), ret);
    }
    return;
}

void HadmUserGetSoundingState(void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[Hadm] param is null in HadmUserGetSoundingState");
    HadmSoundingStateParam_S *soundingStateResult = (HadmSoundingStateParam_S *)param;
    soundingStateResult->state = HADM_SOUNDING_STOP;

    HadmSoundingState_E soundingState = HadmGetSoundingStateByAddr(&(soundingStateResult->addr));
    NLSTK_CHECK_RETURN_VOID(soundingState != HADM_SOUNDING_STATE_INVALID,
                              "[Hadm] addr not exsit, return default state. addr: %s",
                              GET_ENC_ADDR(&(soundingStateResult->addr)));

    HadmUserOperate_E op = HadmGetUserOperate(&(soundingStateResult->addr));
    if (op == HADM_USER_INVALID_OPERATE) {
        // 当用户没有缓存的时候，根据当前的状态机状态返回对应的状态值，只有当空闲、就绪、停止的时候才是返回Stop；
        if (soundingState == HADM_SOUNDING_STATE_IDLE || soundingState == HADM_SOUNDING_STATE_SOUNDING_READY ||
            soundingState == HADM_SOUNDING_STATE_DISABLE_SOUNDING) {
            soundingStateResult->state = HADM_SOUNDING_STOP;
        } else {
            soundingStateResult->state = HADM_SOUNDING_START;
        }
    } else if (op == HADM_USER_START_SOUNDING) {
        // 当用户缓存中存在Start的时候，返回start，因为后面一定会启动测距
        soundingStateResult->state = HADM_SOUNDING_START;
    } else if (op == HADM_USER_STOP_SOUNDING) {
        // 当用户缓存中有stop的时候，只有当下发了启动测距或者当前处于测距的时候，才返回start，因为停止测距属于异步逻辑
        // 其他情况返回stop
        if (soundingState == HADM_SOUNDING_STATE_ENABLE_SOUNDING || soundingState == HADM_SOUNDING_STATE_SOUNDING) {
            soundingStateResult->state = HADM_SOUNDING_START;
        } else {
            soundingStateResult->state = HADM_SOUNDING_STOP;
        }
    }
    return;
}

void HadmUserGetSoundingAddrInfo(void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[Hadm] param is null in HadmUserGetSoundingAddrInfo");
    HadmGetSoundingAddrParam_S *soundingAddrResult = (HadmGetSoundingAddrParam_S *)param;
    soundingAddrResult->soundingAddrNum =
        HadmGetSoundingNumAndAddr(soundingAddrResult->addr, HADM_MAX_PARALLEL_SOUNDING_NUM);
    return;
}