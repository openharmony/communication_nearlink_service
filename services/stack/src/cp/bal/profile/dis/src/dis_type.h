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
#ifndef DIS_TYPE_H
#define DIS_TYPE_H

#include "nlstk_dis_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t disServiceHandle;
    uint16_t manufacturerInfoHandle;
    uint16_t modelInfoHandle;
    uint16_t serialInfoHandle;
    uint16_t hardwareInfoHandle;
    uint16_t firmwareInfoHandle;
    uint16_t softwareInfoHandle;
    uint16_t deviceLocalAliasHandle;
    uint16_t deviceAppearanceHandle;
} DisHandleInfo_S;

typedef struct {
    SLE_Addr_S addr;
    int32_t appId;
    uint8_t state;
    NLSTK_DeviceInfo_S devicesInfo;
    DisHandleInfo_S deviceHandleInfo;
} DisDeviceInfo_S;

typedef struct {
    SLE_Addr_S addr;
    uint32_t appearance;
} DisReadAppearance_S;

typedef struct {
    NLSTK_VariableData_S value;
    SLE_Addr_S addr;
    NLSTK_DisInfoType_E type;
} DisReadInfoByType_S;

#ifdef __cplusplus
}
#endif
#endif /* DIS_TYPE_H */