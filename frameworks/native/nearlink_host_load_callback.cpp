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

#include "nearlink_host_load_callback.h"
#include "nearlink_base_host_load_callback.h"

namespace OHOS {
namespace Nearlink {

namespace {
inline NearlinkBaseHostLoadCallBack *ToBase(NearlinkHostLoadCallBack *self)
{
    return reinterpret_cast<NearlinkBaseHostLoadCallBack *>(self);
}
}
void NearlinkHostLoadCallBack::OnLoadSystemAbilitySuccess(
    int32_t systemAbilityId, const sptr<IRemoteObject> &remoteObject)
{
    ToBase(this)->NearlinkBaseHostLoadCallBack::OnLoadSystemAbilitySuccess(systemAbilityId, remoteObject);
}

void NearlinkHostLoadCallBack::OnLoadSystemAbilityFail(int32_t systemAbilityId)
{
    ToBase(this)->NearlinkBaseHostLoadCallBack::OnLoadSystemAbilityFail(systemAbilityId);
}

}  // namespace Nearlink
}  // namespace OHOS
