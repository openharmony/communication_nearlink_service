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

#include "log.h"
#include "nearlink_cloud_pair_device.h"

namespace OHOS {
namespace Nearlink {

NearlinkCloudPairDevice::NearlinkCloudPairDevice(const CloudPairDeviceInfo& devParam)
{
    btAddr_ = devParam.GetBtAddr();
    deviceName_ = devParam.GetDeviceName();
    token_ = devParam.GetToken();
    reportAddr_ = devParam.GetReportAddr();
    membersAddr_ = devParam.GetMembersAddr();
    model_ = devParam.GetModel();
    subModelId_ = devParam.GetSubModelId();
    deviceIconId_ = devParam.GetDeviceIconId();
}

bool NearlinkCloudPairDevice::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(btAddr_)) {
        return false;
    }
    if (!parcel.WriteString(deviceName_)) {
        return false;
    }
    if (!MarshallingVecSafe(parcel, token_)) {
        return false;
    }
    if (!parcel.WriteString(reportAddr_)) {
        return false;
    }
    if (!MarshallingVecStrSafe(parcel, membersAddr_)) {
        return false;
    }
    if (!parcel.WriteString(model_)) {
        return false;
    }
    if (!parcel.WriteString(subModelId_)) {
        return false;
    }
    if (!parcel.WriteString(deviceIconId_)) {
        return false;
    }
    return true;
}

NearlinkCloudPairDevice *NearlinkCloudPairDevice::Unmarshalling(Parcel &parcel)
{
    NearlinkCloudPairDevice *cloudDevice = new (std::nothrow) NearlinkCloudPairDevice();
    if (cloudDevice != nullptr && !cloudDevice->ReadFromParcel(parcel)){
        delete cloudDevice;
        cloudDevice = nullptr;
    }
    return cloudDevice;
}


bool NearlinkCloudPairDevice::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkCloudPairDevice::ReadFromParcel(Parcel &parcel)
{
    if (!parcel.ReadString(btAddr_)) {
        return false;
    }
    if (!parcel.ReadString(deviceName_)) {
        return false;
    }
    if (!ReadParcelVecSafe(parcel, token_)) {
        return false;
    }
    if (!parcel.ReadString(reportAddr_)) {
        return false;
    }
    if (!ReadParcelVecStrSafe(parcel, membersAddr_)) {
        return false;
    }
    if (!parcel.ReadString(model_)) {
        return false;
    }
    if (!parcel.ReadString(subModelId_)) {
        return false;
    }
    if (!parcel.ReadString(deviceIconId_)) {
        return false;
    }
    return true;
}

bool NearlinkCloudPairDevice::MarshallingVecSafe(Parcel &parcel, const std::vector<uint8_t> &vec) const
{
    if (vec.size() > CLOUD_PAIR_DEVICE_SIZE_MAX) {
        return false;
    }
    if (!parcel.WriteUint32(vec.size())) {
        return false;
    }
    for (auto &data : vec) {
        if (!parcel.WriteUint8(data)) {
            return false;
        }
    }
    return true;
}

bool NearlinkCloudPairDevice::ReadParcelVecSafe(Parcel &parcel, std::vector<uint8_t> &res)
{
    uint32_t vecSize = 0;
    if (!parcel.ReadUint32(vecSize) || vecSize > CLOUD_PAIR_DEVICE_SIZE_MAX) {
        return false;
    }
    uint8_t data = 0;
    for (uint32_t i = 0; i < vecSize; i++) {
        if (!parcel.ReadUint8(data)) {
            return false;
        }
        res.push_back(data);
    }
    return true;
}

bool NearlinkCloudPairDevice::MarshallingVecStrSafe(Parcel &parcel, const std::vector<std::string> &vec) const
{
    if (vec.size() > CLOUD_PAIR_DEVICE_SIZE_MAX) {
        return false;
    }
    if (!parcel.WriteUint32(vec.size())) {
        return false;
    }
    for (auto &data : vec) {
        if (!parcel.WriteString(data)) {
            return false;
        }
    }
    return true;
}

bool NearlinkCloudPairDevice::ReadParcelVecStrSafe(Parcel &parcel, std::vector<std::string> &res)
{
    uint32_t vecSize = 0;
    if (!parcel.ReadUint32(vecSize) || vecSize > CLOUD_PAIR_DEVICE_SIZE_MAX) {
        return false;
    }
    std::string data;
    for (uint32_t i = 0; i < vecSize; i++) {
        if (!parcel.ReadString(data)) {
            return false;
        }
        res.push_back(data);
    }
    return true;
}

} // namespace Nearlink
} // namespace OHOS