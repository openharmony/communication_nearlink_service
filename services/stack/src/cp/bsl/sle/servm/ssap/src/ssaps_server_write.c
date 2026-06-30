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
#include <stdio.h>
#include "securec.h"
#include "sdf_mem.h"
#include "sdf_string.h"
#include "sdf_buff.h"
#include "sdf_trace.h"

#include "nlstk_sm.h"

#include "cpfwk_log.h"
#include "ssap_type.h"
#include "ssap_pkt.h"
#include "ssap_link.h"
#include "ssap_utils.h"
#include "ssap_manager.h"
#include "ssap_common.h"
#include "ssaps_service.h"
#include "nlstk_log.h"
#include "ssaps_server_api.h"
#include "ssaps_server_app.h"
#include "ssaps_server.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_WRITE_ERROR_COUNT 255

static bool CompDescriptorType(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    SSAP_Descriptor_S *descriptor = (SSAP_Descriptor_S *)ptr;
    uint8_t *type = (uint8_t *)args;
    return descriptor->type == *type;
}

static bool CompClientConfigAddr(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    SSAP_ClientPropertyConfigDescriptor_S *clientConfig = (SSAP_ClientPropertyConfigDescriptor_S *)ptr;
    uint8_t *addr = (uint8_t *)args;
    return memcmp(clientConfig->addr.addr, addr, SLE_ADDR_LEN) == 0;
}

static SDF_Buff_S *SSAPS_BuildWriteRspPayload(uint8_t errorCode, SSAP_BufferedOperation_S *operation)
{
    uint32_t realSize;
    if ((operation->controlCode & SSAP_WRITE_REQ_NEED_ORIGIN_RET) != 0) {
        realSize = sizeof(SSAP_PduWriteRsp_S) + sizeof(SSAP_PduWriteRspItem_S) + operation->value.len;
    } else {
        realSize = sizeof(SSAP_PduWriteRsp_S);
    }
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN(sdfBuff != NULL, NULL, "[SSAP] write rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] write rsp create buf fail");
        return NULL;
    }
    SSAP_PduWriteRsp_S *writeRsp = (SSAP_PduWriteRsp_S *)buf;
    writeRsp->msgCode = SSAP_WRITE_RSP;
    writeRsp->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    writeRsp->ctrl.result = (errorCode == SSAP_ERRCODE_SUCCESS) ? SSAP_CTRL_WRITE_SUCCESS : SSAP_CTRL_WRITE_PART;
    if ((operation->controlCode & SSAP_WRITE_REQ_NEED_ORIGIN_RET) != 0) {
        SSAP_PduWriteRspItem_S *writeRspItem = (SSAP_PduWriteRspItem_S *)(writeRsp->items);
        writeRspItem->handle = operation->propertyHandle;
        writeRspItem->type = operation->dataType;
        (void)memcpy_s(writeRspItem->value, operation->value.len, operation->value.value, operation->value.len);
    }
    return sdfBuff;
}

static SDF_Buff_S *SSAPS_BuildWriteRspErrorPayload(uint8_t errorCode, SSAP_BufferedOperation_S *operation)
{
    uint32_t realSize = (sizeof(SSAP_PduWriteRsp_S) + sizeof(SSAP_PduWriteRspErrorInfo_S) +
        sizeof(SSAP_PduWriteRspErrorItem_S));
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN(sdfBuff != NULL, NULL, "[SSAP] write error rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] write error rsp create buf fail");
        return NULL;
    }
    SSAP_PduWriteRsp_S *writeRsp = (SSAP_PduWriteRsp_S *)buf;
    writeRsp->msgCode = SSAP_WRITE_RSP;
    writeRsp->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    writeRsp->ctrl.result = SSAP_CTRL_WRITE_PART;
    SSAP_PduWriteRspErrorInfo_S *writeRspErrorItem = (SSAP_PduWriteRspErrorInfo_S *)(writeRsp->items);
    writeRspErrorItem->errorNum = SSAP_WRITE_ERROR_SINGLE_NUM;
    SSAP_PduWriteRspErrorItem_S *errList = writeRspErrorItem->errList;
    errList[0].handle = operation->propertyHandle;
    errList[0].errorCode = errorCode;
    return sdfBuff;
}

static void SSAPS_UpdatePropertyValueCpcd(SSAP_Descriptor_S *descriptor, SLE_Addr_S *addr,
    SSAP_LengthValue_S *value)
{
    size_t index = 0;
    CP_LOG_INFO("[SSAP] update cpcd value len %d, val: %s", value->len, SDF_GET_UINT8_STR(value->value, value->len));
    CP_CHECK_LOG_RETURN_VOID(descriptor->clientConfigs != NULL, "[SSAP] update cpcd vector null");
    SSAP_LengthValue_S *newConfigVal = (SSAP_LengthValue_S *)SDF_MemZalloc(sizeof(SSAP_LengthValue_S) + value->len);
    CP_CHECK_LOG_RETURN_VOID(newConfigVal != NULL, "[SSAP] update cpcd malloc fail");
    newConfigVal->len = value->len;
    (void)memcpy_s(newConfigVal->value, value->len, value->value, value->len);
    if (SDF_VectorFindFirst(descriptor->clientConfigs, CompClientConfigAddr, addr->addr, &index)) {
        SSAP_ClientPropertyConfigDescriptor_S *clientConfig = SDF_VectorElementAt(descriptor->clientConfigs, index);
        SDF_MemFree(clientConfig->val);
        clientConfig->val = newConfigVal;
    } else {
        SSAP_ClientPropertyConfigDescriptor_S *clientConfigNew =
            (SSAP_ClientPropertyConfigDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ClientPropertyConfigDescriptor_S));
        if (clientConfigNew == NULL) {
            CP_LOG_ERROR("[SSAP] update cpcd malloc fail");
            SDF_MemFree(newConfigVal);
            return;
        }
        (void)memcpy_s(&clientConfigNew->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
        clientConfigNew->type = DESC_TYPE_CLIENT_CONFIG;
        clientConfigNew->val = newConfigVal;
        if (!SDF_VectorEmplaceBack(descriptor->clientConfigs, clientConfigNew)) {
            CP_LOG_ERROR("[SSAP] update cpcd emplace back fail");
            SDF_MemFree(newConfigVal);
            SDF_MemFree(clientConfigNew);
            return;
        }
    }
}

