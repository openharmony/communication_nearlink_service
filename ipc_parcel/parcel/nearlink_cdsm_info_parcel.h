/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef NEARLINK_CDSM_INFO_PARCEL_H
#define NEARLINK_CDSM_INFO_PARCEL_H

#include "parcel.h"
#include "nearlink_cdsm_info.h"

namespace OHOS {
namespace Nearlink {
class NearlinkCdsMemberInfoParcel : public Parcelable, public NearlinkCdsMemberInfo {
public:
    NearlinkCdsMemberInfoParcel() = default;
    ~NearlinkCdsMemberInfoParcel() = default;
    explicit NearlinkCdsMemberInfoParcel(const std::string& addr) : NearlinkCdsMemberInfo(addr) {}
    explicit NearlinkCdsMemberInfoParcel(const NearlinkCdsMemberInfo &other) : NearlinkCdsMemberInfo(other) {}
    NearlinkCdsMemberInfoParcel(const NearlinkCdsMemberInfoParcel &other) = default;
    NearlinkCdsMemberInfoParcel &operator = (const NearlinkCdsMemberInfoParcel &other);

    bool Marshalling(Parcel &parcel) const override;
    static NearlinkCdsMemberInfoParcel *Unmarshalling(Parcel &parcel);

    bool WriteToParcel(Parcel &parcel);
    bool ReadFromParcel(Parcel &parcel);
};

class NearlinkCdsInfoParcel : public Parcelable, public NearlinkCdsInfo {
public:
    NearlinkCdsInfoParcel() = default;
    ~NearlinkCdsInfoParcel() = default;
    explicit NearlinkCdsInfoParcel(const NearlinkCdsInfo &other) : NearlinkCdsInfo(other) {}
    NearlinkCdsInfoParcel(const NearlinkCdsInfoParcel &other) = default;
    NearlinkCdsInfoParcel &operator = (const NearlinkCdsInfoParcel &other);

    bool Marshalling(Parcel &parcel) const override;
    static NearlinkCdsInfoParcel *Unmarshalling(Parcel &parcel);

    bool WriteToParcel(Parcel &parcel);
    bool ReadFromParcel(Parcel &parcel);
};
} // namespace Nearlink
} // namespace OHOS

#endif // NEARLINK_CDSM_INFO_PARCEL_H