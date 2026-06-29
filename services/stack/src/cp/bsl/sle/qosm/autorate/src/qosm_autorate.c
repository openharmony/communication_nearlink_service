/****************************************************************************
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
****************************************************************************/

#include "qosm_autorate.h"
#include "cp_worker.h"
#include "dli_cmd.h"
#include "qosm_errno.h"
#include "qosm_icg_mgr.h"
#include "qosm_log.h"
#include "qosm_table_mgr.h"
#include "sdf_mem.h"
#include "securec.h"
#include "dli_errno.h"
#include "qosm_icg_callback.h"
#include "qosm_autorate_notify.h"

uint32_t QOSM_AutoRateRegisterCallback(const QOSM_AutoRateCallback *callback)
{
    QOSM_CHECK_RETURN_RET(callback != NULL && callback->paramChangedCbk && callback->connChangedCbk != NULL &&
        callback->dataPathChangedCbk != NULL && callback->bitrateChangedCbk != NULL &&
        callback->callBitrateUpDownCbk != NULL, QOSM_NULL_PTR_ERR,
        "callback is null");

    uint32_t ret = QOSM_ICGMgrRegisterCallback(callback);
    if (ret != QOSM_SUCCESS) {
        QOSM_LOGE("register cbk failed, ret:%8x", ret);
        return ret;
    }
    return QOSM_SUCCESS;
}

uint32_t QOSM_AutoRateUnregisterCallback(void)
{
    uint32_t ret = QOSM_ICGMgrUnregisterCallback();
    if (ret != QOSM_SUCCESS) {
        QOSM_LOGE("unregister conn changed cbk failed, ret:%8x", ret);
    }
    return ret;
}

static uint32_t QOSM_AutoRateSetParamInner(const QOSM_AutoRateParam *param, void (*cpTask)(void *))
{
    QOSM_CHECK_RETURN_RET(param != NULL, QOSM_NULL_PTR_ERR, "param is null");
    QOSM_CHECK_RETURN_RET(param->linkCnt != 0 && param->linkCnt <= QOSM_AUTORATE_MAX_LINK_CNT,
        QOSM_INVALID_COUNT_ERR, "invalid link cnt");
    QOSM_CHECK_RETURN_RET(param->supportedBitrateCnt != 0 &&
        param->supportedBitrateCnt <= QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT,
        QOSM_INVALID_COUNT_ERR, "invalid supported bitrate cnt");

    QOSM_LOGI("set param, qos id: %hhu, qos index: %d, link count: %d",
        param->qosId, param->qosIndex, param->linkCnt);

    QOSM_StartParam *startParam = QOSM_GetStartParamByIndex(param->qosIndex, param->bitrate, param->linkCnt);
    QOSM_CHECK_RETURN_RET(startParam != NULL, QOSM_NOT_FOUND_ERR, "find start param failed, qosIndex:%u",
        param->qosIndex);
    QOSM_LOGI("get start param, level: %d, band: %d, duty cycle: %d, level cnt: %u",
        startParam->startLevel, startParam->startBand, QOSM_GetDutyCycle(), startParam->levelCnt);

    QOSM_ICGMgrParam *icgParam = (QOSM_ICGMgrParam *)SDF_MemZalloc(sizeof(QOSM_ICGMgrParam));
    QOSM_CHECK_RETURN_RET(icgParam != NULL, QOSM_MALLOC_ERR, "malloc auto rate param failed");

    if (memcpy_s(&icgParam->autorateParam, sizeof(QOSM_AutoRateParam), param, sizeof(QOSM_AutoRateParam)) != EOK) {
        SDF_MemFree(icgParam);
        QOSM_LOGE("copy auto rate param failed");
        return QOSM_MEMCPY_ERR;
    }
    if (memcpy_s(&icgParam->startParam, sizeof(QOSM_StartParam), startParam, sizeof(QOSM_StartParam)) != EOK) {
        SDF_MemFree(icgParam);
        QOSM_LOGE("copy start param failed");
        return QOSM_MEMCPY_ERR;
    }

    uint32_t ret = CP_PostTask(cpTask, (void *)icgParam, SDF_MemFree);
    if (ret != CP_OK) {
        QOSM_LOGE("cp post failed, ret:%8x", ret);
        return QOSM_POST_TASK_ERR;
    }
    return QOSM_SUCCESS;
}

uint32_t QOSM_AutoRateSetParam(const QOSM_AutoRateParam *param)
{
    return QOSM_AutoRateSetParamInner(param, QOSM_ICGMgrSetParam);
}

uint32_t QOSM_AutoRateSetTestParam(const QOSM_AutoRateParam *param)
{
    return QOSM_AutoRateSetParamInner(param, QOSM_ICGMgrSetTestParam);
}