uint8_t SSAPS_UpdatePropertyValue(SSAP_Property_S *property, uint8_t type, SSAP_LengthValue_S *value,
    SLE_Addr_S *addr, bool isRmt)
{
    size_t index = 0;
    SSAP_LengthValue_S *newVal = (SSAP_LengthValue_S *)SDF_MemZalloc(sizeof(SSAP_LengthValue_S) + value->len);
    CP_CHECK_LOG_RETURN(newVal != NULL, SSAP_ERRCODE_NO_RESOURCE, "[SSAP] update property value malloc fail");
    newVal->len = value->len;
    (void)memcpy_s(newVal->value, newVal->len, value->value, value->len);
    if (type == SSAP_TYPE_DATA) {
        SDF_MemFree(property->val);
        property->val = newVal;
        return SSAP_ERRCODE_SUCCESS;
    } else if (SDF_VectorFindFirst(property->descriptors, CompDescriptorType, &type, &index)) {
        SSAP_Descriptor_S *descriptor = SDF_VectorElementAt(property->descriptors, index);
        SDF_MemFree(descriptor->val);
        descriptor->val = newVal;
        if (type == DESC_TYPE_CLIENT_CONFIG && isRmt) {
            SSAPS_UpdatePropertyValueCpcd(descriptor, addr, value);
        }
        return SSAP_ERRCODE_SUCCESS;
    }
    SDF_MemFree(newVal);
    return SSAP_ERRCODE_DATA_TYPE;
}

void SSAPS_SendWriteReqRsp(SSAP_Link_S *link, uint8_t errorCode, SSAP_BufferedOperation_S *operation)
{
    SDF_Buff_S *sdfBuff = NULL;
    if (errorCode != SSAP_ERRCODE_SUCCESS) {
        sdfBuff = SSAPS_BuildWriteRspErrorPayload(errorCode, operation);
        CP_LOG_ERROR("[SSAP] write rsp error, error code: %d", errorCode);
    } else {
        sdfBuff = SSAPS_BuildWriteRspPayload(errorCode, operation);
    }
    if (sdfBuff == NULL) {
        CP_LOG_ERROR("[SSAP] write rsp buff is null");
        return;
    }
    link->sendFunc(link, sdfBuff, SSAP_WRITE_RSP);
}

static uint32_t SSAPS_CalcWriteResultItemSize(SSAP_WriteResultItem_S *result)
{
    uint32_t size = 0;
    for (uint8_t i = 0; i < result->originDataCount; i++) {
        if (result->originData[i].value != NULL && result->originData[i].value->len > 0) {
            size += sizeof(SSAP_PduWriteMultiSubItem_S) + result->originData[i].value->len;
        }
    }
    return size;
}

static uint32_t SSAPS_CalcMultiWriteRspSize(SSAP_WriteResultItem_S *results, uint16_t resultCount,
    uint8_t controlCode)
{
    uint32_t size = sizeof(SSAP_PduWriteRsp_S);
    bool needOriginReturn = ((controlCode & SSAP_WRITE_REQ_NEED_ORIGIN_RET) != 0);

    bool hasError = false;
    uint8_t errorCount = 0;
    uint32_t originDataSize = 0;

    for (uint16_t i = 0; i < resultCount; i++) {
        if (results[i].errorCode != SSAP_ERRCODE_SUCCESS) {
            errorCount++;
            if (errorCount == MAX_WRITE_ERROR_COUNT) {
                return 0;
            }
            hasError = true;
        } else if (needOriginReturn) {
            originDataSize += sizeof(SSAP_PduWriteMultiItem_S);
            originDataSize += SSAPS_CalcWriteResultItemSize(&results[i]);
        }
    }

    if (hasError) {
        size += sizeof(SSAP_PduWriteRspErrorInfo_S) + (errorCount * sizeof(SSAP_PduWriteRspErrorItem_S));
    }
    size += originDataSize;

    CP_LOG_INFO("size=%d, resultCount=%d, errorCount=%d", size, resultCount, errorCount);

    return size;
}

static uint8_t* SSAPS_AppendMultiOriginValue(uint8_t *curBuf, const SSAP_WriteResultItem_S *result)
{
    (void)memcpy_s(curBuf, sizeof(uint16_t), &result->handle, sizeof(uint16_t));
    curBuf += sizeof(uint16_t);
    *curBuf = result->originDataCount;
    curBuf += sizeof(uint8_t);

    for (uint8_t i = 0; i < result->originDataCount; i++) {
        if (result->originData[i].value != NULL && result->originData[i].value->len > 0) {
            SSAP_PduWriteMultiSubItem_S *subItem = (SSAP_PduWriteMultiSubItem_S *)curBuf;
            subItem->type = result->originData[i].dataType;
            subItem->len = result->originData[i].value->len;
            (void)memcpy_s(subItem->value, subItem->len,
                           result->originData[i].value->value, subItem->len);
            curBuf += sizeof(SSAP_PduWriteMultiSubItem_S) + subItem->len;
        }
    }
    return curBuf;
}

static bool SSAPS_HasError(SSAP_WriteResultItem_S *results, uint16_t resultCount)
{
    for (uint16_t i = 0; i < resultCount; i++) {
        if (results[i].errorCode != SSAP_ERRCODE_SUCCESS) {
            return true;
        }
    }
    return false;
}

