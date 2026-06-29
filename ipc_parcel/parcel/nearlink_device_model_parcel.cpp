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

#include "log.h"
#include "nearlink_device_model_parcel.h"

namespace OHOS {
namespace Nearlink {
NearlinkDeviceModel& NearlinkDeviceModel::operator = (const NearlinkDeviceModel &other)
{
    if (this != &other) {
        modelId_ = other.modelId_;
        subModelId_ = other.subModelId_;
        iconId_ = other.iconId_;
        devType_ = other.devType_;
    }
    return *this;
}

NearlinkDeviceModel& NearlinkDeviceModel::operator = (const DeviceModel &other)
{
    if (this != &other) {
        modelId_ = other.GetModelId();
        subModelId_ = other.GetSubModelId();
        iconId_ = other.GetIconId();
        devType_ = other.GetDevType();
    }
    return *this;
}

bool NearlinkDeviceModel::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(modelId_)) {
        return false;
    }
    if (!parcel.WriteString(subModelId_)) {
        return false;
    }
    if (!parcel.WriteString(iconId_)) {
        return false;
    }
    if (!parcel.WriteString(devType_)) {
        return false;
    }
    return true;
}

NearlinkDeviceModel *NearlinkDeviceModel::Unmarshalling(Parcel &parcel)
{
    NearlinkDeviceModel *model = new (std::nothrow) NearlinkDeviceModel();
    if (model != nullptr && !model->ReadFromParcel(parcel)) {
        delete model;
        model = nullptr;
    }
    return model;
}

bool NearlinkDeviceModel::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkDeviceModel::ReadFromParcel(Parcel &parcel)
{
    if (!parcel.ReadString(modelId_)) {
        return false;
    }
    if (!parcel.ReadString(subModelId_)) {
        return false;
    }
    if (!parcel.ReadString(iconId_)) {
        return false;
    }
    if (!parcel.ReadString(devType_)) {
        return false;
    }
    return true;
}
}  // namespace Nearlink
}  // namespace OHOS
