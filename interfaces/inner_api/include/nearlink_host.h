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

/**
 * @addtogroup Nearlink
 * @{
 *
 * @brief Defines nearlink host, including observer and common functions.
 *
 * @since 6
 */

/**
 * @file nearlink_host.h
 *
 * @brief Framework nearlink host interface.
 *
 * @since 6
 */

#ifndef NEARLINK_HOST_H
#define NEARLINK_HOST_H

#include "nearlink_def.h"
#include "nearlink_errorcode.h"
#include "nearlink_types.h"
#include "nearlink_remote_device.h"
#include "nearlink_uuid.h"
#include "refbase.h"

namespace OHOS { class IRemoteObject; }
namespace OHOS {
namespace Nearlink {
/**
 * @brief Represents framework host device basic observer.
 *
 * @since 6
 */
class NearlinkHostObserver {
public:
    /**
     * @brief A destructor used to delete the <b>nearlinkHostObserver</b> instance.
     *
     * @since 6
     */
    virtual ~NearlinkHostObserver() = default;

    // common
    /**
     * @brief Adapter state change function.
     *
     * @param transport Transport type when state change.
     *        SleTransport::ADAPTER_SLB : classic;
     *        SleTransport::ADAPTER_SLE : sle.
     * @param state Change to the new state.
     *        SleStateID::STATE_TURNING_ON;
     *        SleStateID::STATE_TURN_ON;
     *        SleStateID::STATE_TURNING_OFF;
     *        SleStateID::STATE_TURN_OFF.
     * @since 6
     */
    virtual void OnStateChanged(const int transport, const int status) = 0;

    /**
     * @brief Adapter full state change function.
     *
     * @param transport Transport type when state change.
     *        SleTransport::ADAPTER_SLB : classic;
     *        SleTransport::ADAPTER_SLE : sle.
     * @param state Change to the new state.
     *        SleStateID::STATE_TURNING_ON;
     *        SleStateID::STATE_TURN_ON;
     *        SleStateID::STATE_TURNING_OFF;
     *        SleStateID::STATE_TURN_OFF;
     *        SleStateID::STATE_TURNING_HALF_TO_OFF;
     *        SleStateID::STATE_TURNING_OFF_TO_HALF;
     *        SleStateID::STATE_TURNING_HALF_TO_ON;
     *        SleStateID::STATE_TURNING_ON_TO_HALF;
     *        SleStateID::STATE_TURN_HALF.
     * @since 6
     */
    virtual void OnFullStateChanged(const int transport, const int status){};

    /**
     * @brief Pair confirmed observer.
     *
     * @param device Remote device.
     * @param reqType Pair type.
     * @param number Paired passkey.
     * @since 6
     */
    virtual void OnPairConfirmed(const NearlinkRemoteDevice &device, int reqType, int number){};

    /**
     * @brief Device name changed observer.
     *
     * @param deviceName Device name.
     * @since 6
     */
    virtual void OnDeviceNameChanged(const std::string &deviceName){};

    /**
     * @brief Device address changed observer, address is little endian order.
     *
     * @param address Device address.
     * @since 6
     */
    virtual void OnDeviceAddrChanged(const std::string &address){};
};

class NearlinkRemoteDeviceObserver {
public:
    /**
     * @brief A destructor used to delete the <b>NearlinkRemoteDeviceObserver</b> instance.
     *
     * @since 6
     */
    virtual ~NearlinkRemoteDeviceObserver() = default;

    /**
     * @brief Acb state changed observer.
     *
     * @param device Remote device.
     * @param state Remote device acb state.
     * @param reason Remote device reason.
     * @since 6
     */
    virtual void OnAcbStateChanged(const NearlinkRemoteDevice &device, int state, int reason) = 0;

    /**
     * @brief Pairing request.
     *
     * @param device Remote device.
     * @param passkey Key for the device pairing.
     * @param type Paring type.
     * @since 6
     */
    virtual void OnPairingRequest(const NearlinkRemoteDevice &device, const std::string &passkey, int type) = 0;

    /**
     * @brief Pair status changed observer.
     *
     * @param device Remote device.
     * @param status Remote device pair status.
     * @param reason pair reason.
     * @since 6
     */
    virtual void OnPairStatusChanged(const NearlinkRemoteDevice &device, int preState, int state, int reason) = 0;

    /**
     * @brief Profile connect state changed observer.
     *
     * @param device Remote device.
     * @param state Profile connect state.
     * @param reason Profile connect reason.
     * @since 6
     */
    virtual void OnConnectionStateChanged(const NearlinkRemoteDevice &device, int preState, int state, int reason) = 0;

