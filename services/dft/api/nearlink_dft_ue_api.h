/*
 * Copyright (C) 2024-2026 Huawei Device Co., Ltd.
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

#ifndef NEARLINK_DFT_UE_API_H
#define NEARLINK_DFT_UE_API_H

namespace OHOS {
namespace Nearlink {

class RawAddress;

class NearlinkDftUe {
public:
    static NearlinkDftUe& GetInstance();
    void WriteAudioSourceDeviceUe(const RawAddress &curDevice, const RawAddress &oldDevice,
        int sceneCode, int subSceneCode);
private:
    NearlinkDftUe();
};

}   // namespace Nearlink
}   // namespace OHOS

#endif // NEARLINK_DFT_UE_API_H
