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

#include "nearlink_sa_manager.h"
#include "nearlink_base_sa_manager.h"

namespace OHOS {
namespace Nearlink {

namespace {
inline NearlinkBaseSaManager *ToBase(NearlinkSaManager *self)
{
    return reinterpret_cast<NearlinkBaseSaManager *>(self);
}
}
NearlinkSaManager &NearlinkSaManager::GetInstance()
{
    return reinterpret_cast<NearlinkSaManager&>(NearlinkBaseSaManager::GetInstance());
}

sptr<IRemoteObject> NearlinkSaManager::GetRemoteProfile(const std::string &profileName)
{
    return ToBase(this)->GetRemoteProfile(profileName);
}

int32_t NearlinkSaManager::RegisterFunc(std::shared_ptr<NearlinkRegisterInfo> info)
{
    return ToBase(this)->RegisterFunc(info);
}

void NearlinkSaManager::DeregisterFunc(int32_t profileId)
{
    ToBase(this)->DeregisterFunc(profileId);
}

void NearlinkSaManager::OnStateChanged(const int transport, const int status)
{
    ToBase(this)->OnStateChanged(transport, status);
}

NearlinkSaManager::NearlinkSaManager()
{
}

NearlinkSaManager::~NearlinkSaManager()
{
}

}  // namespace Nearlink
}  // namespace OHOS