uint32_t QOSM_AutoRateRemoveParam(uint8_t qosId)
{
    uint32_t ret = CP_PostTask(QOSM_ICGMgrRemoveParam, (void *)(uintptr_t)qosId, NULL);
    if (ret != CP_OK) {
        QOSM_LOGE("cp post failed, ret:%8x", ret);
        return QOSM_POST_TASK_ERR;
    }
    return QOSM_SUCCESS;
}

static void QOSM_FreeConnParam(void *param)
{
    QOSM_AutoRateConnParam *connParam = (QOSM_AutoRateConnParam *)param;
    if (connParam == NULL) {
        return;
    }
    if (connParam->link != NULL) {
        SDF_MemFree(connParam->link);
    }
    SDF_MemFree(connParam);
}

uint32_t QOSM_AutoRateAddConnection(const QOSM_AutoRateConnParam *param)
{
    QOSM_CHECK_RETURN_RET(param != NULL && param->link != NULL, QOSM_NULL_PTR_ERR, "param is null");
    QOSM_CHECK_RETURN_RET(param->linkCnt != 0 && param->linkCnt <= QOSM_AUTORATE_MAX_LINK_CNT,
        QOSM_INVALID_COUNT_ERR, "invalid channelCnt");

    QOSM_AutoRateConnParam *connParam =
        (QOSM_AutoRateConnParam *)SDF_MemZalloc(sizeof(QOSM_AutoRateConnParam));
    QOSM_CHECK_RETURN_RET(connParam != NULL, QOSM_MALLOC_ERR, "malloc auto rate connection param failed");

    size_t linkSize = param->linkCnt * sizeof(QOSM_ConnParam);
    connParam->link = (QOSM_ConnParam *)SDF_MemZalloc(linkSize);
    if (connParam->link == NULL) {
        SDF_MemFree(connParam);
        QOSM_LOGE("malloc auto rate link failed");
        return QOSM_MALLOC_ERR;
    }
    connParam->qosId = param->qosId;
    connParam->linkCnt = param->linkCnt;
    if (memcpy_s(connParam->link, linkSize, param->link, linkSize) != EOK) {
        QOSM_FreeConnParam(connParam);
        QOSM_LOGE("copy auto rate link failed");
        return QOSM_MEMCPY_ERR;
    }
    uint32_t ret = CP_PostTask(QOSM_ICGMgrAddConnection, (void *)connParam, QOSM_FreeConnParam);
    if (ret != CP_OK) {
        QOSM_LOGE("cp post failed, ret:%8x", ret);
        return QOSM_POST_TASK_ERR;
    }
    return QOSM_SUCCESS;
}

uint32_t QOSM_AutoRateDeleteConnection(const QOSM_AutoRateConnParam *param)
{
    QOSM_CHECK_RETURN_RET(param != NULL && param->link != NULL, QOSM_NULL_PTR_ERR, "param is null");
    QOSM_CHECK_RETURN_RET(param->linkCnt != 0 && param->linkCnt <= QOSM_AUTORATE_MAX_LINK_CNT,
        QOSM_INVALID_COUNT_ERR, "invalid linkCnt");

    QOSM_AutoRateConnParam *connParam =
        (QOSM_AutoRateConnParam *)SDF_MemZalloc(sizeof(QOSM_AutoRateConnParam));
    QOSM_CHECK_RETURN_RET(connParam != NULL, QOSM_MALLOC_ERR, "malloc auto rate connection param failed");

    size_t linkSize = param->linkCnt * sizeof(QOSM_ConnParam);
    connParam->link = (QOSM_ConnParam *)SDF_MemZalloc(linkSize);
    if (connParam->link == NULL) {
        SDF_MemFree(connParam);
        QOSM_LOGE("malloc auto rate link failed");
        return QOSM_MALLOC_ERR;
    }
    connParam->qosId = param->qosId;
    connParam->linkCnt = param->linkCnt;
    if (memcpy_s(connParam->link, linkSize, param->link, linkSize) != EOK) {
        QOSM_FreeConnParam(connParam);
        QOSM_LOGE("copy auto rate link failed");
        return QOSM_MEMCPY_ERR;
    }

    uint32_t ret = CP_PostTask(QOSM_ICGMgrDeleteConnection, (void *)connParam, QOSM_FreeConnParam);
    if (ret != CP_OK) {
        QOSM_LOGE("cp post failed, ret:%8x", ret);
        return QOSM_POST_TASK_ERR;
    }
    return QOSM_SUCCESS;
}

