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

#include "nearlink_permission_item.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

NearLinkPermissionItem::NearLinkPermissionItem(bool systemHapNeeded, const std::set<std::string> permissions)
{
    systemHapNeeded_ = systemHapNeeded;
    permissions_ = permissions;
}

NearLinkPermissionItem::~NearLinkPermissionItem()
{}

bool NearLinkPermissionItem::SystemHapNeeded()
{
    return systemHapNeeded_;
}

std::set<std::string> NearLinkPermissionItem::GetPermissions()
{
    return permissions_;
}
}  // namespace Nearlink
}  // namespace OHOS