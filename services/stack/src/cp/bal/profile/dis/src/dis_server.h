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
#ifndef DIS_SERVER_H
#define DIS_SERVER_H

#include "ssap_type.h"
#include "nlstk_ssap_app_server.h"
#include "nlstk_dis_def.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t DisAddServiceProperty(NLSTK_ServiceParam_S *service, NLSTK_DeviceInfo_S *basicInfo);

void DisSaveDevInfoTask(void *arg);

void DisUpdateDevNameTask(void *arg);

void DisDestroyInstTask(void *arg);

void DisDeepCopyDevInfo(NLSTK_DeviceInfo_S* destDevInfo, NLSTK_DeviceInfo_S* srcDevInfo);

void DisFreeDeviceInfoTask(void *arg);
#ifdef __cplusplus
}
#endif
#endif
