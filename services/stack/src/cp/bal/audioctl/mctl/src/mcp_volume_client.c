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
#include "securec.h"
#include "sdf_mem.h"
#include "sdf_addr.h"
#include "sdf_vector.h"
#include "nlstk_log.h"
#include "ssap_common.h"
#include "ssap_type.h"
#include "ssap_pkt.h"
#include "ssapc_app.h"
#include "nlstk_ssap_app_client.h"
#include "mcp_type.h"
#include "mcp_utils.h"
#include "mcp_volume_dev.h"
#include "nlstk_mcp_volume.h"
#include "nlstk_mcp_volume_client.h"
#include "mcp_volume_client.h"

static NLSTK_McpVolumeClientCallBack_S g_mcpVolumeClientCbk = {0};

static void McpVolumeBuildServiceCache(McpVolumeDevice_S *dev, NLSTK_SsapPrty_S *property)
{
    uint16_t uuid = McpConvertUuidTo16Bits(property->uuid);
    NLSTK_LOG_INFO("[MCP] volume item handle: %u, uuid: 0x%04x", property->handle, uuid);
    switch (uuid) {
        case MCP_VOLUME_STATUS_UUID:
            dev->volumeHandle = property->handle;
            break;
        case MCP_CHANNEL_VOLUME_STATUS_UUID:
            dev->channelVolumeHandle = property->handle;
            break;
        case MCP_STREAM_VOLUME_STATUS_UUID:
            dev->streamVolumeHandle = property->handle;
            break;
        case MCP_VOLUME_CONTROL_POINT_UUID:
            dev->volumeControlHandle = property->handle;
            break;
        case MCP_VOLUME_SYNC_EVENT_UUID:
            dev->volumeSyncEventHandle = property->handle;
            break;
        default:
            break;
    }
}

static uint32_t McpVolumeGetService(int32_t appId, McpVolumeDevice_S *dev)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpVolumeGetService");
    NLSTK_SsapServObtain_S param = {0};
    NLSTK_SsapUuid_S uuid = McpConvertUuidToStru(MCP_VOLUME_SERVICE_UUID);
    NLSTK_SsapServ_S *services = NULL;
    uint16_t servNum = 0;
    NLSTK_SsapClientFreeFunc freeFunc = NULL;
    param.appId = appId;
    param.uuid = &uuid;
    param.serv = &services;
    param.num = &servNum;
    param.func = &freeFunc;
    SsapcAppGetServ(&param);
    NLSTK_CHECK_RETURN(services != NULL && servNum > 0, NLSTK_ERRCODE_NO_RECORD, "[MCP] get no service");
    NLSTK_SsapServ_S *service = &services[0];
    for (uint16_t i = 0; i < service->propertyNum; i++) {
        McpVolumeBuildServiceCache(dev, &service->properties[i]);
    }
    for (uint16_t i = 0; i < service->methodNum; i++) {
        McpVolumeBuildServiceCache(dev, &service->methods[i]);
    }
    for (uint16_t i = 0; i < service->eventNum; i++) {
        McpVolumeBuildServiceCache(dev, &service->events[i]);
    }
    if (freeFunc != NULL) {
        freeFunc(services, servNum);
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void McpVolumeConnStateChangeCbk(int32_t appId, uint8_t state, NLSTK_Errcode_E ret, int32_t reason)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpVolumeConnStateChangeCbk");
    McpVolumeDevice_S *dev = McpVolumeFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] find dev is null");
    if (state == SSAP_CONNECT_STATE_CONNECTED && dev->connState == NLSTK_MCP_VOLUME_CONNECTING) {
        if (McpVolumeGetService(appId, dev) != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[MCP] get service fail");
            NLSTK_SsapClientDisconnect(dev->appId);
            return;
        }
        if (NLSTK_SsapClientSetPropertyNtf(appId, dev->volumeHandle, true) != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[MCP] set cpcd ntf fail");
            NLSTK_SsapClientDisconnect(dev->appId);
            return;
        }
        if (NLSTK_SsapClientReadProperty(appId, dev->volumeHandle) != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[MCP] read volume fail");
            NLSTK_SsapClientDisconnect(dev->appId);
            return;
        }
    } else if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
        NLSTK_McpVolumeState_E preState = dev->connState;
        dev->connState = NLSTK_MCP_VOLUME_DISCONNECTED;
        if (g_mcpVolumeClientCbk.stateChange != NULL && preState != NLSTK_MCP_VOLUME_DISCONNECTED) {
            g_mcpVolumeClientCbk.stateChange(&dev->addr, NLSTK_MCP_VOLUME_DISCONNECTED, preState);
        }
        SsapcAppRegParam_S param = {.appId = appId};
        SsapcAppDeregister(&param);
        if (McpVolumeRemoveDevice(appId) != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[MCP] dereg app fail");
        }
    }
}

