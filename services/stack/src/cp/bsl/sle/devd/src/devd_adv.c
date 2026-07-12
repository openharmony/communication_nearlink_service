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
#include "devd_adv.h"
#include "devd_local.h"
#include "devd_cbk.h"
#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "sdf_util.h"
#include "nlstk_schedule.h"
#include "dli_cmd.h"
#include "dli_errno.h"
#include "nlstk_cfgdb.h"
#include "securec.h"

static void BuildAdvParam(DevdAdvParam_S *param, DLI_AdvParam *dliParam)
{
    dliParam->advHandle = param->handle;
    dliParam->advGtRole = param->basic.gtRole;
    dliParam->primAdvIntervalMin[SLE_INDEX_0] = (uint8_t)(param->basic.advMinInterval & 0xFF);
    dliParam->primAdvIntervalMin[SLE_INDEX_1] = (uint8_t)((param->basic.advMinInterval >> SLE_SHIFT_BITS_8) & 0xFF);
    dliParam->primAdvIntervalMin[SLE_INDEX_2] = (uint8_t)((param->basic.advMinInterval >> SLE_SHIFT_BITS_16) & 0xFF);
    dliParam->primAdvIntervalMax[SLE_INDEX_0] = (uint8_t)(param->basic.advMaxInterval & 0xFF);
    dliParam->primAdvIntervalMax[SLE_INDEX_1] = (uint8_t)((param->basic.advMaxInterval >> SLE_SHIFT_BITS_8) & 0xFF);
    dliParam->primAdvIntervalMax[SLE_INDEX_2] = (uint8_t)((param->basic.advMaxInterval >> SLE_SHIFT_BITS_16) & 0xFF);
    dliParam->primAdvChannelMap = param->basic.channelMap;
    dliParam->ownAddrType = param->basic.ownAddr.type;
    dliParam->peerAddrType = param->basic.peerAddr.type;
    (void)memcpy_s(dliParam->ownAddr, SLE_ADDR_LEN, param->basic.ownAddr.addr, SLE_ADDR_LEN);
    (void)memcpy_s(dliParam->peerAddr, SLE_ADDR_LEN, param->basic.peerAddr.addr, SLE_ADDR_LEN);
    dliParam->primAdvFrameFormat = param->basic.primaryFrameType;
    dliParam->advMode = param->basic.advMode;
    // 扩展参数
    dliParam->advFilterPolicy = param->filterPolicy;
    dliParam->advTxPower = param->txPower;
    if (dliParam->primAdvFrameFormat == PRIM_ADV_FRAME_TYPE_1) {
        // 扩展广播无线帧类型，0：帧一，8~13：帧四
        dliParam->secondAdvFrameFormat = SECOND_ADV_FRAME_TYPE_1;
        dliParam->supervisionTimeout = DEFAULT_FRAME_TYPE_1_ADV_TIMEOUT;
    } else {
        dliParam->secondAdvFrameFormat = SECOND_ADV_FRAME_TYPE_4_M_0;
        dliParam->supervisionTimeout = DEFAULT_FRAME_TYPE_4_ADV_TIMEOUT;
    }
    dliParam->secondAdvMcs = 0;
    dliParam->secondAdvPhy = param->phy.secondaryPhy;
    dliParam->secondAdvPilot = param->phy.secondaryPilot;
    dliParam->secondAdvMaxSkip = param->phy.secondaryMaxSkip;
    dliParam->advSid = param->sid;
    (void)memcpy_s(&dliParam->scanReqNotifEnable, sizeof(DLI_AdvScanParam), &param->scan, sizeof(DLI_AdvScanParam));
    // 连接参数
    if (param->basic.gtRole != ADV_GT_ROLE_T_NO_NEGO) {
        NLSTK_LOG_INFO("set adv param connParam");
        (void)memcpy_s(&dliParam->connIntervalMin, sizeof(DLI_ConnParam), &param->connect, sizeof(DLI_ConnParam));
        if (dliParam->primAdvFrameFormat == PRIM_ADV_FRAME_TYPE_4_M_0) {
            dliParam->connIntervalMin = DEFAULT_FRAME_4_CONN_INTERVAL;
            dliParam->connIntervalMax = DEFAULT_FRAME_4_CONN_INTERVAL;
        }
    }
    // 推荐参数
    dliParam->maxLatency = 0;
    NLSTK_LOG_INFO("set adv param, primAdvFrameFormat: %hhu, ownAddrType:%hhu, advMode:%hhu, "
                   "secondAdvPhy=0x%02x, secondAdvMcs:0x%02x, secondAdvMaxSkip:0x%02x, advTxPower:%d",
        dliParam->primAdvFrameFormat,
        dliParam->ownAddrType,
        dliParam->advMode,
        dliParam->secondAdvPhy,
        dliParam->secondAdvMcs,
        dliParam->secondAdvMaxSkip,
        dliParam->advTxPower);
}

