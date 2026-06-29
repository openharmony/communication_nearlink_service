/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "nearlink_device_manager.h"
#include <chrono>
#include <random>
#include <memory>
#include "log_util.h"
#include "log.h"
#include "securec.h"
#include "nearlink_permission_manager.h"
#include "SleConfig.h"

namespace OHOS {
namespace Nearlink {
constexpr int ARRAY_SIZE = 4;
constexpr int MAC_BIT_SIZE = 12;
constexpr int FIRST_BIT = 1;
constexpr int LAST_BIT = 11;
constexpr int NUMBER_TWO = 2;
constexpr int HEX_BASE = 16;
constexpr int OCT_BASE = 8;

void ToUpper(char* arr)
{
    for (size_t i = 0; i < strlen(arr); ++i) {
        if (arr[i] >= 'a' && arr[i] <= 'z') {
            arr[i] = toupper(arr[i]);
        }
    }
}

RawAddress GenerateRandomMacAddress()
{
    int ret = 0;
    std::string randomMac = "";
    char strMacTmp[ARRAY_SIZE] = {0};
    std::mt19937_64 gen(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    for (int i = 0; i < MAC_BIT_SIZE; i++) {
        if (i != FIRST_BIT) {
            std::uniform_int_distribution<> distribution(0, HEX_BASE - 1);
            ret = sprintf_s(strMacTmp, ARRAY_SIZE, "%x", distribution(gen));
        } else {
            std::uniform_int_distribution<> distribution(0, OCT_BASE - 1);
            ret = sprintf_s(strMacTmp, ARRAY_SIZE, "%x", NUMBER_TWO * distribution(gen));
        }
        if (ret == -1) {
            HILOGE("GenerateRandomMacAddress failed, sprintf_s return -1!");
        }
        ToUpper(strMacTmp);
        randomMac += strMacTmp;
        if ((i % NUMBER_TWO) != 0 && (i != LAST_BIT)) {
            randomMac.append(":");
        }
    }
    return RawAddress(randomMac);
}

NearlinkDeviceManager::NearlinkDeviceManager()
{
    auto timeoutFunc = []() {
        HILOGI("NearlinkDeviceManager: Timeout");
        NearlinkDeviceManager::GetInstance()->ScheduleCleanDeviceInfo();
    };
    nearlinkDeviceManagerTimer_ = std::make_shared<NearlinkTimer>(timeoutFunc);
}

NearlinkDeviceManager::~NearlinkDeviceManager()
{
    ClearDevicesInfo();
}

NearlinkDeviceManager* NearlinkDeviceManager::GetInstance(void)
{
    static NearlinkDeviceManager singleton;
    return &singleton;
}

static int64_t GetTimestamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

void NearlinkDeviceManager::AddDeviceInfo(const RawAddress &realAddr, const RawAddress &randomAddr, bool isRetention)
{
    NearlinkDeviceInfo deviceInfo(realAddr.GetAddress(), randomAddr.GetAddress(), GetTimestamp(), isRetention);
    nearlinkDevicesMap_.EnsureInsert(realAddr.GetAddress(), deviceInfo);
    HILOGI("AddDeviceInfo: realMacAddr: %{public}s, randomMacAddr: %{public}s, isRetention: %{public}d",
        GetEncryptAddr(realAddr.GetAddress()).c_str(), GetEncryptAddr(randomAddr.GetAddress()).c_str(), isRetention);

    if (isRetention) {
        SleConfig::GetInstance().SetPeerRandomAddress(realAddr.GetAddress(), randomAddr.GetAddress());
        // please call SleConfig::GetInstance().Save() outside properly
    } else {
        nearlinkDeviceManagerTimer_->Start(timerMs_, true); // 周期定时器
    }
}

int32_t NearlinkDeviceManager::GetDeviceRandomAddr(const RawAddress &realAddr, RawAddress &randomAddr)
{
    auto func = [&randomAddr, &realAddr](std::string key, NearlinkDeviceInfo &value) -> void {
        HILOGD("realMacAddr: %{public}s, randomMacAddr: %{public}s",
            GET_ENCRYPT_ADDR(realAddr), GetEncryptAddr(value.randomMacAddr).c_str());
        value.updateTime = GetTimestamp();
        randomAddr.SetAddress(value.randomMacAddr);
    };
    if (nearlinkDevicesMap_.GetValueAndOpt(realAddr.GetAddress(), func)) {
        return RET_SUCCESS;
    } else {
        HILOGI("not exist, realMacAddr: %{public}s", GET_ENCRYPT_ADDR(realAddr));
        return RET_NO_EXIST;
    }
}

// 1）注意此函数为重载函数；2）此函数内调用IsUseRealAddr方法，限制在ipc上下文中使用。
void NearlinkDeviceManager::GetDeviceRealAddr(const RawAddress &addr, RawAddress &realAddr)
{
    auto func = [&addr, &realAddr](std::string key, NearlinkDeviceInfo &value) -> bool {
        if (value.randomMacAddr == addr.GetAddress()) {
            value.updateTime = GetTimestamp();
            realAddr.SetAddress(key);
            HILOGI("realMacAddr: %{public}s, randomMacAddr: %{public}s",
                   GetEncryptAddr(key).c_str(), GetEncryptAddr(value.randomMacAddr).c_str());
            return true;
        } else {
            return false;
        }
    };
    if (NearLinkPermissionManager::IsUseRealAddr() || !nearlinkDevicesMap_.Find(func)) {
        realAddr.SetAddress(addr.GetAddress());
        HILOGD("The current address is considered a real address, mac: %{public}s", GET_ENCRYPT_ADDR(addr));
        return;
    }
}

// 1）注意此函数为重载函数；2）此函数内调用IsUseRealAddr方法，限制在ipc上下文中使用。
void NearlinkDeviceManager::GetDeviceRealAddr(const std::string &addr, std::string &realAddr)
{
    auto func = [&addr, &realAddr](std::string key, NearlinkDeviceInfo &value) -> bool {
        if (value.randomMacAddr == addr) {
            value.updateTime = GetTimestamp();
            realAddr = key;
            HILOGI("GetDeviceRealAddr realMacAddr: %{public}s, randomMacAddr: %{public}s",
                   GetEncryptAddr(key).c_str(), GetEncryptAddr(value.randomMacAddr).c_str());
            return true;
        } else {
            return false;
        }
    };
    if (NearLinkPermissionManager::IsUseRealAddr() || !nearlinkDevicesMap_.Find(func)) {
        realAddr = addr;
        HILOGD("The current address is considered a real address, mac: %{public}s", GetEncryptAddr(addr).c_str());
        return;
    }
}

// 1）注意此函数为重载函数；2）此函数内调用IsUseRealAddr方法，限制在ipc上下文中使用。
void NearlinkDeviceManager::GetDeviceRealAddr(const std::string &addr, std::string &realAddr, uint32_t tokenId)
{
    auto func = [&addr, &realAddr](std::string key, NearlinkDeviceInfo &value) -> bool {
        if (value.randomMacAddr == addr) {
            value.updateTime = GetTimestamp();
            realAddr = key;
            HILOGI("GetDeviceRealAddr realMacAddr: %{public}s, randomMacAddr: %{public}s",
                   GetEncryptAddr(key).c_str(), GetEncryptAddr(value.randomMacAddr).c_str());
            return true;
        } else {
            return false;
        }
    };
    if (NearLinkPermissionManager::IsUseRealAddr(tokenId) || !nearlinkDevicesMap_.Find(func)) {
        realAddr = addr;
        HILOGD("The current address is considered a real address, mac: %{public}s", GetEncryptAddr(addr).c_str());
        return;
    }
}

// only called by NearlinkHostServer::OnPairedStatusChanged()
void NearlinkDeviceManager::UpdateRandomAddressMap(const RawAddress &device, int32_t status)
{
    HILOGI("enter NearlinkDeviceManager::UpdateRandomAddressMap, device=%{public}s, status=%{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), status);
    if (status == static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) {
        RetainRandomAddress(device);
    } else if (status == static_cast<int>(SlePairState::SLE_PAIR_NONE)) {
        UnretainRandomAddress(device);
    }
}

void NearlinkDeviceManager::RetainRandomAddress(const RawAddress &realAddr)
{
    RawAddress randomAddr;
    auto updateFunc = [&randomAddr](std::string key, NearlinkDeviceInfo &value) -> void {
        randomAddr.SetAddress(value.randomMacAddr);
        value.updateTime = GetTimestamp();
        value.isRetention = true;
    };
    auto createValueFunc = [&realAddr, &randomAddr]() -> NearlinkDeviceInfo {
        randomAddr = GenerateRandomMacAddress();
        NearlinkDeviceInfo deviceInfo(realAddr.GetAddress(), randomAddr.GetAddress(), GetTimestamp(), true);
        return deviceInfo;
    };
    nearlinkDevicesMap_.FindAndEnsureUpdate(realAddr.GetAddress(), updateFunc, createValueFunc);

    SleConfig::GetInstance().SetPeerRandomAddress(realAddr.GetAddress(), randomAddr.GetAddress());
    SleConfig::GetInstance().Save();
}

void NearlinkDeviceManager::UnretainRandomAddress(const RawAddress &realAddr)
{
    auto func = [](std::string key, NearlinkDeviceInfo &value) -> void {
        value.updateTime = GetTimestamp();
        value.isRetention = false;
    };
    nearlinkDevicesMap_.GetValueAndOpt(realAddr.GetAddress(), func);
    nearlinkDeviceManagerTimer_->Start(timerMs_, true);
    // already RemovePairedDevice() outside
}

void NearlinkDeviceManager::ClearDevicesInfo()
{
    nearlinkDevicesMap_.Clear();
}

void NearlinkDeviceManager::ScheduleCleanDeviceInfo()
{
    HILOGI("ScheduleCleanDeviceInfo begin, nearlinkDevicesMap_ size: %{public}zu", nearlinkDevicesMap_.Size());
    size_t retentionDevCount = 0;
    int64_t currentTime = GetTimestamp();
    auto func = [&retentionDevCount, &currentTime](std::string key, NearlinkDeviceInfo &value) -> bool {
        // 异常情况，如果设备记录时间大于当前时间，记录日志
        if (value.updateTime > currentTime) {
            HILOGI("updateTime large than currentTime, realMacAddr: %{public}s, randomMacAddr: %{public}s",
                   GetEncryptAddr(value.realMacAddr).c_str(), GetEncryptAddr(value.randomMacAddr).c_str());
            return false;
        }
        if (value.isRetention) {
            ++retentionDevCount;
            return false;
        }
        if (currentTime - value.updateTime > TIME_MINUTES_10) {
            HILOGI("remove addr, realMacAddr: %{public}s, randomMacAddr: %{public}s",
                   GetEncryptAddr(value.realMacAddr).c_str(), GetEncryptAddr(value.randomMacAddr).c_str());
            return true;
        }
        return false;
    };
    nearlinkDevicesMap_.IterateAndRmv(func);
    // No device need to delete, stop the timer.
    if (nearlinkDevicesMap_.Size() == retentionDevCount) {
        nearlinkDeviceManagerTimer_->Stop();
    }
    HILOGI("ScheduleCleanDeviceInfo end, nearlinkDevicesMap_ size: %{public}zu, retentionDevCount: %{public}zu",
        nearlinkDevicesMap_.Size(), retentionDevCount);
}

void NearlinkDeviceManager::ConvertToRandomAddress(bool isUseRealAddrFlag, const RawAddress &realAddr,
    RawAddress &randomAddr, bool isRetention)
{
    if (isUseRealAddrFlag) {
        randomAddr = realAddr;
        return;
    }
    int ret = GetDeviceRandomAddr(realAddr, randomAddr);
    if (ret != RET_SUCCESS) {
        randomAddr = GenerateRandomMacAddress();
        AddDeviceInfo(realAddr, randomAddr, isRetention);
        SleConfig::GetInstance().Save();
    }
    return;
}

void NearlinkDeviceManager::ConvertToRandomAddress(
    const RawAddress &realAddr, RawAddress &randomAddr, bool isRetention)
{
    ConvertToRandomAddress(NearLinkPermissionManager::IsUseRealAddr(), realAddr, randomAddr, isRetention);
}

void NearlinkDeviceManager::RecoverRetainedDeviceInfo()
{
    SleConfig::GetInstance().LoadConfigInfo();
    std::vector<std::string> pairedDevices = SleConfig::GetInstance().GetPairedAddrList();
    for (auto realAddrStr : pairedDevices) {
        std::string randomAddrStr = SleConfig::GetInstance().GetPeerRandomAddress(realAddrStr);
        if (randomAddrStr == "") {
            // unlikely case, only in migrating process
            randomAddrStr = GenerateRandomMacAddress().GetAddress();
        }
        AddDeviceInfo(RawAddress(realAddrStr), RawAddress(randomAddrStr), true);
    }
#ifndef TV_STANDARD
    SleConfig::GetInstance().Save();
#endif
}
}  // namespace Nearlink
}  // namespace OHOS