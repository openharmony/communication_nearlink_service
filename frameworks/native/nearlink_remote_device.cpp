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

#include "nearlink_remote_device.h"
#include "nearlink_base_remote_device.h"

namespace OHOS {
namespace Nearlink {

namespace {
inline NearlinkBaseRemoteDevice *ToBase(NearlinkRemoteDevice *self)
{
    return reinterpret_cast<NearlinkBaseRemoteDevice *>(self);
}
inline const NearlinkBaseRemoteDevice *ToBase(const NearlinkRemoteDevice *self)
{
    return reinterpret_cast<const NearlinkBaseRemoteDevice *>(self);
}
}
NearlinkRemoteDevice::NearlinkRemoteDevice(const std::string &addr, const int transport)
{
    address_ = addr;
    transport_ = transport;
}

NlErrCode NearlinkRemoteDevice::GetDeviceName(std::string &name) const
{
    return ToBase(this)->GetDeviceName(name);
}

NlErrCode NearlinkRemoteDevice::GetDeviceAlias(std::string &alias) const
{
    return ToBase(this)->GetDeviceAlias(alias);
}

NlErrCode NearlinkRemoteDevice::SetDeviceAlias(const std::string &aliasName)
{
    return ToBase(this)->SetDeviceAlias(aliasName);
}

NlErrCode NearlinkRemoteDevice::GetPairState(int &pairState) const
{
    return ToBase(this)->GetPairState(pairState);
}

NlErrCode NearlinkRemoteDevice::StartPair()
{
    return ToBase(this)->StartPair();
}

NlErrCode NearlinkRemoteDevice::StartCrediblePair()
{
    return ToBase(this)->StartCrediblePair();
}

NlErrCode NearlinkRemoteDevice::CancelDevicePairing()
{
    return ToBase(this)->CancelDevicePairing();
}

NlErrCode NearlinkRemoteDevice::SetPairingConfirmation(bool cfm)
{
    return ToBase(this)->SetPairingConfirmation(cfm);
}

NlErrCode NearlinkRemoteDevice::SetPairingPassCode(const std::string &passCode)
{
    return ToBase(this)->SetPairingPassCode(passCode);
}

NlErrCode NearlinkRemoteDevice::IsBondedFromLocal(bool &isBondedFromLocal) const
{
    return ToBase(this)->IsBondedFromLocal(isBondedFromLocal);
}

NlErrCode NearlinkRemoteDevice::IsAcbConnected(bool &isAcbConnected) const
{
    return ToBase(this)->IsAcbConnected(isAcbConnected);
}

NlErrCode NearlinkRemoteDevice::IsAcbEncrypted(bool &isAcbEncrypted) const
{
    return ToBase(this)->IsAcbEncrypted(isAcbEncrypted);
}

NlErrCode NearlinkRemoteDevice::GetDeviceProductId(uint16_t &productId) const
{
    return ToBase(this)->GetDeviceProductId(productId);
}

NlErrCode NearlinkRemoteDevice::GetDeviceVendorId(uint16_t &vendorId) const
{
    return ToBase(this)->GetDeviceVendorId(vendorId);
}

NlErrCode NearlinkRemoteDevice::GetDeviceModel(DeviceModel &model) const
{
    return ToBase(this)->GetDeviceModel(model);
}

NlErrCode NearlinkRemoteDevice::GetDeviceInformation(DeviceInformation &information) const
{
    return ToBase(this)->GetDeviceInformation(information);
}

NlErrCode NearlinkRemoteDevice::GetDeviceUuids(std::vector<std::string> &uuids) const
{
    return ToBase(this)->GetDeviceUuids(uuids);
}

NlErrCode NearlinkRemoteDevice::PairRequestReply(bool accept)
{
    return ToBase(this)->PairRequestReply(accept);
}

int NearlinkRemoteDevice::GetTransportType() const
{
    return ToBase(this)->GetTransportType();
}

NlErrCode NearlinkRemoteDevice::UpdateConnectInterval(ConnectionInterval interval) const
{
    return ToBase(this)->UpdateConnectInterval(interval);
}

NlErrCode NearlinkRemoteDevice::ReadRemoteRssiValue()
{
    return ToBase(this)->ReadRemoteRssiValue();
}

bool NearlinkRemoteDevice::IsValidNearlinkRemoteDevice() const
{
    return ToBase(this)->IsValidNearlinkRemoteDevice();
}

NlErrCode NearlinkRemoteDevice::GetDeviceAppearance(int &appearance) const
{
    return ToBase(this)->GetDeviceAppearance(appearance);
}

NlErrCode NearlinkRemoteDevice::GetAcbState(int &acbState) const
{
    return ToBase(this)->GetAcbState(acbState);
}

NlErrCode NearlinkRemoteDevice::GetBatteryLevel()
{
    return ToBase(this)->GetBatteryLevel();
}

}  // namespace Nearlink
}  // namespace OHOS
