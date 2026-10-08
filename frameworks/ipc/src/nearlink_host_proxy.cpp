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
#include "nearlink_base_host_proxy.h"

namespace OHOS {
namespace Nearlink {

namespace {
inline NearlinkBaseHostProxy *ToBase(NearlinkHostProxy *self)
{
    return reinterpret_cast<NearlinkBaseHostProxy *>(self);
}
}
BrokerDelegator<NearlinkHostProxy> NearlinkHostProxy::delegator_;

NlErrCode NearlinkHostProxy::GetProfile(const std::string &name, sptr<IRemoteObject> &remoteProfile)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetProfile(name, remoteProfile);
}

NlErrCode NearlinkHostProxy::DisableSle()
{
    return ToBase(this)->NearlinkBaseHostProxy::DisableSle();
}

NlErrCode NearlinkHostProxy::EnableSle(const SleAutoConnectPolicy autoConnPolicy)
{
    return ToBase(this)->NearlinkBaseHostProxy::EnableSle(autoConnPolicy);
}

NlErrCode NearlinkHostProxy::DisableSleToOff()
{
    return ToBase(this)->NearlinkBaseHostProxy::DisableSleToOff();
}

NlErrCode NearlinkHostProxy::EnableSleToHalf()
{
    return ToBase(this)->NearlinkBaseHostProxy::EnableSleToHalf();
}

NlErrCode NearlinkHostProxy::GetSleFullState(int &sleCurrentState)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetSleFullState(sleCurrentState);
}

NlErrCode NearlinkHostProxy::IsSleEnabled(bool &isSleEnabled)
{
    return ToBase(this)->NearlinkBaseHostProxy::IsSleEnabled(isSleEnabled);
}

NlErrCode NearlinkHostProxy::IsSleHalfDisabled(bool &isSleHalfDisabled)
{
    return ToBase(this)->NearlinkBaseHostProxy::IsSleHalfDisabled(isSleHalfDisabled);
}

NlErrCode NearlinkHostProxy::IsSleDisabled(bool &isSleDisabled)
{
    return ToBase(this)->NearlinkBaseHostProxy::IsSleDisabled(isSleDisabled);
}

NlErrCode NearlinkHostProxy::IsSleAvailableToCaller(bool &isSleAvailable)
{
    return ToBase(this)->NearlinkBaseHostProxy::IsSleAvailableToCaller(isSleAvailable);
}

NlErrCode NearlinkHostProxy::GetAdapterConnectState(int32_t &state)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetAdapterConnectState(state);
}

NlErrCode NearlinkHostProxy::GetProfileConnState(const std::string &remoteAddr, int32_t &state)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetProfileConnState(remoteAddr, state);
}

NlErrCode NearlinkHostProxy::GetLocalName(std::string &name)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetLocalName(name);
}

NlErrCode NearlinkHostProxy::SetLocalName(const std::string &name)
{
    return ToBase(this)->NearlinkBaseHostProxy::SetLocalName(name);
}

NlErrCode NearlinkHostProxy::GetLocalAddress(std::string &addr)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetLocalAddress(addr);
}

NlErrCode NearlinkHostProxy::GetPairedDevices(std::vector<NearlinkRawAddress> &pairedAddr)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetPairedDevices(pairedAddr);
}

NlErrCode NearlinkHostProxy::SetConnectionMode(int32_t connectionMode, int32_t duration)
{
    return ToBase(this)->NearlinkBaseHostProxy::SetConnectionMode(connectionMode, duration);
}

NlErrCode NearlinkHostProxy::RemovePair(const sptr<NearlinkRawAddress> &device)
{
    return ToBase(this)->NearlinkBaseHostProxy::RemovePair(device);
}

NlErrCode NearlinkHostProxy::RemoveAllPairs()
{
    return ToBase(this)->NearlinkBaseHostProxy::RemoveAllPairs();
}

NlErrCode NearlinkHostProxy::RegisterSlePeripheralCallback(const sptr<INearlinkSlePeripheralObserver> &observer)
{
    return ToBase(this)->NearlinkBaseHostProxy::RegisterSlePeripheralCallback(observer);
}

NlErrCode NearlinkHostProxy::DeregisterSlePeripheralCallback(const sptr<INearlinkSlePeripheralObserver> &observer)
{
    return ToBase(this)->NearlinkBaseHostProxy::DeregisterSlePeripheralCallback(observer);
}

