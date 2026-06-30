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

#ifndef NEARLINK_PARCEL_SLE_SCAN_SETTINGS_H
#define NEARLINK_PARCEL_SLE_SCAN_SETTINGS_H

#include "nearlink_scan_def.h"
#include "parcel.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSleScanSettings : public Parcelable, public ScanSettings {
public:
    explicit NearlinkSleScanSettings() = default;
    explicit NearlinkSleScanSettings(const ScanSettings &other) : ScanSettings(other)
    {}
    explicit NearlinkSleScanSettings(const SleScanSettings &other) : ScanSettings(other)
    {}
    NearlinkSleScanSettings(const NearlinkSleScanSettings &other) : ScanSettings(other)
    {}
    NearlinkSleScanSettings& operator = (const NearlinkSleScanSettings &other);
    ~NearlinkSleScanSettings() override = default;

    bool Marshalling(Parcel &parcel) const override;
    static NearlinkSleScanSettings *Unmarshalling(Parcel &parcel);

    bool WriteToParcel(Parcel &parcel);
    bool ReadFromParcel(Parcel &parcel);
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // NEARLINK_PARCEL_SLE_SCAN_SETTINGS_H
