/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "securec.h"
#include "log.h"
#include "nearlink_ssap_descriptor_parcel.h"

namespace OHOS {
namespace Nearlink {
const uint32_t SSAP_DESCRIPTOR_PARCEL_SIZE_MAX = 0x100;
bool NearlinkSsapDescriptorParcel::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteUint16(handle_)) {
        return false;
    }
    if (!parcel.WriteUint8(type_)) {
        return false;
    }
    if (!parcel.WriteUint32(value_.size())|| value_.size() > SSAP_DESCRIPTOR_PARCEL_SIZE_MAX) {
        return false;
    }
    for (size_t i = 0; i < value_.size(); i++) {
        if (!parcel.WriteUint8(value_[i])) {
            return false;
        }
    }
    if (!parcel.WriteInt8(writeInd_)) {
        return false;
    }
    if (!parcel.WriteInt16(permission_)) {
        return false;
    }
    return true;
}

NearlinkSsapDescriptorParcel *NearlinkSsapDescriptorParcel::Unmarshalling(Parcel &parcel)
{
    NearlinkSsapDescriptorParcel *descriptor = new (std::nothrow) NearlinkSsapDescriptorParcel();
    if (descriptor != nullptr && !descriptor->ReadFromParcel(parcel)) {
        delete descriptor;
        descriptor = nullptr;
    }
    return descriptor;
}

bool NearlinkSsapDescriptorParcel::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkSsapDescriptorParcel::ReadFromParcel(Parcel &parcel)
{
    if (!parcel.ReadUint16(handle_)) {
        return false;
    }
    if (!parcel.ReadUint8(type_)) {
        return false;
    }
    uint32_t s;
    if (!parcel.ReadUint32(s)|| s > SSAP_DESCRIPTOR_PARCEL_SIZE_MAX) {
        HILOGE("read parcel size error, size=0x%{public}x", s);
        return false;
    }
    std::vector<uint8_t> value;
    uint8_t v;
    for (size_t i = 0; i < s; i++) {
        if (!parcel.ReadUint8(v)) {
            return false;
        }
        value.emplace_back(v);
    }
    value_ = std::move(value);
    if (!parcel.ReadUint8(writeInd_)) {
        return false;
    }
    if (!parcel.ReadUint16(permission_)) {
        return false;
    }
    return true;
}
}  // namespace Nearlink
}  // namespace OHOS
