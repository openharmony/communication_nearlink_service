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
#ifndef CCP_UTILS_H
#define CCP_UTILS_H

#include "ssap_type.h"
#include "ccp_type.h"
#include "nlstk_ccp_ccs_server.h"

#ifdef __cplusplus
extern "C" {
#endif

NLSTK_SsapUuid_S CcpConvertUuidToStru(uint16_t uuid);

uint16_t CcpConvertUuidTo16Bits(NLSTK_SsapUuid_S uuidStru);

uint16_t CcpGetPropertyUuidByType(NLSTK_CcpCcsPropertyType_E propertyType);

NLSTK_CcpCcsPropertyType_E CcpGetPropertyTypeByUuid(uint16_t uuid);

uint32_t CcpCopyCallControlInfo(NLSTK_CcpCallControlInfo_S *dest, NLSTK_CcpCallControlInfo_S *src);

void CcpFreeCallControlInfo(void *ptr);

void CcpFreeUpdateCcsPropertyParam(void *ptr);

NLSTK_VariableData_S *CcpCcsPropertyValueConvert(void *data, NLSTK_CcpCcsPropertyType_E type);

#ifdef __cplusplus
}
#endif

#endif