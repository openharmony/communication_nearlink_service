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

#ifndef NEARLINK_DEVICE_MANAGER_H
#define NEARLINK_DEVICE_MANAGER_H

#include <unordered_map>
#include <mutex>
#include <string>
#include "nearlink_timer.h"
#include "nearlink_def.h"
#include "raw_address.h"
#include "nearlink_safe_hashmap.h"

namespace OHOS {
namespace Nearlink {

constexpr int32_t RET_SUCCESS = 0;
constexpr int32_t RET_NO_EXIST = -1;
constexpr int64_t TIME_MINUTES_10 = 600000;

class NearlinkDeviceManager {
public:
    struct NearlinkDeviceInfo {
        NearlinkDeviceInfo(const std::string &realMacAddr, const std::string &randomMacAddr, uint64_t updateTime)
            : realMacAddr(realMacAddr), randomMacAddr(randomMacAddr), updateTime(updateTime) {}
        NearlinkDeviceInfo(const std::string &realMacAddr, const std::string &randomMacAddr, uint64_t updateTime,
            bool isRetention)
            : realMacAddr(realMacAddr), randomMacAddr(randomMacAddr), updateTime(updateTime),
              isRetention(isRetention) {}

        std::string realMacAddr {INVALID_MAC_ADDRESS};
        std::string randomMacAddr {INVALID_MAC_ADDRESS};
        int64_t updateTime {0};
        bool isRetention {false};
    };

    NearlinkDeviceManager();
    ~NearlinkDeviceManager();
    static NearlinkDeviceManager *GetInstance(void);

    void AddDeviceInfo(const RawAddress &realAddr, const RawAddress &randomAddr, bool isRetention);
    int32_t GetDeviceRandomAddr(const RawAddress &realAddr, RawAddress &randomAddr);
    void GetDeviceRealAddr(const RawAddress &addrToReal, RawAddress &realAddr);
    void GetDeviceRealAddr(const std::string &addrToReal, std::string &realAddr);
    void GetDeviceRealAddr(const std::string &addrToReal, std::string &realAddr, uint32_t tokenId);
    void ClearDevicesInfo();
    void ConvertToRandomAddress(bool isUseRealAddrFlag, const RawAddress &realAddr,
        RawAddress &randomAddr, bool isRetention);
    void ConvertToRandomAddress(const RawAddress &realAddr, RawAddress &randomAddr, bool isRetention);
    void RecoverRetainedDeviceInfo();
    void UpdateRandomAddressMap(const RawAddress &device, int32_t status);

private:
    void RetainRandomAddress(const RawAddress &realAddr);
    void UnretainRandomAddress(const RawAddress &realAddr);
    void ScheduleCleanDeviceInfo();

private:
    int timerMs_ = 60000; // 1 mintue
    std::shared_ptr<NearlinkTimer> nearlinkDeviceManagerTimer_ = nullptr;
    // key: realMacAddr  value:NearlinkDeviceInfo
    NearlinkSafeHashMap<std::string, NearlinkDeviceInfo> nearlinkDevicesMap_ {};
};
}  // namespace nearlink
}  // namespace OHOS

#endif