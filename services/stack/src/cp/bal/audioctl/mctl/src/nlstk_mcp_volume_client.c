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
#include "sdf_mem.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "nlstk_public_define.h"
#include "mcp_type.h"
#include "mcp_utils.h"
#include "mcp_volume_dev.h"
#include "mcp_volume_client.h"
#include "nlstk_mcp_volume_client.h"

uint32_t NLSTK_McpRegVolumeClientCbk(NLSTK_McpVolumeClientCallBack_S *clientCallback)
{
    NLSTK_CHECK_RETURN(clientCallback != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] callback is null");
    NLSTK_McpVolumeClientCallBack_S *cbk =
        (NLSTK_McpVolumeClientCallBack_S *)SDF_MemZalloc(sizeof(NLSTK_McpVolumeClientCallBack_S));
    NLSTK_CHECK_RETURN(cbk != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] cbk malloc fail");
    (void)memcpy_s(cbk, sizeof(NLSTK_McpVolumeClientCallBack_S),
        clientCallback, sizeof(NLSTK_McpVolumeClientCallBack_S));
    if (SchedulePostTask(McpRegVolumeClientCbk, cbk, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_McpVolumeConnect(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] addr is null");
    McpVolumeDevice_S *dev = McpVolumeFindDeviceByAddr(addr);
    if (dev == NULL) {
        NLSTK_SsapAppClientCb_S cb = {0};
        McpVolumeGetSsapCbk(&cb);
        int32_t appId = -1;
        uint32_t ret = NLSTK_SsapClientRegApp(&appId, &cb, addr);
        NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS && appId >= 0, ret, "[MCP] reg app fail");
        ret = McpVolumeAddDevice(appId, addr);
        if (ret != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_SsapClientDeregApp(appId);
            NLSTK_LOG_ERROR("[MCP] add device fail");
            return ret;
        }
    }
    SLE_Addr_S *copyAddr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(copyAddr != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] copyAddr malloc fail");
    (void)memcpy_s(copyAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(McpVolumeConnect, copyAddr, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_McpVolumeDisconnect(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] addr is null");
    SLE_Addr_S *copyAddr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(copyAddr != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] copyAddr malloc fail");
    (void)memcpy_s(copyAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(McpVolumeDisconnect, copyAddr, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_McpGetVolume(SLE_Addr_S *addr, NLSTK_McpVolumePropertyType_E property)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] addr is null");
    NLSTK_CHECK_RETURN(property == NLSTK_MCP_VOLUME_STATUS || property == NLSTK_MCP_STREAM_VOLUME_STATUS,
        NLSTK_ERRCODE_PARAM_ERR, "[MCP] property type error");
    NLSTK_McpGetVolumeParam_S *param =
        (NLSTK_McpGetVolumeParam_S *)SDF_MemZalloc(sizeof(NLSTK_McpGetVolumeParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] param malloc fail");
    param->property = property;
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(McpGetVolume, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_McpSetVolume(SLE_Addr_S *addr, uint8_t volume)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] addr is null");
    McpSetVolumeParam_S *param = (McpSetVolumeParam_S *)SDF_MemZalloc(sizeof(McpSetVolumeParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] param malloc fail");
    param->volume = volume;
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(McpSetVolume, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_McpSetStreamVolume(SLE_Addr_S *addr, NLSTK_McpSetStreamVolume_S *volumeArray, uint8_t num)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] addr is null");
    NLSTK_CHECK_RETURN(volumeArray != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] volume array is null");
    NLSTK_CHECK_RETURN(num != 0 && num <= MCP_STREAM_VOLUME_SET_MAX_NUM,
        NLSTK_ERRCODE_PARAM_ERR, "[MCP] volume set num error");
    McpSetStreamVolumeParam_S *param = (McpSetStreamVolumeParam_S *)SDF_MemZalloc(sizeof(McpSetStreamVolumeParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] param malloc fail");
    param->num = num;
    size_t len = sizeof(NLSTK_McpSetStreamVolume_S) * num;
    param->volumeArray = (NLSTK_McpSetStreamVolume_S *)SDF_MemZalloc(len);
    if (param->volumeArray == NULL) {
        NLSTK_LOG_ERROR("[MCP] volume array malloc fail");
        SDF_MemFree(param);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    (void)memcpy_s(param->volumeArray, len, volumeArray, len);
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(McpSetStreamVolume, param, McpFreeSetStreamVolumeParam) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

bool NLSTK_McpGetStreamVolumeAbility(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, false, "[MCP] addr is null");
    McpGetAbilityParam_S *param = (McpGetAbilityParam_S *)SDF_MemZalloc(sizeof(McpGetAbilityParam_S));
    NLSTK_CHECK_RETURN(param != NULL, false, "[MCP] param malloc fail");
    param->ability = false;
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTaskBlocked(McpGetStreamVolumeAbility, param, NULL, SEM_ALWAYS_WAIT) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MCP] SchedulePostTaskBlock failed");
        SDF_MemFree(param);
        return false;
    }
    bool ability = param->ability;
    SDF_MemFree(param);
    return ability;
}