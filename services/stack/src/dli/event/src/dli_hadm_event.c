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
#include "dli_hadm_event.h"
#include <stddef.h>
#include "dli_opcode.h"
#include "dli_log.h"
#include "dli_event.h"
#include "dli_cmd.h"
#include "dli_layer_callback.h"

#define SLE_IQ_MAX_CHNL_NUM 79
static uint32_t DLI_GetSlemInfoDataLen(DLI_CsIqReportEvt *evt);

void DLI_CsIqReportCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("read local cs caps cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(len >= sizeof(DLI_CsIqReportEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_CsIqReportEvt));
    DLI_CsIqReportEvt *ev = (DLI_CsIqReportEvt *)arg;
    uint32_t size = DLI_GetSlemInfoDataLen(ev);
    DLI_CHECK_RETURN(len >= sizeof(DLI_CsIqReportEvt) + size, "check len=%u, minDataLen=%u",
        len, sizeof(DLI_CsIqReportEvt) + size);
    DLI_RunRegCbk(DLI_CBK_MEASURE_IQ_REPORT, context, arg, len, evtOpcode, ev->status);
}

void DLI_ReadRemoteCsCapsCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("[RANGING-IQ] read remote cs caps v2 cbk in, evtOpcode: 0x%04X, len=%u.", evtOpcode, len);
    // arg 在回调入口处有做统一校验
    uint32_t baseLen = sizeof(DLI_ReadRemoteCsCapsPrivEvt);
    DLI_CHECK_RETURN(len >= baseLen, "check len=%u, minDataLen=%u", len, baseLen);

    DLI_ReadRemoteCsCapsPrivEvt *param = (DLI_ReadRemoteCsCapsPrivEvt *)arg;
    DLI_ReadCsCapsEvt caps = {0};
    caps.measureSignalCapabilitySupported = param->measureSignalCapabilitySupported;
    caps.multiAntennasSupported = param->multiAntennasSupported;
    caps.multiAntennasSwitchInterval = param->multiAntennasSwitchInterval;
    caps.type1MinTimeIp1 = param->type1MinTimeIp1;
    caps.type1MinTimeIp2 = param->type1MinTimeIp2;
    caps.type1MinTimeIp3 = param->type1MinTimeIp3;
    caps.type1MinTimeIp4 = param->type1MinTimeIp4;
    caps.type1MinTimeInterEvt = param->type1MinTimeInterEvt;
    caps.type2MinTimeInterEvt = param->type2MinTimeInterEvt;
    caps.minTimeInitializeInterEvt = param->minTimeInitializeInterEvt;
    caps.minTimeIntraEvt = param->type1MinTimeIntraEvt;
    caps.minTimeIntraEvtGroup = param->minTimeIntraEvtGroup;
    caps.phaseCaliOffsetCm = param->phaseCaliOffsetCm;
    caps.tofCaliOffsetM = param->tofCaliOffsetM;

    if (DLI_GetCallback()->hadmProcessCsCaps != NULL) {
        DLI_GetCallback()->hadmProcessCsCaps((const uint8_t *)arg, len, false);
    }

    DLI_ReadRemoteCsCapsEvt data = {0};
    if (DLI_IsSupportNewDisMeasure()) {
        data.status = param->status;
        data.connHandle = param->connHandle;
        memcpy_s(&data.caps, sizeof(DLI_ReadCsCapsEvt), &caps, sizeof(DLI_ReadCsCapsEvt));
    } else {
        if (len >= sizeof(DLI_ReadRemoteCsCapsEvt)) {
            memcpy_s(&data, sizeof(DLI_ReadRemoteCsCapsEvt), arg, sizeof(DLI_ReadRemoteCsCapsEvt));
        } else {
            DLI_LOGE("check len=%u, minDataLen=%u", len, sizeof(DLI_ReadRemoteCsCapsEvt));
            return;
        }
    }

    DLI_RunRegCbk(DLI_CBK_READ_REMOTE_MEASURE_CAPS,
        context,
        &data,
        (uint32_t)(sizeof(DLI_ReadRemoteCsCapsEvt)),
        evtOpcode,
        data.status);
}

