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
#include "cm_logic_link_api.h"

#include "cpfwk_log.h"
#include "ssap_type.h"
#include "ssap_pkt.h"
#include "ssap_utils.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssap_common.h"
#include "ssaps_service.h"
#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "ssaps_server_api.h"
#include "ssaps_server_app.h"
#include "ssaps_server.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SSAP_CPCD_MASK_NOTIFICATION 1
#define SSAP_CPCD_MASK_INDICATION 2
#define SSAP_CPCD_MASK_LEN 2
#define SSAP_AUTH_ALLOW 1

#define SSAP_METHOD_REQ_PDU_MIN_LEN 4

static uint8_t g_zeroAddr[SLE_ADDR_LEN] = {0};

static uint16_t g_penddingRequestId = 0;
static SDF_Vector_S *g_ssapPendingVector = NULL;

/**
 * @brief  MTU信息响应报文
 */
static void SSAPS_SendExchangeInfoRsp(SSAP_Link_S *link, uint16_t mtu, uint16_t version)
{
    uint32_t realSize = sizeof(SSAP_PduExchangePkt_S);
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] exchange rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] exchange rsp create buf fail");
        return;
    }
    SSAP_PduExchangePkt_S *exchangePkt = (SSAP_PduExchangePkt_S *)buf;

    /* 操作码 */
    exchangePkt->msgCode = SSAP_EXCHANGE_INFO_RSP;
    /* 消息控制码 */
    exchangePkt->ctrl.mtu = 1;
    exchangePkt->ctrl.version = 1;
    /* MTU */
    exchangePkt->msgMtu = mtu;
    /* version */
    exchangePkt->msgVersion = version;
    link->sendFunc(link, sdfBuff, SSAP_EXCHANGE_INFO_RSP);
}

/**
 * @brief  接收到的MTU信息交换请求报文
 */
void SSAPS_ExchangeInfoReqHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_INFO("[SSAP] enter exchange info req handle");
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(sdfBuff) <= SSAP_STACK_MTU_MAX,
        "[SSAP] exchange info req handle len error");
    uint16_t len = (uint16_t)SDF_DataLenGet(sdfBuff);
    CP_CHECK_LOG_RETURN_VOID(len >= SSAP_EXCHANGE_INFO_PKT_LEN, "[SSAP] exchange info req handle data len error.");
    SSAP_PduExchangePkt_S *exchangePkt = (SSAP_PduExchangePkt_S *)SDF_DataOffset(sdfBuff);

    uint16_t mtu = exchangePkt->msgMtu;
    uint16_t version = exchangePkt->msgVersion;
    CP_LOG_INFO("[SSAP] exchange info req handle mtu is: %d, version is 0x%02x", mtu, version);
    if (mtu < SSAP_STACK_MTU_DEFAULT) {
        CP_LOG_ERROR("[SSAP] exchange info req handle mtu is less than default, mtu: %d", mtu);
        mtu = SSAP_STACK_MTU_DEFAULT;
    } else if (mtu > SSAP_STACK_MTU_MAX) {
        CP_LOG_ERROR("[SSAP] exchange info req handle mtu is greater than default, mtu: %d", mtu);
        mtu = SSAP_STACK_MTU_MAX;
    }
    link->mtu = mtu < link->mtu ? mtu : link->mtu;
    SSAPS_SendExchangeInfoRsp(link, link->mtu, SSAP_EXCHANGE_VERSION);
    SSAP_MtuInfo_S mtuInfo = {0};
    mtuInfo.version = SSAP_EXCHANGE_VERSION;
    mtuInfo.connId = link->lcid;
    mtuInfo.mtuSize = link->mtu;
    mtuInfo.errCode = SSAP_ERRCODE_SUCCESS;
    SsapServerAppMtuExchangeCallBack(&mtuInfo);
}

/**
 * @brief  需要授权的属性操作，需要加入pendding vector，等待用户授权
 */
void SSAPS_PushOperationPenddingVector(SSAP_BufferedOperation_S *operation)
{
    uint16_t len = operation->value.len;
    SSAP_BufferedOperation_S *pendOperation =
        (SSAP_BufferedOperation_S *)SDF_MemZalloc(sizeof(SSAP_BufferedOperation_S) + len);
    CP_CHECK_LOG_RETURN_VOID(pendOperation != NULL, "[SSAP] push operation pendOperation new failed");

    if (g_ssapPendingVector == NULL) {
        SDF_Traits propertyToAccessTraits = {.dtor = SDF_MemFree};
        g_ssapPendingVector = SDF_CreateVector(propertyToAccessTraits);
    }
    if (!SDF_VectorEmplaceBack(g_ssapPendingVector, pendOperation)) {
        CP_LOG_ERROR("[SSAP] push operation emplace back failed");
        SDF_MemFree(pendOperation);
        return;
    }
    operation->requestId = g_penddingRequestId;
    g_penddingRequestId++;
    (void)memcpy_s(pendOperation, sizeof(SSAP_BufferedOperation_S) + len,
        operation, sizeof(SSAP_BufferedOperation_S) + len);
}

/**
 * @brief  获取需要授权的属性操作
 */
SSAP_BufferedOperation_S *SSAPS_PopOperationPenddingVector(uint16_t requestId, size_t *index)
{
    CP_CHECK_LOG_RETURN(g_ssapPendingVector != NULL, NULL,
        "[SSAP] pop operation vector is null");
    SSAP_BufferedOperation_S *operation = NULL;
    for (size_t i = 0; i < g_ssapPendingVector->size; i++) {
        operation = SDF_VectorElementAt(g_ssapPendingVector, i);
        if (operation->requestId == requestId) {
            *index = i;
            return operation;
        }
    }
    return NULL;
}

static bool CompPropertyHandle(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    SSAP_Property_S *property = (SSAP_Property_S *)ptr;
    uint16_t *handle = (uint16_t *)args;
    return property->handle == *handle;
}

static bool CompMethodHandle(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    SSAP_Method_S *method = (SSAP_Method_S *)ptr;
    uint16_t *handle = (uint16_t *)args;
    return method->handle == *handle;
}

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

SSAP_Property_S *SSAPS_GetPropertyByHandle(uint16_t handle)
{
    SSAP_Property_S *property = NULL;
    size_t index = 0;
    SDF_Vector_S *services = SSAPS_GetServices();
    CP_CHECK_LOG_RETURN(services != NULL, NULL, "[SSAP] cannot get services");
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        if (SDF_VectorFindFirst(service->properties, CompPropertyHandle, &handle, &index)) {
            property = SDF_VectorElementAt(service->properties, index);
            return property;
        }
    }
    CP_LOG_ERROR("[SSAP] cannot get property, handle=%u", handle);
    return NULL;
}

static bool SSAPS_IsHandleExist(uint16_t handle)
{
    SDF_Vector_S *services = SSAPS_GetServices();
    CP_CHECK_LOG_RETURN(services != NULL, false, "[SSAP] cannot get services");
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        if (handle >= service->handle && handle <= service->endHandle) {
            return true;
        }
    }
    return false;
}

static SSAP_Method_S *SSAPS_GetMethodByHandle(uint16_t handle)
{
    SSAP_Method_S *method = NULL;
    size_t index = 0;
    SDF_Vector_S *services = SSAPS_GetServices();
    CP_CHECK_LOG_RETURN(services != NULL, NULL, "[SSAP] cannot get services");
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        if (SDF_VectorFindFirst(service->methods, CompMethodHandle, &handle, &index)) {
            method = SDF_VectorElementAt(service->methods, index);
            return method;
        }
    }
    CP_LOG_ERROR("[SSAP] cannot get method");
    return NULL;
}

bool SSAPS_GetPermissionAndOperation(SSAP_Property_S *property, uint8_t *permissions,
    uint32_t *opIndication, uint8_t type)
{
    size_t index = 0;
    if (type == SSAP_TYPE_DATA) {
        *permissions = property->permission.permissionValue;
        *opIndication = property->operation.operationValue;
        return true;
    } else if (SDF_VectorFindFirst(property->descriptors, CompDescriptorType, &type, &index)) {
        SSAP_Descriptor_S *descriptor = SDF_VectorElementAt(property->descriptors, index);
        *permissions = descriptor->permission.permissionValue;
        *opIndication = descriptor->operation.operationValue;
        return true;
    }
    CP_LOG_DEBUG("[SSAP] cannot get descriptor");
    return false;
}

static SSAP_ClientPropertyConfigDescriptor_S *SSAP_FindClientConfig(SSAP_Link_S *link, SSAP_Descriptor_S *descriptor)
{
    size_t index = 0;
    if (SDF_VectorFindFirst(descriptor->clientConfigs, CompClientConfigAddr, link->addr.addr, &index)) {
        SSAP_ClientPropertyConfigDescriptor_S *clientConfig = SDF_VectorElementAt(descriptor->clientConfigs, index);
        return clientConfig;
    }
    return NULL;
}

