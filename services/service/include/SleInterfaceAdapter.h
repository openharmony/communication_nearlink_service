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
#ifndef SLE_INTERFACE_ADAPTER_H
#define SLE_INTERFACE_ADAPTER_H

#include <string>
#include <vector>
#include "nearlink_def.h"
#include "sle_uuid.h"
#include "raw_address.h"
#include "context.h"
#include "nearlink_errorcode.h"
#include "nearlink_device_model.h"

/**
 * @brief nearlink adapter name Define
 */
const std::string ADAPTER_NAME_SLB = "SlbAdapter";
const std::string ADAPTER_NAME_SLE = "SleAdapter";

/**
 * @brief forward declaration for class Context in namespace utility
 */

namespace OHOS {
namespace Nearlink {
/**
 * @brief Represents sle, including the common functions.
 *
 * @since 6
 */
class SleInterfaceAdapter {
public:
    /**
     * @brief A destructor used to delete the <b>SleInterfaceAdapter</b> instance.
     *
     * @since 6
     */
    virtual ~SleInterfaceAdapter() = default;

    /// gap
    /**
     * @brief Get local device address.
     *
     * @return Returns local device address.
     * @since 6
     */
    virtual std::string GetLocalAddress() const = 0;

    /**
     * @brief Get local device name.
     *
     * @return Returns local device name.
     * @since 6
     */
    virtual std::string GetLocalName() const = 0;

    /**
     * @brief Set local device name.
     *
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool SetLocalName(const std::string &name) const = 0;
    /**
     * @brief SleFreqHopping.
     *
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool SleFreqHopping(const std::vector<uint8_t> &freq) = 0;

    /**
     * @brief Set local device bondable mode.
     *
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool SetBondableMode(int mode) const = 0;

    /**
     * @brief Get local device bondable mode.
     *
     * @return Returns local device bondable mode.
     * @since 6
     */
    virtual int GetBondableMode() const = 0;

    /// remote device information
    /**
     * @brief Get remote device name.
     *
     * @param device Remote device address.
     * @return Returns remote device name.
     * @since 6
     */
    virtual std::string GetDeviceName(const RawAddress &device) const = 0;

    /**
     * @brief Get remote Alias device name.
     *
     * @param device Remote device address.
     * @return Returns remote Alias device name.
     * @since 6
     */
    virtual std::string GetAliasName(const RawAddress &device) const = 0;

    /**
     * @brief Get remote Alias device appearance.
     *
     * @param device Remote device address.
     * @return Returns remote device appearance.
     * @since 6
     */
    virtual int GetDeviceAppearance(const RawAddress &device) const = 0;

    virtual uint16_t GetDeviceVendorId(const RawAddress &device) const = 0;
    virtual uint16_t GetDeviceProductId(const RawAddress &device) const = 0;
    virtual uint16_t GetDeviceVersion(const RawAddress &device) const = 0;
    virtual DeviceModel GetDeviceModel(const RawAddress &device) const = 0;

    /**
     * @brief Set remote Alias device name.
     *
     * @param device Remote device address.
     * @param device Remote Alias name.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool SetAliasName(const RawAddress &device, const std::string &name) const = 0;

    /**
     * @brief Get remote device uuids.
     *
     * @param device Remote device address.
     * @return Returns remote device uuids vector.
     * @since 6
     */
    virtual std::vector<Uuid> GetDeviceUuids(const RawAddress &device) const = 0;

    /// pair
    /**
     * @brief Get paired devices.
     *
     * @return Returns paired devices vector.
     * @since 6
     */
    virtual std::vector<RawAddress> GetPairedDevices() const = 0;

    /**
     * @brief Get connected devices.
     *
     * @return Returns connected devices vector.
     * @since 6
     */
    virtual std::vector<RawAddress> GetConnectedDevices() const = 0;

    /**
     * @brief set connection mode for sle.
     *
     * @param connectionModem connection mode.
     * @param duration duration in seconds for the setting mode, if 0 means that unlimited.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual void SetSleConnectionMode(int32_t connectionMode, int32_t duration) = 0;

    /**
     * @brief Start Pair.
     *
     * @param device Remote device address.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool StartPair(const RawAddress &device) = 0;

    /**
     * @brief Start Credible Pair.
     *
     * @param device Remote device address.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool StartCrediblePair(const RawAddress &device) = 0;

    /**
     * @brief Disconnect Action.
     *
     * @param device Remote device address.
     * @param discReason Disconnect reason.
     * @return Returns <b>true</b> if disconnect action is successful;
     *         returns <b>false</b> if disconnect action is fails.
     * @since 6
     */
     virtual bool DisconnectAction(const RawAddress &device, uint8_t discReason) const = 0;

    /**
     * @brief ConnectAllProfile.
     *
     * @param device Remote device address.
     * @return Returns <b>true</b> if ConnectAllProfile is successful;
     *         returns <b>false</b> if ConnectAllProfile is fails.
     * @since 6
     */
    virtual bool ConnectAllProfile(const RawAddress &device) = 0;

    /**
     * @brief RemoveBgConnDevice.
     *
     * @param device Remote device address.
     * @since 6
     */
    virtual void RemoveBgConnDevice(const std::string &device) const = 0;

