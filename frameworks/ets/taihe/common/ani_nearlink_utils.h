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

#ifndef ANI_NEARLINK_UTILS_H_
#define ANI_NEARLINK_UTILS_H_

#include "log_util.h"

namespace OHOS {
namespace Nearlink {

enum TaiheStatus {
    TAIHE_OK = 0,
    TAIHE_INVALID_ARG,
    TAIHE_OBJECT_EXPECTED,
    TAIHE_STRING_EXPECTED,
    TAIHE_NAME_EXPECTED,
    TAIHE_FUNCTION_EXPECTED,
    TAIHE_NUMBER_EXPECTED,
    TAIHE_BOOLEAN_EXPECTED,
    TAIHE_ARRAY_EXPECTED,
    TAIHE_GENERIC_FAILURE,
    TAIHE_PENDING_EXCEPTION,
    TAIHE_CANCELLED,
    TAIHE_ESCAPE_CALLED_TWICE,
    TAIHE_HANDLE_SCOPE_MISMATCH,
    TAIHE_CALLBACK_SCOPE_MISMATCH,
    TAIHE_QUEUE_FULL,
    TAIHE_CLOSING,
    TAIHE_BIGINT_EXPECTED,
    TAIHE_DATE_EXPECTED,
    TAIHE_ARRAYBUFFER_EXPECTED,
    TAIHE_DETACHABLE_ARRAYBUFFER_EXPECTED,
    TAIHE_WOULD_DEADLOCK,  // unused
    TAIHE_CREATE_ARK_RUNTIME_TOO_MANY_ENVS = 22,
    TAIHE_CREATE_ARK_RUNTIME_ONLY_ONE_ENV_PER_THREAD = 23,
    TAIHE_DESTROY_ARK_RUNTIME_ENV_NOT_EXIST = 24
};

#ifndef TAIHE_NEARLINK_CALL_RETURN
#define TAIHE_NEARLINK_CALL_RETURN(func)                                          \
    do {                                                                   \
        TaiheStatus ret = (func);                                          \
        if (ret != TAIHE_OK) {                                              \
            HILOGE("api call function failed. ret:%{public}d", ret);      \
            return ret;                                                    \
        }                                                                  \
    } while (0)
#endif

#define TAIHE_NEARLINK_RETURN_IF(condition, msg, ret)              \
    do {                                                    \
        if ((condition)) {                                  \
            HILOGE(msg);                                    \
            return (ret);                                   \
        }                                                   \
    } while (0)

bool IsValidAddress(std::string addr);
bool CheckDeviceIdParam(std::string &addr);

} // namespace Nearlink
} // namespace OHOS
#endif // ANI_NEARLINK_UTILS_H_