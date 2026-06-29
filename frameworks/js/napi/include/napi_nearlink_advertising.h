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
#ifndef NAPI_NEARLINK_PERIPHERAL_MANAGER_H_
#define NAPI_NEARLINK_PERIPHERAL_MANAGER_H_

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Nearlink {
class NapiNearlinkAdvertising {
public:
    static void DefineAdvertisingJSObject(napi_env env, napi_value exports);
    static napi_value StartAdvertising(napi_env env, napi_callback_info info);
    static napi_value StopAdvertising(napi_env env, napi_callback_info info);
    static napi_value OnAdvertisingStateChange(napi_env env, napi_callback_info info);
    static napi_value OffAdvertisingStateChange(napi_env env, napi_callback_info info);
private:
    static napi_value PropertyInit(napi_env env, napi_value exports);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif /* NAPI_NEARLINK_PERIPHERAL_MANAGER_H_ */
