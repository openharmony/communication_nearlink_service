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

#include "cm_logic_link_api.h"

#include "cpfwk_log.h"
#include "nlstk_log.h"
#include "ssap_type.h"
#include "ssap_pkt.h"
#include "ssap_utils.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssap_common.h"
#include "ssaps_service.h"
#include "ssaps_server.h"

#ifdef __cplusplus
extern "C" {
#endif

static SSAP_UuidType_E GetUuidType(NLSTK_SsapUuid_S *uuid)
{
    SDF_Vector_S *services = SSAPS_GetServices();
    CP_CHECK_LOG_RETURN(services != NULL, UUID_TYPE_UNKNOWN, "[SSAP] cannot get services");
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        CP_CHECK_LOG_RETURN(service->properties != NULL, UUID_TYPE_UNKNOWN, "[SSAP] cannot get property");
        if (SSAP_IsUuidEqual(&service->uuid, uuid)) {
            return UUID_TYPE_SERVICE;
        }
        for (size_t j = 0; j < service->properties->size; j++) {
            SSAP_Property_S *property = SDF_VectorElementAt(service->properties, j);
            if (SSAP_IsUuidEqual(&property->uuid, uuid)) {
                return UUID_TYPE_PROPERTY;
            }
        }
        for (size_t j = 0; j < service->methods->size; j++) {
            SSAP_Method_S *method = SDF_VectorElementAt(service->methods, j);
            if (SSAP_IsUuidEqual(&method->uuid, uuid)) {
                return UUID_TYPE_METHOD;
            }
        }
    }
    return UUID_TYPE_UNKNOWN;
}

static bool CheckFindItemType(NLSTK_SsapUuid_S *uuid, SSAP_FindItemType_E findItemType)
{
    if (findItemType == FIND_ITEM_TYPE_MIX) {
        return true;
    }
    bool isUuidStd = SSAP_CheckUuidStd(uuid);
    if (isUuidStd && findItemType == FIND_ITEM_TYPE_STANDARD) {
        return true;
    }
    if (!isUuidStd && findItemType == FIND_ITEM_TYPE_CUSTOMIZE) {
        return true;
    }
    return false;
}

static void CopyToServiceInfo(SSAP_Service_S *service, SSAP_FindServiceInfo_S *info)
{
    info->startHandle = service->handle;
    info->endHandle = service->endHandle;
    (void)memcpy_s(&info->uuid, sizeof(NLSTK_SsapUuid_S), &service->uuid, sizeof(NLSTK_SsapUuid_S));
    info->isStdUuid = SSAP_CheckUuidStd(&info->uuid);
    if (service->references->size != 0) {
        info->member.reference = 1;
    }
    if (service->properties->size != 0) {
        info->member.property = 1;
    }
    if (service->methods->size != 0) {
        info->member.method = 1;
    }
    if (service->events->size != 0) {
        info->member.event = 1;
    }
}

static void InfoEmplaceBackAndCheckFree(SDF_Vector_S *vector, void *ptr)
{
    if (!SDF_VectorEmplaceBack(vector, ptr)) {
        SDF_MemFree(ptr);   // 调用者需保证ptr非空
        CP_LOG_ERROR("[SSAP] InfoEmplaceBackAndCheckFree emplace back vector failed");
    }
    return;
}

static void FindPrimaryServiceList(SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *uuid, SDF_Vector_S *findServices,
    bool isByUuid)
{
    SDF_Vector_S *services = SSAPS_GetServices();
    CP_CHECK_LOG_RETURN_VOID(services != NULL, "[SSAP] cannot get services");
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        if ((!isByUuid || SSAP_IsUuidEqual(&service->uuid, uuid)) && service->handle >= req->startHandle &&
            service->handle <= req->endHandle && CheckFindItemType(&service->uuid, req->ctrl.itemType)) {
            SSAP_FindServiceInfo_S *info = (SSAP_FindServiceInfo_S *)SDF_MemZalloc(sizeof(SSAP_FindServiceInfo_S));
            CP_CHECK_LOG_RETURN_VOID(info != NULL, "[SSAP] FindPrimaryServiceList malloc failed");
            CopyToServiceInfo(service, info);
            InfoEmplaceBackAndCheckFree(findServices, info);    // 失败会在内部释放info
        }
    }
}

static void InfoEmplaceBackAndCheckFreeV10(SDF_Vector_S *vector, void *ptr, bool *hasCus, bool isStdUuid)
{
    if (!SDF_VectorEmplaceBack(vector, ptr)) {
        SDF_MemFree(ptr);   // 调用者需保证ptr非空
        CP_LOG_ERROR("[SSAP] InfoEmplaceBackAndCheckFreeV10 emplace back vector failed");
    } else {
        // 成功后更新是否有自定义服务标记，因为老版本协议栈不支持自定义类型，只支持标准和混合
        *hasCus = *hasCus || !isStdUuid;
    }
    return;
}

static void FindPrimaryServiceListV10(SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *uuid, SDF_Vector_S *findServices,
    bool isByUuid, bool *hasCus)
{
    *hasCus = false; // 标记是否有自定义服务
    SDF_Vector_S *services = SSAPS_GetServices();
    CP_CHECK_LOG_RETURN_VOID(services != NULL, "[SSAP] cannot get services");
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        if ((!isByUuid || SSAP_IsUuidEqual(&service->uuid, uuid)) && service->handle >= req->startHandle &&
            service->handle <= req->endHandle) {
            SSAP_FindServiceInfo_S *info = (SSAP_FindServiceInfo_S *)SDF_MemZalloc(sizeof(SSAP_FindServiceInfo_S));
            CP_CHECK_LOG_RETURN_VOID(info != NULL, "[SSAP] find primary service malloc failed");
            CopyToServiceInfo(service, info);
            // emplace失败会在内部释放info，成功会检查并更新hasCus标志
            InfoEmplaceBackAndCheckFreeV10(findServices, info, hasCus, info->isStdUuid);
        }
    }
}

static void BuildPrimaryServiceInfo(uint8_t *buf, SSAP_FindServiceInfo_S *info)
{
    uint8_t *curBuf = buf;
    SSAP_UINT16_TO_BYTE_LITTLE(curBuf,  info->startHandle);
    curBuf += SSAP_HANDLE_LEN;
    SSAP_UINT16_TO_BYTE_LITTLE(curBuf,  info->endHandle);
    curBuf += SSAP_HANDLE_LEN;
    uint32_t uuidSize = info->isStdUuid ? SSAP_UUID16_LEN : SSAP_UUID128_LEN;
    SSAP_PutUuidToPktBuf(&info->uuid, curBuf, uuidSize);
    curBuf += uuidSize;
    *curBuf = info->memberValue;
}

static void BuildPrimaryServiceInfoV10(uint8_t *buf, SSAP_FindServiceInfo_S *info)
{
    uint8_t *curBuf = buf;
    SSAP_UINT16_TO_BYTE_LITTLE(curBuf,  info->startHandle);
    curBuf += SSAP_HANDLE_LEN;
    SSAP_UINT16_TO_BYTE_LITTLE(curBuf,  info->endHandle);
    curBuf += SSAP_HANDLE_LEN;
    *curBuf = info->memberValue;
}

static SDF_Buff_S* BuildPrimaryServicePayload(uint32_t mtu, SDF_Vector_S *findServices, uint32_t serviceLen)
{
    CP_CHECK_LOG_RETURN(mtu >= SSAP_PDU_BASE_LEN, NULL, "[SSAP] mtu error");
    uint32_t leftSize = mtu - SSAP_PDU_BASE_LEN;
    uint32_t maxCount = leftSize / serviceLen;
    CP_CHECK_LOG_RETURN(findServices->size <= UINT32_MAX, NULL, "[SSAP] findServices size overflow");
    uint32_t count = maxCount < (uint32_t)findServices->size ? maxCount : (uint32_t)findServices->size;
    CP_CHECK_LOG_RETURN((uint64_t)SSAP_PDU_BASE_LEN + serviceLen * count <= UINT32_MAX, NULL,
        "[SSAP] real size overflow");
    uint32_t realSize = SSAP_PDU_BASE_LEN + serviceLen * count;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN(sdfBuff != NULL, NULL, "[SSAP] find rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] find rsp create buf fail");
        return NULL;
    }
    buf += SSAP_PDU_BASE_LEN;
    for (size_t i = 0; i < count; i++) {
        SSAP_FindServiceInfo_S *info = SDF_VectorElementAt(findServices, i);
        BuildPrimaryServiceInfo(buf, info);
        buf += serviceLen;
    }
    return sdfBuff;
}

static SDF_Buff_S* BuildPrimaryServicePayloadV10(uint32_t mtu, SDF_Vector_S *findServices, uint32_t serviceLen)
{
    CP_CHECK_LOG_RETURN(mtu >= SSAP_PDU_BASE_LEN, NULL, "[SSAP] mtu error");
    uint32_t leftSize = mtu - SSAP_PDU_BASE_LEN;
    uint32_t maxCount = leftSize / serviceLen;
    CP_CHECK_LOG_RETURN(findServices->size <= UINT32_MAX, NULL, "[SSAP] findServices size overflow");
    uint32_t count = maxCount < (uint32_t)findServices->size ? maxCount : (uint32_t)findServices->size;
    uint32_t realSize = SSAP_PDU_BASE_LEN + serviceLen * count;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN(sdfBuff != NULL, NULL, "[SSAP] find rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] find rsp create buf fail");
        return NULL;
    }
    buf += SSAP_PDU_BASE_LEN;
    for (size_t i = 0; i < count; i++) {
        SSAP_FindServiceInfo_S *info = SDF_VectorElementAt(findServices, i);
        BuildPrimaryServiceInfoV10(buf, info);
        buf += serviceLen;
    }
    return sdfBuff;
}