static void McpCheckSendNextVolumeReq(McpVolumeDevice_S *dev)
{
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] dev is null");
    if (dev->volumeReq->size > 0) {
        // 音量请求队列不为空，则取出队首元素继续发送音量控制请求，update不变依然为false
        NLSTK_VariableData_S *value = SDF_VectorElementAt(dev->volumeReq, 0);
        // 请求参数中的变更标记置为最新
        if (value->data[MCP_VOLUME_OPCODE_POS] == MCP_SET_VOLUME_OPCODE) {
            value->data[MCP_VOLUME_SET_CHANGE_ID_POS] = dev->volumeChangeId;
        } else {
            value->data[MCP_VOLUME_CHANGE_ID_POS] = dev->volumeChangeId;
        }
        NLSTK_LOG_INFO("[MCP] send next volume req");
        if (NLSTK_SsapClientCallMethod(dev->appId, dev->volumeControlHandle, value, false) != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[MCP] method call fail");
        }
        SDF_VectorRemove(dev->volumeReq, 0);
    } else {
        // 音量请求队列为空，update置为true，下一次用户调用音量控制可以直接发送请求
        dev->update = true;
    }
}

static void McpCheckSendNextStreamVolumeReq(McpVolumeDevice_S *dev)
{
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] dev is null");
    if (dev->streamVolumeReq->size > 0) {
        // 音频流音量请求队列不为空，则取出队首元素继续发送音量控制请求，update不变依然为false
        NLSTK_VariableData_S *value = SDF_VectorElementAt(dev->streamVolumeReq, 0);
        // 请求参数中的变更标记置为最新
        value->data[value->len - 1] = dev->streamVolumeChangeId;
        NLSTK_LOG_INFO("[MCP] send next stream volume req");
        if (NLSTK_SsapClientCallMethod(dev->appId, dev->volumeControlHandle, value, false) != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[MCP] method call fail");
        }
        SDF_VectorRemove(dev->streamVolumeReq, 0);
    } else {
        // 音频流音量请求队列为空，update置为true，下一次用户调用音频流音量控制可以直接发送请求
        dev->updateV2 = true;
    }
}

