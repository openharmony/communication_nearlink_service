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

#include "nearlink_remote_device.h"
#include "nearlink_safe_map.h"

namespace OHOS {
namespace Nearlink {
namespace {

NearlinkRemoteDevice ToFrameworkDevice(const NearlinkBaseRemoteDevice &device)
{
    return NearlinkRemoteDevice(device.GetDeviceAddr(), device.GetTransportType());
}

class NearlinkHostObserverAdapter : public NearlinkBaseHostObserver {
public:
    explicit NearlinkHostObserverAdapter(std::shared_ptr<NearlinkHostObserver> observer) : observer_(observer){};
    ~NearlinkHostObserverAdapter() override = default;

    void OnStateChanged(const int transport, const int status) override
    {
        observer_->OnStateChanged(transport, status);
    }
    void OnFullStateChanged(const int transport, const int status) override
    {
        observer_->OnFullStateChanged(transport, status);
    }
    void OnPairConfirmed(const NearlinkBaseRemoteDevice &device, int reqType, int number) override
    {
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer_->OnPairConfirmed(remoteDevice, reqType, number);
    }
    void OnDeviceNameChanged(const std::string &deviceName) override
    {
        observer_->OnDeviceNameChanged(deviceName);
    }
    void OnDeviceAddrChanged(const std::string &address) override
    {
        observer_->OnDeviceAddrChanged(address);
    }

private:
    std::shared_ptr<NearlinkHostObserver> observer_;
};

class NearlinkRemoteDeviceObserverAdapter : public NearlinkBaseRemoteDeviceObserver {
public:
    explicit NearlinkRemoteDeviceObserverAdapter(std::shared_ptr<NearlinkRemoteDeviceObserver> observer)
        : observer_(observer){};
    ~NearlinkRemoteDeviceObserverAdapter() override = default;

    void OnAcbStateChanged(const NearlinkBaseRemoteDevice &device, int state, int reason) override
    {
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer_->OnAcbStateChanged(remoteDevice, state, reason);
    }
    void OnPairingRequest(const NearlinkBaseRemoteDevice &device, const std::string &passkey, int type) override
    {
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer_->OnPairingRequest(remoteDevice, passkey, type);
    }
    void OnPairStatusChanged(
        const NearlinkBaseRemoteDevice &device, int preState, int state, int reason) override
    {
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer_->OnPairStatusChanged(remoteDevice, preState, state, reason);
    }
    void OnConnectionStateChanged(
        const NearlinkBaseRemoteDevice &device, int preState, int state, int reason) override
    {
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer_->OnConnectionStateChanged(remoteDevice, preState, state, reason);
    }
    void OnRemoteUuidChanged(const NearlinkBaseRemoteDevice &device, const std::vector<UUID> &uuids) override
    {
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer_->OnRemoteUuidChanged(remoteDevice, uuids);
    }
    void OnRemoteNameChanged(const NearlinkBaseRemoteDevice &device, const std::string &deviceName) override
    {
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer_->OnRemoteNameChanged(remoteDevice, deviceName);
    }
    void OnRemoteAliasChanged(const NearlinkBaseRemoteDevice &device, const std::string &alias) override
    {
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer_->OnRemoteAliasChanged(remoteDevice, alias);
    }
    void OnReadRemoteRssiEvent(const NearlinkBaseRemoteDevice &device, int rssi, int status) override
    {
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer_->OnReadRemoteRssiEvent(remoteDevice, rssi, status);
    }
    void OnLinkFreqBandChanged(const NearlinkBaseRemoteDevice &device, int32_t freqBand) override
    {
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer_->OnLinkFreqBandChanged(remoteDevice, freqBand);
    }

private:
    std::shared_ptr<NearlinkRemoteDeviceObserver> observer_;
};

class NearlinkRemoteDeviceBatteryObserverAdapter : public NearlinkBaseRemoteDeviceBatteryObserver {
public:
    explicit NearlinkRemoteDeviceBatteryObserverAdapter(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer)
        : observer_(observer){};