static void SendAdvParamToDli(DevdAdvNode_S *node)
{
    DevdAdvParam_S *param = node->tempParam;
    DLI_AdvParam dliParam = {0};

    BuildAdvParam(param, &dliParam);
    DLI_SetAdvParam(&dliParam);
}

static uint8_t GetAdvDataOperation(uint16_t totalLen, DevdAdvDataOp_S *op)
{
    if (totalLen <= op->dataOffset) {
        op->sendLen = 0;
        return ADV_OPERATION_FRAGMENT_INVALID;
    }
    uint16_t sendLen = totalLen - op->dataOffset;
    sendLen = SDF_MIN(sendLen, CfgdbGetMaxAdvDataLen());
    op->sendLen = sendLen;
    if (op->dataOffset == 0) {
        if (sendLen == totalLen) {
            return ADV_OPERATION_FRAGMENT_COMPLETE;
        } else {
            return ADV_OPERATION_FRAGMENT_FIRST;
        }
    } else if (sendLen + op->dataOffset == totalLen) {
        return ADV_OPERATION_FRAGMENT_LAST;
    } else {
        return ADV_OPERATION_FRAGMENT_INTERMEDIATE;
    }
}

static DLI_AdvData *BuildDliAdvParam(DevdAdvNode_S *node)
{
    NLSTK_DevdAdvData_S *data = node->tempData;
    uint8_t operation = GetAdvDataOperation(data->advDataLen, &node->dataOp);
    if (node->dataOp.sendLen == 0) {
        NLSTK_LOG_WARN("[DEVD]sendLen %hhu is error", node->dataOp.sendLen);
        return NULL;
    }

    uint32_t cmdSize = (uint32_t)sizeof(DLI_AdvData) + node->dataOp.sendLen;
    DLI_AdvData *advData = (DLI_AdvData *)SDF_MemZalloc(cmdSize);
    if (advData == NULL) {
        NLSTK_LOG_ERROR("[DEVD]advData malloc failed");
        return NULL;
    }
    advData->advHandle = node->handle;
    advData->operation = operation;
    advData->selection = 0;
    advData->advDataLen = node->dataOp.sendLen;
    if (memcpy_s(advData->advData, advData->advDataLen,
        data->advData + node->dataOp.dataOffset, node->dataOp.sendLen) != EOK) {
        SDF_MemFree(advData);
        NLSTK_LOG_ERROR("[DEVD]memcpy failed");
        return NULL;
    }
    return advData;
}

