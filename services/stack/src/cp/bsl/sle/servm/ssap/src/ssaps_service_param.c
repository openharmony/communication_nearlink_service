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
#include "ssaps_service_param.h"
#include "sdf_mem.h"
#include "nlstk_log.h"
#include "securec.h"

void FreeService(void *val)
{
    if (val == NULL) {
        return;
    }
    SSAP_Service_S *service = (SSAP_Service_S *)val;
    SDF_DestroyVector(service->properties);
    SDF_DestroyVector(service->references);
    SDF_DestroyVector(service->methods);
    SDF_DestroyVector(service->events);
    SDF_DestroyVector(service->descriptors);
    SDF_MemFree(service);
}

void FreeProperty(void *val)
{
    if (val == NULL) {
        return;
    }
    SSAP_Property_S *property = (SSAP_Property_S *)val;
    SDF_DestroyVector(property->descriptors);
    SDF_MemFree(property->val);
    SDF_MemFree(property);
}

void FreeDescriptor(void *val)
{
    if (val == NULL) {
        return;
    }
    SSAP_Descriptor_S *descriptor = (SSAP_Descriptor_S *)val;
    SDF_DestroyVector(descriptor->clientConfigs);
    SDF_MemFree(descriptor->val);
    SDF_MemFree(descriptor);
}

void FreeClientConfig(void *val)
{
    if (val == NULL) {
        return;
    }
    SSAP_ClientPropertyConfigDescriptor_S *cpcd = (SSAP_ClientPropertyConfigDescriptor_S *)val;
    SDF_MemFree(cpcd->val);
    SDF_MemFree(cpcd);
}