SSAP_LengthValue_S *SSAPS_GetPropertyValue(SSAP_Link_S *link, SSAP_Property_S *property, uint8_t type,
    uint8_t *errorCode)
{
    size_t index = 0;
    SSAP_LengthValue_S *getVal = NULL;
    if (type == SSAP_TYPE_DATA) {
        getVal = property->val;
    } else if (SDF_VectorFindFirst(property->descriptors, CompDescriptorType, &type, &index)) {
        SSAP_Descriptor_S *descriptor = SDF_VectorElementAt(property->descriptors, index);
        if (type == DESC_TYPE_CLIENT_CONFIG) {
            SSAP_ClientPropertyConfigDescriptor_S *clientConfig = SSAP_FindClientConfig(link, descriptor);
            if (clientConfig != NULL) {
                getVal = clientConfig->val;
            } else {
                getVal = descriptor->val;
            }
        } else {
            getVal = descriptor->val;
        }
    }
    if (getVal == NULL || (getVal->len == 0 && type != SSAP_TYPE_DATA && type != DESC_TYPE_PROPERTY_INSTRUCTION)) {
        CP_LOG_ERROR("[SSAP] cannot get value");
        *errorCode = SSAP_ERRCODE_DATA_TYPE;
        return NULL;
    }
    if (getVal->len > (link->mtu - SSAP_PDU_BASE_LEN)) {
        *errorCode = SSAP_ERRCODE_SERVER_FRAG;
        return NULL;
    }
    return getVal;
}

static void SSAPS_SendErrorRsp(SSAP_Link_S *link, uint8_t msgCodeReq, uint8_t errorCode, uint16_t handle)
{
    uint32_t msgSize = sizeof(SSAP_PduErrRsp_S);
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(msgSize);
    NLSTK_CHECK_RETURN_VOID(sdfBuff != NULL, "[SSAP] read rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, msgSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] read rsp create buf fail");
        return;
    }
    SSAP_PduErrRsp_S *errorRsp = (SSAP_PduErrRsp_S *)buf;
    errorRsp->msgCode = SSAP_ERROR_RSP;
    errorRsp->msgCtrl = 0; // 根据协议描述，此字段为保留参数
    errorRsp->msgCodeReq = msgCodeReq;
    errorRsp->errHandle = handle;
    errorRsp->errorCode = errorCode;
    link->sendFunc(link, sdfBuff, SSAP_ERROR_RSP);
    return;
}

static uint8_t SSAPS_ReadControlCheck(uint32_t opIndication, uint8_t permissions, SSAP_Link_S *link)
{
    if ((opIndication & (uint32_t)SSAP_OPERATE_INDICATION_READ) == 0) {
        return SSAP_ERRCODE_FORBID_READ;
    } else if (((permissions & (uint8_t)SSAP_PERMISSION_AUTHENTICATION_NEED) != 0) &&
        !SmIsSLinkAuthComplete(link->lcid)) {  /* 判断是否需要认证 */
        return SSAP_ERRCODE_UNAUTHENTICATED;
    } else if (((permissions & (uint8_t)SSAP_PERMISSION_ENCRYPTION_NEED) != 0) &&
        !SmIsSLinkEncryptComplete(link->lcid)) {  /* 判断是否需要加密 */
        return SSAP_ERRCODE_UNENCRYPTED;
    }
    return SSAP_ERRCODE_SUCCESS;
}

static void SSAPS_SendReadReqRsp(SSAP_Link_S *link, uint8_t msgCode, uint8_t errorCode, SSAP_LengthValue_S *value)
{
    SSAP_PduReadRsp_S *readRsp = NULL;
    SDF_Buff_S *sdfBuff = NULL;
    uint8_t errCode = errorCode;
    if (errCode != SSAP_ERRCODE_SUCCESS || value == NULL) {
        uint32_t realSize = sizeof(SSAP_PduReadRsp_S) + sizeof(SSAP_PduReadRspItem_S);
        sdfBuff = SDF_BuffNewWithReserve(realSize);
        CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] read rsp sdfBuff malloc fail");
        uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
        if (buf == NULL) {
            SDF_BuffFree(sdfBuff);
            CP_LOG_ERROR("[SSAP] read rsp create buf fail");
            return;
        }
        readRsp = (SSAP_PduReadRsp_S *)buf;
        readRsp->msgCode = msgCode;
        readRsp->ctrl.fragment = SSAP_CTRL_NO_FRAG;
        readRsp->ctrl.error = SSAP_READ_RSP_CONTROL_ERROR;
        SSAP_PduReadRspItem_S *readRspItem = (SSAP_PduReadRspItem_S *)(readRsp->items);
        readRspItem->length = errCode;
        readRspItem->success = SSAP_READ_RSP_ITEMREAD_FAILED;
        CP_LOG_ERROR("[SSAP] read rsp error, error code: %d", errCode);
    } else {
        uint32_t realSize = sizeof(SSAP_PduReadRsp_S) + value->len;
        sdfBuff = SDF_BuffNewWithReserve(realSize);
        CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] read rsp sdfBuff malloc fail");
        uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
        if (buf == NULL) {
            SDF_BuffFree(sdfBuff);
            CP_LOG_ERROR("[SSAP] read rsp create buf fail");
            return;
        }
        readRsp = (SSAP_PduReadRsp_S *)buf;
        readRsp->msgCode = msgCode;
        readRsp->ctrl.fragment = SSAP_CTRL_NO_FRAG;
        readRsp->ctrl.error = SSAP_READ_RSP_CONTROL_SUCCESS;
        (void)memcpy_s(readRsp->items, value->len, value->value, value->len);
    }
    link->sendFunc(link, sdfBuff, msgCode);
}

static bool SSAPS_CheckMultiReadAuthNeed(Ssap_PduReadReqItem_S *readReqItems, size_t itemCount)
{
    if (readReqItems == NULL || itemCount == 0) {
        return false;
    }
    for (size_t i = 0; i < itemCount; i++) {
        SSAP_Property_S *property = SSAPS_GetPropertyByHandle(readReqItems[i].handle);
        if (property == NULL) {
            continue;
        }
        uint8_t permissions = 0;
        uint32_t opIndication = 0;
        if (!SSAPS_GetPermissionAndOperation(property, &permissions, &opIndication, readReqItems[i].type)) {
            continue;
        }
        if ((permissions & (uint8_t)SSAP_PERMISSION_AUTHORIZATION_NEED) != 0) {
            return true;
        }
    }
    return false;
}

static void SSAPS_InitReadResultItem(SSAP_ReadResultItem_S *result, uint16_t handle, uint8_t dataType)
{
    result->handle = handle;
    result->errorCode = SSAP_ERRCODE_SUCCESS;
    result->dataType = dataType;
}

static uint32_t SSAPS_CalcMultiReadTotalSize(SSAP_ReadResultItem_S *results, uint16_t resultCount)
{
    uint32_t totalItemsSize = 0;
    for (uint16_t i = 0; i < resultCount; i++) {
        if (results[i].value != NULL) {
            totalItemsSize += sizeof(SSAP_PduReadRspItem_S) + results[i].value->len;
        } else {
            totalItemsSize += sizeof(SSAP_PduReadRspItem_S);
        }
    }
    return totalItemsSize;
}

static bool SSAPS_SerializeMultiReadItem(uint8_t **curBuf, size_t *leftSize, SSAP_ReadResultItem_S *result,
    bool *hasError)
{
    SSAP_PduReadRspItem_S *readRspItem = (SSAP_PduReadRspItem_S *)*curBuf;
    if (*leftSize < sizeof(SSAP_PduReadRspItem_S)) {
        return false; // 缓冲区不足
    }
    if (result->errorCode != SSAP_ERRCODE_SUCCESS) {
        *hasError = true;
        readRspItem->length = result->errorCode;
        readRspItem->success = SSAP_READ_RSP_ITEMREAD_FAILED;
        *curBuf += sizeof(SSAP_PduReadRspItem_S);
        *leftSize -= sizeof(SSAP_PduReadRspItem_S);
        return true;
    }

    if (result->value == NULL || result->value->len == 0) {
        readRspItem->length = 0;
        readRspItem->success = SSAP_READ_RSP_ITEMREAD_SUCCESS;
        *curBuf += sizeof(SSAP_PduReadRspItem_S);
        *leftSize -= sizeof(SSAP_PduReadRspItem_S);
        return true;
    }

    readRspItem->length = (uint16_t)result->value->len;
    readRspItem->success = SSAP_READ_RSP_ITEMREAD_SUCCESS;
    size_t needSize = sizeof(SSAP_PduReadRspItem_S) + result->value->len;
    if (*leftSize < needSize) {
        return false;
    }
    if (memcpy_s(*curBuf + sizeof(SSAP_PduReadRspItem_S), *leftSize - sizeof(SSAP_PduReadRspItem_S),
        result->value->value, result->value->len) != EOK) {
        return false;
    }
    *curBuf += needSize;
    *leftSize -= needSize;
    return true;
}

static void SSAPS_SendMultiReadReqRsp(SSAP_Link_S *link, uint8_t msgCode,
    SSAP_ReadResultItem_S *results, uint16_t resultCount, Ssap_PduReadReqItem_S *readReqItems)
{
    // 调用保证results!=NULL，resultCount>=1

    uint32_t totalItemsSize = SSAPS_CalcMultiReadTotalSize(results, resultCount);
    uint32_t realSize = sizeof(SSAP_PduReadRsp_S) + totalItemsSize;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (sdfBuff == NULL || buf == NULL) {
        // 内存错误，发送错误响应
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] multi read rsp create buf fail");
        SSAP_PduErrorRsp(link, SSAP_READ_REQ, readReqItems[0].handle, SSAP_ERRCODE_NO_RESOURCE); // 内存错误，发送错误响应
        return;
    }

    SSAP_PduReadRsp_S *readRsp = (SSAP_PduReadRsp_S *)buf;
    readRsp->msgCode = msgCode;
    readRsp->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    readRsp->ctrl.multi = SSAP_CTRL_MULTI_MULTI;
    readRsp->ctrl.error = SSAP_READ_RSP_CONTROL_SUCCESS;

    uint8_t *curBuf = readRsp->items;
    size_t leftSize = realSize - sizeof(SSAP_PduReadRsp_S);
    bool hasError = false;
    for (uint16_t i = 0; i < resultCount; i++) {
        if (!SSAPS_SerializeMultiReadItem(&curBuf, &leftSize, &results[i], &hasError)) {
            SDF_BuffFree(sdfBuff);
            SSAP_PduErrorRsp(link, SSAP_READ_REQ, results[i].handle, SSAP_ERRCODE_INVALID_PDU); // 编码失败，发送错误响应
            return;
        }
    }

    if (hasError) {
        readRsp->ctrl.error = SSAP_READ_RSP_CONTROL_ERROR;
    }
    link->sendFunc(link, sdfBuff, msgCode);
}