static SDF_Buff_S* FillMixPrimaryServicePayload(SDF_Vector_S *findServices, uint32_t realSize, uint32_t totalStdCount,
    uint32_t totalCusCount)
{
    if (totalStdCount > UINT8_MAX || totalCusCount > UINT8_MAX) {
        CP_LOG_ERROR("[SSAP] find rsp stdCount or cusCount overflow");
        return NULL;
    }
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN(sdfBuff != NULL, NULL, "[SSAP] find rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] find rsp create buf fail");
        return NULL;
    }
    buf += SSAP_PDU_BASE_LEN;
    SSAP_FindInfoIndicator_S *stdInd = (SSAP_FindInfoIndicator_S *)buf;
    stdInd->count = (uint8_t)totalStdCount;
    stdInd->type = SSAP_FIND_INFO_INDICATION_STD;
    buf += SSAP_FIND_INFO_INDICATION_LEN;
    for (size_t i = 0, count = totalStdCount; i < findServices->size && count > 0; i++) {
        SSAP_FindServiceInfo_S *info = SDF_VectorElementAt(findServices, i);
        if (info->isStdUuid) {
            BuildPrimaryServiceInfo(buf, info);
            buf += SSAP_FIND_PRIMARY_SERVICE_STD_LEN;
            count--;
        }
    }
    SSAP_FindInfoIndicator_S *cusInd = (SSAP_FindInfoIndicator_S *)buf;
    cusInd->count = (uint8_t)totalCusCount;
    cusInd->type = SSAP_FIND_INFO_INDICATION_CUS;
    buf += SSAP_FIND_INFO_INDICATION_LEN;
    for (size_t i = 0, count = totalCusCount; i < findServices->size && count > 0; i++) {
        SSAP_FindServiceInfo_S *info = SDF_VectorElementAt(findServices, i);
        if (!info->isStdUuid) {
            BuildPrimaryServiceInfo(buf, info);
            buf += SSAP_FIND_PRIMARY_SERVICE_CUS_LEN;
            count--;
        }
    }
    return sdfBuff;
}

static SDF_Buff_S* FillMixPrimaryServicePayloadV10(SDF_Vector_S *findServices, uint32_t realSize,
    uint32_t totalStdCount, uint32_t totalCusCount)
{
    if (totalStdCount > UINT8_MAX || totalCusCount > UINT8_MAX) {
        CP_LOG_ERROR("[SSAP] find rsp stdCount or cusCount overflow");
        return NULL;
    }
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN(sdfBuff != NULL, NULL, "[SSAP] find rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] find rsp create buf fail");
        return NULL;
    }
    buf += SSAP_PDU_BASE_LEN;
    SSAP_FindInfoIndicator_S *stdInd = (SSAP_FindInfoIndicator_S *)buf;
    stdInd->count = (uint8_t)totalStdCount;
    stdInd->type = SSAP_FIND_INFO_INDICATION_STD;
    buf += SSAP_FIND_INFO_INDICATION_LEN;
    for (size_t i = 0, count = totalStdCount; i < findServices->size && count > 0; i++) {
        SSAP_FindServiceInfo_S *info = SDF_VectorElementAt(findServices, i);
        if (info->isStdUuid) {
            BuildPrimaryServiceInfoV10(buf, info);
            buf += SSAP_FIND_PRIMARY_SERVICE_BASE_LEN;
            count--;
        }
    }
    SSAP_FindInfoIndicator_S *cusInd = (SSAP_FindInfoIndicator_S *)buf;
    cusInd->count = (uint8_t)totalCusCount;
    cusInd->type = SSAP_FIND_INFO_INDICATION_CUS;
    buf += SSAP_FIND_INFO_INDICATION_LEN;
    for (size_t i = 0, count = totalCusCount; i < findServices->size && count > 0; i++) {
        SSAP_FindServiceInfo_S *info = SDF_VectorElementAt(findServices, i);
        if (!info->isStdUuid) {
            BuildPrimaryServiceInfoV10(buf, info);
            buf += SSAP_FIND_PRIMARY_SERVICE_BASE_LEN;
            count--;
        }
    }
    return sdfBuff;
}

static SDF_Buff_S* BuildMixPrimaryServicePayload(uint32_t mtu, SDF_Vector_S *findServices)
{
    uint32_t stdCount = 0;
    uint32_t cusCount = 0;
    NLSTK_CHECK_RETURN(mtu >= SSAP_PDU_BASE_LEN + SSAP_FIND_INFO_INDICATION_LEN * 2, NULL, "[SSAP] mtu is invalid");
    uint32_t leftSize = mtu - SSAP_PDU_BASE_LEN - SSAP_FIND_INFO_INDICATION_LEN * 2;
    for (size_t i = 0; i < findServices->size; i++) {
        SSAP_FindServiceInfo_S *info = SDF_VectorElementAt(findServices, i);
        uint32_t uuidSize = info->isStdUuid ? SSAP_UUID16_LEN : SSAP_UUID128_LEN;
        uint32_t needSize = SSAP_FIND_PRIMARY_SERVICE_BASE_LEN + uuidSize;
        if (leftSize < needSize) {
            break;
        }
        leftSize -= needSize;
        if (info->isStdUuid) {
            stdCount++;
        } else {
            cusCount++;
        }
    }
    uint32_t realSize = mtu - leftSize;
    return FillMixPrimaryServicePayload(findServices, realSize, stdCount, cusCount);
}

static SDF_Buff_S* BuildMixPrimaryServicePayloadV10(uint32_t mtu, SDF_Vector_S *findServices)
{
    uint32_t stdCount = 0;
    uint32_t cusCount = 0;
    uint32_t leftSize = mtu - SSAP_PDU_BASE_LEN - SSAP_FIND_INFO_INDICATION_LEN * 2;
    for (size_t i = 0; i < findServices->size; i++) {
        SSAP_FindServiceInfo_S *info = SDF_VectorElementAt(findServices, i);
        uint32_t needSize = SSAP_FIND_PRIMARY_SERVICE_BASE_LEN;
        if (leftSize < needSize) {
            break;
        }
        leftSize -= needSize;
        if (info->isStdUuid) {
            stdCount++;
        } else {
            cusCount++;
        }
    }
    uint32_t realSize = mtu - leftSize;
    return FillMixPrimaryServicePayloadV10(findServices, realSize, stdCount, cusCount);
}

static void SendFindRspPkt(SSAP_Link_S *link, SSAP_PduFindStructReq_S *req, SDF_Buff_S *sdfBuff, uint8_t itemType)
{
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] find rsp buff is null");
    SSAP_PduFindStructRsp_S *rsp = (SSAP_PduFindStructRsp_S *)SDF_DataOffset(sdfBuff);
    if (req->msgCode == SSAP_FIND_STRUCTURE_REQ) {
        rsp->msgCode = SSAP_FIND_STRUCTURE_RSP;
    } else {
        rsp->msgCode = SSAP_FIND_STRUCTURE_BY_UUID_RSP;
    }
    rsp->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    rsp->ctrl.itemType = itemType;
    link->sendFunc(link, sdfBuff, rsp->msgCode);
}

static void SendFindPrimaryServiceRsp(SSAP_Link_S *link, SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *uuid,
    bool isByUuid)
{
    int sceneCode = req->msgCode == SSAP_FIND_STRUCTURE_REQ ?
            EXCEP_SSAP_FIND_STRUCTURE_REQ_RECV : EXCEP_SSAP_FIND_BY_UUID_REQ_RECV;
    if (isByUuid && GetUuidType(uuid) != UUID_TYPE_SERVICE) {
        CP_LOG_ERROR("[SSAP] find uuid wrong type");
        SSAP_PduErrorRsp(link, req->msgCode, req->startHandle, SSAP_ERRCODE_ITEM_INEXIST);
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_ITEM_INEXIST);
        return;
    }
    uint32_t mtu = link->mtu;
    SDF_Traits findServiceTraits = {.dtor = SDF_MemFree};
    SDF_Vector_S *findServices = SDF_CreateVector(findServiceTraits);
    if (findServices == NULL) {
        CP_LOG_ERROR("[SSAP] find uuid memory failed");
        SSAP_PduErrorRsp(link, req->msgCode, 0, SSAP_ERRCODE_NO_RESOURCE);
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_NO_RESOURCE);
        return;
    }
    FindPrimaryServiceList(req, uuid, findServices, isByUuid);
    if (findServices->size == 0) {
        SSAP_PduErrorRsp(link, req->msgCode, req->startHandle, SSAP_ERRCODE_ITEM_INEXIST);
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_ITEM_INEXIST);
        SDF_DestroyVector(findServices);
        return;
    }
    SDF_Buff_S *sdfBuff = NULL;
    if (req->ctrl.itemType == FIND_ITEM_TYPE_STANDARD) {
        sdfBuff = BuildPrimaryServicePayload(mtu, findServices, SSAP_FIND_PRIMARY_SERVICE_STD_LEN);
    } else if (req->ctrl.itemType == FIND_ITEM_TYPE_CUSTOMIZE) {
        sdfBuff = BuildPrimaryServicePayload(mtu, findServices, SSAP_FIND_PRIMARY_SERVICE_CUS_LEN);
    } else if (req->ctrl.itemType == FIND_ITEM_TYPE_MIX) {
        sdfBuff = BuildMixPrimaryServicePayload(mtu, findServices);
    }
    uint8_t itemType = req->ctrl.itemType;
    SendFindRspPkt(link, req, sdfBuff, itemType);
    SDF_DestroyVector(findServices);
}

