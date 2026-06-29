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
#include "nearlink_dft_ue.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

NearlinkDftUe& NearlinkDftUe::GetInstance()
{
    static NearlinkDftUe instance;
    return instance;
}

void NearlinkDftUe::WriteAudioControlUeAndExcep(const RawAddress &device, int sceneCode, int subSceneCode,
    uint16_t changeReason, NlErrCode controlResult)
{
    HILOGI("[NearlinkDftUe Mocker] WriteAudioControlUeAndExcep enter");
}

void NearlinkDftUe::WriteAudioSourceDeviceUe(const RawAddress &curDevice, const RawAddress &oldDevice, int sceneCode,
    int subSceneCode)
{
    HILOGI("[NearlinkDftUe Mocker] WriteAudioSourceDeviceUe enter");
}

}  // namespace Nearlink
}  // namespace OHOS