static void SSAPS_ReadMultiHandleItem(SSAP_Link_S *link, Ssap_PduReadReqItem_S *readReqItem,
    SSAP_ReadResultItem_S *result, SSAP_BufferedOperation_S *operation, uint8_t msgCode)
{
    uint8_t permissions = 0;
    uint32_t opIndication = 0;
    SSAP_Property_S *property = SSAPS_GetPropertyByHandle(readReqItem->handle);
    if (property == NULL ||
        !SSAPS_GetPermissionAndOperation(property, &permissions, &opIndication, readReqItem->type)) {
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_READ_REQ_RECV,
            (property == NULL) ? EXCEP_SSAP_INVALID_HANDLE : EXCEP_SSAP_DATA_TYPE);
        operation->errCode = (property == NULL) ? SSAP_ERRCODE_INVALID_HANDLE : SSAP_ERRCODE_DATA_TYPE;
        result->errorCode = operation->errCode;
        return;
    }

    (void)memcpy_s(&operation->propertyUuid, sizeof(NLSTK_SsapUuid_S), &property->uuid, sizeof(NLSTK_SsapUuid_S));

    uint8_t errorCode = SSAPS_ReadControlCheck(opIndication, permissions, link);
    if (errorCode != SSAP_ERRCODE_SUCCESS) {
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_READ_REQ_RECV, errorCode);
        operation->errCode = errorCode;
        return;
    }

    SSAP_LengthValue_S *value = SSAPS_GetPropertyValue(link, property, readReqItem->type, &errorCode);
    if (value != NULL && errorCode == SSAP_ERRCODE_SUCCESS) {
        result->value = value;
    } else {
        result->errorCode = errorCode;
    }
    operation->errCode = errorCode;
}

static void SSAPS_ReadMultiHandleReq(SSAP_Link_S *link, Ssap_PduReadReqItem_S *readReqItems,
    size_t itemCount, SSAP_PduReadReq_S *readReq)
{
    SSAP_ReadResultItem_S *results = (SSAP_ReadResultItem_S *)SDF_MemZalloc(sizeof(SSAP_ReadResultItem_S) * itemCount);
    if (results == NULL) {
        CP_LOG_ERROR("[SSAP] multi read req handle results malloc fail");
        SSAP_PduErrorRsp(link, SSAP_READ_REQ, readReqItems[0].handle, SSAP_ERRCODE_NO_RESOURCE);
        return;
    }

    SSAP_BufferedOperation_S operation;
    for (size_t i = 0; i < itemCount; i++) {
        SSAPS_InitReadResultItem(&results[i], readReqItems[i].handle, readReqItems[i].type);
        CP_LOG_INFO("[SSAP] read multi req handle[%zu] handle: %d, type: %d",
            i, readReqItems[i].handle, readReqItems[i].type);

        (void)memset_s(&operation, sizeof(SSAP_BufferedOperation_S), 0, sizeof(SSAP_BufferedOperation_S));
        operation.msgCode = readReq->msgCode;
        operation.propertyHandle = readReqItems[i].handle;
        operation.dataType = readReqItems[i].type;
        (void)memcpy_s(&operation.addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));

        SSAPS_ReadMultiHandleItem(link, &readReqItems[i], &results[i], &operation, readReq->msgCode);
        SsapServerAppReadValueCallback(&operation);
    }

    SSAPS_SendMultiReadReqRsp(link, readReq->msgCode + 1, results, (uint16_t)itemCount, readReqItems);
    SDF_MemFree(results);
}

static void SSAPS_HandleReadReqError(SSAP_Link_S *link, uint16_t handle)
{
    uint8_t errCode = SSAP_ERRCODE_SUCCESS;
    int subSceneCode = 0;
    bool findHandle = SSAPS_IsHandleExist(handle);
    // 如果该句柄是属性，则由前置调用分支可知请求属性的数据类型错误；如果该句柄不是属性，则请求读取的句柄对应的类型错误
    // 协议规定它们对应的错误码都是数据类型错误
    if (findHandle) {
        errCode = SSAP_ERRCODE_DATA_TYPE;
        subSceneCode = EXCEP_SSAP_DATA_TYPE;
    } else {
        errCode = SSAP_ERRCODE_INVALID_HANDLE;
        subSceneCode = EXCEP_SSAP_INVALID_HANDLE;
    }
    CP_LOG_ERROR("[SSAP] read req handle inexist or type error");
    SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_READ_REQ_RECV, subSceneCode);
    SSAPS_SendReadReqRsp(link, SSAP_READ_RSP, errCode, NULL);
}

static void SSAPS_ReadSingleHandleReq(SSAP_Link_S *link, Ssap_PduReadReqItem_S *readReqItem,
    SSAP_PduReadReq_S *readReq)
{
    CP_LOG_INFO("[SSAP] read single handle req handle: %d, type: %d", readReqItem->handle, readReqItem->type);
    SSAP_BufferedOperation_S *operation = (SSAP_BufferedOperation_S *)SDF_MemZalloc(sizeof(SSAP_BufferedOperation_S));
    CP_CHECK_LOG_RETURN_VOID(operation != NULL, "[SSAP] read single handle req operation malloc fail");
    operation->msgCode = readReq->msgCode;
    operation->propertyHandle = readReqItem->handle;
    operation->dataType = readReqItem->type;
    (void)memcpy_s(&operation->addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));

    uint8_t permissions = 0;
    uint32_t opIndication = 0;
    SSAP_Property_S *property = SSAPS_GetPropertyByHandle(readReqItem->handle);
    if (property == NULL ||
        !SSAPS_GetPermissionAndOperation(property, &permissions, &opIndication, readReqItem->type)) {
        SSAPS_HandleReadReqError(link, readReqItem->handle);
        SsapServerAppReadValueCallback(operation);
        SDF_MemFree(operation);
        return;
    }

    (void)memcpy_s(&operation->propertyUuid, sizeof(NLSTK_SsapUuid_S), &property->uuid, sizeof(NLSTK_SsapUuid_S));

    uint8_t errorCode = SSAPS_ReadControlCheck(opIndication, permissions, link);
    if (errorCode != SSAP_ERRCODE_SUCCESS) {
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_READ_REQ_RECV, errorCode);
        SSAPS_SendReadReqRsp(link, readReq->msgCode + 1, errorCode, NULL);
        SsapServerAppReadValueCallback(operation);
        SDF_MemFree(operation);
        return;
    }

    if ((permissions & (uint8_t)SSAP_PERMISSION_AUTHORIZATION_NEED) != 0) {
        operation->needAuth = true;
        SSAPS_PushOperationPenddingVector(operation);
    } else {
        SSAP_LengthValue_S *value = SSAPS_GetPropertyValue(link, property, readReqItem->type, &errorCode);
        SSAPS_SendReadReqRsp(link, readReq->msgCode + 1, errorCode, value);
    }

    SsapServerAppReadValueCallback(operation);
    SDF_MemFree(operation);
}

static uint8_t SSAPS_MethodCallCheck(uint8_t permissions, SSAP_Link_S *link)
{
    if (((permissions & (uint8_t)SSAP_PERMISSION_AUTHENTICATION_NEED) != 0) &&
        !SmIsSLinkAuthComplete(link->lcid)) {  /* 判断是否需要认证 */
        return SSAP_ERRCODE_UNAUTHENTICATED;
    } else if (((permissions & (uint8_t)SSAP_PERMISSION_ENCRYPTION_NEED) != 0) &&
        !SmIsSLinkEncryptComplete(link->lcid)) {  /* 判断是否需要加密 */
        return SSAP_ERRCODE_UNENCRYPTED;
    }
    return SSAP_ERRCODE_SUCCESS;
}

static bool SSAPS_ReadReqHandlePreCheck(SSAP_Link_S *link, SDF_Buff_S *sdfBuff, uint16_t len)
{
    if (len < sizeof(SSAP_PduReadReq_S) + sizeof(Ssap_PduReadReqItem_S) ||
        (len - sizeof(SSAP_PduReadReq_S)) % sizeof(Ssap_PduReadReqItem_S) != 0) {
        CP_LOG_ERROR("[SSAP] read req handle len error, len: %d", len);
        SSAP_PduErrorRsp(link, SSAP_READ_REQ, 0, SSAP_ERRCODE_INVALID_PDU);
        return false;
    }
    SSAP_PduReadReq_S *readReq = (SSAP_PduReadReq_S *)SDF_DataOffset(sdfBuff);
    Ssap_PduReadReqItem_S *readReqItem = (Ssap_PduReadReqItem_S *)(readReq->items);
    if (readReqItem->type > DESC_TYPE_PROPERTY_FORMAT) {
        CP_LOG_ERROR("[SSAP] read req handle type error");
        SSAP_PduErrorRsp(link, SSAP_READ_REQ, 0, SSAP_ERRCODE_UNSUPPORT_PDU);
        return false;
    }
    return true;
}

