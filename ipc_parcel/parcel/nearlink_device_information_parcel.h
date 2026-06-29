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

#ifndef NEARLINK_PARCEL_DEVICE_INFORMATION_H
#define NEARLINK_PARCEL_DEVICE_INFORMATION_H

#include "parcel.h"
#include "nearlink_device_information.h"

namespace OHOS {
namespace Nearlink {
class NearlinkDeviceInformation : public Parcelable, public DeviceInformation {
public:
    NearlinkDeviceInformation() = default;
    ~NearlinkDeviceInformation() = default;
    explicit NearlinkDeviceInformation(const NearlinkDeviceInformation &other) : DeviceInformation(other) {}
    explicit NearlinkDeviceInformation(const DeviceInformation &other) : DeviceInformation(other) {}
    explicit NearlinkDeviceInformation(const std::string &manufactureData, const std::string &modelData)
        : DeviceInformation(manufactureData, modelData) {}
    NearlinkDeviceInformation& operator = (const NearlinkDeviceInformation &other);
    NearlinkDeviceInformation& operator = (const DeviceInformation &other);

    bool Marshalling(Parcel &parcel) const override;

    static NearlinkDeviceInformation *Unmarshalling(Parcel &parcel);

    bool WriteToParcel(Parcel &parcel);

    bool ReadFromParcel(Parcel &parcel);
};
}  // namespace Nearlink
}  // namespace OHOS

#endif // NEARLINK_PARCEL_DEVICE_INFORMATION_H