static void SendFindPrimaryServiceRspV10(SSAP_Link_S *link, SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *uuid,
    bool isByUuid)
{
    int sceneCode = req->msgCode == SSAP_FIND_STRUCTURE_REQ ?
            EXCEP_SSAP_FIND_STRUCTURE_REQ_RECV : EXCEP_SSAP_FIND_BY_UUID_REQ_RECV;
    if (isByUuid && GetUuidType(uuid) != UUID_TYPE_SERVICE) {
        CP_LOG_ERROR("[SSAP] find uuid wrong type");
        SSAP_PduErrorRsp(link, req->msgCode, req->startHandle, SSAP_ERRCODE_ITEM_INEXIST);
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_ITEM_INEXIST);
        return;
    }
    uint32_t mtu = link->mtu;
    SDF_Traits findServiceTraits = {.dtor = SDF_MemFree};
    SDF_Vector_S *findServices = SDF_CreateVector(findServiceTraits);
    if (findServices == NULL) {
        CP_LOG_ERROR("[SSAP] find uuid memory failed");
        SSAP_PduErrorRsp(link, req->msgCode, 0, SSAP_ERRCODE_NO_RESOURCE);
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_NO_RESOURCE);
        return;
    }
    bool hasCus = false;
    FindPrimaryServiceListV10(req, uuid, findServices, isByUuid, &hasCus);
    if (findServices->size == 0) {
        SSAP_PduErrorRsp(link, req->msgCode, req->startHandle, SSAP_ERRCODE_ITEM_INEXIST);
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_ITEM_INEXIST);
        SDF_DestroyVector(findServices);
        return;
    }
    SDF_Buff_S *sdfBuff = NULL;
    uint8_t itemType = 0;
    if (hasCus) {
        itemType = FIND_ITEM_TYPE_MIX;
        if (req->msgCode == SSAP_FIND_STRUCTURE_REQ) {
            sdfBuff = BuildMixPrimaryServicePayload(mtu, findServices);
        } else {
            sdfBuff = BuildMixPrimaryServicePayloadV10(mtu, findServices);
        }
    } else {
        itemType = FIND_ITEM_TYPE_STANDARD;
        if (req->msgCode == SSAP_FIND_STRUCTURE_REQ) {
            sdfBuff = BuildPrimaryServicePayload(mtu, findServices, SSAP_FIND_PRIMARY_SERVICE_STD_LEN);
        } else {
            sdfBuff = BuildPrimaryServicePayloadV10(mtu, findServices, SSAP_FIND_PRIMARY_SERVICE_BASE_LEN);
        }
    }
    SendFindRspPkt(link, req, sdfBuff, itemType);
    SDF_DestroyVector(findServices);
}

static void FreeFindPropertyInfo(void *ptr)
{
    SSAP_FindPropertyInfo_S *info = (SSAP_FindPropertyInfo_S *)ptr;
    if (info == NULL) {
        return;
    }
    if (info->descriptors != NULL) {
        SDF_MemFree(info->descriptors);
    }
    SDF_MemFree(info);
}

static void CopyToPropertyInfo(SSAP_Property_S *property, SSAP_FindPropertyInfo_S *info)
{
    info->handle = property->handle;
    (void)memcpy_s(&info->uuid, sizeof(NLSTK_SsapUuid_S), &property->uuid, sizeof(NLSTK_SsapUuid_S));
    info->isStdUuid = SSAP_CheckUuidStd(&info->uuid);
    info->operation.operationValue = property->operation.operationValue;
    CP_CHECK_LOG_RETURN_VOID(property->descriptors->size <= UINT8_MAX, "[SSAP] property descriptors size overflow");
    info->descriptorCount = (uint8_t)property->descriptors->size;
    if (property->descriptors->size != 0) {
        info->descriptors = SDF_MemZalloc(property->descriptors->size);
        if (info->descriptors == NULL) {
            CP_LOG_ERROR("[SSAP] copy to property memory fail");
            info->descriptorCount = 0;
            return;
        }
    }
    for (size_t i = 0; i < property->descriptors->size; i++) {
        SSAP_Descriptor_S *descriptor = SDF_VectorElementAt(property->descriptors, i);
        info->descriptors[i] = descriptor->type;
    }
}

static void FindPropertyListByService(SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *uuid, SDF_Vector_S *findProperty)
{
    SDF_Vector_S *services = SSAPS_GetServices();
    CP_CHECK_LOG_RETURN_VOID(services != NULL, "[SSAP] cannot get services");
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        CP_CHECK_LOG_RETURN_VOID(service->properties != NULL, "[SSAP] cannot get service properties.");
        if (!SSAP_IsUuidEqual(&service->uuid, uuid)) {
            continue;
        }
        for (size_t j = 0; j < service->properties->size; j++) {
            SSAP_Property_S *property = SDF_VectorElementAt(service->properties, j);
            CP_CHECK_LOG_RETURN_VOID(property->descriptors != NULL, "[SSAP] find property descriptors fail");
            if (property->handle >= req->startHandle && property->handle <= req->endHandle &&
                CheckFindItemType(&property->uuid, req->ctrl.itemType)) {
                SSAP_FindPropertyInfo_S *info =
                    (SSAP_FindPropertyInfo_S *)SDF_MemZalloc(sizeof(SSAP_FindPropertyInfo_S));
                CP_CHECK_LOG_RETURN_VOID(info != NULL, "[SSAP] find property malloc failed");
                CopyToPropertyInfo(property, info);
                InfoEmplaceBackAndCheckFree(findProperty, info);    // emplace失败会在内部释放info
            }
        }
        return;
    }
}

static void FindPropertyList(SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *uuid, SDF_Vector_S *findProperty,
    bool isByUuid)
{
    SDF_Vector_S *services = SSAPS_GetServices();
    CP_CHECK_LOG_RETURN_VOID(services != NULL, "[SSAP] cannot get services");
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        CP_CHECK_LOG_RETURN_VOID(service->properties != NULL, "[SSAP] cannot get service properties.");
        for (size_t j = 0; j < service->properties->size; j++) {
            SSAP_Property_S *property = SDF_VectorElementAt(service->properties, j);
            CP_CHECK_LOG_RETURN_VOID(property->descriptors != NULL, "[SSAP] cannot get property descriptors.");
            if ((!isByUuid || SSAP_IsUuidEqual(&property->uuid, uuid)) && property->handle >= req->startHandle &&
                property->handle <= req->endHandle && CheckFindItemType(&property->uuid, req->ctrl.itemType)) {
                SSAP_FindPropertyInfo_S *info =
                    (SSAP_FindPropertyInfo_S *)SDF_MemZalloc(sizeof(SSAP_FindPropertyInfo_S));
                CP_CHECK_LOG_RETURN_VOID(info != NULL, "[SSAP] find property malloc failed");
                CopyToPropertyInfo(property, info);
                InfoEmplaceBackAndCheckFree(findProperty, info);    // emplace失败会在内部释放info
            }
        }
    }
}

static void FindPropertyListV10(SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *uuid, SDF_Vector_S *findProperty,
    bool isByUuid, bool *hasCus)
{
    *hasCus = false;
    SDF_Vector_S *services = SSAPS_GetServices();
    CP_CHECK_LOG_RETURN_VOID(services != NULL, "[SSAP] cannot get services");
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        CP_CHECK_LOG_RETURN_VOID(service->properties != NULL, "[SSAP] cannot get service properties.");
        for (size_t j = 0; j < service->properties->size; j++) {
            SSAP_Property_S *property = SDF_VectorElementAt(service->properties, j);
            CP_CHECK_LOG_RETURN_VOID(property->descriptors != NULL, "[SSAP] cannot get property descriptors.");
            if ((!isByUuid || SSAP_IsUuidEqual(&property->uuid, uuid)) && property->handle >= req->startHandle &&
                property->handle <= req->endHandle) {
                SSAP_FindPropertyInfo_S *info =
                    (SSAP_FindPropertyInfo_S *)SDF_MemZalloc(sizeof(SSAP_FindPropertyInfo_S));
                CP_CHECK_LOG_RETURN_VOID(info != NULL, "[SSAP] find property malloc failed");
                CopyToPropertyInfo(property, info);
                // emplace失败会在内部释放info，成功会检查并更新hasCus标志
                InfoEmplaceBackAndCheckFreeV10(findProperty, info, hasCus, info->isStdUuid);
            }
        }
    }
}

