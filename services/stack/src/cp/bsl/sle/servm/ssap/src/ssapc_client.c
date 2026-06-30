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
#include "sdf_buff.h"
#include "sdf_trace.h"
#include "cpfwk_log.h"
#include "cm_logic_link_api.h"
#include "ssap_type.h"
#include "ssap_pkt.h"
#include "ssap_utils.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssap_common.h"
#include "ssapc_app.h"
#include "ssapc_cache.h"
#include "ssapc_client.h"
#include "ssapc_client_api.h"

#ifdef __cplusplus
extern "C" {
#endif

static void GetFindReqUuid(NLSTK_SsapUuid_S *uuid, uint8_t *buf, uint32_t size)
{
    if (size == 0) {
        return;
    }
    if (size != SSAP_UUID128_LEN && size != SSAP_UUID16_LEN) {
        CP_LOG_ERROR("[SSAP] get find req uuid failed, size: %d", size);
        return;
    }
    SSAP_GetUuidFromPktBuf(uuid, buf, size);
}

void SSAPC_ExchangeInfoErrorHandle(SSAP_Link_S *link, uint8_t errCode)
{
    SDF_Buff_S *lastBuff = SSAP_GetLastBuff(link);
    CP_CHECK_LOG_RETURN_VOID(lastBuff != NULL, "[SSAP] lastBuff is NULL");
    uint8_t *lastBuf = SDF_DataOffset(lastBuff);
    SSAP_PduExchangePkt_S *exchangeInfoReqInfo = (SSAP_PduExchangePkt_S *)lastBuf;
    SSAP_ExchangeComplete_S complete = {0};
    complete.mtu = exchangeInfoReqInfo->msgMtu;
    complete.version = exchangeInfoReqInfo->msgVersion;
    complete.errCode = errCode;
    (void)memcpy_s(&complete.addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    SsapTaskExecuteCallback(link, &complete);
}

void SSAPC_FindReqErrorHandle(SSAP_Link_S *link, uint8_t errCode)
{
    CP_LOG_DEBUG("[SSAP] enter find req error handle.");
    SDF_Buff_S *lastBuff = SSAP_GetLastBuff(link);
    CP_CHECK_LOG_RETURN_VOID(lastBuff != NULL, "[SSAP] FindRspErrorHandle lastBuff is NULL");
    uint8_t *lastBuf = SDF_DataOffset(lastBuff);
    uint32_t lastSize = (uint32_t)SDF_DataLenGet(lastBuff);
    uint32_t reqUuidSize = lastSize - sizeof(SSAP_PduFindStructReq_S);
    SSAP_PduFindStructReq_S *req = (SSAP_PduFindStructReq_S *)lastBuf;
    uint8_t preFindType = req->ctrl.findType;
    SSAP_DiscoveryComplete_S complete = {0};
    NLSTK_SsapUuid_S reqUuid = {0};
    GetFindReqUuid(&reqUuid, req->uuid, reqUuidSize);
    complete.opCode = req->msgCode;
    complete.lcid = link->lcid;
    complete.type = preFindType;
    complete.errCode = errCode;
    complete.preFindHandle = req->startHandle;
    (void)memcpy_s(&complete.uuid, sizeof(NLSTK_SsapUuid_S), &reqUuid, sizeof(NLSTK_SsapUuid_S));
    (void)memcpy_s(&complete.addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    CP_LOG_INFO("[SSAP] discovery complete callback, opcode: %u, lcid: %u, type: %u, errcode: %u, addr: %s,",
        complete.opCode, complete.lcid, complete.type, complete.errCode, GET_ENC_ADDR(&complete.addr));
    SsapTaskExecuteCallback(link, &complete);
}

void SSAPC_ValueErrorHandle(SSAP_Link_S *link, uint8_t errCode)
{
    CP_LOG_INFO("[SSAP] enter value error handle.");
    SSAP_ValuePkt_S valuePkt = {0};
    valuePkt.opCode = (link->curTask.opcode + 1);
    valuePkt.errorCode = errCode;
    (void)memcpy_s(&valuePkt.addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    CP_LOG_INFO("[SSAP] value complete callback, opcode: %u, errcode: %u, addr: %s,",
        valuePkt.opCode, valuePkt.errorCode, GET_ENC_ADDR(&valuePkt.addr));
    SsapTaskExecuteCallback(link, &valuePkt);
}

void SSAPC_ReadByUuidErrorHandle(SSAP_Link_S *link, uint8_t errCode)
{
    CP_LOG_INFO("[SSAP] enter read by uuid error handle");
    SDF_Buff_S *lastBuff = SSAP_GetLastBuff(link);
    CP_CHECK_LOG_RETURN_VOID(lastBuff != NULL, "[SSAP] lastBuff is NULL");
    uint8_t *lastBuf = SDF_DataOffset(lastBuff);
    uint32_t lastSize = (uint32_t)SDF_DataLenGet(lastBuff);
    SSAP_PduReadByUuidReq_S *req = (SSAP_PduReadByUuidReq_S *)lastBuf;
    uint32_t reqUuidSize = lastSize - sizeof(SSAP_PduReadByUuidReq_S);
    SSAP_ReadByUuidComplete_S complete = {0};
    SSAP_GetUuidFromPktBuf(&(complete.uuid), req->uuid, reqUuidSize);
    complete.dataType = req->dataType;
    complete.beginHandle = req->handleStart;
    complete.endHandle = req->handleEnd;
    complete.errCode = errCode;
    (void)memcpy_s(&complete.addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    CP_LOG_INFO("[SSAP] read by uuid callback, errcode: %u, addr: %s,",
        complete.errCode, GET_ENC_ADDR(&complete.addr));
    SsapTaskExecuteCallback(link, &complete);
}

void SSAPC_CallMethodErrorHandle(SSAP_Link_S *link, uint8_t errCode)
{
    CP_LOG_INFO("[SSAP] enter call method error handle");
    SDF_Buff_S *lastBuff = SSAP_GetLastBuff(link);
    CP_CHECK_LOG_RETURN_VOID(lastBuff != NULL, "[SSAP] lastBuff is NULL");
    uint8_t *lastBuf = SDF_DataOffset(lastBuff);
    SSAP_PduCallMethodReq_S *req = (SSAP_PduCallMethodReq_S *)lastBuf;
    SSAP_MethodResult_S result = {0};
    result.errorCode = errCode;
    result.handle = req->handle;
    (void)memcpy_s(&result.addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    CP_LOG_INFO("[SSAP] method call req callback, errcode: %u, addr: %s,",
        result.errorCode, GET_ENC_ADDR(&result.addr));
    SsapTaskExecuteCallback(link, &result);
}

/**
 * @brief  接收到的MTU信息交换响应报文处理
 */
void SSAPC_ExchangeInfoRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_DEBUG("[SSAP] enter exchange rsp handle");
    uint64_t len = SDF_DataLenGet(sdfBuff);
    SSAP_ExchangeComplete_S complete = {0};
    if (len < SSAP_EXCHANGE_INFO_PKT_LEN) {
        CP_LOG_ERROR("[SSAP] exchange data len error");
        complete.errCode = SSAP_ERRCODE_INVALID_PDU;
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_EXCHANGE_INFO_RSP_RECV, EXCEP_SSAP_INVALID_PDU);
        (void)memcpy_s(&complete.addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
        SsapTaskExecuteCallback(link, &complete);
        return;
    }

    SSAP_PduExchangePkt_S *exchangePkt = (SSAP_PduExchangePkt_S *)SDF_DataOffset(sdfBuff);
    CP_CHECK_LOG_RETURN_VOID(exchangePkt != NULL, "[SSAP] exchange data is null");

    uint16_t mtu = 0;
    uint16_t version = 0;
    if (exchangePkt->ctrl.mtu != 0) {
        CP_LOG_INFO("[SSAP] exchange mtu is: %d", exchangePkt->msgMtu);
        mtu = exchangePkt->msgMtu;
    }
    if (exchangePkt->ctrl.version != 0) {
        CP_LOG_INFO("[SSAP] exchange version is: %d", exchangePkt->msgVersion);
        version = exchangePkt->msgVersion;
    }
    if (mtu < SSAP_STACK_MTU_DEFAULT) {
        CP_LOG_ERROR("[SSAP] mtu is less than default, mtu: %d", mtu);
        link->mtu = SSAP_STACK_MTU_DEFAULT;
    } else if (mtu > SSAP_STACK_MTU_MAX) {
        CP_LOG_ERROR("[SSAP] mtu is greater than default, mtu: %d", mtu);
        link->mtu = SSAP_STACK_MTU_MAX;
    } else {
        link->mtu = mtu;
    }
    complete.errCode = SSAP_ERRCODE_SUCCESS;
    (void)memcpy_s(&complete.addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    complete.mtu = link->mtu;
    complete.version = version;
    SsapTaskExecuteCallback(link, &complete);
}

static void SSAP_DecodeSinglePrimaryService(SSAP_Link_S *link, uint8_t *data, uint32_t size, uint32_t uuidLen)
{
    uint32_t needSize = SSAP_HANDLE_LEN + SSAP_HANDLE_LEN + uuidLen + SSAP_FIND_PRIMARY_SERVICE_MEMBER_LEN;
    CP_CHECK_LOG_RETURN_VOID(size >= needSize, "[SSAP] decode single primary service size error, %d", size);
    uint8_t *curData = data;
    SsapCacheServInfo_S service = {0};
    service.handle = SSAP_BYTE_TO_UINT16_LITTLE(curData);
    curData += SSAP_HANDLE_LEN;
    service.endHandle = SSAP_BYTE_TO_UINT16_LITTLE(curData);
    curData += SSAP_HANDLE_LEN;
    SSAP_GetUuidFromPktBuf(&service.uuid, curData, uuidLen);
    curData += uuidLen;
    service.memberValue = *curData;
    service.serviceType =
        uuidLen == SSAP_UUID16_LEN ? ITEM_TYPE_STD_PRIMARY_SERVICE : ITEM_TYPE_VENDOR_PRIMARY_SERVICE;
    CP_LOG_INFO("[SSAP] decode service start handle: %d, end handle: %d, uuid: %s, member val: %d",
        service.handle, service.endHandle, SSAP_GET_ENC_UUID(&service.uuid), service.memberValue);
    SsapcCacheServ(&link->addr, &service);
}

static void SSAP_DecodeSinglePrimaryServiceV10(SSAP_Link_S *link, uint8_t *data, uint32_t size,
    NLSTK_SsapUuid_S *reqUuid)
{
    uint32_t needSize = SSAP_HANDLE_LEN + SSAP_HANDLE_LEN + SSAP_FIND_PRIMARY_SERVICE_MEMBER_LEN;
    CP_CHECK_LOG_RETURN_VOID(size >= needSize, "[SSAP] decode single primary service v10 size error");
    uint8_t *curData = data;
    SsapCacheServInfo_S service = {0};
    service.handle = SSAP_BYTE_TO_UINT16_LITTLE(curData);
    curData += SSAP_HANDLE_LEN;
    service.endHandle = SSAP_BYTE_TO_UINT16_LITTLE(curData);
    curData += SSAP_HANDLE_LEN;
    (void)memcpy_s(&service.uuid, sizeof(NLSTK_SsapUuid_S), reqUuid, sizeof(NLSTK_SsapUuid_S));
    service.memberValue = *curData;
    service.serviceType =
        SSAP_CheckUuidStd(reqUuid) ? ITEM_TYPE_STD_PRIMARY_SERVICE : ITEM_TYPE_VENDOR_PRIMARY_SERVICE;
    CP_LOG_INFO("[SSAP] decode service start handle: %d, end handle: %d, uuid: %s, member val: %d",
        service.handle, service.endHandle, SSAP_GET_ENC_UUID(&service.uuid), service.memberValue);
    SsapcCacheServ(&link->addr, &service);
}

static void SSAP_DecodeSingleTypePrimaryService(SSAP_Link_S *link, uint8_t *data, uint32_t size, uint32_t uuidLen)
{
    uint32_t leftSize = size;
    uint8_t *leftData = data;
    uint32_t singleServiceLen = SSAP_FIND_PRIMARY_SERVICE_BASE_LEN + uuidLen;
    while (leftSize >= singleServiceLen) {
        SSAP_DecodeSinglePrimaryService(link, leftData, leftSize, uuidLen);
        leftSize -= singleServiceLen;
        leftData += singleServiceLen;
    }
}

static void SSAP_DecodeSingleTypePrimaryServiceV10(SSAP_Link_S *link, uint8_t *data, uint32_t size,
    NLSTK_SsapUuid_S *reqUuid)
{
    uint32_t leftSize = size;
    uint8_t *leftData = data;
    uint32_t singleServiceLen = SSAP_FIND_PRIMARY_SERVICE_BASE_LEN;
    while (leftSize >= singleServiceLen) {
        SSAP_DecodeSinglePrimaryServiceV10(link, leftData, leftSize, reqUuid);
        leftSize -= singleServiceLen;
        leftData += singleServiceLen;
    }
}

static void SSAP_DecodeMixPrimaryService(SSAP_Link_S *link, uint8_t *data, uint32_t size)
{
    uint32_t leftSize = size;
    uint8_t *leftData = data;
    while (leftSize >= SSAP_FIND_INFO_INDICATION_LEN) {
        SSAP_FindInfoIndicator_S *indicator = (SSAP_FindInfoIndicator_S *)leftData;
        leftSize -= SSAP_FIND_INFO_INDICATION_LEN;
        leftData += SSAP_FIND_INFO_INDICATION_LEN;
        uint8_t leftCount = indicator->count;
        uint32_t uuidLen = 0;
        if (indicator->type == 0) {
            uuidLen = SSAP_UUID16_LEN;
        } else if (indicator->type == 1) {
            uuidLen = SSAP_UUID128_LEN;
        }
        uint32_t singleServiceLen = SSAP_FIND_PRIMARY_SERVICE_BASE_LEN + uuidLen;
        while (leftCount > 0 && leftSize >= singleServiceLen) {
            SSAP_DecodeSinglePrimaryService(link, leftData, leftSize, uuidLen);
            leftSize -= singleServiceLen;
            leftData += singleServiceLen;
            leftCount--;
        }
    }
}

static void SSAP_DecodeMixPrimaryServiceV10(SSAP_Link_S *link, uint8_t *data, uint32_t size, NLSTK_SsapUuid_S *uuid)
{
    uint32_t leftSize = size;
    uint8_t *leftData = data;
    while (leftSize > 0) {
        SSAP_FindInfoIndicator_S *indicator = (SSAP_FindInfoIndicator_S *)leftData;
        leftSize -= SSAP_FIND_INFO_INDICATION_LEN;
        leftData += SSAP_FIND_INFO_INDICATION_LEN;
        uint8_t leftCount = indicator->count;
        uint32_t singleServiceLen = SSAP_FIND_PRIMARY_SERVICE_BASE_LEN;
        while (leftCount > 0 && leftSize >= singleServiceLen) {
            SSAP_DecodeSinglePrimaryServiceV10(link, leftData, leftSize, uuid);
            leftSize -= singleServiceLen;
            leftData += singleServiceLen;
            leftCount--;
        }
    }
}

static void SSAP_DecodePrimaryService(SSAP_Link_S *link, SSAP_PduFindStructRsp_S *rsp, uint8_t *data, uint32_t size)
{
    if (rsp->ctrl.itemType == FIND_ITEM_TYPE_STANDARD) {
        SSAP_DecodeSingleTypePrimaryService(link, data, size, SSAP_UUID16_LEN);
    } else if (rsp->ctrl.itemType == FIND_ITEM_TYPE_CUSTOMIZE) {
        SSAP_DecodeSingleTypePrimaryService(link, data, size, SSAP_UUID128_LEN);
    } else if (rsp->ctrl.itemType == FIND_ITEM_TYPE_MIX) {
        SSAP_DecodeMixPrimaryService(link, data, size);
    } else {
        CP_LOG_ERROR("[SSAP] find rsp wrong ctrl item type: %d", rsp->ctrl.itemType);
    }
}

static void SSAP_DecodePrimaryServiceByUuidV10(SSAP_Link_S *link, SSAP_PduFindStructRsp_S *rsp, uint8_t *data,
    uint32_t size, NLSTK_SsapUuid_S *reqUuid)
{
    if (rsp->ctrl.itemType == FIND_ITEM_TYPE_STANDARD || rsp->ctrl.itemType == FIND_ITEM_TYPE_CUSTOMIZE) {
        SSAP_DecodeSingleTypePrimaryServiceV10(link, data, size, reqUuid);
    } else if (rsp->ctrl.itemType == FIND_ITEM_TYPE_MIX) {
        SSAP_DecodeMixPrimaryServiceV10(link, data, size, reqUuid);
    } else {
        CP_LOG_ERROR("[SSAP] find rsp wrong ctrl item type: %d", rsp->ctrl.itemType);
    }
}

static void SsapcCacheCharacter(SLE_Addr_S *addr, NLSTK_SsapPrty_S *prty, uint8_t memberType)
{
    if (memberType == MEMBER_TYPE_PROPERTY) {
        SsapcCachePrty(addr, prty);
    } else if (memberType == MEMBER_TYPE_METHOD) {
        SsapcCacheMethod(addr, prty);
    } else if (memberType == MEMBER_TYPE_EVENT) {
        SsapcCacheEvent(addr, prty);
    }
}

static void SSAP_DecodeSingleProperty(SSAP_Link_S *link, uint8_t *data, uint32_t size, uint32_t uuidLen,
    uint8_t descriptorCount, uint8_t memberType)
{
    uint32_t needSize = SSAP_HANDLE_LEN + SSAP_FIND_OPERATION_LEN + uuidLen +
        SSAP_FIND_DESCRIPTOR_COUNT_LEN + descriptorCount;
    CP_CHECK_LOG_RETURN_VOID(size >= needSize, "[SSAP] decode single property size error");
    uint8_t *curData = data;
    NLSTK_SsapPrty_S character = {0};
    if (descriptorCount != 0) {
        character.descriptors = (NLSTK_SsapDtor_S *)SDF_MemZalloc(descriptorCount * sizeof(NLSTK_SsapDtor_S));
        CP_CHECK_LOG_RETURN_VOID(character.descriptors != NULL, "[SSAP] descriptors malloc error");
    }
    character.handle = SSAP_BYTE_TO_UINT16_LITTLE(curData);
    curData += SSAP_HANDLE_LEN;
    SSAP_GetUuidFromPktBuf(&character.uuid, curData, uuidLen);
    curData += uuidLen;
    character.operation.operationValue = SSAP_BYTE_TO_UINT32_LITTLE(curData);
    curData += SSAP_FIND_OPERATION_LEN;
    character.descriptorNum = descriptorCount;
    curData += SSAP_FIND_DESCRIPTOR_COUNT_LEN;
    for (uint8_t i = 0; i < descriptorCount; i++) {
        character.descriptors[i].type = *(curData + i);
    }
    CP_LOG_INFO("[SSAP] decode property handle: %d, uuid: %s, op val: %d, desc count: %d",
        character.handle, SSAP_GET_ENC_UUID(&character.uuid), character.operation.operationValue,
        character.descriptorNum);
    SsapcCacheCharacter(&link->addr, &character, memberType);
    SDF_MemFree(character.descriptors);
}

static void SSAP_DecodeSinglePropertyV10(SSAP_Link_S *link, uint8_t *data, uint32_t size, uint8_t descriptorCount,
    NLSTK_SsapUuid_S *reqUuid)
{
    uint32_t needSize = SSAP_HANDLE_LEN + SSAP_FIND_OPERATION_LEN + SSAP_FIND_DESCRIPTOR_COUNT_LEN + descriptorCount;
    CP_CHECK_LOG_RETURN_VOID(size >= needSize, "[SSAP] decode single property v10 size error");
    uint8_t *curData = data;
    NLSTK_SsapPrty_S character = {0};
    if (descriptorCount != 0) {
        character.descriptors = (NLSTK_SsapDtor_S *)SDF_MemZalloc(descriptorCount * sizeof(NLSTK_SsapDtor_S));
        CP_CHECK_LOG_RETURN_VOID(character.descriptors != NULL, "[SSAP] descriptors malloc error");
    }
    character.handle = SSAP_BYTE_TO_UINT16_LITTLE(curData);
    curData += SSAP_HANDLE_LEN;
    (void)memcpy_s(&character.uuid, sizeof(NLSTK_SsapUuid_S), reqUuid, sizeof(NLSTK_SsapUuid_S));
    character.operation.operationValue = SSAP_BYTE_TO_UINT32_LITTLE(curData);
    curData += SSAP_FIND_OPERATION_LEN;
    character.descriptorNum = descriptorCount;
    curData += SSAP_FIND_DESCRIPTOR_COUNT_LEN;
    for (uint8_t i = 0; i < descriptorCount; i++) {
        character.descriptors[i].type = *(curData + i);
    }
    CP_LOG_INFO("[SSAP] decode property handle: %d, uuid: %s, op val: %d, desc count: %d",
        character.handle, SSAP_GET_ENC_UUID(&character.uuid), character.operation.operationValue,
        character.descriptorNum);
    SsapcCachePrty(&link->addr, &character);
    SDF_MemFree(character.descriptors);
}

static void SSAP_DecodeSingleTypeProperty(SSAP_Link_S *link, uint8_t *data, uint32_t size, uint32_t uuidLen,
    uint8_t memberType)
{
    uint32_t leftSize = size;
    uint8_t *leftData = data;
    uint32_t singlePropertyLen = SSAP_FIND_PROPERTY_BASE_LEN + uuidLen;
    while (leftSize >= singlePropertyLen) {
        uint8_t descriptorCount = *(leftData + SSAP_HANDLE_LEN + uuidLen + SSAP_FIND_OPERATION_LEN);
        if (leftSize < singlePropertyLen + descriptorCount) {
            CP_LOG_ERROR("[SSAP] find rsp decode property wrong");
            return;
        }
        SSAP_DecodeSingleProperty(link, leftData, leftSize, uuidLen, descriptorCount, memberType);
        leftSize -= singlePropertyLen + descriptorCount;
        leftData += singlePropertyLen + descriptorCount;
    }
}

static void SSAP_DecodeSingleTypePropertyV10(SSAP_Link_S *link, uint8_t *data, uint32_t size,
    NLSTK_SsapUuid_S *reqUuid)
{
    uint32_t leftSize = size;
    uint8_t *leftData = data;
    uint32_t singlePropertyLen = SSAP_FIND_PROPERTY_BASE_LEN;
    while (leftSize >= singlePropertyLen) {
        uint8_t descriptorCount = *(leftData + SSAP_HANDLE_LEN + SSAP_FIND_OPERATION_LEN);
        uint32_t needSize = singlePropertyLen + descriptorCount;
        if (leftSize < needSize) {
            CP_LOG_ERROR("[SSAP] find rsp decode property wrong");
            return;
        }
        SSAP_DecodeSinglePropertyV10(link, leftData, needSize, descriptorCount, reqUuid);
        leftSize -= needSize;
        leftData += needSize;
    }
}

static void SSAP_DecodeMixProperty(SSAP_Link_S *link, uint8_t *data, uint32_t size, uint8_t memberType)
{
    uint32_t leftSize = size;
    uint8_t *leftData = data;
    while (leftSize > 0) {
        SSAP_FindInfoIndicator_S *indicator = (SSAP_FindInfoIndicator_S *)leftData;
        leftSize -= SSAP_FIND_INFO_INDICATION_LEN;
        leftData += SSAP_FIND_INFO_INDICATION_LEN;
        uint8_t leftCount = indicator->count;
        uint32_t uuidLen = 0;
        if (indicator->type == 0) {
            uuidLen = SSAP_UUID16_LEN;
        } else if (indicator->type == 1) {
            uuidLen = SSAP_UUID128_LEN;
        }
        uint32_t singlePropertyLen = SSAP_FIND_PROPERTY_BASE_LEN + uuidLen;
        while (leftCount > 0 && leftSize >= singlePropertyLen) {
            uint8_t descriptorCount = *(leftData + SSAP_HANDLE_LEN + uuidLen + SSAP_FIND_OPERATION_LEN);
            if (leftSize < singlePropertyLen + descriptorCount) {
                CP_LOG_ERROR("[SSAP] find rsp decode property wrong");
                return;
            }
            SSAP_DecodeSingleProperty(link, leftData, leftSize, uuidLen, descriptorCount, memberType);
            leftSize -= singlePropertyLen + descriptorCount;
            leftData += singlePropertyLen + descriptorCount;
            leftCount--;
        }
    }
}

static void SSAP_DecodeMixPropertyV10(SSAP_Link_S *link, uint8_t *data, uint32_t size, NLSTK_SsapUuid_S *reqUuid)
{
    uint32_t leftSize = size;
    uint8_t *leftData = data;
    while (leftSize > 0) {
        SSAP_FindInfoIndicator_S *indicator = (SSAP_FindInfoIndicator_S *)leftData;
        leftSize -= SSAP_FIND_INFO_INDICATION_LEN;
        leftData += SSAP_FIND_INFO_INDICATION_LEN;
        uint8_t leftCount = indicator->count;
        uint32_t singlePropertyLen = SSAP_FIND_PROPERTY_BASE_LEN;
        while (leftCount > 0 && leftSize >= singlePropertyLen) {
            uint8_t descriptorCount = *(leftData + SSAP_HANDLE_LEN + SSAP_FIND_OPERATION_LEN);
            uint32_t needSize = singlePropertyLen + descriptorCount;
            if (leftSize < needSize) {
                CP_LOG_ERROR("[SSAP] find rsp decode property wrong");
                return;
            }
            SSAP_DecodeSinglePropertyV10(link, leftData, needSize, descriptorCount, reqUuid);
            leftSize -= needSize;
            leftData += needSize;
            leftCount--;
        }
    }
}

static void SSAP_DecodeProperty(SSAP_Link_S *link, SSAP_PduFindStructRsp_S *rsp, uint8_t *data, uint32_t size,
    uint8_t memberType)
{
    if (rsp->ctrl.itemType == FIND_ITEM_TYPE_STANDARD) {
        SSAP_DecodeSingleTypeProperty(link, data, size, SSAP_UUID16_LEN, memberType);
    } else if (rsp->ctrl.itemType == FIND_ITEM_TYPE_CUSTOMIZE) {
        SSAP_DecodeSingleTypeProperty(link, data, size, SSAP_UUID128_LEN, memberType);
    } else if (rsp->ctrl.itemType == FIND_ITEM_TYPE_MIX) {
        SSAP_DecodeMixProperty(link, data, size, memberType);
    } else {
        CP_LOG_ERROR("[SSAP] find rsp wrong ctrl item type: %d", rsp->ctrl.itemType);
    }
}

static void SSAP_DecodePropertyByUuidV10(SSAP_Link_S *link, SSAP_PduFindStructRsp_S *rsp, uint8_t *data, uint32_t size,
    NLSTK_SsapUuid_S *reqUuid)
{
    if (rsp->ctrl.itemType == FIND_ITEM_TYPE_STANDARD || rsp->ctrl.itemType == FIND_ITEM_TYPE_CUSTOMIZE) {
        SSAP_DecodeSingleTypePropertyV10(link, data, size, reqUuid);
    } else if (rsp->ctrl.itemType == FIND_ITEM_TYPE_MIX) {
        SSAP_DecodeMixPropertyV10(link, data, size, reqUuid);
    } else {
        CP_LOG_ERROR("[SSAP] find rsp wrong ctrl item type: %d", rsp->ctrl.itemType);
    }
}

/* 忽略服务引用，服务声明的描述符类型列表 */
static void SSAP_DecodeSingleServiceMixInStru(SSAP_Link_S *link, uint8_t *data, uint32_t uuidLen,
    uint8_t descriptorCount)
{
    // 前面已经校验过size合法
    uint8_t *curData = data;
    SsapCacheServInfo_S service = {0};
    service.handle = SSAP_BYTE_TO_UINT16_LITTLE(curData);
    curData += SSAP_HANDLE_LEN;
    // 需要在解析完该服务所有成员后才能确定结束句柄,这里为了方便缓存成员先设最大，解析到下个服务或解析结束时更新
    service.endHandle = SSAP_END_HANDLE;

    service.serviceType = *curData;
    curData += SSAP_FIND_STRUCTURE_TYPE_LEN;

    SSAP_GetUuidFromPktBuf(&service.uuid, curData, uuidLen);
    curData += uuidLen;

    // 服务声明描述符暂不支持，直接跳过
    curData += SSAP_FIND_DESCRIPTOR_COUNT_LEN;
    curData += descriptorCount;

    // 缓存到SsapcCache_S g_ssapcCache->serv中，endHandle和memberValue无效，该服务全部解析结束后更新
    CP_LOG_INFO("[SSAP] decode service start handle: %d, uuid: %s, itemType: %d", service.handle,
        SSAP_GET_ENC_UUID(&service.uuid), service.serviceType);
    SsapcCacheServ(&link->addr, &service);
}

static uint8_t GetMemberTypeByItemType(NLSTK_SsapItemType_E itemType)
{
    uint8_t memberType = 0;
    switch (itemType) {
        case ITEM_TYPE_STD_PROPERTY:
        case ITEM_TYPE_VENDOR_PROPERTY:
            memberType = MEMBER_TYPE_PROPERTY;
            break;
        case ITEM_TYPE_STD_METHOD:
        case ITEM_TYPE_VENDOR_METHOD:
            memberType = MEMBER_TYPE_METHOD;
            break;
        case ITEM_TYPE_STD_EVENT:
        case ITEM_TYPE_VENDOR_EVENT:
            memberType = MEMBER_TYPE_EVENT;
            break;
        default:
            CP_LOG_ERROR("[SSAP] unsupport item type: %d", itemType);
            break;
    }
    return memberType;
}

static void SSAP_DecodeSingleCharacterMixInStru(SSAP_Link_S *link, uint8_t *data, uint32_t uuidLen,
    uint8_t descriptorCount)
{
    // 前面已经校验过size合法
    uint8_t *curData = data;
    NLSTK_SsapPrty_S character = {0};
    if (descriptorCount != 0) {
        character.descriptors = (NLSTK_SsapDtor_S *)SDF_MemZalloc(descriptorCount * sizeof(NLSTK_SsapDtor_S));
        CP_CHECK_LOG_RETURN_VOID(character.descriptors != NULL, "[SSAP] descriptors malloc error");
    }
    character.handle = SSAP_BYTE_TO_UINT16_LITTLE(curData);
    curData += SSAP_HANDLE_LEN;

    NLSTK_SsapItemType_E itemType = *curData;
    curData += SSAP_FIND_STRUCTURE_TYPE_LEN;
    uint8_t memberType = GetMemberTypeByItemType(itemType);

    SSAP_GetUuidFromPktBuf(&character.uuid, curData, uuidLen);
    curData += uuidLen;

    character.operation.operationValue = SSAP_BYTE_TO_UINT32_LITTLE(curData);
    curData += SSAP_FIND_OPERATION_LEN;

    character.descriptorNum = descriptorCount;
    curData += SSAP_FIND_DESCRIPTOR_COUNT_LEN;
    for (uint8_t i = 0; i < descriptorCount; i++) {
        character.descriptors[i].type = *(curData + i);
    }
    curData += descriptorCount;

    CP_LOG_INFO("[SSAP] decode character in stru handle: %d, uuid: %s, op val: %d, desc count: %d, itemType: %d",
        character.handle, SSAP_GET_ENC_UUID(&character.uuid), character.operation.operationValue,
        character.descriptorNum, itemType);
    SsapcCacheCharacter(&link->addr, &character, memberType);

    if (character.descriptors != NULL) {
        SDF_MemFree(character.descriptors);
    }
}

static void SSAP_DecodeSingleMemberInStru(SSAP_Link_S *link, uint8_t *data, uint32_t uuidLen, uint8_t descriptorCount)
{
    // size前面已校验合法
    uint8_t *curdata = data;
    uint8_t itemType = *(curdata + SSAP_HANDLE_LEN);
    switch (itemType) {
        case ITEM_TYPE_STD_PRIMARY_SERVICE:
        case ITEM_TYPE_VENDOR_PRIMARY_SERVICE:
            // 混合模式解析单个服务，根据uuidLen确定是标准条目还是自定义条目
            SSAP_DecodeSingleServiceMixInStru(link, curdata, uuidLen, descriptorCount);
            break;
        case ITEM_TYPE_STD_PROPERTY:
        case ITEM_TYPE_VENDOR_PROPERTY:
        case ITEM_TYPE_STD_METHOD:
        case ITEM_TYPE_VENDOR_METHOD:
        case ITEM_TYPE_STD_EVENT:
        case ITEM_TYPE_VENDOR_EVENT:
            // 解析该服务内的属性/方法/事件
            SSAP_DecodeSingleCharacterMixInStru(link, curdata, uuidLen, descriptorCount);
            break;
        default:
            CP_LOG_ERROR("[SSAP] unsupported itemType exists in findRsp: %d", itemType);
            break;
    }
}

/* 处理混合模式返回的报文，该函数进行完备的长度检查，实行尽最大努力解析策略，不会上报错误 */
static void SSAP_DecodeMixStructure(SSAP_Link_S *link, uint8_t *data, uint32_t size)
{
    uint32_t leftSize = size;
    uint8_t *leftData = data;
    uint16_t lastServHandle = SSAP_START_HANDLE;
    // 解析单个服务声明/属性/方法/事件至少需要能解析出来descriptorCount
    while (leftSize >= SSAP_FIND_STRUCTURE_BASE_LNE) {
        uint32_t uuidLen = 0;
        uint8_t itemType = *(leftData + SSAP_HANDLE_LEN);
        if ((itemType >= ITEM_TYPE_STD_PRIMARY_SERVICE) && (itemType <= ITEM_TYPE_STD_DESCRIPTOR)) {
            uuidLen = SSAP_UUID16_LEN;
        } else if ((itemType >= ITEM_TYPE_VENDOR_PRIMARY_SERVICE) && (itemType <= ITEM_TYPE_VENDOR_DESCRIPTOR)) {
            uuidLen = SSAP_UUID128_LEN;
        }
        CP_CHECK_LOG_RETURN_VOID(uuidLen != 0, "[SSAP] find rsp decode uuidLen is 0");

        // 对上个已完成解析的服务进行更新endHandle和memberValue以及对属性/方法/事件按handle排序
        if (itemType == ITEM_TYPE_STD_PRIMARY_SERVICE || itemType == ITEM_TYPE_VENDOR_PRIMARY_SERVICE) {
            if (lastServHandle != SSAP_START_HANDLE) {
                CP_CHECK_LOG_RETURN_VOID(SsapcCacheSortCachedServ(&link->addr, lastServHandle) == NLSTK_ERRCODE_SUCCESS,
                    "[SSAP] error when sort cached service");
            }
            lastServHandle = SSAP_BYTE_TO_UINT16_LITTLE(leftData);
        }
        uint8_t operationLen = GetOperationLenByItemType(itemType);
        CP_CHECK_LOG_RETURN_VOID(leftSize >= SSAP_FIND_STRUCTURE_BASE_LNE + uuidLen + operationLen,
            "[SSAP] find rsp decode descriptorCount error");
        uint8_t descriptorCount = *(leftData + SSAP_HANDLE_LEN + SSAP_FIND_STRUCTURE_TYPE_LEN + uuidLen + operationLen);
        uint32_t singleMemberLen = SSAP_FIND_STRUCTURE_BASE_LNE + uuidLen + operationLen + descriptorCount;
        if (leftSize < singleMemberLen) {
            CP_LOG_ERROR("[SSAP] find rsp decode structure size error, left size: %d, need: %d",
                leftSize, singleMemberLen);
            return;
        }

        SSAP_DecodeSingleMemberInStru(link, leftData, uuidLen, descriptorCount);
        leftSize -= singleMemberLen;
        leftData += singleMemberLen;
    }
    // 当find报文全都解析完后把最后解析的那个服务排序并更新下
    CP_CHECK_LOG_RETURN_VOID(SsapcCacheSortCachedServ(&link->addr, lastServHandle) == NLSTK_ERRCODE_SUCCESS,
        "[SSAP] error when sort cached service");
    // 剩下的数据不足以解析出来成员类型，丢弃掉
    if (leftSize > 0) {
        CP_LOG_ERROR("[SSAP] find structure rsp include extra data, size: %d", leftSize);
        return;
    }
}

static void SSAP_DecodeStructure(SSAP_Link_S *link, SSAP_PduFindStructRsp_S *rsp, uint8_t *data, uint32_t size)
{
    if (rsp->ctrl.itemType >= FIND_ITEM_TYPE_STANDARD && rsp->ctrl.itemType <= FIND_ITEM_TYPE_MIX) {
        SSAP_DecodeMixStructure(link, data, size);
    } else {
        CP_LOG_ERROR("[SSAP] find rsp wrong ctrl item type: %d", rsp->ctrl.itemType);
    }
}

static void SSAPC_FindRspHandleCbk(SSAP_Link_S *link, SSAP_PduFindStructReq_S *req, uint8_t preFindType,
    NLSTK_SsapUuid_S *uuid)
{
    SSAP_DiscoveryComplete_S complete = {0};
    complete.opCode = req->msgCode;
    complete.lcid = link->lcid;
    complete.type = preFindType;
    complete.preFindHandle = req->startHandle;
    (void)memcpy_s(&complete.uuid, sizeof(NLSTK_SsapUuid_S), uuid, sizeof(NLSTK_SsapUuid_S));
    (void)memcpy_s(&complete.addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    SsapTaskExecuteCallback(link, &complete);
}

static uint8_t GetMemberTypeByFindType(uint8_t findType)
{
    uint8_t memberType = 0;
    switch (findType) {
        case FIND_STRUCTURE_TYPE_PROPERTY:
            memberType = MEMBER_TYPE_PROPERTY;
            break;
        case FIND_STRUCTURE_TYPE_METHOD:
            memberType = MEMBER_TYPE_METHOD;
            break;
        case FIND_STRUCTURE_TYPE_EVENT:
            memberType = MEMBER_TYPE_EVENT;
            break;
        default:
            break;
    }
    return memberType;
}

static void FindRspCleanCache(SSAP_Link_S *link, uint8_t preFindType, uint16_t reqStartHandle,
    uint8_t msgCode, NLSTK_SsapUuid_S *uuid)
{
    if (((preFindType == FIND_STRUCTURE_TYPE_PRIMARY_SERVICE) || (preFindType == FIND_STRUCTURE_TYPE_SERVICE_STRUCTURE))
        && reqStartHandle == SSAP_START_HANDLE) {
        SsapcCacheCleanServ(&link->addr);
        SsapcCacheSetCurFindByUuid(&link->addr, msgCode == SSAP_FIND_STRUCTURE_REQ ? false : true, uuid);
    }
}

static bool SSAP_CheckSizeAndTrace(SSAP_Link_S *link, int sceneCode, int subSceneCode, uint32_t size)
{
    if (size < sizeof(SSAP_PduFindStructRsp_S)) {
        SDF_SsapTrace(link->addr.addr, sceneCode, subSceneCode);
        return false;
    } else {
        return true;
    }
    return false;
}

void SSAPC_FindRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_DEBUG("[SSAP] enter find rsp handle");
    uint8_t *buf = SDF_DataOffset(sdfBuff);
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(sdfBuff) <= SSAP_STACK_MTU_MAX, "[SSAP] recv datalen is invalid");
    uint32_t size = (uint32_t)SDF_DataLenGet(sdfBuff);
    SSAP_PduFindStructRsp_S *rsp = (SSAP_PduFindStructRsp_S *)buf;
    int sceneCode = rsp->msgCode == SSAP_FIND_STRUCTURE_RSP ?
            EXCEP_SSAP_FIND_STRUCTURE_RSP_RECV : EXCEP_SSAP_FIND_BY_UUID_RSP_RECV;
    CP_CHECK_LOG_RETURN_VOID(SSAP_CheckSizeAndTrace(link, sceneCode, EXCEP_SSAP_PDU_DECODE_FAILED, size),
        "[SSAP] find rsp handle wrong size, size: %d", size);
    uint8_t *data = buf + sizeof(SSAP_PduFindStructRsp_S);
    uint32_t leftSize = size - sizeof(SSAP_PduFindStructRsp_S);
    SDF_Buff_S *lastBuff = SSAP_GetLastBuff(link);
    CP_CHECK_LOG_RETURN_VOID(lastBuff != NULL, "[SSAP] FindRspHandle lastBuff is NULL");
    uint8_t *lastBuf = SDF_DataOffset(lastBuff);
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(lastBuff) <= UINT32_MAX, "[SSAP] last datalen is invalid");
    uint32_t lastSize = (uint32_t)SDF_DataLenGet(lastBuff);
    CP_CHECK_LOG_RETURN_VOID(SSAP_CheckSizeAndTrace(link, sceneCode, EXCEP_SSAP_PDU_DECODE_FAILED, size),
        "[SSAP] find rsp handle get prev req wrong size, size: %d", lastSize);
    SSAP_PduFindStructReq_S *req = (SSAP_PduFindStructReq_S *)lastBuf;
    uint32_t reqUuidSize = lastSize - sizeof(SSAP_PduFindStructReq_S);
    uint8_t preFindType = req->ctrl.findType;
    uint8_t deviceType = CM_GetLogicLinkDeviceType(link->lcid);
    CP_LOG_INFO("[SSAP] find rsp handle pre find type: %d, device type: %d, msg code: %d",
        preFindType, deviceType, req->msgCode);
    NLSTK_SsapUuid_S reqUuid = {0};
    GetFindReqUuid(&reqUuid, req->uuid, reqUuidSize);
    FindRspCleanCache(link, preFindType, req->startHandle, req->msgCode, &reqUuid);
    if (req->msgCode == SSAP_FIND_STRUCTURE_BY_UUID_REQ && deviceType == CM_DEVTYPE_OLD) {
        if (preFindType == FIND_STRUCTURE_TYPE_PRIMARY_SERVICE) {
            SSAP_DecodePrimaryServiceByUuidV10(link, rsp, data, leftSize, &reqUuid);
        } else if (preFindType == FIND_STRUCTURE_TYPE_PROPERTY) {
            SSAP_DecodePropertyByUuidV10(link, rsp, data, leftSize, &reqUuid);
        }
    } else {
        if (preFindType == FIND_STRUCTURE_TYPE_PRIMARY_SERVICE) {
            SSAP_DecodePrimaryService(link, rsp, data, leftSize);
        } else if (preFindType == FIND_STRUCTURE_TYPE_PROPERTY || preFindType == FIND_STRUCTURE_TYPE_METHOD ||
                preFindType == FIND_STRUCTURE_TYPE_EVENT) {
            SSAP_DecodeProperty(link, rsp, data, leftSize, GetMemberTypeByFindType(preFindType));
        } else if (preFindType == FIND_STRUCTURE_TYPE_SERVICE_STRUCTURE) {
            SSAP_DecodeStructure(link, rsp, data, leftSize);
        }
    }
    SSAPC_FindRspHandleCbk(link, req, preFindType, &reqUuid);
}

static void SingleReadRspDecode(SSAP_Link_S *link, SSAP_PduReadReq_S *readReq, SSAP_PduReadRsp_S *readRsp,
    uint16_t rspLen)
{
    Ssap_PduReadReqItem_S *readReqItem = (Ssap_PduReadReqItem_S *)(readReq->items);
    SSAP_ValuePkt_S *valuePkt = NULL;

    if (readRsp->ctrl.error == 0) {
        // 前置已判断rspLen大于offset
        uint16_t readItemLen = rspLen - SSAP_READ_RSP_DATA_OFFSET;
        valuePkt = (SSAP_ValuePkt_S *)SDF_MemZalloc(sizeof(SSAP_ValuePkt_S) + readItemLen);
        CP_CHECK_LOG_RETURN_VOID(valuePkt != NULL, "[SSAP] read rsp handle valuePkt malloc fail");
        valuePkt->opCode = readRsp->msgCode;
        valuePkt->controlCode = readRsp->msgCtrl;
        valuePkt->handle = readReqItem->handle;
        valuePkt->dataType = readReqItem->type;
        valuePkt->value.len = readItemLen;
        (void)memcpy_s(valuePkt->value.value, readItemLen, readRsp->items, readItemLen);
        CP_LOG_INFO("[SSAP] read rsp handle opcode: %d, ctrl: %d, value len: %d", valuePkt->opCode,
            valuePkt->controlCode, valuePkt->value.len);
    } else {
        CP_CHECK_LOG_RETURN_VOID(rspLen - SSAP_READ_RSP_DATA_OFFSET >= sizeof(SSAP_PduReadRspItem_S),
            "[SSAP] read rsp item len error");
        SSAP_PduReadRspItem_S *readRspItem = (SSAP_PduReadRspItem_S *)(readRsp->items);
        CP_CHECK_LOG_RETURN_VOID(readRspItem->length <= UINT8_MAX, "[SSAP] read rsp handle error code invalid");
        valuePkt = (SSAP_ValuePkt_S *)SDF_MemZalloc(sizeof(SSAP_ValuePkt_S));
        CP_CHECK_LOG_RETURN_VOID(valuePkt != NULL, "[SSAP] read rsp handle valuePkt malloc fail");
        valuePkt->opCode = readRsp->msgCode;
        valuePkt->controlCode = readRsp->msgCtrl;
        valuePkt->handle = readReqItem->handle;
        valuePkt->dataType = readReqItem->type;
        valuePkt->errorCode = (uint8_t)readRspItem->length;
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_READ_RSP_RECV, valuePkt->errorCode);
        CP_LOG_INFO("[SSAP] read rsp handle opcode: %d, ctrl: %d, errorCode: %d", valuePkt->opCode,
            valuePkt->controlCode, valuePkt->errorCode);
    }
    (void)memcpy_s(&valuePkt->addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    SsapTaskExecuteCallback(link, valuePkt);
    SDF_MemFree(valuePkt);
}

static SSAP_ValuePkt_S *MultiReadRspDecodeSingleItem(SSAP_PduReadRspItem_S *readRspItem, uint16_t rspLen,
    uint16_t dataLen, SSAP_ReadByHandleComplete_S *complete, uint16_t *valueLen)
{
    uint16_t errorCode = SSAP_ERRCODE_SUCCESS;
    if (readRspItem->success != 0) {
        if (rspLen < dataLen + sizeof(SSAP_PduReadRspItem_S) + readRspItem->length) {
            CP_LOG_ERROR("[SSAP] rspLen check failed, curr item len:%u", readRspItem->length);
            complete->errCode = SSAP_ERRCODE_DATA_LENGTH; // complete为上层传入栈变量的地址，不需要判空
            return NULL;
        }
        *valueLen = readRspItem->length;
    } else {
        errorCode = readRspItem->length;
    }
    SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)SDF_MemZalloc(sizeof(SSAP_ValuePkt_S) + *valueLen);
    if (valuePkt == NULL) {
        CP_LOG_ERROR("[SSAP] malloc failed in multi read rsp decode single item");
        complete->errCode = SSAP_ERRCODE_NO_RESOURCE;
        return NULL;
    }
    valuePkt->errorCode = (uint8_t)errorCode;
    valuePkt->value.len = *valueLen;
    if (*valueLen != 0) {
        (void)memcpy_s(valuePkt->value.value, *valueLen, readRspItem->value, *valueLen);
    }

    return valuePkt;
}

static void MultiReadRspDecode(SSAP_Link_S *link, Ssap_PduReadReqItem_S *readReqItems, uint8_t handleNum,
    uint8_t *data, uint16_t rspLen)
{
    uint16_t dataLen = 0;
    SSAP_ReadByHandleComplete_S complete = {.errCode = SSAP_ERRCODE_SUCCESS};
    (void)memcpy_s(&complete.addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    complete.readVals = SDF_CreateVector(MAKE_TRAITS(SDF_MemFree, NULL));
    if (complete.readVals == NULL) {
        complete.errCode = SSAP_ERRCODE_NO_RESOURCE;
        goto CB_LABEL;
    }
    while (rspLen > dataLen + sizeof(SSAP_PduReadRspItem_S)) {
        SSAP_PduReadRspItem_S *readRspItem = (SSAP_PduReadRspItem_S *)data;
        uint16_t valueLen = 0;
        // valuePkt会在下面函数内部申请空间，返回地址，后面要考虑对该空间的管理和释放问题
        SSAP_ValuePkt_S *valuePkt = MultiReadRspDecodeSingleItem(readRspItem, rspLen, dataLen, &complete, &valueLen);
        if (complete.errCode != SSAP_ERRCODE_SUCCESS || valuePkt == NULL) {
            CP_LOG_ERROR("[SSAP] decode single item failed in multi read rsp decode");
            goto CB_LABEL;
        }
        if (!SDF_VectorEmplaceBack(complete.readVals, valuePkt)) {
            SDF_MemFree(valuePkt);
            CP_LOG_ERROR("[SSAP] valuePkt emplace back failed in multi read rsp decode");
            complete.errCode = SSAP_ERRCODE_UNKNOWN;
            goto CB_LABEL;
        }
        data += sizeof(SSAP_PduReadRspItem_S) + valueLen;
        dataLen += sizeof(SSAP_PduReadRspItem_S) + valueLen;
    }
    if (complete.readVals->size != handleNum) {
        complete.errCode = SSAP_ERRCODE_UNKNOWN;
        goto CB_LABEL;
    }
    for (uint8_t i = 0; i < handleNum; i++) {
        SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)SDF_VectorElementAt(complete.readVals, i);
        valuePkt->handle = readReqItems[i].handle;
        valuePkt->dataType = readReqItems[i].type;
    }
CB_LABEL:
    CP_LOG_DEBUG("[SSAP] multi read rsp decode complete, errcode:0x%u", complete.errCode);
    SsapTaskExecuteCallback(link, &complete);
    SDF_DestroyVector(complete.readVals);
    return;
}

static uint8_t CountReqHandleNum(SDF_Buff_S *buff)
{
    uint32_t result = 0;
    CP_CHECK_LOG_RETURN(SDF_DataLenGet(buff) > sizeof(SSAP_PduReadReq_S) && SDF_DataLenGet(buff) <= UINT32_MAX, 0,
        "[SSAP] buff len error");
    uint32_t dataLen = (uint32_t)SDF_DataLenGet(buff);
    result = (dataLen - sizeof(SSAP_PduReadReq_S)) / sizeof(Ssap_PduReadReqItem_S);
    CP_CHECK_LOG_RETURN(result <= UINT8_MAX, 0, "[SSAP] read req size error");
    return (uint8_t)result;
}

/**
 * @brief  接收到的读取数据响应报文处理
 */
void SSAPC_ReadRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    SDF_Buff_S *lastBuff = SSAP_GetLastBuff(link);
    CP_CHECK_LOG_RETURN_VOID(lastBuff != NULL, "[SSAP] read rsp handle lastBuff is null");
    uint8_t *lastBuffData = SDF_DataOffset(lastBuff);
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(lastBuff) <= UINT32_MAX, "[SSAP] last datalen is invalid");
    SSAP_PduReadReq_S *readReq = (SSAP_PduReadReq_S *)lastBuffData;

    CP_LOG_DEBUG("[SSAP] enter read rsp handle");
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(sdfBuff) <= SSAP_STACK_MTU_MAX, "[SSAP] recv datalen is invalid");
    uint16_t len = (uint16_t)SDF_DataLenGet(sdfBuff);
    CP_CHECK_LOG_RETURN_VOID(len >= SSAP_READ_RSP_DATA_OFFSET, "[SSAP] read rsp handle data len error");
    SSAP_PduReadRsp_S *readRsp = (SSAP_PduReadRsp_S *)SDF_DataOffset(sdfBuff);

    uint8_t handleNum = CountReqHandleNum(lastBuff);
    if (handleNum == 1) {
        SingleReadRspDecode(link, readReq, readRsp, len);
    } else {
        MultiReadRspDecode(link, (Ssap_PduReadReqItem_S *)(readReq->items), handleNum, readRsp->items, len);
    }
}

