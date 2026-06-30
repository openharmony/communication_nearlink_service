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
#include "securec.h"
#include "sdf_mem.h"
#include "sdf_vector.h"
#include "sdf_buff.h"
#include "cpfwk_log.h"
#include "nlstk_log.h"

#include "cm_logic_link_api.h"
#include "nlstk_sm_api.h"
#include "nlstk_sm_algos.h"

#include "ssap_type.h"
#include "ssap_common.h"
#include "ssap_utils.h"
#include "ssap_manager.h"
#include "ssap_link.h"
#include "ssaps_server.h"
#include "ssaps_service_param.h"
#include "ssap_handle.h"
#include "ssaps_server_app.h"
#include "ssaps_service.h"

#define SSAP_INVALID_HANDLE 0x0000
#define SSAP_INIT_HANDLE 0x0010
#define SSAP_STRUCTURE_FIXED_SIZE    \
    (sizeof(uint16_t) + sizeof(NLSTK_SsapItemType_E) + sizeof(NLSTK_SsapUuid_S) + sizeof(NLSTK_SsapOperation_S))
#define SSAP_SERVICE_OPERATION 0x00000001
#define SSAP_HASH_REAL_SIZE 16
#define SSAP_SERVICE_CHANGE_EVENT_SIZE 4

static uint16_t g_curHandle = SSAP_INIT_HANDLE;
static uint8_t g_ssapHash[SSAP_HASH_REAL_SIZE];

/*
 * 由于北向接口限制，service、property、descriptor会按照顺序添加
 * 添加完成后会调用startservice，此时才能够确认handle数值
 * 为了防止此时有其他任务读取到不完整的service，因此需要用一个临时变量存储新添加的service
*/
static SSAP_Service_S *g_tempService = NULL;
static SDF_Vector_S *g_services = NULL;

static void FindAvaliableHandle(uint16_t handleNum, uint16_t *start, uint16_t *end)
{
    if (g_curHandle == SSAP_INVALID_HANDLE || g_curHandle > SSAP_END_HANDLE - handleNum + 1) {
        return;
    }
    *start = g_curHandle;
    *end = *start - 1 + handleNum;
    if (g_curHandle > SSAP_END_HANDLE - handleNum) {
        g_curHandle = SSAP_INVALID_HANDLE;
    } else {
        g_curHandle += handleNum;
    }
}

void SSAPS_CacheService(SSAP_ParamAddService_S *addService)
{
    if (g_tempService != NULL) {
        FreeService(g_tempService);
    }

    g_tempService = (SSAP_Service_S *)SDF_MemZalloc(sizeof(SSAP_Service_S));
    CP_CHECK_LOG_RETURN_VOID(g_tempService != NULL, "[SSAP] cache service malloc failed");
    SDF_Traits propertyTraits = {.dtor = FreeProperty};
    g_tempService->properties = SDF_CreateVector(propertyTraits);
    if (g_tempService->properties == NULL) {
        goto FAIL;
    }
    SDF_Traits referenceTraits = {.dtor = SDF_MemFree};
    g_tempService->references = SDF_CreateVector(referenceTraits);
    if (g_tempService->references == NULL) {
        goto FAIL;
    }
    SDF_Traits methodTraits = {.dtor = SDF_MemFree};
    g_tempService->methods = SDF_CreateVector(methodTraits);
    if (g_tempService->methods == NULL) {
        goto FAIL;
    }
    SDF_Traits eventTraits = {.dtor = SDF_MemFree};
    g_tempService->events = SDF_CreateVector(eventTraits);
    if (g_tempService->events == NULL) {
        goto FAIL;
    }
    g_tempService->serviceType = addService->serviceType;
    g_tempService->protocol = SSAP_SERVICE_TYPE_GLE;
    g_tempService->callback = addService->callback;
    (void)memcpy_s(&g_tempService->uuid, sizeof(NLSTK_SsapUuid_S), &addService->uuid, sizeof(NLSTK_SsapUuid_S));
    CP_LOG_INFO("[SSAP] cache service success, uuid: %s, type: %d", SSAP_GET_ENC_UUID(&g_tempService->uuid),
        g_tempService->serviceType);
    return;
FAIL:
    SDF_DestroyVector(g_tempService->methods);
    SDF_DestroyVector(g_tempService->references);
    SDF_DestroyVector(g_tempService->properties);
    CP_LOG_ERROR("[SSAP] cache service create vector failed");
    SDF_MemFree(g_tempService);
    g_tempService = NULL;
}