static void BuildPropertyInfo(uint8_t *buf, SSAP_FindPropertyInfo_S *info)
{
    uint8_t *curBuf = buf;
    SSAP_UINT16_TO_BYTE_LITTLE(curBuf,  info->handle);
    curBuf += SSAP_HANDLE_LEN;
    uint32_t uuidSize = info->isStdUuid ? SSAP_UUID16_LEN : SSAP_UUID128_LEN;
    SSAP_PutUuidToPktBuf(&info->uuid, curBuf, uuidSize);
    curBuf += uuidSize;
    SSAP_UINT32_TO_BYTE_LITTLE(curBuf, info->operation.operationValue);
    curBuf += SSAP_FIND_OPERATION_LEN;
    *curBuf = info->descriptorCount;
    curBuf += SSAP_FIND_DESCRIPTOR_COUNT_LEN;
    for (int i = 0; i < info->descriptorCount; i++) {
        curBuf[i] = info->descriptors[i];
    }
}

static void BuildPropertyInfoV10(uint8_t *buf, SSAP_FindPropertyInfo_S *info)
{
    uint8_t *curBuf = buf;
    SSAP_UINT16_TO_BYTE_LITTLE(curBuf,  info->handle);
    curBuf += SSAP_HANDLE_LEN;
    SSAP_UINT32_TO_BYTE_LITTLE(curBuf, info->operation.operationValue);
    curBuf += SSAP_FIND_OPERATION_LEN;
    *curBuf = info->descriptorCount;
    curBuf += SSAP_FIND_DESCRIPTOR_COUNT_LEN;
    for (int i = 0; i < info->descriptorCount; i++) {
        curBuf[i] = info->descriptors[i];
    }
}

static SDF_Buff_S *BuildPropertyPayload(uint32_t mtu, SDF_Vector_S *findProperty, uint32_t basePropertyLen)
{
    CP_CHECK_LOG_RETURN(mtu >= SSAP_PDU_BASE_LEN, NULL, "[SSAP] mtu error");
    size_t count = 0;
    uint32_t leftSize = mtu - SSAP_PDU_BASE_LEN;
    for (size_t i = 0; i < findProperty->size; i++) {
        SSAP_FindPropertyInfo_S *info = SDF_VectorElementAt(findProperty, i);
        uint32_t infoNeedSize = basePropertyLen + info->descriptorCount;
        if (leftSize >= infoNeedSize) {
            count++;
            leftSize -= infoNeedSize;
        } else {
            break;
        }
    }
    uint32_t realSize = mtu - leftSize;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN(sdfBuff != NULL, NULL, "[SSAP] find rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] find rsp create buf fail");
        return NULL;
    }
    buf += SSAP_PDU_BASE_LEN;
    for (size_t i = 0; i < count; i++) {
        SSAP_FindPropertyInfo_S *info = SDF_VectorElementAt(findProperty, i);
        uint32_t infoNeedSize = basePropertyLen + info->descriptorCount;
        BuildPropertyInfo(buf, info);
        buf += infoNeedSize;
    }
    return sdfBuff;
}

static SDF_Buff_S *BuildPropertyPayloadV10(uint32_t mtu, SDF_Vector_S *findProperty, uint32_t basePropertyLen)
{
    CP_CHECK_LOG_RETURN(mtu >= SSAP_PDU_BASE_LEN, NULL, "[SSAP] mtu error");
    size_t count = 0;
    uint32_t leftSize = mtu - SSAP_PDU_BASE_LEN;
    for (size_t i = 0; i < findProperty->size; i++) {
        SSAP_FindPropertyInfo_S *info = SDF_VectorElementAt(findProperty, i);
        uint32_t infoNeedSize = basePropertyLen + info->descriptorCount;
        if (leftSize >= infoNeedSize) {
            count++;
            leftSize -= infoNeedSize;
        } else {
            break;
        }
    }
    uint32_t realSize = mtu - leftSize;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN(sdfBuff != NULL, NULL, "[SSAP] find rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] find rsp create buf fail");
        return NULL;
    }
    buf += SSAP_PDU_BASE_LEN;
    for (size_t i = 0; i < count; i++) {
        SSAP_FindPropertyInfo_S *info = SDF_VectorElementAt(findProperty, i);
        uint32_t infoNeedSize = basePropertyLen + info->descriptorCount;
        BuildPropertyInfoV10(buf, info);
        buf += infoNeedSize;
    }
    return sdfBuff;
}

static SDF_Buff_S *FillMixPropertyPayload(SDF_Vector_S *findProperty, uint32_t realSize, uint32_t totalStdCount,
    uint32_t totalCusCount)
{
    if (totalStdCount > UINT8_MAX || totalCusCount > UINT8_MAX) {
        CP_LOG_ERROR("[SSAP] find rsp stdCount or cusCount overflow");
        return NULL;
    }
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN(sdfBuff != NULL, NULL, "[SSAP] find rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] find rsp create buf fail");
        return NULL;
    }
    buf += SSAP_PDU_BASE_LEN;
    SSAP_FindInfoIndicator_S *stdInd = (SSAP_FindInfoIndicator_S *)buf;
    stdInd->count = (uint8_t)totalStdCount;
    stdInd->type = SSAP_FIND_INFO_INDICATION_STD;
    buf += SSAP_FIND_INFO_INDICATION_LEN;
    for (size_t i = 0, count = totalStdCount; i < findProperty->size && count > 0; i++) {
        SSAP_FindPropertyInfo_S *info = SDF_VectorElementAt(findProperty, i);
        if (info->isStdUuid) {
            BuildPropertyInfo(buf, info);
            buf += (SSAP_FIND_PROPERTY_STD_LEN + info->descriptorCount);
            count--;
        }
    }
    SSAP_FindInfoIndicator_S *cusInd = (SSAP_FindInfoIndicator_S *)buf;
    cusInd->count = (uint8_t)totalCusCount;
    cusInd->type = SSAP_FIND_INFO_INDICATION_CUS;
    buf += SSAP_FIND_INFO_INDICATION_LEN;
    for (size_t i = 0, count = totalCusCount; i < findProperty->size && count > 0; i++) {
        SSAP_FindPropertyInfo_S *info = SDF_VectorElementAt(findProperty, i);
        if (!info->isStdUuid) {
            BuildPropertyInfo(buf, info);
            buf += (SSAP_FIND_PROPERTY_CUS_LEN + info->descriptorCount);
            count--;
        }
    }
    return sdfBuff;
}

static SDF_Buff_S *FillMixPropertyPayloadV10(SDF_Vector_S *findProperty, uint32_t realSize, uint32_t totalStdCount,
    uint32_t totalCusCount)
{
    if (totalStdCount > UINT8_MAX || totalCusCount > UINT8_MAX) {
        CP_LOG_ERROR("[SSAP] find rsp stdCount or cusCount overflow");
        return NULL;
    }
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN(sdfBuff != NULL, NULL, "[SSAP] find rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] find rsp create buf fail");
        return NULL;
    }
    buf += SSAP_PDU_BASE_LEN;
    SSAP_FindInfoIndicator_S *stdInd = (SSAP_FindInfoIndicator_S *)buf;
    stdInd->count = (uint8_t)totalStdCount;
    stdInd->type = SSAP_FIND_INFO_INDICATION_STD;
    buf += SSAP_FIND_INFO_INDICATION_LEN;
    for (size_t i = 0, count = totalStdCount; i < findProperty->size && count > 0; i++) {
        SSAP_FindPropertyInfo_S *info = SDF_VectorElementAt(findProperty, i);
        if (info->isStdUuid) {
            BuildPropertyInfoV10(buf, info);
            buf += (SSAP_FIND_PROPERTY_BASE_LEN + info->descriptorCount);
            count--;
        }
    }
    SSAP_FindInfoIndicator_S *cusInd = (SSAP_FindInfoIndicator_S *)buf;
    cusInd->count = (uint8_t)totalCusCount;
    cusInd->type = SSAP_FIND_INFO_INDICATION_CUS;
    buf += SSAP_FIND_INFO_INDICATION_LEN;
    for (size_t i = 0, count = totalCusCount; i < findProperty->size && count > 0; i++) {
        SSAP_FindPropertyInfo_S *info = SDF_VectorElementAt(findProperty, i);
        if (!info->isStdUuid) {
            BuildPropertyInfoV10(buf, info);
            buf += (SSAP_FIND_PROPERTY_BASE_LEN + info->descriptorCount);
            count--;
        }
    }
    return sdfBuff;
}

static SDF_Buff_S *BuildMixPropertyPayload(uint32_t mtu, SDF_Vector_S *findProperty)
{
    uint32_t stdCount = 0;
    uint32_t cusCount = 0;
    uint32_t leftSize = mtu - SSAP_PDU_BASE_LEN - SSAP_FIND_INFO_INDICATION_LEN * 2;
    for (size_t i = 0; i < findProperty->size; i++) {
        SSAP_FindPropertyInfo_S *info = SDF_VectorElementAt(findProperty, i);
        uint32_t uuidSize = info->isStdUuid ? SSAP_UUID16_LEN : SSAP_UUID128_LEN;
        uint32_t needSize = SSAP_FIND_PROPERTY_BASE_LEN + uuidSize + info->descriptorCount;
        if (leftSize < needSize) {
            break;
        }
        leftSize -= needSize;
        if (info->isStdUuid) {
            stdCount++;
        } else {
            cusCount++;
        }
    }
    uint32_t realSize = mtu - leftSize;
    return FillMixPropertyPayload(findProperty, realSize, stdCount, cusCount);
}

