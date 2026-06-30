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

#include "securec.h"
#include "nearlink_uuid_parcel.h"
#include "nearlink_ssap_descriptor_parcel.h"
#include "nearlink_uuid_parcel.h"
#include "log.h"
#include "nearlink_ssap_property_parcel.h"

namespace OHOS {
namespace Nearlink {
const uint32_t SSAP_PROPERTY_PARCEL_SIZE_MAX = 0x1000;
bool NearlinkSsapPropertyParcel::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteUint16(handle_)) {
        return false;
    }
    NearlinkUuidParcel uuid(uuid_);
    if (!parcel.WriteParcelable(&uuid)) {
        return false;
    }
    if (!parcel.WriteUint32(value_.size()) || value_.size() > SSAP_PROPERTY_PARCEL_SIZE_MAX) {
        return false;
    }
    for (size_t i = 0; i < value_.size(); i++) {
        if (!parcel.WriteUint8(value_[i])) {
            return false;
        }
    }
    if (!parcel.WriteUint32(opInd_)) {
        return false;
    }
    uint32_t size = descriptors_.size();
    if (!parcel.WriteUint32(size) || size > SSAP_PROPERTY_PARCEL_SIZE_MAX) {
        return false;
    }
    for (auto &des : descriptors_) {
        NearlinkSsapDescriptorParcel descriptor =  NearlinkSsapDescriptorParcel(des);
        if (!parcel.WriteParcelable(&descriptor)) {
            return false;
        }
    }
    if (!parcel.WriteInt32(permission_)) {
        return false;
    }
    return true;
}

NearlinkSsapPropertyParcel *NearlinkSsapPropertyParcel::Unmarshalling(Parcel &parcel)
{
    NearlinkSsapPropertyParcel *property = new (std::nothrow) NearlinkSsapPropertyParcel();
    if (property != nullptr && !property->ReadFromParcel(parcel)) {
        delete property;
        property = nullptr;
    }
    return property;
}

bool NearlinkSsapPropertyParcel::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkSsapPropertyParcel::ReadFromParcel(Parcel &parcel)
{
    if (!parcel.ReadUint16(handle_)) {
        return false;
    }
    std::shared_ptr<NearlinkUuidParcel> uuid(parcel.ReadParcelable<NearlinkUuidParcel>());
    if (!uuid) {
        return false;
    }
    uuid_ = NearlinkUuidParcel(*uuid);
    uint32_t s;
    if (!parcel.ReadUint32(s)|| s > SSAP_PROPERTY_PARCEL_SIZE_MAX) {
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
    if (!parcel.ReadUint32(opInd_)) {
        return false;
    }
    uint32_t size = 0;
    if (!parcel.ReadUint32(size) || size > SSAP_PROPERTY_PARCEL_SIZE_MAX) {
        HILOGE("read parcel size error, size=0x%{public}x", size);
        return false;
    }
    for (size_t i = 0; i < size; i++) {
        std::shared_ptr<NearlinkSsapDescriptorParcel> descriptor(parcel.ReadParcelable<NearlinkSsapDescriptorParcel>());
        if (!descriptor) {
            return false;
        }
        descriptors_.push_back(*descriptor);
    }
    if (!parcel.ReadUint16(permission_)) {
        return false;
    }
    return true;
}
}  // namespace Nearlink
}  // namespace OHOS