static void SSAPC_ReadByUuidRspDecode(uint8_t controlCode, uint8_t *data, uint16_t dataLen, SDF_Vector_S *readVals)
{
    uint16_t errorCode = SSAP_ERRCODE_SUCCESS;
    uint8_t dataIndicationLen = 0;
    uint16_t dataIndication = 0;
    uint16_t index = 0;
    while (dataLen > index + SSAP_HANDLE_LEN) {
        uint16_t handle = SSAP_BYTE_TO_UINT16_LITTLE(data);
        uint16_t valueLen = dataLen - SSAP_HANDLE_LEN;
        if ((controlCode & SSAP_READ_BY_UUID_RSP_MULTI_CONTROL) != 0) {
            CP_CHECK_LOG_RETURN_VOID(dataLen >= (index + SSAP_HANDLE_LEN + SSAP_INDICATION_LEN),
                "[SSAP] data len error");
            dataIndication = SSAP_BYTE_TO_UINT16_LITTLE(data + SSAP_HANDLE_LEN);
            dataIndicationLen = SSAP_INDICATION_LEN;
            uint8_t errorFlag = (uint8_t)(dataIndication >> SSAP_READ_BY_UUID_RSP_ERROR_CONTROL);
            if (errorFlag != 0) {
                valueLen = dataIndication & ~(1 << SSAP_READ_BY_UUID_RSP_ERROR_CONTROL);
            } else {
                errorCode = dataIndication & ~(1 << SSAP_READ_BY_UUID_RSP_ERROR_CONTROL);
                valueLen = 0;
            }
        } else if ((controlCode & SSAP_READ_BY_UUID_RSP_ERR_CONTROL) != 0) {
            dataIndication = SSAP_BYTE_TO_UINT16_LITTLE(data + SSAP_HANDLE_LEN);
            errorCode = dataIndication & ~(1 << SSAP_READ_BY_UUID_RSP_ERROR_CONTROL);
            dataIndicationLen = SSAP_INDICATION_LEN;
            valueLen = 0;
        }
        CP_CHECK_LOG_RETURN_VOID(dataLen >= (index + SSAP_HANDLE_LEN + dataIndicationLen + valueLen),
            "[SSAP] data len error");
        data += SSAP_HANDLE_LEN + dataIndicationLen;
        SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)SDF_MemZalloc(sizeof(SSAP_ValuePkt_S) + valueLen);
        CP_CHECK_LOG_RETURN_VOID(valuePkt != NULL, "[SSAP] read by uuid rsp decode valuePkt memzalloc error");
        valuePkt->errorCode = (uint8_t)errorCode;
        valuePkt->controlCode = controlCode;
        valuePkt->handle = handle;
        valuePkt->value.len = valueLen;
        if (valueLen != 0) {
            (void)memcpy_s(valuePkt->value.value, valueLen, data, valueLen);
        }
        CP_LOG_INFO("[SSAP] read by uuid rsp handle: %d, errorCode: %d", valuePkt->handle, valuePkt->errorCode);
        if (!SDF_VectorEmplaceBack(readVals, valuePkt)) {
            CP_LOG_ERROR("[SSAP] read by uuid rsp decode val push vector failed");
            SDF_MemFree(valuePkt);
        }
        data += valueLen;
        index += SSAP_HANDLE_LEN + dataIndicationLen + valueLen;
    }
}

