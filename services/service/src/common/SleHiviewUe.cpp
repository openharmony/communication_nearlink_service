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

#include "SleHiviewUe.h"
#include "nearlink_datashare_helper.h"
#include "nearlink_dft_manager_c.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

SleHiviewUe::SleHiviewUe()
{}

SleHiviewUe::~SleHiviewUe()
{}

SleHiviewUe &SleHiviewUe::GetInstance()
{
    static SleHiviewUe instance;
    return instance;
}

void SleHiviewUe::Init()
{
    HILOGI("enter");
    isHiviewUeOn_.store(GetHiviewUeStateFromDataShare());
    DftHiviewUeStateChanged(isHiviewUeOn_.load());
    HILOGI("Init:%{public}d", isHiviewUeOn_.load());
    NL_CHECK_RETURN(NearlinkDataShareHelper::GetInstance().RegisterHiviewUeChangeObserver(),
        "RegisterHiviewUeChangeObserver() failed");
}

bool SleHiviewUe::GetHiviewUeStateFromDataShare() const
{
    return NearlinkDataShareHelper::GetInstance().GetHiviewUeState();
}

void SleHiviewUe::OnHiviewUeStateChanged(bool isHiviewUeOn)
{
    HILOGI("isHiviewUeOn: %{public}d", isHiviewUeOn);
    if (isHiviewUeOn_.load() == isHiviewUeOn) {
        HILOGI("HiviewUe state is unchanged");
        return;
    }
    isHiviewUeOn_.store(isHiviewUeOn);
    DftHiviewUeStateChanged(isHiviewUeOn_.load());
}

void SleHiviewUeChangeObserver::OnChange()
{
    bool isHiviewUeOn = NearlinkDataShareHelper::GetInstance().GetHiviewUeState();
    HILOGI("OnChange:%{public}d", isHiviewUeOn);
    SleHiviewUe::GetInstance().OnHiviewUeStateChanged(isHiviewUeOn);
}

} // namespace Nearlink
} // namespace OHOS