static void SSAPS_BuildMultiWriteErrorRsp(SSAP_PduWriteRsp_S *writeRsp, SSAP_WriteResultItem_S *results,
    uint16_t resultCount, bool needOriginReturn)
{
    writeRsp->ctrl.result = SSAP_CTRL_WRITE_PART;

    SSAP_PduWriteRspErrorInfo_S *errorInfo = (SSAP_PduWriteRspErrorInfo_S *)writeRsp->items;
    uint8_t *errorListCur = (uint8_t *)(errorInfo->errList);
    uint8_t errorCount = 0;
    for (uint16_t i = 0; i < resultCount; i++) {
        if (results[i].errorCode != SSAP_ERRCODE_SUCCESS) {
            // handle
            (void)memcpy_s(errorListCur, sizeof(uint16_t), &results[i].handle, sizeof(uint16_t));
            errorListCur += sizeof(uint16_t);

            // errorCode
            *errorListCur = results[i].errorCode;
            errorListCur += sizeof(uint8_t);
            errorCount++;
        }
    }
    errorInfo->errorNum = errorCount;

    uint8_t *curBuf = (uint8_t *)errorListCur;
    if (!needOriginReturn) {
        return;
    }
    for (uint16_t i = 0; i < resultCount; i++) {
        if (results[i].errorCode == SSAP_ERRCODE_SUCCESS) {
            curBuf = SSAPS_AppendMultiOriginValue(curBuf, &results[i]);
        }
    }
}

static void SSAPS_BuildMultiWriteSuccessRsp(SSAP_PduWriteRsp_S *writeRsp, SSAP_WriteResultItem_S *results,
    uint16_t resultCount)
{
    writeRsp->ctrl.result = SSAP_CTRL_WRITE_SUCCESS;
    uint8_t *curBuf = (uint8_t *)writeRsp->items;
    for (uint16_t i = 0; i < resultCount; i++) {
        curBuf = SSAPS_AppendMultiOriginValue(curBuf, &results[i]);
    }
}

static void SSAPS_SendMultiWriteReqRsp(SSAP_Link_S *link, SSAP_WriteResultItem_S *results,
    uint16_t resultCount, uint8_t controlCode)
{
    if (results == NULL || resultCount == 0) {
        return;
    }

    bool needOriginReturn = ((controlCode & SSAP_WRITE_REQ_NEED_ORIGIN_RET) != 0);
    uint32_t realSize = SSAPS_CalcMultiWriteRspSize(results, resultCount, controlCode);
    if (realSize == 0) {
        CP_LOG_ERROR("realSize == 0");
        SSAP_PduErrorRsp(link, SSAP_WRITE_REQ, 0, SSAP_ERRCODE_INVALID_PDU);
        return;
    }

    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] multi write rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] multi write rsp create buf fail");
        return;
    }

    SSAP_PduWriteRsp_S *writeRsp = (SSAP_PduWriteRsp_S *)buf;
    writeRsp->msgCode = SSAP_WRITE_RSP;
    writeRsp->ctrl.fragment = SSAP_CTRL_NO_FRAG;

    if (SSAPS_HasError(results, resultCount)) {
        SSAPS_BuildMultiWriteErrorRsp(writeRsp, results, resultCount, needOriginReturn);
    } else if (needOriginReturn) {
        SSAPS_BuildMultiWriteSuccessRsp(writeRsp, results, resultCount);
    } else {
        writeRsp->ctrl.result = SSAP_CTRL_WRITE_SUCCESS;
    }

    link->sendFunc(link, sdfBuff, SSAP_WRITE_RSP);
}

static uint8_t *SSAPS_GetNextMultiItemPos(SSAP_PduWriteMultiItem_S *multiItem, size_t *leftSize)
{
    if (*leftSize < sizeof(SSAP_PduWriteMultiItem_S)) {
        CP_LOG_ERROR("[SSAP] leftSize < sizeof(SSAP_PduWriteMultiItem_S)");
        return NULL;
    }
    *leftSize -= sizeof(SSAP_PduWriteMultiItem_S);
    SSAP_PduWriteMultiSubItem_S *subItem = multiItem->subItem;
    for (uint8_t i = 0; i < multiItem->subItemCount; i++) {
        if (*leftSize < sizeof(SSAP_PduWriteMultiSubItem_S) ||
            *leftSize - sizeof(SSAP_PduWriteMultiSubItem_S) < subItem->len) {
            CP_LOG_ERROR("[SSAP] leftSize < sizeof(SSAP_PduWriteMultiSubItem_S)");
            return NULL;
        }
        *leftSize -= (sizeof(SSAP_PduWriteMultiSubItem_S) + subItem->len);
        subItem = (SSAP_PduWriteMultiSubItem_S *)((uint8_t *)subItem +
            sizeof(SSAP_PduWriteMultiSubItem_S) + subItem->len);
    }
    CP_LOG_DEBUG("leftSize=%d", (int32_t)*leftSize);
    return (uint8_t *)subItem;
}

static uint32_t SSAPS_CalculateWriteItemCount(SSAP_Link_S *link, SSAP_PduWriteMultiItem_S *multiItem, uint16_t len)
{
    size_t leftSize = (size_t)len - sizeof(SSAP_PduWriteReq_S);
    uint32_t itemCount = 0;

    CP_LOG_DEBUG("leftSize=%d", (int32_t)leftSize);
    while (leftSize > 0) {
        itemCount++;
        multiItem = (SSAP_PduWriteMultiItem_S *)SSAPS_GetNextMultiItemPos(multiItem, &leftSize);
        if (multiItem == NULL) {
            CP_LOG_ERROR("[SSAP] SSAPS_GetNextMultiItemPos fail");
            return 0;
        }
        CP_LOG_DEBUG("itemCount=%d, leftSize=%d", itemCount, (int32_t)leftSize);
    }
    return itemCount;
}