    /**
     * @brief Remote uuid changed observer.
     *
     * @param device Remote device.
     * @param uuids Remote device uuids, uuid is big endian order.
     * @since 6
     */
    virtual void OnRemoteUuidChanged(const NearlinkRemoteDevice &device, const std::vector<UUID> &uuids) = 0;

    /**
     * @brief Remote name changed observer.
     *
     * @param device Remote device.
     * @param deviceName Remote device name.
     * @since 6
     */
    virtual void OnRemoteNameChanged(const NearlinkRemoteDevice &device, const std::string &deviceName) = 0;

    /**
     * @brief Remote alias changed observer.
     *
     * @param device Remote device.
     * @param alias Remote device alias.
     * @since 6
     */
    virtual void OnRemoteAliasChanged(const NearlinkRemoteDevice &device, const std::string &alias) = 0;

    /**
     * @brief Remote rssi event observer.
     *
     * @param device Remote device.
     * @param rssi Remote device rssi.
     * @param status Read status.
     * @since 6
     */
    virtual void OnReadRemoteRssiEvent(const NearlinkRemoteDevice &device, int rssi, int status) = 0;

    /**
     * @brief Link frequency band changed observer.
     *
     * @param device Remote device.
     * @param freqBand New frequency band.
     */
    virtual void OnLinkFreqBandChanged(const NearlinkRemoteDevice &device, int32_t freqBand){};
};

class NearlinkRemoteDeviceBatteryObserver {
public:
    /**
     * @brief Get batteryLevel observer.
     *
     * @param device Remote device.
     * @param batteryLevel battery level. default value or no response value: -1.
     */
    virtual void OnGetBatteryLevelEvent(const NearlinkRemoteDevice &device, int32_t batteryLevel){};

    /**
     * @brief Notify batteryLevel changed.
     *
     * @param device Remote device.
     * @param batteryLevel battery level. default value or no response value: -1.
     */
    virtual void OnBatteryLevelChanged(const NearlinkRemoteDevice &device, int32_t batteryLevel){};
};

class NearlinkRemoteDeviceRssiObserver {
public:
    /**
    * @brief A destructor used to delete the <b>NearlinkRemoteDeviceRssiObserver</b> instance.
    *
    * @since 6
    */
    virtual ~NearlinkRemoteDeviceRssiObserver() = default;

    /**
     * @brief Remote rssi event observer.
     *
     * @param device Remote device.
     * @param rssi Remote device rssi.
     * @param status Read status.
     * @since 6
     */
    virtual void OnReadRemoteRssiEvent(const NearlinkRemoteDevice &device, int rssi, int status){};
};

/**
 * @brief Represents framework host device.
 *
 * @since 6
 */
class NEARLINK_API NearlinkHost {
public:
    /**
     * @brief Get default host device.
     *
     * @return Returns the singleton instance.
     * @since 6
     */
    static NearlinkHost &GetInstance();

    /**
     * @brief Register observer.
     *
     * @param observer Class nearlinkHostObserver pointer to register observer.
     * @return Returns the status code for this function called.
     */
    NlErrCode RegisterObserver(std::shared_ptr<NearlinkHostObserver> observer);

    /**
     * @brief Deregister observer.
     *
     * @param observer Class nearlinkHostObserver pointer to deregister observer.
     * @return Returns the status code for this function called.
     */
    NlErrCode DeregisterObserver(std::shared_ptr<NearlinkHostObserver> observer);

    /**
     * @brief Register BAS observer.
     *
     * @param observer Class NearlinkRemoteDeviceBatteryObserver pointer to register observer.
     * @return Returns the status code for this function called.
     */
    NlErrCode RegisterBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer);

    /**
     * @brief Deregister BAS observer.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode DeregisterBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer);

    /**
    * @brief Register rssi observer.
    *
    * @param observer Class NearlinkRemoteDeviceRssiObserver pointer to register observer.
    * @return Returns the status code for this function called.
    */
    NlErrCode RegisterRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer);

    /**
     * @brief Deregister rssi observer.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode DeregisterRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer);

    /**
     * @brief Load nearlink SA.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode LoadNearlinkSa();

    /**
     * @brief Enable nearlink.
     *
     * @param autoConnPolicy Enum of auto-connecting policy when nearlink is enabled.
     * @return Returns the status code for this function called.
     */
    NlErrCode EnableNl(const SleAutoConnectPolicy autoConnPolicy = SleAutoConnectPolicy::AUTO_CONN_GENERAL);

