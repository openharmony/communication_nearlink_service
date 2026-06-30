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
#include "sdf_mem.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "hadm_user_proc.h"
#include "hadm_api.h"

NLSTK_Errcode_E HadmRegCbk(HadmSoundingCbk_S *cbk)
{
    NLSTK_CHECK_RETURN(cbk != NULL, NLSTK_ERRCODE_POINTER_NULL, "[HADM] Register event cbks null pointer.");
    HadmSoundingCbk_S *hadmCbks = (HadmSoundingCbk_S *)SDF_MemZalloc(sizeof(HadmSoundingCbk_S));
    NLSTK_CHECK_RETURN(hadmCbks != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[HADM] alloc mem fail in HadmRegCbk.");
    hadmCbks->reportIqDataCbk = cbk->reportIqDataCbk;
    hadmCbks->controlResultCbk = cbk->controlResultCbk;
    hadmCbks->soundingStateReportCbk = cbk->soundingStateReportCbk;
    uint32_t taskRet =
        SchedulePostTaskBlocked(HadmServiceRegisterCallBack, (void *)hadmCbks, SDF_MemFree, NLSTK_API_TIME_OUT);
    if (taskRet == NLSTK_ERRCODE_TASK_TIMEOUT) {
        NLSTK_LOG_ERROR("[Hadm] HadmRegCbk task time out");
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (taskRet != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E HadmStartSounding(SLE_Addr_S *addr, HadmConnectionParam_S *param, HadmSoundingParam_S *soundParam)
{
    NLSTK_CHECK_RETURN(addr != NULL && param != NULL && soundParam != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[Hadm]input null point when start sounding");
    NLSTK_LOG_INFO("HadmStartSounding start, addr is %s.", GET_ENC_ADDR(addr));
    HadmUserStartSoundingParam_S *soundingParam =
        (HadmUserStartSoundingParam_S *)SDF_MemZalloc(sizeof(HadmUserStartSoundingParam_S));
    NLSTK_CHECK_RETURN(soundingParam != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[Hadm]mem alloc fail, when start sounding");
    (void)memcpy_s(&(soundingParam->addr), sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(&(soundingParam->connectionParam), sizeof(HadmConnectionParam_S), param,
        sizeof(HadmConnectionParam_S));
    (void)memcpy_s(&(soundingParam->soundingParam), sizeof(HadmSoundingParam_S), soundParam,
        sizeof(HadmSoundingParam_S));
    soundingParam->startResult = NLSTK_ERRCODE_SYS_ERROR;

    uint32_t taskRet = SchedulePostTaskBlocked(HadmUserStartSounding, (void *)soundingParam, NULL, NLSTK_API_TIME_OUT);
    if (taskRet == NLSTK_ERRCODE_TASK_TIMEOUT) {
        SchedulePostTask(SDF_MemFree, (void *)soundingParam, NULL);
        NLSTK_LOG_ERROR("[Hadm] HadmStartSounding task time out, addr is %s", GET_ENC_ADDR(addr));
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (taskRet != NLSTK_OK) {
        SDF_MemFree(soundingParam);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    NLSTK_Errcode_E ret = soundingParam->startResult;
    SDF_MemFree(soundingParam);
    return ret;
}

NLSTK_Errcode_E HadmStopSounding(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[Hadm]input nulln point when stop sounding");
    SLE_Addr_S *stopAddr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(stopAddr != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
                         "[Hadm]mem alloc fail, when get stop stop sounding");
    NLSTK_LOG_INFO("HadmStopSounding start, addr is %s.", GET_ENC_ADDR(addr));
    (void)memcpy_s(stopAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    uint32_t ret = SchedulePostTask(HadmUserStopSounding, (void *)stopAddr, SDF_MemFree);
    if (ret != NLSTK_OK) {
        NLSTK_LOG_ERROR("[Hadm] fail to post task HadmUserStopSounding");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E HadmGetSoundingState(SLE_Addr_S *addr, uint8_t *state)
{
    NLSTK_CHECK_RETURN(addr != NULL && state != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[Hadm]input null point when Get the sounding state");
    *state = HADM_SOUNDING_STOP;
    HadmSoundingStateParam_S *soundingState =
        (HadmSoundingStateParam_S *)SDF_MemZalloc(sizeof(HadmSoundingStateParam_S));
    NLSTK_CHECK_RETURN(soundingState != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
                         "[Hadm]mem alloc fail, when get sounding state");
    (void)memcpy_s(&(soundingState->addr), sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));

    soundingState->state = HADM_SOUNDING_STOP;
    uint32_t ret = SchedulePostTaskBlocked(HadmUserGetSoundingState, (void *)soundingState, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        SchedulePostTask(SDF_MemFree, (void *)soundingState, NULL);
        NLSTK_LOG_ERROR("[Hadm] HadmGetSoundingState task time out, addr is %s", GET_ENC_ADDR(addr));
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_OK) {
        SDF_MemFree(soundingState);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *state = soundingState->state;
    SDF_MemFree(soundingState);
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t HadmGetSoundingAddrInfo(SLE_Addr_S *addr, uint32_t maxNum)
{
    HadmGetSoundingAddrParam_S *soundingAddrInfo =
        (HadmGetSoundingAddrParam_S *)SDF_MemZalloc(sizeof(HadmGetSoundingAddrParam_S));
    NLSTK_CHECK_RETURN(soundingAddrInfo != NULL, 0, "[Hadm] HadmGetSoundingAddrInfo mem alloc fail");
    soundingAddrInfo->soundingAddrNum = 0;
    uint32_t taskRet =
        SchedulePostTaskBlocked(HadmUserGetSoundingAddrInfo, (void *)soundingAddrInfo, NULL, NLSTK_API_TIME_OUT);
    if (taskRet == NLSTK_ERRCODE_TASK_TIMEOUT) {
        SchedulePostTask(SDF_MemFree, (void *)soundingAddrInfo, NULL);
        NLSTK_LOG_ERROR("[Hadm] HadmGetSoundingState task time out, addr is %s", GET_ENC_ADDR(addr));
        return 0;
    } else if (taskRet != NLSTK_OK) {
        NLSTK_LOG_ERROR("[Hadm] HadmGetSoundingState task failed, addr is %s", GET_ENC_ADDR(addr));
        SDF_MemFree(soundingAddrInfo);
        return 0;
    }
    uint32_t soundingAddrNum = soundingAddrInfo->soundingAddrNum;
    if (addr == NULL || maxNum == 0) {
        SDF_MemFree(soundingAddrInfo);
        return soundingAddrNum;  // if addr is NULL or maxNum is 0, just return the number of sounding address
    }
    if (memcpy_s(addr, maxNum * sizeof(SLE_Addr_S), soundingAddrInfo->addr,
                 soundingAddrInfo->soundingAddrNum * sizeof(SLE_Addr_S)) != EOK) {
        NLSTK_LOG_ERROR("[Hadm] HadmGetSoundingAddrInfo memcpy fail");
    }
    SDF_MemFree(soundingAddrInfo);
    return soundingAddrNum;
}