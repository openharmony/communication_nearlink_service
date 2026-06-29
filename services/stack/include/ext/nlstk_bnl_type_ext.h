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

#ifndef NLSTK_BNL_TYPE_EXT_H
#define NLSTK_BNL_TYPE_EXT_H

#include <stdint.h>
#include "sdf_addr.h"
#include "nlstk_public_define_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BNL_LINK_STATE_CONNECTED                 = 0x00,  /* 已经连接 */
    BNL_LINK_STATE_DISCONNECTED              = 0x02,  /* 已经断开 */
} NLSTK_BnlLinkState_E;

typedef struct {
    uint16_t  lcid;         /* 星闪逻辑链路handle */
    uint8_t   role;         /* 链路角色：G或者T */
    SLE_Addr_S addr;        /* 星闪设备地址 */
    uint8_t   result;       /* 链路连接状态 */
    uint8_t discReason;     /* 链路断链原因 */
    uint8_t connCompleteType; /* 连接完成类型，0：通过本端扫描建立的连接，1：通过本端广播建立的连接 */
    uint8_t advHandle;        /* 广播标识，若是通过本端广播建立的连接，则为对应启动广播标识，否则默认为0，无效值 */
} NLSTK_BnlLogicLinkState_S;

typedef struct {
    uint8_t pi;         /* 上层协议指示 */
    uint16_t lcid;      /* 逻辑链路handle */
    uint8_t tcid;       /* 传输通道tcid */
    uint32_t len;
    uint8_t val[0];     /* 待发送数据 */
} __attribute__((packed)) NLSTK_BnlSendMsg_S;

typedef struct {
    void (*connectDeviceFunc)(const SLE_Addr_S *addr);
    void (*disConnectDeviceFunc)(const SLE_Addr_S *addr);
    void (*sendMsgFunc)(const uint8_t *cmdVal, uint32_t cmdLen);
} NLSTK_BnlProxyFunc_S;

#ifdef __cplusplus
}
#endif

#endif // NLSTK_BNL_TYPE_EXT_H