    /**
     * @brief Disable nearlink. Nearlink could be turned to STATE_TURN_OFF or STATE_TURN_HALF.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode DisableNl();

    /**
     * @brief Disable nearlink to STATE_TURN_OFF.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode DisableNlToOff();

    /**
     * @brief Enable nearlink to STATE_TURN_HALF.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode EnableNlToHalf();

    /**
     * @brief Get current state of Nearlink adapter.
     *
     * @return current state of Nearlink adapter.
     *        SleStateID::STATE_TURNING_ON;
     *        SleStateID::STATE_TURN_ON;
     *        SleStateID::STATE_TURNING_OFF;
     *        SleStateID::STATE_TURN_OFF;
     *        SleStateID::STATE_TURNING_HALF_TO_OFF;
     *        SleStateID::STATE_TURNING_OFF_TO_HALF;
     *        SleStateID::STATE_TURNING_HALF_TO_ON;
     *        SleStateID::STATE_TURNING_ON_TO_HALF;
     *        SleStateID::STATE_TURN_HALF.
     */
    SleStateID GetSleFullState();

    /**
     * @brief Check whether sle is in enabled state.
     *
     * @return Returns <b>true</b> if sle is enabled;
     *         returns <b>false</b> if sle is not in enabled state.
     */
    bool IsSleEnabled() const;

    /**
     * @brief Check whether sle is in half-disabled state.
     *
     * @return Returns <b>true</b> if sle is half-disabled;
     *         returns <b>false</b> if sle is not in half-disabled state.
     */
    bool IsSleHalfDisabled() const;

    /**
     * @brief Check whether sle is disabled.
     *
     * @return Returns <b>true</b> if sle is disabled;
     *         returns <b>false</b> if sle is not in disabled state.
     */
    bool IsSleDisabled() const;

    /**
     * @brief Check whether sle is available.
     *        If sle is enbled, it's available to all callers.
     *        If sle is half-disabled, it's available only to native callers.
     *        In half-disabled state, native callers are allowed to use functions which are not related to pairing.
     *
     * @return Returns <b>true</b> if sle is available to caller;
     *         returns <b>false</b> if sle is not available to caller.
     */
    bool IsSleAvailableToCaller() const;

    /**
     * @brief Check whether nearlink is Support.
     *
     * @return Returns <b>true</b> if nearlink is support to caller;
     *         returns <b>false</b> if nearlink is not support to caller.
     */
    bool IsNearlinkSupport() const;

    /**
     * @brief Check whether nearlink frame4 is supported. Caller needn't ensure nearlink is on.
     *
     * @return Returns <b>true</b> if nearlink frame4 is support to caller;
     *         returns <b>false</b> if nearlink frame4 is not support to caller.
     */
    bool IsNearlinkSupportFrame4() const;

    /**
     * @brief Check whether nearlink audio is Support.
     *
     * @return Returns <b>true</b> if nearlink audio is support to caller;
     *         returns <b>false</b> if nearlink audio is not support to caller.
     */
    bool IsNearlinkAudioSupport() const;

    /**
     * @brief Factory reset Nearlink service.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode NearlinkFactoryReset();

    /**
     * @brief Nearlink FreqHopping interface.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode SetFreqHopping(const std::vector<uint8_t> &freq);

    /**
     * @brief Returns 1(CONNECTED) if a certain profile of a certain device has been connected.
     *
     * @param[out] SLE adapter connection state.
     *         SleConnectState::CONNECTING(0);
     *         SleConnectState::CONNECTED(1);
     *         SleConnectState::DISCONNECTING(2);
     *         SleConnectState::DISCONNECTED(3).
     * @return Returns the status code for this function called.
     */
    NlErrCode GetAdapterConnectState(int &state) const;

    /**
     * @brief Get profile service connect state of remote device.
     *
     * @param remoteAddr Profile service ID, remoteAddr is little endian order.
     * @param[out] connect state for designated profile service.
     *         BTConnectState::CONNECTING;
     *         BTConnectState::CONNECTED;
     *         BTConnectState::DISCONNECTING;
     *         BTConnectState::DISCONNECTED.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetProfileConnState(const std::string &remoteAddr, int &connState) const;

    /**
     * @brief Get local device supported uuids.
     *
     * @param[out] uuids which use to return support uuids, uuid is big endian order.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetLocalSupportedUuids(std::vector<UUID> &uuids);

    /**
     * @brief Get local device appearance.
     *
     * @param[out] deviceAppearance Returns local device appearance.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetLocalDeviceAppearance(int &deviceAppearance) const;

    /**
     * @brief Set local device appearance.
     *
     * @param appearance Device appearance.
     * @return Returns the status code for this function called.
     */
    NlErrCode SetLocalDeviceAppearance(int appearance);

