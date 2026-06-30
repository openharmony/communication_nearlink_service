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

#include "nearlink_sle_scan_result.h"
#include "log.h"
#include "nearlink_uuid_parcel.h"

namespace OHOS {
namespace Nearlink {
const uint64_t SLE_SCAN_READ_DATA_SIZE_MAX_LEN = 0x400;
bool NearlinkSleScanResult::Marshalling(Parcel &parcel) const
{
    if (!WriteServiceUuidsToParcel(parcel)) {
        return false;
    }
    if (!WriteManufacturerDataToParcel(parcel)) {
        return false;
    }
    if (!WriteServiceDataToParcel(parcel)) {
        return false;
    }
    if (!parcel.WriteString(addr_.GetAddress())) {
        return false;
    }
    if (!parcel.WriteInt8(rssi_)) {
        return false;
    }
    if (!parcel.WriteBool(connectable_)) {
        return false;
    }
    if (!parcel.WriteUint8(advertiseFlag_)) {
        return false;
    }
    if (!parcel.WriteString(payload_)) {
        return false;
    }
    if (!parcel.WriteString(name_)) {
        return false;
    }
    if (!parcel.WriteUint32(deviceClass_)) {
        return false;
    }
    if (!parcel.WriteUint8(primFrameType_)) {
        return false;
    }
    return true;
}

NearlinkSleScanResult *NearlinkSleScanResult::Unmarshalling(Parcel &parcel)
{
    NearlinkSleScanResult *settings = new (std::nothrow) NearlinkSleScanResult();
    if (settings != nullptr && !settings->ReadFromParcel(parcel)) {
        delete settings;
        settings = nullptr;
    }
    return settings;
}

bool NearlinkSleScanResult::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkSleScanResult::ReadFromParcel(Parcel &parcel)
{
    if (!ReadServiceUuidsFromParcel(parcel)) {
        return false;
    }
    if (!ReadManufacturerDataFromParcel(parcel)) {
        return false;
    }
    if (!ReadServiceDataFromParcel(parcel)) {
        return false;
    }
    std::string address = "";
    if (parcel.ReadString(address)) {
        addr_ = RawAddress(address);
    } else {
        return false;
    }
    if (!parcel.ReadInt8(rssi_)) {
        return false;
    }
    if (!parcel.ReadBool(connectable_)) {
        return false;
    }
    if (!parcel.ReadUint8(advertiseFlag_)) {
        return false;
    }
    if (!parcel.ReadString(payload_)) {
        return false;
    }
    if (!parcel.ReadString(name_)) {
        return false;
    }
    if (!parcel.ReadUint32(deviceClass_)) {
        return false;
    }
    if (!parcel.ReadUint8(primFrameType_)) {
        return false;
    }
    return true;
}

bool NearlinkSleScanResult::WriteServiceUuidsToParcel(Parcel &parcel) const
{
    if (serviceUuids_.size() > SLE_SCAN_READ_DATA_SIZE_MAX_LEN || !parcel.WriteUint64(serviceUuids_.size())) {
        return false;
    }
    for (auto iter : serviceUuids_) {
        NearlinkUuidParcel uuid(iter);
        if (!parcel.WriteParcelable(&uuid)) {
            return false;
        }
    }
    return true;
}

bool NearlinkSleScanResult::ReadServiceUuidsFromParcel(Parcel &parcel)
{
    uint64_t uuidSize = 0;
    if (!parcel.ReadUint64(uuidSize) || uuidSize > SLE_SCAN_READ_DATA_SIZE_MAX_LEN) {
        HILOGE("read Parcelable size failed.");
        return false;
    }
    Uuid tmpUuid;
    for (uint64_t i = 0; i < uuidSize; ++i) {
        std::shared_ptr<NearlinkUuidParcel> serviceUuid(parcel.ReadParcelable<NearlinkUuidParcel>());
        if (!serviceUuid) {
            return false;
        }
        tmpUuid = NearlinkUuidParcel(*serviceUuid);
        serviceUuids_.push_back(tmpUuid);
    }
    return true;
}

bool NearlinkSleScanResult::WriteManufacturerDataToParcel(Parcel &parcel) const
{
    if (manufacturerSpecificData_.size() > SLE_SCAN_READ_DATA_SIZE_MAX_LEN ||
        !parcel.WriteUint64(manufacturerSpecificData_.size())) {
        return false;
    }
    for (auto iter = manufacturerSpecificData_.begin(); iter != manufacturerSpecificData_.end(); ++iter) {
        if (!parcel.WriteUint16(iter->first)) {
            return false;
        }
        if (!parcel.WriteString(iter->second)) {
            return false;
        }
    }
    return true;
}

bool NearlinkSleScanResult::ReadManufacturerDataFromParcel(Parcel &parcel)
{
    uint64_t manuSize = 0;
    if (!parcel.ReadUint64(manuSize) || manuSize > SLE_SCAN_READ_DATA_SIZE_MAX_LEN) {
        HILOGE("read Parcelable size failed.");
        return false;
    }
    for (uint64_t i = 0; i < manuSize; i++) {
        uint16_t manufacturerId = 0;
        std::string manufacturedData;
        if (!parcel.ReadUint16(manufacturerId)) {
            return false;
        }
        if (!parcel.ReadString(manufacturedData)) {
            return false;
        }
        manufacturerSpecificData_.emplace(manufacturerId, manufacturedData);
    }
    return true;
}

bool NearlinkSleScanResult::WriteServiceDataToParcel(Parcel &parcel) const
{
    if (serviceData_.size() > SLE_SCAN_READ_DATA_SIZE_MAX_LEN || !parcel.WriteUint64(serviceData_.size())) {
        return false;
    }
    for (auto iter = serviceData_.begin(); iter != serviceData_.end(); ++iter) {
        NearlinkUuidParcel uuid(iter->first);
        if (!parcel.WriteParcelable(&uuid)) {
            return false;
        }
        if (!parcel.WriteString(iter->second)) {
            return false;
        }
    }
    return true;
}

bool NearlinkSleScanResult::ReadServiceDataFromParcel(Parcel &parcel)
{
    uint64_t serviceDataSize = 0;
    if (!parcel.ReadUint64(serviceDataSize) || serviceDataSize > SLE_SCAN_READ_DATA_SIZE_MAX_LEN) {
        HILOGE("read Parcelable size failed.");
        return false;
    }
    Uuid tmpUuid;
    for (uint64_t i = 0; i < serviceDataSize; i++) {
        std::shared_ptr<NearlinkUuidParcel> serviceUuid(parcel.ReadParcelable<NearlinkUuidParcel>());
        if (!serviceUuid) {
            return false;
        }
        tmpUuid = NearlinkUuidParcel(*serviceUuid);

        std::string serviceData;
        if (!parcel.ReadString(serviceData)) {
            return false;
        }

        serviceData_.emplace(tmpUuid, serviceData);
    }
    return true;
}
}  // namespace Nearlink
}  // namespace OHOS