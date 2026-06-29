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
#ifndef NLSTK_BAS_DEF_H
#define NLSTK_BAS_DEF_H

#include <stdint.h>

#include "nlstk_public_define.h"
#include "sdf_addr.h"
#include "ssap_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BAS_MAX_PROPERTY_SIZE 8
#define BAS_NULL_INFO  1
#define BAS_MAX_VAR_LEN 255
#define BAS_CONVERT_EIGHT 8
#define BAS_PROPERTY_LENGTH 4
#define BAS_MANDATORY_PROPERTY_LENGTH 1

#define BAS_UUID_FIFTEENTH_BYTE 14
#define BAS_UUID_SIXTEENTH_BYTE 15
#define BAS_MAX_SERVICE_NUM 0xFF
#define BAS_MAX_DATA_LEN 0xFFFF

// 服务的属性的定义
#define BAS_SERVICE_UUID                            0x060A
#define BAS_SERVICE_UUID_PEN                        0x0A06

#define BAS_REMAIN_BATTERY_PERCENTAGE_UUID          0x1034
#define BAS_REMAIN_BATTERY_PERCENTAGE_UUID_PEN      0x3410
#define BAS_REMAIN_BATTERY_UUID                     0x1035
#define BAS_BATTERY_CAPACITY_UUID                   0x1036
#define BAS_BATTERY_RATED_CAPACITY_UUID             0x1037
#define BAS_REMAINING_WORKING_TIME_UUID             0x1038

// 设备信息管理服务的连接状态，这个枚举用来通知上层设备信息管理服务的连接状态变化
typedef enum {
    BAS_CONNECTING = 0,
    BAS_CONNECTED,
    BAS_DISCONNECTING,
    BAS_DISCONNECTED,
} NLSTK_BasConnectState_E;

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
typedef void (*NLSTK_BasConnectStateChangeCbk)(SLE_Addr_S *addr, NLSTK_BasConnectState_E curState,
    NLSTK_BasConnectState_E prevState, NLSTK_Errcode_E errNumb);

#ifdef __cplusplus
}
#endif
#endif /* NLSTK_BAS_DEF_H */