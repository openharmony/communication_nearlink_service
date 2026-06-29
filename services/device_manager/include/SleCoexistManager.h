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

#ifndef SLE_COEXIST_MANAGER_H
#define SLE_COEXIST_MANAGER_H

#include "SleCoexistData.h"
#include "nearlink_safe_list.h"
#include <functional>

namespace OHOS {
namespace Nearlink {

/**
 * @brief 连接共存参数管理器
 * 
 * 负责管理所有连接的共存参数信息，提供数据存取接口
 * 采用单例模式，数据存储在 NearlinkSafeList 中保证线程安全
 */
class SleCoexistManager {
public:
    static SleCoexistManager* GetInstance();

    // 统一插入/更新连接信息
    void UpdateConnectionInfo(const CoexistConnInfo& info);

    // 连接断开
    void OnConnectionRemoved(uint16_t lcid);

    // 获取连接参数
    bool GetConnectionParam(const SLE_Addr_S& addr, uint16_t& timeout, uint16_t& maxLatency);

    // 遍历所有连接信息
    void IterateConnInfo(std::function<void(const CoexistConnInfo&)> callback);

    // 检查是否有多个连接
    bool HasMultipleConnections();

    // 清除所有连接信息
    void ClearAll();

private:
    SleCoexistManager();
    ~SleCoexistManager();

    SleCoexistManager(const SleCoexistManager&) = delete;
    SleCoexistManager& operator=(const SleCoexistManager&) = delete;

    NearlinkSafeList<CoexistConnInfo> connInfoList_ {};
};

} // namespace Nearlink
} // namespace OHOS

#endif // SLE_COEXIST_MANAGER_H