void SSAPS_CacheProperty(SSAP_ParamAddProperty_S *addProperty)
{
    CP_CHECK_LOG_RETURN_VOID(g_tempService != NULL, "[SSAP] cache property temp service is null");
    SSAP_Property_S *property = (SSAP_Property_S *)SDF_MemZalloc(sizeof(SSAP_Property_S));
    CP_CHECK_LOG_RETURN_VOID(property != NULL, "[SSAP] cache property malloc failed");
    SSAP_LengthValue_S *val = (SSAP_LengthValue_S *)SDF_MemZalloc(sizeof(SSAP_LengthValue_S) + addProperty->val.len);
    if (val == NULL) {
        CP_LOG_ERROR("[SSAP] cache property malloc failed");
        SDF_MemFree(property);
        return;
    }
    SDF_Traits descriptorTraits = {.dtor = FreeDescriptor};
    property->descriptors = SDF_CreateVector(descriptorTraits);
    if (property->descriptors == NULL) {
        CP_LOG_ERROR("[SSAP] cache property create vector failed");
        SDF_MemFree(val);
        SDF_MemFree(property);
        return;
    }
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &addProperty->uuid, sizeof(NLSTK_SsapUuid_S));
    property->operation.operationValue = addProperty->operation.operationValue;
    property->permission.permissionValue = addProperty->permission.permissionValue;
    property->val = val;
    property->val->len = addProperty->val.len;
    if (addProperty->val.len != 0) {
        (void)memcpy_s(property->val->value, addProperty->val.len, addProperty->val.value, addProperty->val.len);
    } else {
        CP_LOG_ERROR("[SSAP] cache property value len is 0");
    }

    if (!SDF_VectorEmplaceBack(g_tempService->properties, property)) {
        CP_LOG_ERROR("[SSAP] cache property add vector failed");
        FreeProperty(property);
        return;
    }
    CP_LOG_INFO("[SSAP] cache property success, uuid: %s, opCode: %d, perm: %d, value len: %d",
        SSAP_GET_ENC_UUID(&addProperty->uuid), property->operation.operationValue,
        property->permission.permissionValue, property->val->len);
}

void SSAPS_CacheDescriptor(SSAP_ParamAddDescriptor_S *addDescriptor)
{
    CP_CHECK_LOG_RETURN_VOID(g_tempService != NULL, "[SSAP] cache desc temp service is null");
    CP_CHECK_LOG_RETURN_VOID(g_tempService->properties->size != 0, "[SSAP] cache desc temp properties is empty");
    SSAP_Property_S *property = SDF_VectorElementAt(g_tempService->properties, g_tempService->properties->size - 1);
    SSAP_Descriptor_S *descriptor = (SSAP_Descriptor_S *)SDF_MemZalloc(sizeof(SSAP_Descriptor_S));
    CP_CHECK_LOG_RETURN_VOID(descriptor != NULL, "[SSAP] cache desc malloc failed");
    SSAP_LengthValue_S *val = (SSAP_LengthValue_S *)SDF_MemZalloc(sizeof(SSAP_LengthValue_S) + addDescriptor->val.len);
    if (val == NULL) {
        CP_LOG_ERROR("[SSAP] cache desc malloc failed");
        SDF_MemFree(descriptor);
        return;
    }
    SDF_Traits clientConfigTraits = {.dtor = FreeClientConfig};
    descriptor->clientConfigs = SDF_CreateVector(clientConfigTraits);
    if (descriptor->clientConfigs == NULL) {
        SDF_MemFree(val);
        SDF_MemFree(descriptor);
        return;
    }
    descriptor->type = addDescriptor->type;
    descriptor->operation.operationValue = addDescriptor->operation.operationValue;
    descriptor->permission.permissionValue = addDescriptor->permission.permissionValue;
    descriptor->val = val;
    descriptor->val->len = addDescriptor->val.len;
    if (addDescriptor->val.len != 0) {
        (void)memcpy_s(descriptor->val->value, addDescriptor->val.len, addDescriptor->val.value,
            addDescriptor->val.len);
    } else {
        CP_LOG_ERROR("[SSAP] cache desc value len is 0");
    }
    if (!SDF_VectorEmplaceBack(property->descriptors, descriptor)) {
        CP_LOG_ERROR("[SSAP] cache desc add vector failed");
        FreeDescriptor(descriptor);
    }
    CP_LOG_INFO("[SSAP] cache desc success, type: %d, opCode: %d, perm: %d, value len: %d",
        addDescriptor->type, addDescriptor->operation, addDescriptor->permission, addDescriptor->val.len);
}

