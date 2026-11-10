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

// NearLinkPermissionManager 整替 stub：真实现经 AccessTokenKit/PrivacyKit 触达
// libaccesstoken_sdk（其 spm 符号在生产 .so 上不导出，测试二进制加载即失败）。
// stub 全授权、零 SDK 引用；测试进程已用 NearlinkAccessTokenMock 预置 native token 权限。
#include <set>
#include <string>

#include "nearlink_errorcode.h"
#include "nearlink_permission_manager.h"

namespace OHOS {
namespace Nearlink {

bool NearLinkPermissionManager::VerifyPermission(const std::string &permissionName)
{
    (void)permissionName;
    return true;
}

bool NearLinkPermissionManager::VerifyPermission(const std::string &permissionName, const uint32_t &callerToken)
{
    (void)permissionName;
    (void)callerToken;
    return true;
}

int32_t NearLinkPermissionManager::VerifyMultiPermissions(const std::shared_ptr<NearLinkPermissionItem> &item)
{
    (void)item;
    return NL_NO_ERROR;
}

int32_t NearLinkPermissionManager::GetNearlinkApiVersion()
{
    return 0;
}

int32_t NearLinkPermissionManager::GetNearlinkApiVersion(uint32_t tokenId)
{
    (void)tokenId;
    return 0;
}

std::string NearLinkPermissionManager::GetCallingName()
{
    return "nearlink_test";
}

std::string NearLinkPermissionManager::GetCallingName(const uint32_t &tokenId)
{
    (void)tokenId;
    return "nearlink_test";
}

bool NearLinkPermissionManager::IsHapCaller(void)
{
    return false;
}

bool NearLinkPermissionManager::IsHapCaller(uint32_t tokenId)
{
    (void)tokenId;
    return false;
}

bool NearLinkPermissionManager::IsNativeCaller(void)
{
    return true;
}

bool NearLinkPermissionManager::IsNativeCaller(const uint32_t &tokenId)
{
    (void)tokenId;
    return true;
}

bool NearLinkPermissionManager::CheckSystemPermission()
{
    return true;
}

bool NearLinkPermissionManager::CheckSystemPermission(uint64_t fullTokenId)
{
    (void)fullTokenId;
    return true;
}

bool NearLinkPermissionManager::IsSystemHap()
{
    return false;
}

bool NearLinkPermissionManager::IsSystemHap(uint64_t fullTokenId)
{
    (void)fullTokenId;
    return false;
}

bool NearLinkPermissionManager::IsUseRealAddr()
{
    return false;
}

bool NearLinkPermissionManager::IsUseRealAddr(const uint32_t &tokenId)
{
    (void)tokenId;
    return false;
}

std::shared_ptr<NearLinkPermissionItem> NearLinkPermissionManager::CreateItem(bool isSystemApi,
    const std::set<std::string> permSet)
{
    return std::make_shared<NearLinkPermissionItem>(isSystemApi, permSet);
}

std::string NearLinkPermissionManager::GetHapAppIdentifier(int32_t userId, const std::string &bundleName)
{
    (void)userId;
    (void)bundleName;
    return "";
}

std::string NearLinkPermissionManager::GetHapAppId(uint32_t tokenId)
{
    (void)tokenId;
    return "";
}

std::string NearLinkPermissionManager::GetHapBundleName(uint32_t tokenId)
{
    (void)tokenId;
    return "";
}

bool NearLinkPermissionManager::IsPermissionsGranted(const std::set<std::string> &permissions)
{
    (void)permissions;
    return true;
}

bool NearLinkPermissionManager::IsNeedAddPermissionUsedRecord(const std::string &permissionName,
    const uint32_t &callerToken)
{
    (void)permissionName;
    (void)callerToken;
    return false;
}
}  // namespace Nearlink
}  // namespace OHOS