/**
 * @brief  接收到的通过UUID读取数据响应报文处理
 */
void SSAPC_ReadByUuidRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_INFO("[SSAP] enter read by uuid rsp handle");
    SSAP_ReadByUuidComplete_S complete = {0};
    (void)memcpy_s(&complete.addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    SDF_Vector_S *readVals = NULL;
    SDF_Buff_S *lastBuff = SSAP_GetLastBuff(link);
    if (lastBuff == NULL || SDF_DataLenGet(lastBuff) > UINT32_MAX ||
        SDF_DataLenGet(lastBuff) < sizeof(SSAP_PduReadByUuidReq_S)) {
        CP_LOG_ERROR("[SSAP] read by uuid rsp handle get prev req wrong size");
        complete.errCode = SSAP_ERRCODE_UNKNOWN;
        goto CB_LABEL;
    }
    uint32_t lastSize = (uint32_t)SDF_DataLenGet(lastBuff);
    SSAP_PduReadByUuidReq_S *req = (SSAP_PduReadByUuidReq_S *)SDF_DataOffset(lastBuff);
    uint32_t reqUuidSize = lastSize - sizeof(SSAP_PduReadByUuidReq_S);
    GetFindReqUuid(&(complete.uuid), req->uuid, reqUuidSize);
    complete.dataType = req->dataType;
    complete.beginHandle = req->handleStart;
    complete.endHandle = req->handleEnd;
    if (SDF_DataLenGet(sdfBuff) > SSAP_STACK_MTU_MAX || SDF_DataLenGet(sdfBuff) <= SSAP_READ_BY_UUID_RSP_DATA_OFFSET) {
        CP_LOG_ERROR("[SSAP] read by uuid rsp handle data len error");
        complete.errCode = SSAP_ERRCODE_INVALID_PDU;
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_READ_BY_UUID_RSP_RECV, SSAP_ERRCODE_INVALID_PDU);
        goto CB_LABEL;
    }
    uint16_t len = (uint16_t)SDF_DataLenGet(sdfBuff);
    SSAP_PduReadByUuidRsp_S *readByUuidRsp = (SSAP_PduReadByUuidRsp_S *)SDF_DataOffset(sdfBuff);
    uint8_t *data = readByUuidRsp->items;
    uint16_t dataLen = len - SSAP_READ_RSP_DATA_OFFSET;
    uint8_t controlCode = readByUuidRsp->msgCtrl;
    // 存读取解析到的SSAP_ValuePkt
    readVals = SDF_CreateVector(MAKE_TRAITS(SDF_MemFree, NULL));
    if (readVals == NULL) {
        complete.errCode = SSAP_ERRCODE_NO_RESOURCE;
        SsapTaskExecuteCallback(link, &complete);
        return;
    }
    SSAPC_ReadByUuidRspDecode(controlCode, data, dataLen, readVals);
    if (readVals->size == 0) {
        complete.errCode = SSAP_ERRCODE_ITEM_INEXIST;
    } else {
        complete.errCode = SSAP_ERRCODE_SUCCESS;
        complete.readVals = readVals;
    }
    CP_LOG_INFO("[SSAP] read by uuid rsp handle uuid: %s, data type: %d, begin handle: %d, end handle %d",
        SSAP_GET_ENC_UUID(&complete.uuid), complete.dataType, complete.beginHandle, complete.endHandle);