static DLI_ScanRspData *BuildDliScanRspParam(DevdAdvNode_S *node)
{
    NLSTK_DevdAdvData_S *data = node->tempData;
    uint8_t operation = GetAdvDataOperation(data->scanRspDataLen, &node->dataOp);
    if (node->dataOp.sendLen == 0) {
        NLSTK_LOG_WARN("[DEVD]sendLen %hhu is error", node->dataOp.sendLen);
        return NULL;
    }

    uint32_t cmdSize = (uint32_t)sizeof(DLI_ScanRspData) + node->dataOp.sendLen;
    DLI_ScanRspData *scanRspData = (DLI_ScanRspData *)SDF_MemZalloc(cmdSize);
    if (scanRspData == NULL) {
        NLSTK_LOG_ERROR("[DEVD]scanRspData malloc failed");
        return NULL;
    }
    scanRspData->advHandle = node->handle;
    scanRspData->operation = operation;
    scanRspData->selection = 0;
    scanRspData->scanRspDataLen = node->dataOp.sendLen;
    if (memcpy_s(scanRspData->scanRspData, scanRspData->scanRspDataLen,
        data->scanRspData + node->dataOp.dataOffset, node->dataOp.sendLen) != EOK) {
        SDF_MemFree(scanRspData);
        NLSTK_LOG_ERROR("[DEVD]memcpy failed");
        return NULL;
    }
    return scanRspData;
}

static void SendAdvDataToDli(DevdAdvNode_S *node)
{
    NLSTK_CHECK_RETURN_VOID(node->param != NULL, "[DEVD]adv node param is null");
    if (node->tempData == NULL) {
        node->status = DEVD_SLE_STATUS_IDLE;
        return;
    }
    if (node->status == DEVD_SLE_STATUS_SET_ADV_DATA) {
        DLI_AdvData *advData = BuildDliAdvParam(node);
        NLSTK_CHECK_RETURN_VOID(advData != NULL, "[DEVD]advData is null");
        DLI_SetAdvData(advData);
        node->dataOp.dataOffset += node->dataOp.sendLen;
        SDF_MemFree(advData);
    } else if (node->status == DEVD_SLE_STATUS_SET_SCAN_RSP_DATA &&
        node->param->basic.advMode >= ADV_MODE_NONCONN_SCANABLE) {
        DLI_ScanRspData *scanRspData = BuildDliScanRspParam(node);
        NLSTK_CHECK_RETURN_VOID(scanRspData != NULL, "[DEVD]scanRspData is null");
        DLI_SetScanRspData(scanRspData);
        node->dataOp.dataOffset += node->dataOp.sendLen;
        SDF_MemFree(scanRspData);
    } else {
        node->status = DEVD_SLE_STATUS_IDLE;
        return;
    }
}

void DevdSetAdvParam(void *arg)
{
    DevdSetAdvParams_S *params = (DevdSetAdvParams_S *)arg;
    NLSTK_CHECK_RETURN_VOID(params != NULL && params->data.advData != NULL, "[DEVD]set adv params is null");
    DevdAdvNode_S *node = NULL;
    NLSTK_LOG_INFO("[DEVD]set adv param, adv handle = 0x%x", params->param.handle);
    node = DevdGetAdvNode(params->param.handle, DEVD_ADV_LIST);
    NLSTK_CHECK_RETURN_VOID(node != NULL, "[DEVD]adv node is null");
    NLSTK_CHECK_RETURN_VOID(node->advStatus != ADV_ENABLED, "[DEVD]adv already enabled, refuse to set param");
    NLSTK_CHECK_RETURN_VOID(node->status == DEVD_SLE_STATUS_IDLE, "[DEVD]adv node is busy");
    NLSTK_CHECK_RETURN_VOID(DevdSaveAdvParamToTbl(node, &params->param) == NLSTK_OK, "[DEVD]save adv param failed");
    NLSTK_CHECK_RETURN_VOID(DevdSaveAdvDataToTbl(node, &params->data) == NLSTK_OK, "[DEVD]save adv data failed");
    node->status = DEVD_SLE_STATUS_SET_ADV_PARAMS;
    SendAdvParamToDli(node);
}

