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

#ifndef NEARLINK_PARCEL_SLE_ADVERTISER_SETTINGS_H
#define NEARLINK_PARCEL_SLE_ADVERTISER_SETTINGS_H

#include "nearlink_advertiser_def.h"
#include "parcel.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSleAdvertiserSettings : public Parcelable, public AdvertiserSettings {
public:
    explicit NearlinkSleAdvertiserSettings() = default;
    explicit NearlinkSleAdvertiserSettings(const AdvertiserSettings &other) : AdvertiserSettings(other)
    {}
    NearlinkSleAdvertiserSettings(const NearlinkSleAdvertiserSettings &other) : AdvertiserSettings(other)
    {}
    NearlinkSleAdvertiserSettings& operator = (const NearlinkSleAdvertiserSettings &other);
    ~NearlinkSleAdvertiserSettings() override = default;

    bool Marshalling(Parcel &parcel) const override;
    static NearlinkSleAdvertiserSettings *Unmarshalling(Parcel &parcel);

    bool WriteToParcel(Parcel &parcel);
    bool ReadFromParcel(Parcel &parcel);
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // NEARLINK_PARCEL_SLE_ADVERTISER_SETTINGS_H