CB_LABEL:
    SsapTaskExecuteCallback(link, &complete);
    SDF_DestroyVector(readVals);
    return;
}

static void HandleWriteRspHandleError(SSAP_Link_S *link, SSAP_PduWriteReq_S *req,
    SSAP_PduWriteRsp_S *writeRsp, uint16_t len)
{
    CP_CHECK_LOG_RETURN_VOID(len == SSAP_WRITE_RSP_ERROR_OFFSET, "[SSAP] write rsp handle data len error");
    SSAP_PduWriteRspErrorItem_S *writeRspItem = (SSAP_PduWriteRspErrorItem_S *)(writeRsp->items);
    SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)SDF_MemZalloc(sizeof(SSAP_ValuePkt_S));
    CP_CHECK_LOG_RETURN_VOID(valuePkt != NULL, "[SSAP] write rsp handle valuePkt malloc fail");
    valuePkt->opCode = writeRsp->msgCode;
    valuePkt->controlCode = writeRsp->msgCtrl;
    valuePkt->handle = writeRspItem->handle;
    valuePkt->errorCode = writeRspItem->errorCode;
    SSAP_PduWriteReqItem_S *writeReqItem = (SSAP_PduWriteReqItem_S *)(req->items);
    valuePkt->dataType = writeReqItem->type;
    SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_WRITE_RSP_RECV, valuePkt->errorCode);
    CP_LOG_INFO("[SSAP] write rsp handle opcode: %d, ctrl: %d, handle: %d, error code: %d",
        valuePkt->opCode, valuePkt->controlCode, valuePkt->handle, valuePkt->errorCode);
    (void)memcpy_s(&valuePkt->addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));

    SsapTaskExecuteCallback(link, valuePkt);
    SDF_MemFree(valuePkt);
}

