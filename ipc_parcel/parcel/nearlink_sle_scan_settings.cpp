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

#include "nearlink_sle_scan_settings.h"

namespace OHOS {
namespace Nearlink {
bool NearlinkSleScanSettings::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt64(reportDelayMillis_)) {
        return false;
    }
    if (!parcel.WriteInt32(duration_)) {
        return false;
    }
    if (!parcel.WriteInt32(scanMode_)) {
        return false;
    }
    if (!parcel.WriteBool(legacy_)) {
        return false;
    }
    if (!parcel.WriteInt32(phy_)) {
        return false;
    }
    if (!parcel.WriteUint8(frameType_)) {
        return false;
    }
    return true;
}

NearlinkSleScanSettings *NearlinkSleScanSettings::Unmarshalling(Parcel &parcel)
{
    NearlinkSleScanSettings *settings = new (std::nothrow) NearlinkSleScanSettings();
    if (settings != nullptr && !settings->ReadFromParcel(parcel)) {
        delete settings;
        settings = nullptr;
    }
    return settings;
}

bool NearlinkSleScanSettings::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkSleScanSettings::ReadFromParcel(Parcel &parcel)
{
    int64_t reportDelayMillis = 0;
    if (parcel.ReadInt64(reportDelayMillis)) {
        NearlinkSleScanSettings::SetReportDelay(reportDelayMillis);
    } else {
        return false;
    }
    if (!parcel.ReadInt32(duration_)) {
        return false;
    }
    if (!parcel.ReadInt32(scanMode_)) {
        return false;
    }
    if (!parcel.ReadBool(legacy_)) {
        return false;
    }
    if (!parcel.ReadInt32(phy_)) {
        return false;
    }
    if (!parcel.ReadUint8(frameType_)) {
        return false;
    }
    return true;
}
}  // namespace Nearlink
}  // namespace OHOS