static void McpHandleGetVolumeRsp(McpVolumeDevice_S *dev, NLSTK_SsapClientReadPropertyInfo_S *property,
    NLSTK_Errcode_E ret)
{
    if (ret != SSAP_ERRCODE_SUCCESS) {
        if (dev->connState == NLSTK_MCP_VOLUME_CONNECTING) {
            NLSTK_SsapClientDisconnect(dev->appId);
        } else if (dev->connState == NLSTK_MCP_VOLUME_CONNECTED && g_mcpVolumeClientCbk.getVolumeRsp != NULL) {
            g_mcpVolumeClientCbk.getVolumeRsp(&dev->addr, NLSTK_MCP_VOLUME_STATUS, ret, NULL);
        }
        return;
    }
    NLSTK_CHECK_RETURN_VOID(property->value.len == MCP_VOLUME_STATUS_LEN, "[MCP] volume len error");
    NLSTK_McpVolumeStatus_S volume = {0};
    uint8_t *data = property->value.data;
    PARSE_TO_UINT8(volume.volume, data);
    PARSE_TO_UINT8(volume.mute, data);
    PARSE_TO_UINT8(volume.additionalInfo, data);
    PARSE_TO_UINT8(dev->volumeChangeId, data);
    NLSTK_LOG_INFO("[MCP] read volume = %u, changeId = %u", volume.volume, dev->volumeChangeId);

    if (dev->connState == NLSTK_MCP_VOLUME_CONNECTING) {
        dev->update = true;
        if (dev->streamVolumeHandle == 0) {
            dev->connState = NLSTK_MCP_VOLUME_CONNECTED;
            if (g_mcpVolumeClientCbk.stateChange != NULL) {
                g_mcpVolumeClientCbk.stateChange(&dev->addr, NLSTK_MCP_VOLUME_CONNECTED, NLSTK_MCP_VOLUME_CONNECTING);
            }
            return;
        }
        NLSTK_LOG_DEBUG("[MCP] server support stream volume, continue connecting");
        if (NLSTK_SsapClientSetPropertyNtf(dev->appId, dev->streamVolumeHandle, true) != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[MCP] set stream volume cpcd ntf fail");
            NLSTK_SsapClientDisconnect(dev->appId);
            return;
        }
        if (NLSTK_SsapClientReadProperty(dev->appId, dev->streamVolumeHandle) != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[MCP] read stream volume fail");
            NLSTK_SsapClientDisconnect(dev->appId);
            return;
        }
    } else if (dev->connState == NLSTK_MCP_VOLUME_CONNECTED) {
        McpCheckSendNextVolumeReq(dev);
        if (g_mcpVolumeClientCbk.getVolumeRsp != NULL) {
            g_mcpVolumeClientCbk.getVolumeRsp(&dev->addr, NLSTK_MCP_VOLUME_STATUS, ret, &volume);
        }
    }
}

static void UpdateMcpVolumeClient(McpVolumeDevice_S *dev, NLSTK_Errcode_E ret,
    NLSTK_McpStreamVolumeStatus_S streamVolume)
{
    if (dev->connState == NLSTK_MCP_VOLUME_CONNECTING) {
        dev->connState = NLSTK_MCP_VOLUME_CONNECTED;
        dev->updateV2 = true;
        if (g_mcpVolumeClientCbk.stateChange != NULL) {
            g_mcpVolumeClientCbk.stateChange(&dev->addr, NLSTK_MCP_VOLUME_CONNECTED, NLSTK_MCP_VOLUME_CONNECTING);
        }
    } else if (dev->connState == NLSTK_MCP_VOLUME_CONNECTED) {
        McpCheckSendNextStreamVolumeReq(dev);
        if (g_mcpVolumeClientCbk.getVolumeRsp != NULL) {
            g_mcpVolumeClientCbk.getVolumeRsp(&dev->addr, NLSTK_MCP_STREAM_VOLUME_STATUS, ret, &streamVolume);
        }
    }
}

static void McpHandleGetStreamVolumeRsp(McpVolumeDevice_S *dev, NLSTK_SsapClientReadPropertyInfo_S *property,
                                        NLSTK_Errcode_E ret)
{
    if (ret != SSAP_ERRCODE_SUCCESS) {
        if (dev->connState == NLSTK_MCP_VOLUME_CONNECTING) {
            NLSTK_SsapClientDisconnect(dev->appId);
        } else if (dev->connState == NLSTK_MCP_VOLUME_CONNECTED && g_mcpVolumeClientCbk.getVolumeRsp != NULL) {
            g_mcpVolumeClientCbk.getVolumeRsp(&dev->addr, NLSTK_MCP_STREAM_VOLUME_STATUS, ret, NULL);
        }
        return;
    }
    uint8_t *data = property->value.data;
    uint8_t num = 0;
    PARSE_TO_UINT8(num, data);
    NLSTK_CHECK_RETURN_VOID(property->value.len == num * MCP_STREAM_VOLUME_EXTRA_LEN + MCP_STREAM_VOLUME_BASE_LEN,
        "[MCP] stream volume len error");
    NLSTK_McpStreamVolumeStatus_S streamVolume = {0};
    SDF_CleanVector(dev->streamVolumeStatus);

    // 用于拼接日志的缓冲区
    char logBuffer[MCP_VOLUME_LOG_INFO_LEN] = {0};
    int offset = 0;
    offset += snprintf_s(logBuffer + offset, sizeof(logBuffer) - offset, sizeof(logBuffer) - offset - 1, "");
    for (int i = 0; i < num; i++) {
        McpStreamVolumeStatus_S *vol = (McpStreamVolumeStatus_S *)SDF_MemZalloc(sizeof(McpStreamVolumeStatus_S));
        NLSTK_CHECK_RETURN_VOID(vol != NULL, "[MCP] stream volume status malloc fail");
        PARSE_TO_UINT8(vol->accessPoint, data);
        PARSE_TO_UINT8(vol->volume, data);
        PARSE_TO_UINT8(vol->additionalInfo, data);
        if (vol->accessPoint == dev->mediaPoint) {
            streamVolume.mediaVolume = vol->volume;
            streamVolume.mediaInfo = vol->additionalInfo;
        } else if (vol->accessPoint == dev->callPoint) {
            streamVolume.callVolume = vol->volume;
            streamVolume.callInfo = vol->additionalInfo;
        }

        // 拼接音量信息
        if (offset < sizeof(logBuffer) - MCP_VOLUME_LOG_PER_ITEM_LEN) {
            offset += snprintf_s(logBuffer + offset, sizeof(logBuffer) - offset, sizeof(logBuffer) - offset - 1,
                "volume=%u, ap=%u;", vol->volume, vol->accessPoint);
        }
        if (!SDF_VectorEmplaceBack(dev->streamVolumeStatus, vol)) {
            SDF_MemFree(vol);
        }
    }
    PARSE_TO_UINT8(dev->streamVolumeChangeId, data);
    NLSTK_LOG_INFO("[MCP] read stream %s changeId=%u", logBuffer, dev->streamVolumeChangeId);
    UpdateMcpVolumeClient(dev, ret, streamVolume);
}