/**
 * @brief  接收到的读取数据请求报文处理
 */
void SSAPS_ReadReqHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_DEBUG("[SSAP] enter read req handle");
    if (SDF_DataLenGet(sdfBuff) > SSAP_STACK_MTU_MAX) {
        CP_LOG_ERROR("[SSAP] read req handle len error");
        SSAP_PduErrorRsp(link, SSAP_READ_REQ, 0, SSAP_ERRCODE_INVALID_PDU);
        return;
    }
    uint16_t len = (uint16_t)SDF_DataLenGet(sdfBuff);
    if (!SSAPS_ReadReqHandlePreCheck(link, sdfBuff, len)) {
        return;
    }
    SSAP_PduReadReq_S *readReq = (SSAP_PduReadReq_S *)SDF_DataOffset(sdfBuff);
    if ((readReq->ctrl.fragment & SSAP_CTRL_NO_FRAG) != SSAP_CTRL_NO_FRAG) { /* 暂不支持分包组包 */
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_READ_REQ_RECV, EXCEP_SSAP_SERVER_FRAG);
        SSAPS_SendReadReqRsp(link, readReq->msgCode + 1, SSAP_ERRCODE_SERVER_FRAG, NULL);
        return;
    }
    size_t itemSize = sizeof(Ssap_PduReadReqItem_S);
    size_t itemCount = (len - sizeof(SSAP_PduReadReq_S)) / itemSize;
    Ssap_PduReadReqItem_S *readReqItems = (Ssap_PduReadReqItem_S *)(readReq->items);
    CP_LOG_INFO("[SSAP] read req handle item count: %zu", itemCount);

    if (itemCount > 1 && SSAPS_CheckMultiReadAuthNeed(readReqItems, itemCount)) {
        CP_LOG_ERROR("[SSAP] multi read with auth item not supported, itemCount: %zu", itemCount);
        SSAP_PduErrorRsp(link, SSAP_READ_REQ, readReqItems[0].handle, SSAP_ERRCODE_ITEM_OVER_LIMIT);
        return;
    }

    // SSAPS_ReadReqHandlePreCheck保证itemCount >= 1
    if (itemCount == 1) {
        SSAPS_ReadSingleHandleReq(link, readReqItems, readReq);
    } else {
        SSAPS_ReadMultiHandleReq(link, readReqItems, itemCount, readReq);
    }
}

static void SSAPS_GetPropertyByUuid(uint16_t handleStart, uint16_t handleEnd, NLSTK_SsapUuid_S *uuid,
    SDF_Vector_S *findPropertys)
{
    SDF_Vector_S *services = SSAPS_GetServices();
    CP_CHECK_LOG_RETURN_VOID(services != NULL, "[SSAP] cannot get services");
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        CP_CHECK_LOG_RETURN_VOID(service->properties != NULL, "[SSAP] cannot get property");
        for (size_t j = 0; j < service->properties->size; j++) {
            SSAP_Property_S *property = SDF_VectorElementAt(service->properties, j);
            if (SSAP_IsUuidEqual(&property->uuid, uuid) && property->handle >= handleStart &&
                property->handle <= handleEnd) {
                CP_CHECK_LOG_RETURN_VOID(SDF_VectorEmplaceBack(findPropertys, property),
                    "[SSAP] find property uuid property emplace back fail");
            }
        }
    }
}

static void SSAPS_SendReadByUuidReqErrorRsp(SSAP_Link_S *link, uint8_t errorCode, uint16_t handle)
{
    uint32_t realSize = sizeof(SSAP_PduReadByUuidRsp_S) + sizeof(SSAP_PduReadByUuidRspItem_S);
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] read by uuid error rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] read by uuid error rsp create buf fail");
        return;
    }
    SSAP_PduReadByUuidRsp_S *readByUuidRsp = (SSAP_PduReadByUuidRsp_S *)buf;
    readByUuidRsp->msgCode = SSAP_READ_BY_UUID_RSP;
    readByUuidRsp->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    readByUuidRsp->ctrl.error = SSAP_READ_RSP_CONTROL_ERROR;
    SSAP_PduReadByUuidRspItem_S *readByUuidRspItem = (SSAP_PduReadByUuidRspItem_S *)(readByUuidRsp->items);
    readByUuidRspItem->handle = handle;
    readByUuidRspItem->length = errorCode;
    readByUuidRspItem->success = SSAP_READ_RSP_ITEMREAD_FAILED;
    CP_LOG_ERROR("[SSAP] read by uuid rsp error, error code: %d", errorCode);
    link->sendFunc(link, sdfBuff, SSAP_READ_BY_UUID_RSP);
}

static void SSAPS_SendReadByUuidReqRsp(SSAP_Link_S *link, uint8_t *data, uint16_t realRspDataLen,
    uint8_t multiFlag, uint8_t errorFlag)
{
    if (realRspDataLen > (link->mtu - SSAP_PDU_BASE_LEN)) {
        SSAPS_SendReadByUuidReqErrorRsp(link, SSAP_ERRCODE_SERVER_FRAG, 0);
        return;
    }
    uint32_t realSize = sizeof(SSAP_PduReadByUuidRsp_S) + realRspDataLen;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] read by uuid rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] read by uuid rsp create buf fail");
        return;
    }
    SSAP_PduReadByUuidRsp_S *readByUuidRsp = (SSAP_PduReadByUuidRsp_S *)buf;
    readByUuidRsp->msgCode = SSAP_READ_BY_UUID_RSP;
    readByUuidRsp->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    readByUuidRsp->ctrl.multi = multiFlag;
    readByUuidRsp->ctrl.error = errorFlag;
    (void)memcpy_s(readByUuidRsp->items, realRspDataLen, data, realRspDataLen);
    link->sendFunc(link, sdfBuff, SSAP_READ_BY_UUID_RSP);
}

static bool SSAPS_ReadByUuidReqHandleCheck(uint16_t len, uint8_t *errorCode, SSAP_PduReadByUuidReq_S *readByUuidReq)
{
    uint8_t payloadLen = 0;

    /* 判断control code是否合法 */
    if (readByUuidReq->ctrl.uuidType == SSAP_READ_BY_STANDARD_UUID_CONTROL) {
        payloadLen = SSAP_READ_BY_STANDARD_UUID_REQ_PKT_LEN;
    } else if (readByUuidReq->ctrl.uuidType == SSAP_READ_BY_CUSTOM_UUID_CONTROL) {
        payloadLen = SSAP_READ_BY_CUSTOM_UUID_REQ_PKT_LEN;
    } else {
        *errorCode = SSAP_ERRCODE_INVALID_PDU;
        return false;
    }
    /* 判断数据长度是否合法 */
    if (payloadLen != len) {
        *errorCode = SSAP_ERRCODE_INVALID_PDU;
        CP_LOG_ERROR("[SSAP] read by uuid req handle len error");
        return false;
    }
    if ((readByUuidReq->handleStart > readByUuidReq->handleEnd) || (readByUuidReq->handleStart == 0)) {
        *errorCode = SSAP_ERRCODE_INVALID_HANDLE;
        return false;
    }
    return true;
}

static uint16_t SSAPS_ReadByUuidReqRspLen(SSAP_Link_S *link, SDF_Vector_S *findProperty, uint8_t dataType)
{
    uint8_t errorCode = SSAP_ERRCODE_SUCCESS;
    uint16_t dataLen = 0;
    if (findProperty->size == 1) {
        SSAP_Property_S *property = SDF_VectorElementAt(findProperty, 0);
        SSAP_LengthValue_S *value = SSAPS_GetPropertyValue(link, property, dataType, &errorCode);
        if (value != NULL) {
            // 后续权限有可能拦住回复error，所以需要计算回复成功的长度与回复失败的长度，两者取最大值
            uint16_t errorLen = sizeof(SSAP_PduReadByUuidRspItem_S);
            uint16_t successLen = sizeof(SSAP_PduReadByUuidRspSingleItem_S) + value->len;
            dataLen = errorLen > successLen ? errorLen : successLen;
        } else {
            dataLen = sizeof(SSAP_PduReadByUuidRspItem_S);
        }
        return dataLen;
    }
    for (size_t i = 0; i < findProperty->size; i++) {
        SSAP_Property_S *property = SDF_VectorElementAt(findProperty, i);
        SSAP_LengthValue_S *value = SSAPS_GetPropertyValue(link, property, dataType, &errorCode);
        uint16_t appendLen = 0;
        if (value != NULL) {
            appendLen = sizeof(SSAP_PduReadByUuidRspItem_S) + value->len;
        } else {
            appendLen = sizeof(SSAP_PduReadByUuidRspItem_S);
        }
        if (dataLen > UINT16_MAX - appendLen) {
            CP_LOG_ERROR("[SSAP] append rsp len over U16 MAX. appendLen: %u, dataLen: %u", appendLen, dataLen);
            break;
        }
        dataLen += appendLen;
    }
    return dataLen;
}

