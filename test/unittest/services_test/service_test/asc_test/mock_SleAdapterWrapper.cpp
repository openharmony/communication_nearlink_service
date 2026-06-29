/*
 * Copyright (C) 2026 Huawei Device Co., Ltd. All rights reserved.
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
#include "log.h"
#include "HidHostDefines.h"

namespace OHOS {
namespace Nearlink {

SleAdapterWrapper::SleAdapterWrapper()
{
    HILOGI("[SleAdapterWrapper Mocker] Constructor");
}

std::vector<RawAddress> SleAdapterWrapper::GetConnectedDevices() const
{
    HILOGI("[SleAdapterWrapper Mocker] GetConnectedDevices");
    return {};
}

uint16_t SleAdapterWrapper::GetLcidByAddress(const RawAddress &device)
{
    HILOGI("[SleAdapterWrapper Mocker] GetLcidByAddress, address=%{public}s", device.GetAddress().c_str());
    return 0;
}

void SleAdapterWrapper::AddBgConnDevice(const std::string &address)
{
    HILOGI("[SleAdapterWrapper Mocker] AddBgConnDevice, address=%{public}s", address.c_str());
}

std::string SleAdapterWrapper::GetAliasName(const std::string &address) const
{
    HILOGI("[SleAdapterWrapper Mocker] GetAliasName, address=%{public}s", address.c_str());
    return "";
}

bool SleAdapterWrapper::RegisterStateObserver(IAdapterStateObserver &observer) const
{
    HILOGI("[SleAdapterWrapper Mocker] RegisterStateObserver");
    return true;
}

bool SleAdapterWrapper::DeregisterStateObserver(IAdapterStateObserver &observer) const
{
    HILOGI("[SleAdapterWrapper Mocker] DeregisterStateObserver");
    return true;
}

uint8_t SleAdapterWrapper::GetPeerDeviceAddrType(const RawAddress &address) const
{
    HILOGI("[SleAdapterWrapper Mocker] GetPeerDeviceAddrType, address=%{public}s", address.GetAddress().c_str());
    return 0;
}

void SleAdapterWrapper::RegisterSlePeripheralCallback(ISlePeripheralCallback &callback) const
{
    HILOGI("[SleAdapterWrapper Mocker] RegisterSlePeripheralCallback");
}

void SleAdapterWrapper::DeregisterSlePeripheralCallback(ISlePeripheralCallback &callback) const
{
    HILOGI("[SleAdapterWrapper Mocker] DeregisterSlePeripheralCallback");
}

int SleAdapterWrapper::HidSendData(const HidReportInfo &reportInfo) const
{
    HILOGI("[SleAdapterWrapper Mocker] HidSendData, reportId=%{public}u, dataLength=%{public}u",
        reportInfo.reportId_, reportInfo.dataLength_);
    return HID_HOST_SUCCESS;
}

} // namespace Nearlink
} // namespace OHOS