void DevdSetAdvData(void *arg)
{
    NLSTK_DevdSetAdvData_S *data = (NLSTK_DevdSetAdvData_S *)arg;
    NLSTK_CHECK_RETURN_VOID(data != NULL && data->data.advData != NULL, "[DEVD]set adv data is null");
    DevdAdvNode_S *node = NULL;
    NLSTK_LOG_INFO("[DEVD]set adv data, adv handle = 0x%x", data->advHandle);
    node = DevdGetAdvNode(data->advHandle, DEVD_ADV_LIST);
    NLSTK_CHECK_RETURN_VOID(node != NULL, "[DEVD]adv node is null");
    NLSTK_CHECK_RETURN_VOID(node->status == DEVD_SLE_STATUS_IDLE, "[DEVD]adv node is busy");
    NLSTK_CHECK_RETURN_VOID(DevdSaveAdvDataToTbl(node, &data->data) == NLSTK_OK, "[DEVD]save adv data failed");
    node->status = DEVD_SLE_STATUS_SET_ADV_DATA;
    // 设置isUpdateData这个状态是为了和设置参数后的设置数据作区分，设置数据完成后不通知service，更新数据完成后通知service
    node->isUpdateData = true;
    node->dataOp.dataOffset = 0;
    node->dataOp.sendLen = 0;
    SendAdvDataToDli(node);
}

void DevdEnableAdv(void *arg)
{
    NLSTK_DevdSetAdvEnable_S *enable = (NLSTK_DevdSetAdvEnable_S *)arg;
    NLSTK_CHECK_RETURN_VOID(enable != NULL, "[DEVD]enable is null");
    DevdAdvNode_S *node = NULL;
    NLSTK_LOG_INFO("[DEVD]set adv enable, adv handle = 0x%x", enable->advHandle);
    node = DevdGetAdvNode(enable->advHandle, DEVD_ADV_LIST);
    NLSTK_CHECK_RETURN_VOID(node != NULL, "[DEVD]adv node is null");
    NLSTK_CHECK_RETURN_VOID(node->advStatus != enable->enable, "[DEVD]adv node already set to %u", node->advStatus);
    NLSTK_CHECK_RETURN_VOID(node->status == DEVD_SLE_STATUS_IDLE, "[DEVD]adv node is busy");
    if (enable->enable == ADV_ENABLED) {
        NLSTK_LOG_INFO("[DEVD]enable adv");
        DLI_AdvEnable advEnable = {0};
        advEnable.enable = enable->enable;
        advEnable.duration = enable->duration;
        advEnable.maxAdvEvents = enable->maxAdvEvent;
        node->status = DEVD_SLE_STATUS_ENABLE_ADV;
        if (DLI_EnableAdv(node->handle, &advEnable) != DLI_SUCCESS) {
            NLSTK_LOG_ERROR("[DEVD] dli enable adv fail");
            node->status = DEVD_SLE_STATUS_IDLE;
            return;
        }
    } else {
        NLSTK_LOG_DEBUG("[DEVD]disable adv");
        node->status = DEVD_SLE_STATUS_DISABLE_ADV;
        if (DLI_EnableAdv(node->handle, NULL) != DLI_SUCCESS) {
            NLSTK_LOG_ERROR("[DEVD] dli disable adv fail");
            node->status = DEVD_SLE_STATUS_IDLE;
            return;
        }
    }
}

void DevdSetTxPower(void *arg)
{
    NLSTK_DevdSetTxPower_S *txPower = (NLSTK_DevdSetTxPower_S *)arg;
    NLSTK_CHECK_RETURN_VOID(txPower != NULL, "[DEVD]txPower is null");
    DLI_SetTxPowerParam param = {0};
    param.bleMaxPower = txPower->bleMaxPower;
    param.sleMaxPower = txPower->sleMaxPower;
}

