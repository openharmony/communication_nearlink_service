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
#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "sdf_addr.h"
#include "sdf_mem.h"
#include "icce_type.h"
#include "icce_common.h"
#include "icce_stm.h"
#include "nlstk_icce_client.h"
#include "icce_init.h"
#include "icce_client.h"

void IcceClientEnable(void)
{
    IcceDeviceInit();
}

void IcceClientDisable(void)
{
    IcceDeviceDeinit();
}

void IcceRegClientCbk(void *arg)
{
    NLSTK_LOG_INFO("[ICCE] enter IcceRegClientCbk");
    NLSTK_IcceClientCallBack_S *cbk = (NLSTK_IcceClientCallBack_S *)arg;
    NLSTK_CHECK_RETURN_VOID(cbk != NULL, "[ICCE] cbk is null");
    IcceSetUserCbk(cbk);
}

void IcceConnectTask(void *arg)
{
    NLSTK_LOG_INFO("[ICCE] enter IcceConnectTask");
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[ICCE] addr is null");
    IcceDevice_S *info = IcceFindDeviceByAddr(addr);
    if (info == NULL) {
        info = (IcceDevice_S *)SDF_MemZalloc(sizeof(IcceDevice_S));
        NLSTK_CHECK_RETURN_VOID(info != NULL, "[ICCE] info is null");
        (void)memcpy_s(&info->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
        info->appId = ICCE_INVALID_APPID;      // 初始值设置为-1，表示未注册
        info->state = ICCE_APPID_UNREGISTERED; // 初始状态设置为未注册
        if (!IcceAddDevice(info)) {
            NLSTK_LOG_ERROR("[ICCE] add device fail");
            SDF_MemFree(info);
            return;
        }
    }
    IcceStmParam param = { .what = ICCE_ON_USER_CONNECT, .extData = NULL};
    IcceClientStmCall(info, param); // 触发状态机
}

void IcceDisconnectTask(void *arg)
{
    NLSTK_LOG_INFO("[ICCE] enter IcceDisconnectTask");
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[ICCE] addr is null");
    IcceDevice_S *info = IcceFindDeviceByAddr(addr);
    NLSTK_CHECK_RETURN_VOID(info != NULL, "[ICCE] info is null");
    IcceStmParam param = { .what = ICCE_ON_USER_DISCONNECT, .extData = NULL};
    IcceClientStmCall(info, param);
}

void IcceReadIcceInfoTask(void *arg)
{
    NLSTK_LOG_INFO("[ICCE] enter IcceReadIcceInfoTask");
    IcceReadInfo_S *readInfo = (IcceReadInfo_S *)arg;
    NLSTK_CHECK_RETURN_VOID(readInfo != NULL && readInfo->port != NULL, "[ICCE] readInfo is null");
    IcceDevice_S *info = IcceFindDeviceByAddr(readInfo->addr);
    if (info != NULL && info->state == ICCE_DEV_CONNECTED) {
        *(readInfo->port) = (int32_t)info->devicesInfo.iccePort;
    } else {
        NLSTK_LOG_ERROR("[ICCE] can not find device or device unconnected");
        *(readInfo->port) = ICCE_INVALID_PORT;
    }
}

void IcceCountConnectedDevicesTask(void *arg)
{
    NLSTK_LOG_INFO("[ICCE] enter IcceCountConnectedDevicesTask");
    uint8_t *num = (uint8_t *)arg;
    NLSTK_CHECK_RETURN_VOID(num != NULL, "[ICCE] num is null");
    *num = IcceGetConnectedDeviceNum();
}