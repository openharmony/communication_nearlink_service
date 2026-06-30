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
#include "nlstk_log.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_ssap_app_link.h"
#include "ssap_pkt.h"
#include "micp_dev.h"
#include "micp_callback.h"
#include "micp_client.h"

#define AUDIO_MIC_CONTROL_UUID 0x0619
#define MIC_SWITCH_STATE_UUID  0x107e

#define OCTETS_14 14
#define OCTETS_15 15
#define SHIFT_8_BITS 8

static uint8_t g_ssapStdBaseUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static NLSTK_SsapUuid_S ConvertUuidTo128Bits(uint16_t uuid)
{
    NLSTK_SsapUuid_S uuidStru;
    for (int i = 0; i < OCTETS_14; i++) {
        uuidStru.uuid[i] = g_ssapStdBaseUuid[i];
    }
    uuidStru.uuid[OCTETS_14] = (uint8_t)((uuid & 0xFF00) >> 0x08);
    uuidStru.uuid[OCTETS_15] = (uint8_t)(uuid & 0x00FF);
    return uuidStru;
}

static uint16_t ConvertUuidTo16Bits(NLSTK_SsapUuid_S *uuid)
{
    return (uint16_t)(uuid->uuid[OCTETS_14]) << SHIFT_8_BITS | uuid->uuid[OCTETS_15];
}

static void MicpRegAppCb(int32_t appId, SLE_Addr_S *addr, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_DEBUG("[MICP] enter MicpRegAppCb");
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[MICP] addr is null");
    MicpDevice_S *device = MicpFindDeviceByAddr(addr);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[MICP] device is null");
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        MicpConnectCbk(device, NLSTK_MICP_STATE_DISCONNECTED, ret);
        return;
    }
    device->appId = appId;
    NLSTK_SsapClientConnect(device->appId);
}

static void MicpConnStateChangedCb(int32_t appId, uint8_t state, NLSTK_Errcode_E ret, int32_t reason)
{
    NLSTK_LOG_DEBUG("[MICP] enter MicpConnStateChangedCb");
    MicpDevice_S *device = MicpFindDeviceByApp(appId);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[MICP] device is null");
    if (state == SSAP_CONNECT_STATE_CONNECTED) {
        if (ret != NLSTK_ERRCODE_SUCCESS) {
            MicpConnectCbk(device, NLSTK_MICP_STATE_DISCONNECTED, ret);
            return;
        }
        NLSTK_SsapUuid_S uuid = ConvertUuidTo128Bits(AUDIO_MIC_CONTROL_UUID);
        NLSTK_SsapClientGetServicesByUuidAsyn(device->appId, &uuid);
    } else if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
        NLSTK_SsapClientDeregAppAsync(device->appId);
        MicpConnectCbk(device, NLSTK_MICP_STATE_DISCONNECTED, NLSTK_ERRCODE_SUCCESS);
        MicpDeleteDevice(&device->addr);
    }
}

static void MicpGetServicesCb(int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_SsapServ_S *service, uint16_t serviceNum,
    NLSTK_SsapClientFreeFunc func)
{
    NLSTK_LOG_DEBUG("[MICP] enter MicpGetServicesCb");
    NLSTK_CHECK_RETURN_VOID(uuid && service, "[MICP] param is null");
    MicpDevice_S *device = MicpFindDeviceByApp(appId);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[MICP] device is null");
    if (serviceNum == 0) {
        MicpConnectCbk(device, NLSTK_MICP_STATE_DISCONNECTED, NLSTK_ERRCODE_FAIL);
        return;
    }
    device->switchHandle = 0;
    for (uint16_t i = 0; i < service[0].propertyNum; i++) {
        NLSTK_SsapPrty_S *property = &service[0].properties[i];
        if (ConvertUuidTo16Bits(&property->uuid) == MIC_SWITCH_STATE_UUID) {
            device->switchHandle = property->handle;
            break;
        }
    }
    if (func != NULL) {
        func(service, serviceNum);
    }
    if (device->switchHandle == 0) {
        MicpConnectCbk(device, NLSTK_MICP_STATE_DISCONNECTED, NLSTK_ERRCODE_FAIL);
        return;
    }
    NLSTK_SsapClientSetPropertyNtf(device->appId, device->switchHandle, true);
}

