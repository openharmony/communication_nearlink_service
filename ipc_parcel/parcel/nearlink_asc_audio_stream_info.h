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

#ifndef NEARLINK_ASC_AUDIO_STREAM_INFO_H
#define NEARLINK_ASC_AUDIO_STREAM_INFO_H

#include "parcel.h"
#include "nearlink_ASC_source.h"

namespace OHOS {
namespace Nearlink {
class NearlinkASCAudioStreamInfo : public Parcelable {
public:
    NearlinkASCAudioStreamInfo() = default;
    explicit NearlinkASCAudioStreamInfo(const NearlinkASCAudioStreamInfo &other)
    {}
    NearlinkASCAudioStreamInfo& operator = (const NearlinkASCAudioStreamInfo &other);
    ~NearlinkASCAudioStreamInfo() override = default;

    bool Marshalling(Parcel &parcel) const override;

    static NearlinkASCAudioStreamInfo *Unmarshalling(Parcel &parcel);

    bool WriteToParcel(Parcel &parcel);

    bool ReadFromParcel(Parcel &parcel);

    void GetStreamState(std::vector<struct AudioStreamInfo> &streamData) const;
    void SetStreamState(const std::vector<struct AudioStreamInfo> &streamData);
    void AddStreamState(AudioStreamType streamType, AudioStreamState streamState);
private:
    std::vector<struct AudioStreamInfo> streamInfo_ { };
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // NEARLINK_ASC_AUDIO_STREAM_INFO_H