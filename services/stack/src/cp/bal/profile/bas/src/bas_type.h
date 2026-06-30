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
#ifndef BAS_TYPE_H
#define BAS_TYPE_H

#include "nlstk_bas_def.h"
#include "nlstk_ssap_app_client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    NLSTK_VariableData_S remainBatPctInfo;      // 电池剩余容量占比信息
    NLSTK_VariableData_S remainBatInfo;         // 电池剩余容量信息
    NLSTK_VariableData_S batCapacityInfo;       // 电池工作总容量信息
    NLSTK_VariableData_S batRateCapacityInfo;   // 电池额定总容量信息
    NLSTK_VariableData_S remainWorkingTime;     // 设备剩余工作时长信息
} BasPropertyInfo_S;

typedef struct {
    uint16_t basServiceHdl;
    uint16_t remainBatPctHdl;               // 电池剩余容量占比信息
    uint16_t remainBatHdl;                  // 电池剩余容量信息
    uint16_t batCapacityHdl;                // 电池工作总容量信息
    uint16_t batRateCapacityHdl;            // 电池额定总容量信息
    uint16_t remainWorkingTimeHdl;          // 设备剩余工作时长信息
} BasHandleInfo_S;

typedef struct {
    int32_t appId;
    SLE_Addr_S addr;
    uint16_t lastReadHandle;
    uint16_t lastSetNtfHandle;
    SDF_Vector_S *indexHandle;
    uint8_t state;
    BasHandleInfo_S devHandleInfo;
    BasPropertyInfo_S devDeviceInfo;
} BasDeviceInfo_S;

typedef struct {
    int what;
    void *extData;
} BadStmParam_S;

typedef struct {
    uint16_t serviceNum;
    NLSTK_SsapServ_S *service;
    NLSTK_SsapClientFreeFunc func;
} BasServiceMsg_S;

typedef struct {
    NLSTK_SsapClientReadPropertyInfo_S *property;
    NLSTK_Errcode_E ret;
} BasReadPropertyMsg_S;

typedef struct {
    NLSTK_SsapUuid_S *uuid;
    uint16_t handle;
    bool enable;
    NLSTK_Errcode_E ret;
} BasSetPropertyNtfMsg_S;

#ifdef __cplusplus
}
#endif
#endif /* BAS_TYPE_H */