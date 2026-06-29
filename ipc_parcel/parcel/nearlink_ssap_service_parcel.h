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

#ifndef NEARLINK_PARCEL_SSAP_SERVICES_H
#define NEARLINK_PARCEL_SSAP_SERVICES_H

#include "ssap_data.h"
#include "parcel.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSsapServiceParcel : public Parcelable, public Nearlink::Service {
public:
    NearlinkSsapServiceParcel() = default;
    explicit NearlinkSsapServiceParcel(const Nearlink::Service &other) : Nearlink::Service(other)
    {}
    explicit NearlinkSsapServiceParcel(const NearlinkSsapServiceParcel &other) : Nearlink::Service(other)
    {}
    NearlinkSsapServiceParcel& operator = (const NearlinkSsapServiceParcel &other);

    ~NearlinkSsapServiceParcel() override = default;

    bool SsapServiceMemberWrite(Parcel &parcel) const;

    bool SsapServiceMemberRead(Parcel &parcel);

    bool Marshalling(Parcel &parcel) const override;

    static NearlinkSsapServiceParcel *Unmarshalling(Parcel &parcel);

    bool WriteToParcel(Parcel &parcel);

    bool ReadFromParcel(Parcel &parcel);
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // NEARLINK_PARCEL_GATT_SERVICES_H