static void SSAPS_AddOneErrorItem(SSAP_PduReadByUuidRspItem_S *rspItem, uint16_t handle, uint8_t errorCode)
{
    rspItem->handle = handle;
    rspItem->length = errorCode;
    rspItem->success = SSAP_READ_RSP_ITEMREAD_FAILED;
}

static void SSAPS_AddMultiOneItem(SSAP_PduReadByUuidRspItem_S *rspItem, uint16_t handle, SSAP_LengthValue_S *value)
{
    rspItem->handle = handle;
    rspItem->length = value->len;
    rspItem->success = SSAP_READ_RSP_ITEMREAD_SUCCESS;
    (void)memcpy_s(rspItem->value, value->len, value->value, value->len);
}

static void SSAPS_AddSingleItem(SSAP_PduReadByUuidRspSingleItem_S *rspSingleItem, uint16_t handle,
    SSAP_LengthValue_S *value)
{
    rspSingleItem->handle = handle;
    (void)memcpy_s(rspSingleItem->value, value->len, value->value, value->len);
}

static SSAP_ReadByUuidReqPktInfo_S *SSAP_BuildReadReqPktInfo(uint8_t errorFlag, uint8_t multiFlag,
    uint16_t realRspDataLen)
{
    SSAP_ReadByUuidReqPktInfo_S *pktInfo =
        (SSAP_ReadByUuidReqPktInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadByUuidReqPktInfo_S));
    CP_CHECK_LOG_RETURN(pktInfo != NULL, NULL, "[SSAP] read by uuid req pkt malloc fail");
    pktInfo->errorFlag = errorFlag;
    pktInfo->multiFlag = multiFlag;
    pktInfo->realRspDataLen = realRspDataLen;
    return pktInfo;
}

static uint8_t SSAP_ReadByUuidCheckError(SSAP_Link_S *link, SSAP_Property_S *property, uint8_t dataType)
{
    uint8_t permissions = 0;
    uint32_t opIndication = 0;
    if (!SSAPS_GetPermissionAndOperation(property, &permissions, &opIndication, dataType)) {
        return SSAP_ERRCODE_DATA_TYPE;
    }
    /* 判断是否可读 */
    if ((opIndication & (uint32_t)SSAP_OPERATE_INDICATION_READ) == 0) {
        return SSAP_ERRCODE_FORBID_READ;
    }
    /* 判断是否需要认证 */
    if (((permissions & (uint8_t)SSAP_PERMISSION_AUTHENTICATION_NEED) != 0) && !SmIsSLinkAuthComplete(link->lcid)) {
        return SSAP_ERRCODE_UNAUTHENTICATED;
    }
    /* 判断是否需要加密 */
    if (((permissions & (uint8_t)SSAP_PERMISSION_ENCRYPTION_NEED) != 0) && !SmIsSLinkEncryptComplete(link->lcid)) {
        return SSAP_ERRCODE_UNENCRYPTED;
    }
    return SSAP_ERRCODE_SUCCESS;
}

static SSAP_ReadByUuidReqPktInfo_S *SSAPS_ReadByUuidReqRspPkt(SSAP_Link_S *link, SDF_Vector_S *findPropertys,
    uint8_t dataType, SSAP_LengthValue_S *lengthValue, SSAP_BufferedOperation_S *operation)
{
    uint8_t errorFlag = 0;
    uint8_t multiFlag = 0;
    uint8_t *curData = lengthValue->value;
    uint16_t leftLen = lengthValue->len;
    for (size_t i = 0; i < findPropertys->size; i++) {
        SSAP_Property_S *property = SDF_VectorElementAt(findPropertys, i);
        uint8_t errorCode = SSAP_ReadByUuidCheckError(link, property, dataType);
        uint16_t needLen = 0;
        if (errorCode == SSAP_ERRCODE_SUCCESS) {
            SSAP_LengthValue_S *value = SSAPS_GetPropertyValue(link, property, dataType, &errorCode);
            if (value == NULL) {
                needLen = sizeof(SSAP_PduReadByUuidRspItem_S);
                CP_CHECK_LOG_RETURN(leftLen >= needLen, NULL, "[SSAP] read by uuid req pkt len error");
                SSAPS_AddOneErrorItem((SSAP_PduReadByUuidRspItem_S *)curData, property->handle, errorCode);
                errorFlag = 1;
            } else if (findPropertys->size > 1) {
                needLen = sizeof(SSAP_PduReadByUuidRspItem_S) + value->len;
                CP_CHECK_LOG_RETURN(leftLen >= needLen, NULL, "[SSAP] read by uuid req pkt len error");
                SSAPS_AddMultiOneItem((SSAP_PduReadByUuidRspItem_S *)curData, property->handle, value);
                multiFlag = 1;
            } else {
                needLen = sizeof(SSAP_PduReadByUuidRspSingleItem_S) + value->len;
                CP_CHECK_LOG_RETURN(leftLen >= needLen, NULL, "[SSAP] read by uuid req pkt len error");
                SSAPS_AddSingleItem((SSAP_PduReadByUuidRspSingleItem_S *)curData, property->handle, value);
            }
        } else {
            needLen = sizeof(SSAP_PduReadByUuidRspItem_S);
            CP_CHECK_LOG_RETURN(leftLen >= needLen, NULL, "[SSAP] read by uuid req pkt len error");
            SSAPS_AddOneErrorItem((SSAP_PduReadByUuidRspItem_S *)curData, property->handle, errorCode);
            errorFlag = 1;
        }
        if (errorCode != SSAP_ERRCODE_DATA_TYPE) {
            operation->propertyHandle = property->handle;
        }
        leftLen -= needLen;
        curData += needLen;
    }
    return SSAP_BuildReadReqPktInfo(errorFlag, multiFlag, lengthValue->len - leftLen);
}

static SSAP_BufferedOperation_S *SSAPS_ReadByUuidComposeOperation(SSAP_Link_S *link,
    SSAP_PduReadByUuidReq_S *readByUuidReq)
{
    SSAP_BufferedOperation_S *operation = (SSAP_BufferedOperation_S *)SDF_MemZalloc(sizeof(SSAP_BufferedOperation_S));
    CP_CHECK_LOG_RETURN(operation != NULL, NULL, "[SSAP] read by uuid compose operation malloc fail");
    operation->msgCode = readByUuidReq->msgCode;
    operation->dataType = readByUuidReq->dataType;
    operation->beginHandle = readByUuidReq->handleStart;
    operation->endHandle = readByUuidReq->handleEnd;
    operation->addr.type = link->addr.type;
    (void)memcpy_s(operation->addr.addr, SLE_ADDR_LEN, link->addr.addr, SLE_ADDR_LEN);
    if (readByUuidReq->ctrl.uuidType == 0) {
        SSAP_GetUuidFromPktBuf(&operation->propertyUuid, readByUuidReq->uuid, SSAP_UUID16_LEN);
    } else {
        SSAP_GetUuidFromPktBuf(&operation->propertyUuid, readByUuidReq->uuid, SSAP_UUID128_LEN);
    }
    return operation;
}

static SSAP_PduReadByUuidReq_S *SSAPS_ReadByUuidReqPreHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_DEBUG("[SSAP] enter read by uuid req handle");
    CP_CHECK_LOG_RETURN(SDF_DataLenGet(sdfBuff) <= SSAP_STACK_MTU_MAX, NULL,
        "[SSAP] exchange info req handle len error");
    uint16_t len = (uint16_t)SDF_DataLenGet(sdfBuff);
    if (len != SSAP_READ_BY_STANDARD_UUID_REQ_PKT_LEN && len != SSAP_READ_BY_CUSTOM_UUID_REQ_PKT_LEN) {
        SSAP_PduErrorRsp(link, SSAP_READ_BY_UUID_REQ, 0, SSAP_ERRCODE_INVALID_PDU);
        return NULL;
    }
    SSAP_PduReadByUuidReq_S *readByUuidReq = (SSAP_PduReadByUuidReq_S *)SDF_DataOffset(sdfBuff);
    if (readByUuidReq->dataType > DESC_TYPE_PROPERTY_FORMAT) {
        SSAP_PduErrorRsp(link, SSAP_READ_BY_UUID_REQ, 0, SSAP_ERRCODE_UNSUPPORT_PDU);
        return NULL;
    }
    uint8_t errorCode = SSAP_ERRCODE_SUCCESS;
    if (!SSAPS_ReadByUuidReqHandleCheck(len, &errorCode, readByUuidReq)) {
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_READ_BY_UUID_REQ_RECV, errorCode);
        SSAPS_SendReadByUuidReqErrorRsp(link, errorCode, readByUuidReq->handleStart);
        return NULL;
    }
    return readByUuidReq;
}

/**
 * @brief  接收到的通过uuid读取数据请求报文
 */