/**
 * @brief  接收到的写入数据响应报文处理
 */
void SSAPC_WriteRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_DEBUG("SSAP ENTER Write Rsp Handle");
    uint16_t len = (uint16_t)sdfBuff->dataLen;
    CP_CHECK_LOG_RETURN_VOID(len > SSAP_WRITE_RSP_PDU_LEN, "[SSAP] write rsp handle data len error");
    SSAP_PduWriteRsp_S *writeRsp = (SSAP_PduWriteRsp_S *)SDF_DataOffset(sdfBuff);
    SSAP_ValuePkt_S *valuePkt = NULL;
    SDF_Buff_S *lastBuff = SSAP_GetLastBuff(link);
    CP_CHECK_LOG_RETURN_VOID(lastBuff != NULL, "[SSAP] write rsq handle lastBuff is null");
    uint8_t *lastBuffData = SDF_DataOffset(lastBuff);
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(lastBuff) <= UINT32_MAX, "[SSAP] last datalen is invalid");
    uint32_t lastSize = (uint32_t)SDF_DataLenGet(lastBuff);
    CP_CHECK_LOG_RETURN_VOID(lastSize > SSAP_WRITE_REQ_DATA_OFFSET, "[SSAP] write rsq handle get prev req wrong size");
    SSAP_PduWriteReq_S *req = (SSAP_PduWriteReq_S *)lastBuffData;
    SSAP_PduWriteReqItem_S *writeReqItem = (SSAP_PduWriteReqItem_S *)(req->items);

    if (writeRsp->ctrl.result == SSAP_CTRL_WRITE_SUCCESS) {
        if (req->ctrl.verify != 0) {
            CP_CHECK_LOG_RETURN_VOID(len > SSAP_WRITE_RSP_DATA_OFFSET, "[SSAP] write rsp handle data len error");
            SSAP_PduWriteRspItem_S *writeRspItem = (SSAP_PduWriteRspItem_S *)(writeRsp->items);
            valuePkt = (SSAP_ValuePkt_S *)SDF_MemZalloc(sizeof(SSAP_ValuePkt_S) + (len - SSAP_WRITE_RSP_DATA_OFFSET));
            CP_CHECK_LOG_RETURN_VOID(valuePkt != NULL, "[SSAP] write rsp handle valuePkt malloc fail");
            valuePkt->handle = writeRspItem->handle;
            valuePkt->dataType = writeRspItem->type;
            valuePkt->value.len = len - SSAP_WRITE_RSP_DATA_OFFSET;
            (void)memcpy_s(valuePkt->value.value, valuePkt->value.len, writeRspItem->value, valuePkt->value.len);
        } else {
            CP_CHECK_LOG_RETURN_VOID(len == SSAP_PDU_BASE_LEN, "[SSAP] write rsp handle data len error");
            valuePkt = (SSAP_ValuePkt_S *)SDF_MemZalloc(sizeof(SSAP_ValuePkt_S));
            CP_CHECK_LOG_RETURN_VOID(valuePkt != NULL, "[SSAP] write rsp handle valuePkt malloc fail");
        }
        valuePkt->opCode = writeRsp->msgCode;
        valuePkt->controlCode = writeRsp->msgCtrl;
        valuePkt->dataType = writeReqItem->type;
        (void)memcpy_s(&valuePkt->addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
        SsapTaskExecuteCallback(link, valuePkt);
        SDF_MemFree(valuePkt);
    } else if ((writeRsp->ctrl.result == SSAP_CTRL_WRITE_CANCEL && req->ctrl.oper != SSAP_CTRL_WRITE_CANCEL) ||
        writeRsp->ctrl.result == SSAP_CTRL_WRITE_PART) {
        HandleWriteRspHandleError(link, req, writeRsp, len);
    }
}

