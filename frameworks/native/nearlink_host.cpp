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

#include "nearlink_host.h"
#include "nearlink_base_host.h"
#include "nearlink_base_remote_device.h"

namespace OHOS {
namespace Nearlink {

namespace {
inline NearlinkBaseHost *ToBase(NearlinkHost *self)
{
    return reinterpret_cast<NearlinkBaseHost *>(self);
}
inline const NearlinkBaseHost *ToBase(const NearlinkHost *self)
{
    return reinterpret_cast<const NearlinkBaseHost *>(self);
}
}
NearlinkHost& NearlinkHost::GetInstance()
{
    return reinterpret_cast<NearlinkHost&>(NearlinkBaseHost::GetInstance());
}

NlErrCode NearlinkHost::RegisterObserver(std::shared_ptr<NearlinkHostObserver> observer)
{
    return ToBase(this)->RegisterObserver(observer);
}

NlErrCode NearlinkHost::DeregisterObserver(std::shared_ptr<NearlinkHostObserver> observer)
{
    return ToBase(this)->DeregisterObserver(observer);
}

NlErrCode NearlinkHost::RegisterBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer)
{
    return ToBase(this)->RegisterBatteryObserver(observer);
}

NlErrCode NearlinkHost::DeregisterBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer)
{
    return ToBase(this)->DeregisterBatteryObserver(observer);
}

NlErrCode NearlinkHost::RegisterRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer)
{
    return ToBase(this)->RegisterRssiObserver(observer);
}

NlErrCode NearlinkHost::DeregisterRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer)
{
    return ToBase(this)->DeregisterRssiObserver(observer);
}

NlErrCode NearlinkHost::LoadNearlinkSa()
{
    return ToBase(this)->LoadNearlinkSa();
}

NlErrCode NearlinkHost::EnableNl(const SleAutoConnectPolicy autoConnPolicy)
{
    return ToBase(this)->EnableNl(autoConnPolicy);
}

NlErrCode NearlinkHost::EnableNlAsync(const SleAutoConnectPolicy autoConnPolicy)
{
    return ToBase(this)->EnableNlAsync(autoConnPolicy);
}

NlErrCode NearlinkHost::DisableNl()
{
    return ToBase(this)->DisableNl();
}

NlErrCode NearlinkHost::DisableNlToOff()
{
    return ToBase(this)->DisableNlToOff();
}

NlErrCode NearlinkHost::EnableNlToHalf()
{
    return ToBase(this)->EnableNlToHalf();
}

NlErrCode NearlinkHost::EnableNlToHalfAsync()
{
    return ToBase(this)->EnableNlToHalfAsync();
}

SleStateID NearlinkHost::GetSleFullState()
{
    return ToBase(this)->GetSleFullState();
}

bool NearlinkHost::IsSleEnabled() const
{
    return ToBase(this)->IsSleEnabled();
}

bool NearlinkHost::IsSleHalfDisabled() const
{
    return ToBase(this)->IsSleHalfDisabled();
}

bool NearlinkHost::IsSleDisabled() const
{
    return ToBase(this)->IsSleDisabled();
}

bool NearlinkHost::IsSleAvailableToCaller() const
{
    return ToBase(this)->IsSleAvailableToCaller();
}

bool NearlinkHost::IsNearlinkSupport() const
{
    return ToBase(this)->IsNearlinkSupport();
}

bool NearlinkHost::IsNearlinkSupportFrame4() const
{
    return ToBase(this)->IsNearlinkSupportFrame4();
}

bool NearlinkHost::IsNearlinkAudioSupport() const
{
    return ToBase(this)->IsNearlinkAudioSupport();
}

NlErrCode NearlinkHost::NearlinkFactoryReset()
{
    return ToBase(this)->NearlinkFactoryReset();
}

NlErrCode NearlinkHost::SetFreqHopping(const std::vector<uint8_t> &freq)
{
    return ToBase(this)->SetFreqHopping(freq);
}

NlErrCode NearlinkHost::GetAdapterConnectState(int &state) const
{
    return ToBase(this)->GetAdapterConnectState(state);
}

NlErrCode NearlinkHost::GetProfileConnState(const std::string &remoteAddr, int &connState) const
{
    return ToBase(this)->GetProfileConnState(remoteAddr, connState);
}