static void SsapServerGenerateHandleForProperties(SDF_Vector_S *properties, uint16_t *nextHandle)
{
    uint16_t start = *nextHandle;
    for (size_t i = 0; i < properties->size; i++) {
        SSAP_Property_S *property = SDF_VectorElementAt(properties, i);
        property->handle = start++;
        NLSTK_LOG_INFO("[SSAP] start service property handle: %d, uuid: %s.", property->handle,
                     SSAP_GET_ENC_UUID(&property->uuid));
    }
    *nextHandle = start;
}

static void SsapServerGenerateHandleForMethod(SDF_Vector_S *methods, uint16_t *nextHandle)
{
    uint16_t start = *nextHandle;
    for (size_t i = 0; i < methods->size; i++) {
        SSAP_Method_S *method = SDF_VectorElementAt(methods, i);
        method->handle = start++;
        NLSTK_LOG_INFO("[SSAP] start service method handle: %d, uuid: %s.", method->handle,
                     SSAP_GET_ENC_UUID(&method->uuid));
    }
    *nextHandle = start;
}

static int SSAPSortServicesByHandle(const void *a, const void *b)
{
    const SSAP_Service_S *serviceA = *(const SSAP_Service_S **)a;
    const SSAP_Service_S *serviceB = *(const SSAP_Service_S **)b;
    return (int)(serviceA->handle - serviceB->handle);
}

// 此函数需要特别注意，该函数的入参param所指向的内存，需要在本函数中管理
// 只有将此param加入到g_services中之后，才无需释放内存，否则均需要做释放内存的操作
void SsapServerAddService(void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[SSAP] add service param is null");
    SSAP_Service_S *ssapService = (SSAP_Service_S *)param;
    SsapAsyncProcessResult_S result = {
        .appId = ssapService->appId,
        .errorCode = NLSTK_ERRCODE_SUCCESS,
    };
    uint16_t handleNum = 1;
    // 此处不会发生溢出或者下溢，但表述形式有优化空间
    handleNum += (uint16_t)ssapService->references->size;
    handleNum += (uint16_t)ssapService->properties->size;
    handleNum += (uint16_t)ssapService->methods->size;
    handleNum += (uint16_t)ssapService->events->size;
    SSAP_HandleRange_S rangeResult = {0};
    if (!SSAP_HandleAllocate(handleNum, &rangeResult)) {
        NLSTK_LOG_ERROR("[SSAP] start service can not get avaliable handle");
        if (ssapService->callback != NULL) {
            ssapService->callback(NULL, 0, NULL);
        }
        FreeService(ssapService);
        ssapService = NULL;
        result.errorCode = NLSTK_ERRCODE_SSAP_NO_INVALIDE_HANLDE;
        SsapServerAppAddServiceExceptionCallBack(&result);
        return;
    }
    ssapService->handle = rangeResult.start;
    ssapService->endHandle = rangeResult.end;
    CP_LOG_INFO("[SSAP] start service service handle: %d, end handle: %d, uuid: %s.", rangeResult.start,
        rangeResult.end, SSAP_GET_ENC_UUID(&ssapService->uuid));
    uint16_t nextHandle = rangeResult.start + 1;
    // 当前仅有Property,method，后续还有ref、event
    SsapServerGenerateHandleForProperties(ssapService->properties, &nextHandle);
    SsapServerGenerateHandleForMethod(ssapService->methods, &nextHandle);

    if (!SDF_VectorEmplaceBack(g_services, ssapService)) {
        NLSTK_LOG_ERROR("[SSAP] start service start service failed");
        if (ssapService->callback != NULL) {
            ssapService->callback(NULL, 0, NULL);
        }
        FreeService(ssapService);
        result.errorCode = NLSTK_ERRCODE_SYS_ERROR;
        SsapServerAppAddServiceExceptionCallBack(&result);
        return ;
    } else {
        SDF_VectorSort(g_services, SSAPSortServicesByHandle);
        if (ssapService->callback != NULL) {
            ssapService->callback(NULL, 0, ssapService);
        }
        // 服务添加成功，通过startService回调通知给Service
        SsapServerAppStartServiceCallBack(ssapService);
    }
    SSAPS_UpdateHash(rangeResult.start, rangeResult.end);
}