static void HandleServChange(SLE_Addr_S *addr, SSAP_PduValueNtfItem_S *valueItem, uint16_t dataLen)
{
    CP_CHECK_LOG_RETURN_VOID(dataLen == SSAP_HANDLE_LEN + SSAP_HANDLE_LEN, "[SSAP] serv change length error");
    uint16_t startHandle = 0;
    uint16_t endHandle = 0;
    uint8_t *data = valueItem->value;
    (void)memcpy_s(&startHandle, SSAP_HANDLE_LEN, data, SSAP_HANDLE_LEN);
    (void)memcpy_s(&endHandle, SSAP_HANDLE_LEN, data + SSAP_HANDLE_LEN, SSAP_HANDLE_LEN);
    CP_LOG_INFO("[SSAP] serv change handle start: %d, end: %d", startHandle, endHandle);
    SsapcAppServChange(addr, startHandle, endHandle);
}

/**
 * @brief  客户端接收到的数据变化通知处理（支持多组{句柄, 数据值长度, 数据值}）
 */
static void SSAPC_HandleServChangeEvent(SLE_Addr_S *addr,
    SSAP_PduValueNtfItem_S *valueItem, uint8_t **data, size_t *leftSize)
{
    CP_LOG_INFO("[SSAP] value ntf handle recv server hash update");
    uint16_t itemTotalLen = SSAP_HANDLE_LEN + SSAP_VALUE_ITEM_LEN + valueItem->length;
    HandleServChange(addr, valueItem, valueItem->length);
    *data += itemTotalLen;
    *leftSize -= itemTotalLen;
}

static void SSAPC_BuildAndNotifyValuePkt(const SLE_Addr_S *addr, uint8_t opCode,
    uint8_t controlCode, SSAP_PduValueNtfItem_S *valueItem, uint8_t **data, size_t *leftSize)
{
    uint16_t itemTotalLen = SSAP_HANDLE_LEN + SSAP_VALUE_ITEM_LEN + valueItem->length;
    SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)SDF_MemZalloc(sizeof(SSAP_ValuePkt_S) + valueItem->length);
    if (valuePkt == NULL) {
        CP_LOG_ERROR("[SSAP] value ntf handle valuePkt malloc fail, skip item");
        *data += itemTotalLen;
        *leftSize -= itemTotalLen;
        return;
    }
    valuePkt->opCode = opCode;
    valuePkt->controlCode = controlCode;
    valuePkt->handle = valueItem->handle;
    valuePkt->value.len = valueItem->length;
    if (valueItem->length > 0) {
        (void)memcpy_s(valuePkt->value.value, valueItem->length, valueItem->value, valueItem->length);
    }
    (void)memcpy_s(&valuePkt->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    CP_LOG_INFO("[SSAP] value ntf handle opcode: %d, ctrl: %d, handle: %d, value len: %d",
        valuePkt->opCode, valuePkt->controlCode, valuePkt->handle, valuePkt->value.len);
    SsapcAppPropertyNtf(valuePkt);
    SDF_MemFree(valuePkt);
    *data += itemTotalLen;
    *leftSize -= itemTotalLen;
}

void SSAPC_ValueNtfHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_INFO("[SSAP] enter value ntf handle");
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(sdfBuff) <= SSAP_STACK_MTU_MAX, "[SSAP] recv datalen is invalid");
    uint16_t len = (uint16_t)SDF_DataLenGet(sdfBuff);
    CP_CHECK_LOG_RETURN_VOID(len >= SSAP_VALUE_NTF_PDU_MIN_LEN, "[SSAP] value ntf handle data len error");
    size_t leftSize = len - sizeof(SSAP_PduValueNtf_S);
    uint8_t *dataBuff = SDF_DataOffset(sdfBuff);
    SSAP_PduValueNtf_S *valueNtfPkt = (SSAP_PduValueNtf_S *)dataBuff;
    uint8_t *data = valueNtfPkt->items;
    while (leftSize > 0) {
        if (leftSize < SSAP_HANDLE_LEN + SSAP_VALUE_ITEM_LEN) {
            CP_LOG_ERROR("[SSAP] leftSize < SSAP_HANDLE_LEN + SSAP_VALUE_ITEM_LEN, leftSize: %zu", leftSize);
            break;
        }
        SSAP_PduValueNtfItem_S *valueItem = (SSAP_PduValueNtfItem_S *)data;
        if (leftSize - (SSAP_HANDLE_LEN + SSAP_VALUE_ITEM_LEN) < valueItem->length) {
            CP_LOG_ERROR("[SSAP] value ntf handle item overflow, itemLen: %u, leftSize: %zu",
                valueItem->length, leftSize);
            break;
        }
        if (CM_GetLogicLinkDeviceType(link->lcid) != CM_DEVTYPE_OLD &&
            valueItem->handle == SSAP_SERVICE_CHANGE_EVENT_HANDLE) {
            SSAPC_HandleServChangeEvent(&link->addr, valueItem, &data, &leftSize);
            continue;
        }
        SSAPC_BuildAndNotifyValuePkt(&link->addr, valueNtfPkt->msgCode, valueNtfPkt->msgCtrl,
            valueItem, &data, &leftSize);
    }
}

