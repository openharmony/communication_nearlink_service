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
#include "nlstk_devd_api.h"
#include "nlstk_stm_collab_ext.h"
#include "nlstk_public_define.h"
#include "devd_adv.h"
#include "devd_scan.h"
#include "devd_scan_filter.h"
#include "nlstk_devd.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "sdf_mem.h"
#include "dli.h"
#include "dli_cmd.h"
#include "securec.h"

#define ADV_INVALID_HANDLE 0xFF

typedef struct {
    uint8_t handle;
    NLSTK_DevdAdvEventCbk cbk;
} NLSTK_CreateHandleParam_S;

static NLSTK_Errcode_E DevdCheckAdvParamsAccessMode(uint8_t accessMode)
{
    if (accessMode > ADV_ACCESS_MODE_DEFAULT) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! accessMode=%d", accessMode);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckAdvParamsDiscoveryLevel(uint8_t discoveryLevel)
{
    if (discoveryLevel > ADV_DISCOVERY_LEVEL_SPECIAL) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! discoveryLevel=%d", discoveryLevel);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckAdvParamsAdvMode(uint8_t advMode)
{
    if (advMode > ADV_MODE_CONNECTABLE_SCANABLE && advMode != ADV_MODE_CONNECTABLE_DIRECTED) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! advMode=%d", advMode);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckAdvParamsAdvGtRole(uint8_t advGtRole)
{
    if (advGtRole > ADV_GT_ROLE_G_NO_NEGO) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! advGtRole=%d", advGtRole);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckAdvParamsAdvIntervalMin(uint32_t advIntervalMin)
{
    if ((advIntervalMin < DEVD_ADV_INTERVAL_MIN) || (advIntervalMin > DEVD_ADV_INTERVAL_MAX)) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! advIntervalMin=%d", advIntervalMin);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckAdvParamsAdvIntervlMax(uint32_t advIntervalMax)
{
    if ((advIntervalMax < DEVD_ADV_INTERVAL_MIN) || (advIntervalMax > DEVD_ADV_INTERVAL_MAX)) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! advIntervalMax=%d", advIntervalMax);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckAdvParamsAdvChannelMap(uint8_t advChannelMap)
{
    if (advChannelMap > ADV_CHANNEL_MAP_DEFAULT) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! advChannelMap=%d", advChannelMap);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckAdvParamsOwnAddrType(uint8_t ownAddrType)
{
    if (ownAddrType >= ADDR_TYPE_END) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! ownAddrType=%d", ownAddrType);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckAdvParamsPeerAddrType(uint8_t peerAddrType)
{
    if (peerAddrType >= ADDR_TYPE_END) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! peerAddrType=%d", peerAddrType);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckConnParamIntervalMin(uint16_t intervalMin)
{
    if ((intervalMin < CONN_INTV_MIN) || (intervalMin > CONN_INTV_MAX)) {
        NLSTK_LOG_ERROR("[DEVD] set conn param check failed! intervalMin=%d", intervalMin);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckConnParamIntervalMax(uint16_t intervalMax)
{
    if ((intervalMax < CONN_INTV_MIN) || (intervalMax > CONN_INTV_MAX)) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! intervalMax=%d", intervalMax);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckConnParamMaxLatency(uint16_t maxLatency)
{
    if (maxLatency > CONN_MAX_LATENCY) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! maxLatency=%d", maxLatency);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckConnParamSupervisionTimeout(const NLSTK_DevdConnParam_S *connParam)
{
    uint8_t unitOfTimeout = 10;
    uint8_t uintOfBitShift = 2;
    uint8_t offsetOfLatency = 1;
    if ((connParam->supervisionTimeout < CONN_SUPERVISION_TIMEOUT_MIN) ||
        (connParam->supervisionTimeout > CONN_SUPERVISION_TIMEOUT_MAX) ||
        // 以毫秒为单位的超时时间应该大于
        // (1 + 最大延迟周期) * 异步数据链路事件组周期最大值 * 2
        // 超时时间单位10毫秒，interval单位0.125毫秒，计算结果右移2位
        ((connParam->supervisionTimeout * unitOfTimeout) <=
            ((connParam->intervalMax >> uintOfBitShift) * (offsetOfLatency + connParam->maxLatency)))) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! supervisionTimeout=%d", connParam->supervisionTimeout);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckExtParamAdvFilterPolicy(uint8_t advFilterPolicy)
{
    if (advFilterPolicy > ADV_FLT_ALLOW_SCAN_ALLOW_CONNECT) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! advFilterPolicy=%d", advFilterPolicy);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckExtParamAdvTxPower(int8_t advTxPower)
{
    if ((advTxPower != ADV_TX_POWER_DEFAULT) &&
        ((advTxPower < ADV_TX_POWER_MIN) || (advTxPower > ADV_TX_POWER_MAX))) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! advTxPower=%d", advTxPower);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckExtParamPrimAdvPhy(uint8_t primAdvPhy)
{
    if (primAdvPhy > ADV_PHY_4M) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! primAdvPhy=%d", primAdvPhy);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckExtParamSecondAdvFrame(uint8_t secondAdvFrame)
{
    if (secondAdvFrame > ADV_FRAME_TYPE_SHORT_HEADER) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! secondAdvFrame=%d", secondAdvFrame);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckExtParamSecondAdvPhy(uint8_t secondAdvPhy)
{
    if (secondAdvPhy > ADV_PHY_4M) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! secondAdvPhy=%d", secondAdvPhy);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckExtParamSecondAdvMcs(uint8_t secondAdvMcs)
{
    if (secondAdvMcs > ADV_MCS_MAX) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! secondAdvMcs=%d", secondAdvMcs);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckExtParamSecondAdvPilot(uint8_t secondAdvPilot)
{
    if (secondAdvPilot > ADV_PILOT_RATIO_NO) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! secondAdvPilot=%d", secondAdvPilot);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCheckExtParamScanReqNotifEnable(uint8_t scanReqNotifEnable)
{
    if (scanReqNotifEnable > SCAN_REQ_REPORT) {
        NLSTK_LOG_ERROR("[DEVD] set adv param check failed! scanReqNotifEnable=%d", scanReqNotifEnable);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCopyBasicParam(DevdAdvParam_S *param, const NLSTK_DevdSetAdvParams_S *advParams)
{
    if (advParams == NULL) {
        return NLSTK_ERRCODE_POINTER_NULL;
    }
    if ((DevdCheckAdvParamsAccessMode(advParams->accessMode) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckAdvParamsDiscoveryLevel(advParams->discoveryLevel) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckAdvParamsAdvMode(advParams->param.advMode) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckAdvParamsAdvGtRole(advParams->param.advGtRole) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckAdvParamsAdvIntervalMin(advParams->param.advIntervalMin) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckAdvParamsAdvIntervlMax(advParams->param.advIntervalMax) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckAdvParamsAdvChannelMap(advParams->param.advChannelMap) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckAdvParamsOwnAddrType(advParams->param.ownAddr.type) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckAdvParamsPeerAddrType(advParams->param.peerAddr.type) != NLSTK_ERRCODE_SUCCESS)) {
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    DevdAdvBasicParam_S *basic = &param->basic;
    /* 广播基本参数 */
    basic->advMode        = advParams->param.advMode;
    basic->gtRole         = advParams->param.advGtRole;
    basic->advMinInterval = advParams->param.advIntervalMin;
    basic->advMaxInterval = advParams->param.advIntervalMax;
    basic->channelMap     = advParams->param.advChannelMap;
    basic->ownAddr.type   = advParams->param.ownAddr.type;
    basic->peerAddr.type  = advParams->param.peerAddr.type;
    basic->primaryFrameType = advParams->param.primaryFrameType;
    (void)memcpy_s(&(basic->ownAddr.addr), SLE_ADDR_LEN, &(advParams->param.ownAddr.addr), SLE_ADDR_LEN);
    (void)memcpy_s(&(basic->peerAddr.addr), SLE_ADDR_LEN, &(advParams->param.peerAddr.addr), SLE_ADDR_LEN);

    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCopyExtparam(DevdAdvParam_S *param, const NLSTK_DevdAdvExtParam_S *extParam)
{
    NLSTK_LOG_INFO("[DEVD] DevdCopyExtparam");
    if (extParam == NULL) {
        return NLSTK_ERRCODE_POINTER_NULL;
    }
    if ((DevdCheckExtParamAdvFilterPolicy(extParam->advFilterPolicy) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckExtParamAdvTxPower(extParam->advTxPower) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckExtParamPrimAdvPhy(extParam->phyParam.primAdvPhy) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckExtParamSecondAdvFrame(extParam->phyParam.secondAdvFrame) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckExtParamSecondAdvPhy(extParam->phyParam.secondAdvPhy) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckExtParamSecondAdvMcs(extParam->phyParam.secondAdvMcs) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckExtParamSecondAdvPilot(extParam->phyParam.secondAdvPilot) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckExtParamScanReqNotifEnable(extParam->scanParam.scanReqNotifEnable) !=
            NLSTK_ERRCODE_SUCCESS)) {
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    DevdAdvPhyParam_S *phy = &param->phy;
    DevdAdvScanParam_S *scan = &param->scan;
    /* 广播扩展基本参数 */
    param->filterPolicy = extParam->advFilterPolicy;
    param->sid          = extParam->advSid;
    param->txPower      = extParam->advTxPower;
    /* 广播信道参数 */
    phy->primaryPhy       = extParam->phyParam.primAdvPhy;
    phy->secondaryFrame   = extParam->phyParam.secondAdvFrame;
    phy->secondaryPhy     = extParam->phyParam.secondAdvPhy;
    phy->secondaryPilot   = extParam->phyParam.secondAdvPilot;
    phy->secondaryMcs     = extParam->phyParam.secondAdvMcs;
    phy->secondaryMaxSkip = extParam->phyParam.secondAdvMaxSkip;
    /* 广播扫描参数 */
    scan->notifyEnable  = extParam->scanParam.scanReqNotifEnable;
    scan->rxMaxNumber   = extParam->scanParam.scanReqRecvNumberMax;
    scan->rxMaxDuration = extParam->scanParam.scanReqRxDurMax;

    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E DevdCopyConnParam(DevdAdvParam_S *param, const NLSTK_DevdConnParam_S *connParam)
{
    if (connParam == NULL) {
        NLSTK_LOG_ERROR("[DEVD] check connParam null error");
        return NLSTK_ERRCODE_POINTER_NULL;
    }
    if ((DevdCheckConnParamIntervalMin(connParam->intervalMin) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckConnParamIntervalMax(connParam->intervalMax) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckConnParamMaxLatency(connParam->maxLatency) != NLSTK_ERRCODE_SUCCESS) ||
        (DevdCheckConnParamSupervisionTimeout(connParam) != NLSTK_ERRCODE_SUCCESS)) {
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    DevdAdvConnectParam_S *connect = &param->connect;
    /* 广播连接参数 */
    connect->connMinInterval    = connParam->intervalMin;
    connect->connMaxInterval    = connParam->intervalMax;
    connect->maxLatency         = connParam->maxLatency;
    connect->supervisionTimeout = connParam->supervisionTimeout;
    connect->minCeLength        = connParam->minCeLength;
    connect->maxCeLength        = connParam->maxCeLength;

    return NLSTK_ERRCODE_SUCCESS;
}

static void DevdSetDefaultAdvExtParam(DevdAdvParam_S *param)
{
    NLSTK_LOG_INFO("[DEVD] DevdSetDefaultAdvExtParam");
    param->filterPolicy         = ADV_FLT_ANY_SCAN_ANY_CONNECT;
    param->sid                  = 0;              /* 广播分组 */
    param->txPower              = ADV_TX_POWER_DEFAULT;
    param->phy.primaryPhy       = ADV_PHY_1M;
    param->phy.secondaryFrame   = ADV_FRAME_TYPE_GFSK;
    param->phy.secondaryPhy     = ADV_PHY_1M;
    param->phy.secondaryMcs     = 0;
    param->phy.secondaryMaxSkip = 0;              /* 0:优先发送数据广播 */
    param->scan.notifyEnable    = 0;              /* 0:不上报收到的scan req */
    param->scan.rxMaxNumber     = 1;              /* 一个广播周期内扫描请求最大接收数目 */
    param->scan.rxMaxDuration   = 0;              /* 一个广播周期内扫描请求最大接收时间 */
}

static NLSTK_Errcode_E DevdCopyAdvParams(DevdSetAdvParams_S *setParam, const NLSTK_DevdSetAdvParams_S *advParams)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    ret = DevdCopyBasicParam(&setParam->param, advParams);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        return ret;
    }
    setParam->accessMode     = advParams->accessMode;
    setParam->param.discoveryLevel = advParams->discoveryLevel;
    setParam->param.handle = advParams->param.advHandle;
    if (advParams->param.extParam == NULL) {
        DevdSetDefaultAdvExtParam(&setParam->param);
    } else {
        ret = DevdCopyExtparam(&setParam->param, advParams->param.extParam);
        if (ret != NLSTK_ERRCODE_SUCCESS) {
            return ret;
        }
    }
    if (advParams->param.advGtRole == ADV_GT_ROLE_T_NO_NEGO) {
        return NLSTK_ERRCODE_SUCCESS;
    } else {
        /* 非ADV_GT_ROLE_T_NO_NEGO模式，需要配置连接参数 */
        ret = DevdCopyConnParam(&setParam->param, advParams->param.connParam);
    }
    return ret;
}

static NLSTK_Errcode_E DevdCopyAdvData(NLSTK_DevdAdvData_S *advData, NLSTK_DevdAdvData_S *setData)
{
    NLSTK_CHECK_RETURN(setData->advData != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD]setData is null");
    if (setData->scanRspDataLen != 0 && setData->scanRspData == NULL) {
        NLSTK_LOG_ERROR("[DEVD] scanRspData is null");
        return NLSTK_ERRCODE_POINTER_NULL;
    }
    advData->advDataLen = setData->advDataLen;
    NLSTK_CHECK_RETURN(advData->advDataLen > 0 && advData->advDataLen <= DEFAULT_MAX_ADV_DATA_LEN,
        NLSTK_ERRCODE_PARAM_ERR, "[DEVD]memory len error");
    advData->advData = (uint8_t *)SDF_MemZalloc(advData->advDataLen);
    NLSTK_CHECK_RETURN(advData->advData != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[DEVD]memory alloc error");
    (void)memcpy_s(advData->advData, advData->advDataLen, setData->advData, setData->advDataLen);
    advData->scanRspDataLen = setData->scanRspDataLen;
    if (setData->scanRspDataLen != 0) {
        advData->scanRspData = (uint8_t *)SDF_MemZalloc(advData->scanRspDataLen);
        if (advData->scanRspData == NULL) {
            NLSTK_LOG_ERROR("[DEVD] memory alloc error");
            SDF_MemFree(advData->advData);
            return NLSTK_ERRCODE_MALLOC_FAIL;
        }
        (void)memcpy_s(advData->scanRspData, advData->scanRspDataLen, setData->scanRspData, setData->scanRspDataLen);
    }

    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdSetAdvData(NLSTK_DevdSetAdvData_S *setAdvData)
{
    NLSTK_CHECK_RETURN(setAdvData != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD]setAdvData is null");
    NLSTK_DevdSetAdvData_S *advData = (NLSTK_DevdSetAdvData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSetAdvData_S));
    NLSTK_CHECK_RETURN(advData != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[DEVD]advData is null");
    advData->advHandle = setAdvData->advHandle;
    if (DevdCopyAdvData(&advData->data, &setAdvData->data) != NLSTK_ERRCODE_SUCCESS) {
        SDF_MemFree(advData);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    if (SchedulePostTask((SDF_WorkCb)DevdSetAdvData, advData, (SDF_FreeWorkArg)DevdFreeSetAdvData) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[DEVD] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdStartAdv(NLSTK_DevdSetAdvParams_S *advParams)
{
    NLSTK_CHECK_RETURN(advParams != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD]advParams is null");
    DevdSetAdvParams_S *setParam = (DevdSetAdvParams_S *)SDF_MemZalloc(sizeof(DevdSetAdvParams_S));
    NLSTK_CHECK_RETURN(setParam != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD]setParam is null");
    if (DevdCopyAdvParams(setParam, advParams) != NLSTK_ERRCODE_SUCCESS) {
        SDF_MemFree(setParam);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    NLSTK_DevdSetAdvData_S setData = {0};
    setData.data.advDataLen = advParams->data.advDataLen;
    setData.data.scanRspDataLen = advParams->data.scanRspDataLen;
    setData.data.advData = advParams->data.advData;
    setData.data.scanRspData = advParams->data.scanRspData;
    if (DevdCopyAdvData(&setParam->data, &setData.data) != NLSTK_ERRCODE_SUCCESS) {
        SDF_MemFree(setParam);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    if (SchedulePostTask((SDF_WorkCb)DevdSetAdvParam, setParam, (SDF_FreeWorkArg)DevdFreeSetAdvParams) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[DEVD] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdEnableAdv(NLSTK_DevdSetAdvEnable_S *setEnable)
{
    NLSTK_CHECK_RETURN(setEnable != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD] setEnable is null");
    NLSTK_CHECK_RETURN(setEnable->enable == 0 || setEnable->enable == 1, NLSTK_ERRCODE_PARAM_ERR,
        "[DEVD] enable param is error");
    NLSTK_DevdSetAdvEnable_S *advEnable = (NLSTK_DevdSetAdvEnable_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSetAdvEnable_S));
    NLSTK_CHECK_RETURN(advEnable != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[DEVD] advEnable is null");
    advEnable->advHandle = setEnable->advHandle;
    advEnable->enable = setEnable->enable;
    advEnable->duration = setEnable->duration;
    advEnable->maxAdvEvent = setEnable->maxAdvEvent;

    if (SchedulePostTask((SDF_WorkCb)DevdEnableAdv, advEnable, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[DEVD] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdSetTxPower(NLSTK_DevdSetTxPower_S *setTxPower)
{
    NLSTK_CHECK_RETURN(setTxPower != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD] setTxPower is null");
    NLSTK_DevdSetTxPower_S *txPower = (NLSTK_DevdSetTxPower_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSetTxPower_S));
    NLSTK_CHECK_RETURN(txPower != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[DEVD] txPower is null");
    txPower->bleMaxPower = setTxPower->bleMaxPower;
    txPower->sleMaxPower = setTxPower->sleMaxPower;

    if (SchedulePostTask((SDF_WorkCb)DevdSetTxPower, txPower, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[DEVD] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdRemoveAdv(uint8_t *setAdvHandle)
{
    NLSTK_CHECK_RETURN(setAdvHandle != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD] setAdvHandle is null");
    uint8_t *advHandle = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    NLSTK_CHECK_RETURN(advHandle != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[DEVD] advHandle is null");
    *advHandle = *setAdvHandle;
    if (SchedulePostTask((SDF_WorkCb)DevdRemoveAdv, advHandle, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[DEVD] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void DevdCreateAdvHandleInner(void *arg)
{
    NLSTK_CreateHandleParam_S *param = (NLSTK_CreateHandleParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[DEVD] create adv handle param is null");
    NLSTK_CHECK_RETURN_VOID(param->cbk != NULL, "[DEVD] adv cbk is null");
    // 当创建句柄失败时，返回0xFF无效句柄
    param->handle = DevdCreateAdvHandle(param->cbk);
}

uint8_t NLSTK_DevdCreateAdvHandle(NLSTK_DevdAdvEventCbk cbk)
{
    NLSTK_CHECK_RETURN(cbk != NULL, ADV_INVALID_HANDLE, "[DEVD] adv cbk is null");
    NLSTK_CreateHandleParam_S param = {0};
    param.handle = ADV_INVALID_HANDLE;
    param.cbk = cbk;
    if (SchedulePostTaskBlocked(DevdCreateAdvHandleInner, &param, NULL, SEM_ALWAYS_WAIT) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[DEVD] SchedulePostTaskBlocked failed");
        return ADV_INVALID_HANDLE;
    }
    return param.handle;
}

NLSTK_Errcode_E NLSTK_DevdSleStartScan(NLSTK_DevdSleScanParams_S *sleScanParams)
{
    NLSTK_CHECK_RETURN(sleScanParams != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD] sleScanParams is null");

    uint8_t phyCount = DLI_GetPhyCountByFrameType(sleScanParams->frameType);
    NLSTK_DevdSleScanParams_S *scanParams = (NLSTK_DevdSleScanParams_S *)SDF_MemZalloc(
        sizeof(NLSTK_DevdSleScanParams_S) + phyCount * sizeof(NLSTK_DevdSleScanParamsNoPhy_S));
    NLSTK_CHECK_RETURN(scanParams != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD] scanParams is null");
    scanParams->localAddrType = sleScanParams->localAddrType;
    scanParams->scanFilterPolicy = sleScanParams->scanFilterPolicy;
    scanParams->frameType = sleScanParams->frameType;

    for (uint8_t idx = 0; idx < phyCount; idx++) {
        scanParams->params[idx].scanType = sleScanParams->params[idx].scanType;
        scanParams->params[idx].scanInterval = sleScanParams->params[idx].scanInterval;
        scanParams->params[idx].scanWindow = sleScanParams->params[idx].scanWindow;
    }
    if ((scanParams->localAddrType >= ADDR_TYPE_END) ||
        (scanParams->scanFilterPolicy > SCAN_FLT_EXTEND) ||
        (scanParams->frameType > ((uint8_t)SCAN_FRAME_TYPE_1 | (uint8_t)SCAN_FRAME_TYPE_4))) {
        NLSTK_LOG_ERROR("[DEVD] set scan param check failed");
        SDF_MemFree(scanParams);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    for (uint8_t idx = 0; idx < phyCount; idx++) {
        if ((scanParams->params[idx].scanType > SCAN_TYPE_ACTIVE) ||
            (scanParams->params[idx].scanInterval < SCAN_INTERVAL_MIN) ||
            (scanParams->params[idx].scanWindow < SCAN_WINDOW_MIN)) {
            NLSTK_LOG_ERROR("[DEVD] set scan param check failed");
            SDF_MemFree(scanParams);
            return NLSTK_ERRCODE_PARAM_ERR;
        }
    }
    if (SchedulePostTask((SDF_WorkCb)DevdSleSetScanParam, scanParams, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[DEVD] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdSleEnableScan(NLSTK_DevdSleScanEnable_S *sleScanEnable)
{
    NLSTK_CHECK_RETURN(sleScanEnable != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD] sleScanEnable is null");
    NLSTK_DevdSleScanEnable_S *scanEnable =
        (NLSTK_DevdSleScanEnable_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSleScanEnable_S));
    NLSTK_CHECK_RETURN(scanEnable != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD] scanEnable is null");
    scanEnable->scanEnable = sleScanEnable->scanEnable;
    scanEnable->scanFilterDuplicates = sleScanEnable->scanFilterDuplicates;
    if (SchedulePostTask((SDF_WorkCb)DevdSleEnableScan, scanEnable, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[DEVD] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdRegScanEventCbk(NLSTK_DevdSleScanExterCbk_S *scanEventCbk)
{
    NLSTK_CHECK_RETURN(scanEventCbk != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD] param is null");
    NLSTK_DevdSleScanExterCbk_S *devdCbk = SDF_MemZalloc(sizeof(NLSTK_DevdSleScanExterCbk_S));
    NLSTK_CHECK_RETURN(devdCbk != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD] devdCbk is null");
    devdCbk->scanCbk = scanEventCbk->scanCbk;
    devdCbk->reportCbk = scanEventCbk->reportCbk;
    devdCbk->scanFilterCbk = scanEventCbk->scanFilterCbk;
    if (SchedulePostTask((SDF_WorkCb)DevdRegScanEventCbk, devdCbk, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[DEVD] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}