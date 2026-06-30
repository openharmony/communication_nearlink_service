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

#include "nearlink_dft_device_data.h"

namespace OHOS {
namespace Nearlink {


std::string NearlinkDeviceData::GetName()
{
    return name_;
}

int32_t NearlinkDeviceData::GetAppearance()
{
    return appearance_;
}

RawAddress NearlinkDeviceData::GetReportAddr()
{
    return reportAddr_;
}

std::string NearlinkDeviceData::GetAddr()
{
    return address_;
}

void NearlinkDeviceData::SetName(const std::string &name)
{
    name_ = name;
}

void NearlinkDeviceData::SetAppearance(int32_t appearance)
{
    appearance_ = appearance;
}

void NearlinkDeviceData::SetReportAddr(RawAddress reportAddr)
{
    reportAddr_ = reportAddr;
}

void NearlinkDeviceData::SetAddr(const std::string &address)
{
    address_ = address;
}

void NearlinkDeviceData::SetManufacturer(const std::string &manufacturer)
{
    manufacturer_ = manufacturer;
}

DftDeviceManager &DftDeviceManager::GetInstance()
{
    static DftDeviceManager DftDeviceManager;
    return DftDeviceManager;
}

void DftDeviceManager::AddDevice(RawAddress address, const int32_t appearance, const std::string &name)
{
    // makesure the reportAddr is not overrided
    RawAddress reportAddr;
    GetReportAddr(address, reportAddr);
    NearlinkDeviceData device;
    device.SetName(name);
    device.SetAppearance(appearance);
    device.SetAddr(address.GetAddress());
    device.SetReportAddr(reportAddr);
    peerDeviceSafeList_.EnsureInsert(address.GetAddress(), device);
}

void DftDeviceManager::EnsureUpdateReportAddr(RawAddress address, RawAddress reportAddr)
{
    bool ret = peerDeviceSafeList_.GetValueAndOpt(address.GetAddress(), [&reportAddr](
        std::string addr, NearlinkDeviceData value) -> void {
            value.SetReportAddr(reportAddr);
    });
    if (!ret) {
        // if insert failed, store the device address
        NearlinkDeviceData device;
        device.SetAddr(address.GetAddress());
        device.SetReportAddr(reportAddr);
        peerDeviceSafeList_.EnsureInsert(address.GetAddress(), device);
    }
}

void DftDeviceManager::UpdateAppearance(RawAddress address, const int32_t appearance)
{
    peerDeviceSafeList_.GetValueAndOpt(address.GetAddress(), [&appearance](
        std::string addr, NearlinkDeviceData value) -> void {
            value.SetAppearance(appearance);
    });
}

void DftDeviceManager::UpdateName(RawAddress address, const std::string &name)
{
    peerDeviceSafeList_.GetValueAndOpt(address.GetAddress(), [&name](
        std::string addr, NearlinkDeviceData value) -> void {
            value.SetName(name);
    });
}

void DftDeviceManager::UpdateManufacturer(RawAddress address, const std::string &manufacturer)
{
    peerDeviceSafeList_.GetValueAndOpt(address.GetAddress(), [&manufacturer](
        std::string addr, NearlinkDeviceData value) -> void {
            value.SetManufacturer(manufacturer);
    });
}

void DftDeviceManager::DelDevice(RawAddress address)
{
    peerDeviceSafeList_.FindAndRmv([&address](
        std::string addr, NearlinkDeviceData value) -> bool {
            return address.GetAddress() == addr;
    });
}

void DftDeviceManager::DelAllDevice()
{
    peerDeviceSafeList_.Clear();
}

void DftDeviceManager::FillCommonDeviceInfo(RawAddress address, std::string &name, int32_t &appearance)
{
    peerDeviceSafeList_.GetValueAndOpt(address.GetAddress(), [&name, &appearance](
        std::string addr, NearlinkDeviceData value) -> void {
            name = value.GetName();
            appearance = value.GetAppearance();
        });
}

void DftDeviceManager::GetReportAddr(RawAddress address, RawAddress &reportAddr)
{
    peerDeviceSafeList_.GetValueAndOpt(address.GetAddress(), [&reportAddr](
        std::string addr, NearlinkDeviceData value) -> void {
            reportAddr = value.GetReportAddr();
        });
}

}  // namespace Nearlink
}  // namespace OHOS