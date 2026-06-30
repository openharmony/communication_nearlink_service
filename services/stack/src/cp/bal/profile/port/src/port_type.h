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
#ifndef PORT_TYPE_H
#define PORT_TYPE_H

#include <stdint.h>
#include "nlstk_port_def.h"
#include "nlstk_ssap_app_client.h"
#include "ssap_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PORT_NUM_LEN 2

typedef struct {
    SLE_Addr_S addr;
    int state;
} PortGetConnStateParam_S;

typedef struct {
    SLE_Addr_S addr;
    NLSTK_SsapUuid_S uuid;
    uint16_t portId;
} PortGetPortIdParam_S;

typedef struct {
    SLE_Addr_S addr;
    int portConnectNum;
} PortGetConnDevNumParam_S;

// 私有端口信息
typedef struct {
    uint16_t portId;            // 端口号
    uint16_t manufactureId;     // 厂商标识
    NLSTK_SsapUuid_S uuid;        // UUID
} PortPrivateInfo_S;

typedef struct {
    int32_t appId;
    SLE_Addr_S addr;
    uint16_t propertyHandle;
    uint8_t stmState;
    SDF_Vector_S *portInfoList;
    NLSTK_ConnParam_S connParam;
    bool findFlag;
} PortInfoCache_S;

typedef struct {
    uint8_t state;
    NLSTK_Errcode_E ret;
    int32_t reason;
} PortConnStateMsg_S;

typedef struct {
    uint16_t serviceNum;
    NLSTK_SsapServ_S *service;
    NLSTK_SsapClientFreeFunc func;
} PortGetServiceMsg_S;

typedef struct {
    NLSTK_SsapClientReadPropertyInfo_S *property;
    NLSTK_Errcode_E ret;
} PortReadPropertyMsg_S;

typedef struct {
    uint16_t handle;
    bool enable;
    NLSTK_Errcode_E ret;
} PortSetPropertyNtfMsg_S;

static NLSTK_SsapUuid_S g_portServiceStdUuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x06, 0x60}};

static NLSTK_SsapUuid_S g_portPropertyStdUuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x11, 0x01}};

#ifdef __cplusplus
}
#endif

#endif