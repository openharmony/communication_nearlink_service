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
#include "base/nearlink_host_core.h"
#include "base/nearlink_remote_device_core.h"

namespace OHOS {
namespace Nearlink {

namespace {
inline NearlinkHostCore *ToCore(NearlinkHost *self)
{
    return reinterpret_cast<NearlinkHostCore *>(self);
}
inline const NearlinkHostCore *ToCore(const NearlinkHost *self)
{
    return reinterpret_cast<const NearlinkHostCore *>(self);
}
}
NearlinkHost& NearlinkHost::GetInstance()
{
    return reinterpret_cast<NearlinkHost&>(NearlinkHostCore::GetInstance());
}

NlErrCode NearlinkHost::RegisterObserver(std::shared_ptr<NearlinkHostObserver> observer)
{
    return ToCore(this)->RegisterObserver(observer);
}

NlErrCode NearlinkHost::DeregisterObserver(std::shared_ptr<NearlinkHostObserver> observer)
{
    return ToCore(this)->DeregisterObserver(observer);
}

NlErrCode NearlinkHost::RegisterBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer)
{
    return ToCore(this)->RegisterBatteryObserver(observer);
}

NlErrCode NearlinkHost::DeregisterBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer)
{
    return ToCore(this)->DeregisterBatteryObserver(observer);
}

NlErrCode NearlinkHost::RegisterRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer)
{
    return ToCore(this)->RegisterRssiObserver(observer);
}

NlErrCode NearlinkHost::DeregisterRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer)
{
    return ToCore(this)->DeregisterRssiObserver(observer);
}

NlErrCode NearlinkHost::LoadNearlinkSa()
{
    return ToCore(this)->LoadNearlinkSa();
}

NlErrCode NearlinkHost::EnableNl(const SleAutoConnectPolicy autoConnPolicy)
{
    return ToCore(this)->EnableNl(autoConnPolicy);
}

NlErrCode NearlinkHost::EnableNlAsync(const SleAutoConnectPolicy autoConnPolicy)
{
    return ToCore(this)->EnableNlAsync(autoConnPolicy);
}

NlErrCode NearlinkHost::DisableNl()
{
    return ToCore(this)->DisableNl();
}

NlErrCode NearlinkHost::DisableNlToOff()
{
    return ToCore(this)->DisableNlToOff();
}

NlErrCode NearlinkHost::EnableNlToHalf()
{
    return ToCore(this)->EnableNlToHalf();
}

NlErrCode NearlinkHost::EnableNlToHalfAsync()
{
    return ToCore(this)->EnableNlToHalfAsync();
}

SleStateID NearlinkHost::GetSleFullState()
{
    return ToCore(this)->GetSleFullState();
}

bool NearlinkHost::IsSleEnabled() const
{
    return ToCore(this)->IsSleEnabled();
}

bool NearlinkHost::IsSleHalfDisabled() const
{
    return ToCore(this)->IsSleHalfDisabled();
}

bool NearlinkHost::IsSleDisabled() const
{
    return ToCore(this)->IsSleDisabled();
}

bool NearlinkHost::IsSleAvailableToCaller() const
{
    return ToCore(this)->IsSleAvailableToCaller();
}

bool NearlinkHost::IsNearlinkSupport() const
{
    return ToCore(this)->IsNearlinkSupport();
}

bool NearlinkHost::IsNearlinkSupportFrame4() const
{
    return ToCore(this)->IsNearlinkSupportFrame4();
}

bool NearlinkHost::IsNearlinkAudioSupport() const
{
    return ToCore(this)->IsNearlinkAudioSupport();
}

NlErrCode NearlinkHost::NearlinkFactoryReset()
{
    return ToCore(this)->NearlinkFactoryReset();
}

NlErrCode NearlinkHost::SetFreqHopping(const std::vector<uint8_t> &freq)
{
    return ToCore(this)->SetFreqHopping(freq);
}

NlErrCode NearlinkHost::GetAdapterConnectState(int &state) const
{
    return ToCore(this)->GetAdapterConnectState(state);
}

NlErrCode NearlinkHost::GetProfileConnState(const std::string &remoteAddr, int &connState) const
{
    return ToCore(this)->GetProfileConnState(remoteAddr, connState);
}

NlErrCode NearlinkHost::GetLocalAddress(std::string &localDeviceAddres) const
{
    return ToCore(this)->GetLocalAddress(localDeviceAddres);
}