static uint32_t SsapAllocServiceParamProfile(SSAP_Service_S *ssapService)
{
    NLSTK_CHECK_RETURN(ssapService != NULL, NLSTK_ERRCODE_POINTER_NULL, "input param is NULL");
    SDF_Traits propertyTraits = {.dtor = FreeProperty};
    ssapService->properties = SDF_CreateVector(propertyTraits);
    if (ssapService->properties == NULL) {
        goto FAIL;
    }
    SDF_Traits referenceTraits = {.dtor = SDF_MemFree};
    ssapService->references = SDF_CreateVector(referenceTraits);
    if (ssapService->references == NULL) {
        goto FAIL;
    }
    SDF_Traits methodTraits = {.dtor = SDF_MemFree};
    ssapService->methods = SDF_CreateVector(methodTraits);
    if (ssapService->methods == NULL) {
        goto FAIL;
    }
    SDF_Traits eventTraits = {.dtor = SDF_MemFree};
    ssapService->events = SDF_CreateVector(eventTraits);
    if (ssapService->events == NULL) {
        goto FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
FAIL:
    SDF_DestroyVector(ssapService->methods);
    SDF_DestroyVector(ssapService->references);
    SDF_DestroyVector(ssapService->properties);
    NLSTK_LOG_ERROR("[SSAP] cache service create vector failed");
    return NLSTK_ERRCODE_MALLOC_FAIL;
}

static uint32_t SsapAllocServiceCpyDescriptorParm(SDF_Vector_S *descriptorVector,
                                                  const NLSTK_SsapServiceDescriptorParam_S *descriptorParmArray,
                                                  uint32_t descriptorParmNum)
{
    NLSTK_CHECK_RETURN((descriptorVector != NULL && descriptorParmArray != NULL), NLSTK_ERRCODE_POINTER_NULL,
                         "input param is NULL");
    for (uint32_t index = 0; index < descriptorParmNum; index++) {
        // 当失败的时候，仅仅释放此for循环中的descriptor，不需要释放已经加入vector中的descriptor，
        // 失败时，由外部调用SDF_DestroyVector释放在vector中描述符的内存
        NLSTK_SsapServiceDescriptorParam_S *descriptorParm =
            (NLSTK_SsapServiceDescriptorParam_S *)&(descriptorParmArray[index]);
        SSAP_Descriptor_S *descriptor = (SSAP_Descriptor_S *)SDF_MemZalloc(sizeof(SSAP_Descriptor_S));
        if (descriptor == NULL) {
            return NLSTK_ERRCODE_MALLOC_FAIL;
        }
        descriptor->clientConfigs = SDF_CreateVector(MAKE_TRAITS(FreeClientConfig, NULL));
        if (descriptor->clientConfigs == NULL) {
            SDF_MemFree(descriptor);
            return NLSTK_ERRCODE_MALLOC_FAIL;
        }
        descriptor->type = descriptorParm->type;
        descriptor->operation = descriptorParm->operation;
        descriptor->permission = descriptorParm->permission;
        descriptor->val = (SSAP_LengthValue_S *)SDF_MemZalloc(sizeof(SSAP_LengthValue_S) + descriptorParm->val.len);
        if (descriptor->val == NULL) {
            SDF_DestroyVector(descriptor->clientConfigs);
            SDF_MemFree(descriptor);
            return NLSTK_ERRCODE_MALLOC_FAIL;
        }
        descriptor->val->len = descriptorParm->val.len;
        if (descriptorParm->val.len != 0) {
            (void)memcpy_s(descriptor->val->value, descriptorParm->val.len, descriptorParm->val.data,
                descriptorParm->val.len);
        }

        if (!SDF_VectorEmplaceBack(descriptorVector, descriptor)) {
            SDF_DestroyVector(descriptor->clientConfigs);
            SDF_MemFree(descriptor->val);
            SDF_MemFree(descriptor);
            return NLSTK_ERRCODE_MAX;
        }
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static uint32_t SsapAllocServiceCpyServiceParam(SSAP_Service_S *ssapService, const NLSTK_ServiceParam_S *service)
{
    NLSTK_CHECK_RETURN((ssapService != NULL && service != NULL), NLSTK_ERRCODE_POINTER_NULL, "input param is NULL");
    NLSTK_SsapServiceStatementParam_S *serviceStatement =
        (NLSTK_SsapServiceStatementParam_S *)&(service->serviceStatement);
    ssapService->serviceType = serviceStatement->serviceType;
    ssapService->protocol = SSAP_SERVICE_TYPE_GLE;
    (void)memcpy_s(&(ssapService->uuid), sizeof(NLSTK_SsapUuid_S), &(serviceStatement->uuid), sizeof(NLSTK_SsapUuid_S));
    if (serviceStatement->descriptorNum == 0) {
        return NLSTK_ERRCODE_SUCCESS;
    }
    NLSTK_CHECK_RETURN((serviceStatement->descriptors != NULL), NLSTK_ERRCODE_POINTER_NULL, "input param is NULL");
    SDF_Traits descriptorTraits = { .dtor = FreeDescriptor };
    ssapService->descriptors = SDF_CreateVector(descriptorTraits);
    NLSTK_CHECK_RETURN((ssapService->descriptors != NULL), NLSTK_ERRCODE_MALLOC_FAIL, "input param is NULL");
    uint32_t ret = SsapAllocServiceCpyDescriptorParm(ssapService->descriptors, serviceStatement->descriptors,
                                                     serviceStatement->descriptorNum);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        // 外部收到失败的错误码后会统一释放内存，这里不释放服务描述符vector
        return ret;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static uint32_t SsapAllocServiceCpyProperty(SDF_Vector_S *propertyVector,
    NLSTK_SsapServicePropertyParam_S *propertyParmArray, uint32_t propertyParmNum)
{
    NLSTK_CHECK_RETURN((propertyVector != NULL && propertyParmArray != NULL), NLSTK_ERRCODE_POINTER_NULL,
                         "input param is NULL");
    for (uint32_t index = 0; index < propertyParmNum; index++) {
        NLSTK_SsapServicePropertyParam_S *propertyParm = &(propertyParmArray[index]);

        // 为property分配内存
        SSAP_Property_S *property = (SSAP_Property_S *)SDF_MemZalloc(sizeof(SSAP_Property_S));
        NLSTK_CHECK_RETURN(property != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "property malloc failed");
        (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &propertyParm->uuid, sizeof(NLSTK_SsapUuid_S));
        property->operation.operationValue = propertyParm->operation.operationValue;
        property->permission.permissionValue = propertyParm->permission.permissionValue;

        // 为属性分配值的内存
        SSAP_LengthValue_S *val =
            (SSAP_LengthValue_S *)SDF_MemZalloc(sizeof(SSAP_LengthValue_S) + propertyParm->val.len);
        if (val == NULL) {
            NLSTK_LOG_ERROR("property malloc failed");
            FreeProperty(property);
            return NLSTK_ERRCODE_MALLOC_FAIL;
        }
        val->len = propertyParm->val.len;
        if (propertyParm->val.len != 0) {
            (void)memcpy_s(val->value, propertyParm->val.len, propertyParm->val.data, propertyParm->val.len);
        }
        property->val = val;

        SDF_Traits descriptorTraits = { .dtor = FreeDescriptor };
        property->descriptors = SDF_CreateVector(descriptorTraits);
        if (property->descriptors == NULL) {
            NLSTK_LOG_ERROR("descriptors malloc failed");
            FreeProperty(property);
            return NLSTK_ERRCODE_MALLOC_FAIL;
        }
        // 如果属性有描述符，则添加属性的描述符
        if (propertyParm->descriptorNum > 0 && propertyParm->descriptors != NULL) {
            // 在添加的时候，应该要校验下参数的正确性的，例如描述符的数量，描述符的类型等，这里暂时不做校验
            uint32_t ret = SsapAllocServiceCpyDescriptorParm(property->descriptors, propertyParm->descriptors,
                                                             propertyParm->descriptorNum);
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                FreeProperty(property);
                return ret;
            }
        }

        // 完成之后将属性放入vector中
        if (!SDF_VectorEmplaceBack(propertyVector, property)) {
            NLSTK_LOG_ERROR("[SSAP] cache property add vector failed");
            FreeProperty(property);
            return NLSTK_ERRCODE_MAX;
        }
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static uint32_t SsapAllocServiceCpyMethod(SDF_Vector_S *methodVector, NLSTK_SsapServiceMethodParam_S *methodParmArray,
                                          uint32_t methodParmNum)
{
    NLSTK_CHECK_RETURN((methodVector != NULL && methodParmArray != NULL), NLSTK_ERRCODE_POINTER_NULL,
                         "input param is NULL when cpy method param");
    for (uint32_t i = 0; i < methodParmNum; i++) {
        SSAP_Method_S *method = (SSAP_Method_S *)SDF_MemZalloc(sizeof(SSAP_Method_S));
        NLSTK_CHECK_RETURN(method != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "method malloc failed");
        NLSTK_SsapServiceMethodParam_S *methodParm = &(methodParmArray[i]);
        (void)memcpy_s(&method->uuid, sizeof(NLSTK_SsapUuid_S), &methodParm->uuid, sizeof(NLSTK_SsapUuid_S));
        method->permission.permissionValue = methodParm->permission.permissionValue;

        // 完成之后将属性放入vector中
        if (!SDF_VectorEmplaceBack(methodVector, method)) {
            NLSTK_LOG_ERROR("[SSAP] cache property add vector failed");
            SDF_MemFree(method);
            return NLSTK_ERRCODE_MAX;
        }
    }
    return NLSTK_ERRCODE_SUCCESS;
}

/* 申请用于缓存的ssap服务结构，内部子结构资源申请失败会在函数fail标签处统一释放内存 */
SSAP_Service_S *SsapAllocServiceParam(const NLSTK_ServiceParam_S *service)
{
    if (service == NULL) {
        return NULL;
    }
    SSAP_Service_S *ssapService = (SSAP_Service_S *)SDF_MemZalloc(sizeof(SSAP_Service_S));
    if (ssapService == NULL) {
        return NULL;
    }
    // 生成service的框架，申请vector的内存
    uint32_t ret = SsapAllocServiceParamProfile(ssapService);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("SsapAllocServiceParamProfile failed");
        SDF_MemFree(ssapService);
        ssapService = NULL;
        return NULL;
    }
    // 拷贝服务声明的参数
    ret = SsapAllocServiceCpyServiceParam(ssapService, service);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("SsapAllocServiceCpyServiceParam failed");
        goto FAIL;
    }
    // 拷贝服务引用--未实现
    // 拷贝服务属性的参数
    ret = SsapAllocServiceCpyProperty(ssapService->properties, service->property, service->servicePropertyNum);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("SsapAllocServiceCpyProperty failed");
        goto FAIL;
    }
    // 拷贝服务方法
    if (service->method != NULL && service->serviceMethodNum > 0) {
        ret = SsapAllocServiceCpyMethod(ssapService->methods, service->method, service->serviceMethodNum);
        if (ret != NLSTK_ERRCODE_SUCCESS) {
            goto FAIL;
        }
    }
    // 拷贝服务事件--未实现
    return ssapService;
FAIL:
    NLSTK_LOG_ERROR("copy the service param failed");
    FreeService(ssapService);   // 统一释放service的内存
    ssapService = NULL;
    return NULL;
}
