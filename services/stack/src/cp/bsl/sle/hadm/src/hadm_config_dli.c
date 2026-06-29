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
#include "sdf_vector.h"
#include "sdf_mem.h"
#include "securec.h"
#include "hadm_dft.h"
#include "dli_errno.h"
#include "dli_cmd.h"
#include "dli_def.h"
#include "dli_reg_ext_func.h"
#include "hadm_config_dli.h"
#include "hadm_ext_func_wrapper.h"
#include "dli_reg_ext_func.h"

typedef struct {
    uint16_t lcid;
    uint16_t expectRspTyp;
} HadmDliCmd_S;

// 协议栈针对下发到DLI的"设置窄带跳频测量链路参数(DLI_CBK_SET_MEASURE_CONFIG_PARAM)"和"测量行为指示(DLI_CBK_SET_MEASURE_EN)"要做队列缓存
// 因为目前DLI在收到这两种类型的指令，执行上报的结果中不会携带链路信息，因此需要在协议栈记录一下下发指令的链路信息；
// DLI保证所有的执行采用顺序执行的逻辑；
SDF_Vector_S *g_hadmDliCmdVec = NULL;

uint32_t HadmInitDliCmdVec(void)
{
    SDF_Traits linkCbTraits = { .dtor = SDF_MemFree };
    g_hadmDliCmdVec = SDF_CreateVector(linkCbTraits);
    if (g_hadmDliCmdVec == NULL) {
        NLSTK_LOG_ERROR("[SSAP] init DliCmd vector failed");
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

void HadmDeInitDliCmdVec(void)
{
    if (g_hadmDliCmdVec != NULL) {
        SDF_DestroyVector(g_hadmDliCmdVec);
        g_hadmDliCmdVec = NULL;
    }
    return;
}

static uint32_t HadmPushDliCmd(uint16_t lcid, uint16_t expectRspTyp)
{
    NLSTK_LOG_INFO("[hadm]HadmPushDliCmd lcid:%u expectRspTyp:%u", lcid, expectRspTyp);
    HadmDliCmd_S *dliCmd = (HadmDliCmd_S *)SDF_MemZalloc(sizeof(HadmDliCmd_S));
    if (dliCmd == NULL) {
        NLSTK_LOG_ERROR("[hadm] alloc dli cmd fail");
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    dliCmd->lcid = lcid;
    dliCmd->expectRspTyp = expectRspTyp;
    if (!SDF_VectorEmplaceBack(g_hadmDliCmdVec, dliCmd)) {
        NLSTK_LOG_ERROR("[hadm] push dli cmd into vector fail");
        SDF_MemFree(dliCmd);
        return NLSTK_ERRCODE_SYS_ERROR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint16_t HadmPopLastDliCmd(uint16_t *expectRspType)
{
    if (g_hadmDliCmdVec->size == 0) {
        NLSTK_LOG_ERROR("[HADM] DliCmdVec is empty.");
        return NLSTK_INVALID_LCID;
    }
    HadmDliCmd_S *dliCmd = (HadmDliCmd_S *)SDF_VectorElementAt(g_hadmDliCmdVec, 0);  // 先进先出，因此取第一个元素
    if (dliCmd == NULL) {
        NLSTK_LOG_ERROR("[HADM]dli cmd is null point.");
        return NLSTK_INVALID_LCID;
    }
    uint16_t lcid = dliCmd->lcid;
    *expectRspType = dliCmd->expectRspTyp;
    NLSTK_LOG_INFO("[hadm]HadmPopLastDliCmd lcid:%u expectRspTyp:%u", lcid, *expectRspType);
    SDF_VectorRemove(g_hadmDliCmdVec, 0);  // pop_front
    return lcid;
}

static void BuildMeasureParam(uint16_t lcid, HadmSoundingParam_S *args, DLI_SetMeasureConfigParam *params)
{
    uint8_t pmInitSignal2Tone = args->pmInitSignal2Tone;
    uint8_t pmReflSignal2Tone = args->pmReflSignal2Tone;
    HADM_ExtCheckAndUpdateMultiToneConfig(lcid, &pmInitSignal2Tone, &pmReflSignal2Tone);
    params->connHandle = lcid;
    params->configId = args->configId;
    params->measureConfigDirect = MEASURE_CONFIG_DIRECT;
    params->occurrenceGroupPeriod = args->occurrenceGroupPeriod;
    params->schedulingTimeslot = args->schedulingTimeslot;
    params->rttPhy = args->rttPhy;
    params->freqHoppingMode = args->freqHoppingMode;
    params->fmFreq = args->fmFreq;
    params->sendDirect = TX_ORDER_SECOND;
    params->antennaOrderConfig = 0;
    params->firstAntennaTypeConfig = 0;
    params->secondAntennaTypeConfig = 0;
    params->eventsCount = 0;
    params->bitWidth = 0;
    params->pmInitAntCount = args->pmInitAntCount;
    params->pmInitSignal2Tone = pmInitSignal2Tone;
    params->firstNodeInterval = 0;
    params->pmReflAntCount = args->pmReflAntCount;
    params->pmReflSignal2Tone = pmReflSignal2Tone;
    params->secondNodeInterval = 0;
    params->channelBandwidth = FREQUENCY_BAND_2_4GHZ;
    (void)memcpy_s(params->pm2400mBand, SLE_MEASURE_PM_24G_BAND_LEN, args->pm2400mBand, HADM_MEASURE_PM_BAND_24G_LEN);
}

uint32_t HadmSetMeasureParam(uint16_t lcid, HadmSoundingParam_S *args)
{
    NLSTK_CHECK_RETURN(args != NULL, NLSTK_ERRCODE_POINTER_NULL, "[HADM] Set channel sounding params args nullptr.");
    DLI_SetMeasureConfigParam params = { 0 };
    BuildMeasureParam(lcid, args, &params);
    uint32_t ret = HadmPushDliCmd(lcid, DLI_CBK_SET_MEASURE_CONFIG_PARAM);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        return ret;
    }
    if (DLI_IsSupportNewDisMeasure()) {
        NLSTK_LOG_INFO("[HADM] Start to set measure params, conn id: %u.", params.connHandle);
        ret = DLI_SetMeasureParam(&params);
    } else {
        if (DLI_GetExtFuncList()->setMeasureParamExt != NULL) {
            NLSTK_LOG_INFO("[HADM] Start to set measure params ext, conn id: %u.", params.connHandle);
            DLI_MeasureConfigExtParam cmd = {0};
            cmd.connHandle = lcid;
            (void)memcpy_s(&cmd.configId, sizeof(HadmSoundingParam_S), args, sizeof(HadmSoundingParam_S));
            ret = DLI_GetExtFuncList()->setMeasureParamExt(&cmd);
        }
    }
    if (ret != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM] Set measure params post dli task fail. %u", ret);
        SDF_VectorRemoveLast(g_hadmDliCmdVec);
        HadmDftReport((uint16_t)ret);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}


uint32_t SetMeasureEnable(DLI_SetMeasureEnableParam *param)
{
    if (DLI_IsSupportNewDisMeasure()) {
        return DLI_SetMeasureEnable(param);
    } else {
        if (DLI_GetExtFuncList()->setMeasureEnableExt != NULL) {
            return DLI_GetExtFuncList()->setMeasureEnableExt(param);
        }
    }
    return NLSTK_ERRCODE_FAIL;
}

uint32_t HadmSetMeasureEnable(uint16_t lcid, uint8_t csEnable)
{
    uint32_t ret = HadmPushDliCmd(lcid, DLI_CBK_SET_MEASURE_EN);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        return ret;
    }

    DLI_SetMeasureEnableParam params = { 0 };
    params.connHandle = lcid;
    params.enable = csEnable;
    HadmDftCacheTimestamp(NLSTK_DFT_EVENT_HADM_EXCEP, HADM_DFT_ENABLE_TIME);
    if (DLI_IsSupportNewDisMeasure()) {
        NLSTK_LOG_INFO(
            "[HADM] Start to set measure enable, conn id: %u, enable: %u.", params.connHandle, params.enable);
        ret = SetMeasureEnable(&params);
    } else {
        if (DLI_GetExtFuncList()->setMeasureEnableExt != NULL) {
            NLSTK_LOG_INFO("[HADM] Start to set measure enable ext, conn id: %u, enable: %u.",
                params.connHandle,
                params.enable);
            ret = DLI_GetExtFuncList()->setMeasureEnableExt(&params);
        }
    }
    if (ret != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM] Set measure enable post dli task fail. ret: %u", ret);
        SDF_VectorRemoveLast(g_hadmDliCmdVec);  // pop_back
        HadmDftReport((uint16_t)ret);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static uint32_t ReadRemoteMeasureCaps(DLI_ReadRemoteMeasureCapsParam *param)
{
    if (DLI_IsSupportNewDisMeasure()) {
        return DLI_ReadRemoteMeasureCaps(param);
    } else {
        if (DLI_GetExtFuncList()->readRemoteMeasureCapsExt != NULL) {
            return DLI_GetExtFuncList()->readRemoteMeasureCapsExt(param);
        }
    }
    return NLSTK_ERRCODE_FAIL;
}

uint32_t HadmReadRemoteMeasureCaps(uint16_t lcid)
{
    DLI_ReadRemoteMeasureCapsParam params = { 0 };
    params.connHandle = lcid;
    HadmDftCacheTimestamp(NLSTK_DFT_EVENT_HADM_EXCEP, HADM_DFT_READ_REMOTE_MEASURE_TIME);
    uint32_t ret = DLI_SUCCESS;
    if (DLI_IsSupportNewDisMeasure()) {
        NLSTK_LOG_INFO("[HADM] Start to read remote measure caps, conn id: %u.", params.connHandle);
        ret = ReadRemoteMeasureCaps(&params);
    } else {
        if (DLI_GetExtFuncList()->readRemoteMeasureCapsExt != NULL) {
            NLSTK_LOG_INFO("[HADM] Start to read remote measure caps ext, conn id: %u.", params.connHandle);
            ret = DLI_GetExtFuncList()->readRemoteMeasureCapsExt(&params);
        }
    }
    if (ret != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM] Read remote measure caps post dli task fail, ret: %u", ret);
        HadmDftReport((uint16_t)ret);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}