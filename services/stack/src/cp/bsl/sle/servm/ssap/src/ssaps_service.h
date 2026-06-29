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
#ifndef SSAPS_SERVICE_H
#define SSAPS_SERVICE_H

#include "ssap_type.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct SSAP_ParamAddDescriptor {
    NLSTK_SsapPermission_S permission;
    NLSTK_SsapDescriptorType_E type;
    NLSTK_SsapOperation_S operation;
    SSAP_LengthValue_S val;
} SSAP_ParamAddDescriptor_S;

typedef struct SSAP_ParamAddProperty {
    NLSTK_SsapUuid_S uuid;
    NLSTK_SsapPermission_S permission;
    NLSTK_SsapOperation_S operation;
    SSAP_LengthValue_S val;
} SSAP_ParamAddProperty_S;

typedef struct SSAP_ParamAddService {
    NLSTK_SsapUuid_S uuid;
    NLSTK_SsapItemType_E serviceType;
    SSAP_ServiceRegisterCallback callback;
} SSAP_ParamAddService_S;

typedef struct SSAP_ParamRemoveService {
    uint16_t startHandle;
    uint16_t endHandle;
} SSAP_ParamRemoveService_S;

void SSAPS_CacheService(SSAP_ParamAddService_S *addService);
void SSAPS_CacheProperty(SSAP_ParamAddProperty_S *addProperty);
void SSAPS_CacheDescriptor(SSAP_ParamAddDescriptor_S *addDescriptor);
void SsapServerAddService(void *param);
void SSAPS_StartService(void);
void SSAPS_RemoveService(SSAP_ParamRemoveService_S *removeParam);

SDF_Vector_S *SSAPS_GetServices(void);

uint32_t SSAPS_ServiceInit(void);
void SSAPS_ServiceDeInit(void);

void SSAPS_UpdateHash(uint16_t startHandle, uint16_t endHandle);
void SSAPS_CleanServiceCpcd(SLE_Addr_S *addr);

#ifdef __cplusplus
}
#endif

#endif