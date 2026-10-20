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

#include "nearlink_observer_adapter.h"

#include "log.h"
#include "nearlink_remote_device.h"
#include "nearlink_safe_map.h"

namespace OHOS {
namespace Nearlink {
namespace {

NearlinkRemoteDevice ToFrameworkDevice(const NearlinkBaseRemoteDevice &device)
{
    return NearlinkRemoteDevice(device);
}

class NearlinkHostObserverAdapter : public NearlinkBaseHostObserver {
public:
    explicit NearlinkHostObserverAdapter(std::shared_ptr<NearlinkHostObserver> observer) : observer_(observer){};
    ~NearlinkHostObserverAdapter() override = default;

    void OnStateChanged(const int transport, const int status) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseHostObserver(observer_);
            return;
        }
        observer->OnStateChanged(transport, status);
    }
    void OnFullStateChanged(const int transport, const int status) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseHostObserver(observer_);
            return;
        }
        observer->OnFullStateChanged(transport, status);
    }
    void OnPairConfirmed(const NearlinkBaseRemoteDevice &device, int reqType, int number) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseHostObserver(observer_);
            return;
        }
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer->OnPairConfirmed(remoteDevice, reqType, number);
    }
    void OnDeviceNameChanged(const std::string &deviceName) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseHostObserver(observer_);
            return;
        }
        observer->OnDeviceNameChanged(deviceName);
    }
    void OnDeviceAddrChanged(const std::string &address) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseHostObserver(observer_);
            return;
        }
        observer->OnDeviceAddrChanged(address);
    }

private:
    std::weak_ptr<NearlinkHostObserver> observer_;
};

class NearlinkRemoteDeviceObserverAdapter : public NearlinkBaseRemoteDeviceObserver {
public:
    explicit NearlinkRemoteDeviceObserverAdapter(std::shared_ptr<NearlinkRemoteDeviceObserver> observer)
        : observer_(observer){};
    ~NearlinkRemoteDeviceObserverAdapter() override = default;

    void OnAcbStateChanged(const NearlinkBaseRemoteDevice &device, int state, int reason) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseRemoteDeviceObserver(observer_);
            return;
        }
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer->OnAcbStateChanged(remoteDevice, state, reason);
    }
    void OnPairingRequest(const NearlinkBaseRemoteDevice &device, const std::string &passkey, int type) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseRemoteDeviceObserver(observer_);
            return;
        }
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer->OnPairingRequest(remoteDevice, passkey, type);
    }
    void OnPairStatusChanged(
        const NearlinkBaseRemoteDevice &device, int preState, int state, int reason) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseRemoteDeviceObserver(observer_);
            return;
        }
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer->OnPairStatusChanged(remoteDevice, preState, state, reason);
    }
    void OnConnectionStateChanged(
        const NearlinkBaseRemoteDevice &device, int preState, int state, int reason) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseRemoteDeviceObserver(observer_);
            return;
        }
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer->OnConnectionStateChanged(remoteDevice, preState, state, reason);
    }
    void OnRemoteUuidChanged(const NearlinkBaseRemoteDevice &device, const std::vector<UUID> &uuids) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseRemoteDeviceObserver(observer_);
            return;
        }
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer->OnRemoteUuidChanged(remoteDevice, uuids);
    }
    void OnRemoteNameChanged(const NearlinkBaseRemoteDevice &device, const std::string &deviceName) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseRemoteDeviceObserver(observer_);
            return;
        }
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer->OnRemoteNameChanged(remoteDevice, deviceName);
    }
    void OnRemoteAliasChanged(const NearlinkBaseRemoteDevice &device, const std::string &alias) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseRemoteDeviceObserver(observer_);
            return;
        }
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer->OnRemoteAliasChanged(remoteDevice, alias);
    }
    void OnReadRemoteRssiEvent(const NearlinkBaseRemoteDevice &device, int rssi, int status) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseRemoteDeviceObserver(observer_);
            return;
        }
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer->OnReadRemoteRssiEvent(remoteDevice, rssi, status);
    }
    void OnLinkFreqBandChanged(const NearlinkBaseRemoteDevice &device, int32_t freqBand) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseRemoteDeviceObserver(observer_);
            return;
        }
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer->OnLinkFreqBandChanged(remoteDevice, freqBand);
    }

private:
    std::weak_ptr<NearlinkRemoteDeviceObserver> observer_;
};

class NearlinkRemoteDeviceBatteryObserverAdapter : public NearlinkBaseRemoteDeviceBatteryObserver {
public:
    explicit NearlinkRemoteDeviceBatteryObserverAdapter(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer)
        : observer_(observer){};
    ~NearlinkRemoteDeviceBatteryObserverAdapter() override = default;

    void OnGetBatteryLevelEvent(const NearlinkBaseRemoteDevice &device, int32_t batteryLevel) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseBatteryObserver(observer_);
            return;
        }
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer->OnGetBatteryLevelEvent(remoteDevice, batteryLevel);
    }
    void OnBatteryLevelChanged(const NearlinkBaseRemoteDevice &device, int32_t batteryLevel) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseBatteryObserver(observer_);
            return;
        }
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer->OnBatteryLevelChanged(remoteDevice, batteryLevel);
    }

