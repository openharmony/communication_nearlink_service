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

#include "nearlink_sle_scan_filter.h"
#include "nearlink_uuid_parcel.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
bool NearlinkSleScanFilter::Marshalling(Parcel &parcel) const
{
    NL_CHECK_RETURN_RET(parcel.WriteString(deviceId_), false, "WriteString wrong");
    NL_CHECK_RETURN_RET(parcel.WriteString(name_), false, "WriteString wrong");
    NL_CHECK_RETURN_RET(parcel.WriteBool(hasServiceUuid_), false, "WriteBool wrong");
    NL_CHECK_RETURN_RET(parcel.WriteBool(hasServiceUuidMask_), false, "WriteBool wrong");
    NL_CHECK_RETURN_RET(parcel.WriteBool(hasSolicitationUuid_), false, "WriteBool wrong");
    NL_CHECK_RETURN_RET(parcel.WriteBool(hasSolicitationUuidMask_), false, "WriteBool wrong");
    NL_CHECK_RETURN_RET(parcel.WriteBool(hasServiceData_), false, "WriteBool wrong");
    NL_CHECK_RETURN_RET(parcel.WriteBool(hasServiceDataMask_), false, "WriteBool wrong");
    NL_CHECK_RETURN_RET(WriteUuidToParcel(parcel), false, "WriteUuid wrong");
    NL_CHECK_RETURN_RET(parcel.WriteUInt8Vector(serviceData_), false, "WriteUInt8Vector wrong");
    NL_CHECK_RETURN_RET(parcel.WriteUInt8Vector(serviceDataMask_), false, "WriteUInt8Vector wrong");
    NL_CHECK_RETURN_RET(parcel.WriteUint16(manufacturerId_), false, "WriteUint16 wrong");
    NL_CHECK_RETURN_RET(parcel.WriteUInt8Vector(manufactureData_), false, "WriteUInt8Vector wrong");
    NL_CHECK_RETURN_RET(parcel.WriteUInt8Vector(manufactureDataMask_), false, "WriteUInt8Vector wrong");
    NL_CHECK_RETURN_RET(parcel.WriteBool(isSensorHubChannel_), false, "WriteBool wrong");
    NL_CHECK_RETURN_RET(parcel.WriteBool(advIndReport_), false, "WriteBool wrong");
    NL_CHECK_RETURN_RET(parcel.WriteInt8(rssiThreshold_), false, "WriteInt8 wrong");
    NL_CHECK_RETURN_RET(parcel.WriteBool(hasRssiThreshold_), false, "WriteBool wrong");
    return true;
}

NearlinkSleScanFilter *NearlinkSleScanFilter::Unmarshalling(Parcel &parcel)
{
    NearlinkSleScanFilter *filter = new (std::nothrow) NearlinkSleScanFilter();
    if (filter != nullptr && !filter->ReadFromParcel(parcel)) {
        delete filter;
        filter = nullptr;
    }
    return filter;
}

