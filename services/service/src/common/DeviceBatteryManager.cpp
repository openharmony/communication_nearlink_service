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
#include "nearlink_utils.h"
#include "nearlink_common_event_helper.h"
#include "DeviceBatteryManager.h"
#include "TwsService.h"
#include "SleInterfaceManager.h"
#include "SleRemoteDeviceAdapter.h"

namespace OHOS {
namespace Nearlink {

namespace {
    constexpr int32_t INVALID_BATTERY_LEVEL = -1;
}

DeviceBatteryManager::DeviceBatteryManager()
{}

DeviceBatteryManager::~DeviceBatteryManager()
{}

DeviceBatteryManager &DeviceBatteryManager::GetInstance()
{
    static DeviceBatteryManager instance;
    return instance;
}

// 电量更新
void DeviceBatteryManager::PublishBatteryLevel(const RawAddress &peerAddr, const BatteryInfo &batteryInfo)
{
    if (!SleRemoteDeviceAdapter::GetInstance()->IsAudioDevice(peerAddr.GetAddress())) {
        HILOGI("It is not a audio device, no need to publish.");
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    NL_CHECK_RETURN(IsValidAddress(peerAddr.GetAddress()), "[device battery] peerAddr param invalid");
    TwsService *serviceInstance = TwsService::GetService();
    NL_CHECK_RETURN(serviceInstance != nullptr, "[device battery] TwsService GetService failed");
    RawAddress reportAddr = serviceInstance->GetReportAddr(peerAddr);
    SetDeviceSupportBattery(reportAddr.GetAddress());
    batteryDeviceMap_[reportAddr.GetAddress()] = batteryInfo;

    int32_t reportBatteryValue = GetDeviceReportBatteryLevel(reportAddr.GetAddress());
    NotifyForBatteryInfoChanged(reportAddr.GetAddress(), reportBatteryValue);
}

// 变更激活设备
void DeviceBatteryManager::ProcessActiveDeviceChanged(const std::string &addr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    HILOGI("[device battery] activeDevice changed: addr = %{public}s", GetEncryptAddr(addr).c_str());
    activeDevice_ = addr;

    std::string newDeviceAddr = "";
    if (IsValidAddress(addr)) {
        newDeviceAddr = addr;
        HILOGI("[device battery] currentBatteryDevice = %{public}s", GetEncryptAddr(newDeviceAddr).c_str());
    } else {
        newDeviceAddr = GetCurrentBatteryDevice(true);
    }
    NotifyForDeviceChanged(newDeviceAddr);
}

// profile变更
void DeviceBatteryManager::ProcessProfileStateChanged(const std::string &addr, int32_t connectState)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (connectState == static_cast<int>(SleConnectState::CONNECTED)) {
        ProcessProfileConnected(addr);
    } else if (connectState == static_cast<int>(SleConnectState::DISCONNECTED)) {
        ProcessProfileDisconnected(addr);
    } else {
        HILOGD("[device battery] no need process");
        return;
    }

    std::string newDeviceAddr = GetCurrentBatteryDevice(true);
    NotifyForDeviceChanged(newDeviceAddr);
}

void DeviceBatteryManager::ProcessProfileConnected(const std::string &addr)
{
    AddConnectedDevice(addr);
    HILOGD("[device battery] device: %{public}s, map size: %{public}d",
        GetEncryptAddr(addr).c_str(), connectedDevices_.size());
}

void DeviceBatteryManager::ProcessProfileDisconnected(const std::string &addr)
{
    HILOGI("[device battery] addr = %{public}s", GetEncryptAddr(addr).c_str());
    NL_CHECK_RETURN(IsValidAddress(addr), "[device battery] addr is invalid");
    batteryDeviceMap_.erase(addr);
    std::pair<std::string, bool> connectedDevice = FindConnectedDevice(addr);
    auto it = std::find(connectedDevices_.begin(), connectedDevices_.end(), connectedDevice);
    if (it != connectedDevices_.end()) {
        connectedDevices_.erase(it);
    }
    if (activeDevice_ == addr) {
        activeDevice_ = "";
    }
}

std::string DeviceBatteryManager::GetCurrentBatteryDevice(bool isBatteryDevice)
{
    std::string newCurrentBatteryDevice = "";
    bool activeDeviceSupportBattery = FindConnectedDevice(activeDevice_).second;

    if (IsValidAddress(activeDevice_) && activeDeviceSupportBattery) {
        newCurrentBatteryDevice = activeDevice_;
    } else {
        newCurrentBatteryDevice = GetLastConnectedDevice(isBatteryDevice);
    }
    HILOGD("[device battery] newCurrentBatteryDevice: %{public}s", GetEncryptAddr(newCurrentBatteryDevice).c_str());
    return newCurrentBatteryDevice;
}

void DeviceBatteryManager::NotifyForBatteryInfoChanged(const std::string &addr, int32_t batteryLevel)
{
    NL_CHECK_RETURN(currentBatteryDevice_ != "", "currentBatteryDevice_ is empty");
    HILOGD("[device battery] addr = %{public}s, batteryLevel = %{public}d", GetEncryptAddr(addr).c_str(), batteryLevel);
    if (addr != currentBatteryDevice_) {
        HILOGI("[device battery] addr: %{public}s, currentBatteryDevice_: %{public}s",
            GetEncryptAddr(addr).c_str(), GetEncryptAddr(currentBatteryDevice_).c_str());
        return;
    }
    if (reportBatteryLevel_ == batteryLevel) {
        HILOGD("[device battery] addr = %{public}s, batteryLevel = %{public}d not change",
            GetEncryptAddr(addr).c_str(), batteryLevel);
        return;
    }
    ReportDeviceBatteryInfo(addr, batteryLevel);
}

void DeviceBatteryManager::NotifyForDeviceChanged(const std::string &newBatteryDevice)
{
    HILOGD("[device battery] newBatteryDevice = %{public}s, currentBatteryDevice_ = %{public}s",
        GetEncryptAddr(newBatteryDevice).c_str(), GetEncryptAddr(currentBatteryDevice_).c_str());
    if (!newBatteryDevice.empty() && !FindConnectedDevice(newBatteryDevice).second) {
        HILOGE("[device battery] device not battery device");
        return;
    }
    if (newBatteryDevice == currentBatteryDevice_) {
        HILOGD("[device battery] currentBatteryDevice no change, currentBatteryDevice = %{public}s",
            GetEncryptAddr(currentBatteryDevice_).c_str());
        return;
    }
    int32_t batteryLevel = GetDeviceReportBatteryLevel(newBatteryDevice);
    currentBatteryDevice_ = newBatteryDevice;
    if (reportBatteryLevel_ == INVALID_BATTERY_LEVEL && batteryLevel == INVALID_BATTERY_LEVEL) {
        HILOGI("[device battery] newBatteryDevice = %{public}s batteryLevel is -1",
            GetEncryptAddr(newBatteryDevice).c_str());
        return;
    }
    ReportDeviceBatteryInfo(newBatteryDevice, batteryLevel);
}

void DeviceBatteryManager::ReportDeviceBatteryInfo(const std::string &addr, int32_t batteryLevel)
{
    HILOGI("[device battery] addr = %{public}s, batteryLevel = %{public}d", GetEncryptAddr(addr).c_str(), batteryLevel);
    reportBatteryLevel_ = batteryLevel;
    NearlinkHelper::NearlinkCommonEventHelper::PublishDeviceBatteryLevelEvent(addr, batteryLevel);
}

int32_t DeviceBatteryManager::GetDeviceReportBatteryLevel(const std::string &addr)
{
    if (!IsValidAddress(addr)) {
        HILOGD("currentBatteryDevice is invalid");
        return INVALID_BATTERY_LEVEL;
    }
    if (!batteryDeviceMap_.count(addr)) {
        HILOGE("[device battery] addr: %{public}s not exist", GetEncryptAddr(addr).c_str());
        return INVALID_BATTERY_LEVEL;
    }

    return calculateBatteryLevel(batteryDeviceMap_[addr]);
}

int32_t DeviceBatteryManager::calculateBatteryLevel(const BatteryInfo &batteryInfo)
{
    int batteryValue = INVALID_BATTERY_LEVEL;
    uint8_t higherBattery = (batteryInfo.leftBattery_ >= batteryInfo.rightBattery_) ?
        batteryInfo.leftBattery_ : batteryInfo.rightBattery_;
    uint8_t lowerBattery = (batteryInfo.leftBattery_ < batteryInfo.rightBattery_) ?
        batteryInfo.leftBattery_ : batteryInfo.rightBattery_;

    if (higherBattery == 0) {
        // 左、右耳机电量信息都未携带，则上报一般电量信息
        if (batteryInfo.devBattery_ != 0) {
            batteryValue = static_cast<int>(batteryInfo.devBattery_);
        }
    } else {
        // 若只携带一只耳机的电量，则上报该电量；若两只耳机的电量都有携带，则上报电量较低的耳机电量
        batteryValue = (lowerBattery == 0) ?
            static_cast<int>(higherBattery) : static_cast<int>(lowerBattery);
    }
    HILOGD("[device battery] reportBatteryValue = %{public}d", batteryValue);

    return batteryValue;
}

void DeviceBatteryManager::SetDeviceSupportBattery(const std::string &addr)
{
    NL_CHECK_RETURN(IsValidAddress(addr), "[device battery] device not set");
    NL_CHECK_RETURN(IsValidAddress(FindConnectedDevice(addr).first), "[device battery] failed!");
    HILOGD("[device battery] device: %{public}s supports battery ", GetEncryptAddr(addr).c_str());
    UpdateBatteryStatus(addr, true);
    currentBatteryDevice_ = GetCurrentBatteryDevice(true);
}

std::string DeviceBatteryManager::GetLastConnectedDevice(bool isBatteryDevice)
{
    std::string addr = "";
    NL_CHECK_RETURN_LOGD_RET(!connectedDevices_.empty(), addr, "[device battery] No connected device exists.");

    int vectorSize = static_cast<int>(connectedDevices_.size());
    for (int idx = vectorSize - 1; idx >= 0; idx--) {
        if (connectedDevices_[idx].second == isBatteryDevice) {
            addr = connectedDevices_[idx].first;
            break;
        }
    }
    return addr;
}

void DeviceBatteryManager::AddConnectedDevice(std::string addr)
{
    // 该函数只适用于第一次添加设备，如果该设备已经存在于设备列表里，将会删除后重新添加，这样会改变原有的连接顺序
    // 请谨慎使用，如果只是需要更新设备电量状态，可以使用UpdateBatteryStatus
    std::pair<std::string, bool> connectDevice = FindConnectedDevice(addr);
    if (IsValidAddress(connectDevice.first)) {
        HILOGD("[device battery] device: %{public}s is exist, add device again",
            GetEncryptAddr(connectDevice.first).c_str());
        auto it = std::find(connectedDevices_.begin(), connectedDevices_.end(), connectDevice);
        if (it != connectedDevices_.end()) {
            connectedDevices_.erase(it);
        }
    } else {
        HILOGD("[device battery] first add device! %{public}s", GetEncryptAddr(connectDevice.first).c_str());
    }
    connectedDevices_.push_back(std::make_pair(addr, false));

    // 避免porfile 连接中, 存在电量上报, 需要更新一下
    if (batteryDeviceMap_.count(addr)) {
        SetDeviceSupportBattery(addr);
        int32_t reportBatteryValue = GetDeviceReportBatteryLevel(addr);
        NotifyForBatteryInfoChanged(addr, reportBatteryValue);
    }
}

void DeviceBatteryManager::UpdateBatteryStatus(std::string addr, bool batteryState)
{
    std::pair<std::string, bool> connectDevice = FindConnectedDevice(addr);
    NL_CHECK_RETURN(IsValidAddress(connectDevice.first), "[device battery] device not exist!");

    HILOGD("[device battery] device supports battery status change");
    for (auto &device : connectedDevices_) {
        if (device.first == addr) {
            device.second = batteryState;
        }
    }
}

std::pair<std::string, bool> DeviceBatteryManager::FindConnectedDevice(const std::string &addr)
{
    std::pair<std::string, bool> connectedDevice = {"", false};
    if (connectedDevices_.size() == 0) {
        HILOGD("[device battery] No connected battery device exists.");
        return connectedDevice;
    }
    HILOGD("[device battery] connected device size: %{public}d", connectedDevices_.size());
    for (auto &device : connectedDevices_) {
        if (device.first == addr) {
            connectedDevice = device;
            break;
        }
    }
    return connectedDevice;
}
} // namespace Nearlink
} // namespace OHOS