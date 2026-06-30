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
#ifndef NLSTK_ICCE_CLIENT_H
#define NLSTK_ICCE_CLIENT_H

#include <stdint.h>
#include "nlstk_public_define.h"
#include "sdf_addr.h"
#include "nlstk_icce_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief profile连接状态变化回调函数
 *
 * 该回调函数用于通知上层ICCE连接状态变化。
 *
 * 若上层调用IcceConnect时已连接或IcceDisconnect已断连，则会触发该回调将最新的连接状态上报
 *
 * @param[in] addr 对端地址，标识对端设备地址
 * @param[in] state 连接状态，标识当前设备的连接状态
 */
typedef void (*NLSTK_IcceConnectStateChangeCbk)(SLE_Addr_S *addr, NLSTK_IcceConnectState_E curstate,
    NLSTK_IcceConnectState_E prestate, NLSTK_Errcode_E errCode);

typedef struct {
    NLSTK_IcceConnectStateChangeCbk connectStateChangeCbk;
} NLSTK_IcceClientCallBack_S;

uint32_t NLSTK_IcceRegisterReadInfoCallBack(NLSTK_IcceClientCallBack_S *clientCallback);

/**
* @brief
* 1) 调用SSAP接口建链
* 2）服务发现
* 3）读取ICCE信息
* 4）连接状态变化回调，将连接状态通知给上层
* @param[in] addr 对端设备地址
*/
uint32_t NLSTK_IcceConnect(SLE_Addr_S *addr);

/**
* @brief
* 1) 调用SSAP接口断链
* 2）连接状态变化回调，将连接状态通知给上层
* @param[in] addr 对端设备地址
*/
uint32_t NLSTK_IcceDisconnect(SLE_Addr_S *addr);

/* 同步接口，上层调用时，获取本地缓存的ICCE信息（目前仅有ICCE数字钥匙应用数据交互目的端口信息） */
uint32_t NLSTK_IcceGetPort(SLE_Addr_S *addr, int32_t *port);

/* 同步接口，上层调用时，根据本地缓存的ICCE信息，返回状态为已连接设备的个数 */
uint8_t NLSTK_GetConnectionsDeviceNum(void);

#ifdef __cplusplus
}
#endif

#endif /* NLSTK_ICCE_CLIENT_H */