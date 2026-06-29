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

#include "napi_async_callback.h"
#include "nearlink_errorcode.h"
#include "log.h"
#include "napi_nearlink_error.h"
#include "napi_ha_manager.h"
namespace OHOS {
namespace Nearlink {
void NapiAsyncCallback::CallFunction(int errCode, const std::shared_ptr<NapiNativeObject> &object)
{
    if (object == nullptr) {
        HILOGE("napi native object is nullptr");
        return;
    }

    if (deferred_) {
        if (errCode == NL_NO_ERROR) {
            napi_value val = object->ToNapiValue(env_);
            napi_resolve_deferred(env_, deferred_, val);
        } else {
            std::string errMsg = "";
            ConvertNapiError(env_, errCode, errMsg);
            napi_value eCode = nullptr;
            napi_create_int32(env_, errCode, &eCode);
            napi_value message = nullptr;
            napi_create_string_utf8(env_, errMsg.c_str(), NAPI_AUTO_LENGTH, &message);
            napi_value result = nullptr;
            napi_create_error(env_, eCode, message, &result);
            napi_reject_deferred(env_, deferred_, result);
        }
    } else {
        HILOGE("promise or deferred is nullptr");
        return;
    }
}

/*************************** env_cleanup_hook ********************************/
void NapiCallbackEnvCleanupHook(void *data)
{
    if (data == nullptr) {
        HILOGE("data is nullptr");
        return;
    }

    NapiCallback *callback = static_cast<NapiCallback *>(data);
    callback->SetNapiEnvValidity(false);
}
/*************************** env_cleanup_hook ********************************/

napi_value NapiAsyncCallback::GetPromiseRet(void)
{
        return promise_;
}

NapiCallback::NapiCallback(napi_env env, napi_value callback) : env_(env)
{
    auto status = napi_create_reference(env, callback, 1, &callbackRef_);
    if (status != napi_ok) {
        HILOGE("napi_create_reference failed, status: %{public}d", status);
    }
    // Used to clean up resources when env exit
    napi_add_env_cleanup_hook(env, NapiCallbackEnvCleanupHook, this);
}

NapiCallback::~NapiCallback()
{
    if (!IsValidNapiEnv()) {
        return;
    }
    auto status = napi_delete_reference(env_, callbackRef_);
    if (status != napi_ok) {
        HILOGE("napi_delete_reference failed, status: %{public}d", status);
    }
}

void NapiCallback::CallFunction(const std::shared_ptr<NapiNativeObject> &object)
{
    if (!IsValidNapiEnv()) {
        HILOGW("napi env is exit");
        return;
    }
    if (object == nullptr) {
        HILOGE("napi native object is nullptr");
        return;
    }

    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env_, &scope);
    if (scope == nullptr) {
        return;
    }
    napi_value callbackFunc = nullptr;
    if (callbackRef_ == nullptr) {
        HILOGE("napi calklback is nullptr");
        napi_close_handle_scope(env_, scope);
        return;
    }
    napi_get_reference_value(env_, callbackRef_, &callbackFunc);
    napi_value callResult = nullptr;
    napi_value val = object->ToNapiValue(env_);
    napi_call_function(env_, nullptr, callbackFunc, ARGS_SIZE_ONE, &val, &callResult);
    napi_close_handle_scope(env_, scope);
}

napi_env NapiCallback::GetNapiEnv(void)
{
    return env_;
}

bool NapiCallback::Equal(napi_env env, napi_value &callback) const
{
    if (!IsValidNapiEnv()) {
        HILOGW("napi env is exit");
        return false;
    }
    if (env != env_) {
        HILOGD("Callback is not in the same thread, not uqual");
        return false;
    }
    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env_, &scope);
    if (scope == nullptr) {
        HILOGE("scope is nullptr");
        return false;
    }
    napi_value storedCallback = nullptr;
    napi_get_reference_value(env_, callbackRef_, &storedCallback);

    bool isEqual = false;
    napi_strict_equals(env_, storedCallback, callback, &isEqual);
    napi_close_handle_scope(env_, scope);
    return isEqual;
}

}  // namespace Nearlink
}  // namespace OHOS
