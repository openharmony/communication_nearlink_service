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
#ifndef DEVICE_BATTERY_MANAGER
#define DEVICE_BATTERY_MANAGER

#include <mutex>
#include <vector>
#include <map>
#include "raw_address.h"
#include "TwsDefines.h"

namespace OHOS {
namespace Nearlink {
class BatteryInfo {
public:
    BatteryInfo() = default;
    ~BatteryInfo() = default;

    uint8_t devBattery_;      /* 常规电量具体电量值 */
    uint8_t leftBattery_;     /* 左耳具体电量值 */
    uint8_t leftCharge_;     /* 0：未充电（普通充电）1：充电中（普通充电）2：未充电（超级快充）3：充电中（超级快充） */
    uint8_t rightBattery_;    /* 右耳具体电量值 */
    uint8_t rightCharge_;    /* 0：未充电（普通充电）1：充电中（普通充电）2：未充电（超级快充）3：充电中（超级快充） */
    uint8_t boxBattery_;      /* 耳机盒子具体电量值 */
    uint8_t boxCharge_;      /* 耳机盒充电状态：0:未充电 1:充电中 */
    uint8_t earErrCode_;     /* 0~254：未定义使用  255：耳机连接胶囊 */
    uint8_t boxOpen_;        /* 开盒提醒 */
    uint8_t leftModel_;      /* 左耳动画展示模型ID */
    uint8_t rightModel_;     /* 右耳动画展示模型ID */
    uint8_t dialogState_;    /* 入盒弹窗 */
    uint8_t earStatus_;      /* 1：双耳在盒 2：双耳不在盒 3：左耳在盒，右耳出盒 4：左耳出盒，右耳在盒 */
};

class DeviceBatteryManager {
public:
    static DeviceBatteryManager &GetInstance();
    void PublishBatteryLevel(const RawAddress &peerAddr, const BatteryInfo &batteryInfo);
    void ProcessProfileStateChanged(const std::string &addr, int32_t connectState);
    void ProcessActiveDeviceChanged(const std::string &addr);
    void ReportDeviceBatteryInfo(const std::string &addr, int32_t batteryLevel);

private:
    DeviceBatteryManager();
    ~DeviceBatteryManager();
    std::string GetCurrentBatteryDevice(bool isBatteryDevice);
    void ProcessProfileConnected(const std::string &addr);
    void ProcessProfileDisconnected(const std::string &addr);

    void NotifyForBatteryInfoChanged(const std::string &addr, int32_t batteryLevel);
    int32_t GetDeviceReportBatteryLevel(const std::string &addr);
    int32_t calculateBatteryLevel(const BatteryInfo &batteryInfo);

    void SetDeviceSupportBattery(const std::string &addr);
    std::string GetLastConnectedDevice(bool isBatteryDevice);
    // 管理connectedDevices_
    std::pair<std::string, bool> FindConnectedDevice(const std::string &addr);
    void AddConnectedDevice(std::string addr);
    void UpdateBatteryStatus(std::string addr, bool batteryState);
    void NotifyForDeviceChanged(const std::string &newBatteryDevice);

private:
    std::string activeDevice_ = "";
    std::string currentBatteryDevice_ = "";
    int32_t reportBatteryLevel_ = -1;
    std::map<std::string, BatteryInfo> batteryDeviceMap_ {};
    std::vector<std::pair<std::string, bool>> connectedDevices_ = {}; // 已连接设备地址/支持电量信息
    std::mutex mutex_;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // DEVICE_BATTERY_MANAGER