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

NearlinkRemoteDevice::NearlinkRemoteDevice() : base_()
{
}

NearlinkRemoteDevice::NearlinkRemoteDevice(const std::string &addr, const int transport)
    : base_(addr, transport)
{
}

NearlinkRemoteDevice::NearlinkRemoteDevice(const NearlinkBaseRemoteDevice &base)
    : base_(base)
{
}

NearlinkRemoteDevice::~NearlinkRemoteDevice() = default;

NlErrCode NearlinkRemoteDevice::GetDeviceName(std::string &name) const
{
    return base_.GetDeviceName(name);
}

NlErrCode NearlinkRemoteDevice::GetDeviceAlias(std::string &alias) const
{
    return base_.GetDeviceAlias(alias);
}

NlErrCode NearlinkRemoteDevice::SetDeviceAlias(const std::string &aliasName)
{
    return base_.SetDeviceAlias(aliasName);
}

NlErrCode NearlinkRemoteDevice::GetPairState(int &pairState) const
{
    return base_.GetPairState(pairState);
}

NlErrCode NearlinkRemoteDevice::StartPair()
{
    return base_.StartPair();
}

NlErrCode NearlinkRemoteDevice::StartCrediblePair()
{
    return base_.StartCrediblePair();
}

NlErrCode NearlinkRemoteDevice::CancelDevicePairing()
{
    return base_.CancelDevicePairing();
}

NlErrCode NearlinkRemoteDevice::SetPairingConfirmation(bool cfm)
{
    return base_.SetPairingConfirmation(cfm);
}

NlErrCode NearlinkRemoteDevice::SetPairingPassCode(const std::string &passCode)
{
    return base_.SetPairingPassCode(passCode);
}

NlErrCode NearlinkRemoteDevice::IsBondedFromLocal(bool &isBondedFromLocal) const
{
    return base_.IsBondedFromLocal(isBondedFromLocal);
}

NlErrCode NearlinkRemoteDevice::IsAcbConnected(bool &isAcbConnected) const
{
    return base_.IsAcbConnected(isAcbConnected);
}

NlErrCode NearlinkRemoteDevice::IsAcbEncrypted(bool &isAcbEncrypted) const
{
    return base_.IsAcbEncrypted(isAcbEncrypted);
}

NlErrCode NearlinkRemoteDevice::GetDeviceProductId(uint16_t &productId) const
{
    return base_.GetDeviceProductId(productId);
}

NlErrCode NearlinkRemoteDevice::GetDeviceVendorId(uint16_t &vendorId) const
{
    return base_.GetDeviceVendorId(vendorId);
}

NlErrCode NearlinkRemoteDevice::GetDeviceModel(DeviceModel &model) const
{
    return base_.GetDeviceModel(model);
}

NlErrCode NearlinkRemoteDevice::GetDeviceInformation(DeviceInformation &information) const
{
    return base_.GetDeviceInformation(information);
}

NlErrCode NearlinkRemoteDevice::GetDeviceUuids(std::vector<std::string> &uuids) const
{
    return base_.GetDeviceUuids(uuids);
}

NlErrCode NearlinkRemoteDevice::PairRequestReply(bool accept)
{
    return base_.PairRequestReply(accept);
}

int NearlinkRemoteDevice::GetTransportType() const
{
    return base_.GetTransportType();
}

NlErrCode NearlinkRemoteDevice::UpdateConnectInterval(ConnectionInterval interval) const
{
    return base_.UpdateConnectInterval(interval);
}

NlErrCode NearlinkRemoteDevice::ReadRemoteRssiValue()
{
    return base_.ReadRemoteRssiValue();
}

bool NearlinkRemoteDevice::IsValidNearlinkRemoteDevice() const
{
    return base_.IsValidNearlinkRemoteDevice();
}

NlErrCode NearlinkRemoteDevice::GetDeviceAppearance(int &appearance) const
{
    return base_.GetDeviceAppearance(appearance);
}

NlErrCode NearlinkRemoteDevice::GetAcbState(int &acbState) const
{
    return base_.GetAcbState(acbState);
}

NlErrCode NearlinkRemoteDevice::GetBatteryLevel()
{
    return base_.GetBatteryLevel();
}

}  // namespace Nearlink
}  // namespace OHOS