NlErrCode NearlinkHostProxy::RegisterSleAdapterObserver(const sptr<INearlinkHostObserver> &observer)
{
    return ToBase(this)->NearlinkBaseHostProxy::RegisterSleAdapterObserver(observer);
}

NlErrCode NearlinkHostProxy::DeregisterSleAdapterObserver(const sptr<INearlinkHostObserver> &observer)
{
    return ToBase(this)->NearlinkBaseHostProxy::DeregisterSleAdapterObserver(observer);
}

NlErrCode NearlinkHostProxy::RegisterDeviceBatteryObserver(const sptr<INearlinkDeviceBatteryObserver> &observer)
{
    return ToBase(this)->NearlinkBaseHostProxy::RegisterDeviceBatteryObserver(observer);
}

NlErrCode NearlinkHostProxy::DeregisterDeviceBatteryObserver(const sptr<INearlinkDeviceBatteryObserver> &observer)
{
    return ToBase(this)->NearlinkBaseHostProxy::DeregisterDeviceBatteryObserver(observer);
}

NlErrCode NearlinkHostProxy::RegisterDeviceRssiObserver(const sptr<INearlinkDeviceRssiObserver> &observer)
{
    return ToBase(this)->NearlinkBaseHostProxy::RegisterDeviceRssiObserver(observer);
}

NlErrCode NearlinkHostProxy::DeregisterDeviceRssiObserver(const sptr<INearlinkDeviceRssiObserver> &observer)
{
    return ToBase(this)->NearlinkBaseHostProxy::DeregisterDeviceRssiObserver(observer);
}

NlErrCode NearlinkHostProxy::GetSleMaxAdvertisingDataLength(uint32_t &maxAdvDataLen)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetSleMaxAdvertisingDataLength(maxAdvDataLen);
}

NlErrCode NearlinkHostProxy::GetDeviceName(int32_t transport, const std::string &address, std::string &name)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetDeviceName(transport, address, name);
}

NlErrCode NearlinkHostProxy::GetDeviceAlias(int32_t transport, const std::string &address, std::string &alias)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetDeviceAlias(transport, address, alias);
}

NlErrCode NearlinkHostProxy::SetDeviceAlias(int32_t transport, const std::string &address, const std::string &alias)
{
    return ToBase(this)->NearlinkBaseHostProxy::SetDeviceAlias(transport, address, alias);
}

NlErrCode NearlinkHostProxy::GetPairState(int32_t transport, const std::string &address, int &pairState)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetPairState(transport, address, pairState);
}

NlErrCode NearlinkHostProxy::StartPair(int32_t transport, const std::string &address)
{
    return ToBase(this)->NearlinkBaseHostProxy::StartPair(transport, address);
}

NlErrCode NearlinkHostProxy::StartCrediblePair(int32_t transport, const std::string &address)
{
    return ToBase(this)->NearlinkBaseHostProxy::StartCrediblePair(transport, address);
}

NlErrCode NearlinkHostProxy::CancelPairing(int32_t transport, const std::string &address)
{
    return ToBase(this)->NearlinkBaseHostProxy::CancelPairing(transport, address);
}

NlErrCode NearlinkHostProxy::SetPairingConfirmation(int32_t transport, const std::string &address, bool cfm)
{
    return ToBase(this)->NearlinkBaseHostProxy::SetPairingConfirmation(transport, address, cfm);
}

NlErrCode NearlinkHostProxy::SetPairingPassCode(int32_t transport, const std::string &address,
    const std::string &passCode)
{
    return ToBase(this)->NearlinkBaseHostProxy::SetPairingPassCode(transport, address, passCode);
}

NlErrCode NearlinkHostProxy::IsBondedFromLocal(int32_t transport, const std::string &address,
    bool &isBondedFromLocal)
{
    return ToBase(this)->NearlinkBaseHostProxy::IsBondedFromLocal(transport, address, isBondedFromLocal);
}

NlErrCode NearlinkHostProxy::IsAcbConnected(int32_t transport, const std::string &address, bool &isAcbConnected)
{
    return ToBase(this)->NearlinkBaseHostProxy::IsAcbConnected(transport, address, isAcbConnected);
}

NlErrCode NearlinkHostProxy::IsAcbEncrypted(int32_t transport, const std::string &address, bool &isAcbEncrypted)
{
    return ToBase(this)->NearlinkBaseHostProxy::IsAcbEncrypted(transport, address, isAcbEncrypted);
}