static void McpGetVolumeCbk(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpGetVolumeCbk");
    NLSTK_CHECK_RETURN_VOID(property != NULL && property->value.data != NULL && property->value.len != 0,
        "[MCP] property is null");
    McpVolumeDevice_S *dev = McpVolumeFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] find dev is null");
    if (property->handle == dev->volumeHandle) {
        McpHandleGetVolumeRsp(dev, property, ret);
    } else if (property->handle == dev->streamVolumeHandle) {
        McpHandleGetStreamVolumeRsp(dev, property, ret);
    } else {
        NLSTK_LOG_ERROR("[MCP] property type error");
    }
}

static void McpHandleVolumeNtf(McpVolumeDevice_S *dev, NLSTK_SsapClientReadPropertyInfo_S *property)
{
    NLSTK_CHECK_RETURN_VOID(property->value.len == MCP_VOLUME_STATUS_LEN, "[MCP] property len error");
    uint8_t *data = property->value.data;
    NLSTK_McpVolumeStatus_S volume = {0};
    PARSE_TO_UINT8(volume.volume, data);
    PARSE_TO_UINT8(volume.mute, data);
    PARSE_TO_UINT8(volume.additionalInfo, data);
    PARSE_TO_UINT8(dev->volumeChangeId, data);
    NLSTK_LOG_INFO("[MCP] update volume = %u, changeId = %u", volume.volume, dev->volumeChangeId);
    McpCheckSendNextVolumeReq(dev);
    McpVolumeDelTimer(dev);
    if (g_mcpVolumeClientCbk.notifyVolumeChange != NULL) {
        g_mcpVolumeClientCbk.notifyVolumeChange(&dev->addr, NLSTK_MCP_VOLUME_STATUS, &volume);
    }
}