static void MicpSetPropertyNtfCb(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handle, bool enable,
    NLSTK_Errcode_E ret)
{
    NLSTK_LOG_DEBUG("[MICP] enter MicpSetPropertyNtfCb");
    NLSTK_CHECK_RETURN_VOID(uuid != NULL, "[MICP] uuid is null");
    MicpDevice_S *device = MicpFindDeviceByApp(appId);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[MICP] device is null");
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        MicpConnectCbk(device, NLSTK_MICP_STATE_DISCONNECTED, ret);
        return;
    }
    if (ConvertUuidTo16Bits(uuid) != MIC_SWITCH_STATE_UUID || device->switchHandle != handle) {
        MicpConnectCbk(device, NLSTK_MICP_STATE_DISCONNECTED, NLSTK_ERRCODE_FAIL);
        return;
    }
    if (enable) {
        NLSTK_SsapClientReadProperty(device->appId, device->switchHandle);
    }
}

static void MicpReadPropertyCb(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_DEBUG("[MICP] enter MicpReadPropertyCb");
    NLSTK_CHECK_RETURN_VOID(property != NULL, "[MICP] property is null");
    MicpDevice_S *device = MicpFindDeviceByApp(appId);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[MICP] device is null");
    NLSTK_CHECK_RETURN_VOID(device->switchHandle == property->handle, "[MICP] handle error");
    if (ret != NLSTK_ERRCODE_SUCCESS || property->errorCode != SSAP_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MICP] read failed");
        MicpConnectCbk(device, NLSTK_MICP_STATE_DISCONNECTED, NLSTK_ERRCODE_FAIL);
        return;
    }
    if (property->value.len < 1 || property->value.data == NULL) {
        NLSTK_LOG_ERROR("[MICP] property value error");
        MicpConnectCbk(device, NLSTK_MICP_STATE_DISCONNECTED, NLSTK_ERRCODE_FAIL);
        return;
    }
    uint8_t micState = property->value.data[0];
    if (micState != NLSTK_MICP_MIC_OFF && micState != NLSTK_MICP_MIC_ON) {
        NLSTK_LOG_ERROR("[MICP] unknown micState value");
        MicpConnectCbk(device, NLSTK_MICP_STATE_DISCONNECTED, NLSTK_ERRCODE_FAIL);
        return;
    }
    device->micState = micState;
    MicpConnectCbk(device, NLSTK_MICP_STATE_CONNECTED, NLSTK_ERRCODE_SUCCESS);
    MicpMicStateCbk(device);
}

static void MicpPropertyChangeCb(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property)
{
    NLSTK_LOG_DEBUG("[MICP] enter MicpPropertyChangeCb");
    NLSTK_CHECK_RETURN_VOID(property != NULL, "[MICP] property is null");
    MicpDevice_S *device = MicpFindDeviceByApp(appId);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[MICP] device is null");
    NLSTK_CHECK_RETURN_VOID(device->switchHandle == property->handle, "[MICP] handle error");
    NLSTK_CHECK_RETURN_VOID(property->errorCode == SSAP_ERRCODE_SUCCESS, "[MICP] ntf error");
    if (property->value.len >= 1 && property->value.data != NULL) {
        uint8_t micState = property->value.data[0];
        NLSTK_CHECK_RETURN_VOID(micState == NLSTK_MICP_MIC_OFF || micState == NLSTK_MICP_MIC_ON,
            "[MICP] unknown micState value");
        device->micState = micState;
        MicpMicStateCbk(device);
    }
}

void MicpConnectTask(void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[MICP] addr is null");
    MicpAddDevice(addr);
    MicpDevice_S *device = MicpFindDeviceByAddr(addr);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[MICP] device is null");
    if (device->appId != SSAP_APP_INVALID_ID) {
        NLSTK_SsapClientConnect(device->appId);
        return;
    }
    NLSTK_SsapAppClientCb_S cb = {
        .onRegisterApp = MicpRegAppCb,
        .onConnectionStateChanged = MicpConnStateChangedCb,
        .onGetServices = MicpGetServicesCb,
        .onSetPropertyNtf = MicpSetPropertyNtfCb,
        .onReadProperty = MicpReadPropertyCb,
        .onPropertyChanged = MicpPropertyChangeCb
    };
    NLSTK_ConnParam_S connParam = {0};
    NLSTK_SsapClientRegAppAsyn(&device->addr, &connParam, &cb);
    MicpConnectCbk(device, NLSTK_MICP_STATE_CONNECTING, NLSTK_ERRCODE_SUCCESS);
}

void MicpDisconnectTask(void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[MICP] addr is null");
    MicpDevice_S *device = MicpFindDeviceByAddr(addr);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[MICP] device is null");
    NLSTK_CHECK_RETURN_VOID(device->appId != SSAP_APP_INVALID_ID, "[MICP] appId is invalid");
    NLSTK_SsapClientDisconnect(device->appId);
    MicpConnectCbk(device, NLSTK_MICP_STATE_DISCONNECTING, NLSTK_ERRCODE_SUCCESS);
}