NlErrCode NearlinkHost::GetLocalAddress(std::string &localDeviceAddres) const
{
    return ToBase(this)->GetLocalAddress(localDeviceAddres);
}

NlErrCode NearlinkHost::GetLocalName(std::string &localDeviceName) const
{
    return ToBase(this)->GetLocalName(localDeviceName);
}

NlErrCode NearlinkHost::SetLocalName(const std::string &name)
{
    return ToBase(this)->SetLocalName(name);
}

NlErrCode NearlinkHost::GetPairedDevices(int transport, std::vector<NearlinkRemoteDevice> &pairedDevices) const
{
    return ToBase(this)->GetPairedDevices(transport,
    reinterpret_cast<std::vector<NearlinkBaseRemoteDevice>&>(pairedDevices));
}

NlErrCode NearlinkHost::SetConnectionMode(int32_t connectionMode, int32_t duration)
{
    return ToBase(this)->SetConnectionMode(connectionMode, duration);
}

NlErrCode NearlinkHost::RemovePair(const NearlinkRemoteDevice &device)
{
    return ToBase(this)->RemovePair(reinterpret_cast<const NearlinkBaseRemoteDevice&>(device));
}

NlErrCode NearlinkHost::RemoveAllPairs()
{
    return ToBase(this)->RemoveAllPairs();
}

NlErrCode NearlinkHost::GetLinkRole(const NearlinkRemoteDevice &device, uint8_t &role) const
{
    return ToBase(this)->GetLinkRole(reinterpret_cast<const NearlinkBaseRemoteDevice&>(device), role);
}

NlErrCode NearlinkHost::RegisterRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer)
{
    return ToBase(this)->RegisterRemoteDeviceObserver(observer);
}

NlErrCode NearlinkHost::DeregisterRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer)
{
    return ToBase(this)->DeregisterRemoteDeviceObserver(observer);
}

NlErrCode NearlinkHost::GetSleMaxAdvertisingDataLength(uint32_t &maxAdvDataLen) const
{
    return ToBase(this)->GetSleMaxAdvertisingDataLength(maxAdvDataLen);
}

void NearlinkHost::LoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject)
{
    ToBase(this)->LoadSystemAbilitySuccess(remoteObject);
}

void NearlinkHost::LoadSystemAbilityFail()
{
    ToBase(this)->LoadSystemAbilityFail();
}

NlErrCode NearlinkHost::ConnectAllowedProfiles(const std::string &remoteAddr) const
{
    return ToBase(this)->ConnectAllowedProfiles(remoteAddr);
}

NlErrCode NearlinkHost::DisconnectAllowedProfiles(const std::string &remoteAddr) const
{
    return ToBase(this)->DisconnectAllowedProfiles(remoteAddr);
}

NlErrCode NearlinkHost::UpdateSleVirtualDevice(int32_t cmd, const std::string &address)
{
    return ToBase(this)->UpdateSleVirtualDevice(cmd, address);
}

NlErrCode NearlinkHost::UpdateRefusePolicy(const int32_t protocolType, const int32_t pid, const int64_t refuseTime)
{
    return ToBase(this)->UpdateRefusePolicy(protocolType, pid, refuseTime);
}

NlErrCode NearlinkHost::CheckPermissionForNapi(const std::string &permission, bool &isGranted)
{
    return ToBase(this)->CheckPermissionForNapi(permission, isGranted);
}

NlErrCode NearlinkHost::GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr)
{
    return ToBase(this)->GetSleAddrByBtAddr(btAddr, sleAddr);
}

NlErrCode NearlinkHost::GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr)
{
    return ToBase(this)->GetBtAddrBySleAddr(sleAddr, btAddr);
}

NlErrCode NearlinkHost::SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr)
{
    return ToBase(this)->SetBtAddrBySleAddr(sleAddr, btAddr);
}

bool NearlinkHost::IsFeatureSupported(SleFeatureSupported feature)
{
    return ToBase(this)->IsFeatureSupported(feature);
}

bool NearlinkHost::IsConnectionExist()
{
    return ToBase(this)->IsConnectionExist();
}

bool NearlinkHost::IsBasSupported()
{
    return ToBase(this)->IsBasSupported();
}

NearlinkHost::NearlinkHost()
{
}

NearlinkHost::~NearlinkHost()
{
}

}  // namespace Nearlink
}  // namespace OHOS
