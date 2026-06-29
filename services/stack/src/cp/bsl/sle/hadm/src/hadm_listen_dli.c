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
#include "securec.h"
#include "hadm_sm.h"
#include "dli.h"
#include "dli_errno.h"
#include "dli_event_struct.h"
#include "hadm_parser_iq.h"
#include "hadm_config_dli.h"
#include "hadm_listen_dli.h"

/*****************************************************************************************
                                Event report functions
*****************************************************************************************/

/**
 * @brief  上报远端参数
 * @return void
 */
static void EvtReportReadRemoteCsCaps(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    SDF_UNUSED(context);
    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL,
                              "[HADM] Event report read remote cs caps nullptr.");
    // 已对cmdRes->eventParameter和result判空，此处可以直接指针转换
    DLI_ReadRemoteCsCapsEvt *evt = (DLI_ReadRemoteCsCapsEvt *)cmdRes->eventParameter;
    NLSTK_LOG_INFO("[HADM] Report remote channel sounding caps, status = %u, lcid = %u.", status, evt->connHandle);
    HadmDliReportRemoteCsParam_S remoteCs = { 0 };
    remoteCs.status = NLSTK_ERRCODE_SUCCESS;
    if (status != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM] Event report read remote cs caps evt status %u", status);
        remoteCs.status = NLSTK_HADM_ERRCODE_CONFIG_DLI_FAIL;
    }
    if (memcpy_s(&(remoteCs.csCaps), sizeof(HadmRemoteCsParam_S), evt->caps, sizeof(HadmRemoteCsParam_S)) != EOK) {
        NLSTK_LOG_ERROR("[HADM] Event report read remote cs caps memcpy_s fail.");
        return;
    }
    NLSTK_LOG_INFO(
        "[HADM] Report remote channel sounding caps, lcid = %u, phaseCaliOffsetCm = %u, tofCaliOffsetM = %u.",
        evt->connHandle,
        remoteCs.csCaps.phaseCaliOffsetCm,
        remoteCs.csCaps.tofCaliOffsetM);
    HadmTriggerStateMachineByLcid(evt->connHandle, DLI_REPORT_REMOTE_MEASURE_EVENT, (void *)&remoteCs);  // 触发状态机
}

/**
 * @brief  上报CS参数设置结果
 * @return void
 */
static void EvtReportSetCsConfig(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    SDF_UNUSED(context);
    SDF_UNUSED(cmdRes);
    uint16_t expectRspTyp = DLI_CBK_MAX;
    uint16_t lcid = HadmPopLastDliCmd(&expectRspTyp);
    if (lcid == NLSTK_INVALID_LCID || expectRspTyp != DLI_CBK_SET_MEASURE_CONFIG_PARAM) {
        NLSTK_LOG_ERROR("[HADM]DLI response is unexpected in EvtReportSetCsConfig, expectRspTyp = %u, lcid = %u.",
                      expectRspTyp, lcid);
        return;
    }
    NLSTK_LOG_INFO("[HADM] Report remote set cs confg result, status = %u, lcid = %u.", status, lcid);
    uint16_t reportStatus = NLSTK_ERRCODE_SUCCESS;  // 上报状态，0表示成功，非0表示失败
    if (status != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM] Event report set cs config evt status %u", status);  // DLI配置失败
        reportStatus = NLSTK_HADM_ERRCODE_CONFIG_DLI_FAIL;
    }
    HadmTriggerStateMachineByLcid(lcid, DLI_REPORT_CONFIG_RESULT_EVENT, (void *)&reportStatus);  // 触发状态机
}

/**
 * @brief  上报CS使能设置结果
 * @return void
 */
static void EvtReportSetCsEnable(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    SDF_UNUSED(context);
    SDF_UNUSED(cmdRes);
    uint16_t expectRspTyp = DLI_CBK_MAX;
    uint16_t lcid = HadmPopLastDliCmd(&expectRspTyp);
    if (lcid == NLSTK_INVALID_LCID || expectRspTyp != DLI_CBK_SET_MEASURE_EN) {
        NLSTK_LOG_ERROR("[HADM]DLI response is unexpected in EvtReportSetCsEnable, expectRspTyp = %u, lcid = %u.",
                      expectRspTyp, lcid);
        return;
    }
    NLSTK_LOG_INFO("[HADM]DLI report set cs enable result, status = %u, lcid = %u.", status, lcid);
    uint16_t reportStatus = NLSTK_ERRCODE_SUCCESS;  // 上报状态，0表示成功，非0表示失败
    if (status != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM] Event report enable cs evt status %u", status);  // DLI配置失败
        reportStatus = NLSTK_HADM_ERRCODE_CONFIG_DLI_FAIL;
    }
    HadmTriggerStateMachineByLcid(lcid, DLI_REPORT_SOUNDING_RESULT_EVENT, (void *)&reportStatus);  // 触发状态机
}