static uint8_t SSAPS_WriteControlCheck(SSAP_Link_S *link, uint32_t opIndication,
    uint8_t permissions, uint8_t type, uint8_t opCode)
{
    uint32_t writeIndication = 0;
    switch (type) {
        case SSAP_TYPE_DATA:
            if (opCode == SSAP_WRITE_CMD) {
                writeIndication = opIndication & SSAP_OPERATE_INDICATION_WRITE_NO_RSP;
            } else if (opCode == SSAP_WRITE_REQ) {
                writeIndication = opIndication & SSAP_OPERATE_INDICATION_WRITE;
            }
            break;
        case DESC_TYPE_PROPERTY_INSTRUCTION:
            writeIndication = opIndication & SSAP_OPERATE_INDICATION_DESCRITOR_WRITE;
            break;
        case DESC_TYPE_CLIENT_CONFIG:
            writeIndication = opIndication & SSAP_OPERATE_INDICATION_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE;
            break;
        case DESC_TYPE_SERVER_CONFIG:
            writeIndication = opIndication & SSAP_OPERATE_INDICATION_DESCRIPTOR_SERVER_CONFIGURATION_WRITE;
            break;
        default:
            break;
    }
    if (writeIndication == 0) {
        return SSAP_ERRCODE_FORBID_WRITE;
    } else if (((permissions & (uint8_t)SSAP_PERMISSION_AUTHENTICATION_NEED) != 0) &&
        !SmIsSLinkAuthComplete(link->lcid)) {
        return SSAP_ERRCODE_UNAUTHENTICATED;
    } else if (((permissions & (uint8_t)SSAP_PERMISSION_ENCRYPTION_NEED) != 0) &&
        !SmIsSLinkEncryptComplete(link->lcid)) {
        return SSAP_ERRCODE_UNENCRYPTED;
    }
    return SSAP_ERRCODE_SUCCESS;
}

static SSAP_LoopControlType_E SSAPS_ValidateSubItem(SSAP_Link_S *link, SSAP_PduWriteMultiSubItem_S *subItem,
    SSAP_Property_S *property, uint8_t *errCode)
{
    if (subItem->type > DESC_TYPE_PROPERTY_FORMAT) {
        *errCode = SSAP_ERRCODE_DATA_TYPE;
        return LOOP_RET_TRUE;
    }

    uint8_t permissions = 0;
    uint32_t opIndication = 0;
    if (!SSAPS_GetPermissionAndOperation(property, &permissions, &opIndication, subItem->type)) {
        *errCode = SSAP_ERRCODE_DATA_TYPE;
        return LOOP_RET_TRUE;
    }

    *errCode = SSAPS_WriteControlCheck(link, opIndication, permissions, subItem->type, SSAP_WRITE_REQ);
    if (*errCode != SSAP_ERRCODE_SUCCESS) {
        return LOOP_RET_TRUE;
    }

    if ((permissions & (uint8_t)SSAP_PERMISSION_AUTHORIZATION_NEED) != 0) {
        CP_LOG_ERROR("[SSAP] multi write with auth item not supported");
        SSAP_PduErrorRsp(link, SSAP_WRITE_REQ, 0, SSAP_ERRCODE_UNSUPPORT_PDU);
        return LOOP_RET_FALSE;
    }

    if (subItem->len == 0) {
        return LOOP_CONTINUE;
    }

    return LOOP_NORMAL_EXECUTION;
}