    void OnGetBatteryLevelEvent(const NearlinkBaseRemoteDevice &device, int32_t batteryLevel) override
    {
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer_->OnGetBatteryLevelEvent(remoteDevice, batteryLevel);
    }
    void OnBatteryLevelChanged(const NearlinkBaseRemoteDevice &device, int32_t batteryLevel) override
    {
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer_->OnBatteryLevelChanged(remoteDevice, batteryLevel);
    }

private:
    std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer_;
};

class NearlinkRemoteDeviceRssiObserverAdapter : public NearlinkBaseRemoteDeviceRssiObserver {
public:
    explicit NearlinkRemoteDeviceRssiObserverAdapter(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer)
        : observer_(observer){};

    void OnReadRemoteRssiEvent(const NearlinkBaseRemoteDevice &device, int rssi, int status) override
    {
        NearlinkRemoteDevice remoteDevice = ToFrameworkDevice(device);
        observer_->OnReadRemoteRssiEvent(remoteDevice, rssi, status);
    }

private:
    std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer_;
};

NearlinkSafeMap<std::shared_ptr<NearlinkHostObserver>, std::shared_ptr<NearlinkBaseHostObserver>> g_hostObserverMap;
NearlinkSafeMap<std::shared_ptr<NearlinkRemoteDeviceObserver>, std::shared_ptr<NearlinkBaseRemoteDeviceObserver>>
    g_remoteDeviceObserverMap;
NearlinkSafeMap<std::shared_ptr<NearlinkRemoteDeviceBatteryObserver>,
    std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver>> g_batteryObserverMap;
NearlinkSafeMap<std::shared_ptr<NearlinkRemoteDeviceRssiObserver>,
    std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver>> g_rssiObserverMap;
} // namespace

std::shared_ptr<NearlinkBaseHostObserver> WrapHostObserver(std::shared_ptr<NearlinkHostObserver> observer)
{
    auto adapter = std::make_shared<NearlinkHostObserverAdapter>(observer);
    g_hostObserverMap.Insert(observer, adapter);
    return adapter;
}

bool UnwrapHostObserver(
    std::shared_ptr<NearlinkHostObserver> observer, std::shared_ptr<NearlinkBaseHostObserver> &adapter)
{
    bool found = g_hostObserverMap.GetValue(observer, adapter);
    if (found) {
        g_hostObserverMap.Erase(observer);
    }
    return found;
}

std::shared_ptr<NearlinkBaseRemoteDeviceObserver> WrapRemoteDeviceObserver(
    std::shared_ptr<NearlinkRemoteDeviceObserver> observer)
{
    auto adapter = std::make_shared<NearlinkRemoteDeviceObserverAdapter>(observer);
    g_remoteDeviceObserverMap.Insert(observer, adapter);
    return adapter;
}

bool UnwrapRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer,
    std::shared_ptr<NearlinkBaseRemoteDeviceObserver> &adapter)
{
    bool found = g_remoteDeviceObserverMap.GetValue(observer, adapter);
    if (found) {
        g_remoteDeviceObserverMap.Erase(observer);
    }
    return found;
}

std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver> WrapBatteryObserver(
    std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer)
{
    auto adapter = std::make_shared<NearlinkRemoteDeviceBatteryObserverAdapter>(observer);
    g_batteryObserverMap.Insert(observer, adapter);
    return adapter;
}

bool UnwrapBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer,
    std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver> &adapter)
{
    bool found = g_batteryObserverMap.GetValue(observer, adapter);
    if (found) {
        g_batteryObserverMap.Erase(observer);
    }
    return found;
}

std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver> WrapRssiObserver(
    std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer)
{
    auto adapter = std::make_shared<NearlinkRemoteDeviceRssiObserverAdapter>(observer);
    g_rssiObserverMap.Insert(observer, adapter);
    return adapter;
}

bool UnwrapRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer,
    std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver> &adapter)
{
    bool found = g_rssiObserverMap.GetValue(observer, adapter);
    if (found) {
        g_rssiObserverMap.Erase(observer);
    }
    return found;
}

} // namespace Nearlink
} // namespace OHOS
