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

#include "nearlink_dft_manager_c.h"

#include "log.h"
#include "def.h"
#include "interface_dft_manager.h"
#include "nearlink_dft_manager.h"

using namespace OHOS::Nearlink;

void DftManagerStart(void)
{
    HILOGI("DftManagerStart");
    auto mgr = InterfaceDftManager::GetInstance();
    if (mgr) {
        mgr->Start();
    }
}

void DftManagerStop(void)
{
    HILOGI("DftManagerStop");
    auto mgr = InterfaceDftManager::GetInstance();
    if (mgr) {
        mgr->Stop();
    }
}

void DftManagerReport(DftEventEnum eventId, const DftParamC params[], size_t size)
{
    if (!IsValidExcep(eventId) || size > OHOS::HiviewDFX::MAX_PARAM_NUMBER) {
        HILOGE("invalid excep id=%{public}d size=%{public}lu", eventId, size);
        return;
    }
    HILOGD("DftManagerReport id=%{public}d size=%{public}lu", eventId, size);
    auto set = DftParam::ConvertToParamSet(eventId, params, size);
}

void DftManagerCache(DftEventEnum eventId, const DftParamC params[], size_t size)
{
    if (!params || size == 0 || size > OHOS::HiviewDFX::MAX_PARAM_NUMBER) {
        HILOGE("no param to cache id=%{public}d size=%{public}lu", eventId, size);
        return;
    }
    if (!IsValidEvent(eventId)) {
        HILOGE("invalid id=%{public}d", eventId);
        return;
    }
    HILOGD("DftManagerCache id=%{public}d size=%{public}lu", eventId, size);
    auto set = DftParam::ConvertToParamSet(eventId, params, size);
    auto mgr = InterfaceDftManager::GetInstance();
    if (mgr) {
        mgr->Cache(eventId, set);
    }
}

void DftManagerEraseCache(DftEventEnum eventId, const DftParamC keyParams[], size_t size)
{
    if (!keyParams || size == 0 || size > OHOS::HiviewDFX::MAX_PARAM_NUMBER) {
        HILOGE("no keyParam to erase id=%{public}d size=%{public}lu", eventId, size);
        return;
    }
    if (!IsValidExcep(eventId)) {
        HILOGE("invalid id=%{public}d", eventId);
        return;
    }
    HILOGD("DftManagerEraseCache id=%{public}d size=%{public}lu", eventId, size);
    auto set = DftParam::ConvertToParamSet(eventId, keyParams, size);
    auto mgr = NearlinkDftManager::GetInstance();
    if (mgr) {
        mgr->EraseCache(eventId, set);
    }
}