    /**
     * @brief DisconnectAllProfile.
     *
     * @param device Remote device address.
     * @return Returns <b>true</b> if ConnectAllProfile is successful;
     *         returns <b>false</b> if ConnectAllProfile is fails.
     * @since 6
     */
    virtual bool DisconnectAllProfile(const RawAddress &device) = 0;

    /**
     * @brief Check if device was bonded from local.
     *
     * @param device Remote device address.
     * @return Returns <b>true</b> if device was bonded from local;
     *         returns <b>false</b> if device was not bonded from local.
     * @since 6
     */
    virtual bool IsBondedFromLocal(const RawAddress &device) const = 0;

    /**
     * @brief Cancel pair operation.
     *
     * @param device Remote device address.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool CancelPairing(const RawAddress &device) = 0;

    /**
     * @brief Remove pair.
     *
     * @param device Remote device address.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool RemovePair(const RawAddress &device) = 0;

    /**
     * @brief Remove all pairs.
     *
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool RemoveAllPairs() = 0;

    /**
     * @brief Set Pairing Confirmation.
     *
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool SetPairingConfirmation(const RawAddress &device) const = 0;
    /**
     * @brief Set Pairing PassCode.
     *
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool SetPairingPassCode(const RawAddress &device, const std::string &passCode) = 0;

    /**
     * @brief Get device pair state.
     *
     * @param device Remote device address.
     * @return Returns device pair state.
     * @since 6
     */
    virtual int GetPairState(const RawAddress &device) const = 0;

    /**
     * @brief Get device connection state.
     *
     * @param device Remote device address.
     * @return Returns device connect state.
     * @since 6
     */
    virtual int GetAcbState(const RawAddress &device) const = 0;

    /**
     * @brief Get the number of ACB connections.
     *
     * @return Returns number of ACB connections.
     */
    virtual uint32_t GetAcbCount() const = 0;

    /**
     * @brief Check device pair request reply.
     *
     * @param device Remote device address.
     * @param accept Set gap accept flag.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool PairRequestReply(const RawAddress &device, bool accept) const = 0;

    /// other
    /**
     * @brief Check if device acb connected.
     *
     * @param device Remote device address.
     * @return Returns <b>true</b> if device acb connected;
     *         returns <b>false</b> if device does not acb connect.
     * @since 6
     */
    virtual bool IsAcbConnected(const RawAddress &device) const = 0;

    /**
     * @brief Check if device acb Encrypted.
     *
     * @param device Remote device address.
     * @return Returns <b>true</b> if device acb Encrypted;
     *         returns <b>false</b> if device does not acb Encrypt.
     * @since 6
     */
    virtual bool IsAcbEncrypted(const RawAddress &device) const = 0;

    /**
     * @brief Get local link role when acb connected.
     *
     * @param device Remote device address.
     * @return Returns local link role.
     * @since 6
     */
    virtual uint8_t GetLinkRole(const RawAddress &device) const = 0;

    /**
     * @brief Get utility::Context pointer for adapter.
     *
     * @return Returns the pointer for adapter.
     * @since 6
     */
    virtual utility::Context *GetContext() = 0;

    virtual bool FactoryReset() const = 0;

    virtual void CancelAllConnection() = 0;

    virtual int GetProfileConnState(const RawAddress &device) = 0;

    virtual bool WrapperCdsmGetAllMemberInfo(const RawAddress &rawAddr, std::vector<RawAddress> &cdsmInfoAddr) = 0;

    virtual bool UpdateSleVirtualDevice(int32_t cmd, const RawAddress &device) = 0;
    virtual bool GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr) const = 0;
    virtual bool GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr) const = 0;
    virtual bool SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr) const = 0;
    virtual bool IsScanConnTypeAndFrameType4(const RawAddress &device, uint8_t connCompleteType) const = 0;
    virtual void UpdateDeviceModelInfo(const std::string &address, const DeviceModel &model,
        const std::string &newModelId) = 0;
    virtual void DisconnectAllProfileForSilentPort(const RawAddress &device) = 0;
    virtual void ConnectAcb(const RawAddress &device) = 0;
    virtual void RemoveNotPairedCloudDevice(const RawAddress &device) const = 0;
    virtual bool DisconnectAcb(const RawAddress &device, uint8_t discReason) const = 0;
    virtual void ClearBgConnDevice() const = 0;
    virtual bool UpdateRefusePolicy(const int32_t protocolType, const int32_t pid, const int64_t refuseTime) const = 0;
    virtual void DelConnFrameType(const std::string &addr) = 0;
    virtual void SetConnFrameType(const std::string &addr, uint8_t frameType) = 0;
    virtual bool GetConnFrameType(const std::string &addr, uint8_t &frameType) const = 0;
    virtual void NotifyPairStatusChanged(const RawAddress &device, int preStatus, int status, int reason) const = 0;
    virtual void NotifyConnectionStateChanged(
        const RawAddress &device, const SleConnectionChangedParam &connChangedParam) const = 0;
    virtual void SetPhy(const RawAddress &device, uint8_t frameType, uint8_t phyType) = 0;
};
}  // namespace Sle
}  // namespace OHOS

#endif  // SLE_INTERFACE_ADAPTER_H