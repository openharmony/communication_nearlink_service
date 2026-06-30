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
#ifndef CDSM_DEFINES_H
#define CDSM_DEFINES_H

#include <cstdint>
#include <string>
#include "ssap_data.h"
#include "cdsm_api.h"

namespace OHOS {
namespace Nearlink {

constexpr uint32_t CDSM_SERVICE_APP_MAX = 50;                         /* 每个合作集最多有50个应用回调 */
constexpr uint32_t CDSM_SERVICE_INVALID_APP_ID = 0xFFFFFFFF;     /* 回调分配时，非法appID */
constexpr uint32_t CDSM_SERVICE_INVALID_GROUP_ID = 0xFFFFFFFF;   /* 与协议栈约定：非法合作集组ID */

/* 合作集服务，服务层收到协议栈回调时，设备的连接状态定义 */
constexpr uint8_t CDSM_STACK_CLIENT_DISCONNECTED = 0;
constexpr uint8_t CDSM_STACK_CLIENT_CONNECTED = 1;

/* 合作集服务，状态机状态值定义，单设备连接状态复用（取值范围：0-9） */
enum class CdsmClientState : uint8_t {
    CDSM_STATE_DISCONNECTED = 0,
    CDSM_STATE_CONNECTING,
    CDSM_STATE_DISCONNECTING,
    CDSM_STATE_CONNECTED,
};

/* 合作集服务，Profile管理模块启动、关闭服务处理（取值范围：10-19） */
constexpr int CDSM_SERVICE_STARTUP_EVT = 10;
constexpr int CDSM_SERVICE_SHUTDOWN_EVT = 11;
constexpr int CDSM_SERVICE_GET_DEVICES_EVT = 12;

/* 合作集服务，单设备状态变更消息值（取值范围：20-29） */
constexpr int CDSM_SERVICE_CONNECT_START_EVT = 20;
constexpr int CDSM_SERVICE_DISCONNECT_START_EVT = 21;
constexpr int CDSM_SERVICE_CONNECT_CMPL_EVT = 22;
constexpr int CDSM_SERVICE_DISCONNECT_CMPL_EVT  = 23;

} // namespace Sle
} // namespace OHOS

#endif