void DevdRemoveAdv(void *arg)
{
    uint8_t *advHandle = (uint8_t *)arg;
    NLSTK_CHECK_RETURN_VOID(advHandle != NULL, "[DEVD]advHandle is null");
    DevdAdvNode_S *node = NULL;
    NLSTK_LOG_INFO("[DEVD]remove adv, adv handle = 0x%x", *advHandle);
    node = DevdGetAdvNode(*advHandle, DEVD_ADV_LIST);
    NLSTK_CHECK_RETURN_VOID(node != NULL, "[DEVD]adv node is null");
    NLSTK_CHECK_RETURN_VOID(node->advStatus != ADV_ENABLED, "[DEVD]adv already enabled, disable first");
    node->status = DEVD_SLE_STATUS_REMOVE_ADV;
    DLI_RemoveAdvSet(node->handle);
}

static bool IsMaxAdvNodes(void)
{
    return SDF_DListCount(DEVD_ADV_LIST) >= CfgdbGetMaxAdvNodesNum();
}

uint8_t DevdCreateAdvHandle(NLSTK_DevdAdvEventCbk cbk)
{
    uint8_t newHandle = DEVD_INVALID_ADV_HANDLE;
    SDF_DListHead_S *advList = DEVD_ADV_LIST;
    NLSTK_CHECK_RETURN(advList != NULL, newHandle, "[DEVD]adv list is null");
    NLSTK_CHECK_RETURN(IsMaxAdvNodes() != true, newHandle, "[DEVD]adv node is max");
    NLSTK_CHECK_RETURN(cbk != NULL, newHandle, "[DEVD]adv cbk is null");

    uint8_t maxHandleNum = CfgdbGetMaxAdvNodesNum();
    for (uint8_t i = 1; i <= maxHandleNum; i++) {
        uint8_t prevHandle = DevdGetPreAdvHandle(); 
        newHandle = prevHandle == DEVD_INVALID_ADV_HANDLE ? DEVD_LEGACY_ADV_HANDLE : (prevHandle + i) % maxHandleNum;
        DevdAdvNode_S *node = DevdGetAdvNode(newHandle, advList);
        if (node == NULL) {
            node = DevdCreateAdvNode(newHandle, cbk, advList);
            NLSTK_CHECK_RETURN(node != NULL, DEVD_INVALID_ADV_HANDLE, "[DEVD]create adv node fail");
            NLSTK_LOG_INFO("[DEVD]new handle = %u", newHandle);
            DevdSetPreAdvHandle(newHandle);
            return newHandle;
        }
    }

    NLSTK_LOG_ERROR("[DEVD]no valid handle");
    return DEVD_INVALID_ADV_HANDLE;
}

void DevdSetAdvParamCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes)
{
    DLI_AdvCbkContext *cbkContext = (DLI_AdvCbkContext *)context;
    NLSTK_CHECK_RETURN_VOID(cbkContext != NULL, "[DEVD]cbk context is null");
    uint8_t advHandle = cbkContext->advHandle;

    DevdAdvNode_S *node = DevdGetAdvNode(advHandle, DEVD_ADV_LIST);
    NLSTK_CHECK_RETURN_VOID(node != NULL, "[DEVD]adv node is null");
    if (status != DLI_SUCCESS) {
        // 设置广播参数失败，回报service开启广播失败
        NLSTK_DevdAdvCbkParam_S cbkParam = {0};
        NLSTK_LOG_INFO("[DEVD]set adv param cbk, advHandle = 0x%x, status = %d", advHandle, status);
        cbkParam.advHandle = advHandle;
        cbkParam.event = DEVD_CBK_EVENT_ENABLE_ADV;
        NLSTK_CHECK_RETURN_VOID(status <= UINT8_MAX, "[DEVD]adv param too long");
        cbkParam.result = (uint8_t)status;
        if (node->cbk != NULL) {
            node->cbk(&cbkParam);
        }

        SDF_MemFree(node->tempParam);
        node->tempParam = NULL;
        DevdFreeAdvData(node->tempData);
        node->tempData = NULL;
        if (node->param == NULL) {
            DevdRemoveAdvNode(node->handle, DEVD_ADV_LIST);
        } else {
            node->status = DEVD_SLE_STATUS_IDLE;
        }
        return;
    }
    if (node->param != NULL) {
        SDF_MemFree(node->param);
    }
    node->param = node->tempParam;
    node->tempParam = NULL;
    node->status = DEVD_SLE_STATUS_SET_ADV_DATA;
    node->dataOp.dataOffset = 0;
    node->dataOp.sendLen = 0;
    SendAdvDataToDli(node);
    SDF_UNUSED(cmdRes);
    return;
}