static void McpHandleStreamVolumeNtf(McpVolumeDevice_S *dev, NLSTK_SsapClientReadPropertyInfo_S *property)
{
    uint8_t *data = property->value.data;
    uint8_t num = 0;
    PARSE_TO_UINT8(num, data);
    NLSTK_CHECK_RETURN_VOID(property->value.len == num * MCP_STREAM_VOLUME_EXTRA_LEN + MCP_STREAM_VOLUME_BASE_LEN,
        "[MCP] stream volume len error");
    NLSTK_McpStreamVolumeStatus_S streamVolume = {0};
    SDF_CleanVector(dev->streamVolumeStatus);

    // 用于拼接日志的缓冲区
    char logBuffer[MCP_VOLUME_LOG_INFO_LEN] = {0};
    int offset = 0;
    offset += snprintf_s(logBuffer + offset, sizeof(logBuffer) - offset, sizeof(logBuffer) - offset - 1, "");
    for (int i = 0; i < num; i++) {
        McpStreamVolumeStatus_S *vol = (McpStreamVolumeStatus_S *)SDF_MemZalloc(sizeof(McpStreamVolumeStatus_S));
        NLSTK_CHECK_RETURN_VOID(vol != NULL, "[MCP] stream volume status malloc fail");
        PARSE_TO_UINT8(vol->accessPoint, data);
        PARSE_TO_UINT8(vol->volume, data);
        PARSE_TO_UINT8(vol->additionalInfo, data);
        if (vol->accessPoint == dev->mediaPoint) {
            streamVolume.mediaVolume = vol->volume;
            streamVolume.mediaInfo = vol->additionalInfo;
        } else if (vol->accessPoint == dev->callPoint) {
            streamVolume.callVolume = vol->volume;
            streamVolume.callInfo = vol->additionalInfo;
        }

         // 拼接音量信息
        if (offset < sizeof(logBuffer) - MCP_VOLUME_LOG_PER_ITEM_LEN) {
            offset += snprintf_s(logBuffer + offset, sizeof(logBuffer) - offset, sizeof(logBuffer) - offset - 1,
                "volume=%u, ap=%u;", vol->volume, vol->accessPoint);
        }
        if (!SDF_VectorEmplaceBack(dev->streamVolumeStatus, vol)) {
            SDF_MemFree(vol);
        }
    }
    PARSE_TO_UINT8(dev->streamVolumeChangeId, data);
    NLSTK_LOG_INFO("[MCP] update stream %s changeId=%u", logBuffer, dev->streamVolumeChangeId);

    McpCheckSendNextStreamVolumeReq(dev);
    McpStreamVolumeDelTimer(dev);
    if (g_mcpVolumeClientCbk.notifyVolumeChange != NULL) {
        g_mcpVolumeClientCbk.notifyVolumeChange(&dev->addr, NLSTK_MCP_STREAM_VOLUME_STATUS, &streamVolume);
    }
}

static void McpVolumePropertyNtfCbk(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpVolumePropertyNtf");
    NLSTK_CHECK_RETURN_VOID(property != NULL && property->value.data, "[MCP] property is null");
    McpVolumeDevice_S *dev = McpVolumeFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] find dev is null");
    if (property->handle == dev->volumeHandle) {
        McpHandleVolumeNtf(dev, property);
    } else if (property->handle == dev->streamVolumeHandle) {
        McpHandleStreamVolumeNtf(dev, property);
    } else {
        NLSTK_LOG_ERROR("[MCP] property handle error");
    }
}

static void McpVolumeCallMethodCbk(int32_t appId, NLSTK_SsapClientCallMethodResult_S *result, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpVolumeCallMethodCbk");
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_ERRCODE_SUCCESS, "[MCP] ret error");
    NLSTK_CHECK_RETURN_VOID(result != NULL && result->value.data != NULL, "[MCP] result is null");
    NLSTK_CHECK_RETURN_VOID(result->value.len >= MCP_VOLUME_CALL_RES_MIN_LEN, "[MCP] result value len error");
    McpVolumeDevice_S *dev = McpVolumeFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] find dev is null");
    uint8_t *data = result->value.data;
    uint8_t opCode = 0;
    uint8_t resCode = 0;
    PARSE_TO_UINT8(opCode, data);
    PARSE_TO_UINT8(resCode, data);
    NLSTK_LOG_INFO("[MCP] method rsp opCode = %u, resCode = %u", opCode, resCode);
    if (opCode == MCP_SET_VOLUME_OPCODE) {
        if (resCode == 0) {
            // 若响应的结果码成功，预期接收音量状态最新通知，如超时未收到则主动读取一次音量状态，触发下一次请求
            McpVolumeStartTimer(dev);
        } else {
            // 若响应的结果码非成功，则主动读取一次音量状态，触发下一次请求
            NLSTK_LOG_INFO("[MCP] method rsp unsuccess, read volume property");
            if (NLSTK_SsapClientReadProperty(dev->appId, dev->volumeHandle) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[MCP] read property fail");
            }
        }
    } else if (opCode == MCP_SET_STREAM_VOLUME_OPCODE) {
        if (resCode == 0) {
            McpStreamVolumeStartTimer(dev);
        } else {
            NLSTK_LOG_INFO("[MCP] method rsp unsuccess, read stream volume property");
            if (NLSTK_SsapClientReadProperty(dev->appId, dev->streamVolumeHandle) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[MCP] read property fail");
            }
        }
    }
    if (g_mcpVolumeClientCbk.setVolumeRsp != NULL) {
        g_mcpVolumeClientCbk.setVolumeRsp(&dev->addr, resCode);
    }
}

