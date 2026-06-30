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
#ifndef NLSTK_DIS_DEF_H
#define NLSTK_DIS_DEF_H

#include <stdint.h>

#include "nlstk_public_define.h"
#include "sdf_addr.h"
#include "ssap_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DIS_MAX_PROPERTY_SIZE 8
#define DIS_DESC_CLIENT_CONFIG_LEN 2
#define DIS_UUID_FIFTEENTH_BYTE 14
#define DIS_UUID_SIXTEENTH_BYTE 15
#define DIS_MAX_VAR_LEN 255
#define DIS_CONVERT_EIGHT 8
#define DIS_INVALID_APPEARANCE 0xFFFFFFFF

// 服务的属性的定义
#define DIS_SERVICE_UUID                  0x0609
#define DIS_SERVICE_UUID_PEN              0x0906

#define DIS_MANUFACTURER_UUID             0x102E
#define DIS_MANUFACTURER_UUID_PEN         0x2E10

#define DIS_MODEL_UUID                    0x102F
#define DIS_SERIAL_NUMBER_UUID            0x1030
#define DIS_HARDWARE_VERSION_UUID         0x1031
#define DIS_FIRMWARE_VERSION_UUID         0x1032
#define DIS_SOFTWARE_VERSION_UUID         0x1033

#define DIS_LOCAL_ALIAS_UUID              0x103F
#define DIS_LOCAL_ALIAS_UUID_PEN          0x3F10

#define DIS_APPEARANCE_UUID               0x1041
#define DIS_APPEARANCE_UUID_PEN           0x4110

// 设备信息管理服务的连接状态，这个枚举用来通知上层设备信息管理服务的连接状态变化
typedef enum {
    DIS_CONNECTING = 0,
    DIS_CONNECTED,
    DIS_DISCONNECTING,
    DIS_DISCONNECTED,
} NLSTK_DisConnectState_E;

typedef enum {
    DIS_MANUFACTURER_INFO = 0x00,
    DIS_MODEL_INFO,
    DIS_SERIAL_INFO,
    DIS_HARDWARE_VERSION_INFO,
    DIS_FIRMWARE_VERSION_INFO,
    DIS_SOFTWARE_VERSION_INFO,
    DIS_LOCAL_ALIAS_INFO,
    DIS_APPEARANCE_INFO,
} NLSTK_DisInfoType_E;

/**
 * @brief profile连接状态变化回调函数
 *
 * 该回调函数用于通知上层DIS连接状态变化。
 *
 * 若上层调用DisConnect时已连接或DisDisconnect已断连，则会触发该回调将最新的连接状态上报
 *
 * @param[in] addr 对端地址，标识对端设备地址
 * @param[in] state 连接状态，标识当前设备的连接状态
 */
typedef void (*NLSTK_DisConnectStateChangeCbk)(SLE_Addr_S *addr, NLSTK_DisConnectState_E curState,
    NLSTK_DisConnectState_E prevState, NLSTK_Errcode_E errNumb);

typedef struct {
    NLSTK_DisConnectStateChangeCbk stateChangeCbk;
} NLSTK_DisClientCbk_S;

/* 设备信息完整结构（同时会被缓存到客户端和服务端） */
typedef struct {
    NLSTK_VariableData_S manufacturerInfo;      /* 厂商信息 */
    NLSTK_VariableData_S deviceModel;           /* 设备型号 */
    NLSTK_VariableData_S deviceSerialNumber;    /* 设备序列号 */
    NLSTK_VariableData_S hardwareVersion;       /* 硬件版本 */
    NLSTK_VariableData_S firmwareVersion;       /* 固件版本 */
    NLSTK_VariableData_S softwareVersion;       /* 软件版本 */
    NLSTK_VariableData_S deviceLocalAlias;      /* 设备本地别名 */
    uint32_t deviceAppearance;      /* 设备外观 */
    uint8_t propertyRights[DIS_MAX_PROPERTY_SIZE];
} NLSTK_DeviceInfo_S;

typedef struct {
    uint16_t len;
    uint8_t var[DIS_MAX_VAR_LEN];
} NLSTK_DisPropData_S;

typedef struct {
    SLE_Addr_S addr;
    bool find;
    NLSTK_DisPropData_S manufacturerInfo;      /* 厂商信息 */
    NLSTK_DisPropData_S deviceModel;           /* 设备型号 */
    NLSTK_DisPropData_S deviceSerialNumber;    /* 设备序列号 */
    NLSTK_DisPropData_S hardwareVersion;       /* 硬件版本 */
    NLSTK_DisPropData_S firmwareVersion;       /* 固件版本 */
    NLSTK_DisPropData_S softwareVersion;       /* 软件版本 */
    NLSTK_DisPropData_S deviceLocalAlias;      /* 设备本地别名 */
    uint32_t deviceAppearance;                 /* 设备外观 */
} NLSTK_DisAllPropInfo_S;

#ifdef __cplusplus
}
#endif
#endif /* NLSTK_DIS_DEF_H */