    /**
     * @brief Get local device address, localDeviceAddres is little endian order.
     *
     * @param[out] localDeviceAddres Returns local device address.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetLocalAddress(std::string &localDeviceAddres) const;

    /**
     * @brief Get local device name.
     *
     * @param[out] localDeviceName Returns local device name.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetLocalName(std::string &localDeviceName) const;

    /**
     * @brief Set local device name.
     *
     * @param name Device name.
     * @return Returns the status code for this function called.
     */
    NlErrCode SetLocalName(const std::string &name);

    /**
     * @brief Get paired devices.
     *
     * @param transport Adapter transport.
     * @param[out] pairedDevices Returns paired devices vector.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetPairedDevices(int transport, std::vector<NearlinkRemoteDevice> &pairedDevices) const;

    /**
     * @brief Set the connection status of the device
     *
     * @param connectionMode connection status of the device.
     * @param duration Duration of the state
     * @return Returns the status code for this function called.
     */
    NlErrCode SetConnectionMode(int32_t connectionMode, int32_t duration);

    /**
     * @brief Remove pair.
     *
     * @param device Remote device address, address is little endian order.
     * @return Returns the status code for this function called.
     */
    NlErrCode RemovePair(const NearlinkRemoteDevice &device);

    /**
     * @brief Remove all pairs.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode RemoveAllPairs();

    /**
     * @brief Get local link role if acb is connected.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode GetLinkRole(const NearlinkRemoteDevice &device, uint8_t &role) const;

    /**
     * @brief Register remote device observer.
     *
     * @param observer Class NearlinkRemoteDeviceObserver pointer to register observer.
     * @return Returns the status code for this function called.
     */
    NlErrCode RegisterRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer);

    /**
     * @brief Deregister remote device observer.
     *
     * @param observer Class NearlinkRemoteDeviceObserver pointer to deregister observer.
     * @return Returns the status code for this function called.
     */
    NlErrCode DeregisterRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer);

    /**
     * @brief Get max advertising data length.
     *
     * @param[out] maxAdvDataLen Returns max advertising data length.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetSleMaxAdvertisingDataLength(uint32_t &maxAdvDataLen) const;

    void LoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject);

    void LoadSystemAbilityFail();

    /**
     * @brief Get local profile uuids.
     *
     * @param[out] uuids Returns local profile uuids, uuid is big endian order.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetLocalProfileUuids(std::vector<std::string> &uuids);

    /**
     * @brief Connects all allowed nearlink profiles between the local and remote device.
     *
     * @param remoteAddr remote device addr, remoteAddr is little endian order.
     * @return Returns the status code for this function called.
     */
    NlErrCode ConnectAllowedProfiles(const std::string &remoteAddr) const;

    /**
     * @brief Disconnects all allowed nearlink profiles between the local and remote device.
     *
     * @param remoteAddr remote device addr, remoteAddr is little endian order.
     * @return Returns the status code for this function called.
     */
    NlErrCode DisconnectAllowedProfiles(const std::string &remoteAddr) const;

    /**
     * @brief Update a virtual connection device
     *
     * @param cmd cmd of add or delete.
     *        UpdateVirtualDeviceCmd::NL_SLE_VIRTUAL_DEVICE_CMD_ADD : add;
     *        UpdateVirtualDeviceCmd::NL_SLE_VIRTUAL_DEVICE_CMD_DELETE : delete.
     * @param address virtual connection device address
     * @return Returns the status code for this function called.
     */
    NlErrCode UpdateSleVirtualDevice(int32_t cmd, const std::string &address);

    NlErrCode UpdateRefusePolicy(const int32_t protocolType, const int32_t pid, const int64_t refuseTime);

    /**
     * @brief Check whether the current HAP has the permission.
     *
     * @param permission permission to be checked.
     * @return Returns the status code for this function called.
     */
    NlErrCode CheckPermissionForNapi(const std::string &permission, bool &isGranted);

    NlErrCode GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr);
    NlErrCode GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr);
    NlErrCode SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr);

    bool IsFeatureSupported(SleFeatureSupported feature);

    bool IsConnectionExist();
    /**
     * @brief Check whether the BAS service is supported.
     *
     * @return Returns the isSupport true is support,otherwise false
     */
    bool IsBasSupported();
private:
    NearlinkHost();
    ~NearlinkHost();
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkHost);
    struct impl;
    std::shared_ptr<impl> pimpl;
};

} // namespace Nearlink
} // namespace OHOS

#endif  // nearlink_HOST_H