static SDF_Buff_S *BuildMixPropertyPayloadV10(uint32_t mtu, SDF_Vector_S *findProperty)
{
    uint32_t stdCount = 0;
    uint32_t cusCount = 0;
    uint32_t leftSize = mtu - SSAP_PDU_BASE_LEN - SSAP_FIND_INFO_INDICATION_LEN * 2;
    for (size_t i = 0; i < findProperty->size; i++) {
        SSAP_FindPropertyInfo_S *info = SDF_VectorElementAt(findProperty, i);
        uint32_t needSize = SSAP_FIND_PROPERTY_BASE_LEN + info->descriptorCount;
        if (leftSize < needSize) {
            break;
        }
        leftSize -= needSize;
        if (info->isStdUuid) {
            stdCount++;
        } else {
            cusCount++;
        }
    }
    uint32_t realSize = mtu - leftSize;
    return FillMixPropertyPayloadV10(findProperty, realSize, stdCount, cusCount);
}

static void SendFindPropertyRsp(SSAP_Link_S *link, SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *uuid, bool isByUuid)
{
    int sceneCode = req->msgCode == SSAP_FIND_STRUCTURE_REQ ?
            EXCEP_SSAP_FIND_STRUCTURE_REQ_RECV : EXCEP_SSAP_FIND_BY_UUID_REQ_RECV;
    SDF_Traits findPropertyTraits = {.dtor = FreeFindPropertyInfo};
    SDF_Vector_S *findPropertys = SDF_CreateVector(findPropertyTraits);
    if (findPropertys == NULL) {
        CP_LOG_ERROR("[SSAP] find uuid memory failed");
        SSAP_PduErrorRsp(link, req->msgCode, 0, SSAP_ERRCODE_NO_RESOURCE);
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_NO_RESOURCE);
        return;
    }
    if (isByUuid) {
        SSAP_UuidType_E uuidType = GetUuidType(uuid);
        if (uuidType == UUID_TYPE_SERVICE) {
            FindPropertyListByService(req, uuid, findPropertys);
        } else if (uuidType == UUID_TYPE_PROPERTY) {
            FindPropertyList(req, uuid, findPropertys, isByUuid);
        } else {
            CP_LOG_ERROR("[SSAP] find uuid wrong type");
            SSAP_PduErrorRsp(link, req->msgCode, req->startHandle, SSAP_ERRCODE_ITEM_INEXIST);
            SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_ITEM_INEXIST);
            SDF_DestroyVector(findPropertys);
            return;
        }
    } else {
        FindPropertyList(req, uuid, findPropertys, isByUuid);
    }
    if (findPropertys->size == 0) {
        SSAP_PduErrorRsp(link, req->msgCode, req->startHandle, SSAP_ERRCODE_ITEM_INEXIST);
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_ITEM_INEXIST);
        SDF_DestroyVector(findPropertys);
        return;
    }
    uint32_t mtu = link->mtu;
    SDF_Buff_S *sdfBuff = NULL;
    if (req->ctrl.itemType == FIND_ITEM_TYPE_STANDARD) {
        sdfBuff = BuildPropertyPayload(mtu, findPropertys, SSAP_FIND_PROPERTY_STD_LEN);
    } else if (req->ctrl.itemType == FIND_ITEM_TYPE_CUSTOMIZE) {
        sdfBuff = BuildPropertyPayload(mtu, findPropertys, SSAP_FIND_PROPERTY_CUS_LEN);
    } else if (req->ctrl.itemType == FIND_ITEM_TYPE_MIX) {
        sdfBuff = BuildMixPropertyPayload(mtu, findPropertys);
    }
    uint8_t itemType = req->ctrl.itemType;
    SendFindRspPkt(link, req, sdfBuff, itemType);
    SDF_DestroyVector(findPropertys);
}

static SDF_Buff_S *BuildFindPropertyRspV10(SSAP_Link_S *link, SSAP_PduFindStructReq_S *req, SDF_Vector_S *findPropertys,
    bool hasCus)
{
    uint32_t mtu = link->mtu;
    if (hasCus) {
        if (req->msgCode == SSAP_FIND_STRUCTURE_REQ) {
            return BuildMixPropertyPayload(mtu, findPropertys);
        } else {
            return BuildMixPropertyPayloadV10(mtu, findPropertys);
        }
    } else {
        if (req->msgCode == SSAP_FIND_STRUCTURE_REQ) {
            return BuildPropertyPayload(mtu, findPropertys, SSAP_FIND_PROPERTY_STD_LEN);
        } else {
            return BuildPropertyPayloadV10(mtu, findPropertys, SSAP_FIND_PROPERTY_BASE_LEN);
        }
    }
}

static void SendFindPropertyRspV10(SSAP_Link_S *link, SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *uuid,
    bool isByUuid)
{
    int sceneCode = req->msgCode == SSAP_FIND_STRUCTURE_REQ ?
            EXCEP_SSAP_FIND_STRUCTURE_REQ_RECV : EXCEP_SSAP_FIND_BY_UUID_REQ_RECV;
    SDF_Traits findPropertyTraits = {.dtor = FreeFindPropertyInfo};
    SDF_Vector_S *findPropertys = SDF_CreateVector(findPropertyTraits);
    if (findPropertys == NULL) {
        CP_LOG_ERROR("[SSAP] find uuid memory failed");
        SSAP_PduErrorRsp(link, req->msgCode, 0, SSAP_ERRCODE_NO_RESOURCE);
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_NO_RESOURCE);
        return;
    }
    bool hasCus = false;
    if (isByUuid) {
        SSAP_UuidType_E uuidType = GetUuidType(uuid);
        if (uuidType == UUID_TYPE_PROPERTY) {
            FindPropertyListV10(req, uuid, findPropertys, isByUuid, &hasCus);
        } else {
            // 老版本不支持通过service uuid查property，返回没找到
            CP_LOG_ERROR("[SSAP] find uuid wrong type");
            SSAP_PduErrorRsp(link, req->msgCode, req->startHandle, SSAP_ERRCODE_ITEM_INEXIST);
            SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_ITEM_INEXIST);
            SDF_DestroyVector(findPropertys);
            return;
        }
    } else {
        FindPropertyListV10(req, uuid, findPropertys, isByUuid, &hasCus);
    }
    if (findPropertys->size == 0) {
        CP_LOG_ERROR("[SSAP] find property uuid inexist");
        SSAP_PduErrorRsp(link, req->msgCode, req->startHandle, SSAP_ERRCODE_ITEM_INEXIST);
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_ITEM_INEXIST);
        SDF_DestroyVector(findPropertys);
        return;
    }
    uint8_t itemType = hasCus ? FIND_ITEM_TYPE_MIX : FIND_ITEM_TYPE_STANDARD;
    SDF_Buff_S *sdfBuff = BuildFindPropertyRspV10(link, req, findPropertys, hasCus);
    SendFindRspPkt(link, req, sdfBuff, itemType);
    SDF_DestroyVector(findPropertys);
}

static void BuildMethodInfo(uint8_t *buf, SSAP_FindMethodInfo_S *info)
{
    uint8_t *curBuf = buf;
    SSAP_UINT16_TO_BYTE_LITTLE(curBuf, info->handle);
    curBuf += SSAP_HANDLE_LEN;
    uint32_t uuidSize = info->isStdUuid ? SSAP_UUID16_LEN : SSAP_UUID128_LEN;
    SSAP_PutUuidToPktBuf(&info->uuid, curBuf, uuidSize);
    curBuf += uuidSize;
    (void)memset_s(curBuf, SSAP_FIND_METHOD_EXTRA_LEN, 0, SSAP_FIND_METHOD_EXTRA_LEN);
}

static SDF_Buff_S *BuildMethodPayload(uint32_t mtu, SDF_Vector_S *findMethod, uint32_t baseMethodLen)
{
    CP_CHECK_LOG_RETURN(mtu >= SSAP_PDU_BASE_LEN, NULL, "[SSAP] mtu error");
    size_t count = 0;
    uint32_t leftSize = mtu - SSAP_PDU_BASE_LEN;
    for (size_t i = 0; i < findMethod->size; i++) {
        if (leftSize >= baseMethodLen) {
            count++;
            leftSize -= baseMethodLen;
        } else {
            break;
        }
    }
    uint32_t realSize = mtu - leftSize;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN(sdfBuff != NULL, NULL, "[SSAP] find rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] find rsp create buf fail");
        return NULL;
    }
    buf += SSAP_PDU_BASE_LEN;
    for (size_t i = 0; i < count; i++) {
        SSAP_FindMethodInfo_S *info = SDF_VectorElementAt(findMethod, i);
        BuildMethodInfo(buf, info);
        buf += baseMethodLen;
    }
    return sdfBuff;
}