bool NearlinkSleScanFilter::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkSleScanFilter::ReadFromParcel(Parcel &parcel)
{
    NL_CHECK_RETURN_RET(parcel.ReadString(deviceId_), false, "ReadString wrong");
    NL_CHECK_RETURN_RET(parcel.ReadString(name_), false, "ReadString wrong");
    NL_CHECK_RETURN_RET(parcel.ReadBool(hasServiceUuid_), false, "ReadBool wrong");
    NL_CHECK_RETURN_RET(parcel.ReadBool(hasServiceUuidMask_), false, "ReadBool wrong");
    NL_CHECK_RETURN_RET(parcel.ReadBool(hasSolicitationUuid_), false, "ReadBool wrong");
    NL_CHECK_RETURN_RET(parcel.ReadBool(hasSolicitationUuidMask_), false, "ReadBool wrong");
    NL_CHECK_RETURN_RET(parcel.ReadBool(hasServiceData_), false, "ReadBool wrong");
    NL_CHECK_RETURN_RET(parcel.ReadBool(hasServiceDataMask_), false, "ReadBool wrong");
    NL_CHECK_RETURN_RET(ReadUuidFromParcel(parcel), false, "ReadUuid wrong");
    NL_CHECK_RETURN_RET(parcel.ReadUInt8Vector(&serviceData_), false, "ReadUInt8Vector wrong");
    NL_CHECK_RETURN_RET(parcel.ReadUInt8Vector(&serviceDataMask_), false, "ReadUInt8Vector wrong");
    NL_CHECK_RETURN_RET(parcel.ReadUint16(manufacturerId_), false, "ReadUint16 wrong");
    NL_CHECK_RETURN_RET(parcel.ReadUInt8Vector(&manufactureData_), false, "ReadUInt8Vector wrong");
    NL_CHECK_RETURN_RET(parcel.ReadUInt8Vector(&manufactureDataMask_), false, "ReadUInt8Vector wrong");
    NL_CHECK_RETURN_RET(parcel.ReadBool(isSensorHubChannel_), false, "ReadBool wrong");
    NL_CHECK_RETURN_RET(parcel.ReadBool(advIndReport_), false, "ReadBool wrong");
    NL_CHECK_RETURN_RET(parcel.ReadInt8(rssiThreshold_), false, "ReadInt8 wrong");
    NL_CHECK_RETURN_RET(parcel.ReadBool(hasRssiThreshold_), false, "ReadBool wrong");

    return true;
}

bool NearlinkSleScanFilter::WriteUuidToParcel(Parcel &parcel) const
{
    NearlinkUuidParcel serviceUuid = NearlinkUuidParcel(serviceUuid_);
    if (!parcel.WriteParcelable(&serviceUuid)) {
        return false;
    }
    NearlinkUuidParcel serviceUuidMask = NearlinkUuidParcel(serviceUuidMask_);
    if (!parcel.WriteParcelable(&serviceUuidMask)) {
        return false;
    }
    NearlinkUuidParcel serviceSolicitationUuid = NearlinkUuidParcel(serviceSolicitationUuid_);
    if (!parcel.WriteParcelable(&serviceSolicitationUuid)) {
        return false;
    }
    NearlinkUuidParcel serviceSolicitationUuidMask = NearlinkUuidParcel(serviceSolicitationUuidMask_);
    if (!parcel.WriteParcelable(&serviceSolicitationUuidMask)) {
        return false;
    }
    return true;
}

bool NearlinkSleScanFilter::ReadUuidFromParcel(Parcel &parcel)
{
    std::shared_ptr<NearlinkUuidParcel> serviceUuid(parcel.ReadParcelable<NearlinkUuidParcel>());
    if (!serviceUuid) {
        return false;
    }
    serviceUuid_ = NearlinkUuidParcel(*serviceUuid);
    std::shared_ptr<NearlinkUuidParcel> serviceUuidMask(parcel.ReadParcelable<NearlinkUuidParcel>());
    if (!serviceUuidMask) {
        return false;
    }
    serviceUuidMask_ = NearlinkUuidParcel(*serviceUuidMask);
    std::shared_ptr<NearlinkUuidParcel> serviceSolicitationUuid(parcel.ReadParcelable<NearlinkUuidParcel>());
    if (!serviceSolicitationUuid) {
        return false;
    }
    serviceSolicitationUuid_ = NearlinkUuidParcel(*serviceSolicitationUuid);
    std::shared_ptr<NearlinkUuidParcel> serviceSolicitationUuidMask(parcel.ReadParcelable<NearlinkUuidParcel>());
    if (!serviceSolicitationUuidMask) {
        return false;
    }
    serviceSolicitationUuidMask_ = NearlinkUuidParcel(*serviceSolicitationUuidMask);
    return true;
}
}  // namespace Nearlink
}  // namespace OHOS