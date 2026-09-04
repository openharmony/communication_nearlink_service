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

#include "ani_nearlink_remote_device_rssi_observer.h"
#include "taihe_async_work.h"
#include "taihe_native_object.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

AniRemoteDeviceRssiObserver::AniRemoteDeviceRssiObserver()
{
}

std::shared_ptr<AniRemoteDeviceRssiObserver> AniRemoteDeviceRssiObserver::GetInstance(void)
{
    static std::shared_ptr<AniRemoteDeviceRssiObserver> instance = std::make_shared<AniRemoteDeviceRssiObserver>();
    return instance;
}

void AniRemoteDeviceRssiObserver::OnReadRemoteRssiEvent(const NearlinkRemoteDevice &device, int rssi, int status)
{
    auto taiheRssi = std::make_shared<TaiheNativeInt>(rssi);
    AsyncWorkCallFunction(asyncPromiseMap_, TaiheAsyncType::GET_REMOTE_DEVICE_RSSI, taiheRssi, status);
}
}  // namespace Nearlink
}  // namespace OHOS
