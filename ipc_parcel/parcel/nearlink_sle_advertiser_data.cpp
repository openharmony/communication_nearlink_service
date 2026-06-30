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

#include "nearlink_sle_advertiser_data.h"
#include "log.h"
#include "sle_uuid.h"
#include "nearlink_uuid_parcel.h"

namespace OHOS {
namespace Nearlink {
const uint64_t SLE_ADV_SERVICE_READ_DATA_SIZE_MAX_LEN = 0x400;
bool NearlinkSleAdvertiserData::Marshalling(Parcel &parcel) const
{
    if (!WriteServiceUuids(parcel)) {
        return false;
    }
    if (!WriteManufacturerData(parcel)) {
        return false;
    }
    if (!WriteServiceData(parcel)) {
        return false;
    }
    if (!parcel.WriteUint8(advFlag_)) {
        return false;
    }
    if (!parcel.WriteString(payload_)) {
        return false;
    }
    if (!parcel.WriteBool(includeDeviceName_)) {
        return false;
    }
    if (!parcel.WriteBool(includeTxPower_)) {
        return false;
    }
    return true;
}

bool NearlinkSleAdvertiserData::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

NearlinkSleAdvertiserData *NearlinkSleAdvertiserData::Unmarshalling(Parcel &parcel)
{
    NearlinkSleAdvertiserData *advertiserData = new (std::nothrow) NearlinkSleAdvertiserData();
    if (advertiserData != nullptr && !advertiserData->ReadFromParcel(parcel)) {
        delete advertiserData;
        advertiserData = nullptr;
    }
    return advertiserData;
}

bool NearlinkSleAdvertiserData::ReadFromParcel(Parcel &parcel)
{
    if (!ReadServiceUuids(serviceUuids_, parcel)) {
        return false;
    }
    if (!ReadManufacturerData(manufacturerSpecificData_, parcel)) {
        return false;
    }
    if (!ReadServiceData(serviceData_, parcel)) {
        return false;
    }
    if (!parcel.ReadUint8(advFlag_)) {
        return false;
    }
    if (!parcel.ReadString(payload_)) {
        return false;
    }
    if (!parcel.ReadBool(includeDeviceName_)) {
        return false;
    }
    if (!parcel.ReadBool(includeTxPower_)) {
        return false;
    }
    return true;
}

bool NearlinkSleAdvertiserData::WriteServiceUuids(Parcel &parcel) const
{
    if (serviceUuids_.size() > SLE_ADV_SERVICE_READ_DATA_SIZE_MAX_LEN || !parcel.WriteUint64(serviceUuids_.size())) {
        return false;
    }
    for (auto &serviceUuids : serviceUuids_) {
        NearlinkUuidParcel uuid(serviceUuids);
        if (!parcel.WriteParcelable(&uuid)) {
            return false;
        }
    }
    return true;
}

bool NearlinkSleAdvertiserData::ReadServiceUuids(std::vector<Uuid> &serviceUuids, Parcel &parcel)
{
    uint64_t serviceUuidSize = 0;
    if (!parcel.ReadUint64(serviceUuidSize) || serviceUuidSize > SLE_ADV_SERVICE_READ_DATA_SIZE_MAX_LEN) {
        HILOGE("read Parcelable size failed.");
        return false;
    }
    Uuid tmpUuid;
    for (uint64_t i = 0; i < serviceUuidSize; ++i) {
        std::shared_ptr<NearlinkUuidParcel> serviceUuid(parcel.ReadParcelable<NearlinkUuidParcel>());
        if (!serviceUuid) {
            return false;
        }
        tmpUuid = NearlinkUuidParcel(*serviceUuid);
        serviceUuids.push_back(tmpUuid);
    }
    return true;
}

bool NearlinkSleAdvertiserData::WriteManufacturerData(Parcel &parcel) const
{
    if (manufacturerSpecificData_.size() > SLE_ADV_SERVICE_READ_DATA_SIZE_MAX_LEN ||
        !parcel.WriteUint64(manufacturerSpecificData_.size())) {
        return false;
    }
    for (auto &manufacturer : manufacturerSpecificData_) {
        if (!parcel.WriteUint16(manufacturer.first)) {
            return false;
        }
        if (!parcel.WriteString(manufacturer.second)) {
            return false;
        }
    }
    return true;
}

bool NearlinkSleAdvertiserData::ReadManufacturerData(std::map<uint16_t, std::string> &manufacturerData, Parcel &parcel)
{
    uint64_t manufacturerSize = 0;
    if (!parcel.ReadUint64(manufacturerSize) || manufacturerSize > SLE_ADV_SERVICE_READ_DATA_SIZE_MAX_LEN) {
        HILOGE("read Parcelable size failed.");
        return false;
    }
    uint16_t manufacturerId;
    std::string manufacturerDataValue;
    for (uint64_t i = 0; i < manufacturerSize; ++i) {
        if (!parcel.ReadUint16(manufacturerId)) {
            return false;
        }
        if (!parcel.ReadString(manufacturerDataValue)) {
            return false;
        }
        manufacturerData.emplace(manufacturerId, manufacturerDataValue);
    }
    return true;
}

bool NearlinkSleAdvertiserData::WriteServiceData(Parcel &parcel) const
{
    if (serviceData_.size() > SLE_ADV_SERVICE_READ_DATA_SIZE_MAX_LEN || !parcel.WriteUint64(serviceData_.size())) {
        return false;
    }
    for (auto &service : serviceData_) {
        NearlinkUuidParcel uuid(service.first);
        if (!parcel.WriteParcelable(&uuid)) {
            return false;
        }
        if (!parcel.WriteString(service.second)) {
            return false;
        }
    }
    return true;
}

bool NearlinkSleAdvertiserData::ReadServiceData(std::map<Uuid, std::string> &serviceData, Parcel &parcel)
{
    uint64_t serviceDataSize = 0;
    if (!parcel.ReadUint64(serviceDataSize) || serviceDataSize > SLE_ADV_SERVICE_READ_DATA_SIZE_MAX_LEN) {
        HILOGE("read Parcelable size failed.");
        return false;
    }
    Uuid serviceDataId;
    std::string serviceDataData;
    for (uint64_t i = 0; i < serviceDataSize; ++i) {
        std::shared_ptr<NearlinkUuidParcel> uuid(parcel.ReadParcelable<NearlinkUuidParcel>());
        if (!uuid) {
            return false;
        }
        serviceDataId = NearlinkUuidParcel(*uuid);
        if (!parcel.ReadString(serviceDataData)) {
            return false;
        }
        serviceData.emplace(serviceDataId, serviceDataData);
    }
    return true;
}
}  // namespace Nearlink
}  // namespace OHOS
