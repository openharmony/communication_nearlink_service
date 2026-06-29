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
#include "nearlink_uuid_parcel.h"
#include "log.h"
#include "nearlink_ssap_event_parcel.h"

namespace OHOS {
namespace Nearlink {
const uint32_t SSAP_EVENT_PARCEL_SIZE_MAX = 0x1000;
bool NearlinkSsapEventParcel::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteUint16(handle_)) {
        return false;
    }
    NearlinkUuidParcel uuid(uuid_);
    if (!parcel.WriteParcelable(&uuid)) {
        return false;
    }
    if (parameter_.size() > SSAP_EVENT_PARCEL_SIZE_MAX || !parcel.WriteUint32(parameter_.size())) {
        return false;
    }
    for (size_t i = 0; i < parameter_.size(); i++) {
        if (!parcel.WriteUint8(parameter_[i])) {
            return false;
        }
    }

    return true;
}

NearlinkSsapEventParcel *NearlinkSsapEventParcel::Unmarshalling(Parcel &parcel)
{
    NearlinkSsapEventParcel *event = new (std::nothrow) NearlinkSsapEventParcel();
    if (event != nullptr && !event->ReadFromParcel(parcel)) {
        delete event;
        event = nullptr;
    }
    return event;
}

bool NearlinkSsapEventParcel::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkSsapEventParcel::ReadFromParcel(Parcel &parcel)
{
    if (!parcel.ReadUint16(handle_)) {
        return false;
    }
    std::shared_ptr<NearlinkUuidParcel> uuid(parcel.ReadParcelable<NearlinkUuidParcel>());
    if (!uuid) {
        return false;
    }
    uuid_ = NearlinkUuidParcel(*uuid);
    uint32_t ps;
    if (!parcel.ReadUint32(ps)|| ps > SSAP_EVENT_PARCEL_SIZE_MAX) {
        HILOGE("read parcel parameter size error, parameter size=0x%{public}x", ps);
        return false;
    }
    std::vector<uint8_t> parameter;
    uint8_t p;
    for (size_t i = 0; i < ps; i++) {
        if (!parcel.ReadUint8(p)) {
            return false;
        }
        parameter.emplace_back(p);
    }
    parameter_ = std::move(parameter);
    return true;
}
}  // namespace Nearlink
}  // namespace OHOS