static uint8_t UpdateAdvDataStatus(DevdAdvNode_S *node)
{
    uint16_t dataLen = 0;
    uint8_t ret = DEVD_CBK_EVENT_MAX;
    if (node->status == DEVD_SLE_STATUS_SET_ADV_DATA) {
        dataLen = node->tempData->advDataLen;
    } else {
        dataLen = node->tempData->scanRspDataLen;
    }
    if (dataLen > node->dataOp.dataOffset) {
        /* 广播数据未发送完，状态不变，继续发送 */
        return ret;
    }
    node->dataOp.dataOffset = 0;
    node->dataOp.sendLen = 0;
    if (node->status == DEVD_SLE_STATUS_SET_ADV_DATA) {
        ret = DEVD_CBK_EVENT_SET_ADV_DATA;
        if (node->tempData->scanRspDataLen != 0 && node->param->basic.advMode >= ADV_MODE_NONCONN_SCANABLE) {
            node->status = DEVD_SLE_STATUS_SET_SCAN_RSP_DATA;
        } else {
            node->status = DEVD_SLE_STATUS_IDLE;
        }
    } else {
        ret = DEVD_CBK_EVENT_SET_SCAN_RSP_DATA;
        node->status = DEVD_SLE_STATUS_IDLE;
    }

    return ret;
}

// 单独处理更新广播数据指令的回调
static void DevdUpdateAdvDataHandle(uint8_t event, uint8_t advHandle, uint16_t status, DevdAdvNode_S *node)
{
    if (event != DEVD_CBK_EVENT_MAX) {
        NLSTK_DevdAdvCbkParam_S cbkParam = {0};
        cbkParam.advHandle = advHandle;
        NLSTK_CHECK_RETURN_VOID(status <= UINT8_MAX, "[DEVD]adv param too long");
        cbkParam.result = (uint8_t)status;
        cbkParam.event = event;    // 分别通知service设置广播数据完成和设置扫描响应数据完成（如果有扫描响应数据的话）
        if (node->cbk != NULL) {
            node->cbk(&cbkParam);
        }
    }
    if (status != DLI_SUCCESS) {
        DevdFreeAdvData(node->tempData);
        node->tempData = NULL;
        if (node->data == NULL) {
            DevdRemoveAdvNode(node->handle, DEVD_ADV_LIST);
        } else {
            node->status = DEVD_SLE_STATUS_IDLE;
        }
        return;
    }
    if (node->status == DEVD_SLE_STATUS_IDLE) {
        DevdFreeAdvData(node->data);
        node->data = node->tempData;
        node->tempData = NULL;
    } else {
        NLSTK_LOG_INFO("[DEVD]set more adv data, advHandle = 0x%x", advHandle);
        SendAdvDataToDli(node);
    }
}

