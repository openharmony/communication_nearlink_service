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

#ifndef NAPI_ASYNC_CALLBACK_H
#define NAPI_ASYNC_CALLBACK_H

#include <map>
#include <memory>
#include <mutex>
#include "napi/native_api.h"
#include "napi_async_work.h"
#include "napi_native_object.h"

namespace OHOS {
namespace Nearlink {
class NapiCallback;

struct NapiAsyncCallback {
    void CallFunction(int errCode, const std::shared_ptr<NapiNativeObject> &object);
    napi_value GetPromiseRet(void);
    napi_env env_;
    napi_deferred deferred_;
    napi_value promise_;
};

class NapiCallback {
public:
    NapiCallback(napi_env env, napi_value callback);
    ~NapiCallback();

    void CallFunction(const std::shared_ptr<NapiNativeObject> &object);
    napi_env GetNapiEnv(void);
    bool Equal(napi_env env, napi_value &callback) const;
    void SetNapiEnvValidity(bool isValid)
    {
        isValid_ = isValid;
    }
    bool IsValidNapiEnv(void) const
    {
        return isValid_;
    }

private:
    NapiCallback(const NapiCallback &) = delete;
    NapiCallback &operator=(const NapiCallback &) = delete;
    NapiCallback(NapiCallback &&) = delete;
    NapiCallback &operator=(NapiCallback &&) noexcept = delete;

    napi_env env_;
    napi_ref callbackRef_;
    /*************************** env_cleanup_hook ********************************/
    bool isValid_ = true;
    /*************************** env_cleanup_hook ********************************/
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NAPI_ASYNC_CALLBACK_H