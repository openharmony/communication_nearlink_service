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

#include "securec.h"
#include "log.h"
#include "nearlink_asc_audio_control_result.h"

namespace OHOS {
namespace Nearlink {
bool NearlinkASCAudioControlResult::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt32(streamType_)) {
        return false;
    }
    if (!parcel.WriteInt32(cmd_)) {
        return false;
    }
    if (!parcel.WriteInt32(result_)) {
        return false;
    }
    return true;
}

NearlinkASCAudioControlResult *NearlinkASCAudioControlResult::Unmarshalling(Parcel &parcel)
{
    NearlinkASCAudioControlResult *result = new (std::nothrow) NearlinkASCAudioControlResult();
    if (result != nullptr && !result->ReadFromParcel(parcel)) {
        delete result;
        result = nullptr;
    }
    return result;
}

bool NearlinkASCAudioControlResult::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkASCAudioControlResult::ReadFromParcel(Parcel &parcel)
{
    if (!parcel.ReadInt32(streamType_)) {
        return false;
    }
    if (!parcel.ReadInt32(cmd_)) {
        return false;
    }
    if (!parcel.ReadInt32(result_)) {
        return false;
    }
    return true;
}

int NearlinkASCAudioControlResult::GetStreamType() const
{
    return streamType_;
}

void NearlinkASCAudioControlResult::SetStreamType(int streamType)
{
    streamType_ = streamType;
}

int NearlinkASCAudioControlResult::GetCmd() const
{
    return cmd_;
}

void NearlinkASCAudioControlResult::SetCmd(int cmd)
{
    cmd_ = cmd;
}

int NearlinkASCAudioControlResult::GetResult() const
{
    return result_;
}

void NearlinkASCAudioControlResult::SetResult(int result)
{
    result_ = result;
}
}  // namespace Nearlink
}  // namespace OHOS