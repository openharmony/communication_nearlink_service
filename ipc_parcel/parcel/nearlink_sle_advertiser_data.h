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
#ifndef NEARLINK_PARCEL_SLE_ADVERTISER_DATA_H
#define NEARLINK_PARCEL_SLE_ADVERTISER_DATA_H

#include "nearlink_advertiser_def.h"
#include "parcel.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSleAdvertiserData : public Parcelable, public AdvertiserData {
public:
    explicit NearlinkSleAdvertiserData() = default;
    explicit NearlinkSleAdvertiserData(const AdvertiserData &other) : AdvertiserData(other)
    {}
    explicit NearlinkSleAdvertiserData(const NearlinkSleAdvertiserData &other) : AdvertiserData(other)
    {}
    NearlinkSleAdvertiserData& operator=(const AdvertiserData &other) = delete;
    NearlinkSleAdvertiserData& operator=(const NearlinkSleAdvertiserData &other) = delete;
    ~NearlinkSleAdvertiserData() override = default;

    bool Marshalling(Parcel &parcel) const override;

    static NearlinkSleAdvertiserData *Unmarshalling(Parcel &parcel);

    bool WriteToParcel(Parcel &parcel);
    bool ReadFromParcel(Parcel &parcel);

private:
    bool WriteServiceUuids(Parcel &parcel) const;
    bool ReadServiceUuids(std::vector<Uuid> &serviceUuids, Parcel &parcel);

    bool WriteManufacturerData(Parcel &parcel) const;
    bool ReadManufacturerData(std::map<uint16_t, std::string> &manufacturerData, Parcel &parcel);

    bool WriteServiceData(Parcel &parcel) const;
    bool ReadServiceData(std::map<Uuid, std::string> &serviceData, Parcel &parcel);
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // NEARLINK_PARCEL_SLE_ADVERTISER_DATA_H