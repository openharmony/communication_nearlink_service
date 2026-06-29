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
#include "devd_tbl.h"
#include "devd_local.h"
#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "nlstk_cfgdb.h"
#include "securec.h"

void DevdFreeAdvData(NLSTK_DevdAdvData_S *data)
{
    if (data == NULL) {
        return;
    }
    if (data->advData != NULL) {
        SDF_MemFree(data->advData);
    }
    if (data->scanRspData != NULL) {
        SDF_MemFree(data->scanRspData);
    }
    SDF_MemFree(data);
}

void DevdFreeSetAdvData(void *arg)
{
    NLSTK_DevdSetAdvData_S *data = (NLSTK_DevdSetAdvData_S *)arg;
    if (data == NULL) {
        return;
    }
    if (data->data.advData != NULL) {
        SDF_MemFree(data->data.advData);
    }
    if (data->data.scanRspData != NULL) {
        SDF_MemFree(data->data.scanRspData);
    }
    SDF_MemFree(data);
}

void DevdFreeSetAdvParams(void *arg)
{
    DevdSetAdvParams_S *params = (DevdSetAdvParams_S *)arg;
    if (params == NULL) {
        return;
    }
    if (params->data.advData != NULL) {
        SDF_MemFree(params->data.advData);
    }
    if (params->data.scanRspData != NULL) {
        SDF_MemFree(params->data.scanRspData);
    }
    SDF_MemFree(params);
}

void DevdFreeAdvNode(DevdAdvNode_S *node)
{
    if (node == NULL) {
        return;
    }
    if (node->param != NULL) {
        SDF_MemFree(node->param);
    }
    if (node->tempParam != NULL) {
        SDF_MemFree(node->tempParam);
    }
    DevdFreeAdvData(node->data);
    DevdFreeAdvData(node->tempData);
    SDF_MemFree(node);
}

DevdAdvNode_S *DevdCreateAdvNode(uint8_t handle, NLSTK_DevdAdvEventCbk cbk, SDF_DListHead_S *list)
{
    NLSTK_CHECK_RETURN(list != NULL, NULL, "[DEVD]adv list is null");
    NLSTK_CHECK_RETURN(SDF_DListCount(list) < CfgdbGetMaxAdvNodesNum(), NULL, "[DEVD]adv nodes over max adv num");
    NLSTK_CHECK_RETURN(cbk != NULL, NULL, "[DEVD]adv cbk is null");
    /* free adv node when deinit or remove by DevdFreeAdvNode */
    DevdAdvNode_S *node = (DevdAdvNode_S *)SDF_MemZalloc(sizeof(DevdAdvNode_S));
    NLSTK_CHECK_RETURN(node != NULL, NULL, "[DEVD]memory alloc error");
    (void)memset_s(node, sizeof(DevdAdvNode_S), 0, sizeof(DevdAdvNode_S));
    node->handle = handle;
    node->status = DEVD_SLE_STATUS_IDLE;
    node->isUpdateData = false;
    node->cbk = cbk;
    SDF_DListEntryInit(&node->entry);
    SDF_DListElmTailInsert(list, node, entry);

    return node;
}

DevdAdvNode_S *DevdGetAdvNode(uint8_t handle, SDF_DListHead_S *list)
{
    NLSTK_CHECK_RETURN(list != NULL, NULL, "[DEVD]adv list is null");
    DevdAdvNode_S *node = NULL;
    DevdAdvNode_S *tmp = NULL;
    SDF_DListElmForeach(tmp, list, entry) {
        if (tmp->handle == handle) {
            node = tmp;
            break;
        }
    }

    return node;
}

void DevdRemoveAdvNode(uint8_t handle, SDF_DListHead_S *list)
{
    DevdAdvNode_S *node = NULL;
    node = DevdGetAdvNode(handle, list);
    NLSTK_CHECK_RETURN_VOID(node != NULL, "[DEVD]adv node is null");
    SDF_DListElmDel(list, node, entry);
    DevdFreeAdvNode(node);
}

uint32_t DevdSaveAdvParamToTbl(DevdAdvNode_S *node, DevdAdvParam_S *param)
{
    if (node->tempParam != NULL) {
        SDF_MemFree(node->tempParam);
        node->tempParam = NULL;
    }
    node->tempParam = (DevdAdvParam_S *)SDF_MemZalloc(sizeof(DevdAdvParam_S));
    NLSTK_CHECK_RETURN(node->tempParam != NULL, NLSTK_ERR, "[DEVD]memory alloc error");
    (void)memcpy_s(node->tempParam, sizeof(DevdAdvParam_S), param, sizeof(DevdAdvParam_S));
    return NLSTK_OK;
}

uint32_t DevdSaveAdvDataToTbl(DevdAdvNode_S *node, NLSTK_DevdAdvData_S *data)
{
    if (node->tempData != NULL) {
        DevdFreeAdvData(node->tempData);
        node->tempData = NULL;
    }
    NLSTK_CHECK_RETURN(data->advDataLen <= DEFAULT_MAX_ADV_DATA_LEN, NLSTK_ERR, "[DEVD]adv len error");
    NLSTK_CHECK_RETURN(data->scanRspDataLen <= DEFAULT_MAX_ADV_DATA_LEN, NLSTK_ERR, "[DEVD]adv scan rsp len error");
    node->tempData = (NLSTK_DevdAdvData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvData_S));
    NLSTK_CHECK_RETURN(node->tempData != NULL, NLSTK_ERR, "[DEVD]memory alloc error");
    node->tempData->advDataLen = data->advDataLen;
    node->tempData->scanRspDataLen = data->scanRspDataLen;
    node->tempData->advData = (uint8_t *)SDF_MemZalloc(node->tempData->advDataLen);
    if (node->tempData->advData == NULL) {
        NLSTK_LOG_ERROR("[DEVD]memory alloc error");
        SDF_MemFree(node->tempData);
        node->tempData = NULL;
        return NLSTK_ERR;
    }
    // node->tempData->advDataLen与data->advDataLen值相等
    (void)memcpy_s(node->tempData->advData, node->tempData->advDataLen, data->advData, data->advDataLen);
    if (node->tempData->scanRspDataLen != 0) {
        node->tempData->scanRspData = (uint8_t *)SDF_MemZalloc(node->tempData->scanRspDataLen);
        if (node->tempData->scanRspData == NULL) {
            NLSTK_LOG_ERROR("[DEVD]memory alloc error");
            SDF_MemFree(node->tempData->advData);
            SDF_MemFree(node->tempData);
            node->tempData = NULL;
            return NLSTK_ERR;
        }
        // node->tempData->scanRspDataLen与data->scanRspDataLen值相等
        (void)memcpy_s(node->tempData->scanRspData, node->tempData->scanRspDataLen,
            data->scanRspData, data->scanRspDataLen);
    }
    return NLSTK_OK;
}