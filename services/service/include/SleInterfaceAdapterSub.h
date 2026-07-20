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
#ifndef SLE_INTERFACE_ADAPTER_SUB_H
#define SLE_INTERFACE_ADAPTER_SUB_H

#include <memory>
#include "SleInterfaceAdapter.h"
#include "SleMultiScanData.h"
#include "nearlink_def.h"
#include "nearlink_sle_controller_def.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief Represents sle adapter observer.
 *
 * @since 6
 */
class IAdapterSleObserver {
public:
    /**
     * @brief A destructor used to delete the <b>IBleAdapterObserver</b> instance.
     *
     * @since 6
     */
    virtual ~IAdapterSleObserver() = default;

    /**
     * @brief Pair confirmed observer.
     *
     * @param device Remote device.
     * @param reqType Pair type.
     * @param number Paired passkey.
     * @since 6
     */
    virtual void OnPairConfirmed(
        const SleTransport transport, const RawAddress &device, const int reqType, const int number){};

    /**
     * @brief Device name changed observer.
     *
     * @param deviceName Device name.
     * @since 6
     */
    virtual void OnDeviceNameChanged(const std::string deviceName){};

    /**
     * @brief Device address changed observer.
     *
     * @param address Device address.
     * @since 6
     */
    virtual void OnDeviceAddrChanged(const std::string address){};
};

/**
 * @brief Represents peripheral callback.
 *
 * @since 6
 */
class ISlePeripheralCallback {
public:
    /**
     * @brief A destructor used to delete the <b>ISlePeripheralCallback</b> instance.
     *
     * @since 6
     */
    virtual ~ISlePeripheralCallback() = default;

    /**
     * @brief Pair request callback.
     *
     * @param device Remote device.
     * @param passkey Key for the device pairing.
     * @param type Indicates the paring type.
     * @since 6
     */
    virtual void OnPairingRequest(const RawAddress &device, const std::string &passkey, int32_t type){};

    /**
     * @brief Pair State callback.
     *
     * @param transport transport slb/sle.
     * @param device Remote device.
     * @param status Read status.
     * @since 6
     */
    virtual void OnPairStatusChanged(const RawAddress &device, int preStatus, int Status, int reason){};

    /**
     * @brief ACB State callback.
     *
     * @param device Remote device.
     * @param status Read status.
     * @since 6
     */
    virtual void OnAcbStateChanged(const RawAddress &device, int state, int reason){};

    /**
     * @brief Profile Connection state changed observer.
     *
     * @param state Connection state.
     * @since 6
     */
    virtual void OnConnectionStateChanged(
        const RawAddress &device, int state, int preState, int reason = -1){};

    /**
    * @brief Link frequency band changed observer.
    *
    * @param device Remote device.
    * @param freqBand New frequency band.
    */
    virtual void OnLinkFreqBandChanged(const RawAddress &device, const int32_t freqBand){};

    /**
    * @brief Link power level changed observer.
    *
    * @param device Remote device.
    * @param powerLevel Power level.
    */
    virtual void OnLinkPowerLevelChanged(const RawAddress &device, const uint8_t powerLevel){};

    /**
    * @brief Get batteryLevel observer.
    *
    * @param device Remote device.
    * @param batteryLevel Power level.
    */
    virtual void OnGetBatteryLevelEvent(const RawAddress &device, const int8_t batteryLevel){};

    /**
    * @brief Notify batteryLevel Changed.
    *
    * @param device Remote device.
    * @param batteryLevel Power level.
    */
    virtual void OnBatteryLevelChanged(const RawAddress &device, const int8_t batteryLevel){};
};

/**
 * @brief Represents rssi callback.
 *
 * @since 6
 */
class ISleDeviceRssiCallback {
public:
    /**
     * @brief A destructor used to delete the <b>ISleDeviceRssiCallback</b> instance.
     *
     * @since 6
     */
    virtual ~ISleDeviceRssiCallback() = default;

    /**
     * @brief Read remote rssi event callback.
     *
     * @param device Remote device.
     * @param rssi Remote device rssi.
     * @param status Read status.
     * @since 6
     */
    virtual void OnReadRemoteRssiEvent(const RawAddress &device, int rssi, int status){};
};

/**
 * @brief Represents Connection callback.
 *
 * @since 6
 */
class ISleConnectionCallback {
public:
    /**
     * @brief A destructor used to delete the <b>ISleConnectionCallback</b> instance.
     *
     * @since 6
     */
    virtual ~ISleConnectionCallback() = default;

