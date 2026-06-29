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

#ifndef NAPI_NEARLINK_MANAGER_OBSERVER_H
#define NAPI_NEARLINK_MANAGER_OBSERVER_H

#include "nearlink_host.h"
#include "napi_async_callback.h"
#include "napi_nearlink_utils.h"
#include "napi_event_subscribe_module.h"
namespace OHOS {
namespace Nearlink {
const char *const REGISTER_NEARLINK_STATE_CHANGE_TYPE = "stateChange";
class NapiNearlinkManagerObserver : public NearlinkHostObserver {
public:
    NapiNearlinkManagerObserver();
    ~NapiNearlinkManagerObserver() override = default;
    void OnStateChanged(const int transport, const int status) override;
    NapiEventSubscribeModule eventSubscribe;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NAPI_NEARLINK_MANAGER_OBSERVER_H