static void QOSM_FreeDataPath(void *param)
{
    QOSM_AutoRateDataPath *dataPath = (QOSM_AutoRateDataPath *)param;
    if (dataPath == NULL) {
        return;
    }
    if (dataPath->codecConfigData != NULL) {
        SDF_MemFree(dataPath->codecConfigData);
    }
    SDF_MemFree(dataPath);
}

uint32_t QOSM_AutoRateAddDataPath(const QOSM_AutoRateDataPath *param)
{
    QOSM_CHECK_RETURN_RET(param != NULL && param->codecConfigData != NULL, QOSM_NULL_PTR_ERR, "param is null");
    QOSM_CHECK_RETURN_RET(param->codecConfigLen != 0 && param->codecConfigLen <= QOSM_AUTORATE_MAX_CODEC_CONFIG_LEN,
        QOSM_INVALID_COUNT_ERR, "invalid codecConfigLen");

    QOSM_LOGI("add data path, qos id: %d, conn handle: %d, direction: %d, path id: %d",
        param->qosId, param->connHandle, param->direction, param->pathId);

    QOSM_AutoRateDataPath *dataPath =
        (QOSM_AutoRateDataPath *)SDF_MemZalloc(sizeof(QOSM_AutoRateDataPath));
    QOSM_CHECK_RETURN_RET(dataPath != NULL, QOSM_MALLOC_ERR, "malloc auto rate data path failed");

    dataPath->codecConfigData = (uint8_t *)SDF_MemZalloc(param->codecConfigLen);
    if (dataPath->codecConfigData == NULL) {
        SDF_MemFree(dataPath);
        QOSM_LOGE("malloc auto rate codec config data failed");
        return QOSM_MALLOC_ERR;
    }

    dataPath->qosId = param->qosId;
    dataPath->connHandle = param->connHandle;
    dataPath->direction = param->direction;
    dataPath->pathId = param->pathId;
    dataPath->codec.codecId = param->codec.codecId;
    dataPath->codec.vendorId = param->codec.vendorId;
    dataPath->codec.vendorCodecId = param->codec.vendorCodecId;
    dataPath->controllerDelay = param->controllerDelay;
    dataPath->codecConfigLen = param->codecConfigLen;
    if (memcpy_s(dataPath->codecConfigData, param->codecConfigLen, param->codecConfigData,
        param->codecConfigLen) != EOK) {
        QOSM_FreeDataPath(dataPath);
        QOSM_LOGE("copy auto rate codecConfigData failed");
        return QOSM_MEMCPY_ERR;
    }

    uint32_t ret = CP_PostTask(QOSM_ICGMgrAddDataPath, (void *)dataPath, QOSM_FreeDataPath);
    if (ret != CP_OK) {
        QOSM_LOGE("cp post failed, ret:%8x", ret);
        return QOSM_POST_TASK_ERR;
    }
    return QOSM_SUCCESS;
}

uint32_t QOSM_AutoRateDeleteDataPath(const QOSM_AutoRateDeletedDataPath *param)
{
    QOSM_CHECK_RETURN_RET(param != NULL, QOSM_NULL_PTR_ERR, "param is null");

    QOSM_AutoRateDeletedDataPath *dataPath =
        (QOSM_AutoRateDeletedDataPath *)SDF_MemZalloc(sizeof(QOSM_AutoRateDeletedDataPath));
    QOSM_CHECK_RETURN_RET(dataPath != NULL, QOSM_MALLOC_ERR, "malloc auto rate deleted data path failed");

    QOSM_LOGI("delete data path, qos id: %d, conn handle: %d, direction: %d", param->qosId, param->connHandle,
        param->direction);

    dataPath->qosId = param->qosId;
    dataPath->connHandle = param->connHandle;
    dataPath->direction = param->direction;

    uint32_t ret = CP_PostTask(QOSM_ICGMgrDeleteDataPath, (void *)dataPath, SDF_MemFree);
    if (ret != CP_OK) {
        QOSM_LOGE("cp post failed, ret:%8x", ret);
        return QOSM_POST_TASK_ERR;
    }
    return QOSM_SUCCESS;
}

