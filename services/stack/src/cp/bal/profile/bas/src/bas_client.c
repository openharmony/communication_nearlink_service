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
#include "nlstk_bas_client.h"
#include "bas_client.h"
#include "bas_stm.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "nlstk_public_define.h"
#include "bas_common.h"
#include "bas_def.h"
#include "sdf_traits.h"
#include "sdf_vector.h"

void BasRegClientCbk(void *arg)
{
    BasClientCallBack_S *cbk = (BasClientCallBack_S *)arg;
    NLSTK_CHECK_RETURN_VOID(cbk != NULL, "[BAS] cbk is null");
    BasRegClientCbkIn(cbk);
}

NLSTK_Errcode_E BasEnable(void)
{
    NLSTK_LOG_INFO("[BAS] enter BasEnable");
    NLSTK_CHECK_RETURN(BasClientInit(), NLSTK_ERRCODE_FAIL, "[BAS] device init fail");
    return NLSTK_ERRCODE_SUCCESS;
}

void BasDisable(void)
{
    NLSTK_LOG_INFO("[BAS] enter BasDisable");
    BasClientDeinit();
}

void BasConnectTask(void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[BAS] addr is null");
    BasClientInitIfEmpty();
    BasDeviceInfo_S *info = BasFindDeviceInfo(addr);
    if (info == NULL) {
        NLSTK_LOG_INFO("[BAS] new connection, create new BasDeviceInfo_S");
        info = (BasDeviceInfo_S *)SDF_MemZalloc(sizeof(BasDeviceInfo_S));
        NLSTK_CHECK_RETURN_VOID(info != NULL, "[BAS] info is null");
        (void)memcpy_s(&info->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
        info->appId = -1;                     // 初始值设置为-1，表示未注册
        info->state = BAS_APPID_UNREGISTERED; // 初始状态设置为未注册
        SDF_Traits handleTraits = {.dtor = SDF_MemFree};
        info->indexHandle = SDF_CreateVector(handleTraits);
        if (info->indexHandle == NULL) {
            NLSTK_LOG_ERROR("[BAS] handle vector create fail");
            BasClearDeviceData(info);
            return;
        }
        NLSTK_CHECK_RETURN_VOID(BasClientAddInfoIntoVector(info), "[BAS] AddInfoIntoVector failed");
    }
    NLSTK_CHECK_RETURN_VOID(info != NULL, "[BAS] info is null");
    BasStmParam_S param = { .what = BAS_ON_USER_CONNECT, .extData = NULL};
    BasClientStmCall(info, param); // 触发状态机
}

void BasDisconnectTask(void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[BAS] addr is null");
    BasDeviceInfo_S *info = BasFindDeviceInfo(addr);
    NLSTK_CHECK_RETURN_VOID(info != NULL, "[BAS] info is null");
    if (info->state != BAS_DEVICE_CONNECTED) {
        NLSTK_LOG_INFO("[BAS] BAS is not in a state to be disconnected");
    }
    BasStmParam_S param = { .what = BAS_ON_USER_DISCONNECT, .extData = NULL};
    BasClientStmCall(info, param);
}

void BasCountConnectedDevicesTask(void *arg)
{
    uint8_t *num = (uint8_t *)arg;
    NLSTK_CHECK_RETURN_VOID(num != NULL, "[BAS] num is null");
    uint8_t cnt = BasCountConnectedDevices();
    *num = cnt;
}

void BasGetBatteryLevelTask(void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[BAS] addr is null");
    BasDeviceInfo_S *info = BasFindDeviceInfo(addr);
    NLSTK_CHECK_RETURN_VOID(info != NULL, "[BAS] info is null");
    NLSTK_SsapClientReadProperty(info->appId, info->lastReadHandle);
}