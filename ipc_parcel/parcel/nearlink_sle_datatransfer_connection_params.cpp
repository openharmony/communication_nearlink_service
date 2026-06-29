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

#include "nearlink_sle_datatransfer_connection_params.h"
#include "log.h"

namespace OHOS::Nearlink {
bool NearlinkSleDataTransferConnectionParams::Marshalling(Parcel &parcel) const
{
    NL_CHECK_RETURN_RET(parcel.WriteString(address_), false, "write address_ failed");
    NL_CHECK_RETURN_RET(parcel.WriteString(uuid_), false, "write uuid_ failed");
    NL_CHECK_RETURN_RET(parcel.WriteUint16(port_), false, "write port_ failed");
    NL_CHECK_RETURN_RET(parcel.WriteInt32(state_), false, "write state_ failed");
    NL_CHECK_RETURN_RET(parcel.WriteUint8(transMode_), false, "write transMode_ failed");
    NL_CHECK_RETURN_RET(parcel.WriteUint8(frameType_), false, "write frameType_ failed");
    NL_CHECK_RETURN_RET(parcel.WriteUint16(mtu_), false, "write mtu_ failed");
    return true;
}

NearlinkSleDataTransferConnectionParams *NearlinkSleDataTransferConnectionParams::Unmarshalling(Parcel &parcel)
{
    NearlinkSleDataTransferConnectionParams *params = new (std::nothrow) NearlinkSleDataTransferConnectionParams();
    if (params != nullptr && !params->ReadFromParcel(parcel)) {
        delete params;
        params = nullptr;
    }
    return params;
}

bool NearlinkSleDataTransferConnectionParams::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkSleDataTransferConnectionParams::ReadFromParcel(Parcel &parcel)
{
    NL_CHECK_RETURN_RET(parcel.ReadString(address_), false, "read address_ failed");
    NL_CHECK_RETURN_RET(parcel.ReadString(uuid_), false, "read uuid_ failed");
    NL_CHECK_RETURN_RET(parcel.ReadUint16(port_), false, "read port_ failed");
    NL_CHECK_RETURN_RET(parcel.ReadInt32(state_), false, "read state_ failed");
    NL_CHECK_RETURN_RET(parcel.ReadUint8(transMode_), false, "read transMode_ failed");
    NL_CHECK_RETURN_RET(parcel.ReadUint8(frameType_), false, "read frameType_ failed");
    NL_CHECK_RETURN_RET(parcel.ReadUint16(mtu_), false, "read mtu_ failed");
    return true;
}
}  // namespace OHOS::Nearlink
