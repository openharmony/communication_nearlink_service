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

#ifndef NEARLINK_HADM_CLIENT_SOUNDING_RESULT_H
#define NEARLINK_HADM_CLIENT_SOUNDING_RESULT_H

#include "nearlink_hadm_sounding_result.h"
#include "parcel.h"

namespace OHOS {
namespace Nearlink {
class NearlinkHadmClientSoundingResult : public Parcelable, public NearlinkHadmSoundingResult {
public:
    explicit NearlinkHadmClientSoundingResult() = default;
    explicit NearlinkHadmClientSoundingResult(
        const NearlinkHadmClientSoundingResult &other) : NearlinkHadmSoundingResult(other)
    {}
    explicit NearlinkHadmClientSoundingResult(
        const NearlinkHadmSoundingResult &other) : NearlinkHadmSoundingResult(other)
    {}
    NearlinkHadmClientSoundingResult& operator = (const NearlinkHadmClientSoundingResult &other);

    ~NearlinkHadmClientSoundingResult() override = default;

    bool Marshalling(Parcel &parcel) const override;
    static NearlinkHadmClientSoundingResult *Unmarshalling(Parcel &parcel);

    bool WriteToParcel(Parcel &parcel);
    bool ReadFromParcel(Parcel &parcel);

private:
    bool WriteIQData(std::vector<uint16_t> iqDatas, Parcel &parcel) const;
    bool ReadIQData(std::vector<uint16_t> &iqDatas, Parcel &parcel);
    bool MarshalBasicFields(Parcel &parcel) const;
    bool MarshalIqDataFields(Parcel &parcel) const;
    bool MarshalTimingAndParams(Parcel &parcel) const;
    bool MarshalChmapArrays(Parcel &parcel) const;
    bool MarshalOffsetFields(Parcel &parcel) const;
    bool ReadBasicFields(Parcel &parcel);
    bool ReadIqDataFields(Parcel &parcel);
    bool ReadTimingAndParams(Parcel &parcel);
    bool ReadChmapArrays(Parcel &parcel);
    bool ReadOffsetFields(Parcel &parcel);
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // NEARLINK_HADM_CLIENT_SOUNDING_RESULT_H