void DLI_ReadLocalCsCapsCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("[RANGING-IQ] read local cs caps cbk in, evtOpcode: 0x%04X, len=%u.", evtOpcode, len);
    uint32_t baseLen = sizeof(DLI_ReadLocalCsCapsPrivEvt);
    DLI_CHECK_RETURN(len >= baseLen, "check len=%u, minDataLen=%u", len, baseLen);
    DLI_ReadLocalCsCapsPrivEvt *param = (DLI_ReadLocalCsCapsPrivEvt *)arg;
    DLI_ReadCsCapsEvt caps = {0};
    caps.measureSignalCapabilitySupported = param->measureSignalCapabilitySupported;
    caps.multiAntennasSupported = param->multiAntennasSupported;
    caps.multiAntennasSwitchInterval = param->multiAntennasSwitchInterval;
    caps.type1MinTimeIp1 = param->type1MinTimeIp1;
    caps.type1MinTimeIp2 = param->type1MinTimeIp2;
    caps.type1MinTimeIp3 = param->type1MinTimeIp3;
    caps.type1MinTimeIp4 = param->type1MinTimeIp4;
    caps.type1MinTimeInterEvt = param->type1MinTimeInterEvt;
    caps.type2MinTimeInterEvt = param->type2MinTimeInterEvt;
    caps.minTimeInitializeInterEvt = param->minTimeInitializeInterEvt;
    caps.minTimeIntraEvt = param->type1MinTimeIntraEvt;
    caps.minTimeIntraEvtGroup = param->minTimeIntraEvtGroup;
    caps.phaseCaliOffsetCm = param->phaseCaliOffsetCm;
    caps.tofCaliOffsetM = param->tofCaliOffsetM;

    if (DLI_GetCallback()->hadmProcessCsCaps != NULL) {
        DLI_GetCallback()->hadmProcessCsCaps((const uint8_t *)arg, len, true);
    }

    DLI_ReadLocalCsCapsEvt data = {0};
    memcpy_s(&data.caps, sizeof(DLI_ReadCsCapsEvt), &caps, sizeof(DLI_ReadCsCapsEvt));

    DLI_RunRegCbk(DLI_CBK_READ_LOCAL_MEASURE_CAPS,
        context,
        &data,
        (uint32_t)(sizeof(DLI_ReadLocalCsCapsEvt)),
        evtOpcode,
        param->status);
}

void DLI_MeasureStateChangeCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("measure state change in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(len >= sizeof(DLI_MeasureStateChangeEvt),
        "check len=%u, minDataLen=%zu",
        len,
        sizeof(DLI_MeasureStateChangeEvt));
    DLI_MeasureStateChangeEvt *param = (DLI_MeasureStateChangeEvt *)arg;
    DLI_LOGI("measure state change to %hhu, status %hhu, index %hhu.", param->measureState, param->status,
        param->posMeasureSigConfigIdx);
    DLI_RunRegCbk(DLI_CBK_MEASURE_STATE, context, arg, (uint32_t)(sizeof(DLI_MeasureStateChangeEvt)), evtOpcode,
        param->status);
}

static uint32_t DLI_GetSlemInfoDataLen(DLI_CsIqReportEvt *evt)
{
    uint32_t dataLen = 0;
    if (evt->slemInfoType.tof) {
        dataLen += sizeof(DLI_SlemTof);
    }
    if (evt->slemInfoType.chnlMeas) {
        dataLen += sizeof(DLI_SlemChnlMeas) + sizeof(DLI_SlemIqData) * SLE_IQ_MAX_CHNL_NUM;
    }
    if (evt->slemInfoType.vender) {
        dataLen += sizeof(DLI_SlemVender);
    }
    return dataLen;
}