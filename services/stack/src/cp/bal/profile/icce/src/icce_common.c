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
#include "nlstk_log.h"
#include "sdf_vector.h"
#include "sdf_mem.h"
#include "icce_utils.h"
#include "icce_stm.h"
#include "icce_common.h"

static SDF_Vector_S *g_icceDevice = NULL;
static NLSTK_IcceClientCallBack_S g_icceClientCbk = {0};

NLSTK_IcceClientCallBack_S *IcceGetUserCbk(void)
{
    return &g_icceClientCbk;
}

void IcceSetUserCbk(NLSTK_IcceClientCallBack_S *cbk)
{
    (void)memcpy_s(&g_icceClientCbk, sizeof(NLSTK_IcceClientCallBack_S), cbk, sizeof(NLSTK_IcceClientCallBack_S));
}

void IcceDeviceInit(void)
{
    if (g_icceDevice != NULL) {
        SDF_DestroyVector(g_icceDevice);
        g_icceDevice = NULL;
    }
    SDF_Traits icceTraits = {.dtor = SDF_MemFree};
    g_icceDevice = SDF_CreateVector(icceTraits);
}

void IcceDeviceDeinit(void)
{
    SDF_DestroyVector(g_icceDevice);
    g_icceDevice = NULL;
}

bool IcceAddDevice(IcceDevice_S *dev)
{
    NLSTK_CHECK_RETURN(dev != NULL, false, "[ICCE] dev is null");
    if (!SDF_VectorEmplaceBack(g_icceDevice, dev)) {
        NLSTK_LOG_ERROR("[HID] emplace back fail");
        return false;
    }
    return true;
}

void IcceRemoveDevice(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[ICCE] addr is null");
    size_t index = 0;
    NLSTK_CHECK_RETURN_VOID(SDF_VectorFindFirst(g_icceDevice, IcceCompAddr, addr, &index), "[ICCE] addr not found");
    SDF_VectorRemove(g_icceDevice, index);
}

IcceDevice_S *IcceFindDeviceByAppId(int32_t appId)
{
    size_t index = 0;
    NLSTK_CHECK_RETURN(SDF_VectorFindFirst(g_icceDevice, IcceCompAppId, &appId, &index),
        NULL, "[ICCE] appId not found");
    IcceDevice_S *dev = (IcceDevice_S *)SDF_VectorElementAt(g_icceDevice, index);
    return dev;
}

IcceDevice_S *IcceFindDeviceByAddr(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NULL, "[HID] addr is null");
    size_t index = 0;
    NLSTK_CHECK_RETURN(SDF_VectorFindFirst(g_icceDevice, IcceCompAddr, addr, &index), NULL, "[ICCE] addr not found");
    IcceDevice_S *dev = (IcceDevice_S *)SDF_VectorElementAt(g_icceDevice, index);
    return dev;
}

uint8_t IcceGetConnectedDeviceNum(void)
{
    if (g_icceDevice == NULL) {
        return 0;
    }
    uint8_t num = 0;
    // 需要对设备数量规格做约束，当前认为num不会溢出
    for (size_t i = 0; i < g_icceDevice->size; ++i) {
        IcceDevice_S *info = (IcceDevice_S *)SDF_VectorElementAt(g_icceDevice, i);
        NLSTK_CHECK_RETURN(info != NULL, 0, "[ICCE] info is null");
        if (info->state == ICCE_DEV_CONNECTED || info->state == ICCE_SERVICE_FOUND ||
            info->state == ICCE_LINK_CONNECTED || info->state == ICCE_APPID_REGISTERING) {
            num++;
        }
    }
    return num;
}