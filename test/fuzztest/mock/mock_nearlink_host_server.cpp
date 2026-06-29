/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "nearlink_host_server.h"
#include "nearlink_errorcode.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
NlErrCode NearlinkHostServer::IsSleEnabled(bool &isSleEnabled)
{
    isSleEnabled = true;
    HILOGI("mock isSleEnabled(%{public}d)", isSleEnabled);
    return NL_NO_ERROR;
}

NlErrCode NearlinkHostServer::IsSleAvailableToCaller(bool &isSleAvailable)
{
    isSleAvailable = true;
    HILOGI("mock isSleAvailable(%{public}d)", isSleAvailable);
    return NL_NO_ERROR;
}

}  // namespace Nearlink
}  // namespace OHOS