static int32_t SSAPC_ValueIndBuildSingleItemPkt(SSAP_Link_S *link, SSAP_PduValueInd_S *valueIndPkt,
    SSAP_PduValueIndItem_S *item)
{
    SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)SDF_MemZalloc(sizeof(SSAP_ValuePkt_S) + item->length);
    CP_CHECK_LOG_RETURN(valuePkt != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[SSAP] value ind handle valuePkt malloc fail");
    valuePkt->opCode = valueIndPkt->msgCode;
    valuePkt->controlCode = valueIndPkt->msgCtrl;
    valuePkt->handle = item->handle;
    valuePkt->value.len = item->length;
    if (item->length > 0) {
        (void)memcpy_s(valuePkt->value.value, item->length, item->value, item->length);
    }
    (void)memcpy_s(&valuePkt->addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    CP_LOG_INFO("[SSAP] handle %d, value len %d", valuePkt->handle, valuePkt->value.len);
    SsapcAppPropertyNtf(valuePkt);
    SDF_MemFree(valuePkt);
    return NLSTK_ERRCODE_SUCCESS;
}

static uint16_t SSAPC_ValueIndParseItems(SSAP_Link_S *link, SSAP_PduValueInd_S *valueIndPkt,
    uint8_t *data, size_t leftSize)
{
    uint16_t itemCount = 0;
    while (leftSize > 0) {
        if (leftSize < SSAP_HANDLE_LEN + SSAP_VALUE_ITEM_LEN) {
            CP_LOG_ERROR("[SSAP] leftSize < SSAP_HANDLE_LEN + SSAP_VALUE_ITEM_LEN, leftSize: %zu", leftSize);
            break;
        }
        SSAP_PduValueIndItem_S *item = (SSAP_PduValueIndItem_S *)data;
        if (leftSize - (SSAP_HANDLE_LEN + SSAP_VALUE_ITEM_LEN) < + item->length) {
            CP_LOG_ERROR("[SSAP] value ind handle item overflow, itemLen: %u, leftSize: %zu",
                item->length, leftSize);
            break;
        }
        if (SSAPC_ValueIndBuildSingleItemPkt(link, valueIndPkt, item) != NLSTK_ERRCODE_SUCCESS) {
            break;
        }
        data += SSAP_HANDLE_LEN + SSAP_VALUE_ITEM_LEN + item->length;
        leftSize -= SSAP_HANDLE_LEN + SSAP_VALUE_ITEM_LEN + item->length;
        itemCount++;
    }
    return itemCount;
}

static void SSAPC_ValueIndSendAck(SSAP_Link_S *link, uint16_t itemCount)
{
    if (itemCount == 0) {
        CP_LOG_INFO("[SSAP] itemCount == 0");
        return;
    }
    uint8_t *ackResults = SDF_MemZalloc(itemCount);
    if (ackResults == NULL) {
        CP_LOG_INFO("[SSAP] SDF_MemZalloc fail");
        return;
    }
    for (uint32_t i = 0; i < itemCount; i++) {
        ackResults[i] = SSAP_ACK_RECV_SUCCESS;
    }
    SSAP_ValueAckInfo_S *valueAck = (SSAP_ValueAckInfo_S *)SDF_MemZalloc(
        sizeof(SSAP_ValueAckInfo_S) + itemCount);
    if (valueAck != NULL) {
        (void)memcpy_s(&valueAck->addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
        valueAck->value.len = itemCount;
        (void)memcpy_s(valueAck->value.value, itemCount, ackResults, itemCount);
        SSAP_ValueAck(valueAck);
        SDF_MemFree(valueAck);
    }
    SDF_MemFree(ackResults);
}

/**
 * @brief 客户端接收到的数据变化指示处理（支持多组{句柄, 数据值长度,数据值}）
 *        需要回复ACK报文，由service层去控制
 */
void SSAPC_ValueIndHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_INFO("[SSAP] enter value ind handle");
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(sdfBuff) <= SSAP_STACK_MTU_MAX, "[SSAP] recv datalen is invalid");
    uint16_t len = (uint16_t)SDF_DataLenGet(sdfBuff);
    CP_CHECK_LOG_RETURN_VOID(len >= SSAP_VALUE_IND_PDU_MIN_LEN, "[SSAP] value ind handle data len error");
    size_t leftSize = len - sizeof(SSAP_PduValueInd_S);
    uint8_t *dataBuff = SDF_DataOffset(sdfBuff);
    SSAP_PduValueInd_S *valueIndPkt = (SSAP_PduValueInd_S *)dataBuff;
    uint8_t *data = valueIndPkt->items;
    uint16_t itemCount = SSAPC_ValueIndParseItems(link, valueIndPkt, data, leftSize);
    SSAPC_ValueIndSendAck(link, itemCount);
}

/**
 * @brief  客户端接收到的方法调用响应报文处理
 */
void SSAPC_CallMethodRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    uint32_t len = (uint32_t)SDF_DataLenGet(sdfBuff);
    CP_CHECK_LOG_RETURN_VOID(len >= sizeof(SSAP_PduCallMethodRsp_S), "SSAPC_CallMethodRspHandle data len error");
    uint8_t *buf = SDF_DataOffset(sdfBuff);
    SSAP_PduCallMethodRsp_S *data = (SSAP_PduCallMethodRsp_S *)buf;
    uint32_t dataLen = len - SSAP_PDU_BASE_LEN;
    SDF_Buff_S *lastBuff = SSAP_GetLastBuff(link);
    CP_CHECK_LOG_RETURN_VOID(lastBuff != NULL, "SSAPC_CallMethodRspHandle lastBuff is null");
    uint8_t *lastBuf = SDF_DataOffset(lastBuff);
    SSAP_PduCallMethodReq_S *req = (SSAP_PduCallMethodReq_S *)lastBuf;
    SSAP_MethodResult_S *result = (SSAP_MethodResult_S *)SDF_MemZalloc(sizeof(SSAP_MethodResult_S) + dataLen);
    CP_CHECK_LOG_RETURN_VOID(result != NULL, "SSAPC_CallMethodRspHandle malloc error");
    result->handle = req->handle;
    result->value.len = (uint16_t)dataLen;
    result->errorCode = SSAP_ERRCODE_SUCCESS;
    (void)memcpy_s(&result->addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(result->value.value, dataLen, data->result, dataLen);
    SsapTaskExecuteCallback(link, result);
    SDF_MemFree(result);
}

void SSAPC_InitFindReq(SSAP_Link_S *link, void *arg)
{
    CP_LOG_DEBUG("[SSAP] enter init find req");
    CP_CHECK_LOG_RETURN_VOID(arg != NULL, "[SSAP] init find req arg is null");
    SSAP_ParamFind_S *findParam = (SSAP_ParamFind_S *)arg;
    uint32_t realSize = sizeof(SSAP_PduFindStructReq_S);
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] init find req sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] init find req create buf fail");
        return;
    }
    SSAP_PduFindStructReq_S *findReq = (SSAP_PduFindStructReq_S *)buf;
    findReq->msgCode = SSAP_FIND_STRUCTURE_REQ;
    findReq->ctrl.findType = findParam->type;
    findReq->ctrl.itemType = FIND_ITEM_TYPE_STANDARD;
    findReq->ctrl.rspMode = FIND_RSP_MODE_SINGLE_RSP;
    findReq->startHandle = findParam->startHandle;
    findReq->endHandle = findParam->endHandle;
    CP_LOG_INFO("[SSAP] init find req find type: %d, item type: %d, rsp mode: %d, start: %d, end: %d",
        findReq->ctrl.findType, findReq->ctrl.itemType, findReq->ctrl.rspMode,
        findReq->startHandle, findReq->endHandle);
    link->sendFunc(link, sdfBuff, SSAP_FIND_STRUCTURE_REQ);
}

/**
 * @brief  客户端Mtu信息交互
 */
void SSAPC_ExchangeInfoReq(SSAP_Link_S *link, void *arg)
{
    CP_LOG_INFO("[SSAP] enter exchange info req");
    SSAP_ExchangeInfoReqInfo_S *exchangeInfoReqInfo = (SSAP_ExchangeInfoReqInfo_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(exchangeInfoReqInfo != NULL, "[SSAP] exchange info req arg is null");
    uint32_t realSize = sizeof(SSAP_PduExchangePkt_S);
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] exchange info req sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] exchange info req create buf fail");
        return;
    }
    SSAP_PduExchangePkt_S *exchangePkt = (SSAP_PduExchangePkt_S *)buf;
    exchangePkt->msgCode = SSAP_EXCHANGE_INFO_REQ;
    exchangePkt->ctrl.mtu = 1;
    exchangePkt->ctrl.version = 1;
    exchangePkt->msgMtu = exchangeInfoReqInfo->mtu;
    exchangePkt->msgVersion = SSAP_EXCHANGE_VERSION;
    CP_LOG_INFO("[SSAP] exchange info req mtu: %d, version: 0x%x", exchangePkt->msgMtu, exchangePkt->msgVersion);
    link->sendFunc(link, sdfBuff, SSAP_EXCHANGE_INFO_REQ);
}

static void SendFindPkt(SSAP_Link_S *link, SSAP_ParamFind_S *findParam)
{
    uint32_t realSize = sizeof(SSAP_PduFindStructReq_S);
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] find req sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] find req create buf fail");
        return;
    }
    SSAP_PduFindStructReq_S *findReq = (SSAP_PduFindStructReq_S *)buf;
    findReq->msgCode = SSAP_FIND_STRUCTURE_REQ;
    findReq->ctrl.findType = findParam->type;
    if (CM_GetLogicLinkDeviceType(link->lcid) == CM_DEVTYPE_OLD) {
        findReq->ctrl.itemType = FIND_ITEM_TYPE_STANDARD;
    } else {
        findReq->ctrl.itemType = FIND_ITEM_TYPE_MIX;
    }
    findReq->ctrl.rspMode = FIND_RSP_MODE_SINGLE_RSP;
    findReq->startHandle = findParam->startHandle;
    findReq->endHandle = findParam->endHandle;
    CP_LOG_INFO("[SSAP] find req find type: %d, item type: %d, rsp mode: %d, start: %d, end: %d",
        findReq->ctrl.findType, findReq->ctrl.itemType, findReq->ctrl.rspMode,
        findReq->startHandle, findReq->endHandle);
    link->sendFunc(link, sdfBuff, SSAP_FIND_STRUCTURE_REQ);
}

void SSAPC_FindReq(SSAP_Link_S *link, void *arg)
{
    CP_LOG_DEBUG("[SSAP] enter find req");
    CP_CHECK_LOG_RETURN_VOID(arg != NULL, "[SSAP] find req arg is null");
    SSAP_ParamFind_S *findParam = (SSAP_ParamFind_S *)arg;
    SendFindPkt(link, findParam);
}

static void SendFindByUuidPkt(SSAP_Link_S *link, SSAP_ParamFindByUuid_S *findParam)
{
    bool isUuidStd = SSAP_CheckUuidStd(&findParam->uuid);
    uint32_t uuidSize = isUuidStd ? SSAP_UUID16_LEN : SSAP_UUID128_LEN;
    uint32_t realSize = sizeof(SSAP_PduFindStructReq_S) + uuidSize;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] find by uuid req sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] find by uuid req create buf fail");
        return;
    }
    SSAP_PduFindStructReq_S *findReq = (SSAP_PduFindStructReq_S *)buf;
    findReq->msgCode = SSAP_FIND_STRUCTURE_BY_UUID_REQ;
    findReq->ctrl.findType = findParam->type;
    if (CM_GetLogicLinkDeviceType(link->lcid) == CM_DEVTYPE_OLD) {
        findReq->ctrl.itemType = FIND_ITEM_TYPE_STANDARD;
    } else {
        findReq->ctrl.itemType = FIND_ITEM_TYPE_MIX;
    }
    findReq->ctrl.rspMode = FIND_RSP_MODE_SINGLE_RSP;
    findReq->startHandle = findParam->startHandle;
    findReq->endHandle = findParam->endHandle;
    SSAP_PutUuidToPktBuf(&findParam->uuid, findReq->uuid, uuidSize);
    CP_LOG_INFO("[SSAP] find by uuid req find type: %d, item type: %d, rsp mode: %d, start: %d, end: %d, uuid: %s",
        findReq->ctrl.findType, findReq->ctrl.itemType, findReq->ctrl.rspMode,
        findReq->startHandle, findReq->endHandle, SSAP_GET_ENC_UUID(&findParam->uuid));
    link->sendFunc(link, sdfBuff, SSAP_FIND_STRUCTURE_BY_UUID_REQ);
}

void SSAPC_FindByUuidReq(SSAP_Link_S *link, void *arg)
{
    CP_LOG_INFO("[SSAP] enter find by uuid req");
    CP_CHECK_LOG_RETURN_VOID(arg != NULL, "[SSAP] find by uuid req arg is null");
    SSAP_ParamFindByUuid_S *findParam = (SSAP_ParamFindByUuid_S *)arg;
    SendFindByUuidPkt(link, findParam);
}