NlErrCode NearlinkHostProxy::GetLinkRole(int32_t transport, const std::string &address, uint8_t &role)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetLinkRole(transport, address, role);
}

NlErrCode NearlinkHostProxy::GetDeviceUuids(int32_t transport, const std::string &address,
    std::vector<std::string> &uuids)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetDeviceUuids(transport, address, uuids);
}

NlErrCode NearlinkHostProxy::PairRequestReply(int32_t transport, const std::string &address, bool accept)
{
    return ToBase(this)->NearlinkBaseHostProxy::PairRequestReply(transport, address, accept);
}

NlErrCode NearlinkHostProxy::ReadRemoteRssiValue(const std::string &address)
{
    return ToBase(this)->NearlinkBaseHostProxy::ReadRemoteRssiValue(address);
}

NlErrCode NearlinkHostProxy::GetDeviceAppearance(const std::string &address, int &appearance)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetDeviceAppearance(address, appearance);
}

NlErrCode NearlinkHostProxy::ConnectAllowedProfiles(const std::string &remoteAddr)
{
    return ToBase(this)->NearlinkBaseHostProxy::ConnectAllowedProfiles(remoteAddr);
}

NlErrCode NearlinkHostProxy::DisconnectAllowedProfiles(const std::string &remoteAddr)
{
    return ToBase(this)->NearlinkBaseHostProxy::DisconnectAllowedProfiles(remoteAddr);
}

NlErrCode NearlinkHostProxy::GetDeviceProductId(const std::string &address, uint16_t &productId)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetDeviceProductId(address, productId);
}

NlErrCode NearlinkHostProxy::GetDeviceVendorId(const std::string &address, uint16_t &vendorId)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetDeviceVendorId(address, vendorId);
}

NlErrCode NearlinkHostProxy::GetDeviceModel(const std::string &address, NearlinkDeviceModel &model)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetDeviceModel(address, model);
}

NlErrCode NearlinkHostProxy::GetDeviceInformation(const std::string &address, NearlinkDeviceInformation &information)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetDeviceInformation(address, information);
}

NlErrCode NearlinkHostProxy::GetAcbState(const std::string &address, int &acbState)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetAcbState(address, acbState);
}

NlErrCode NearlinkHostProxy::FactoryReset()
{
    return ToBase(this)->NearlinkBaseHostProxy::FactoryReset();
}

NlErrCode NearlinkHostProxy::SetFreqHopping(const std::vector<uint8_t> &freq)
{
    return ToBase(this)->NearlinkBaseHostProxy::SetFreqHopping(freq);
}

NlErrCode NearlinkHostProxy::UpdateRefusePolicy(const int32_t protocolType, const int32_t pid,
    const int64_t refuseTime)
{
    return ToBase(this)->NearlinkBaseHostProxy::UpdateRefusePolicy(protocolType, pid, refuseTime);
}

NlErrCode NearlinkHostProxy::CheckPermissionForNapi(const std::string &permission, bool &isGranted)
{
    return ToBase(this)->NearlinkBaseHostProxy::CheckPermissionForNapi(permission, isGranted);
}

NlErrCode NearlinkHostProxy::UpdateSleVirtualDevice(int32_t cmd, const std::string &address)
{
    return ToBase(this)->NearlinkBaseHostProxy::UpdateSleVirtualDevice(cmd, address);
}

NlErrCode NearlinkHostProxy::GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetSleAddrByBtAddr(btAddr, sleAddr);
}

NlErrCode NearlinkHostProxy::GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetBtAddrBySleAddr(sleAddr, btAddr);
}

NlErrCode NearlinkHostProxy::SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr)
{
    return ToBase(this)->NearlinkBaseHostProxy::SetBtAddrBySleAddr(sleAddr, btAddr);
}

NlErrCode NearlinkHostProxy::GetBatteryLevel(const std::string &address)
{
    return ToBase(this)->NearlinkBaseHostProxy::GetBatteryLevel(address);
}

NlErrCode NearlinkHostProxy::IsFeatureSupported(int32_t feature, bool &isSupported)
{
    return ToBase(this)->NearlinkBaseHostProxy::IsFeatureSupported(feature, isSupported);
}

NlErrCode NearlinkHostProxy::IsConnectionExist(bool &isConnectionExist)
{
    return ToBase(this)->NearlinkBaseHostProxy::IsConnectionExist(isConnectionExist);
}

}  // namespace Nearlink
}  // namespace OHOS
