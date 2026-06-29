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

#ifndef ANI_NEARLINK_ERROR_H
#define ANI_NEARLINK_ERROR_H

#include <cstdint>
#include <string>
#include <optional>
#include "nearlink_errorcode.h"

namespace OHOS {
namespace Nearlink {
using namespace Nearlink;

#ifndef ANI_NL_ASSERT_RETURN
#define ANI_NL_ASSERT_RETURN(cond, errCode, retObj)     \
do {                                                      \
    if (!(cond)) {                                        \
        HandleSyncErr((errCode));                         \
        HILOGE("nearlink ani assert failed.");  \
        return (retObj);                                  \
    }                                                     \
} while (0)
#endif

#ifndef ANI_NL_ASSERT_RETURN_VOID
#define ANI_NL_ASSERT_RETURN_VOID(cond, errCode)        \
do {                                                      \
    if (!(cond)) {                                        \
        HandleSyncErr((errCode));                         \
        HILOGE("nearlink ani assert failed.");         \
        return;                                           \
    }                                                     \
} while (0)
#endif

std::string GetAniErrMsg(const int32_t errCode);
void HandleSyncErr(int32_t errCode);

} // namespace Nearlink
} // namespace OHOS
#endif // ANI_NEARLINK_ERROR_H