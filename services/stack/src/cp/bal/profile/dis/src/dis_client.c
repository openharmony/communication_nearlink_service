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
#include "nlstk_dis_client.h"
#include "dis_client.h"
#include "dis_stm.h"
#include "nlstk_log.h"
#include "sdf_mem.h"

void DisRegClientCbk(void *arg)
{
    NLSTK_DisClientCbk_S *cbk = (NLSTK_DisClientCbk_S *)arg;
    NLSTK_CHECK_RETURN_VOID(cbk != NULL, "[DIS] cbk is null");
    DisRegClientCbkIn(cbk);
}

void DisConnectTask(void *arg)
{
    NLSTK_LOG_DEBUG("[DIS] DisConnectTask START");
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[DIS] addr is null");
    DisClientInitIfEmpty(); // 如果g_peerDevicesDisInfo为空，则初始化
    DisDeviceInfo_S *info = DisFindDeviceInfo(addr);
    if (info == NULL) {
        NLSTK_LOG_DEBUG("[DIS] new connection, create new DisDeviceInfo_S");
        info = (DisDeviceInfo_S *)SDF_MemZalloc(sizeof(DisDeviceInfo_S));
        NLSTK_CHECK_RETURN_VOID(info != NULL, "[DIS] info is null");
        (void)memcpy_s(&info->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
        info->appId = -1;                     // 初始值设置为-1，表示未注册
        info->state = DIS_APPID_UNREGISTERED; // 初始状态设置为未注册
        info->devicesInfo.deviceAppearance = DIS_INVALID_APPEARANCE;
        NLSTK_CHECK_RETURN_VOID(DisClientAddInfoIntoVector(info), "[DIS] AddInfoIntoVector failed");
    }
    NLSTK_CHECK_RETURN_VOID(info != NULL, "[DIS] info is null");
    DisStmParam param = { .what = DIS_ON_USER_CONNECT, .extData = NULL};
    DisClientStmCall(info, param); // 触发状态机
}

void DisDisconnectTask(void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[DIS] addr is null");
    DisDeviceInfo_S *info = DisFindDeviceInfo(addr);
    NLSTK_CHECK_RETURN_VOID(info != NULL, "[DIS] info is null");
    if (info->state != DIS_DEV_CONNECTED) {
        NLSTK_LOG_INFO("[DIS] DIS is not in a state to be disconnected");
    }
    DisStmParam param = { .what = DIS_ON_USER_DISCONNECT, .extData = NULL};
    DisClientStmCall(info, param);
}

void DisCountConnectedDevicesTask(void *arg)
{
    uint8_t *num = (uint8_t *)arg;
    NLSTK_CHECK_RETURN_VOID(num != NULL, "[DIS] num is null");
    uint8_t cnt = DisCountConnectedDevices();
    *num = cnt;
}