static SDF_Buff_S *FillMixMethodPayload(SDF_Vector_S *findMethod, uint32_t realSize, uint32_t totalStdCount,
    uint32_t totalCusCount)
{
    if (totalStdCount > UINT8_MAX || totalCusCount > UINT8_MAX) {
        CP_LOG_ERROR("[SSAP] find rsp stdCount or cusCount overflow");
        return NULL;
    }
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN(sdfBuff != NULL, NULL, "[SSAP] find rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] find rsp create buf fail");
        return NULL;
    }
    buf += SSAP_PDU_BASE_LEN;
    SSAP_FindInfoIndicator_S *stdInd = (SSAP_FindInfoIndicator_S *)buf;
    stdInd->count = (uint8_t)totalStdCount;
    stdInd->type = SSAP_FIND_INFO_INDICATION_STD;
    buf += SSAP_FIND_INFO_INDICATION_LEN;
    for (size_t i = 0, count = totalStdCount; i < findMethod->size && count > 0; i++) {
        SSAP_FindMethodInfo_S *info = SDF_VectorElementAt(findMethod, i);
        if (info->isStdUuid) {
            BuildMethodInfo(buf, info);
            buf += SSAP_FIND_METHOD_STD_LEN;
            count--;
        }
    }
    SSAP_FindInfoIndicator_S *cusInd = (SSAP_FindInfoIndicator_S *)buf;
    cusInd->count = (uint8_t)totalCusCount;
    cusInd->type = SSAP_FIND_INFO_INDICATION_CUS;
    buf += SSAP_FIND_INFO_INDICATION_LEN;
    for (size_t i = 0, count = totalCusCount; i < findMethod->size && count > 0; i++) {
        SSAP_FindMethodInfo_S *info = SDF_VectorElementAt(findMethod, i);
        if (!info->isStdUuid) {
            BuildMethodInfo(buf, info);
            buf += SSAP_FIND_METHOD_CUS_LEN;
            count--;
        }
    }
    return sdfBuff;
}

static SDF_Buff_S *BuildMixMethodPayload(uint32_t mtu, SDF_Vector_S *findMethod)
{
    uint32_t stdCount = 0;
    uint32_t cusCount = 0;
    uint32_t leftSize = mtu - SSAP_PDU_BASE_LEN - SSAP_FIND_INFO_INDICATION_LEN * 2;
    for (size_t i = 0; i < findMethod->size; i++) {
        SSAP_FindMethodInfo_S *info = SDF_VectorElementAt(findMethod, i);
        uint32_t needSize = info->isStdUuid ? SSAP_FIND_METHOD_STD_LEN : SSAP_FIND_METHOD_CUS_LEN;
        if (leftSize < needSize) {
            break;
        }
        leftSize -= needSize;
        if (info->isStdUuid) {
            stdCount++;
        } else {
            cusCount++;
        }
    }
    uint32_t realSize = mtu - leftSize;
    return FillMixMethodPayload(findMethod, realSize, stdCount, cusCount);
}

static void FindMethodList(SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *uuid, SDF_Vector_S *findMethod,
    bool isByUuid)
{
    SDF_Vector_S *services = SSAPS_GetServices();
    CP_CHECK_LOG_RETURN_VOID(services != NULL, "[SSAP] cannot get services");
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        if (service->methods == NULL) {
            continue;
        }
        for (size_t j = 0; j < service->methods->size; j++) {
            SSAP_Method_S *method = SDF_VectorElementAt(service->methods, j);
            if ((!isByUuid || SSAP_IsUuidEqual(&method->uuid, uuid)) && method->handle >= req->startHandle &&
                method->handle <= req->endHandle && CheckFindItemType(&method->uuid, req->ctrl.itemType)) {
                CP_LOG_INFO("[SSAP] find method, handle = %u", method->handle);
                SSAP_FindMethodInfo_S *info =
                    (SSAP_FindMethodInfo_S *)SDF_MemZalloc(sizeof(SSAP_FindMethodInfo_S));
                CP_CHECK_LOG_RETURN_VOID(info != NULL, "[SSAP] find method malloc failed");
                info->handle = method->handle;
                (void)memcpy_s(&info->uuid, sizeof(NLSTK_SsapUuid_S), &method->uuid, sizeof(NLSTK_SsapUuid_S));
                info->isStdUuid = SSAP_CheckUuidStd(&info->uuid);
                InfoEmplaceBackAndCheckFree(findMethod, info);    // emplace失败会在内部释放info
            }
        }
    }
}

static void FindMethodListByService(SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *uuid, SDF_Vector_S *findMethod)
{
    SDF_Vector_S *services = SSAPS_GetServices();
    CP_CHECK_LOG_RETURN_VOID(services != NULL, "[SSAP] cannot get services");
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        if (!SSAP_IsUuidEqual(&service->uuid, uuid) || service->methods == NULL) {
            continue;
        }
        for (size_t j = 0; j < service->methods->size; j++) {
            SSAP_Method_S *method = SDF_VectorElementAt(service->methods, j);
            if (method->handle >= req->startHandle && method->handle <= req->endHandle &&
                CheckFindItemType(&method->uuid, req->ctrl.itemType)) {
                CP_LOG_INFO("[SSAP] find method, handle = %u", method->handle);
                SSAP_FindMethodInfo_S *info =
                    (SSAP_FindMethodInfo_S *)SDF_MemZalloc(sizeof(SSAP_FindMethodInfo_S));
                CP_CHECK_LOG_RETURN_VOID(info != NULL, "[SSAP] find method malloc failed");
                info->handle = method->handle;
                (void)memcpy_s(&info->uuid, sizeof(NLSTK_SsapUuid_S), &method->uuid, sizeof(NLSTK_SsapUuid_S));
                info->isStdUuid = SSAP_CheckUuidStd(&info->uuid);
                InfoEmplaceBackAndCheckFree(findMethod, info);  // emplace失败会在内部释放info
            }
        }
        return;
    }
}

static void SendFindMethodRsp(SSAP_Link_S *link, SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *uuid, bool isByUuid)
{
    SDF_Traits findMethodTraits = {.dtor = SDF_MemFree};
    SDF_Vector_S *findMethod = SDF_CreateVector(findMethodTraits);
    if (findMethod == NULL) {
        CP_LOG_ERROR("[SSAP] findMethod memory failed");
        SSAP_PduErrorRsp(link, req->msgCode, req->startHandle, SSAP_ERRCODE_NO_RESOURCE);
        return;
    }
    if (isByUuid) {
        SSAP_UuidType_E uuidType = GetUuidType(uuid);
        if (uuidType == UUID_TYPE_SERVICE) {
            FindMethodListByService(req, uuid, findMethod);
        } else if (uuidType == UUID_TYPE_METHOD) {
            FindMethodList(req, uuid, findMethod, isByUuid);
        } else {
            CP_LOG_ERROR("[SSAP] find uuid wrong type");
            SSAP_PduErrorRsp(link, req->msgCode, req->startHandle, SSAP_ERRCODE_ITEM_INEXIST);
            SDF_DestroyVector(findMethod);
            return;
        }
    } else {
        FindMethodList(req, uuid, findMethod, isByUuid);
    }
    if (findMethod->size == 0) {
        SSAP_PduErrorRsp(link, req->msgCode, req->startHandle, SSAP_ERRCODE_ITEM_INEXIST);
        SDF_DestroyVector(findMethod);
        return;
    }
    uint32_t mtu = link->mtu;
    SDF_Buff_S *sdfBuff = NULL;
    uint8_t itemType = req->ctrl.itemType;
    if (itemType == FIND_ITEM_TYPE_STANDARD) {
        sdfBuff = BuildMethodPayload(mtu, findMethod, SSAP_FIND_METHOD_STD_LEN);
    } else if (itemType == FIND_ITEM_TYPE_CUSTOMIZE) {
        sdfBuff = BuildMethodPayload(mtu, findMethod, SSAP_FIND_METHOD_CUS_LEN);
    } else if (itemType == FIND_ITEM_TYPE_MIX) {
        sdfBuff = BuildMixMethodPayload(mtu, findMethod);
    }
    SendFindRspPkt(link, req, sdfBuff, itemType);
    SDF_DestroyVector(findMethod);
}

static bool ShouldIncludeService(SSAP_Service_S *service, NLSTK_SsapUuid_S *uuid, bool isByUuid, uint16_t startHandle,
    uint16_t endHandle, uint8_t itemType)
{
    if ((service->handle >= startHandle && service->handle <= endHandle) ||
        (service->endHandle >= startHandle && service->endHandle <= endHandle)) {
        if (isByUuid && !SSAP_IsUuidEqual(&service->uuid, uuid)) {
            return false;
        }
        if (!CheckFindItemType(&service->uuid, itemType)) {
            return false;
        }
        return true;
    }
    return false;
}