/**
 * @brief  上报slem iq数据
 * @return void
 */
static void EvtReportSlemIQ(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    SDF_UNUSED(context);
    SDF_UNUSED(status);

    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL, "[HADM] Event report slem iq nullptr.");

    HadmIqInfoFromDli_S *iqInfo = HadmPaserIqInfoFromDli(cmdRes->eventParameter, cmdRes->size);
    NLSTK_CHECK_RETURN_VOID(iqInfo != NULL, "[HADM]iqInfo is NULL after parser the dli result.");
    if (iqInfo->status != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM] Event report slem iq status: %u.", iqInfo->status);
        HadmFreeIqInfo(iqInfo);
        return;
    }
    int8_t rssi = (int8_t)iqInfo->rssi;
    NLSTK_LOG_INFO("[HADM]DLI report iq infor, the iqChnlNum = %u, timeStampSn = %u, connHandle = %u, localId = %u, "
        "rssi = %d, tofResult = %u",
        iqInfo->iqChnlNum, iqInfo->timeStampSn, iqInfo->connHandle, iqInfo->localId, rssi, iqInfo->tofResult);
    if (((iqInfo->slemInfoType.chnlInfo24g != 0) && iqInfo->iqChnlNum < 1) ||
        iqInfo->iqChnlNum > (HADM_IQ_MAX_CHNL_NUM)) {
        NLSTK_LOG_ERROR("[HADM] Slem iq report iqChnlNum: %u error", iqInfo->iqChnlNum);
        HadmFreeIqInfo(iqInfo);
        iqInfo = NULL;
        return;
    }
    uint32_t ret = HadmReportSoundingIqInfoFromDli(iqInfo->connHandle, iqInfo);  // 上报IQ信息
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM] Report iq info to hal failed, connHandle = %u, ret = %u.", iqInfo->connHandle, ret);
        HadmFreeIqInfo(iqInfo);
        iqInfo = NULL;
        return;
    }
}

/**
 * @brief  上报当前测量状态
 * @return void
 */
static void EvtReportMeasureState(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    SDF_UNUSED(context);
    SDF_UNUSED(status);
    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL,
                              "[HADM] Event report measure state nullptr.");

    DLI_MeasureStateChangeEvt *evt = (DLI_MeasureStateChangeEvt *)cmdRes->eventParameter;
    HadmSoundingStateInfo_S soundingStateInfo = { 0 };
    soundingStateInfo.status = evt->status;
    soundingStateInfo.measureState = evt->measureState;
    soundingStateInfo.posMeasureSigConfigIdx = evt->posMeasureSigConfigIdx;
    HadmReportSoundingStateFromDli(&soundingStateInfo);  // 上报状态
    return;
}

static const DLI_CbkLineStru g_hadmDliCbkTable[] = {
    { DLI_CBK_READ_REMOTE_MEASURE_CAPS, (void *)EvtReportReadRemoteCsCaps },
    { DLI_CBK_SET_MEASURE_CONFIG_PARAM, (void *)EvtReportSetCsConfig },
    { DLI_CBK_SET_MEASURE_EN, (void *)EvtReportSetCsEnable },
    { DLI_CBK_MEASURE_IQ_REPORT, (void *)EvtReportSlemIQ },
    { DLI_CBK_MEASURE_STATE, (void *)EvtReportMeasureState },
};

uint32_t HadmRegDliCbk(void)
{
    uint32_t ret = DLI_CmdCbkReg(HADM, NULL, 0,
        g_hadmDliCbkTable, sizeof(g_hadmDliCbkTable) / sizeof(DLI_CbkLineStru));
    NLSTK_CHECK_RETURN(ret == DLI_SUCCESS, NLSTK_HADM_ERRCODE_CALL_DLI_FAIL,
                         "[HADM] Dli register callbacks failure with ret = %d.", ret);
    return NLSTK_ERRCODE_SUCCESS;
}

void HadmUnRegDliCbk(void)
{
    DLI_CmdCbkUnReg(HADM, NULL, 0, g_hadmDliCbkTable, sizeof(g_hadmDliCbkTable) / sizeof(DLI_CbkLineStru));
}