void SSAPS_ReadByUuidReqHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_DEBUG("[SSAP] enter read by uuid req handle");
    SSAP_PduReadByUuidReq_S *readByUuidReq = SSAPS_ReadByUuidReqPreHandle(link, sdfBuff);
    CP_CHECK_LOG_RETURN_VOID(readByUuidReq != NULL, "[SSAP] read by uuid req handle pre fail");
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(sdfBuff) <= SSAP_STACK_MTU_MAX,
        "[SSAP] exchange info req handle len error");
    uint16_t len = (uint16_t)SDF_DataLenGet(sdfBuff);
    CP_CHECK_LOG_RETURN_VOID(len >= SSAP_READ_BY_UUID_REQ_UUID_OFFSET, "[SSAP] read by uuid req handle len too short");
    NLSTK_SsapUuid_S uuid = {0};
    uint32_t uuidSize = len - SSAP_READ_BY_UUID_REQ_UUID_OFFSET;
    SSAP_GetUuidFromPktBuf(&uuid, readByUuidReq->uuid, uuidSize);
    SDF_Traits findPropertyTraits = {.dtor = NULL};
    SDF_Vector_S *findPropertys = SDF_CreateVector(findPropertyTraits);
    CP_CHECK_LOG_RETURN_VOID(findPropertys != NULL, "[SSAP] read by uuid req handle vector malloc fail");
    SSAPS_GetPropertyByUuid(readByUuidReq->handleStart, readByUuidReq->handleEnd, &uuid, findPropertys);
    if (findPropertys->size == 0) {
        CP_LOG_ERROR("[SSAP] read by uuid req handle cannot find property item");
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_READ_BY_UUID_REQ_RECV, EXCEP_SSAP_ITEM_INEXIST);
        SSAPS_SendReadByUuidReqErrorRsp(link, SSAP_ERRCODE_ITEM_INEXIST, readByUuidReq->handleStart);
        SDF_DestroyVector(findPropertys);
        return;
    }
    uint16_t rspDataLen = SSAPS_ReadByUuidReqRspLen(link, findPropertys, readByUuidReq->dataType);
    SSAP_LengthValue_S *valueAll = (SSAP_LengthValue_S *)SDF_MemZalloc(sizeof(SSAP_LengthValue_S) + rspDataLen);
    if (valueAll == NULL) {
        SDF_DestroyVector(findPropertys);
        return;
    }
    valueAll->len = rspDataLen;
    SSAP_BufferedOperation_S *operation = SSAPS_ReadByUuidComposeOperation(link, readByUuidReq);
    if (operation == NULL) {
        SDF_MemFree(valueAll);
        SDF_DestroyVector(findPropertys);
        return;
    }
    SSAP_ReadByUuidReqPktInfo_S *readByUuidReqPktInfo = SSAPS_ReadByUuidReqRspPkt(link, findPropertys,
        readByUuidReq->dataType, valueAll, operation);
    if (readByUuidReqPktInfo != NULL) {
        SSAPS_SendReadByUuidReqRsp(link, valueAll->value, readByUuidReqPktInfo->realRspDataLen,
            readByUuidReqPktInfo->multiFlag, readByUuidReqPktInfo->errorFlag);
        SsapServerAppReadValueCallback(operation);
        SDF_MemFree(readByUuidReqPktInfo);
    }
    SDF_MemFree(operation);
    SDF_MemFree(valueAll);
    SDF_DestroyVector(findPropertys);
}

/**
 * @brief  接受到的指示确认报文处理
 */
void SSAPS_ValueAckHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_INFO("[SSAP] enter value ack handle");
    // 加宏判断是否在uint16_t内，不在就报错
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(sdfBuff) <= SSAP_STACK_MTU_MAX,
        "[SSAP] exchange info req handle len error");
    uint16_t len = (uint16_t)SDF_DataLenGet(sdfBuff);
    CP_CHECK_LOG_RETURN_VOID(len >= SSAP_VALUE_ACK_PDU_MIN_LEN, "[SSAP] value ack handle data len error");
    SSAP_PduValueAck_S *valueAck = (SSAP_PduValueAck_S *)SDF_DataOffset(sdfBuff);
    CP_LOG_INFO("[SSAP] value ack handle opCode[0x%02x], controlCode[0x%02x]", valueAck->msgCode, valueAck->msgCtrl);
    uint16_t valueLen = len - SSAP_PDU_BASE_LEN;
    SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)SDF_MemZalloc(sizeof(SSAP_ValuePkt_S) + valueLen);
    CP_CHECK_LOG_RETURN_VOID(valuePkt != NULL, "[SSAP] value ack handle valuePdu mem new failed");
    valuePkt->opCode = valueAck->msgCode;
    valuePkt->controlCode = valueAck->msgCtrl;
    valuePkt->value.len = valueLen;
    (void)memcpy_s(valuePkt->value.value, valueLen, valueAck->result, valueLen);
    (void)memcpy_s(&valuePkt->addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    SsapServerAppIndicateCfmValueCallback(valuePkt);
    SDF_MemFree(valuePkt);
}

static uint32_t SsapGenerateMethodCallOp(SSAP_BufferedOperation_S *operation, SSAP_PduCallMethodReq_S *callMethodMsg,
                                         uint16_t paramLen, SLE_Addr_S *addr)
{
    operation->msgCode = callMethodMsg->msgCode;
    operation->propertyHandle = callMethodMsg->handle;
    operation->controlCode = callMethodMsg->msgCtrl;
    (void)memcpy_s(&operation->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (paramLen > 0) {
        operation->value.len = paramLen;
        if (memcpy_s(operation->value.value, paramLen, callMethodMsg->param, paramLen) != EOK) {
            NLSTK_LOG_ERROR("memcpy failed when cpy the param of method call req");
            return NLSTK_ERRCODE_MEMCPY_FAIL;
        }
    }
    operation->needRsp = (callMethodMsg->msgCode == SSAP_CALL_METHOD_REQ) ? true : false;
    return NLSTK_ERRCODE_SUCCESS;
}

static void SSAPS_MethodErrorProcess(SSAP_Link_S *link, uint8_t msgCode, uint8_t errorCode, uint16_t handle)
{
    if (msgCode == SSAP_CALL_METHOD_REQ) {
        SSAPS_SendErrorRsp(link, SSAP_CALL_METHOD_REQ, errorCode, handle);
    }
}

static void SSAPS_MethodHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    NLSTK_LOG_INFO("[SSAP] enter call method process");
    uint8_t *buf = SDF_DataOffset(sdfBuff);
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(sdfBuff) <= SSAP_STACK_MTU_MAX, "[SSAP] method handle len error");
    uint16_t size = (uint16_t)SDF_DataLenGet(sdfBuff);
    if (size < SSAP_METHOD_REQ_PDU_MIN_LEN) {
        NLSTK_LOG_ERROR("[SSAP] method handle len error, len: %d", size);
        SSAPS_MethodErrorProcess(link, buf[0], SSAP_ERRCODE_INVALID_PDU, 0);
        return;
    }
    SSAP_PduCallMethodReq_S *callMethodMsg = (SSAP_PduCallMethodReq_S *)buf;
    if (callMethodMsg->ctrl.fragment != 0x03) {  // 判断消息是否是一个完整的数据包
        // 目前仅支持单个完整的数据包，不支持分片的处理;
        NLSTK_LOG_ERROR("[SSAP] ssap server can only handle single package, can not process fragmentation");
        SSAPS_MethodErrorProcess(link, callMethodMsg->msgCode, SSAP_ERRCODE_SERVER_FRAG, callMethodMsg->handle);
        return;
    }

    SSAP_Method_S *method = SSAPS_GetMethodByHandle(callMethodMsg->handle);
    if (method == NULL) {
        // 如果没有找到对应的方法，则返回错误，需要生成错误响应
        // 如果handle不存在则返回SSAP_ERRCODE_INVALID_HANDLE
        // 如果handle是其他条目类型，则返回SSAP_ERRCODE_METHOD_ACCESS
        if (!SSAPS_IsHandleExist(callMethodMsg->handle)) {
            SSAPS_MethodErrorProcess(link, callMethodMsg->msgCode, SSAP_ERRCODE_INVALID_HANDLE, callMethodMsg->handle);
        } else {
            SSAPS_MethodErrorProcess(link, callMethodMsg->msgCode, SSAP_ERRCODE_METHOD_ACCESS, callMethodMsg->handle);
        }
        return;
    }

    uint8_t errorCode = SSAPS_MethodCallCheck(method->permission.permissionValue, link);
    if (errorCode != SSAP_ERRCODE_SUCCESS) {
        // 如果权限检查失败，则返回错误，需要生成错误响应
        SSAPS_MethodErrorProcess(link, callMethodMsg->msgCode, errorCode, callMethodMsg->handle);
        return;
    }

    // 上述检查完成之后，可以开始处理方法调用
    // 在函数入口出判断了size的最大值不会超过UINT16_MAX，所以这里可以直接使用uint16_t,不会出现截断的情况；
    uint16_t paramLen = (uint16_t)(size - SSAP_METHOD_REQ_PDU_MIN_LEN);
    SSAP_BufferedOperation_S *operation =
        (SSAP_BufferedOperation_S *)SDF_MemZalloc(sizeof(SSAP_BufferedOperation_S) + paramLen);
    if (operation == NULL) {
        // 如果内存申请失败，则返回错误，需要生成错误响应
        SSAPS_MethodErrorProcess(link, callMethodMsg->msgCode, SSAP_ERRCODE_NO_RESOURCE, callMethodMsg->handle);
        return;
    }
    uint32_t ret = SsapGenerateMethodCallOp(operation, callMethodMsg, paramLen, &link->addr);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        SDF_MemFree(operation);
        return;
    }
    operation->needAuth =
        ((method->permission.permissionValue & (uint8_t)SSAP_PERMISSION_ENCRYPTION_NEED) != 0) ? true : false;
    SSAPS_PushOperationPenddingVector(operation);
    // 任务添加到队列中之后，还需要启动一个定时器，避免因为service的操作阻塞，导致协议栈一直等待，当前没有实现；
    SsapServerAppCallMethodCallback(operation);

    SDF_MemFree(operation);
    operation = NULL;
}

