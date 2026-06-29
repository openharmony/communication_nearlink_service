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

#ifndef NEARLINK_ASC_AUDIO_CONTROL_RESULT_H
#define NEARLINK_ASC_AUDIO_CONTROL_RESULT_H

#include "parcel.h"

namespace OHOS {
namespace Nearlink {
class NearlinkASCAudioControlResult : public Parcelable {
public:
    NearlinkASCAudioControlResult() = default;
    explicit NearlinkASCAudioControlResult(const NearlinkASCAudioControlResult &other)
    {}
    NearlinkASCAudioControlResult& operator = (const NearlinkASCAudioControlResult &other);
    ~NearlinkASCAudioControlResult() override = default;

    bool Marshalling(Parcel &parcel) const override;

    static NearlinkASCAudioControlResult *Unmarshalling(Parcel &parcel);

    bool WriteToParcel(Parcel &parcel);

    bool ReadFromParcel(Parcel &parcel);

    int GetStreamType() const;
    void SetStreamType(int streamType);
    int GetCmd() const;
    void SetCmd(int cmd);
    int GetResult() const;
    void SetResult(int result);
private:
    int streamType_ {0};
    int cmd_ {0};
    int result_ {0};
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // NEARLINK_ASC_AUDIO_CONTROL_RESULT_H