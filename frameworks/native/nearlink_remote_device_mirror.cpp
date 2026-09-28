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
#include "base/nearlink_remote_device_core.h"

namespace OHOS {
namespace Nearlink {

namespace {
inline NearlinkRemoteDeviceCore *ToCore(NearlinkRemoteDevice *self)
{
    return reinterpret_cast<NearlinkRemoteDeviceCore *>(self);
}
inline const NearlinkRemoteDeviceCore *ToCore(const NearlinkRemoteDevice *self)
{
    return reinterpret_cast<const NearlinkRemoteDeviceCore *>(self);
}
}
NearlinkRemoteDevice::NearlinkRemoteDevice(const std::string &addr, const int transport)
{
    address_ = addr;
    transport_ = transport;
}

NlErrCode NearlinkRemoteDevice::GetDeviceName(std::string &name) const
{
    return ToCore(this)->GetDeviceName(name);
}

NlErrCode NearlinkRemoteDevice::GetDeviceAlias(std::string &alias) const
{
    return ToCore(this)->GetDeviceAlias(alias);
}

NlErrCode NearlinkRemoteDevice::SetDeviceAlias(const std::string &aliasName)
{
    return ToCore(this)->SetDeviceAlias(aliasName);
}

NlErrCode NearlinkRemoteDevice::GetPairState(int &pairState) const
{
    return ToCore(this)->GetPairState(pairState);
}

NlErrCode NearlinkRemoteDevice::StartPair()
{
    return ToCore(this)->StartPair();
}

NlErrCode NearlinkRemoteDevice::StartCrediblePair()
{
    return ToCore(this)->StartCrediblePair();
}

NlErrCode NearlinkRemoteDevice::CancelDevicePairing()
{
    return ToCore(this)->CancelDevicePairing();
}

NlErrCode NearlinkRemoteDevice::SetPairingConfirmation(bool cfm)
{
    return ToCore(this)->SetPairingConfirmation(cfm);
}

NlErrCode NearlinkRemoteDevice::SetPairingPassCode(const std::string &passCode)
{
    return ToCore(this)->SetPairingPassCode(passCode);
}

NlErrCode NearlinkRemoteDevice::IsBondedFromLocal(bool &isBondedFromLocal) const
{
    return ToCore(this)->IsBondedFromLocal(isBondedFromLocal);
}

NlErrCode NearlinkRemoteDevice::IsAcbConnected(bool &isAcbConnected) const
{
    return ToCore(this)->IsAcbConnected(isAcbConnected);
}

NlErrCode NearlinkRemoteDevice::IsAcbEncrypted(bool &isAcbEncrypted) const
{
    return ToCore(this)->IsAcbEncrypted(isAcbEncrypted);
}

NlErrCode NearlinkRemoteDevice::GetDeviceProductId(uint16_t &productId) const
{
    return ToCore(this)->GetDeviceProductId(productId);
}

NlErrCode NearlinkRemoteDevice::GetDeviceVendorId(uint16_t &vendorId) const
{
    return ToCore(this)->GetDeviceVendorId(vendorId);
}

NlErrCode NearlinkRemoteDevice::GetDeviceModel(DeviceModel &model) const
{
    return ToCore(this)->GetDeviceModel(model);
}

NlErrCode NearlinkRemoteDevice::GetDeviceInformation(DeviceInformation &information) const
{
    return ToCore(this)->GetDeviceInformation(information);
}

NlErrCode NearlinkRemoteDevice::GetDeviceUuids(std::vector<std::string> &uuids) const
{
    return ToCore(this)->GetDeviceUuids(uuids);
}

NlErrCode NearlinkRemoteDevice::PairRequestReply(bool accept)
{
    return ToCore(this)->PairRequestReply(accept);
}

int NearlinkRemoteDevice::GetTransportType() const
{
    return ToCore(this)->GetTransportType();
}

NlErrCode NearlinkRemoteDevice::UpdateConnectInterval(ConnectionInterval interval) const
{
    return ToCore(this)->UpdateConnectInterval(interval);
}

NlErrCode NearlinkRemoteDevice::ReadRemoteRssiValue()
{
    return ToCore(this)->ReadRemoteRssiValue();
}

bool NearlinkRemoteDevice::IsValidNearlinkRemoteDevice() const
{
    return ToCore(this)->IsValidNearlinkRemoteDevice();
}

NlErrCode NearlinkRemoteDevice::GetDeviceAppearance(int &appearance) const
{
    return ToCore(this)->GetDeviceAppearance(appearance);
}

NlErrCode NearlinkRemoteDevice::GetAcbState(int &acbState) const
{
    return ToCore(this)->GetAcbState(acbState);
}

NlErrCode NearlinkRemoteDevice::GetBatteryLevel()
{
    return ToCore(this)->GetBatteryLevel();
}

}  // namespace Nearlink
}  // namespace OHOS