void SSAPS_StartService(void)
{
    CP_CHECK_LOG_RETURN_VOID(g_tempService != NULL && g_tempService->references != NULL &&
                             g_tempService->properties != NULL && g_tempService->methods != NULL &&
                             g_tempService->events != NULL, "[SSAP] start service temp service is null");
    uint16_t handleNum = 1;
    // 此处不会发生溢出或者下溢，但表述形式有优化空间
    handleNum += (uint16_t)g_tempService->references->size;
    handleNum += (uint16_t)g_tempService->properties->size;
    handleNum += (uint16_t)g_tempService->methods->size;
    handleNum += (uint16_t)g_tempService->events->size;
    uint16_t start = SSAP_INVALID_HANDLE;
    uint16_t end = SSAP_INVALID_HANDLE;
    FindAvaliableHandle(handleNum, &start, &end);
    if (start == SSAP_INVALID_HANDLE || end == SSAP_INVALID_HANDLE) {
        CP_LOG_ERROR("[SSAP] start service can not get avaliable handle");
        if (g_tempService->callback != NULL) {
            g_tempService->callback(NULL, 0, NULL);
        }
        FreeService(g_tempService);
        g_tempService = NULL;
        return;
    }
    g_tempService->handle = start;
    g_tempService->endHandle = end;
    CP_LOG_INFO("[SSAP] start service service handle: %d, end handle: %d, uuid: %s.", start, end,
        SSAP_GET_ENC_UUID(&g_tempService->uuid));
    uint16_t nextHandle = start + 1;
    // 当前仅有Property，后续还有ref、method、event
    for (size_t i = 0; i < g_tempService->properties->size; i++) {
        SSAP_Property_S *property = SDF_VectorElementAt(g_tempService->properties, i);
        property->handle = nextHandle++;
        CP_LOG_INFO("[SSAP] start service property handle: %d, uuid: %s.", property->handle,
            SSAP_GET_ENC_UUID(&property->uuid));
    }
    if (!SDF_VectorEmplaceBack(g_services, g_tempService)) {
        CP_LOG_ERROR("[SSAP] start service start service failed");
        if (g_tempService->callback != NULL) {
            g_tempService->callback(NULL, 0, NULL);
        }
        FreeService(g_tempService);
        g_tempService = NULL;
    } else if (g_tempService->callback != NULL) {
        g_tempService->callback(NULL, 0, g_tempService);
    }
    SsapServerAppStartServiceCallBack(g_tempService);
    g_tempService = NULL;
}

static bool CompRemoveHandle(void *ptr, void *arg)
{
    if (ptr == NULL || arg == NULL) {
        return false;
    }
    SSAP_Service_S *service = (void *)ptr;
    SSAP_ParamRemoveService_S *removeParam = (SSAP_ParamRemoveService_S *)arg;
    return service->handle >= removeParam->startHandle && service->handle <= removeParam->endHandle;
}

