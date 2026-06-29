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

#include "nearlink_device_information_parcel.h"

namespace OHOS {
namespace Nearlink {
NearlinkDeviceInformation& NearlinkDeviceInformation::operator = (const NearlinkDeviceInformation &other)
{
    if (this != &other) {
        manufactureData_ = other.manufactureData_;
        modelData_ = other.modelData_;
    }
    return *this;
}

NearlinkDeviceInformation& NearlinkDeviceInformation::operator = (const DeviceInformation &other)
{
    if (this != &other) {
        manufactureData_ = other.GetManufacturerData();
        modelData_ = other.GetModelData();
    }
    return *this;
}

bool NearlinkDeviceInformation::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(manufactureData_)) {
        return false;
    }
    if (!parcel.WriteString(modelData_)) {
        return false;
    }
    return true;
}

NearlinkDeviceInformation *NearlinkDeviceInformation::Unmarshalling(Parcel &parcel)
{
    NearlinkDeviceInformation *information = new (std::nothrow) NearlinkDeviceInformation();
    if (information != nullptr && !information->ReadFromParcel(parcel)) {
        delete information;
        information = nullptr;
    }
    return information;
}

bool NearlinkDeviceInformation::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkDeviceInformation::ReadFromParcel(Parcel &parcel)
{
    if (!parcel.ReadString(manufactureData_)) {
        return false;
    }
    if (!parcel.ReadString(modelData_)) {
        return false;
    }
    return true;
}
}  // namespace Nearlink
}  // namespace OHOS