private:
    std::weak_ptr<NearlinkRemoteDeviceBatteryObserver> observer_;
};

class NearlinkRemoteDeviceRssiObserverAdapter : public NearlinkBaseRemoteDeviceRssiObserver {
public:
    explicit NearlinkRemoteDeviceRssiObserverAdapter(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer)
        : observer_(observer){};

    void OnReadRemoteRssiEvent(const NearlinkBaseRemoteDevice &device, int rssi, int status) override
    {
        auto observer = observer_.lock();
        if (observer == nullptr) {
            EraseRssiObserver(observer_);
            return;
        }
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer->OnReadRemoteRssiEvent(remoteDevice, rssi, status);
    }

private:
    std::weak_ptr<NearlinkRemoteDeviceRssiObserver> observer_;
};

NearlinkSafeMap<std::weak_ptr<NearlinkHostObserver>, std::shared_ptr<NearlinkBaseHostObserver>,
    std::owner_less<std::weak_ptr<NearlinkHostObserver>>> g_hostObserverMap;
NearlinkSafeMap<std::weak_ptr<NearlinkRemoteDeviceObserver>,
    std::shared_ptr<NearlinkBaseRemoteDeviceObserver>,
    std::owner_less<std::weak_ptr<NearlinkRemoteDeviceObserver>>> g_remoteDeviceObserverMap;
NearlinkSafeMap<std::weak_ptr<NearlinkRemoteDeviceBatteryObserver>,
    std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver>,
    std::owner_less<std::weak_ptr<NearlinkRemoteDeviceBatteryObserver>>> g_batteryObserverMap;
NearlinkSafeMap<std::weak_ptr<NearlinkRemoteDeviceRssiObserver>,
    std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver>,
    std::owner_less<std::weak_ptr<NearlinkRemoteDeviceRssiObserver>>> g_rssiObserverMap;
} // namespace

std::shared_ptr<NearlinkBaseHostObserver> WrapHostObserver(std::shared_ptr<NearlinkHostObserver> observer)
{
    auto adapter = std::make_shared<NearlinkHostObserverAdapter>(observer);
    if (!g_hostObserverMap.Insert(observer, adapter)) {
        HILOGW("observer has been registered");
        return nullptr;
    }
    return adapter;
}

bool UnwrapHostObserver(
    std::shared_ptr<NearlinkHostObserver> observer, std::shared_ptr<NearlinkBaseHostObserver> &adapter)
{
    return g_hostObserverMap.GetValue(observer, adapter);
}

void EraseHostObserver(std::weak_ptr<NearlinkHostObserver> observer)
{
    g_hostObserverMap.Erase(observer);
}

std::shared_ptr<NearlinkBaseRemoteDeviceObserver> WrapRemoteDeviceObserver(
    std::shared_ptr<NearlinkRemoteDeviceObserver> observer)
{
    auto adapter = std::make_shared<NearlinkRemoteDeviceObserverAdapter>(observer);
    if (!g_remoteDeviceObserverMap.Insert(observer, adapter)) {
        HILOGW("remote device observer has been registered");
        return nullptr;
    }
    return adapter;
}

bool UnwrapRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer,
    std::shared_ptr<NearlinkBaseRemoteDeviceObserver> &adapter)
{
    return g_remoteDeviceObserverMap.GetValue(observer, adapter);
}

void EraseRemoteDeviceObserver(std::weak_ptr<NearlinkRemoteDeviceObserver> observer)
{
    g_remoteDeviceObserverMap.Erase(observer);
}

std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver> WrapBatteryObserver(
    std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer)
{
    auto adapter = std::make_shared<NearlinkRemoteDeviceBatteryObserverAdapter>(observer);
    if (!g_batteryObserverMap.Insert(observer, adapter)) {
        HILOGW("battery observer has been registered");
        return nullptr;
    }
    return adapter;
}

bool UnwrapBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer,
    std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver> &adapter)
{
    return g_batteryObserverMap.GetValue(observer, adapter);
}

void EraseBatteryObserver(std::weak_ptr<NearlinkRemoteDeviceBatteryObserver> observer)
{
    g_batteryObserverMap.Erase(observer);
}

std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver> WrapRssiObserver(
    std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer)
{
    auto adapter = std::make_shared<NearlinkRemoteDeviceRssiObserverAdapter>(observer);
    if (!g_rssiObserverMap.Insert(observer, adapter)) {
        HILOGW("rssi observer has been registered");
        return nullptr;
    }
    return adapter;
}

bool UnwrapRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer,
    std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver> &adapter)
{
    return g_rssiObserverMap.GetValue(observer, adapter);
}

void EraseRssiObserver(std::weak_ptr<NearlinkRemoteDeviceRssiObserver> observer)
{
    g_rssiObserverMap.Erase(observer);
}

} // namespace Nearlink
} // namespace OHOS
