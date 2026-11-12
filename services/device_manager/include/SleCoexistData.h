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

#ifndef SLE_COEXIST_DATA_H
#define SLE_COEXIST_DATA_H

#include "BaseDef.h"
#include "sdf_struct.h"
#include <cstdint>

namespace OHOS {
namespace Nearlink {

/**
 * @brief 连接共存信息数据结构
 *
 * 用于存储单个连接的共存参数信息
 */
struct CoexistConnInfo {
    uint16_t lcid = 0;              // 链路 ID
    uint16_t interval = 0;          // 链路调度间隔，单位 slot
    uint16_t latency = 0;           // 延迟周期，单位为事件组周期
    uint16_t timeout = 0;           // 超时时间，单位 10ms
    SLE_Addr_S addr = {};           // 设备地址

    // 定义==运算符，用于 NearlinkSafeList 的 Insert 方法
    bool operator==(const CoexistConnInfo& other) const
    {
        return lcid == other.lcid;
    }
};

} // namespace Nearlink
} // namespace OHOS

#endif // SLE_COEXIST_DATA_H