static void McpVolumeEventNtfCbk(int32_t appId, NLSTK_SsapClientEventInfo_S *event)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpVolumeEventNtf");
    NLSTK_CHECK_RETURN_VOID(event != NULL && event->value.data, "[MCP] event is null");
    McpVolumeDevice_S *dev = McpVolumeFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] find dev is null");
    NLSTK_CHECK_RETURN_VOID(event->handle == dev->volumeSyncEventHandle, "[MCP] event handle error");
    NLSTK_CHECK_RETURN_VOID(event->value.len == MCP_VOLUME_EVENT_PARAM_LEN, "[MCP] event param len error");
    uint8_t *data = event->value.data;
    uint8_t eventCode = 0;
    uint8_t newValue = 0;
    PARSE_TO_UINT8(eventCode, data);
    PARSE_TO_UINT8(newValue, data);
    switch (eventCode) {
        case MCP_VOLUME_CHANGE_EVENT:
            if (g_mcpVolumeClientCbk.volumeChangeEvent != NULL) {
                g_mcpVolumeClientCbk.volumeChangeEvent(&dev->addr, newValue);
            }
            break;
        case MCP_MUTE_CHANGE_EVENT:
            if (g_mcpVolumeClientCbk.muteStatusChangeEvent != NULL) {
                g_mcpVolumeClientCbk.muteStatusChangeEvent(&dev->addr, newValue);
            }
            break;
        default:
            NLSTK_LOG_ERROR("[MCP] event code error");
            break;
    }
}

void McpVolumeGetSsapCbk(NLSTK_SsapAppClientCb_S *cb)
{
    NLSTK_CHECK_RETURN_VOID(cb != NULL, "[MCP] callback is null");
    cb->onConnectionStateChanged = McpVolumeConnStateChangeCbk;
    cb->onReadProperty = McpGetVolumeCbk;
    cb->onPropertyChanged = McpVolumePropertyNtfCbk;
    cb->onCallMethod = McpVolumeCallMethodCbk;
    cb->onEvent = McpVolumeEventNtfCbk;
}

uint32_t McpVolumeEnable(void)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpVolumeEnable");
    uint32_t ret = McpVolumeDevInit();
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] dev init fail");
    return NLSTK_ERRCODE_SUCCESS;
}

void McpVolumeDisable(void)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpVolumeDisable");
    McpVolumeDevDeInit();
}

void McpVolumeNotifyAccessPoint(SLE_Addr_S *addr, uint8_t mediaPoint, uint8_t callPoint)
{
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[MCP] addr is null");
    McpVolumeDevice_S *dev = McpVolumeFindDeviceByAddr(addr);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] find dev is null");
    if (mediaPoint != 0) {
        dev->mediaPoint = mediaPoint;
    }
    if (callPoint != 0) {
        dev->callPoint = callPoint;
    }
    NLSTK_LOG_INFO("[MCP] media point = %u, call point = %u", mediaPoint, callPoint);
}

void McpRegVolumeClientCbk(void *arg)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpRegVolumeClientCbk");
    NLSTK_McpVolumeClientCallBack_S *cbk = (NLSTK_McpVolumeClientCallBack_S *)arg;
    NLSTK_CHECK_RETURN_VOID(cbk != NULL, "[MCP] cbk is null");
    (void)memcpy_s(&g_mcpVolumeClientCbk, sizeof(NLSTK_McpVolumeClientCallBack_S),
        cbk, sizeof(NLSTK_McpVolumeClientCallBack_S));
}