static SSAP_BufferedOperation_S *SSAPS_WriteReqMultiItemHandleComposeOperation(SSAP_Link_S *link,
    SSAP_PduWriteReq_S *writeReq, uint16_t handle, SSAP_PduWriteMultiSubItem_S *writeReqItem)
{
    SSAP_BufferedOperation_S *operation = (SSAP_BufferedOperation_S *)SDF_MemZalloc(
        sizeof(SSAP_BufferedOperation_S) + writeReqItem->len);
    CP_CHECK_LOG_RETURN(operation != NULL, NULL, "[SSAP] write req handle compose operation malloc fail");
    operation->msgCode = writeReq->msgCode;
    operation->propertyHandle = handle;
    operation->dataType = writeReqItem->type;
    operation->value.len = writeReqItem->len;
    operation->controlCode = writeReq->msgCtrl;
    (void)memcpy_s(operation->value.value, writeReqItem->len, writeReqItem->value, writeReqItem->len);
    (void)memcpy_s(&operation->addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    CP_LOG_INFO("[SSAP] write req handle handle: %d, type: %d", writeReqItem->type);
    return operation;
}

static SSAP_LoopControlType_E SSAPS_ExecWriteMultiSubItem(SSAP_Link_S *link, SSAP_BufferedOperation_S *operation,
    SSAP_Property_S *property, SSAP_WriteResultItem_S *results)
{
    (void)memcpy_s(&operation->propertyUuid, sizeof(NLSTK_SsapUuid_S), &property->uuid, sizeof(NLSTK_SsapUuid_S));

    results->errorCode = SSAPS_UpdatePropertyValue(property, operation->dataType,
        &operation->value, &link->addr, true);
    if (results->errorCode != SSAP_ERRCODE_SUCCESS) {
        SDF_MemFree(operation);
        return LOOP_BREAK;
    }

    if (results->originDataCount >= DESC_TYPE_MAX) {
        CP_LOG_ERROR("[SSAP] originData overflow, count: %u", results->originDataCount);
        SSAP_PduErrorRsp(link, SSAP_WRITE_REQ, 0, SSAP_ERRCODE_UNSUPPORT_PDU);
        SDF_MemFree(operation);
        return LOOP_RET_FALSE;
    }

    SSAP_WriteOriginData_S *originDataItem = &results->originData[results->originDataCount++];
    originDataItem->dataType = operation->dataType;
    originDataItem->value = SSAPS_GetPropertyValue(link, property, operation->dataType, &results->errorCode);
    if (results->errorCode != SSAP_ERRCODE_SUCCESS) {
        SDF_MemFree(operation);
        return LOOP_BREAK;
    }

    SsapServerAppWriteValueCallback(operation);
    SDF_MemFree(operation);
    return LOOP_NORMAL_EXECUTION;
}

static bool SSAPS_WriteMultiHandleItem(SSAP_Link_S *link, SSAP_PduWriteReq_S *writeReq,
    SSAP_PduWriteMultiItem_S *multiItem, SSAP_WriteResultItem_S *results)
{
    results->handle = multiItem->handle;
    SSAP_Property_S *property = SSAPS_GetPropertyByHandle(multiItem->handle);
    if (property == NULL) {
        results->errorCode = SSAP_ERRCODE_INVALID_HANDLE;
        return true;
    }

    SSAP_PduWriteMultiSubItem_S *subItem = multiItem->subItem;
    for (uint8_t i = 0; i < multiItem->subItemCount; i++) {
        SSAP_LoopControlType_E ret = SSAPS_ValidateSubItem(link, subItem, property, &results->errorCode);
        if (ret == LOOP_RET_FALSE) {
            return false;
        } else if (ret == LOOP_RET_TRUE) {
            return true;
        } else if (ret == LOOP_CONTINUE) {
            subItem = (SSAP_PduWriteMultiSubItem_S *)((uint8_t *)subItem +
                sizeof(SSAP_PduWriteMultiSubItem_S) + subItem->len);
            continue;
        } // SSAPS_ValidateSubItem只返回LOOP_RET_FALSE/LOOP_RET_TRUE/LOOP_CONTINUE/LOOP_NORMAL_EXECUTION

        SSAP_BufferedOperation_S *operation = SSAPS_WriteReqMultiItemHandleComposeOperation(link, writeReq,
        multiItem->handle, subItem);
        CP_CHECK_LOG_RETURN(operation != NULL, false, "[SSAP] write req handle operation is null.");
        ret = SSAPS_ExecWriteMultiSubItem(link, operation, property, results);
        if (ret == LOOP_RET_FALSE) {
            return false;
        } else if (ret == LOOP_BREAK) {
            break;
        } // SSAPS_ExecWriteMultiSubItem只会返回LOOP_RET_FALSE/LOOP_BREAK/LOOP_NORMAL_EXECUTION

        subItem = (SSAP_PduWriteMultiSubItem_S *)((uint8_t *)subItem +
            sizeof(SSAP_PduWriteMultiSubItem_S) + subItem->len);
    }

    return true;
}

static void SSAPS_WriteMultiHandleReq(SSAP_Link_S *link, SSAP_PduWriteReq_S *writeReq, uint16_t len)
{
    uint32_t itemCount = SSAPS_CalculateWriteItemCount(link, (SSAP_PduWriteMultiItem_S *)writeReq->items, len);
    if (itemCount == 0) {
        CP_LOG_ERROR("[SSAP] itemCount == 0");
        SSAP_PduErrorRsp(link, SSAP_WRITE_REQ, 0, SSAP_ERRCODE_INVALID_PDU);
        return;
    }

    SSAP_WriteResultItem_S *results =
        (SSAP_WriteResultItem_S *)SDF_MemZalloc(itemCount * sizeof(SSAP_WriteResultItem_S));
    CP_CHECK_LOG_RETURN_VOID(results != NULL, "[SSAP] multi write results malloc fail");

    SSAP_PduWriteMultiItem_S *multiItem = (SSAP_PduWriteMultiItem_S *)writeReq->items;
    size_t leftSize = (size_t)len - sizeof(SSAP_PduWriteReq_S);
    SSAP_WriteResultItem_S *curResult = results;

    for (uint32_t i = 0; i < itemCount; i++) {
        if (multiItem->subItemCount == 0) {
            curResult->errorCode = SSAP_ERRCODE_INVALID_HANDLE;
            curResult->handle = multiItem->handle;
            multiItem = (SSAP_PduWriteMultiItem_S *)SSAPS_GetNextMultiItemPos(multiItem, &leftSize);
            if (multiItem == NULL) {
                SDF_MemFree(results);
                return;
            }
            curResult++;
            continue;
        }

        if (!SSAPS_WriteMultiHandleItem(link, writeReq, multiItem, curResult)) {
            SDF_MemFree(results);
            return;
        }

        multiItem = (SSAP_PduWriteMultiItem_S *)SSAPS_GetNextMultiItemPos(multiItem, &leftSize);
        if (multiItem == NULL) {
            SDF_MemFree(results);
            return;
        }
        curResult++;
    }

    SSAPS_SendMultiWriteReqRsp(link, results, itemCount, writeReq->msgCtrl);
    SDF_MemFree(results);
}

static SSAP_BufferedOperation_S *SSAPS_WriteReqHandleComposeOperation(SSAP_Link_S *link, uint16_t dataLen,
    SSAP_PduWriteReq_S *writeReq, SSAP_PduWriteReqItem_S *writeReqItem)
{
    SSAP_BufferedOperation_S *operation = (SSAP_BufferedOperation_S *)SDF_MemZalloc(
        sizeof(SSAP_BufferedOperation_S) + dataLen);
    CP_CHECK_LOG_RETURN(operation != NULL, NULL, "[SSAP] write req handle compose operation malloc fail");
    operation->msgCode = writeReq->msgCode;
    operation->propertyHandle = writeReqItem->handle;
    operation->dataType = writeReqItem->type;
    operation->value.len = dataLen;
    operation->controlCode = writeReq->msgCtrl;
    (void)memcpy_s(operation->value.value, dataLen, writeReqItem->value, dataLen);
    (void)memcpy_s(&operation->addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    CP_LOG_INFO("[SSAP] write req handle handle: %d, type: %d", writeReqItem->handle, writeReqItem->type);
    return operation;
}

static void SSAPS_WriteSingleItem(SSAP_Link_S *link, SSAP_BufferedOperation_S *operation)
{
    uint8_t permissions = 0;
    uint32_t opIndication = 0;
    uint8_t errCode;
    SSAP_Property_S *property = SSAPS_GetPropertyByHandle(operation->propertyHandle);
    if (property != NULL) {
        (void)memcpy_s(&(operation->propertyUuid), sizeof(NLSTK_SsapUuid_S),
            &(property->uuid), sizeof(NLSTK_SsapUuid_S));
    }

    if (property == NULL || !SSAPS_GetPermissionAndOperation(property, &permissions, &opIndication,
        operation->dataType)) {
        errCode = property == NULL ? SSAP_ERRCODE_INVALID_HANDLE : SSAP_ERRCODE_DATA_TYPE;
        CP_LOG_ERROR("[SSAP] write req handle cannot find item");
        SSAPS_SendWriteReqRsp(link, errCode, operation);
        return;
    }

    errCode = SSAPS_WriteControlCheck(link, opIndication, permissions, operation->dataType, SSAP_WRITE_REQ);
    if (errCode != SSAP_ERRCODE_SUCCESS) {
        CP_LOG_ERROR("[SSAP] write req handle write failed, errCode = %u", errCode);
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_WRITE_REQ_RECV, errCode);
        SSAPS_SendWriteReqRsp(link, errCode, operation);
        return;
    }

    if ((permissions & (uint8_t)SSAP_PERMISSION_AUTHORIZATION_NEED) != 0) {
        operation->needAuth = true;
        SSAPS_PushOperationPenddingVector(operation);
        return;
    }

    errCode = SSAPS_UpdatePropertyValue(property, operation->dataType, &operation->value, &link->addr, true);
    SSAPS_SendWriteReqRsp(link, errCode, operation);
}

static void SSAPS_WriteSingleHandleReq(SSAP_Link_S *link, SSAP_PduWriteReq_S *writeReq, uint16_t len)
{
    if (len <= SSAP_WRITE_REQ_DATA_OFFSET) {
        SSAP_PduErrorRsp(link, SSAP_WRITE_REQ, 0, SSAP_ERRCODE_INVALID_PDU);
        return;
    }
    SSAP_PduWriteReqItem_S *writeReqItem = (SSAP_PduWriteReqItem_S *)writeReq->items;
    uint16_t dataLen = len - SSAP_WRITE_REQ_DATA_OFFSET;
    SSAP_BufferedOperation_S *operation = SSAPS_WriteReqHandleComposeOperation(link, dataLen, writeReq, writeReqItem);
    if (operation == NULL) {
        CP_LOG_ERROR("[SSAP] operation malloc fail");
        SSAP_PduErrorRsp(link, SSAP_WRITE_REQ, writeReqItem->handle, SSAP_ERRCODE_NO_RESOURCE);
        return;
    }

    if (operation->dataType > DESC_TYPE_PROPERTY_FORMAT) {
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_WRITE_REQ_RECV, EXCEP_SSAP_UNSUPPORT_PDU);
        SSAP_PduErrorRsp(link, SSAP_WRITE_REQ, writeReqItem->handle, SSAP_ERRCODE_UNSUPPORT_PDU);
        SDF_MemFree(operation);
        return;
    }

    CP_LOG_INFO("[SSAP] write single handle req handle: %d, type: %d", writeReqItem->handle, writeReqItem->type);
    SSAPS_WriteSingleItem(link, operation);

    SsapServerAppWriteValueCallback(operation);
    SDF_MemFree(operation);
}