void SSAPS_RemoveService(SSAP_ParamRemoveService_S *removeParam)
{
    size_t index = 0;
    size_t startIndex = 0;
    while (SDF_VectorFindFirstByStartIndex(g_services, CompRemoveHandle, removeParam, startIndex, &index)) {
        SSAP_Service_S *service = SDF_VectorElementAt(g_services, index);
        SSAP_HandleRange_S removeRange = {0};
        removeRange.start = service->handle;
        removeRange.end = service->endHandle;
        if (!SSAP_HandleRelease(removeRange)) {
            CP_LOG_ERROR("[SSAP] remove service handle release error");
        }
        CP_LOG_INFO("[SSAP] remove service service handle: %d, end handle: %d", removeRange.start, removeRange.end);
        SDF_VectorRemove(g_services, index);
        startIndex = index;
    }
    SSAPS_UpdateHash(removeParam->startHandle, removeParam->endHandle);
}

SDF_Vector_S *SSAPS_GetServices(void)
{
    return g_services;
}

uint32_t SSAPS_ServiceInit(void)
{
    SDF_Traits serviceTraits = {.dtor = FreeService};
    g_services = SDF_CreateVector(serviceTraits);
    if (g_services == NULL) {
        CP_LOG_ERROR("[SSAP] init service create vector failed");
        return SSAP_STACK_FAILED;
    }
    if (!SSAP_InitHandleAllocator(SSAP_HANDLE_DEFAULT_CAPACITY)) {
        SDF_DestroyVector(g_services);
        return SSAP_STACK_FAILED;
    }
    return SSAP_STACK_SUCCESS;
}

void SSAPS_ServiceDeInit(void)
{
    SDF_DestroyVector(g_services);
    g_services = NULL;
    if (g_tempService != NULL) {
        FreeService(g_tempService);
        g_tempService = NULL;
    }
    g_curHandle = SSAP_INIT_HANDLE;
    SSAP_DestroyHandleAllocator();
}

static void SSAPS_MergeStructureInfo(SSAP_StructureInfo_S *structureInfo, uint8_t *buf)
{
    CP_CHECK_LOG_RETURN_VOID(structureInfo != NULL && buf != NULL,
        "[SSAP] SSAPS_MergeStructureInfo check error");
    uint8_t *curBuf = buf;
 
    // 将 handle 写入字节数组（注意字节序，这里为小端）
    (void)memcpy_s(curBuf, sizeof(uint16_t), &structureInfo->handle, sizeof(uint16_t));
    curBuf += sizeof(uint16_t);
 
    // 将 serviceType 写入字节数组
    (void)memcpy_s(curBuf, sizeof(NLSTK_SsapItemType_E), &structureInfo->serviceType, sizeof(NLSTK_SsapItemType_E));
    curBuf += sizeof(NLSTK_SsapItemType_E);
 
    // 将 uuid 写入字节数组
    (void)memcpy_s(curBuf, SSAP_UUID128_LEN, structureInfo->uuid.uuid, SSAP_UUID128_LEN);
    curBuf += SSAP_UUID128_LEN;
 
    // 将 operation 写入字节数组
    (void)memcpy_s(curBuf, sizeof(NLSTK_SsapOperation_S), &structureInfo->operation, sizeof(NLSTK_SsapOperation_S));
    curBuf += sizeof(NLSTK_SsapOperation_S);
 
    // 将 descriptors 的内容写入字节数组
    if (structureInfo->descriptorCount > 0 && (structureInfo->descriptors != NULL)) {
        (void)memcpy_s(curBuf, structureInfo->descriptorCount, structureInfo->descriptors,
            structureInfo->descriptorCount);
        curBuf += structureInfo->descriptorCount;
    }
 
    return;
}
 
