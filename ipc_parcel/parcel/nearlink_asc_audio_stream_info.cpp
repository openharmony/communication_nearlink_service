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

#include "securec.h"
#include "nearlink_asc_audio_stream_info.h"

namespace OHOS {
namespace Nearlink {

namespace {
    const uint32_t NEARLINK_AUDIO_STREAM_TYPE_MAX_NUM = 32;
}

bool NearlinkASCAudioStreamInfo::Marshalling(Parcel &parcel) const
{
    uint32_t size = streamInfo_.size();
    parcel.WriteUint32(size);
    for (const auto &item : streamInfo_) {
        if (!parcel.WriteUint32(item.streamType)) {
            return false;
        }
        if (!parcel.WriteUint8(item.streamState)) {
            return false;
        }
    }
    return true;
}

NearlinkASCAudioStreamInfo *NearlinkASCAudioStreamInfo::Unmarshalling(Parcel &parcel)
{
    NearlinkASCAudioStreamInfo *result = new (std::nothrow) NearlinkASCAudioStreamInfo();
    if (result != nullptr && !result->ReadFromParcel(parcel)) {
        delete result;
        result = nullptr;
    }
    return result;
}

bool NearlinkASCAudioStreamInfo::WriteToParcel(Parcel &parcel)
{
    return Marshalling(parcel);
}

bool NearlinkASCAudioStreamInfo::ReadFromParcel(Parcel &parcel)
{
    uint32_t dataSize = 0;
    if (!parcel.ReadUint32(dataSize) || dataSize > NEARLINK_AUDIO_STREAM_TYPE_MAX_NUM) {
        return false;
    }
    for (uint32_t i = 0; i < dataSize; ++i) {
        uint32_t streamType;
        uint8_t streamState;
        if (!parcel.ReadUint32(streamType)) {
            return false;
        }
        if (!parcel.ReadUint8(streamState)) {
            return false;
        }
        struct AudioStreamInfo data = {};
        data.streamType = static_cast<AudioStreamType>(streamType);
        data.streamState = static_cast<AudioStreamState>(streamState);
        streamInfo_.push_back(data);
    }
    return true;
}

void NearlinkASCAudioStreamInfo::GetStreamState(std::vector<struct AudioStreamInfo> &streamData) const
{
    streamData.clear();
    for (const auto &item : streamInfo_) {
        streamData.push_back(item);
    }
}

void NearlinkASCAudioStreamInfo::SetStreamState(const std::vector<struct AudioStreamInfo> &streamData)
{
    streamInfo_.clear();
    for (const auto &item : streamData) {
        streamInfo_.push_back(item);
    }
}

void NearlinkASCAudioStreamInfo::AddStreamState(AudioStreamType streamType, AudioStreamState streamState)
{
    struct AudioStreamInfo data = {};
    data.streamType = streamType;
    data.streamState = streamState;
    streamInfo_.push_back(data);
}

}  // namespace Nearlink
}  // namespace OHOS