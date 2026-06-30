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
#ifndef ICCE_COMMON_H
#define ICCE_COMMON_H

#include "sdf_addr.h"
#include "icce_type.h"
#include "nlstk_icce_client.h"

#ifdef __cplusplus
extern "C" {
#endif

NLSTK_IcceClientCallBack_S *IcceGetUserCbk(void);

void IcceSetUserCbk(NLSTK_IcceClientCallBack_S *cbk);

void IcceDeviceInit(void);

void IcceDeviceDeinit(void);

bool IcceAddDevice(IcceDevice_S *dev);

void IcceRemoveDevice(SLE_Addr_S *addr);

IcceDevice_S *IcceFindDeviceByAppId(int32_t appId);

IcceDevice_S *IcceFindDeviceByAddr(SLE_Addr_S *addr);

uint8_t IcceGetConnectedDeviceNum(void);

#ifdef __cplusplus
}
#endif
#endif