void McpVolumeConnect(void *arg)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpVolumeConnect");
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[MCP] addr is null");
    McpVolumeDevice_S *dev = McpVolumeFindDeviceByAddr(addr);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] find dev is null");
    NLSTK_CHECK_RETURN_VOID(dev->connState == NLSTK_MCP_VOLUME_DISCONNECTED, "[MCP] profile connecting or connected");
    NLSTK_CHECK_RETURN_VOID(NLSTK_SsapClientConnect(dev->appId) == NLSTK_ERRCODE_SUCCESS, "[MCP] connect fail");

    dev->connState = NLSTK_MCP_VOLUME_CONNECTING;
    if (g_mcpVolumeClientCbk.stateChange != NULL) {
        g_mcpVolumeClientCbk.stateChange(addr, NLSTK_MCP_VOLUME_CONNECTING, NLSTK_MCP_VOLUME_DISCONNECTED);
    }
}

void McpVolumeDisconnect(void *arg)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpVolumeDisconnect");
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[MCP] addr is null");
    McpVolumeDevice_S *dev = McpVolumeFindDeviceByAddr(addr);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] find dev is null");
    NLSTK_CHECK_RETURN_VOID(dev->connState != NLSTK_MCP_VOLUME_DISCONNECTED, "[MCP] profile unconnected");
    NLSTK_CHECK_RETURN_VOID(NLSTK_SsapClientDisconnect(dev->appId) == NLSTK_ERRCODE_SUCCESS, "[MCP] disconnect fail");
}

void McpGetVolume(void *arg)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpGetVolume");
    NLSTK_McpGetVolumeParam_S *param = (NLSTK_McpGetVolumeParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[MCP] param is null");
    McpVolumeDevice_S *dev = McpVolumeFindDeviceByAddr(&param->addr);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] find dev is null");
    NLSTK_CHECK_RETURN_VOID(dev->connState == NLSTK_MCP_VOLUME_CONNECTED, "[MCP] profile unconnected");
    uint16_t handle = 0;
    if (param->property == NLSTK_MCP_VOLUME_STATUS) {
        handle = dev->volumeHandle;
    } else if (param->property == NLSTK_MCP_STREAM_VOLUME_STATUS) {
        handle = dev->streamVolumeHandle;
    } else {
        NLSTK_LOG_ERROR("[MCP] property type error");
        return;
    }
    if (NLSTK_SsapClientReadProperty(dev->appId, handle) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] read property fail");
    }
}

static void McpVolumeReqEnqueue(McpVolumeDevice_S *dev, NLSTK_VariableData_S *value)
{
    if (dev == NULL || dev->volumeReq == NULL) {
        NLSTK_LOG_ERROR("[MCP] data is null");
        SDF_MemFree(value->data);
        SDF_MemFree(value);
        return;
    }
    if (dev->volumeReq->size >= MCP_VOLUME_REQ_MAX_SIZE) {
        NLSTK_LOG_INFO("[MCP] discard a volume req");
        SDF_VectorRemove(dev->volumeReq, 0);
    }
    if (!SDF_VectorEmplaceBack(dev->volumeReq, value)) {
        NLSTK_LOG_ERROR("[MCP] push volume req queue fail");
        SDF_MemFree(value->data);
        SDF_MemFree(value);
    }
}

static void McpStreamVolumeReqEnqueue(McpVolumeDevice_S *dev, NLSTK_VariableData_S *value)
{
    NLSTK_CHECK_RETURN_VOID(dev != NULL && dev->streamVolumeReq != NULL && value != NULL, "[MCP] data is null");
    if (dev->streamVolumeReq->size >= MCP_VOLUME_REQ_MAX_SIZE) {
        NLSTK_LOG_INFO("[MCP] discard a stream volume req");
        SDF_VectorRemove(dev->streamVolumeReq, 0);
    }
    if (!SDF_VectorEmplaceBack(dev->streamVolumeReq, value)) {
        NLSTK_LOG_ERROR("[MCP] push stream volume req queue fail");
        SDF_MemFree(value->data);
        SDF_MemFree(value);
    }
}

