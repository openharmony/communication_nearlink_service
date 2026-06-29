/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "cm_icb_api.h"
#include <stdint.h>
#include <stddef.h>
#include "cm_errno.h"
#include "cm_icb_init.h"
#include "cm_icb_inner_api.h"
#include "cm_icb_mgr.h"
#include "cm_log.h"
#include "cp_worker.h"
#include "dli_cmd.h"
#include "dli_errno.h"
#include "sdf_mem.h"
#include "securec.h"
#include "nlstk_public_define.h"
#include "dli_reg_ext_func.h"
#include "dli_cmd_struct.h"
#include "qosm_icg_types.h"

#define MAX_ICG_ID (0xEF)

uint32_t CM_ICBRegisterCbk(const CM_ICBCallback *cb)
{
    CM_CHECK_RETURN_RET(CM_ICBIsInited(), CM_NOT_INITED, "icb mgr is not inited");
    CM_CHECK_RETURN_RET(cb != NULL, CM_INVALID_PARAM_ERR, "cb is null");

    return CM_ICBMgrRegisterCb(cb);
}

uint32_t CM_ICBUnregisterCbk(void)
{
    CM_CHECK_RETURN_RET(CM_ICBIsInited(), CM_NOT_INITED, "icb mgr is not inited");
    return CM_ICBMgrUnregisterCb();
}

static bool CM_IsValidICBParam(const CM_ICBParam *icbParam, uint8_t icbCnt, uint8_t paramCnt)
{
    uint16_t maxSduG2T = icbParam[0].param[0].maxSduG2T;
    uint16_t maxSduT2G = icbParam[0].param[0].maxSduT2G;
    uint8_t rtnG2T = icbParam[0].param[0].rtnG2T;
    uint8_t rtnT2G = icbParam[0].param[0].rtnT2G;
    for (uint8_t i = 0; i < icbCnt; i++) {
        for (uint8_t j = 0; j < paramCnt; j++) {
            if (maxSduG2T != icbParam[i].param[j].maxSduG2T ||
            maxSduT2G != icbParam[i].param[j].maxSduT2G ||
            rtnG2T != icbParam[i].param[j].rtnG2T ||
            rtnT2G != icbParam[i].param[j].rtnT2G) {
                CM_LOGE("icg param is not same");
                return false;
            }
        }
    }
    return true;
}

