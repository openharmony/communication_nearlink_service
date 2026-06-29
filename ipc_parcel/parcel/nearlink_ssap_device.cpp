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

#include "nearlink_uuid.h"
#include "log.h"
#include "nearlink_raw_address.h"
#include "nearlink_ssap_device.h"

namespace OHOS {
namespace Nearlink {
bool NearlinkSsapDevice::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteUint8(transport_)) {
        return false;
    }
    if (!parcel.WriteString(addr_.GetAddress())) {
        return false;
    }
    return true;
}

NearlinkSsapDevice *NearlinkSsapDevice::Unmarshalling(Parcel &parcel)
{
    NearlinkSsapDevice *ssapDevice = new (std::nothrow) NearlinkSsapDevice();
    if (ssapDevice != nullptr && !ssapDevice->ReadFromParcel(parcel))  {
        delete ssapDevice;
        ssapDevice = nullptr;
    }
    return ssapDevice;
}

bool NearlinkSsapDevice::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkSsapDevice::ReadFromParcel(Parcel &parcel)
{
    if (!parcel.ReadUint8(transport_)) {
        return false;
    }
    std::string addr;
    if (!parcel.ReadString(addr)) {
        return false;
    }
    addr_ = RawAddress(addr);
    return true;
}
}  // namespace Nearlink
}  // namespace OHOS
