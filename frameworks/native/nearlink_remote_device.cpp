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

NearlinkRemoteDevice::NearlinkRemoteDevice(const std::string &addr, const int transport)
    : NearlinkBaseRemoteDevice(addr, transport)
{
}

NlErrCode NearlinkRemoteDevice::GetDeviceName(std::string &name) const
{
    return NearlinkBaseRemoteDevice::GetDeviceName(name);
}

NlErrCode NearlinkRemoteDevice::GetDeviceAlias(std::string &alias) const
{
    return NearlinkBaseRemoteDevice::GetDeviceAlias(alias);
}

NlErrCode NearlinkRemoteDevice::SetDeviceAlias(const std::string &aliasName)
{
    return NearlinkBaseRemoteDevice::SetDeviceAlias(aliasName);
}

NlErrCode NearlinkRemoteDevice::GetPairState(int &pairState) const
{
    return NearlinkBaseRemoteDevice::GetPairState(pairState);
}

NlErrCode NearlinkRemoteDevice::StartPair()
{
    return NearlinkBaseRemoteDevice::StartPair();
}

NlErrCode NearlinkRemoteDevice::StartCrediblePair()
{
    return NearlinkBaseRemoteDevice::StartCrediblePair();
}

NlErrCode NearlinkRemoteDevice::CancelDevicePairing()
{
    return NearlinkBaseRemoteDevice::CancelDevicePairing();
}

NlErrCode NearlinkRemoteDevice::SetPairingConfirmation(bool cfm)
{
    return NearlinkBaseRemoteDevice::SetPairingConfirmation(cfm);
}

NlErrCode NearlinkRemoteDevice::SetPairingPassCode(const std::string &passCode)
{
    return NearlinkBaseRemoteDevice::SetPairingPassCode(passCode);
}

NlErrCode NearlinkRemoteDevice::IsBondedFromLocal(bool &isBondedFromLocal) const
{
    return NearlinkBaseRemoteDevice::IsBondedFromLocal(isBondedFromLocal);
}

NlErrCode NearlinkRemoteDevice::IsAcbConnected(bool &isAcbConnected) const
{
    return NearlinkBaseRemoteDevice::IsAcbConnected(isAcbConnected);
}

NlErrCode NearlinkRemoteDevice::IsAcbEncrypted(bool &isAcbEncrypted) const
{
    return NearlinkBaseRemoteDevice::IsAcbEncrypted(isAcbEncrypted);
}

NlErrCode NearlinkRemoteDevice::GetDeviceProductId(uint16_t &productId) const
{
    return NearlinkBaseRemoteDevice::GetDeviceProductId(productId);
}

NlErrCode NearlinkRemoteDevice::GetDeviceVendorId(uint16_t &vendorId) const
{
    return NearlinkBaseRemoteDevice::GetDeviceVendorId(vendorId);
}

NlErrCode NearlinkRemoteDevice::GetDeviceModel(DeviceModel &model) const
{
    return NearlinkBaseRemoteDevice::GetDeviceModel(model);
}

NlErrCode NearlinkRemoteDevice::GetDeviceInformation(DeviceInformation &information) const
{
    return NearlinkBaseRemoteDevice::GetDeviceInformation(information);
}

NlErrCode NearlinkRemoteDevice::GetDeviceUuids(std::vector<std::string> &uuids) const
{
    return NearlinkBaseRemoteDevice::GetDeviceUuids(uuids);
}

NlErrCode NearlinkRemoteDevice::PairRequestReply(bool accept)
{
    return NearlinkBaseRemoteDevice::PairRequestReply(accept);
}

int NearlinkRemoteDevice::GetTransportType() const
{
    return NearlinkBaseRemoteDevice::GetTransportType();
}

NlErrCode NearlinkRemoteDevice::UpdateConnectInterval(ConnectionInterval interval) const
{
    return NearlinkBaseRemoteDevice::UpdateConnectInterval(interval);
}

NlErrCode NearlinkRemoteDevice::ReadRemoteRssiValue()
{
    return NearlinkBaseRemoteDevice::ReadRemoteRssiValue();
}

bool NearlinkRemoteDevice::IsValidNearlinkRemoteDevice() const
{
    return NearlinkBaseRemoteDevice::IsValidNearlinkRemoteDevice();
}

NlErrCode NearlinkRemoteDevice::GetDeviceAppearance(int &appearance) const
{
    return NearlinkBaseRemoteDevice::GetDeviceAppearance(appearance);
}

NlErrCode NearlinkRemoteDevice::GetAcbState(int &acbState) const
{
    return NearlinkBaseRemoteDevice::GetAcbState(acbState);
}

NlErrCode NearlinkRemoteDevice::GetBatteryLevel()
{
    return NearlinkBaseRemoteDevice::GetBatteryLevel();
}

}  // namespace Nearlink
}  // namespace OHOS