static SSAP_FindStructureInfo_S *CreateServiceStructureInfo(SSAP_Service_S *service)
{
    SSAP_FindStructureInfo_S *info = (SSAP_FindStructureInfo_S *)SDF_MemZalloc(sizeof(SSAP_FindStructureInfo_S));
    CP_CHECK_LOG_RETURN(info != NULL, NULL, "[SSAP] CreateServiceStructureInfo malloc failed");
    info->handle = service->handle;
    info->itemType = service->serviceType;
    (void)memcpy_s(&info->uuid, sizeof(NLSTK_SsapUuid_S), &service->uuid, sizeof(NLSTK_SsapUuid_S));
    info->isStdUuid = SSAP_CheckUuidStd(&info->uuid);
    if (service->descriptors == NULL) {
        return info;
    }
    info->descriptorCount = service->descriptors->size;
    if (info->descriptorCount != 0) {
        info->descriptors = SDF_MemZalloc(info->descriptorCount);
        if (info->descriptors == NULL) {
            CP_LOG_ERROR("[SSAP] copy to property memory fail");
            info->descriptorCount = 0;
            SDF_MemFree(info);
            return NULL;
        }
        for (size_t i = 0; i < info->descriptorCount; i++) {
            SSAP_Descriptor_S *descriptor = SDF_VectorElementAt(service->descriptors, i);
            info->descriptors[i] = descriptor->type;
        }
    }
    return info;
}

static void AddServiceStructureInfo(SDF_Vector_S *findStructures, SSAP_Service_S *service, uint16_t startHandle,
    uint16_t endHandle)
{
    if (service->handle >= startHandle && service->handle <= endHandle) {
        SSAP_FindStructureInfo_S *info = CreateServiceStructureInfo(service);
        if (info != NULL) {
            InfoEmplaceBackAndCheckFree(findStructures, info);
        }
    }
}

static bool InitPropertyInfoDescriptors(SSAP_Property_S *property, SSAP_FindStructureInfo_S *propInfo)
{
    if (propInfo->descriptorCount == 0) {
        propInfo->descriptors = NULL;
        return true;
    }
    propInfo->descriptors = (uint8_t *)SDF_MemZalloc(propInfo->descriptorCount);
    if (propInfo->descriptors == NULL) {
        CP_LOG_ERROR("[SSAP] property descriptors malloc failed");
        propInfo->descriptorCount = 0;
        return false;
    }
    for (size_t i = 0; i < propInfo->descriptorCount; i++) {
        SSAP_Descriptor_S *desc = SDF_VectorElementAt(property->descriptors, i);
        propInfo->descriptors[i] = (uint8_t)desc->type;
    }
    return true;
}

static SSAP_FindStructureInfo_S *CreatePropertyStructureInfo(SSAP_Property_S *property)
{
    SSAP_FindStructureInfo_S *propInfo =
        (SSAP_FindStructureInfo_S *)SDF_MemZalloc(sizeof(SSAP_FindStructureInfo_S));
    CP_CHECK_LOG_RETURN(propInfo != NULL, NULL, "[SSAP] CreatePropertyStructureInfo malloc failed");
    propInfo->handle = property->handle;
    (void)memcpy_s(&propInfo->uuid, sizeof(NLSTK_SsapUuid_S), &property->uuid, sizeof(NLSTK_SsapUuid_S));
    propInfo->isStdUuid = SSAP_CheckUuidStd(&propInfo->uuid);
    propInfo->itemType = propInfo->isStdUuid ? ITEM_TYPE_STD_PROPERTY : ITEM_TYPE_VENDOR_PROPERTY;
    propInfo->operation = property->operation;
    propInfo->descriptorCount = (property->descriptors != NULL) ? (uint8_t)property->descriptors->size : 0;
    if (!InitPropertyInfoDescriptors(property, propInfo)) {
        SDF_MemFree(propInfo);
        return NULL;
    }
    return propInfo;
}

static void AddPropertyStructureInfos(SDF_Vector_S *findStructures, SSAP_Service_S *service, uint16_t startHandle,
    uint16_t endHandle)
{
    if (service->properties == NULL) {
        return;
    }
    for (size_t j = 0; j < service->properties->size; j++) {
        SSAP_Property_S *property = SDF_VectorElementAt(service->properties, j);
        if (property->handle >= startHandle && property->handle <= endHandle) {
            SSAP_FindStructureInfo_S *propInfo = CreatePropertyStructureInfo(property);
            if (propInfo != NULL) {
                InfoEmplaceBackAndCheckFree(findStructures, propInfo);
            }
        }
    }
}

static SSAP_FindStructureInfo_S *CreateMethodStructureInfo(SSAP_Method_S *method)
{
    SSAP_FindStructureInfo_S *methodInfo =
        (SSAP_FindStructureInfo_S *)SDF_MemZalloc(sizeof(SSAP_FindStructureInfo_S));
    CP_CHECK_LOG_RETURN(methodInfo != NULL, NULL, "[SSAP] CreateMethodStructureInfo malloc failed");
    methodInfo->handle = method->handle;
    (void)memcpy_s(&methodInfo->uuid, sizeof(NLSTK_SsapUuid_S), &method->uuid, sizeof(NLSTK_SsapUuid_S));
    methodInfo->isStdUuid = SSAP_CheckUuidStd(&methodInfo->uuid);
    methodInfo->itemType = methodInfo->isStdUuid ? ITEM_TYPE_STD_METHOD : ITEM_TYPE_VENDOR_METHOD;
    return methodInfo;
}

static void AddMethodStructureInfos(SDF_Vector_S *findStructures, SSAP_Service_S *service, uint16_t startHandle,
    uint16_t endHandle)
{
    if (service->methods == NULL) {
        return;
    }
    for (size_t j = 0; j < service->methods->size; j++) {
        SSAP_Method_S *method = SDF_VectorElementAt(service->methods, j);
        if (method->handle >= startHandle && method->handle <= endHandle) {
            SSAP_FindStructureInfo_S *methodInfo = CreateMethodStructureInfo(method);
            if (methodInfo != NULL) {
                InfoEmplaceBackAndCheckFree(findStructures, methodInfo);
            }
        }
    }
}

static void FindStructureList(SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *uuid,
    SDF_Vector_S *findStructures, bool isByUuid)
{
    SDF_Vector_S *services = SSAPS_GetServices();
    CP_CHECK_LOG_RETURN_VOID(services != NULL, "[SSAP] cannot get services");
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        if (!ShouldIncludeService(service, uuid, isByUuid, req->startHandle, req->endHandle, req->ctrl.itemType)) {
            continue;
        }
        AddServiceStructureInfo(findStructures, service, req->startHandle, req->endHandle);
        AddPropertyStructureInfos(findStructures, service, req->startHandle, req->endHandle);
        AddMethodStructureInfos(findStructures, service, req->startHandle, req->endHandle);
    }
}

static void BuildStructureInfo(uint8_t *buf, SSAP_FindStructureInfo_S *info)
{
    uint8_t *curBuf = buf;
    // handle
    SSAP_UINT16_TO_BYTE_LITTLE(curBuf, info->handle);
    curBuf += SSAP_HANDLE_LEN;

    // itemType
    *curBuf = (uint8_t)info->itemType;
    curBuf += SSAP_FIND_STRUCTURE_TYPE_LEN;

    // uuid
    uint32_t uuidSize = info->isStdUuid ? SSAP_UUID16_LEN : SSAP_UUID128_LEN;
    SSAP_PutUuidToPktBuf(&info->uuid, curBuf, uuidSize);
    curBuf += uuidSize;

    // startHandle endHandle
    if (GetStartEndHandleLenByItemType(info->itemType) > 0) {
        SSAP_UINT16_TO_BYTE_LITTLE(curBuf, info->startHandle);
        curBuf += SSAP_HANDLE_LEN;
        SSAP_UINT16_TO_BYTE_LITTLE(curBuf, info->endHandle);
        curBuf += SSAP_HANDLE_LEN;
    }

    // operation
    if (GetOperationLenByItemType(info->itemType) > 0) {
        SSAP_UINT32_TO_BYTE_LITTLE(curBuf, info->operation.operationValue);
        curBuf += SSAP_FIND_OPERATION_LEN;
    }

    // descriptorTypeList
    if (GetDescriptorCountLenByItemType(info->itemType) > 0) {
        *curBuf = info->descriptorCount;
        curBuf += SSAP_FIND_DESCRIPTOR_COUNT_LEN;
        for (int i = 0; i < info->descriptorCount; i++) {
            curBuf[i] = info->descriptors[i];
        }
        curBuf += info->descriptorCount;
    }
}

static uint32_t GetStructureInfoSize(SSAP_FindStructureInfo_S *info)
{
    uint32_t uuidSize = info->isStdUuid ? SSAP_UUID16_LEN : SSAP_UUID128_LEN;
    uint32_t baseLen = SSAP_HANDLE_LEN + SSAP_FIND_STRUCTURE_TYPE_LEN + uuidSize;
    uint8_t startEndHandleLen = GetStartEndHandleLenByItemType(info->itemType);
    uint8_t operationLen = GetOperationLenByItemType(info->itemType);

    uint8_t descriptorCountLen = GetDescriptorCountLenByItemType(info->itemType);
    uint8_t descriptorTypeListLen = 0;
    if (descriptorCountLen != 0) {
        descriptorTypeListLen = descriptorCountLen + info->descriptorCount;
    }

    return baseLen + startEndHandleLen + operationLen + descriptorTypeListLen;
}

