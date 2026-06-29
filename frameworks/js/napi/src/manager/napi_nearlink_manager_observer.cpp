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
#include "napi_nearlink_manager_observer.h"

#include "log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include <uv.h>

namespace OHOS {
namespace Nearlink {

NapiNearlinkManagerObserver::NapiNearlinkManagerObserver()
    : eventSubscribe(REGISTER_NEARLINK_STATE_CHANGE_TYPE, NL_MODULE_NAME)
{}

void NapiNearlinkManagerObserver::OnStateChanged(const int transport, const int status)
{
    HILOGD("start");
    auto napiNative = std::make_shared<NapiNativeInt>(status);
    eventSubscribe.PublishEvent(REGISTER_NEARLINK_STATE_CHANGE_TYPE, napiNative);
}
}  // namespace Nearlink
}  // namespace OHOS