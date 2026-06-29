/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef NEARLINK_PARCEL_DEVICE_MODEL_H
#define NEARLINK_PARCEL_DEVICE_MODEL_H

#include "parcel.h"
#include "nearlink_device_model.h"

namespace OHOS {
namespace Nearlink {
class NearlinkDeviceModel : public Parcelable, public DeviceModel {
public:
    NearlinkDeviceModel() = default;
    ~NearlinkDeviceModel() = default;
    explicit NearlinkDeviceModel(const NearlinkDeviceModel &other) : DeviceModel(other) {}
    explicit NearlinkDeviceModel(const DeviceModel &other) : DeviceModel(other) {}
    explicit NearlinkDeviceModel(const std::string &modelId, const std::string &subModelId, const std::string &iconId)
        : DeviceModel(modelId, subModelId, iconId) {}
    NearlinkDeviceModel& operator = (const NearlinkDeviceModel &other);
    NearlinkDeviceModel& operator = (const DeviceModel &other);

    bool Marshalling(Parcel &parcel) const override;

    static NearlinkDeviceModel *Unmarshalling(Parcel &parcel);

    bool WriteToParcel(Parcel &parcel);

    bool ReadFromParcel(Parcel &parcel);
};
}  // namespace Nearlink
}  // namespace OHOS

#endif // NEARLINK_PARCEL_DEVICE_MODEL_H