void SSAPS_WriteReqHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_DEBUG("[SSAP] enter write req handle");
    if (SDF_DataLenGet(sdfBuff) > SSAP_STACK_MTU_MAX) {
        CP_LOG_ERROR("[SSAP] len > SSAP_STACK_MTU_MAX(1024)");
        SSAP_PduErrorRsp(link, SSAP_WRITE_REQ, 0, SSAP_ERRCODE_INVALID_PDU);
        return;
    }
    uint16_t len = (uint16_t)SDF_DataLenGet(sdfBuff);
    SSAP_PduWriteReq_S *writeReq = (SSAP_PduWriteReq_S *)SDF_DataOffset(sdfBuff);
    if (len <= sizeof(SSAP_PduWriteReq_S)) {
        CP_LOG_ERROR("[SSAP] len(%d) <= sizeof(SSAP_PduWriteReq_S)", len);
        SSAP_PduErrorRsp(link, SSAP_WRITE_REQ, 0, SSAP_ERRCODE_INVALID_PDU);
        return;
    }

    if ((writeReq->ctrl.oper != SSAP_CTRL_WRITE_INSTANT)) {
        SSAP_PduErrorRsp(link, SSAP_WRITE_REQ, 0, SSAP_ERRCODE_UNSUPPORT_PDU);
        return;
    }
    if (writeReq->ctrl.fragment != SSAP_CTRL_NO_FRAG) {
        SSAP_BufferedOperation_S operation;
        (void)memset_s(&operation, sizeof(SSAP_BufferedOperation_S), 0, sizeof(SSAP_BufferedOperation_S));
        SSAPS_SendWriteReqRsp(link, SSAP_ERRCODE_SERVER_FRAG, &operation);
        return;
    }
    if (writeReq->ctrl.multi == SSAP_CTRL_MULTI_SINGLE) {
        SSAPS_WriteSingleHandleReq(link, writeReq, len);
    } else {
        SSAPS_WriteMultiHandleReq(link, writeReq, len);
    }
}