    /**
     * @brief connection update observer.
     *
     * @param connHandle connect handle.
     * @param minInterval min interval.
     * @param maxInterval max interval.
     * @param maxInterval max latency.
     * @since 6
     */
    virtual void OnConnectionUpdate(uint16_t connHandle, uint16_t minInterval,
        uint16_t maxInterval, uint16_t maxLatency){};
};

/**
 * @brief Represents sle adapter interface.
 *
 * @since 6
 */
class SleInterfaceAdapterSub : public SleInterfaceAdapter {
public:
    /**
     * @brief Read remote device rssi value.
     *
     * @param device Remote device
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual int ReadRemoteRssiValue(const RawAddress &device) = 0;

    /**
     * @brief Register sle adapter observer.
     *
     * @param observer Class IBleAdapterObserver pointer to register observer.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool RegisterSleAdapterObserver(IAdapterSleObserver &observer) const = 0;

    /**
     * @brief Deregister sle adapter observer.
     *
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool DeregisterSleAdapterObserver(IAdapterSleObserver &observer) const = 0;

    /**
     * @brief Register peripheral callback.
     *
     * @param callback Class ISlePeripheralCallback pointer to register callback.
     * @since 6
     */
    virtual void RegisterSlePeripheralCallback(ISlePeripheralCallback &callback) const = 0;

    /**
     * @brief Deregister peripheral callback.
     *
     * @since 6
     */
    virtual void DeregisterSlePeripheralCallback(ISlePeripheralCallback &callback) const = 0;

    /**
     * @brief Register rssi callback.
     *
     * @param callback Class ISleDeviceRssiCallback pointer to register callback.
     * @since 6
     */
    virtual void RegisterSleDeviceRssiCallback(ISleDeviceRssiCallback &callback) const = 0;

    /**
     * @brief Deregister rssi callback.
     *
     * @param callback Class ISleDeviceRssiCallback pointer to deregister callback.
     * @since 6
     */
    virtual void DeregisterSleDeviceRssiCallback(ISleDeviceRssiCallback &callback) const = 0;

    /**
     * @brief Register sle connection callback.
     *
     * @param callback Class ISleConnectionCallback pointer to register callback.
     * @since 6
     */
    virtual void RegisterSleConnectionCallback(ISleConnectionCallback &callback) const = 0;

    /**
     * @brief Deregister sle connection callback.
     *
     * @param callback Class ISleConnectionCallback pointer to deregister callback.
     * @since 6
     */
    virtual void DeregisterSleConnectionCallback(ISleConnectionCallback &callback) const = 0;

    /**
     * @brief Get device IO capability.
     *
     * @return Returns device IO capability.
     * @since 6
     */
    virtual int GetIoCapability() const = 0;

    /**
     * @brief Set device IO capability.
     *
     * @param ioCapability IO capability.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool SetIoCapability(int ioCapability) const = 0;

    /**
     * @brief Get max advertising data length.
     *
     * @return Returns max advertising data length.
     * @since 6
     */
    virtual uint32_t GetSleMaxAdvertisingDataLength() const = 0;

    /**
     * @brief Get Link Layer Privacy Supported.
     *
     * @return True:supported; False:not supported.
     * @since 6
     */
    virtual bool IsLlPrivacySupported() const = 0;

    virtual bool IsFeatureSupported(int32_t feature) const = 0;
    virtual bool HasConnectedDevice() = 0;
    virtual bool GetConnectionParam(std::string device, uint16_t &timeout, uint16_t &maxLatency) const = 0;
    virtual bool GetConnectionParam(std::string device, uint16_t &timeout, uint16_t &maxLatency,
        uint16_t &interval) const = 0;
    virtual void ConnectAcb(const RawAddress &device) = 0;
    virtual bool DisconnectAcb(const RawAddress &device, uint8_t discReason) const = 0;
    virtual void ClearBgConnDevice() const = 0;
    virtual bool EnableSleHidCoexMode(const SleHidCoexModeParam &param) = 0;
    virtual bool DisableSleHidCoexMode() = 0;
    virtual std::shared_ptr<SleHidCoexModeParam> GetSleHidCoexModeParam() = 0;
    virtual void SetSleHidCoexModeState(SleCoexModeStatus state) = 0;
};
}  // namespace OHOS
}  // namespace Sle

#endif  // SLE_INTERFACE_ADAPTER_SUB_H
