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
#ifndef NAPI_NEARLINK_ERROR_H_
#define NAPI_NEARLINK_ERROR_H_


#include <cstdint>
#include <string>
#include "napi/native_api.h"
#include "log.h"
namespace OHOS {
namespace Nearlink {

#ifndef NAPI_NL_ASSERT_RETURN
#define NAPI_NL_ASSERT_RETURN(env, cond, errCode, retObj)                      \
do {                                                                           \
    if (!(cond)) {                                                             \
        HandleSyncErr((env), (errCode));                                       \
        HILOGE("nearlink napi assert failed, errCode(%{public}d)", (errCode)); \
        return (retObj);                                                       \
    }                                                                          \
} while (0)
#endif

#define NAPI_NL_ASSERT_RETURN_UNDEF(env, cond, errCode)   \
do {                                                      \
    napi_value res = nullptr;                             \
    napi_get_undefined((env), &res);                      \
    NAPI_NL_ASSERT_RETURN((env), (cond), (errCode), res); \
} while (0)

#define NAPI_NL_ASSERT_RETURN_FALSE(env, cond, errCode)   \
do {                                                      \
    napi_value res = nullptr;                             \
    napi_get_boolean((env), false, &res);                 \
    NAPI_NL_ASSERT_RETURN((env), (cond), (errCode), res); \
} while (0)

std::string GetNapiErrMsg(const napi_env &env, const int32_t errCode);
void HandleSyncErr(const napi_env &env, int32_t errCode);
void ConvertNapiError(const napi_env &env, int32_t &errCode, std::string &errMsg);
} // namespace Nearlink
} // namespace OHOS
#endif