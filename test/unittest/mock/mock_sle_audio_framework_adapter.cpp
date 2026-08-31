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

#include "SleAudioFrameworkAdapter.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace {
    bool g_mockAudioServiceActivate = false;
    bool g_mockBtOut = false;
}

void SetMockAudioServiceActivate(bool isActivate)
{
    g_mockAudioServiceActivate = isActivate;
}

void SetMockBtOut(bool isBtOut)
{
    g_mockBtOut = isBtOut;
}

void ClearMockAudioFwkState()
{
    g_mockAudioServiceActivate = false;
    g_mockBtOut = false;
}

bool SleAudioFrameworkAdapter::IsAudioServiceActivate()
{
    HILOGI("[SleAudioFrameworkAdapter Mocker] IsAudioServiceActivate");
    return g_mockAudioServiceActivate;
}

bool SleAudioFrameworkAdapter::IsBtOut()
{
    HILOGI("[SleAudioFrameworkAdapter Mocker] IsBtOut");
    return g_mockBtOut;
}

} // namespace Nearlink
} // namespace OHOS
