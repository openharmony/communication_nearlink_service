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

#include "SleAdapterWrapper.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceAdapterSub.h"
#include "SleRemoteDeviceAdapter.h"
#include "HidHostService.h"

namespace OHOS {
namespace Nearlink {
SleAdapterWrapper::SleAdapterWrapper()
{}

std::vector<RawAddress> SleAdapterWrapper::GetConnectedDevices() const
{
    return SleRemoteDeviceAdapter::GetInstance()->GetConnectedDevices();
}

uint16_t SleAdapterWrapper::GetLcidByAddress(const RawAddress &device)
{
    return SleRemoteDeviceAdapter::GetInstance()->GetLcidByAddress(device);
}

void SleAdapterWrapper::AddBgConnDevice(const std::string &address)
{
    return SleRemoteDeviceAdapter::GetInstance()->AddBgConnDevice(address);
}

std::string SleAdapterWrapper::GetAliasName(const std::string &address) const
{
    return SleRemoteDeviceAdapter::GetInstance()->GetAliasName(RawAddress(address));
}

uint8_t SleAdapterWrapper::GetPeerDeviceAddrType(const RawAddress &address) const
{
    return SleRemoteDeviceAdapter::GetInstance()->GetPeerDeviceAddrType(address);
}

void SleAdapterWrapper::GetDeviceTypeInfo(const RawAddress &device, SLE_Addr_S &addrInfo, int &devType) const
{
    SleRemoteDeviceAdapter::GetInstance()->GetDeviceTypeInfo(device, addrInfo, devType);
}

int SleAdapterWrapper::HidSendData(const HidReportInfo &reportInfo) const
{
    HidHostService *hidHostService = HidHostService::GetService();
    NL_CHECK_RETURN_RET(hidHostService, HID_HOST_FAILURE, "hidHostService is nullptr.");
    return hidHostService->SendData(reportInfo);
}

bool SleAdapterWrapper::IsAudioDevice(const std::string &address)
{
    return SleRemoteDeviceAdapter::GetInstance()->IsAudioDevice(address);
}

bool SleAdapterWrapper::GetCdsmOtherAddr(const RawAddress &member, RawAddress &other)
{
    return SleRemoteDeviceAdapter::GetInstance()->GetCdsmOtherAddr(member, other);
}

void SleAdapterWrapper::SendBgConnList(NearlinkSafeList<RawAddress> &bgList)
{
    SleRemoteDeviceAdapter::GetInstance()->SendBgConnList(bgList);
}

void SleAdapterWrapper::SendDirectConnList(NearlinkSafeList<RawAddress> &directList)
{
    SleRemoteDeviceAdapter::GetInstance()->SendDirectConnList(directList);
}

int SleAdapterWrapper::GetDeviceAppearance(const RawAddress &device) const
{
    SleInterfaceAdapterSub *sleService = static_cast<SleInterfaceAdapterSub *>(
        SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN_RET(sleService, INVALID_APPEARANCE, "sleService invalid.");

    return sleService->GetDeviceAppearance(device);
}

bool SleAdapterWrapper::DisconnectAllProfile(const RawAddress &device)
{
    SleInterfaceAdapterSub *sleService = static_cast<SleInterfaceAdapterSub *>(
        SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN_RET(sleService, false, "sleService invalid.");

    return sleService->DisconnectAllProfile(device);
}

bool SleAdapterWrapper::WrapperCdsmGetAllMemberInfo(const RawAddress &rawAddr, std::vector<RawAddress> &cdsmInfoAddr)
{
    SleInterfaceAdapterSub *sleService = static_cast<SleInterfaceAdapterSub *>
        (SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN_RET(sleService, false, "sleService invalid.");
    bool ret = sleService->WrapperCdsmGetAllMemberInfo(rawAddr, cdsmInfoAddr);
    return ret;
}

bool SleAdapterWrapper::IsProxyConnectExisted(std::string &devAddress)
{
    return SleInterfaceDataTransfer::GetInstance().IsProxyConnectExisted(devAddress);
}

} // namespace Nearlink
} // namespace OHOS