NlErrCode NearlinkHost::GetLocalName(std::string &localDeviceName) const
{
    return ToCore(this)->GetLocalName(localDeviceName);
}

NlErrCode NearlinkHost::SetLocalName(const std::string &name)
{
    return ToCore(this)->SetLocalName(name);
}

NlErrCode NearlinkHost::GetPairedDevices(int transport, std::vector<NearlinkRemoteDevice> &pairedDevices) const
{
    return ToCore(this)->GetPairedDevices(transport,
    reinterpret_cast<std::vector<NearlinkRemoteDeviceCore>&>(pairedDevices));
}

NlErrCode NearlinkHost::SetConnectionMode(int32_t connectionMode, int32_t duration)
{
    return ToCore(this)->SetConnectionMode(connectionMode, duration);
}

NlErrCode NearlinkHost::RemovePair(const NearlinkRemoteDevice &device)
{
    return ToCore(this)->RemovePair(reinterpret_cast<const NearlinkRemoteDeviceCore&>(device));
}

NlErrCode NearlinkHost::RemoveAllPairs()
{
    return ToCore(this)->RemoveAllPairs();
}

NlErrCode NearlinkHost::GetLinkRole(const NearlinkRemoteDevice &device, uint8_t &role) const
{
    return ToCore(this)->GetLinkRole(reinterpret_cast<const NearlinkRemoteDeviceCore&>(device), role);
}

NlErrCode NearlinkHost::RegisterRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer)
{
    return ToCore(this)->RegisterRemoteDeviceObserver(observer);
}

NlErrCode NearlinkHost::DeregisterRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer)
{
    return ToCore(this)->DeregisterRemoteDeviceObserver(observer);
}

NlErrCode NearlinkHost::GetSleMaxAdvertisingDataLength(uint32_t &maxAdvDataLen) const
{
    return ToCore(this)->GetSleMaxAdvertisingDataLength(maxAdvDataLen);
}

void NearlinkHost::LoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject)
{
    ToCore(this)->LoadSystemAbilitySuccess(remoteObject);
}

void NearlinkHost::LoadSystemAbilityFail()
{
    ToCore(this)->LoadSystemAbilityFail();
}

NlErrCode NearlinkHost::ConnectAllowedProfiles(const std::string &remoteAddr) const
{
    return ToCore(this)->ConnectAllowedProfiles(remoteAddr);
}

NlErrCode NearlinkHost::DisconnectAllowedProfiles(const std::string &remoteAddr) const
{
    return ToCore(this)->DisconnectAllowedProfiles(remoteAddr);
}

NlErrCode NearlinkHost::UpdateSleVirtualDevice(int32_t cmd, const std::string &address)
{
    return ToCore(this)->UpdateSleVirtualDevice(cmd, address);
}

NlErrCode NearlinkHost::UpdateRefusePolicy(const int32_t protocolType, const int32_t pid, const int64_t refuseTime)
{
    return ToCore(this)->UpdateRefusePolicy(protocolType, pid, refuseTime);
}

NlErrCode NearlinkHost::CheckPermissionForNapi(const std::string &permission, bool &isGranted)
{
    return ToCore(this)->CheckPermissionForNapi(permission, isGranted);
}

NlErrCode NearlinkHost::GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr)
{
    return ToCore(this)->GetSleAddrByBtAddr(btAddr, sleAddr);
}

NlErrCode NearlinkHost::GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr)
{
    return ToCore(this)->GetBtAddrBySleAddr(sleAddr, btAddr);
}

NlErrCode NearlinkHost::SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr)
{
    return ToCore(this)->SetBtAddrBySleAddr(sleAddr, btAddr);
}

bool NearlinkHost::IsFeatureSupported(SleFeatureSupported feature)
{
    return ToCore(this)->IsFeatureSupported(feature);
}

bool NearlinkHost::IsConnectionExist()
{
    return ToCore(this)->IsConnectionExist();
}

bool NearlinkHost::IsBasSupported()
{
    return ToCore(this)->IsBasSupported();
}

// 镜像构造/析构保持空实现：对象实体由 base.so 中的 Core 类构造，
// 镜像入口通过 ToCore() cast 直调 Core 真身方法。
NearlinkHost::NearlinkHost()
{
}

NearlinkHost::~NearlinkHost()
{
}

}  // namespace Nearlink
}  // namespace OHOS
