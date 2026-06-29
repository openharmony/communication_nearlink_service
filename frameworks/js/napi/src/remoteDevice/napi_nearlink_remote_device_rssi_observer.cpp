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

#include "napi_nearlink_remote_device_rssi_observer.h"

#include "log.h"
#include "napi_async_work.h"
#include "napi_native_object.h"

namespace OHOS {
namespace Nearlink {

NapiRemoteDeviceRssiObserver::NapiRemoteDeviceRssiObserver()
    : eventSubscribe(NL_REMOTE_DEVICE_READ_REMOTE_RSSIEVENT, NL_MODULE_NAME)
{
}

std::shared_ptr<NapiRemoteDeviceRssiObserver> NapiRemoteDeviceRssiObserver::GetInstance(void)
{
    static std::shared_ptr<NapiRemoteDeviceRssiObserver> instance = std::make_shared<NapiRemoteDeviceRssiObserver>();
    return instance;
}

void NapiRemoteDeviceRssiObserver::OnReadRemoteRssiEvent(const NearlinkRemoteDevice &device, int rssi, int status)
{
    auto napiRssi = std::make_shared<NapiNativeInt>(rssi);
    AsyncWorkCallFunction(asyncPromiseMap_, NapiAsyncType::GET_REMOTE_DEVICE_RSSI, napiRssi, status);
}
}  // namespace Nearlink
}  // namespace OHOS