static SSAP_BufferedOperation_S *SSAPS_WriteCmdHandleComposeOperation(SSAP_Link_S *link, uint16_t dataLen,
    SSAP_PduWriteCmd_S *writeCmd, SSAP_PduWriteCmdItem_S *writeCmdItem)
{
    SSAP_BufferedOperation_S *operation =
        (SSAP_BufferedOperation_S *)SDF_MemZalloc(sizeof(SSAP_BufferedOperation_S) + dataLen);
    CP_CHECK_LOG_RETURN(operation != NULL, NULL, "[SSAP] write cmd handle compose operation malloc fail");
    operation->msgCode = writeCmd->msgCode;
    operation->propertyHandle = writeCmdItem->handle;
    operation->dataType = writeCmdItem->type;
    operation->value.len = dataLen;
    operation->controlCode = writeCmd->msgCtrl;
    (void)memcpy_s(operation->value.value, dataLen, writeCmdItem->value, dataLen);
    (void)memcpy_s(&operation->addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    CP_LOG_DEBUG("[SSAP] write cmd handle handle: %d, type: %d", writeCmdItem->handle, writeCmdItem->type);
    return operation;
}

static SSAP_LoopControlType_E SSAPS_ValidateCmdSubItem(SSAP_Link_S *link, SSAP_PduWriteMultiSubItem_S *subItem,
    SSAP_Property_S *property)
{
    if (subItem->type > DESC_TYPE_PROPERTY_FORMAT) {
        CP_LOG_ERROR("[SSAP] multi write cmd unsupported type: %u", subItem->type);
        return LOOP_RET_TRUE;
    }

    uint8_t permissions = 0;
    uint32_t opIndication = 0;
    if (!SSAPS_GetPermissionAndOperation(property, &permissions, &opIndication, subItem->type)) {
        return LOOP_RET_TRUE;
    }

    uint8_t errCode = SSAPS_WriteControlCheck(link, opIndication, permissions, subItem->type, SSAP_WRITE_CMD);
    if (errCode != SSAP_ERRCODE_SUCCESS) {
        return LOOP_RET_TRUE;
    }

    if ((permissions & (uint8_t)SSAP_PERMISSION_AUTHORIZATION_NEED) != 0) {
        CP_LOG_ERROR("[SSAP] multi write with auth item not supported");
        return LOOP_RET_FALSE;
    }

    if (subItem->len == 0) {
        return LOOP_CONTINUE;
    }

    return LOOP_NORMAL_EXECUTION;
}

static SSAP_BufferedOperation_S *SSAPS_WriteCmdMultiItemComposeOperation(SSAP_Link_S *link,
    SSAP_PduWriteCmd_S *writeCmd, uint16_t handle, SSAP_PduWriteMultiSubItem_S *writeReqItem)
{
    SSAP_BufferedOperation_S *operation = (SSAP_BufferedOperation_S *)SDF_MemZalloc(
        sizeof(SSAP_BufferedOperation_S) + writeReqItem->len);
    CP_CHECK_LOG_RETURN(operation != NULL, NULL,
        "[SSAP] multi write cmd compose operation malloc fail");

    operation->msgCode = writeCmd->msgCode;
    operation->propertyHandle = handle;
    operation->dataType = writeReqItem->type;
    operation->value.len = writeReqItem->len;
    operation->controlCode = writeCmd->msgCtrl;
    operation->needRsp = false;
    operation->needAuth = false;
    (void)memcpy_s(operation->value.value, writeReqItem->len, writeReqItem->value, writeReqItem->len);
    (void)memcpy_s(&operation->addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));

    return operation;
}

static bool SSAPS_WriteMultiHandleCmdItem(SSAP_Link_S *link, SSAP_PduWriteCmd_S *writeCmd,
    SSAP_PduWriteMultiItem_S *multiItem)
{
    SSAP_Property_S *property = SSAPS_GetPropertyByHandle(multiItem->handle);
    if (property == NULL) {
        CP_LOG_ERROR("[SSAP] multi write cmd handle not found: %u", multiItem->handle);
        return true;
    }

    SSAP_PduWriteMultiSubItem_S *subItem = multiItem->subItem;
    for (uint8_t i = 0; i < multiItem->subItemCount; i++) {
        SSAP_LoopControlType_E ret = SSAPS_ValidateCmdSubItem(link, subItem, property);
        if (ret == LOOP_RET_FALSE) {
            return false;
        } else if (ret == LOOP_RET_TRUE) {
            return true;
        } else if (ret == LOOP_CONTINUE) {
            subItem = (SSAP_PduWriteMultiSubItem_S *)((uint8_t *)subItem +
                sizeof(SSAP_PduWriteMultiSubItem_S) + subItem->len);
            continue;
        } // SSAPS_ValidateCmdSubItem只返回LOOP_RET_FALSE/LOOP_RET_TRUE/LOOP_CONTINUE/LOOP_NORMAL_EXECUTION

        SSAP_BufferedOperation_S *operation = SSAPS_WriteCmdMultiItemComposeOperation(
            link, writeCmd, multiItem->handle, subItem);
        CP_CHECK_LOG_RETURN(operation != NULL, false, "[SSAP] multi write cmd operation malloc fail");

        uint8_t errCode =
            SSAPS_UpdatePropertyValue(property, operation->dataType, &operation->value, &link->addr, true);
        if (errCode != SSAP_ERRCODE_SUCCESS) {
            CP_LOG_ERROR("[SSAP] multi write cmd update failed, handle: %u, errCode: %u", multiItem->handle, errCode);
            SDF_MemFree(operation);
            return true;
        }

        subItem = (SSAP_PduWriteMultiSubItem_S *)((uint8_t *)subItem +
            sizeof(SSAP_PduWriteMultiSubItem_S) + subItem->len);

        SsapServerAppWriteValueCallback(operation);
        SDF_MemFree(operation);
    }

    return true;
}