// 单独处理设置广播数据指令的回调，如果为设置广播数据指令，失败通知service，成功继续enable广播
static void DevdSetAdvDataHandle(uint8_t event, uint8_t advHandle, uint16_t status, DevdAdvNode_S *node)
{
    // 设置失败直接直接返回enable的event，通知service使能广播失败
    if (status != DLI_SUCCESS) {
        DevdFreeAdvData(node->tempData);
        node->tempData = NULL;
        if (node->data == NULL) {
            DevdRemoveAdvNode(node->handle, DEVD_ADV_LIST);
        } else {
            node->status = DEVD_SLE_STATUS_IDLE;
        }
        NLSTK_DevdAdvCbkParam_S cbkParam = {0};
        cbkParam.advHandle = advHandle;
        NLSTK_CHECK_RETURN_VOID(status <= UINT8_MAX, "[DEVD]adv param too long");
        cbkParam.result = (uint8_t)status;
        cbkParam.event = DEVD_CBK_EVENT_ENABLE_ADV;
        if (node->cbk != NULL) {
            node->cbk(&cbkParam);
        }
        return;
    }

    // 广播数据设置成功后不回报给service，直接在协议栈处理下一步操作
    if (node->status == DEVD_SLE_STATUS_IDLE) {         // 这里表示数据全部发送完成
        DevdFreeAdvData(node->data);
        node->data = node->tempData;
        node->tempData = NULL;

        NLSTK_DevdSetAdvEnable_S advEnable = {0};
        advEnable.advHandle = advHandle;
        advEnable.enable = true;
        DevdEnableAdv(&advEnable);
    } else {                                            // 数据没有发送完，继续发送
        NLSTK_LOG_INFO("[DEVD]set more adv data, advHandle = 0x%x", advHandle);
        SendAdvDataToDli(node);
    }
    SDF_UNUSED(event);
}

// 设置广播和更新广播都会走这个回调，因此在回调中需要区分当前操作是设置还是更新的回调
void DevdSetAdvDataCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes)
{
    DLI_AdvCbkContext *cbkContext = (DLI_AdvCbkContext *)context;
    NLSTK_CHECK_RETURN_VOID(cbkContext != NULL, "[DEVD]cbk context is null");
    uint8_t advHandle = cbkContext->advHandle;
    NLSTK_LOG_INFO("[DEVD]set adv data cbk, advHandle = 0x%x, status = %d", advHandle, status);
    DevdAdvNode_S *node = DevdGetAdvNode(advHandle, DEVD_ADV_LIST);
    NLSTK_CHECK_RETURN_VOID(node != NULL, "[DEVD]adv node is null");
    NLSTK_CHECK_RETURN_VOID(node->tempData != NULL, "[DEVD]no adv data is setting");
    NLSTK_DevdAdvCbkParam_S cbkParam = {0};
    cbkParam.advHandle = advHandle;
    NLSTK_CHECK_RETURN_VOID(status <= UINT8_MAX, "[DEVD]adv param too long");
    cbkParam.result = (uint8_t)status;
    uint8_t event = UpdateAdvDataStatus(node);
    
    // 如果为更新广播数据指令，通知service结果
    if (node->isUpdateData) {
        DevdUpdateAdvDataHandle(event, advHandle, status, node);
        return;
    // 否则进行设置广播数据指令的处理
    } else {
        DevdSetAdvDataHandle(event, advHandle, status, node);
        return;
    }
    SDF_UNUSED(cmdRes);
}

void DevdEnableAdvCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes)
{
    DLI_AdvCbkContext *cbkContext = (DLI_AdvCbkContext *)context;
    NLSTK_CHECK_RETURN_VOID(cbkContext != NULL, "[DEVD]cbk context is null");
    uint8_t advHandle = cbkContext->advHandle;
    NLSTK_DevdAdvCbkParam_S cbkParam = {0};
    uint8_t msg = 0;
    NLSTK_LOG_INFO("[DEVD]set adv enable cbk, advHandle = 0x%x, status = %d", advHandle, status);
    DevdAdvNode_S *node = DevdGetAdvNode(advHandle, DEVD_ADV_LIST);
    NLSTK_CHECK_RETURN_VOID(node != NULL, "[DEVD]adv node is null");
    if (node->status == DEVD_SLE_STATUS_ENABLE_ADV) {
        msg = DEVD_CBK_EVENT_ENABLE_ADV;
    } else {
        msg = DEVD_CBK_EVENT_DISABLE_ADV;
    }
    cbkParam.event = msg;
    cbkParam.advHandle = advHandle;
    NLSTK_CHECK_RETURN_VOID(status <= UINT8_MAX, "[DEVD]adv param too long");
    cbkParam.result = (uint8_t)status;
    if (node->cbk != NULL) {
        node->cbk(&cbkParam);
    }
    if (status == DLI_SUCCESS) {
        node->advStatus = msg;
    }
    node->status = DEVD_SLE_STATUS_IDLE;
    SDF_UNUSED(cmdRes);
    return;
}

void DevdSetTxPowerCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes)
{
    SDF_UNUSED(context);
    NLSTK_LOG_INFO("[DEVD]set txPower enable cbk, status = %d", status);
    SDF_UNUSED(cmdRes);
    return;
}

void DevdRemoveAdvCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes)
{
    DLI_AdvCbkContext *cbkContext = (DLI_AdvCbkContext *)context;
    NLSTK_CHECK_RETURN_VOID(cbkContext != NULL, "[DEVD]cbk context is null");
    uint8_t advHandle = cbkContext->advHandle;
    NLSTK_DevdAdvCbkParam_S cbkParam = {0};
    NLSTK_LOG_INFO("[DEVD]remove adv cbk, advHandle = 0x%x, status = %d", advHandle, status);
    DevdAdvNode_S *node = DevdGetAdvNode(advHandle, DEVD_ADV_LIST);
    NLSTK_CHECK_RETURN_VOID(node != NULL, "[DEVD]adv node is null");
    cbkParam.event = DEVD_CBK_EVENT_REMOVE_ADV;
    cbkParam.advHandle = advHandle;
    NLSTK_CHECK_RETURN_VOID(status <= UINT8_MAX, "[DEVD]adv param too long");
    cbkParam.result = (uint8_t)status;
    if (node->cbk != NULL) {
        node->cbk(&cbkParam);
    }
    if (status == DLI_SUCCESS) {
        DevdRemoveAdvNode(node->handle, DEVD_ADV_LIST);
    }
    SDF_UNUSED(cmdRes);
    return;
}

void DevdAdvTerminatedCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    if (cmdRes == NULL || cmdRes->eventParameter == NULL) {
        NLSTK_LOG_ERROR("[DEVD]cmd res is null");
        return;
    }
    if (status != DLI_SUCCESS || cmdRes->size < sizeof(DevdAdvTerminatedEvent_S)) {
        NLSTK_LOG_ERROR("[DEVD]cmd res is wrong");
        return;
    }
    DevdAdvTerminatedEvent_S *res = (DevdAdvTerminatedEvent_S *)cmdRes->eventParameter;
    NLSTK_DevdAdvCbkParam_S cbkParam = {0};
    NLSTK_LOG_INFO("[DEVD]adv terminated, status = 0x%x, adv handle = 0x%x, connection handle = 0x%x",
        res->status, res->advHandle, res->connHandle);
    cbkParam.advHandle = res->advHandle;
    cbkParam.result = res->status;
    cbkParam.event = DEVD_CBK_EVENT_TERMINATED_ADV;
    DevdAdvNode_S *node = DevdGetAdvNode(res->advHandle, DEVD_ADV_LIST);
    NLSTK_CHECK_RETURN_VOID(node != NULL, "[DEVD]adv node is null");
    if (node->cbk != NULL) {
        node->cbk(&cbkParam);
    }
    NLSTK_CHECK_RETURN_VOID(res->status == DLI_SUCCESS, "[DEVD]adv terminated event failed");
    node->advStatus = ADV_DISABLED;
    node->status = DEVD_SLE_STATUS_IDLE;
}
