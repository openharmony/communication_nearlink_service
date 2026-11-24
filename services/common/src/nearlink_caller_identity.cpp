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

#include "nearlink_caller_identity.h"

#include "ipc_skeleton.h"
#include "nearlink_permission_manager.h"

namespace OHOS {
namespace Nearlink {

CallerIdentity CallerIdentity::Capture()
{
    CallerIdentity caller;
    caller.pid = IPCSkeleton::GetCallingPid();
    caller.uid = IPCSkeleton::GetCallingUid();
    caller.tokenId = IPCSkeleton::GetCallingTokenID();
    return caller;
}

bool CallerIdentity::IsNative() const
{
    if (nativeFlag_ < 0) {
        nativeFlag_ = NearLinkPermissionManager::IsNativeCaller(tokenId) ? 1 : 0;
    }
    return nativeFlag_ == 1;
}

uint64_t CallerIdentity::GetFullTokenId() const
{
    if (fullTokenFlag_ < 0) {
        fullTokenId_ = IPCSkeleton::GetCallingFullTokenID();
        fullTokenFlag_ = 1;
    }
    return fullTokenId_;
}

bool CallerIdentity::IsSystemHap() const
{
    if (systemHapFlag_ < 0) {
        systemHapFlag_ = NearLinkPermissionManager::IsSystemHap(GetFullTokenId()) ? 1 : 0;
    }
    return systemHapFlag_ == 1;
}

} // namespace Nearlink
} // namespace OHOS
