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

#ifndef NEARLINK_HOST_BASE_H
#define NEARLINK_HOST_BASE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "nearlink_def.h"
#include "nearlink_def_types.h"
#include "nearlink_errorcode.h"
#include "nearlink_uuid.h"
#include "nearlink_base_remote_device.h"
#include "refbase.h"

namespace OHOS { class IRemoteObject; }
namespace OHOS {
namespace Nearlink {

class NearlinkBaseHostObserver {
public:
    virtual ~NearlinkBaseHostObserver() = default;

    virtual void OnStateChanged(const int transport, const int status) = 0;
    virtual void OnFullStateChanged(const int transport, const int status){};
    virtual void OnPairConfirmed(const NearlinkBaseRemoteDevice &device, int reqType, int number){};
    virtual void OnDeviceNameChanged(const std::string &deviceName){};
    virtual void OnDeviceAddrChanged(const std::string &address){};
};

class NearlinkBaseRemoteDeviceObserver {
public:
    virtual ~NearlinkBaseRemoteDeviceObserver() = default;

    virtual void OnAcbStateChanged(const NearlinkBaseRemoteDevice &device, int state, int reason) = 0;
    virtual void OnPairingRequest(const NearlinkBaseRemoteDevice &device, const std::string &passkey, int type) = 0;
    virtual void OnPairStatusChanged(
        const NearlinkBaseRemoteDevice &device, int preState, int state, int reason) = 0;
    virtual void OnConnectionStateChanged(
        const NearlinkBaseRemoteDevice &device, int preState, int state, int reason) = 0;
    virtual void OnRemoteUuidChanged(const NearlinkBaseRemoteDevice &device, const std::vector<UUID> &uuids) = 0;
    virtual void OnRemoteNameChanged(
        const NearlinkBaseRemoteDevice &device, const std::string &deviceName) = 0;
    virtual void OnRemoteAliasChanged(const NearlinkBaseRemoteDevice &device, const std::string &alias) = 0;
    virtual void OnReadRemoteRssiEvent(const NearlinkBaseRemoteDevice &device, int rssi, int status) = 0;
    virtual void OnLinkFreqBandChanged(const NearlinkBaseRemoteDevice &device, int32_t freqBand){};
};

class NearlinkBaseRemoteDeviceBatteryObserver {
public:
    virtual void OnGetBatteryLevelEvent(const NearlinkBaseRemoteDevice &device, int32_t batteryLevel){};
    virtual void OnBatteryLevelChanged(const NearlinkBaseRemoteDevice &device, int32_t batteryLevel){};
};

class NearlinkBaseRemoteDeviceRssiObserver {
public:
    virtual ~NearlinkBaseRemoteDeviceRssiObserver() = default;

    virtual void OnReadRemoteRssiEvent(const NearlinkBaseRemoteDevice &device, int rssi, int status){};
};

/**
 * @brief base.so implementation class of framework host device.
 *
 * Kept in sync with NearlinkHost declared in nearlink_host.h.
 */
class NEARLINK_API NearlinkBaseHost {
public:
    static NearlinkBaseHost &GetInstance();

    NlErrCode RegisterObserver(std::shared_ptr<NearlinkBaseHostObserver> observer);
    NlErrCode DeregisterObserver(std::shared_ptr<NearlinkBaseHostObserver> observer);
    NlErrCode RegisterBatteryObserver(std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver> observer);
    NlErrCode DeregisterBatteryObserver(std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver> observer);
    NlErrCode RegisterRssiObserver(std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver> observer);
    NlErrCode DeregisterRssiObserver(std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver> observer);
    NlErrCode LoadNearlinkSa();
    NlErrCode EnableNl(const SleAutoConnectPolicy autoConnPolicy = SleAutoConnectPolicy::AUTO_CONN_GENERAL);
    NlErrCode EnableNlAsync(const SleAutoConnectPolicy autoConnPolicy = SleAutoConnectPolicy::AUTO_CONN_GENERAL);
    NlErrCode DisableNl();
    NlErrCode DisableNlToOff();
    NlErrCode EnableNlToHalf();
    NlErrCode EnableNlToHalfAsync();
    SleStateID GetSleFullState();
    bool IsSleEnabled() const;
    bool IsSleHalfDisabled() const;
    bool IsSleDisabled() const;
    bool IsSleAvailableToCaller() const;
    bool IsNearlinkSupport() const;
    bool IsNearlinkSupportFrame4() const;
    bool IsNearlinkAudioSupport() const;
    NlErrCode NearlinkFactoryReset();
    NlErrCode SetFreqHopping(const std::vector<uint8_t> &freq);
    NlErrCode GetAdapterConnectState(int &state) const;
    NlErrCode GetProfileConnState(const std::string &remoteAddr, int &connState) const;
    // Declared but never implemented, kept in sync with the pre-split framework surface;
    // no bridge forwarding is established for them (see nearlink_host.h).
    NlErrCode GetLocalSupportedUuids(std::vector<UUID> &uuids);
    NlErrCode GetLocalDeviceAppearance(int &deviceAppearance) const;
    NlErrCode SetLocalDeviceAppearance(int appearance);
    NlErrCode GetLocalProfileUuids(std::vector<std::string> &uuids);
    NlErrCode GetLocalAddress(std::string &localDeviceAddres) const;
    NlErrCode GetLocalName(std::string &localDeviceName) const;
    NlErrCode SetLocalName(const std::string &name);
    NlErrCode GetPairedDevices(int transport, std::vector<NearlinkBaseRemoteDevice> &pairedDevices) const;
    NlErrCode SetConnectionMode(int32_t connectionMode, int32_t duration);
    NlErrCode RemovePair(const NearlinkBaseRemoteDevice &device);
    NlErrCode RemoveAllPairs();
    NlErrCode GetLinkRole(const NearlinkBaseRemoteDevice &device, uint8_t &role) const;
    NlErrCode RegisterRemoteDeviceObserver(std::shared_ptr<NearlinkBaseRemoteDeviceObserver> observer);
    NlErrCode DeregisterRemoteDeviceObserver(std::shared_ptr<NearlinkBaseRemoteDeviceObserver> observer);
    NlErrCode GetSleMaxAdvertisingDataLength(uint32_t &maxAdvDataLen) const;

    void LoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject);
    void LoadSystemAbilityFail();

    NlErrCode ConnectAllowedProfiles(const std::string &remoteAddr) const;
    NlErrCode DisconnectAllowedProfiles(const std::string &remoteAddr) const;
    NlErrCode UpdateSleVirtualDevice(int32_t cmd, const std::string &address);
    NlErrCode UpdateRefusePolicy(const int32_t protocolType, const int32_t pid, const int64_t refuseTime);
    NlErrCode CheckPermissionForNapi(const std::string &permission, bool &isGranted);
    NlErrCode GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr);
    NlErrCode GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr);
    NlErrCode SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr);

    bool IsFeatureSupported(SleFeatureSupported feature);
    bool IsConnectionExist();
    bool IsBasSupported();
private:
    NearlinkBaseHost();
    ~NearlinkBaseHost();
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkBaseHost);
    struct impl;
    std::shared_ptr<impl> pimpl;
};

} // namespace Nearlink
} // namespace OHOS

#endif  // NEARLINK_HOST_BASE_H
