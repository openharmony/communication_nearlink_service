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

#include "nearlink_uuid_parcel.h"
#include "nearlink_ssap_property_parcel.h"
#include "nearlink_ssap_method_parcel.h"
#include "nearlink_ssap_event_parcel.h"
#include "nearlink_ssap_descriptor_parcel.h"
#include "log.h"
#include "nearlink_ssap_service_parcel.h"

namespace OHOS {
namespace Nearlink {
const uint32_t SSAP_SERVICE_PARCEL_SIZE_MAX = 0x100;
bool NearlinkSsapServiceParcel::SsapServiceMemberWrite(Parcel &parcel) const
{
    uint32_t size = properties_.size();
    if (!parcel.WriteUint32(size) || size > SSAP_SERVICE_PARCEL_SIZE_MAX) {
        return false;
    }
    for (auto pp : properties_) {
        NearlinkSsapPropertyParcel property = NearlinkSsapPropertyParcel(pp);
        if (!parcel.WriteParcelable(&property)) {
            return false;
        }
    }
    size = methods_.size();
    if (!parcel.WriteUint32(size) || size > SSAP_SERVICE_PARCEL_SIZE_MAX) {
        return false;
    }
    for (auto mm : methods_) {
        NearlinkSsapMethodParcel method = NearlinkSsapMethodParcel(mm);
        if (!parcel.WriteParcelable(&method)) {
            return false;
        }
    }
    size = events_.size();
    if (!parcel.WriteUint32(size) || size > SSAP_SERVICE_PARCEL_SIZE_MAX) {
        return false;
    }
    for (auto ee : events_) {
        NearlinkSsapEventParcel event = NearlinkSsapEventParcel(ee);
        if (!parcel.WriteParcelable(&event)) {
            return false;
        }
    }
    size = descriptors_.size();
    if (!parcel.WriteUint32(size) || size > SSAP_SERVICE_PARCEL_SIZE_MAX) {
        return false;
    }
    for (auto &des : descriptors_) {
        NearlinkSsapDescriptorParcel descriptor =  NearlinkSsapDescriptorParcel(des);
        if (!parcel.WriteParcelable(&descriptor)) {
            return false;
        }
    }
    return true;
}

bool NearlinkSsapServiceParcel::SsapServiceMemberRead(Parcel &parcel)
{
    uint32_t size = 0;
    if (!parcel.ReadUint32(size) || size > SSAP_SERVICE_PARCEL_SIZE_MAX) {
        HILOGE("readfailed size value:%{public}u", size);
        return false;
    }
    for (size_t i = 0; i < size; i++) {
        std::shared_ptr<NearlinkSsapPropertyParcel> property(parcel.ReadParcelable<NearlinkSsapPropertyParcel>());
        if (!property) {
            return false;
        }
        properties_.push_back(*property);
    }
    if (!parcel.ReadUint32(size) || size > SSAP_SERVICE_PARCEL_SIZE_MAX) {
        HILOGE("readfailed size value:%{public}u", size);
        return false;
    }
    for (size_t i = 0; i < size; i++) {
        std::shared_ptr<NearlinkSsapMethodParcel> method(parcel.ReadParcelable<NearlinkSsapMethodParcel>());
        if (!method) {
            return false;
        }
        methods_.push_back(*method);
    }
    if (!parcel.ReadUint32(size) || size > SSAP_SERVICE_PARCEL_SIZE_MAX) {
        HILOGE("readfailed size value:%{public}u", size);
        return false;
    }
    for (size_t i = 0; i < size; i++) {
        std::shared_ptr<NearlinkSsapEventParcel> event(parcel.ReadParcelable<NearlinkSsapEventParcel>());
        if (!event) {
            return false;
        }
        events_.push_back(*event);
    }
    if (!parcel.ReadUint32(size) || size > SSAP_SERVICE_PARCEL_SIZE_MAX) {
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
    return true;
}

bool NearlinkSsapServiceParcel::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteUint16(handle_)) {
        return false;
    }
    if (!parcel.WriteUint16(startHandle_)) {
        return false;
    }
    if (!parcel.WriteUint16(endHandle_)) {
        return false;
    }
    if (!parcel.WriteBool(isPrimary_)) {
        return false;
    }
    NearlinkUuidParcel uuid(uuid_);
    if (!parcel.WriteParcelable(&uuid)) {
        return false;
    }
    if (!parcel.WriteUint32(opInd_)) {
        return false;
    }
    if (!SsapServiceMemberWrite(parcel)) {
        return false;
    }
    return true;
}

NearlinkSsapServiceParcel *NearlinkSsapServiceParcel::Unmarshalling(Parcel &parcel)
{
    NearlinkSsapServiceParcel *service = new (std::nothrow) NearlinkSsapServiceParcel();
    if (service != nullptr && !service->ReadFromParcel(parcel)) {
        delete service;
        service = nullptr;
    }
    return service;
}

bool NearlinkSsapServiceParcel::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkSsapServiceParcel::ReadFromParcel(Parcel &parcel)
{
    if (!parcel.ReadUint16(handle_)) {
        return false;
    }
    if (!parcel.ReadUint16(startHandle_)) {
        return false;
    }
    if (!parcel.ReadUint16(endHandle_)) {
        return false;
    }
    if (!parcel.ReadBool(isPrimary_)) {
        return false;
    }
    std::shared_ptr<NearlinkUuidParcel> uuid(parcel.ReadParcelable<NearlinkUuidParcel>());
    if (!uuid) {
        return false;
    }
    uuid_ = NearlinkUuidParcel(*uuid);
    if (!parcel.ReadUint32(opInd_)) {
        return false;
    }

    if (!SsapServiceMemberRead(parcel)) {
        return false;
    }
    return true;
}
}  // namespace Nearlink
}  // namespace OHOS