void McpSetVolume(void *arg)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpSetVolume");
    McpSetVolumeParam_S *param = (McpSetVolumeParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[MCP] param is null");
    McpVolumeDevice_S *dev = McpVolumeFindDeviceByAddr(&param->addr);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] find dev is null");
    NLSTK_CHECK_RETURN_VOID(dev->connState == NLSTK_MCP_VOLUME_CONNECTED, "[MCP] profile unconnected");
    NLSTK_CHECK_RETURN_VOID(dev->volumeControlHandle > 0, "[MCP] method handle error");
    NLSTK_VariableData_S *value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    NLSTK_CHECK_RETURN_VOID(value != NULL, "[MCP] value malloc fail");
    value->len = MCP_VOLUME_SET_PARAM_LEN;
    value->data = (uint8_t *)SDF_MemZalloc(value->len);
    if (value->data == NULL) {
        NLSTK_LOG_ERROR("[MCP] value data malloc fail");
        SDF_MemFree(value);
        return;
    }
    size_t index = 0;
    value->data[index++] = MCP_SET_VOLUME_OPCODE;
    value->data[index++] = param->volume;
    value->data[index++] = dev->volumeChangeId;
    value->data[index++] = MCP_VOLUME_WORK_IMMEDIATELY;
    if (dev->update != true) {
        NLSTK_LOG_INFO("[MCP] volume req enqueue");
        McpVolumeReqEnqueue(dev, value);
        return;
    }
    if (NLSTK_SsapClientCallMethod(dev->appId, dev->volumeControlHandle, value, false) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] method call fail");
    }
    dev->update = false;
    SDF_MemFree(value->data);
    SDF_MemFree(value);
}

void McpSetStreamVolume(void *arg)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpSetStreamVolume");
    McpSetStreamVolumeParam_S *param = (McpSetStreamVolumeParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL && param->volumeArray != NULL && param->num != 0, "[MCP] param is null");
    McpVolumeDevice_S *dev = McpVolumeFindDeviceByAddr(&param->addr);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] find dev is null");
    NLSTK_CHECK_RETURN_VOID(dev->connState == NLSTK_MCP_VOLUME_CONNECTED, "[MCP] profile unconnected");
    NLSTK_CHECK_RETURN_VOID(dev->volumeControlHandle > 0, "[MCP] method handle error");
    NLSTK_CHECK_RETURN_VOID(dev->mediaPoint != 0 && dev->callPoint != 0, "[MCP] access point error");
    NLSTK_VariableData_S *value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    NLSTK_CHECK_RETURN_VOID(value != NULL, "[MCP] value malloc fail");
    value->len = MCP_STREAM_VOLUME_SET_BASE_LEN + param->num * MCP_STREAM_VOLUME_SET_EXTRA_LEN;
    value->data = (uint8_t *)SDF_MemZalloc(value->len);
    if (value->data == NULL) {
        NLSTK_LOG_ERROR("[MCP] value data malloc fail");
        SDF_MemFree(value);
        return;
    }
    size_t index = 0;
    value->data[index++] = MCP_SET_STREAM_VOLUME_OPCODE;
    value->data[index++] = param->num;
    for (uint8_t i = 0; i < param->num; i++) {
        if (param->volumeArray[i].streamType == NLSTK_MCP_MEDIA_STREAM) {
            value->data[index] = dev->mediaPoint;
        } else if (param->volumeArray[i].streamType == NLSTK_MCP_CALL_STREAM) {
            value->data[index] = dev->callPoint;
        }
        index++;
        value->data[index++] = param->volumeArray[i].volume;
    }
    value->data[index++] = dev->streamVolumeChangeId;
    if (dev->updateV2 != true) {
        NLSTK_LOG_INFO("[MCP] volume req enqueue");
        McpStreamVolumeReqEnqueue(dev, value);
        return;
    }
    if (NLSTK_SsapClientCallMethod(dev->appId, dev->volumeControlHandle, value, false) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] method call fail");
    }
    dev->updateV2 = false;
    SDF_MemFree(value->data);
    SDF_MemFree(value);
}

void McpGetStreamVolumeAbility(void *arg)
{
    McpGetAbilityParam_S *param = (McpGetAbilityParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[MCP] param is null");
    McpVolumeDevice_S *dev = McpVolumeFindDeviceByAddr(&param->addr);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] find dev is null");
    param->ability = dev->streamVolumeHandle > 0;
}