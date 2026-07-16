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

/****************************************************************************
 *
 * this file contains sle connect param
 *
 ***************************************************************************/

#ifndef SLE_CONNECT_PARAM_H
#define SLE_CONNECT_PARAM_H

#include <stdint.h>
#include "cm_api.h"
#include "cm_inner_api.h"
#include "dli_cmd_struct.h"

#define SLE_INITIATOR_FILTER_POLICY  0     /* 链路是否打开过滤功能 */
#define SLE_INITIATE_PHYS            1     /* 链路扫描通信带宽： 1:1M, 2:2M */
#define SLE_NEGOTIATE                0x1   /* 链路建立时是否进行G和T交互 */
#define SLE_SCAN_INTERVAL            0x20  /* 链路建立时扫描对端设备的interval */
#define SLE_SCAN_WINDOW              0x20  /* 链路建立时扫描对端设备的windows */
#define SLE_CONNECTION_INTERVAL_MAX  0x64  /* 链路调度最大interval */
#define SLE_CONNECTION_INTERVAL_MIN  0x64  /* 链路调度最小interval */
#define SLE_SUPERVISION_TIMEOUT      0x1FC /* 链路超时时间 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SLE连接HCI命令缺省参数初始化
 * @return void
 */
void SleDliConnectParamInit(void);

/**
 * @brief  获取SLE连接参数中的过滤策略
 * @return uint8_t 过滤策略的值
 */
uint8_t SleGetInitiatorFilterPolicy(void);

/**
 * @brief  获取SLE连接HCI参数
 * @return void
 */
DLI_ConnectionCreateParam *SleGetDliConnectParam(void);

/**
 * @brief  配置GLE连接HCI参数
 * @param   [in] setParam:HCI连接参数，包括连接interval,timeout等参数
 * @return void
 */
void SleSetDliConnectParam(CM_ConnectSetParamReq_S *setParam);


#ifdef __cplusplus
}
#endif

#endif /* SLE_CONNECT_PARAM_H */