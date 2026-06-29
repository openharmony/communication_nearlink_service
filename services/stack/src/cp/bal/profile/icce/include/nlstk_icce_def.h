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
#ifndef NLSTK_ICCE_DEF_H
#define NLSTK_ICCE_DEF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NLSTK_ICCE_SLQI 17
#define NLSTK_ICCE_AID 15

// 每个属性的操作权限，用来设置 NLSTK_IcceServiceInfo_S 中的 propertyRights 字段
#define NLSTK_ICCE_READ_ENCRYPT 0x01   // 加密读取权限
#define NLSTK_ICCE_READ_AUTHOR 0x02    // 授权读取权限
#define NLSTK_ICCE_READ_AUTHEN 0x04    // 认证读取权限

typedef enum {
    ICCE_CONNECTING = 0,
    ICCE_CONNECTED,
    ICCE_DISCONNECTING,
    ICCE_DISCONNECTED,
} NLSTK_IcceConnectState_E;

// 定义ICCE服务
typedef struct {
    uint16_t iccePort;       // ICCE端口号
    uint8_t iccePortRight;   // ICCE端口权限
} NLSTK_IcceServiceInfo_S;

#ifdef __cplusplus
}
#endif
#endif /* NLSTK_ICCE_DEF_H */