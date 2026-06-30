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

#ifndef NEARLINK_SSAP_PROPERTY_PARCEL_H
#define NEARLINK_SSAP_PROPERTY_PARCEL_H

#include "ssap_data.h"
#include "parcel.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSsapPropertyParcel : public Parcelable, public Nearlink::Property {
public:
    NearlinkSsapPropertyParcel() = default;
    explicit NearlinkSsapPropertyParcel(const Nearlink::Property &other) : Nearlink::Property(other)
    {}
    explicit NearlinkSsapPropertyParcel(const NearlinkSsapPropertyParcel &other) : Nearlink::Property(other)
    {}
    NearlinkSsapPropertyParcel& operator = (const NearlinkSsapPropertyParcel &other);
    ~NearlinkSsapPropertyParcel() override = default;

    bool Marshalling(Parcel &parcel) const override;

    static NearlinkSsapPropertyParcel *Unmarshalling(Parcel &parcel);

    bool WriteToParcel(Parcel &parcel);

    bool ReadFromParcel(Parcel &parcel);
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // NEARLINK_SSAP_DESCRIPTOR_PARCEL_H