/**
 * @brief  客户端通过句柄和数据类型读取服务端数据
 */
void SSAPC_ReadReq(SSAP_Link_S *link, void *arg)
{
    CP_LOG_DEBUG("[SSAP] enter read req");
    SSAP_ReadReqInfo_S *readReqInfo = (SSAP_ReadReqInfo_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(readReqInfo != NULL, "[SSAP] read req arg is null");
    uint32_t realSize = sizeof(SSAP_PduReadReq_S) + sizeof(Ssap_PduReadReqItem_S);
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] read req sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] read req create buf fail");
        return;
    }
    SSAP_PduReadReq_S *readReq = (SSAP_PduReadReq_S *)buf;
    readReq->msgCode = SSAP_READ_REQ;
    readReq->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    Ssap_PduReadReqItem_S *readReqItem = (Ssap_PduReadReqItem_S *)(readReq->items);
    readReqItem->handle = readReqInfo->handle;
    readReqItem->type = readReqInfo->type;
    CP_LOG_INFO("[SSAP] read req handle: %d, type: %d", readReqItem->handle, readReqItem->type);
    link->sendFunc(link, sdfBuff, SSAP_READ_REQ);
}

/**
 * @brief  客户端通过句柄和数据类型读取多个服务端数据
 */
void SSAPC_ReadProps(SSAP_Link_S *link, void *arg)
{
    CP_LOG_DEBUG("[SSAP] enter read props");
    SSAP_ReadPropsInfo_S *readPropsInfo = (SSAP_ReadPropsInfo_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(readPropsInfo != NULL, "[SSAP] read props arg is null");
    uint32_t realSize = sizeof(SSAP_PduReadReq_S) + readPropsInfo->num * sizeof(Ssap_PduReadReqItem_S);
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] read props sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] read props create buf fail");
        return;
    }
    SSAP_PduReadReq_S *readReq = (SSAP_PduReadReq_S *)buf;
    readReq->msgCode = SSAP_READ_REQ;
    readReq->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    Ssap_PduReadReqItem_S *readReqItems = (Ssap_PduReadReqItem_S *)(readReq->items);
    for (uint8_t i = 0; i < readPropsInfo->num; i++) {
        readReqItems[i].handle = readPropsInfo->handles[i];
        readReqItems[i].type = readPropsInfo->type;
        CP_LOG_INFO("[SSAP] read req handle: %d, type: %d", readReqItems[i].handle, readReqItems[i].type);
    }
    link->sendFunc(link, sdfBuff, SSAP_READ_REQ);
}

/**
 * @brief  客户端通过UUID和数据类型读取服务端数据
 */
void SSAPC_ReadByUuidReq(SSAP_Link_S *link, void *arg)
{
    CP_LOG_INFO("[SSAP] enter read by uuid req");
    SSAP_ReadByUuidReqInfo_S *readByUuidReqInfo = (SSAP_ReadByUuidReqInfo_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(readByUuidReqInfo != NULL, "[SSAP] read by uuid req arg is null");
    bool isUuidStd = SSAP_CheckUuidStd(&readByUuidReqInfo->uuid);
    uint32_t uuidSize = isUuidStd ? SSAP_UUID16_LEN : SSAP_UUID128_LEN;
    uint32_t realSize = sizeof(SSAP_PduReadByUuidReq_S) + uuidSize;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] read by uuid req sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] read by uuid req create buf fail");
        return;
    }
    SSAP_PduReadByUuidReq_S *readByUuidReq = (SSAP_PduReadByUuidReq_S *)buf;
    readByUuidReq->msgCode = SSAP_READ_BY_UUID_REQ;
    if (isUuidStd) {
        readByUuidReq->ctrl.uuidType = SSAP_READ_BY_STANDARD_UUID_CONTROL;
    } else {
        readByUuidReq->ctrl.uuidType = SSAP_READ_BY_CUSTOM_UUID_CONTROL;
    }
    readByUuidReq->handleStart = readByUuidReqInfo->handleStart;
    readByUuidReq->handleEnd = readByUuidReqInfo->handleEnd;
    readByUuidReq->dataType = readByUuidReqInfo->dataType;
    SSAP_PutUuidToPktBuf(&readByUuidReqInfo->uuid, readByUuidReq->uuid, uuidSize);
    CP_LOG_INFO("[SSAP] read by uuid req uuid: %s, start handle: %d, end handle: %d, type: %d",
        SSAP_GET_ENC_UUID(&readByUuidReqInfo->uuid), readByUuidReq->handleStart, readByUuidReq->handleEnd,
        readByUuidReq->dataType);
    link->sendFunc(link, sdfBuff, SSAP_READ_BY_UUID_REQ);
}

/**
 * @brief  发送无需响应的写入报文
 */
void SSAPC_WriteCmd(SSAP_Link_S *link, void *arg)
{
    CP_LOG_DEBUG("[SSAP] enter write cmd");
    SSAP_WriteCmdInfo_S *writeCmdInfo = (SSAP_WriteCmdInfo_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(writeCmdInfo != NULL, "[SSAP] write cmd arg is null");
    uint16_t valueLen = writeCmdInfo->value.len;
    CP_CHECK_LOG_RETURN_VOID(valueLen != 0, "[SSAP] write cmd value is null");
    uint32_t realSize = sizeof(SSAP_PduWriteCmd_S) + sizeof(SSAP_PduWriteCmdItem_S) + valueLen;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] write cmd sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] write cmd create buf fail");
        return;
    }
    SSAP_PduWriteCmd_S *writeCmd = (SSAP_PduWriteCmd_S *)buf;
    writeCmd->msgCode = SSAP_WRITE_CMD;
    writeCmd->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    writeCmd->ctrl.multi = 0;
    writeCmd->ctrl.oper = 0;
    SSAP_PduWriteCmdItem_S *writeCmdItem = (SSAP_PduWriteCmdItem_S *)(writeCmd->items);
    writeCmdItem->handle = writeCmdInfo->handle;
    writeCmdItem->type = writeCmdInfo->type;
    (void)memcpy_s(writeCmdItem->value, valueLen, writeCmdInfo->value.value, valueLen);
    CP_LOG_DEBUG("[SSAP] write cmd item handle: %d, type: %d", writeCmdItem->handle, writeCmdItem->type);
    link->sendFunc(link, sdfBuff, SSAP_WRITE_CMD);
}

/**
 * @brief  客户端有响应写入服务端数据
 */
void SSAPC_WriteReq(SSAP_Link_S *link, void *arg)
{
    CP_LOG_DEBUG("[SSAP] enter write req");
    SSAP_WriteReqInfo_S *writeReqInfo = (SSAP_WriteReqInfo_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(writeReqInfo != NULL, "[SSAP] write req arg is null");
    CP_CHECK_LOG_RETURN_VOID(writeReqInfo->value.len != 0, "[SSAP] write req value is null");
    uint32_t realSize = sizeof(SSAP_PduWriteReq_S) + sizeof(SSAP_PduWriteReqItem_S) + writeReqInfo->value.len;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] write req sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] write req create buf fail");
        return;
    }
    SSAP_PduWriteReq_S *writeReq = (SSAP_PduWriteReq_S *)buf;
    writeReq->msgCode = SSAP_WRITE_REQ;
    writeReq->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    writeReq->ctrl.multi = 0;
    writeReq->ctrl.oper = 0;
    writeReq->ctrl.verify = 1; // client默认带原值返回标记
    SSAP_PduWriteReqItem_S *writeReqItem = (SSAP_PduWriteReqItem_S *)(writeReq->items);
    writeReqItem->handle = writeReqInfo->handle;
    writeReqItem->type = writeReqInfo->type;
    (void)memcpy_s(writeReqItem->value, writeReqInfo->value.len, writeReqInfo->value.value, writeReqInfo->value.len);
    CP_LOG_INFO("[SSAP] write req item handle: %d, type: %d", writeReqItem->handle, writeReqItem->type);
    link->sendFunc(link, sdfBuff, SSAP_WRITE_REQ);
}

/**
 * @brief  数据指示报文确认
 */
void SSAPC_ValueAck(SSAP_Link_S *link, void *arg)
{
    CP_LOG_INFO("[SSAP] enter value ack");
    SSAP_ValueAckInfo_S *valueInfo = (SSAP_ValueAckInfo_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(valueInfo != NULL, "[SSAP] value ack arg param is null");
    CP_CHECK_LOG_RETURN_VOID(valueInfo->value.len != 0, "[SSAP] value ack val is null");
    uint16_t mtu = link->mtu;
    CP_LOG_INFO("[SSAP] value ack value len = %u, mtu = %u", valueInfo->value.len, mtu);
    uint32_t realSize = sizeof(SSAP_PduValueAck_S) + valueInfo->value.len;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] value ack sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] value ack create buf fail");
        return;
    }
    SSAP_PduValueAck_S *valueAck = (SSAP_PduValueAck_S *)buf;
    valueAck->msgCode = SSAP_VALUE_ACK;
    valueAck->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    valueAck->ctrl.type = 0;
    (void)memcpy_s(valueAck->result, valueInfo->value.len, valueInfo->value.value, valueInfo->value.len);
    link->sendFunc(link, sdfBuff, SSAP_VALUE_ACK);
}

/**
 * @brief  请求调用服务端方法(有响应)
 */
void SSAPC_CallMethodReq(SSAP_Link_S *link, void *arg)
{
    SSAP_CallMethodReqInfo_S *callMethodInfo = (SSAP_CallMethodReqInfo_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(callMethodInfo != NULL, "SSAPC_CallMethodReq callMethodInfo is null");
    uint32_t bufSize = sizeof(SSAP_PduCallMethodReq_S) + callMethodInfo->value.len;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(bufSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "SSAPC_CallMethodReq sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, bufSize);
    SSAP_PduCallMethodReq_S *data = (SSAP_PduCallMethodReq_S *)buf;
    if (data == NULL) {
        CP_LOG_ERROR("[SSAP] data nullptr.");
        SDF_BuffFree(sdfBuff);
        return;
    }
    data->msgCode = SSAP_CALL_METHOD_REQ;
    data->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    data->handle = callMethodInfo->handle;
    (void)memcpy_s(data->param, callMethodInfo->value.len, callMethodInfo->value.value, callMethodInfo->value.len);
    link->sendFunc(link, sdfBuff, SSAP_CALL_METHOD_REQ);
}

/**
 * @brief  请求调用服务端方法(无响应)
 */
void SSAPC_CallMethodCmd(SSAP_Link_S *link, void *arg)
{
    SSAP_CallMethodCmdInfo_S *callMethodInfo = (SSAP_CallMethodCmdInfo_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(callMethodInfo != NULL, "SSAPC_CallMethodCmd callMethodInfo is null");
    uint32_t bufSize = sizeof(SSAP_PduCallMethodCmd_S) + callMethodInfo->value.len;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(bufSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "SSAPC_CallMethodCmd sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, bufSize);
    SSAP_PduCallMethodCmd_S *data = (SSAP_PduCallMethodCmd_S *)buf;
    if (data == NULL) {
        CP_LOG_ERROR("[SSAP] data nullptr.");
        SDF_BuffFree(sdfBuff);
        return;
    }
    data->msgCode = SSAP_CALL_METHOD_CMD;
    data->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    data->handle = callMethodInfo->handle;
    (void)memcpy_s(data->param, callMethodInfo->value.len, callMethodInfo->value.value, callMethodInfo->value.len);
    link->sendFunc(link, sdfBuff, SSAP_CALL_METHOD_CMD);

    // 按writeCmd的逻辑这里要有一个回调，目前未实现
}

#ifdef __cplusplus
}
#endif