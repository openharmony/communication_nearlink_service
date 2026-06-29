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

#include "nearlink_hadm_client_sounding_result.h"
#include "nearlink_raw_address.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
const uint32_t IQ_DATA_SIZE_MAX = 316; // 79 * 4 = 316
bool NearlinkHadmClientSoundingResult::Marshalling(Parcel &parcel) const
{
    if (!MarshalBasicFields(parcel)) {
        return false;
    }
    if (!MarshalIqDataFields(parcel)) {
        return false;
    }
    if (!MarshalTimingAndParams(parcel)) {
        return false;
    }
    if (!MarshalChmapArrays(parcel)) {
        return false;
    }
    if (!MarshalOffsetFields(parcel)) {
        return false;
    }
    return true;
}

NearlinkHadmClientSoundingResult *NearlinkHadmClientSoundingResult::Unmarshalling(Parcel &parcel)
{
    NearlinkHadmClientSoundingResult *result = new (std::nothrow) NearlinkHadmClientSoundingResult();
    if (result != nullptr && !result->ReadFromParcel(parcel)) {
        delete result;
        result = nullptr;
    }
    return result;
}

bool NearlinkHadmClientSoundingResult::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkHadmClientSoundingResult::ReadFromParcel(Parcel &parcel)
{
    if (!ReadBasicFields(parcel)) {
        return false;
    }
    if (!ReadIqDataFields(parcel)) {
        return false;
    }
    if (!ReadTimingAndParams(parcel)) {
        return false;
    }
    if (!ReadChmapArrays(parcel)) {
        return false;
    }
    if (!ReadOffsetFields(parcel)) {
        return false;
    }
    return true;
}

bool NearlinkHadmClientSoundingResult::MarshalBasicFields(Parcel &parcel) const
{
    NearlinkRawAddress addr(addr_.GetAddress());
    if (!parcel.WriteParcelable(&addr)) {
        return false;
    }
    if (!parcel.WriteUint8(dutRssi_)) {
        return false;
    }
    return parcel.WriteUint8(rtdRssi_);
}

bool NearlinkHadmClientSoundingResult::MarshalIqDataFields(Parcel &parcel) const
{
    if (!WriteIQData(dutIData_, parcel)) {
        return false;
    }
    if (!WriteIQData(rtdIData_, parcel)) {
        return false;
    }
    if (!WriteIQData(dutQData_, parcel)) {
        return false;
    }
    return WriteIQData(rtdQData_, parcel);
}

bool NearlinkHadmClientSoundingResult::MarshalTimingAndParams(Parcel &parcel) const
{
    if (!parcel.WriteUint16(dutTof_)) {
        return false;
    }
    if (!parcel.WriteUint16(rtdTof_)) {
        return false;
    }
    if (!parcel.WriteUint32(timeStampSn_)) {
        return false;
    }
    if (!parcel.WriteUint8(isMultiTone_)) {
        return false;
    }
    if (!parcel.WriteUint8(dutIqBitLen_)) {
        return false;
    }
    return parcel.WriteUint8(rtdIqBitLen_);
}

bool NearlinkHadmClientSoundingResult::MarshalChmapArrays(Parcel &parcel) const
{
    for (uint32_t i = 0; i < sizeof(dutSlemChmap_); i++) {
        if (!parcel.WriteUint8(dutSlemChmap_[i])) {
            return false;
        }
    }
    for (uint32_t i = 0; i < sizeof(rtdSlemChmap_); i++) {
        if (!parcel.WriteUint8(rtdSlemChmap_[i])) {
            return false;
        }
    }
    return true;
}

bool NearlinkHadmClientSoundingResult::MarshalOffsetFields(Parcel &parcel) const
{
    if (!parcel.WriteUint16(localNvOffset_)) {
        return false;
    }
    if (!parcel.WriteUint16(remoteNvOffset_)) {
        return false;
    }
    if (!parcel.WriteUint16(localTofOffset_)) {
        return false;
    }
    return parcel.WriteUint16(remoteTofOffset_);
}

bool NearlinkHadmClientSoundingResult::ReadBasicFields(Parcel &parcel)
{
    std::shared_ptr<NearlinkRawAddress> addr(parcel.ReadParcelable<NearlinkRawAddress>());
    if (!addr) {
        return false;
    }
    addr_ = *addr;
    if (!parcel.ReadUint8(dutRssi_)) {
        return false;
    }
    return parcel.ReadUint8(rtdRssi_);
}

bool NearlinkHadmClientSoundingResult::ReadIqDataFields(Parcel &parcel)
{
    if (!ReadIQData(dutIData_, parcel)) {
        return false;
    }
    if (!ReadIQData(rtdIData_, parcel)) {
        return false;
    }
    if (!ReadIQData(dutQData_, parcel)) {
        return false;
    }
    return ReadIQData(rtdQData_, parcel);
}

bool NearlinkHadmClientSoundingResult::ReadTimingAndParams(Parcel &parcel)
{
    if (!parcel.ReadUint16(dutTof_)) {
        return false;
    }
    if (!parcel.ReadUint16(rtdTof_)) {
        return false;
    }
    if (!parcel.ReadUint32(timeStampSn_)) {
        return false;
    }
    if (!parcel.ReadUint8(isMultiTone_)) {
        return false;
    }
    if (!parcel.ReadUint8(dutIqBitLen_)) {
        return false;
    }
    return parcel.ReadUint8(rtdIqBitLen_);
}

bool NearlinkHadmClientSoundingResult::ReadChmapArrays(Parcel &parcel)
{
    for (uint32_t i = 0; i < sizeof(dutSlemChmap_); i++) {
        if (!parcel.ReadUint8(dutSlemChmap_[i])) {
            return false;
        }
    }
    for (uint32_t i = 0; i < sizeof(rtdSlemChmap_); i++) {
        if (!parcel.ReadUint8(rtdSlemChmap_[i])) {
            return false;
        }
    }
    return true;
}

bool NearlinkHadmClientSoundingResult::ReadOffsetFields(Parcel &parcel)
{
    if (!parcel.ReadUint16(localNvOffset_)) {
        return false;
    }
    if (!parcel.ReadUint16(remoteNvOffset_)) {
        return false;
    }
    if (!parcel.ReadUint16(localTofOffset_)) {
        return false;
    }
    return parcel.ReadUint16(remoteTofOffset_);
}

bool NearlinkHadmClientSoundingResult::WriteIQData(std::vector<uint16_t> iqDatas, Parcel &parcel) const
{
    uint32_t size = iqDatas.size();
    if (!parcel.WriteUint32(size) || size > IQ_DATA_SIZE_MAX) {
        return false;
    }
    for (auto data : iqDatas) {
        if (!parcel.WriteUint16(data)) {
            return false;
        }
    }
    return true;
}

bool NearlinkHadmClientSoundingResult::ReadIQData(std::vector<uint16_t> &iqDatas, Parcel &parcel)
{
    uint32_t dataSize = 0;
    if (!parcel.ReadUint32(dataSize) || dataSize > IQ_DATA_SIZE_MAX) {
        return false;
    }
    for (uint32_t i = 0; i < dataSize; ++i) {
        uint16_t iqData;
        if (parcel.ReadUint16(iqData)) {
            iqDatas.push_back(iqData);
        } else {
            return false;
        }
    }
    return true;
}

}  // namespace Nearlink
}  // namespace OHOS