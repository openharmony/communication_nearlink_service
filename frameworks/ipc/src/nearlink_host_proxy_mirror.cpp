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

#include "nearlink_host_proxy.h"
#include "nearlink_host_proxy_core.h"

namespace OHOS {
namespace Nearlink {

namespace {
inline NearlinkHostProxyCore *ToCore(NearlinkHostProxy *self)
{
    return reinterpret_cast<NearlinkHostProxyCore *>(self);
}
}
NlErrCode NearlinkHostProxy::GetProfile(const std::string &name, sptr<IRemoteObject> &remoteProfile)
{
    return ToCore(this)->NearlinkHostProxyCore::GetProfile(name, remoteProfile);
}

NlErrCode NearlinkHostProxy::DisableSle()
{
    return ToCore(this)->NearlinkHostProxyCore::DisableSle();
}

NlErrCode NearlinkHostProxy::EnableSle(const SleAutoConnectPolicy autoConnPolicy)
{
    return ToCore(this)->NearlinkHostProxyCore::EnableSle(autoConnPolicy);
}

NlErrCode NearlinkHostProxy::DisableSleToOff()
{
    return ToCore(this)->NearlinkHostProxyCore::DisableSleToOff();
}

NlErrCode NearlinkHostProxy::EnableSleToHalf()
{
    return ToCore(this)->NearlinkHostProxyCore::EnableSleToHalf();
}

NlErrCode NearlinkHostProxy::GetSleFullState(int &sleCurrentState)
{
    return ToCore(this)->NearlinkHostProxyCore::GetSleFullState(sleCurrentState);
}

NlErrCode NearlinkHostProxy::IsSleEnabled(bool &isSleEnabled)
{
    return ToCore(this)->NearlinkHostProxyCore::IsSleEnabled(isSleEnabled);
}

NlErrCode NearlinkHostProxy::IsSleHalfDisabled(bool &isSleHalfDisabled)
{
    return ToCore(this)->NearlinkHostProxyCore::IsSleHalfDisabled(isSleHalfDisabled);
}

NlErrCode NearlinkHostProxy::IsSleDisabled(bool &isSleDisabled)
{
    return ToCore(this)->NearlinkHostProxyCore::IsSleDisabled(isSleDisabled);
}

NlErrCode NearlinkHostProxy::IsSleAvailableToCaller(bool &isSleAvailable)
{
    return ToCore(this)->NearlinkHostProxyCore::IsSleAvailableToCaller(isSleAvailable);
}

NlErrCode NearlinkHostProxy::GetAdapterConnectState(int32_t &state)
{
    return ToCore(this)->NearlinkHostProxyCore::GetAdapterConnectState(state);
}

NlErrCode NearlinkHostProxy::GetProfileConnState(const std::string &remoteAddr, int32_t &state)
{
    return ToCore(this)->NearlinkHostProxyCore::GetProfileConnState(remoteAddr, state);
}

NlErrCode NearlinkHostProxy::GetLocalName(std::string &name)
{
    return ToCore(this)->NearlinkHostProxyCore::GetLocalName(name);
}

NlErrCode NearlinkHostProxy::SetLocalName(const std::string &name)
{
    return ToCore(this)->NearlinkHostProxyCore::SetLocalName(name);
}

NlErrCode NearlinkHostProxy::GetLocalAddress(std::string &addr)
{
    return ToCore(this)->NearlinkHostProxyCore::GetLocalAddress(addr);
}

NlErrCode NearlinkHostProxy::GetPairedDevices(std::vector<NearlinkRawAddress> &pairedAddr)
{
    return ToCore(this)->NearlinkHostProxyCore::GetPairedDevices(pairedAddr);
}

NlErrCode NearlinkHostProxy::SetConnectionMode(int32_t connectionMode, int32_t duration)
{
    return ToCore(this)->NearlinkHostProxyCore::SetConnectionMode(connectionMode, duration);
}

NlErrCode NearlinkHostProxy::RemovePair(const sptr<NearlinkRawAddress> &device)
{
    return ToCore(this)->NearlinkHostProxyCore::RemovePair(device);
}

NlErrCode NearlinkHostProxy::RemoveAllPairs()
{
    return ToCore(this)->NearlinkHostProxyCore::RemoveAllPairs();
}

NlErrCode NearlinkHostProxy::RegisterSlePeripheralCallback(const sptr<INearlinkSlePeripheralObserver> &observer)
{
    return ToCore(this)->NearlinkHostProxyCore::RegisterSlePeripheralCallback(observer);
}

NlErrCode NearlinkHostProxy::DeregisterSlePeripheralCallback(const sptr<INearlinkSlePeripheralObserver> &observer)
{
    return ToCore(this)->NearlinkHostProxyCore::DeregisterSlePeripheralCallback(observer);
}

NlErrCode NearlinkHostProxy::RegisterSleAdapterObserver(const sptr<INearlinkHostObserver> &observer)
{
    return ToCore(this)->NearlinkHostProxyCore::RegisterSleAdapterObserver(observer);
}

NlErrCode NearlinkHostProxy::DeregisterSleAdapterObserver(const sptr<INearlinkHostObserver> &observer)
{
    return ToCore(this)->NearlinkHostProxyCore::DeregisterSleAdapterObserver(observer);
}

NlErrCode NearlinkHostProxy::RegisterDeviceBatteryObserver(const sptr<INearlinkDeviceBatteryObserver> &observer)
{
    return ToCore(this)->NearlinkHostProxyCore::RegisterDeviceBatteryObserver(observer);
}

NlErrCode NearlinkHostProxy::DeregisterDeviceBatteryObserver(const sptr<INearlinkDeviceBatteryObserver> &observer)
{
    return ToCore(this)->NearlinkHostProxyCore::DeregisterDeviceBatteryObserver(observer);
}

NlErrCode NearlinkHostProxy::RegisterDeviceRssiObserver(const sptr<INearlinkDeviceRssiObserver> &observer)
{
    return ToCore(this)->NearlinkHostProxyCore::RegisterDeviceRssiObserver(observer);
}

NlErrCode NearlinkHostProxy::DeregisterDeviceRssiObserver(const sptr<INearlinkDeviceRssiObserver> &observer)
{
    return ToCore(this)->NearlinkHostProxyCore::DeregisterDeviceRssiObserver(observer);
}

NlErrCode NearlinkHostProxy::GetSleMaxAdvertisingDataLength(uint32_t &maxAdvDataLen)
{
    return ToCore(this)->NearlinkHostProxyCore::GetSleMaxAdvertisingDataLength(maxAdvDataLen);
}

NlErrCode NearlinkHostProxy::GetDeviceName(int32_t transport, const std::string &address, std::string &name)
{
    return ToCore(this)->NearlinkHostProxyCore::GetDeviceName(transport, address, &name);
}

NlErrCode NearlinkHostProxy::GetDeviceAlias(int32_t transport, const std::string &address, std::string &alias)
{
    return ToCore(this)->NearlinkHostProxyCore::GetDeviceAlias(transport, address, &alias);
}

NlErrCode NearlinkHostProxy::SetDeviceAlias(int32_t transport, const std::string &address, const std::string &alias)
{
    return ToCore(this)->NearlinkHostProxyCore::SetDeviceAlias(transport, address, alias);
}

NlErrCode NearlinkHostProxy::GetPairState(int32_t transport, const std::string &address, int &pairState)
{
    return ToCore(this)->NearlinkHostProxyCore::GetPairState(transport, address, &pairState);
}

NlErrCode NearlinkHostProxy::StartPair(int32_t transport, const std::string &address)
{
    return ToCore(this)->NearlinkHostProxyCore::StartPair(transport, address);
}

NlErrCode NearlinkHostProxy::StartCrediblePair(int32_t transport, const std::string &address)
{
    return ToCore(this)->NearlinkHostProxyCore::StartCrediblePair(transport, address);
}

NlErrCode NearlinkHostProxy::CancelPairing(int32_t transport, const std::string &address)
{
    return ToCore(this)->NearlinkHostProxyCore::CancelPairing(transport, address);
}

NlErrCode NearlinkHostProxy::SetPairingConfirmation(int32_t transport, const std::string &address, bool cfm)
{
    return ToCore(this)->NearlinkHostProxyCore::SetPairingConfirmation(transport, address, cfm);
}

NlErrCode NearlinkHostProxy::SetPairingPassCode(int32_t transport, const std::string &address,
    const std::string &passCode)
{
    return ToCore(this)->NearlinkHostProxyCore::SetPairingPassCode(transport, address, passCode);
}

NlErrCode NearlinkHostProxy::IsBondedFromLocal(int32_t transport, const std::string &address,
    bool &isBondedFromLocal)
{
    return ToCore(this)->NearlinkHostProxyCore::IsBondedFromLocal(transport, address, &isBondedFromLocal);
}

NlErrCode NearlinkHostProxy::IsAcbConnected(int32_t transport, const std::string &address, bool &isAcbConnected)
{
    return ToCore(this)->NearlinkHostProxyCore::IsAcbConnected(transport, address, &isAcbConnected);
}

NlErrCode NearlinkHostProxy::IsAcbEncrypted(int32_t transport, const std::string &address, bool &isAcbEncrypted)
{
    return ToCore(this)->NearlinkHostProxyCore::IsAcbEncrypted(transport, address, &isAcbEncrypted);
}

NlErrCode NearlinkHostProxy::GetLinkRole(int32_t transport, const std::string &address, uint8_t &role)
{
    return ToCore(this)->NearlinkHostProxyCore::GetLinkRole(transport, address, &role);
}

NlErrCode NearlinkHostProxy::GetDeviceUuids(int32_t transport, const std::string &address,
    std::vector<std::string> &uuids)
{
    return ToCore(this)->NearlinkHostProxyCore::GetDeviceUuids(transport, address, &uuids);
}

NlErrCode NearlinkHostProxy::PairRequestReply(int32_t transport, const std::string &address, bool accept)
{
    return ToCore(this)->NearlinkHostProxyCore::PairRequestReply(transport, address, accept);
}

NlErrCode NearlinkHostProxy::ReadRemoteRssiValue(const std::string &address)
{
    return ToCore(this)->NearlinkHostProxyCore::ReadRemoteRssiValue(address);
}

NlErrCode NearlinkHostProxy::GetDeviceAppearance(const std::string &address, int &appearance)
{
    return ToCore(this)->NearlinkHostProxyCore::GetDeviceAppearance(address, appearance);
}

NlErrCode NearlinkHostProxy::ConnectAllowedProfiles(const std::string &remoteAddr)
{
    return ToCore(this)->NearlinkHostProxyCore::ConnectAllowedProfiles(remoteAddr);
}

NlErrCode NearlinkHostProxy::DisconnectAllowedProfiles(const std::string &remoteAddr)
{
    return ToCore(this)->NearlinkHostProxyCore::DisconnectAllowedProfiles(remoteAddr);
}

NlErrCode NearlinkHostProxy::GetDeviceProductId(const std::string &address, uint16_t &productId)
{
    return ToCore(this)->NearlinkHostProxyCore::GetDeviceProductId(address, productId);
}

NlErrCode NearlinkHostProxy::GetDeviceVendorId(const std::string &address, uint16_t &vendorId)
{
    return ToCore(this)->NearlinkHostProxyCore::GetDeviceVendorId(address, vendorId);
}

NlErrCode NearlinkHostProxy::GetDeviceModel(const std::string &address, NearlinkDeviceModel &model)
{
    return ToCore(this)->NearlinkHostProxyCore::GetDeviceModel(address, model);
}

NlErrCode NearlinkHostProxy::GetDeviceInformation(const std::string &address, NearlinkDeviceInformation &information)
{
    return ToCore(this)->NearlinkHostProxyCore::GetDeviceInformation(address, information);
}

NlErrCode NearlinkHostProxy::GetAcbState(const std::string &address, int &acbState)
{
    return ToCore(this)->NearlinkHostProxyCore::GetAcbState(address, acbState);
}

NlErrCode NearlinkHostProxy::FactoryReset()
{
    return ToCore(this)->NearlinkHostProxyCore::FactoryReset();
}

NlErrCode NearlinkHostProxy::SetFreqHopping(const std::vector<uint8_t> &freq)
{
    return ToCore(this)->NearlinkHostProxyCore::SetFreqHopping(freq);
}

NlErrCode NearlinkHostProxy::UpdateRefusePolicy(const int32_t protocolType, const int32_t pid,
    const int64_t refuseTime)
{
    return ToCore(this)->NearlinkHostProxyCore::UpdateRefusePolicy(protocolType, pid, refuseTime);
}

NlErrCode NearlinkHostProxy::CheckPermissionForNapi(const std::string &permission, bool &isGranted)
{
    return ToCore(this)->NearlinkHostProxyCore::CheckPermissionForNapi(permission, isGranted);
}

NlErrCode NearlinkHostProxy::UpdateSleVirtualDevice(int32_t cmd, const std::string &address)
{
    return ToCore(this)->NearlinkHostProxyCore::UpdateSleVirtualDevice(cmd, address);
}

NlErrCode NearlinkHostProxy::GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr)
{
    return ToCore(this)->NearlinkHostProxyCore::GetSleAddrByBtAddr(btAddr, sleAddr);
}

NlErrCode NearlinkHostProxy::GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr)
{
    return ToCore(this)->NearlinkHostProxyCore::GetBtAddrBySleAddr(sleAddr, btAddr);
}

NlErrCode NearlinkHostProxy::SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr)
{
    return ToCore(this)->NearlinkHostProxyCore::SetBtAddrBySleAddr(sleAddr, btAddr);
}

NlErrCode NearlinkHostProxy::GetBatteryLevel(const std::string &address)
{
    return ToCore(this)->NearlinkHostProxyCore::GetBatteryLevel(address);
}

NlErrCode NearlinkHostProxy::IsFeatureSupported(int32_t feature, bool &isSupported)
{
    return ToCore(this)->NearlinkHostProxyCore::IsFeatureSupported(feature, isSupported);
}

NlErrCode NearlinkHostProxy::IsConnectionExist(bool &isConnectionExist)
{
    return ToCore(this)->NearlinkHostProxyCore::IsConnectionExist(isConnectionExist);
}

}  // namespace Nearlink
}  // namespace OHOS
