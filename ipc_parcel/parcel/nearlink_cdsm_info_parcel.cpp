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
#include "nearlink_cdsm_info_parcel.h"

#include <memory>

namespace OHOS {
namespace Nearlink {
namespace {
    constexpr uint8_t SLE_CDSM_GROUP_MAX_SIZE = 0xFF;
}
NearlinkCdsMemberInfoParcel &NearlinkCdsMemberInfoParcel::operator = (const NearlinkCdsMemberInfoParcel &other)
{
    if (this != &other) {
        addr_ = other.addr_;
        state_ = other.state_;
    }
    return *this;
}

bool NearlinkCdsMemberInfoParcel::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(addr_)) {
        return false;
    }
    if (!parcel.WriteUint8(state_)) {
        return false;
    }
    return true;
}

NearlinkCdsMemberInfoParcel* NearlinkCdsMemberInfoParcel::Unmarshalling(Parcel &parcel)
{
    NearlinkCdsMemberInfoParcel *result = new (std::nothrow) NearlinkCdsMemberInfoParcel();
    if (result != nullptr && !result->ReadFromParcel(parcel)) {
        delete result;
        result = nullptr;
    }
    return result;
}

bool NearlinkCdsMemberInfoParcel::WriteToParcel(Parcel &parcel)
{
    return NearlinkCdsMemberInfoParcel::Marshalling(parcel);
}

bool NearlinkCdsMemberInfoParcel::ReadFromParcel(Parcel &parcel)
{
    if (!parcel.ReadString(addr_)) {
        return false;
    }
    if (!parcel.ReadUint8(state_)) {
        return false;
    }
    return true;
}

NearlinkCdsInfoParcel &NearlinkCdsInfoParcel::operator = (const NearlinkCdsInfoParcel &other)
{
    if (this != &other) {
        members_ = other.members_;
    }
    return *this;
}

bool NearlinkCdsInfoParcel::Marshalling(Parcel &parcel) const
{
    size_t memberSize = members_.size();
    if (memberSize > SLE_CDSM_GROUP_MAX_SIZE) {
        return false;
    }
    if (!parcel.WriteInt32(members_.size())) {
        return false;
    }
    for (const auto& iter : members_) {
        NearlinkCdsMemberInfoParcel memberParcel(iter);
        if (!parcel.WriteParcelable(&memberParcel)) {
            return false;
        }
    }
    return true;
}

NearlinkCdsInfoParcel* NearlinkCdsInfoParcel::Unmarshalling(Parcel &parcel)
{
    NearlinkCdsInfoParcel *result = new (std::nothrow) NearlinkCdsInfoParcel();
    if (result != nullptr && !result->ReadFromParcel(parcel)) {
        delete result;
        result = nullptr;
    }
    return result;
}

bool NearlinkCdsInfoParcel::WriteToParcel(Parcel &parcel)
{
    return NearlinkCdsInfoParcel::Marshalling(parcel);
}

bool NearlinkCdsInfoParcel::ReadFromParcel(Parcel &parcel)
{
    int32_t size = 0;
    if (!parcel.ReadInt32(size) || size > SLE_CDSM_GROUP_MAX_SIZE) {
        HILOGE("read Parcelable size failed.");
        return false;
    }
    for (int i = 0; i < size; ++i) {
        std::shared_ptr<NearlinkCdsMemberInfoParcel> memberParcelPtr(
            parcel.ReadParcelable<NearlinkCdsMemberInfoParcel>());
        if (!memberParcelPtr) {
            return false;
        }
        NearlinkCdsMemberInfoParcel memberInfo = NearlinkCdsMemberInfoParcel(*memberParcelPtr);
        members_.push_back(memberInfo);
    }
    return true;
}
} // namespace Nearlink
} // namespace OHOS