uint32_t CM_ICGSetParam(CM_ICGParam *icgParam)
{
    CM_CHECK_RETURN_RET(CM_ICBIsInited(), CM_NOT_INITED, "icb mgr is not inited");
    CM_CHECK_RETURN_RET(icgParam != NULL && icgParam->icbParam != NULL && icgParam->icbParam->param != NULL,
        CM_INVALID_PARAM_ERR, "icgParam is null");
    CM_CHECK_RETURN_RET(icgParam->id <= MAX_ICG_ID, CM_INVALID_PARAM_ERR, "icg id is invalid");
    CM_CHECK_RETURN_RET(icgParam->icbCnt != 0, CM_INVALID_PARAM_ERR, "icb count is 0");
    CM_CHECK_RETURN_RET(icgParam->icbCnt <= CM_MAX_CHANNEL_COUNT, CM_INVALID_PARAM_ERR, "icb count is over max");
    CM_CHECK_RETURN_RET(icgParam->paramCnt != 0, CM_INVALID_PARAM_ERR, "param count per icb is 0");
    CM_CHECK_RETURN_RET(CM_IsValidICBParam(icgParam->icbParam, icgParam->icbCnt, icgParam->paramCnt),
        CM_INVALID_PARAM_ERR, "icb g2t param is not same");

    DLI_ICGParam param = {};
    param.type = icgParam->type;
    param.opCode = (icgParam->type == CM_IMB) ? DLI_SET_IMG_PARAM : DLI_SET_IOG_PARAM;
    param.id = icgParam->id;
    param.sduIntervalG2T = icgParam->sduIntervalG2T;
    param.sduIntervalT2G = icgParam->sduIntervalT2G;
    param.sca = icgParam->sca;
    param.packing = icgParam->packing;
    param.framing = icgParam->framing;
    param.maxLatencyG2T = icgParam->maxLatencyG2T;
    param.maxLatencyT2G = icgParam->maxLatencyT2G;
    param.icbCnt = icgParam->icbCnt;
    param.paramCnt = icgParam->paramCnt;
    param.icbParam = (DLI_ICBParam *)icgParam->icbParam; // DLI_ICBParam与CM_ICBParam定义相同，所以可以强转
    uint32_t ret = CM_ICBMgrSetParam(&param);
    if (ret != CM_ICB_SUCCESS) {
        CM_LOGE("CM_ICBMgrSetParam failed, ret:%8x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_ICGSetTestParam(CM_ICGTestParam *icgParam, bool supportAutorate)
{
    CM_CHECK_RETURN_RET(CM_ICBIsInited(), CM_NOT_INITED, "icb mgr is not inited");
    CM_CHECK_RETURN_RET(icgParam != NULL && icgParam->icbParam != NULL, CM_INVALID_PARAM_ERR, "icgParam is null");
    CM_CHECK_RETURN_RET(icgParam->id <= MAX_ICG_ID, CM_INVALID_PARAM_ERR, "icg id is invalid");
    CM_CHECK_RETURN_RET(icgParam->icbCnt != 0, CM_INVALID_PARAM_ERR, "param count is 0");
    CM_CHECK_RETURN_RET(icgParam->icbCnt <= CM_MAX_CHANNEL_COUNT, CM_INVALID_PARAM_ERR, "param count is over max");

    DLI_ICGTestParam param = {};
    param.type = icgParam->type;
    param.opCode = (icgParam->type == CM_IMB) ? DLI_SET_IMG_PARAM_TEST : DLI_SET_IOG_PARAM_TEST;
    param.id = icgParam->id;
    param.labelId = icgParam->labelId;
    param.sduIntervalG2T = icgParam->sduIntervalG2T;
    param.sduIntervalT2G = icgParam->sduIntervalT2G;
    param.ftG2T = icgParam->ftG2T;
    param.ftT2G = icgParam->ftT2G;
    param.icbInterval = icgParam->icbInterval;
    param.sca = icgParam->sca;
    param.packing = icgParam->packing;
    param.framing = icgParam->framing;
    param.paramCnt = icgParam->icbCnt;
    param.icbParam = (DLI_ICBTestParam *)icgParam->icbParam; // DLI_ICBTestParam与CM_ICBTestParam定义相同，所以可以强转
    CM_LOGI("type: %u, opcode: 0x%04x, id: %u, label id: %u, sdu interval g2t: %u,"
        " sdu interval t2g: %u, ft g2t: %u, ft t2g: %d, icb interval: %u, sca: %u, packing: %u,"
        " framing: %u, param count: %u",
        param.type, param.opCode, param.id, param.labelId, param.sduIntervalG2T,
        param.sduIntervalT2G, param.ftG2T, param.ftT2G, param.icbInterval, param.sca, param.packing,
        param.framing, param.paramCnt);
    uint32_t ret = CM_ICBMgrSetTestParam(&param, icgParam->type == CM_IMB, supportAutorate);
    if (ret != CM_ICB_SUCCESS) {
        CM_LOGE("CM_ICBMgrSetTestParam failed, ret:%8x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_ICGRemoveParam(CM_ICGRemovedParam *icgParam)
{
    CM_CHECK_RETURN_RET(CM_ICBIsInited(), CM_NOT_INITED, "icb mgr is not inited");
    CM_CHECK_RETURN_RET(icgParam != NULL, CM_INVALID_PARAM_ERR, "icgParam is null");
    CM_CHECK_RETURN_RET(icgParam->id <= MAX_ICG_ID, CM_INVALID_PARAM_ERR, "id is invalid");

    uint32_t ret = CM_ICBMgrRemoveParam(icgParam);
    if (ret != CM_ICB_SUCCESS) {
        CM_LOGE("CM_ICBMgrRemoveParam failed, ret:%8x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_ICGSetLabel(CM_ICGLabelParam *icgLabel, bool supportSubrate, bool supportAutorate)
{
    CM_CHECK_RETURN_RET(CM_ICBIsInited(), CM_NOT_INITED, "icb mgr is not inited");
    CM_CHECK_RETURN_RET(icgLabel != NULL && icgLabel->icb != NULL, CM_INVALID_PARAM_ERR, "icgLabel is null");
    CM_CHECK_RETURN_RET(icgLabel->id <= MAX_ICG_ID, CM_INVALID_PARAM_ERR, "id is invalid");
    CM_CHECK_RETURN_RET(icgLabel->icbCnt != 0, CM_INVALID_PARAM_ERR, "icb count is 0");
    CM_CHECK_RETURN_RET(icgLabel->icbCnt <= CM_MAX_CHANNEL_COUNT, CM_INVALID_PARAM_ERR, "icb count is over max");

    DLI_ICGLabelParam param = {};
    param.type = icgLabel->type;
    param.id = icgLabel->id;
    param.icbCnt = icgLabel->icbCnt;
    DLI_ICBChannel icb[CM_MAX_CHANNEL_COUNT];
    param.icb = icb;
    CM_LOGI("set label, type: %u, opcode: 0x%04x, id: %u, icb count: %u",
        param.type, param.opCode, param.id, param.icbCnt);
    for (uint8_t i = 0; i < icgLabel->icbCnt; i++) {
        icb[i].connHandle = icgLabel->icb[i].connHandle;
        icb[i].lcid = icgLabel->icb[i].lcid;
        CM_LOGI("[%u] lcid: 0x%04x, icb conn handler: 0x%04x", i, icb[i].lcid, icb[i].connHandle);
    }
    uint32_t ret = CM_ICGMgrSetLabel(&param, icgLabel->type == CM_IMB, supportSubrate, supportAutorate);
    if (ret != CM_ICB_SUCCESS) {
        CM_LOGE("CM_ICGMgrSetLabel failed, ret:%8x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_ICBAddConnection(CM_ICBConnectionParam *connParam, bool supportAutorate)
{
    CM_CHECK_RETURN_RET(CM_ICBIsInited(), CM_NOT_INITED, "icb mgr is not inited");
    CM_CHECK_RETURN_RET(connParam != NULL && connParam->channel != NULL, CM_INVALID_PARAM_ERR, "connParam is null");
    CM_CHECK_RETURN_RET(connParam->id <= MAX_ICG_ID, CM_INVALID_PARAM_ERR, "id is invalid");
    CM_CHECK_RETURN_RET(connParam->channelCnt != 0, CM_INVALID_PARAM_ERR, "param count is 0");
    CM_CHECK_RETURN_RET(connParam->channelCnt <= CM_MAX_CHANNEL_COUNT, CM_INVALID_PARAM_ERR, "param count is over max");

    DLI_ICBConnectionParam param = {};
    param.opCode = (connParam->type == CM_IMB) ? DLI_CREATE_IMB : DLI_CREATE_IOB;
    param.type = connParam->type;
    param.id = connParam->id;
    param.labelId = connParam->labelId;
    param.channelCnt = connParam->channelCnt;
    DLI_ICBChannel channel[CM_MAX_CHANNEL_COUNT];
    param.channel = channel;
    for (uint8_t i = 0; i < connParam->channelCnt; i++) {
        channel[i].connHandle = connParam->channel[i].connHandle;
        channel[i].lcid = connParam->channel[i].lcid;
    }
    uint32_t ret = CM_ICBMgrAddConnection(&param, connParam->type == CM_IMB, supportAutorate);
    if (ret != CM_ICB_SUCCESS) {
        CM_LOGE("CM_ICBMgrAddConnection failed, ret:%8x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_ICBDelConnection(CM_ICBConnectionParam *connParam)
{
    CM_CHECK_RETURN_RET(CM_ICBIsInited(), CM_NOT_INITED, "icb mgr is not inited");
    CM_CHECK_RETURN_RET(connParam != NULL && connParam->channel != NULL, CM_INVALID_PARAM_ERR, "connParam is null");
    CM_CHECK_RETURN_RET(connParam->id <= MAX_ICG_ID, CM_INVALID_PARAM_ERR, "icg id is invalid");
    CM_CHECK_RETURN_RET(connParam->channelCnt != 0, CM_INVALID_PARAM_ERR, "param count is 0");
    CM_CHECK_RETURN_RET(connParam->channelCnt <= CM_MAX_CHANNEL_COUNT, CM_INVALID_PARAM_ERR, "param count is over max");

    DLI_ICBConnectionParam param = {};
    param.type = connParam->type;
    param.id = connParam->id;
    param.labelId = connParam->labelId;
    param.channelCnt = connParam->channelCnt;
    DLI_ICBChannel channel[CM_MAX_CHANNEL_COUNT];
    param.channel = channel;
    for (uint8_t i = 0; i < connParam->channelCnt; i++) {
        channel[i].connHandle = connParam->channel[i].connHandle;
        channel[i].lcid = connParam->channel[i].lcid;
    }
    uint32_t ret = CM_ICBMgrDelConnection(&param);
    if (ret != CM_ICB_SUCCESS) {
        CM_LOGE("CM_ICBMgrDelConnection failed, ret:%8x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_ICGUpdateParam(CM_ICGUpdatedParam *icgParam)
{
    CM_CHECK_RETURN_RET(CM_ICBIsInited(), CM_NOT_INITED, "icb mgr is not inited");
    CM_CHECK_RETURN_RET(icgParam != NULL && icgParam->connHandle != NULL, CM_INVALID_PARAM_ERR, "connParam is null");
    CM_CHECK_RETURN_RET(icgParam->id <= MAX_ICG_ID, CM_INVALID_PARAM_ERR, "id is invalid");
    CM_CHECK_RETURN_RET(icgParam->icbCnt != 0, CM_INVALID_PARAM_ERR, "param count is 0");
    CM_CHECK_RETURN_RET(icgParam->icbCnt <= CM_MAX_CHANNEL_COUNT, CM_INVALID_PARAM_ERR, "param count is over max");

    DLI_ICGUpdatedParam param = {};
    param.connHandle = (uint16_t *)SDF_MemZalloc(icgParam->icbCnt * sizeof(uint16_t));
    CM_CHECK_RETURN_RET(param.connHandle != NULL, CM_MEM_ERR, "mem zalloc param channel param failed");

    param.type = icgParam->type;
    param.id = icgParam->id;
    param.labelId = icgParam->labelId;
    param.icbCnt = icgParam->icbCnt;
    for (uint8_t i = 0; i < icgParam->icbCnt; i++) {
        param.connHandle[i] = icgParam->connHandle[i];
    }
    uint32_t ret = CM_ICGMgrUpdateParam(&param, icgParam->type == CM_IMB);
    SDF_MemFree(param.connHandle);
    if (ret != CM_ICB_SUCCESS) {
        CM_LOGE("CM_ICGMgrUpdateParam failed, ret:%8x", ret);
        return CM_FAIL;
    }

    CM_LOGI("send update param cmd success");
    return CM_SUCCESS;
}

uint32_t CM_ICBSetupDataPath(CM_ICBDataPath *dataPath)
{
    CM_CHECK_RETURN_RET(CM_ICBIsInited(), CM_NOT_INITED, "icb mgr is not inited");
    CM_CHECK_RETURN_RET(dataPath != NULL && dataPath->codecConfigData != NULL, CM_INVALID_PARAM_ERR,
        "dataPath or codecConfigData is null");
    CM_CHECK_RETURN_RET(dataPath->codecConfigLen != 0, CM_INVALID_PARAM_ERR, "codecConfigLen is 0");
    CM_CHECK_RETURN_RET(dataPath->direction == 0 || dataPath->direction == 1, CM_INVALID_PARAM_ERR,
        "direction should be 0 or 1");

    DLI_SetupICBDataPathParam param = {};
    param.connHandle = dataPath->connHandle;
    param.direction = dataPath->direction;
    param.pathId = dataPath->pathId;
    param.codec.codecId = dataPath->codec.codecId;
    param.codec.vendorId = dataPath->codec.vendorId;
    param.codec.vendorCodecId = dataPath->codec.vendorCodecId;
    param.codecConfigLen = dataPath->codecConfigLen;
    param.codecConfigData = dataPath->codecConfigData;
    param.controllerDelay = dataPath->controllerDelay;
    uint32_t ret = CM_ICBMgrSetupDataPath(&param);
    if (ret != CM_ICB_SUCCESS) {
        CM_LOGE("CM_ICBMgrSetupDataPath failed, ret:%8x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_ICBRemoveDataPath(CM_ICBRemovedDataPath *dataPath)
{
    CM_CHECK_RETURN_RET(CM_ICBIsInited(), CM_NOT_INITED, "icb mgr is not inited");
    CM_CHECK_RETURN_RET(dataPath != NULL, CM_INVALID_PARAM_ERR, "dataPath is null");
    CM_CHECK_RETURN_RET(dataPath->direction == 0 || dataPath->direction == 1, CM_INVALID_PARAM_ERR,
        "direction should be 0 or 1");
    DLI_RemoveICBDataPathParam param = {};
    param.connHandle = dataPath->connHandle;
    param.direction = dataPath->direction;
    uint32_t ret = CM_ICBMgrRemoveDataPath(&param);
    if (ret != CM_ICB_SUCCESS) {
        CM_LOGE("CM_ICBMgrRemoveDataPath failed, ret:%8x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

static void CM_ListenFreqBandSwitchEventInner(void *param)
{
    CM_FreqBandListener *listener = (CM_FreqBandListener *)param;
    CM_ICBMgrListenFreqBandSwitchEvent(*listener);
}

uint32_t CM_ListenFreqBandSwitchEvent(CM_FreqBandListener listener)
{
    CM_CHECK_RETURN_RET(listener != NULL, CM_INVALID_PARAM_ERR, "listener is null");

    CM_FreqBandListener *param = (CM_FreqBandListener *)SDF_MemZalloc(sizeof(CM_FreqBandListener));
    CM_CHECK_RETURN_RET(param != NULL, CM_MEM_ERR, "mem zalloc error");
    *param = listener;

    uint32_t ret = CP_PostTask(CM_ListenFreqBandSwitchEventInner, (void *)param, SDF_MemFree);
    if (ret != NLSTK_OK) {
        CM_LOGE("CM_ListenFreqBandSwitchEventInner failed, ret:%8x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

static void CM_UnlistenFreqBandSwitchEventInner(void *param)
{
    CM_FreqBandListener *listener = (CM_FreqBandListener *)param;
    CM_ICBMgrUnlistenFreqBandSwitchEvent(*listener);
}

uint32_t CM_UnlistenFreqBandSwitchEvent(CM_FreqBandListener listener)
{
    CM_CHECK_RETURN_RET(listener != NULL, CM_INVALID_PARAM_ERR, "listener is null");

    CM_FreqBandListener *param = (CM_FreqBandListener *)SDF_MemZalloc(sizeof(CM_FreqBandListener));
    CM_CHECK_RETURN_RET(param != NULL, CM_MEM_ERR, "mem zalloc error");
    *param = listener;

    uint32_t ret = CP_PostTask(CM_UnlistenFreqBandSwitchEventInner, (void *)param, SDF_MemFree);
    if (ret != NLSTK_OK) {
        CM_LOGE("CM_UnlistenFreqBandSwitchEventInner failed, ret:%8x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}