uint32_t QOSM_RecvAutoRateMsg(const QOSM_AutoRateRecvMsgParam *param)
{
    QOSM_CHECK_RETURN_RET(param != NULL, QOSM_NULL_PTR_ERR, "param is null");
    QOSM_CHECK_RETURN_RET(param->qosIndex == QOSM_QOSINDEX_CALL, QOSM_INVALID_PARAM_ERR,
        "qosIndex is invalid");
    QOSM_AutoRateRecvMsgParam *updateInfo =
        (QOSM_AutoRateRecvMsgParam *)SDF_MemZalloc(sizeof(QOSM_AutoRateRecvMsgParam));
    QOSM_CHECK_RETURN_RET(updateInfo != NULL, QOSM_MALLOC_ERR, "malloc auto rate label id param failed");
    updateInfo->qosId = param->qosId;
    updateInfo->qosIndex = param->qosIndex;
    updateInfo->labelId = param->labelId;
    updateInfo->msgType = param->msgType;
    updateInfo->result = param->result;
    uint32_t ret = CP_PostTask(QOSM_ProcAutoRateMsg, (void *)updateInfo, SDF_MemFree);
    if (ret != CP_OK) {
        QOSM_LOGE("cp post failed, ret:%8x", ret);
        return QOSM_POST_TASK_ERR;
    }
    return QOSM_SUCCESS;
}

uint32_t QOSM_AutoRateSetEarphoneFeedback(const QOSM_AutoRateEarphoneFeedbackParam *param)
{
    QOSM_CHECK_RETURN_RET(param != NULL, QOSM_NULL_PTR_ERR, "param is null");
    QOSM_CHECK_RETURN_RET(param->supportedBitrateCnt > 0 && param->supportedBitrateCnt <=
        QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT, QOSM_INVALID_PARAM_ERR, "supported bitrate cnt is invalid");

    QOSM_ICGMgrNotifyParam *notifyParam = (QOSM_ICGMgrNotifyParam *)SDF_MemZalloc(sizeof(QOSM_ICGMgrNotifyParam));
    QOSM_CHECK_RETURN_RET(notifyParam != NULL, QOSM_MALLOC_ERR, "malloc auto rate notify param failed");

    notifyParam->type = NOTIFY_TYPE_EARPHONE;
    notifyParam->supportedBitrateCnt = param->supportedBitrateCnt;
    (void)memcpy_s(notifyParam->supportedBitrate, sizeof(uint16_t) * QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT,
        param->supportedBitrate, sizeof(uint16_t) * QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT);
    uint32_t ret = CP_PostTask(QOSM_ICGMgrParamNotify, (void *)notifyParam, SDF_MemFree);
    if (ret != CP_OK) {
        QOSM_LOGE("cp post failed, ret:%8x", ret);
        return QOSM_POST_TASK_ERR;
    }
    return QOSM_SUCCESS;
}

uint32_t QOSM_AutoRateSetCoexistSuggestion(const QOSM_AutoRateCoexistSuggestionParam *param)
{
    QOSM_CHECK_RETURN_RET(param != NULL, QOSM_NULL_PTR_ERR, "param is null");
    QOSM_CHECK_RETURN_RET(param->maxBitrate >= QOSM_MIN_BITRATE && param->maxBitrate <= QOSM_MAX_BITRATE,
        QOSM_INVALID_PARAM_ERR, "max bitrate is invalid");
    QOSM_CHECK_RETURN_RET(param->dutyCycle >= QOS_DUTY_CYCLE_100P && param->dutyCycle < QOS_DUTY_CYCLE_ANY,
        QOSM_INVALID_PARAM_ERR, "duty cycle is invalid");
 
    QOSM_ICGMgrNotifyParam *notifyParam = (QOSM_ICGMgrNotifyParam *)SDF_MemZalloc(sizeof(QOSM_ICGMgrNotifyParam));
    QOSM_CHECK_RETURN_RET(notifyParam != NULL, QOSM_MALLOC_ERR, "malloc auto rate notify param failed");

    notifyParam->type = NOTIFY_TYPE_COEXIST;
    notifyParam->maxBitrate = param->maxBitrate;
    notifyParam->dutyCycle = param->dutyCycle;
    uint32_t ret = CP_PostTask(QOSM_ICGMgrParamNotify, (void *)notifyParam, SDF_MemFree);
    if (ret != CP_OK) {
        QOSM_LOGE("cp post failed, ret:%8x", ret);
        return QOSM_POST_TASK_ERR;
    }
    return QOSM_SUCCESS;
}

uint32_t QOSM_AutoRateGetICGG2TParam(QOSM_QosIndex qosIndex, QOSM_ICBParam *param)
{
    QOSM_CHECK_RETURN_RET(param != NULL, QOSM_NULL_PTR_ERR, "param is null");
    return QOSM_GetICBG2TParamByIndex(qosIndex, param);
}

uint32_t QOSM_AutoRateGetICGT2GParam(QOSM_QosIndex qosIndex, QOSM_ICBParam *param)
{
    QOSM_CHECK_RETURN_RET(param != NULL, QOSM_NULL_PTR_ERR, "param is null");
    return QOSM_GetICBT2GParamByIndex(qosIndex, param);
}
