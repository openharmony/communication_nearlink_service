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

#include "nearlink_host.h"
#include "nearlink_base_remote_device.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief base.so implementation class of framework host device.
 *
 * Kept in sync with NearlinkHost declared in nearlink_host.h; observers and
 * shared types are reused from that header.
 */
class NEARLINK_API NearlinkBaseHost {
public:
    static NearlinkBaseHost &GetInstance();

    NlErrCode RegisterObserver(std::shared_ptr<NearlinkHostObserver> observer);
    NlErrCode DeregisterObserver(std::shared_ptr<NearlinkHostObserver> observer);
    NlErrCode RegisterBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer);
    NlErrCode DeregisterBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer);
    NlErrCode RegisterRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer);
    NlErrCode DeregisterRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer);
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
    NlErrCode RegisterRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer);
    NlErrCode DeregisterRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer);
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