static SDF_Buff_S *BuildStructurePayload(uint32_t mtu, SDF_Vector_S *findStructures)
{
    CP_CHECK_LOG_RETURN(mtu >= SSAP_PDU_BASE_LEN, NULL, "[SSAP] mtu error");
    size_t count = 0;
    uint32_t leftSize = mtu - SSAP_PDU_BASE_LEN;
    for (size_t i = 0; i < findStructures->size; i++) {
        SSAP_FindStructureInfo_S *info = SDF_VectorElementAt(findStructures, i);
        uint32_t infoNeedSize = GetStructureInfoSize(info);
        if (leftSize >= infoNeedSize) {
            count++;
            leftSize -= infoNeedSize;
        } else {
            break;
        }
    }
    uint32_t realSize = mtu - leftSize;
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN(sdfBuff != NULL, NULL, "[SSAP] find rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] find rsp create buf fail");
        return NULL;
    }
    buf += SSAP_PDU_BASE_LEN;
    for (size_t i = 0; i < count; i++) {
        SSAP_FindStructureInfo_S *info = SDF_VectorElementAt(findStructures, i);
        uint32_t infoNeedSize = GetStructureInfoSize(info);
        BuildStructureInfo(buf, info);
        buf += infoNeedSize;
    }
    return sdfBuff;
}

static void FreeFindStructureInfo(void *ptr)
{
    SSAP_FindStructureInfo_S *info = (SSAP_FindStructureInfo_S *)ptr;
    if (info == NULL) {
        return;
    }
    if (info->descriptors != NULL) {
        SDF_MemFree(info->descriptors);
    }
    SDF_MemFree(info);
}

static void SendFindStructureRsp(SSAP_Link_S *link, SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *uuid, bool isByUuid)
{
    CP_LOG_INFO("[SSAP] SendFindStructureRsp");
    int sceneCode = req->msgCode == SSAP_FIND_STRUCTURE_REQ ?
        EXCEP_SSAP_FIND_STRUCTURE_REQ_RECV : EXCEP_SSAP_FIND_BY_UUID_REQ_RECV;
    // by uuid发现服务结构，uuid类型应该是服务的uuid
    if (isByUuid && GetUuidType(uuid) != UUID_TYPE_SERVICE) {
        CP_LOG_ERROR("[SSAP] find uuid wrong type");
        SSAP_PduErrorRsp(link, req->msgCode, req->startHandle, SSAP_ERRCODE_INVALID_PDU);
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_INVALID_PDU);
        return;
    }
    uint32_t mtu = link->mtu;
    SDF_Traits findStructureTrait = {.dtor = FreeFindStructureInfo};
    SDF_Vector_S *findStructures = SDF_CreateVector(findStructureTrait);
    if (findStructures == NULL) {
        CP_LOG_ERROR("[SSAP] find structure memory failed");
        SSAP_PduErrorRsp(link, req->msgCode, 0, SSAP_ERRCODE_NO_RESOURCE);
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_NO_RESOURCE);
        return;
    }
    FindStructureList(req, uuid, findStructures, isByUuid);
    if (findStructures->size == 0) {
        SSAP_PduErrorRsp(link, req->msgCode, req->startHandle, SSAP_ERRCODE_ITEM_INEXIST);
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_ITEM_INEXIST);
        SDF_DestroyVector(findStructures);
        return;
    }
    SDF_Buff_S *sdfBuff = NULL;
    sdfBuff = BuildStructurePayload(mtu, findStructures);
    SendFindRspPkt(link, req, sdfBuff, req->ctrl.itemType);
    SDF_DestroyVector(findStructures);
}

static void SendFindRsp(SSAP_Link_S *link, SSAP_PduFindStructReq_S *req, NLSTK_SsapUuid_S *reqUuid, bool isByUuid)
{
    int sceneCode = req->msgCode == SSAP_FIND_STRUCTURE_REQ ?
            EXCEP_SSAP_FIND_STRUCTURE_REQ_RECV : EXCEP_SSAP_FIND_BY_UUID_REQ_RECV;
    if (((req->ctrl.itemType != FIND_ITEM_TYPE_STANDARD) && (req->ctrl.itemType != FIND_ITEM_TYPE_MIX) &&
        (req->ctrl.itemType != FIND_ITEM_TYPE_CUSTOMIZE)) || req->ctrl.rspMode == FIND_RSP_MODE_MULTI_RSP) {
        CP_LOG_ERROR("[SSAP] recv not support item type: 0x%d", req->ctrl.itemType);
        SSAP_PduErrorRsp(link, req->msgCode, 0, SSAP_ERRCODE_UNSUPPORT_PDU);
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_UNSUPPORT_PDU);
        return;
    }
    switch (req->ctrl.findType) {
        case FIND_STRUCTURE_TYPE_SERVICE_STRUCTURE:
            SendFindStructureRsp(link, req, reqUuid, isByUuid);
            break;
        case FIND_STRUCTURE_TYPE_PRIMARY_SERVICE:
            if (CM_GetLogicLinkDeviceType(link->lcid) == CM_DEVTYPE_OLD) {
                SendFindPrimaryServiceRspV10(link, req, reqUuid, isByUuid);
            } else {
                SendFindPrimaryServiceRsp(link, req, reqUuid, isByUuid);
            }
            break;
        case FIND_STRUCTURE_TYPE_PROPERTY:
            if (CM_GetLogicLinkDeviceType(link->lcid) == CM_DEVTYPE_OLD) {
                SendFindPropertyRspV10(link, req, reqUuid, isByUuid);
            } else {
                SendFindPropertyRsp(link, req, reqUuid, isByUuid);
            }
            break;
        case FIND_STRUCTURE_TYPE_METHOD:
            SendFindMethodRsp(link, req, reqUuid, isByUuid);
            break;
        case FIND_STRUCTURE_TYPE_EVENT:
            CP_LOG_DEBUG("[SSAP] find event item inexist");
            SSAP_PduErrorRsp(link, req->msgCode, 0, SSAP_ERRCODE_ITEM_INEXIST);
            break;
        default:
            CP_LOG_ERROR("[SSAP] recv not support find type: 0x%d", req->ctrl.findType);
            SSAP_PduErrorRsp(link, req->msgCode, 0, SSAP_ERRCODE_UNSUPPORT_PDU);
            SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_UNSUPPORT_PDU);
            break;
    }
}

void SSAPS_FindReqHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    CP_LOG_DEBUG("enter find req handle");
    uint8_t *buf = SDF_DataOffset(sdfBuff);
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(sdfBuff) <= SSAP_STACK_MTU_MAX, "[SSAP] find req pdu size overflow");
    uint32_t size = (uint32_t)SDF_DataLenGet(sdfBuff);
    SSAP_PduFindStructReq_S *req = (SSAP_PduFindStructReq_S *)buf;
    if ((req->msgCode == SSAP_FIND_STRUCTURE_REQ) && (size != sizeof(SSAP_PduFindStructReq_S))) {
        CP_LOG_ERROR("[SSAP] find req handle wrong len, len: %d", size);
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_FIND_STRUCTURE_REQ_RECV, EXCEP_SSAP_INVALID_PDU);
        SSAP_PduErrorRsp(link, req->msgCode, 0, SSAP_ERRCODE_INVALID_PDU);
        return;
    }
    if ((req->msgCode == SSAP_FIND_STRUCTURE_BY_UUID_REQ) && (size <= sizeof(SSAP_PduFindStructReq_S))) {
        CP_LOG_ERROR("[SSAP] find req handle wrong len, len: %d", size);
        SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_FIND_BY_UUID_REQ_RECV, EXCEP_SSAP_INVALID_PDU);
        SSAP_PduErrorRsp(link, req->msgCode, 0, SSAP_ERRCODE_INVALID_PDU);
        return;
    }
    if ((req->startHandle > req->endHandle) || (req->startHandle == 0)) {
        CP_LOG_ERROR("[SSAP] find req handle start handle: %d, end handle: %d",
            req->startHandle, req->endHandle);
        SSAP_PduErrorRsp(link, req->msgCode, req->startHandle, SSAP_ERRCODE_INVALID_HANDLE);
        int sceneCode = req->msgCode == SSAP_FIND_STRUCTURE_REQ ?
            EXCEP_SSAP_FIND_STRUCTURE_REQ_RECV : EXCEP_SSAP_FIND_BY_UUID_REQ_RECV;
        SDF_SsapTrace(link->addr.addr, sceneCode, EXCEP_SSAP_INVALID_HANDLE);
        return;
    }
    NLSTK_SsapUuid_S uuid = {0};
    bool isByUuid = false;
    if (size > sizeof(SSAP_PduFindStructReq_S)) {
        uint32_t uuidLen = size - sizeof(SSAP_PduFindStructReq_S);
        if (uuidLen != SSAP_UUID16_LEN && uuidLen != SSAP_UUID128_LEN) {
            CP_LOG_ERROR("[SSAP] find req handle wrong uuid len, len: %d", size);
            SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_FIND_BY_UUID_REQ_RECV, EXCEP_SSAP_INVALID_PDU);
            SSAP_PduErrorRsp(link, req->msgCode, 0, SSAP_ERRCODE_INVALID_PDU);
            return;
        }
        SSAP_GetUuidFromPktBuf(&uuid, req->uuid, uuidLen);
        isByUuid = true;
    }
    SendFindRsp(link, req, &uuid, isByUuid);
}

#ifdef __cplusplus
}
#endif