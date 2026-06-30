/**
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
 * this file defines transmission channel identifier.
 *
 ***************************************************************************/

#ifndef DTAP_TCID_H
#define DTAP_TCID_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum : uint8_t {
    TCID_SLB_CMTC = 0x01,  // SLB通用管理传输通道(Common Management Transmission Channel)
    TCID_SLE_CMTC = 0x02,  // SLE通用管理传输通道(Common Management Transmission Channel)

    TCID_SLB_SMTC = 0x09,  // SLB服务管理传输通道(Service Management Transmission Channel)
    TCID_SLE_SMTC = 0x0A,  // SLE服务管理传输通道(Service Management Transmission Channel)

    TCID_SLB_RSMTC = 0x10,  // SLB中继服务管理传输通道(Relay Service Management Transmission Channel)
    TCID_MDCMTC = 0x11,  // 多域协调与管理传输通道(Multi-Domain Coordination and Managemet Transmission Channel)
    TCID_5GITC = 0x12,  // 5G融合传输通道(5G Interworking Transmission Channel)

    TCID_FTC_RFU_END = 0x1D,  // 固定传输通道RFU(Reserved for Future Use)结束

    TCID_SLB_CUTC = 0x1E,  // SLB默认单播数据传输通道(Common Unicast Transmission Channel)
    TCID_SLE_CUTC = 0x1F,  // SLE默认单播数据传输通道(Common Unicast Transmission Channel)

    TCID_BC_BEGIN = 0x50,  // 广播动态分配起始
    TCID_BC_END = 0x7F,    // 广播动态分配结束

    TCID_MC_BEGIN = 0x50,  // 组播动态分配起始
    TCID_MC_END = 0x7F,    // 组播动态分配结束

    TCID_UC_BEGIN = 0x80,  // 单播动态分配起始
    TCID_UC_END = 0xDF,    // 单播动态分配结束

    TCID_MAX = 0xFF,  // 最大传输通道标识符
};

#ifdef __cplusplus
}
#endif
#endif  // DTAP_TCID_H