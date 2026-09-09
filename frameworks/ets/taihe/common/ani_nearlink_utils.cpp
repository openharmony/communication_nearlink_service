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

#ifndef LOG_TAG
#define LOG_TAG "nearlink_taihe_utils"
#endif

#include <algorithm>
#include <regex>

#include "ani_nearlink_utils.h"
#include "taihe_async_callback.h"
#include "ani_nearlink_error.h"

namespace OHOS {
namespace Nearlink {

ani_ref TaiheGetNull(ani_env *env)
{
    ani_ref nullRef = nullptr;
    ani_status status = ANI_ERROR;
    if (env == nullptr) {
        HILOGE("null env");
        return nullptr;
    }

    if ((status = env->GetNull(&nullRef)) != ANI_OK) {
        HILOGE("GetNull failed, status: %{public}d", status);
        return nullptr;
    }
    return nullRef;
}

ani_ref TaiheGetUndefined(ani_env *env)
{
    ani_ref undefinedRef = nullptr;
    ani_status status = ANI_ERROR;
    if (env == nullptr) {
        HILOGE("null env");
        return nullptr;
    }

    if ((status = env->GetUndefined(&undefinedRef)) != ANI_OK) {
        HILOGE("GetUndefined failed, status: %{public}d", status);
        return nullptr;
    }
    return undefinedRef;
}

static ani_ref CreateBusinessError(ani_env *env, int code, const std::string &msg)
{
    ani_class cls{};
    ani_method method{};
    ani_object obj = nullptr;
    ani_status status = ANI_ERROR;
    if (env == nullptr) {
        HILOGE("null env");
        return nullptr;
    }

    if ((status = env->FindClass("@ohos.base.BusinessError", &cls)) != ANI_OK) {
        HILOGE("FindClass failed %{public}d", status);
        return nullptr;
    }
    if ((status = env->Class_FindMethod(cls, "<ctor>", ":", &method)) != ANI_OK) {
        HILOGE("Class_FindMethod failed %{public}d", status);
        return nullptr;
    }
    if ((status = env->Object_New(cls, method, &obj)) != ANI_OK) {
        HILOGE("Object_New failed %{public}d", status);
        return nullptr;
    }

    ani_int errCode = static_cast<ani_int>(code);
    ani_string errMsg;
    if ((status = env->String_NewUTF8(msg.c_str(), msg.size(), &errMsg)) != ANI_OK) {
        HILOGE("String_NewUTF8 failed %{public}d", status);
        return nullptr;
    }

    if ((status = env->Object_SetPropertyByName_Int(obj, "code", errCode)) != ANI_OK) {
        HILOGE("Object_SetPropertyByName_Int failed %{public}d", status);
        return nullptr;
    }
    if ((status = env->Object_SetPropertyByName_Ref(obj, "message", errMsg)) != ANI_OK) {
        HILOGE("Object_SetPropertyByName_Ref failed %{public}d", status);
        return nullptr;
    }
    return reinterpret_cast<ani_ref>(obj);
}

ani_ref GetCallbackErrorValue(ani_env *env, int errCode)
{
    HILOGE("errCode: %{public}d", errCode);
    ani_ref result = TaiheGetNull(env);
    if (errCode == NL_NO_ERROR) {
        return result;
    }

    std::string errMsg = "";
    ConvertAniError(errCode, errMsg);
    result = CreateBusinessError(env, errCode, errMsg);
    return result;
}

bool IsValidAddr(const std::string &addr)
{
    const std::regex deviceIdRegex("^[0-9a-fA-F]{2}(:[0-9a-fA-F]{2}){5}$");
    return regex_match(addr, deviceIdRegex);
}

bool CheckDeviceIdParam(const std::string &addr)
{
    TAIHE_NEARLINK_RETURN_IF(!IsValidAddr(addr), "Invalid addr", false);
    return true;
}
} // namespace Nearlink
} // namespace OHOS