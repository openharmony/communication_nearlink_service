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

#ifndef NLSTK_REG_CBK_EXT_H
#define NLSTK_REG_CBK_EXT_H

#include "sdf_func.h"
#include "dli_cmd_struct.h"
#include "nlstk_api_type_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t (*CM_GetAddrByLcidPtr)(uint16_t lcid, SLE_Addr_S *addr);
typedef uint32_t (*CM_GetLcidByAddrPtr)(SLE_Addr_S *addr);
typedef uint32_t (*CM_SetRealACBSubratePtr)(NLSTK_SetAcbSubrateParam_S *param,
    uint32_t (*setSubrate)(DLI_ACBSubrateParam *dliParam));
typedef uint32_t (*CM_EnableRealConnHighPowerPtr)(NLSTK_EnableConnHighPowerParam_S *param,
    uint32_t (*enableConnHighPower)(DLI_EnableConnHighPowerParam *param));
typedef uint32_t (*CM_SetRemoteFeaturePtr)(const NLSTK_RemoteFeatureInfo_S *info);

typedef struct {
    CP_PostTaskPtr postTask;
    CP_PostTaskBlockedPtr postTaskBlocked;
    CP_TimerAddPtr timerAdd;
    CP_TimerDelPtr timerDel;
    CM_GetAddrByLcidPtr getAddr;
    CM_GetLcidByAddrPtr getLcid;
    CM_SetRealACBSubratePtr setRealACBSubrate;
    CM_EnableRealConnHighPowerPtr enableRealConnHighPower;
    CM_SetRemoteFeaturePtr setRemoteFeature;
} STK_Callback;

#ifdef __cplusplus
}
#endif
#endif