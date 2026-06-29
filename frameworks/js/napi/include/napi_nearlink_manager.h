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

#ifndef NAPI_NEARLINK_MANAGER_H_
#define NAPI_NEARLINK_MANAGER_H_

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Nearlink {
class NapiNearlinkManager {
public:
    static napi_value DefineManagerJSFunction(napi_env env, napi_value exports);
    static napi_value Enable(napi_env env, napi_callback_info info);
    static napi_value Disable(napi_env env, napi_callback_info info);
    static napi_value GetState(napi_env env, napi_callback_info info);
    static napi_value OnStateChange(napi_env env, napi_callback_info info);
    static napi_value OffStateChange(napi_env env, napi_callback_info info);
    static napi_value GetLocalAddress(napi_env env, napi_callback_info info);
    static napi_value GetLocalName(napi_env env, napi_callback_info info);
    static napi_value SetLocalName(napi_env env, napi_callback_info info);
    static napi_value GetPairedDevices(napi_env env, napi_callback_info info);
    static napi_value SetConnectionMode(napi_env env, napi_callback_info info);
    static napi_value FactoryReset(napi_env env, napi_callback_info info);
    static napi_value IsNearLinkSupported(napi_env env, napi_callback_info info);
private:
    static napi_value ManagerPropertyValueInit(napi_env env, napi_value exports);
    static napi_value TsNearlinkStateValueInit(napi_env env);
    static napi_value ConnectionModeInit(napi_env env);
    static void RegisterManagerObserverToHost();
};
}  // namespace Nearlink
}  // namespace OHOS
#endif /* NAPI_NEARLINK_MANAGER_H_ */