/**
 * @brief  接受到的无响应方法调用报文处理
 */
void SSAPS_MethodCmdHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_INFO("[SSAP] enter method cmd handle");
    SSAPS_MethodHandle(link, sdfBuff);
}

/**
 * @brief  接受到的有响应方法调用报文处理
 */
void SSAPS_MethodReqHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    NLSTK_LOG_INFO("SSAP recv method req handle");
    SSAPS_MethodHandle(link, sdfBuff);
}

/**
 * @brief  发送服务数据属性或事件变化通知（无确认）
 */
void SSAPS_ValueNtf(SSAP_Link_S *link, void *arg)
{
    CP_LOG_DEBUG("[SSAP] enter value ntf");
    SSAP_ValueInfo_S *valueNtfInfo = (SSAP_ValueInfo_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(valueNtfInfo != NULL, "[SSAP] value ntf arg is null");
    uint32_t realSize = sizeof(SSAP_PduValueNtf_S) + sizeof(SSAP_PduValueNtfItem_S) + valueNtfInfo->value.len;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] value ntf sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] value ntf create buf fail");
        return;
    }
    SSAP_PduValueNtf_S *valueNtf = (SSAP_PduValueNtf_S *)buf;
    valueNtf->msgCode = SSAP_VALUE_NTF;
    valueNtf->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    valueNtf->ctrl.type = valueNtfInfo->type;
    SSAP_PduValueNtfItem_S *valueNtfItem = (SSAP_PduValueNtfItem_S *)(valueNtf->items);
    valueNtfItem->handle = valueNtfInfo->handle;
    valueNtfItem->length = valueNtfInfo->value.len;
    (void)memcpy_s(valueNtfItem->value, valueNtfInfo->value.len, valueNtfInfo->value.value, valueNtfInfo->value.len);
    if (valueNtfInfo->value.len > (link->mtu - SSAP_PDU_BASE_LEN - SSAP_HANDLE_LEN - SSAP_VALUE_ITEM_LEN)) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] value ntf len over mtu");
        return;
    }
    link->sendFunc(link, sdfBuff, SSAP_VALUE_NTF);
}

/**
 * @brief  发送服务数据属性或事件变化指示（有确认）
 */
void SSAPS_ValueInd(SSAP_Link_S *link, void *arg)
{
    CP_LOG_INFO("[SSAP] enter value ind");
    SSAP_ValueInfo_S *valueIndInfo = (SSAP_ValueInfo_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(valueIndInfo != NULL, "[SSAP] value ind arg is null");
    uint32_t realSize = sizeof(SSAP_PduValueInd_S) + sizeof(SSAP_PduValueIndItem_S) + valueIndInfo->value.len;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] value ind sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] value ind create buf fail");
        return;
    }
    SSAP_PduValueInd_S *valueInd = (SSAP_PduValueInd_S *)buf;
    valueInd->msgCode = SSAP_VALUE_IND;
    valueInd->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    valueInd->ctrl.type = valueIndInfo->type;
    SSAP_PduValueIndItem_S *valueIndItem = (SSAP_PduValueIndItem_S *)(valueInd->items);
    valueIndItem->handle = valueIndInfo->handle;
    valueIndItem->length = valueIndInfo->value.len;
    (void)memcpy_s(valueIndItem->value, valueIndInfo->value.len, valueIndInfo->value.value, valueIndInfo->value.len);
    if (valueIndInfo->value.len > (link->mtu - SSAP_PDU_BASE_LEN - SSAP_HANDLE_LEN - SSAP_VALUE_ITEM_LEN)) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] value ind len over mtu");
        return;
    }
    link->sendFunc(link, sdfBuff, SSAP_VALUE_IND);
}

static uint16_t SSAPS_GetClientConfigValue(SSAP_Property_S *property)
{
    size_t index = 0;
    uint16_t value = 0;
    uint8_t type = DESC_TYPE_CLIENT_CONFIG;
    if (SDF_VectorFindFirst(property->descriptors, CompDescriptorType, &type, &index)) {
        SSAP_Descriptor_S *descriptor = SDF_VectorElementAt(property->descriptors, index);
        if (descriptor->val->len == SSAP_CPCD_MASK_LEN) {
            (void)memcpy_s(&value, SSAP_CPCD_MASK_LEN, descriptor->val->value, SSAP_CPCD_MASK_LEN);
        } else {
            CP_LOG_ERROR("[SSAP] get client config descriptor val len error");
        }
    }
    return value;
}

static SSAP_ClientPropertyConfigDescriptor_S *SSAPS_GetClientConfigByAddr(SSAP_Property_S *property, SLE_Addr_S *addr)
{
    size_t index = 0;
    uint8_t type = DESC_TYPE_CLIENT_CONFIG;
    SSAP_ClientPropertyConfigDescriptor_S *clientConfig = NULL;
    if (SDF_VectorFindFirst(property->descriptors, CompDescriptorType, &type, &index)) {
        SSAP_Descriptor_S *descriptor = SDF_VectorElementAt(property->descriptors, index);
        if (!SDF_VectorFindFirst(descriptor->clientConfigs, CompClientConfigAddr, addr->addr, &index)) {
            return NULL;
        }
        clientConfig = SDF_VectorElementAt(descriptor->clientConfigs, index);
    }
    return clientConfig;
}