static void SSAPS_FillHashWithService(uint8_t *buf, SSAP_Service_S *service)
{
    uint8_t *curBuf = buf;
    SSAP_StructureInfo_S *structureInfo = (SSAP_StructureInfo_S *)SDF_MemZalloc(sizeof(SSAP_StructureInfo_S));
    CP_CHECK_LOG_RETURN_VOID(structureInfo != NULL, "[SSAP] serviceStructureInfo malloc falied");
    structureInfo->handle = service->handle;
    structureInfo->serviceType = service->serviceType;
    (void)memcpy_s(structureInfo->uuid.uuid, SSAP_UUID128_LEN, service->uuid.uuid, SSAP_UUID128_LEN);
    structureInfo->operation.operationValue = SSAP_SERVICE_OPERATION;
    structureInfo->descriptorCount = 0;
    structureInfo->descriptors = NULL;
    SSAPS_MergeStructureInfo(structureInfo, curBuf);
    SDF_MemFree(structureInfo);
    return;
}
 
static void SSAPS_FillHashWithProperty(uint8_t *buf, SSAP_Property_S *property)
{
    uint8_t *curBuf = buf;
    SSAP_StructureInfo_S *structureInfo = (SSAP_StructureInfo_S *)SDF_MemZalloc(sizeof(SSAP_StructureInfo_S));
    CP_CHECK_LOG_RETURN_VOID(structureInfo != NULL, "[SSAP] structureInfo malloc falied");
    if (property->descriptors->size > 0) {
        structureInfo->descriptors = (uint8_t *)SDF_MemZalloc(property->descriptors->size);
        if (structureInfo->descriptors == NULL) {
            CP_LOG_ERROR("[SSAP] structureInfo->descriptors malloc falied");
            SDF_MemFree(structureInfo);
            return;
        }
    }
    structureInfo->handle = property->handle;
    structureInfo->serviceType = ITEM_TYPE_STD_PROPERTY;
    (void)memcpy_s(structureInfo->uuid.uuid, SSAP_UUID128_LEN, property->uuid.uuid, SSAP_UUID128_LEN);
    structureInfo->operation = property->operation;
    structureInfo->descriptorCount = (uint8_t)property->descriptors->size;
    for (size_t p = 0; p < property->descriptors->size; p++) {
        SSAP_Descriptor_S *descriptor = SDF_VectorElementAt(property->descriptors, p);
        structureInfo->descriptors[p] = descriptor->type;
    }
    SSAPS_MergeStructureInfo(structureInfo, curBuf);
    if (property->descriptors->size > 0) {
        SDF_MemFree(structureInfo->descriptors);
    }
    SDF_MemFree(structureInfo);
    return;
}
 
static void SSAPS_ServiceChangeEventNtf(uint16_t startHandle, uint16_t endHandle)
{
    CP_LOG_INFO("[SSAP] SSAPS_ServiceChangeEventNtf enter");
    SDF_DListHead_S *ssapLinkList = SSAP_GetSsapLinkList();
    CP_CHECK_LOG_RETURN_VOID(!SDF_DListIsEmpty(ssapLinkList), "[SSAP] ssapLinkList is empty");
    SSAP_LinkNode_S *linkNode = NULL;
    SDF_DListElmForeach(linkNode, ssapLinkList, entry) {
        SSAP_Link_S *link = linkNode->link;
        // 对端是老版本，不支持通知服务变更
        if (CM_GetLogicLinkDeviceType(link->lcid) == CM_DEVTYPE_OLD) {
            continue;
        }
        SSAP_ValueInfo_S *valueInfo = (SSAP_ValueInfo_S *)SDF_MemZalloc(sizeof(SSAP_ValueInfo_S) +
            SSAP_SERVICE_CHANGE_EVENT_SIZE);
        CP_CHECK_LOG_RETURN_VOID(valueInfo != NULL, "[SSAP] valueInfo mem new failed");
        valueInfo->handle = SSAP_SERVICE_CHANGE_EVENT_HANDLE;
        valueInfo->value.len = SSAP_SERVICE_CHANGE_EVENT_SIZE;
        valueInfo->type = SSAP_EVENT_NTF;
        (void)memcpy_s(valueInfo->value.value, sizeof(uint16_t), &startHandle, sizeof(uint16_t));
        (void)memcpy_s(valueInfo->value.value + sizeof(uint16_t), sizeof(uint16_t), &endHandle, sizeof(uint16_t));
        (void)memcpy_s(&valueInfo->addr, sizeof(SLE_Addr_S), &link->addr, sizeof(SLE_Addr_S));
        SSAP_ProcessNormalTask(link, SSAPS_ValueNtf, valueInfo, SDF_MemFree);
    }
}
 
