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

#include "ohos.nearlink.advertising.proj.hpp"
#include "ohos.nearlink.advertising.impl.hpp"
#include "ani_nearlink_advertising_callback.h"
#include "log_util.h"
#include "taihe_native_object.h"

namespace OHOS {
namespace Nearlink {

AniNearlinkAdvertisingCallback::AniNearlinkAdvertisingCallback()
    : eventSubscribe_(REGISTER_ADVERTISING_STATE_INFO_NAME, "nearlinkAdvertising_taihe")
{}

std::shared_ptr<AniNearlinkAdvertisingCallback> AniNearlinkAdvertisingCallback::GetInstance()
{
    static std::shared_ptr<AniNearlinkAdvertisingCallback> instance =
        std::make_shared<AniNearlinkAdvertisingCallback>();
    return instance;
}

void AniNearlinkAdvertisingCallback::OnStartResultEvent(int result, int advHandle)
{
    HILOGI("enter, result: %{public}d advHandle: %{public}d", result, advHandle);

    ::ohos::nearlink::advertising::AdvertisingStateChangeInfo taiheResult {
        advHandle,
        ::ohos::nearlink::advertising::AdvertisingState(ohos::nearlink::advertising::AdvertisingState::key_t::STARTED)
    };

    auto &module = eventSubscribe_.GetModule<void(::ohos::nearlink::advertising::AdvertisingStateChangeInfo const&)
            >(REGISTER_ADVERTISING_STATE_INFO_NAME);
    module.PublishEvent(taiheResult);
}

void AniNearlinkAdvertisingCallback::OnStopResultEvent(int result, int advHandle)
{
    HILOGI("enter, result: %{public}d advHandle: %{public}d", result, advHandle);

    ::ohos::nearlink::advertising::AdvertisingStateChangeInfo taiheResult {
        advHandle,
        ::ohos::nearlink::advertising::AdvertisingState(ohos::nearlink::advertising::AdvertisingState::key_t::STOPPED)
    };

    auto &module = eventSubscribe_.GetModule<void(::ohos::nearlink::advertising::AdvertisingStateChangeInfo const&)
            >(REGISTER_ADVERTISING_STATE_INFO_NAME);
    module.PublishEvent(taiheResult);
}

void AniNearlinkAdvertisingCallback::OnGetAdvHandleEvent(int result, int advHandle)
{
    HILOGI("enter, result: %{public}d advHandle: %{public}d", result, advHandle);
    auto taiheAdvHandle = std::make_shared<TaiheNativeInt>(advHandle);
    AsyncWorkCallFunction(asyncPromiseMap_, TaiheAsyncType::GET_ADVERTISING_HANDLE, taiheAdvHandle, result);
}
}  // namespace Nearlink
}  // namespace OHOS
