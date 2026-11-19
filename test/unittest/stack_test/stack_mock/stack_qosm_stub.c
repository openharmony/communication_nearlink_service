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
#include <assert.h>
#include <stdio.h>

#include "stack_qosm_stub.h"

QOSM_AutoRateCallback g_qosmCbk;

uint32_t TEST_QOSM_AutoRateRegisterCallback(const QOSM_AutoRateCallback *callback)
{
    g_qosmCbk.bitrateChangedCbk = callback->bitrateChangedCbk;
    g_qosmCbk.connChangedCbk = callback->connChangedCbk;
    g_qosmCbk.dataPathChangedCbk = callback->dataPathChangedCbk;
    g_qosmCbk.frequencyBandChangedCbk = callback->frequencyBandChangedCbk;
    g_qosmCbk.highPowerModeChangedCbk = callback->highPowerModeChangedCbk;
    g_qosmCbk.paramChangedCbk = callback->paramChangedCbk;
    g_qosmCbk.callBitrateUpDownCbk = callback->callBitrateUpDownCbk;
    return 0;
}

uint32_t TEST_QOSM_AutoRateUnregisterCallback(void)
{
    g_qosmCbk.bitrateChangedCbk = NULL;
    g_qosmCbk.connChangedCbk = NULL;
    g_qosmCbk.dataPathChangedCbk = NULL;
    g_qosmCbk.frequencyBandChangedCbk = NULL;
    g_qosmCbk.highPowerModeChangedCbk = NULL;
    g_qosmCbk.paramChangedCbk = NULL;
    g_qosmCbk.callBitrateUpDownCbk = NULL;
    return 0;
}

uint32_t AUTORATE_QOSM_AutoRateSetAutorateParam(const QOSM_AutoRateParam *param)
{
    return 0;
}

uint32_t TEST_QOSM_AutoRateAddConnection(const QOSM_AutoRateConnParam *param)
{
    return 0;
}

uint32_t TEST_QOSM_AutoRateAddDataPath(const QOSM_AutoRateDataPath *param)
{
    return 0;
}

uint32_t TEST_QOSM_AutoRateDeleteDataPath(const QOSM_AutoRateDeletedDataPath *param)
{
    return 0;
}

uint32_t TEST_QOSM_AutoRateDeleteConnection(const QOSM_AutoRateConnParam *param)
{
    return 0;
}

uint32_t TEST_QOSM_AutoRateSetEarphoneFeedback(const QOSM_AutoRateEarphoneFeedbackParam *param)
{
    return 0;
}

void TEST_QOSM_NotifyParam(uint8_t icgId, uint8_t state, bool isImg, uint32_t result)
{
    uint16_t connHandle[2] = {0xa5, 0xa6};
    TEST_QOSM_NotifyParamEx(icgId, state, isImg, result, 2, connHandle, 0xa4);
}

void TEST_QOSM_NotifyParamEx(uint8_t icgId, uint8_t state, bool isImg, uint32_t result, uint8_t linkCnt,
    uint16_t *connHandle, uint16_t gHandle)
{
    QOSM_ParamCb param = {0};
    param.qosId = icgId;
    param.state = state;
    param.result = result;
    param.isIMG = isImg;
    param.gHandle = gHandle;
    param.linkCnt = linkCnt;
    param.connHandle = connHandle;
    if (g_qosmCbk.paramChangedCbk != NULL) {
        g_qosmCbk.paramChangedCbk(&param);
    }
}

void TEST_QOSM_NotifyConnection(uint8_t icgId, uint8_t state, uint32_t result)
{
    QOSM_ConnParamCb param = {0};
    param.qosId = icgId;
    param.state = state;
    param.result = result;
    param.isIMG = false;
    param.linkCnt = 2;
    QOSM_ConnParam link[2] = {{0xa5, 3}, {0xa6, 4}};
    param.link = link;
    if (g_qosmCbk.connChangedCbk != NULL) {
        g_qosmCbk.connChangedCbk(&param);
    }
}

void TEST_QOSM_NotifyDataPath(uint8_t icgId, uint8_t state, uint32_t result, uint8_t direction)
{
    QOSM_DataPathParamCb param = {0};
    param.qosId = icgId;
    param.state = state;
    param.result = result;
    param.direction = direction;
    if (g_qosmCbk.dataPathChangedCbk != NULL) {
        param.connHandle = 0xa5;
        g_qosmCbk.dataPathChangedCbk(&param);
        param.connHandle = 0xa6;
        g_qosmCbk.dataPathChangedCbk(&param);
    }
}

void TEST_QOSM_NotifyDataPathByHandle(uint16_t handle, uint8_t state, uint32_t result, uint8_t direction)
{
    QOSM_DataPathParamCb param = {0};
    param.state = state;
    param.result = result;
    param.direction = direction;
    param.connHandle = handle;
    param.qosId = 0xFF; // 非法组号必查不到链路，强制走组句柄分支
    if (g_qosmCbk.dataPathChangedCbk != NULL) {
        g_qosmCbk.dataPathChangedCbk(&param);
    }
}

void TEST_QOSM_NotifyBitrate(uint8_t icgId)
{
    QOSM_BitrateParamCb param = {0};
    param.qosId = icgId;
    if (g_qosmCbk.bitrateChangedCbk != NULL) {
        g_qosmCbk.bitrateChangedCbk(&param, 0);
    }
}

void TEST_QOSM_NotifyAutoRateMsg(const QOSM_AutoRateSendMsgCb *msg)
{
    if (g_qosmCbk.callBitrateUpDownCbk != NULL && msg != NULL) {
        g_qosmCbk.callBitrateUpDownCbk(msg);
    }
}