void SSAPS_UpdateHash(uint16_t startHandle, uint16_t endHandle)
{
    SDF_Vector_S *services = SSAPS_GetServices();
    CP_CHECK_LOG_RETURN_VOID(services != NULL, "[SSAP] SSAPS_UpdateHash cannot get services");
    uint32_t bufSize = 0;
    for (size_t i = 0; i < services->size; i++) {
        bufSize += SSAP_STRUCTURE_FIXED_SIZE;
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        for (size_t j = 0; j < service->properties->size; j++) {
            SSAP_Property_S *property = SDF_VectorElementAt(service->properties, j);
            bufSize += SSAP_STRUCTURE_FIXED_SIZE + (uint32_t)property->descriptors->size;
        }
    }
    uint8_t *buf = (uint8_t *)SDF_MemZalloc(bufSize);
    CP_CHECK_LOG_RETURN_VOID(buf != NULL, "[SSAP] SSAPS_UpdateHash cannot alloc buf");
    uint8_t *curBuf = buf;
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(services, i);
        SSAPS_FillHashWithService(curBuf, service);
        curBuf += SSAP_STRUCTURE_FIXED_SIZE;
        for (size_t j = 0; j < service->properties->size; j++) {
            SSAP_Property_S *property = SDF_VectorElementAt(service->properties, j);
            SSAPS_FillHashWithProperty(curBuf, property);
            curBuf += (SSAP_STRUCTURE_FIXED_SIZE + property->descriptors->size);
        }
    }
    NLSTK_VariableData_S valueLen = {.data = buf, .len = bufSize};
    NLSTK_SmSha256Hash shaHash = {0};
    if (!SmGenSha256(&valueLen, &shaHash)) {
        CP_LOG_ERROR("[SSAP] SSAPS_UpdateHash gen sha256 error");
        SDF_MemFree(buf);
        return;
    }
    // 截取低128位（后16字节）
    (void)memcpy_s(g_ssapHash, SSAP_HASH_REAL_SIZE, shaHash.sha256Hash + SSAP_HASH_REAL_SIZE, SSAP_HASH_REAL_SIZE);
    SSAPS_ServiceChangeEventNtf(startHandle, endHandle);
    SDF_MemFree(buf);
}

static bool ClientConfigAddrCompFunc(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    SSAP_ClientPropertyConfigDescriptor_S *config = (SSAP_ClientPropertyConfigDescriptor_S *)ptr;
    SLE_Addr_S *addr = (SLE_Addr_S *)args;
    return memcmp(&config->addr, addr, sizeof(SLE_Addr_S)) == 0;
}
 
static void CleanDescriptorCpcd(SSAP_Descriptor_S *descriptor, SLE_Addr_S *addr)
{
    if (descriptor->type == DESC_TYPE_CLIENT_CONFIG) {
        size_t index = 0;
        if (SDF_VectorFindFirst(descriptor->clientConfigs, ClientConfigAddrCompFunc, addr, &index)) {
            SDF_VectorRemove(descriptor->clientConfigs, index);
        }
    }
}
 
void SSAPS_CleanServiceCpcd(SLE_Addr_S *addr)
{
    if (g_services == NULL) {
        return;
    }
    for (size_t i = 0; i < g_services->size; i++) {
        SSAP_Service_S *service = SDF_VectorElementAt(g_services, i);
        for (size_t j = 0; j < service->properties->size; j++) {
            SSAP_Property_S *property = SDF_VectorElementAt(service->properties, j);
            for (size_t k = 0; k < property->descriptors->size; k++) {
                SSAP_Descriptor_S *descriptor = SDF_VectorElementAt(property->descriptors, k);
                CleanDescriptorCpcd(descriptor, addr);
            }
        }
    }
}