static void SSAPS_SendMsgToAllRemote(int32_t appId, SSAP_Property_S *property, const uint16_t cpcdValue,
    const uint32_t opIndication)
{
    SDF_DListHead_S *ssapLinkList = SSAP_GetSsapLinkList();
    CP_CHECK_LOG_RETURN_VOID(!SDF_DListIsEmpty(ssapLinkList), "[SSAP] send msg to all remote ssapLinkList is empty");
    SSAP_LinkNode_S *linkNode = NULL;
    SDF_DListElmForeach(linkNode, ssapLinkList, entry) {
        SSAP_Link_S *link = linkNode->link;
        SSAP_ValueInfo_S *valueInfo = (SSAP_ValueInfo_S *)SDF_MemZalloc(sizeof(SSAP_ValueInfo_S) + property->val->len);
        CP_CHECK_LOG_RETURN_VOID(valueInfo != NULL, "[SSAP] send msg to all remote valueInfo mem new failed");
        valueInfo->handle = property->handle;
        valueInfo->value.len = property->val->len;
        valueInfo->type = SSAP_PROPERTY_NTF;
        (void)memcpy_s(valueInfo->value.value, valueInfo->value.len, property->val->value, property->val->len);
        (void)memcpy_s(&valueInfo->addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
        if (cpcdValue == SSAP_CPCD_MASK_INDICATION &&
            (opIndication & (uint32_t)SSAP_OPERATE_INDICATION_INDICATE) != 0) {
            SSAP_TaskParam_S taskParam = {.appId = appId, .arg = valueInfo, .freeFunc = SDF_MemFree,
                .func = SSAPS_ValueInd, .timeout = SSAP_INTERACTION_DEFAULT_TIMEOUT, .valid = true};
            SSAP_ProcessRequestTask(link, &taskParam, false);
        } else if (cpcdValue == SSAP_CPCD_MASK_NOTIFICATION &&
            (opIndication & (uint32_t)SSAP_OPERATE_INDICATION_NOTIFY) != 0) {
            SSAP_ProcessNormalTask(link, SSAPS_ValueNtf, valueInfo, SDF_MemFree);
        } else {
            CP_LOG_ERROR("[SSAP] send msg to all remote cpcdValue error or ind/ntf forbidden");
            SDF_MemFree(valueInfo);
        }
    }
}

static void SSAPS_SendMsgToRemoteByAddr(int32_t appId, SSAP_Property_S *property, SLE_Addr_S *addr,
    const uint16_t cpcdValue, const uint32_t opIndication)
{
    SSAP_ValueInfo_S *valueInfo = (SSAP_ValueInfo_S *)SDF_MemZalloc(sizeof(SSAP_ValueInfo_S) + property->val->len);
    CP_CHECK_LOG_RETURN_VOID(valueInfo != NULL, "[SSAP] send msg to remote malloc error");
    valueInfo->handle = property->handle;
    valueInfo->value.len = property->val->len;
    valueInfo->type = SSAP_PROPERTY_NTF;
    (void)memcpy_s(valueInfo->value.value, valueInfo->value.len, property->val->value, property->val->len);
    (void)memcpy_s(&valueInfo->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&valueInfo->addr);
    if (link == NULL) {
        SDF_MemFree(valueInfo);
        CP_LOG_ERROR("[SSAP] send msg to remote link cannot find");
        return;
    }
    if (cpcdValue == SSAP_CPCD_MASK_INDICATION &&
        (opIndication & (uint32_t)SSAP_OPERATE_INDICATION_INDICATE) != 0) {
        SSAP_TaskParam_S taskParam = {.appId = appId, .arg = valueInfo, .freeFunc = SDF_MemFree,
            .func = SSAPS_ValueInd, .timeout = SSAP_INTERACTION_DEFAULT_TIMEOUT, .valid = true};
        SSAP_ProcessRequestTask(link, &taskParam, false);
    } else if (cpcdValue == SSAP_CPCD_MASK_NOTIFICATION &&
        (opIndication & (uint32_t)SSAP_OPERATE_INDICATION_NOTIFY) != 0) {
        SSAP_ProcessNormalTask(link, SSAPS_ValueNtf, valueInfo, SDF_MemFree);
    } else {
        SDF_MemFree(valueInfo);
        CP_LOG_ERROR("[SSAP] send msg to remote cpcdValue error or ind/ntf forbidden");
    }
}

static void SSAPS_UpdateItemValue(int32_t appId, SSAP_Property_S *property, SSAP_BufferedOperation_S *updateInfo)
{
    /* 本端更新 */
    (void)SSAPS_UpdatePropertyValue(property, updateInfo->dataType, &updateInfo->value, &updateInfo->addr, false);
    uint16_t cpcdValue = 0;
    uint32_t opIndication = property->operation.operationValue;
    SSAP_ClientPropertyConfigDescriptor_S *cpcdStru = NULL;
    bool hasAddr = memcmp(updateInfo->addr.addr, g_zeroAddr, SLE_ADDR_LEN) == 0 ? false : true;
    if (!hasAddr) {
        /* 如果地址为全零，默认应用到全部对端 */
        cpcdValue = SSAPS_GetClientConfigValue(property);
        SSAPS_SendMsgToAllRemote(appId, property, cpcdValue, opIndication);
    } else {
        /* 如果没有对端写建立的cpcd副本则使用本端下发的cpcd,如果有则使用对端写建立的cpcd副本 */
        cpcdStru = SSAPS_GetClientConfigByAddr(property, &updateInfo->addr);
        if (cpcdStru == NULL) {
            cpcdValue = SSAPS_GetClientConfigValue(property);
        } else if (cpcdStru->val->len == SSAP_CPCD_MASK_LEN) {
            (void)memcpy_s(&cpcdValue, SSAP_CPCD_MASK_LEN, cpcdStru->val->value, SSAP_CPCD_MASK_LEN);
        } else {
            CP_LOG_ERROR("[SSAP] update item value descriptor val len error");
        }
        SSAPS_SendMsgToRemoteByAddr(appId, property, &updateInfo->addr, cpcdValue, opIndication);
    }
    CP_LOG_INFO("[SSAP] update item value, CPCD VAL: %d", cpcdValue);
    SsapServerUpdateValueResultCallback(updateInfo);
}

/**
 * @brief  服务端通过句柄更新本地数据
 */
void SSAPS_UpdateItemValueByHandle(void *arg)
{
    CP_LOG_DEBUG("[SSAP] enter update item value by handle");
    SSAP_BufferedOperation_S *updateInfo = (SSAP_BufferedOperation_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(updateInfo != NULL, "[SSAP] update item value by handle updateInfo is null");
    SSAP_Property_S *property = SSAPS_GetPropertyByHandle(updateInfo->propertyHandle);
    CP_CHECK_LOG_RETURN_VOID(property != NULL, "[SSAP] update item value by handle property cannot find");
    // 当前SSAP仅支持更新属性的描述符
    (void)memcpy_s(&(updateInfo->propertyUuid), sizeof(NLSTK_SsapUuid_S), &(property->uuid), sizeof(NLSTK_SsapUuid_S));
    SSAPS_UpdateItemValue(updateInfo->appId, property, updateInfo);
}

static void SSAPS_SendUserResponseCaseWrite(SSAP_Link_S *link, SSAP_BufferedOperation_S *operation, uint8_t opCode)
{
    uint8_t errorCode = SSAP_ERRCODE_SUCCESS;
    SSAP_Property_S *property = SSAPS_GetPropertyByHandle(operation->propertyHandle);
    if (property == NULL) {
        CP_LOG_ERROR("[SSAP] send user response cannot find property item");
        if (opCode == SSAP_WRITE_REQ) {
            SSAPS_SendWriteReqRsp(link, SSAP_ERRCODE_ITEM_INEXIST, operation);
        }
        return;
    }
    errorCode = SSAPS_UpdatePropertyValue(property, operation->dataType, &operation->value, &operation->addr, true);
    if (opCode == SSAP_WRITE_REQ) {
        SSAPS_SendWriteReqRsp(link, errorCode, operation);
    }
    return;
}

static void SSAPS_SendUserResponseCaseRead(SSAP_Link_S *link, SSAP_BufferedOperation_S *operation,
    SSAP_SendResponseValue_S *param)
{
    uint8_t errorCode = SSAP_ERRCODE_SUCCESS;

    SSAP_Property_S *property = SSAPS_GetPropertyByHandle(operation->propertyHandle);
    if (property == NULL) {
        CP_LOG_ERROR("[SSAP] send user response cannot find property item");
        SSAPS_SendReadReqRsp(link, operation->msgCode + 1, SSAP_ERRCODE_ITEM_INEXIST, NULL);
        return;
    }
    if (param->len != 0) {
        SSAP_LengthValue_S *responseValue = (SSAP_LengthValue_S *)SDF_MemZalloc(sizeof(SSAP_LengthValue_S) +
            param->len);
        CP_CHECK_LOG_RETURN_VOID(responseValue != NULL, "[SSAP] send user response responseValue mem failed");
        responseValue->len = param->len;
        (void)memcpy_s(responseValue->value, responseValue->len, param->data, responseValue->len);
        SSAPS_UpdatePropertyValue(property, operation->dataType, responseValue, &operation->addr, true);
        SDF_MemFree(responseValue);
    }
    SSAP_LengthValue_S *value = SSAPS_GetPropertyValue(link, property, operation->dataType, &errorCode);
    SSAPS_SendReadReqRsp(link, operation->msgCode + 1, errorCode, value);
}

/**
 * @brief 用户返回授权结果
 */
void SSAPS_SendUserResponse(void *arg)
{
    CP_LOG_INFO("[SSAP] enter send user response");
    CP_CHECK_LOG_RETURN_VOID(arg != NULL, "[SSAP] send user response arg is null");
    SLE_Addr_S linkAddr = {0};
    size_t index = 0;
    SSAP_SendResponseValue_S *param = (SSAP_SendResponseValue_S *)arg;
    SSAP_BufferedOperation_S *operation = SSAPS_PopOperationPenddingVector(param->requestId, &index);
    CP_CHECK_LOG_RETURN_VOID(operation != NULL, "[SSAP] send user response find operation by requestId failed");
    uint8_t opCode = operation->msgCode;
    (void)memcpy_s(&linkAddr, sizeof(SLE_Addr_S), &operation->addr, sizeof(SLE_Addr_S));
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&linkAddr);
    if (link == NULL) {
        CP_LOG_ERROR("[SSAP] send user response link cannot find");
        SDF_VectorRemove(g_ssapPendingVector, index);
        return;
    }
    switch (opCode) {
        case SSAP_READ_REQ:
            if (param->status == SSAP_AUTH_ALLOW) {
                SSAPS_SendUserResponseCaseRead(link, operation, param);
            } else {
                SSAPS_SendReadReqRsp(link, opCode + 1, SSAP_ERRCODE_UNAUTHORIZED, NULL);
            }
            break;
        case SSAP_WRITE_CMD:
            if (param->status == SSAP_AUTH_ALLOW) {
                SSAPS_SendUserResponseCaseWrite(link, operation, opCode);
            } else {
                CP_LOG_ERROR("[SSAP] write cmd handle write failed, errorCode = %u", SSAP_ERRCODE_UNAUTHORIZED);
            }
            break;
        case SSAP_WRITE_REQ:
            if (param->status == SSAP_AUTH_ALLOW) {
                SSAPS_SendUserResponseCaseWrite(link, operation, opCode);
            } else {
                SSAPS_SendWriteReqRsp(link, SSAP_ERRCODE_UNAUTHORIZED, operation);
            }
            break;
        default:
            break;
    }
    SDF_VectorRemove(g_ssapPendingVector, index);
}

static void SSAPS_SendMethodCallRsp(SSAP_Link_S *link, SSAP_MethodCallResValue_S *param)
{
    uint32_t realSize = sizeof(SSAP_PduCallMethodRsp_S) + param->len;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] call method rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] read rsp create buf fail");
        return;
    }
    SSAP_PduCallMethodRsp_S *callRsp = (SSAP_PduCallMethodRsp_S *)buf;
    callRsp->msgCode = SSAP_CALL_METHOD_RSP;
    callRsp->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    (void)memcpy_s(callRsp->result, param->len, param->data, param->len);
    link->sendFunc(link, sdfBuff, SSAP_CALL_METHOD_RSP);
}
/**
 * @brief 用户返回方法调用结果
 */
void SSAPS_SendMethodCallRes(void *arg)
{
    CP_LOG_INFO("[SSAP] enter send method call res");
    SSAP_MethodCallResValue_S *param = (SSAP_MethodCallResValue_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(param != NULL, "[SSAP] send method call res param is null");
    size_t index = 0;
    SSAP_BufferedOperation_S *operation = SSAPS_PopOperationPenddingVector(param->requestId, &index);
    CP_CHECK_LOG_RETURN_VOID(operation != NULL, "[SSAP] send method call res find operation by requestId failed");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&operation->addr);
    if (link == NULL) {
        CP_LOG_ERROR("[SSAP] send method call res link cannot find");
        SDF_VectorRemove(g_ssapPendingVector, index);
        return;
    }
    uint8_t opCode = operation->msgCode;
    switch (opCode) {
        case SSAP_CALL_METHOD_CMD:
            break;
        case SSAP_CALL_METHOD_REQ:
            SSAPS_SendMethodCallRsp(link, param);
            break;
        default:
            break;
    }
    SDF_VectorRemove(g_ssapPendingVector, index);
}

#ifdef __cplusplus
}
#endif