static void SSAPS_WriteMultiHandleCmd(SSAP_Link_S *link, SSAP_PduWriteCmd_S *writeCmd, uint16_t len)
{
    uint32_t itemCount = SSAPS_CalculateWriteItemCount(link, (SSAP_PduWriteMultiItem_S *)writeCmd->items, len);
    if (itemCount == 0) {
        CP_LOG_ERROR("[SSAP] multi write cmd itemCount == 0");
        return;
    }

    SSAP_PduWriteMultiItem_S *multiItem = (SSAP_PduWriteMultiItem_S *)writeCmd->items;
    size_t leftSize = (size_t)len - sizeof(SSAP_PduWriteCmd_S);

    for (uint32_t i = 0; i < itemCount; i++) {
        if (multiItem->subItemCount == 0) {
            multiItem = (SSAP_PduWriteMultiItem_S *)SSAPS_GetNextMultiItemPos(multiItem, &leftSize);
            if (multiItem == NULL) {
                CP_LOG_ERROR("[SSAP] multi write cmd item bounds error");
                return;
            }
            continue;
        }

        if (!SSAPS_WriteMultiHandleCmdItem(link, writeCmd, multiItem)) {
            return;
        }

        multiItem = (SSAP_PduWriteMultiItem_S *)SSAPS_GetNextMultiItemPos(multiItem, &leftSize);
        if (multiItem == NULL) {
            CP_LOG_ERROR("[SSAP] multi write cmd item bounds error");
            return;
        }
    }
}

static void SSAPS_WriteSingleCmdHandle(SSAP_Link_S *link, SSAP_PduWriteCmd_S *writeCmd, uint16_t len)
{
    CP_CHECK_LOG_RETURN_VOID(len > SSAPS_WRITE_CMD_DATA_OFFSET, "[SSAP] single write cmd data len error");

    SSAP_PduWriteCmdItem_S *writeCmdItem = (SSAP_PduWriteCmdItem_S *)(writeCmd->items);
    uint16_t dataLen = len - SSAPS_WRITE_CMD_DATA_OFFSET;
    CP_LOG_INFO("[SSAP] single write cmd handle handle[%u], dataType[0x%02x], dataLen[%u]",
        writeCmdItem->handle, writeCmdItem->type, dataLen);

    uint8_t permissions = 0;
    uint32_t opIndication = 0;
    SSAP_Property_S *property = SSAPS_GetPropertyByHandle(writeCmdItem->handle);
    if (property == NULL) {
        CP_LOG_ERROR("[SSAP] write cmd handle cannot find property item");
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_WRITE_CMD_RECV, EXCEP_SSAP_INVALID_HANDLE);
        return;
    }
    if (!SSAPS_GetPermissionAndOperation(property, &permissions, &opIndication, writeCmdItem->type)) {
        CP_LOG_DEBUG("[SSAP] write cmd handle cannot find permission or operation");
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_WRITE_CMD_RECV, EXCEP_SSAP_DATA_TYPE);
        return;
    }
    SSAP_BufferedOperation_S *operation = SSAPS_WriteCmdHandleComposeOperation(link, dataLen, writeCmd, writeCmdItem);
    CP_CHECK_LOG_RETURN_VOID(operation != NULL, "[SSAP] write cmd handle operation is null.");
    (void)memcpy_s(&(operation->propertyUuid), sizeof(NLSTK_SsapUuid_S), &(property->uuid), sizeof(NLSTK_SsapUuid_S));
    uint8_t errorCode = SSAPS_WriteControlCheck(link, opIndication, permissions, writeCmdItem->type, SSAP_WRITE_CMD);
    if (errorCode != SSAP_ERRCODE_SUCCESS) {
        CP_LOG_ERROR("[SSAP] write cmd handle write failed, errorCode = %u", errorCode);
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_WRITE_CMD_RECV, errorCode);
        SDF_MemFree(operation);
        return;
    } else if ((permissions & (uint8_t)SSAP_PERMISSION_AUTHORIZATION_NEED) != 0) {
        operation->needRsp = false;
        operation->needAuth = true;
        SSAPS_PushOperationPenddingVector(operation);
    } else {
        operation->needRsp = false;
        operation->needAuth = false;
        (void)SSAPS_UpdatePropertyValue(property, operation->dataType, &operation->value, &link->addr, true);
    }
    SsapServerAppWriteValueCallback(operation);
    SDF_MemFree(operation);
}

void SSAPS_WriteCmdHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_DEBUG("[SSAP] enter write cmd handle");
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(sdfBuff) <= SSAP_STACK_MTU_MAX, "[SSAP] recv datalen is invalid");
    uint16_t len = (uint16_t)SDF_DataLenGet(sdfBuff);
    CP_CHECK_LOG_RETURN_VOID(len > sizeof(SSAP_PduWriteCmd_S), "[SSAP] len(%d) <= sizeof(SSAP_PduWriteCmd_S)", len);

    SSAP_PduWriteCmd_S *writeCmd = (SSAP_PduWriteCmd_S *)SDF_DataOffset(sdfBuff);
    CP_LOG_INFO("[SSAP] write cmd handle opCode[0x%02x], ctrl[0x%02x]",
        writeCmd->msgCode, writeCmd->msgCtrl);

    if ((writeCmd->ctrl.oper != SSAP_CTRL_WRITE_INSTANT) ||
        (writeCmd->ctrl.fragment != SSAP_CTRL_NO_FRAG)) {
        CP_LOG_ERROR("[SSAP] write cmd handle ctrl error");
        return;
    }

    if (writeCmd->ctrl.multi == 0) {
        SSAPS_WriteSingleCmdHandle(link, writeCmd, len);
    } else {
        SSAPS_WriteMultiHandleCmd(link, writeCmd, len);
    }
}

#ifdef __cplusplus
}
#endif