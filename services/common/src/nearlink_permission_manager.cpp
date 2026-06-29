/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "nearlink_permission_manager.h"

#include "ipc_skeleton.h"
#include "tokenid_kit.h"
#include "privacy_kit.h"
#include "log.h"
#include "nearlink_errorcode.h"
#include "system_ability_definition.h"
#include "bundle_mgr_proxy.h"
#include "iservice_registry.h"
#include "accesstoken_kit.h"

namespace OHOS {
namespace Nearlink {

using namespace OHOS;
using namespace Security::AccessToken;
namespace {
#ifdef PERMISSION_ALWAYS_GRANT
bool g_permissionAlwaysGrant = true;
#else
bool g_permissionAlwaysGrant = false;
#endif
}

bool NearLinkPermissionManager::VerifyPermission(const std::string &permissionName)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    return VerifyPermission(permissionName, tokenId);
}

bool NearLinkPermissionManager::IsNeedAddPermissionUsedRecord(const std::string &permissionName,
    const uint32_t &callerToken)
{
    if ((permissionName.compare("ohos.permission.ACCESS_NEARLINK") == 0) && (IsHapCaller(callerToken))) {
        return true;
    }
    return false;
}

bool NearLinkPermissionManager::VerifyPermission(const std::string &permissionName, const uint32_t &callerToken)
{
    if (g_permissionAlwaysGrant) {
        HILOGW("Permission is granted.");
        return true;
    }

    int result = AccessTokenKit::VerifyAccessToken(callerToken, permissionName);
    if (result == PermissionState::PERMISSION_GRANTED) {
        if (IsNeedAddPermissionUsedRecord(permissionName, callerToken)) {
            int ret = PrivacyKit::AddPermissionUsedRecord(callerToken, permissionName, 1, 0);
            if (ret != 0) {
                HILOGE("AddPermissionUsedRecord falied, ret is %{public}d", ret);
            }
        }
        return true;
    } else {
        HILOGD("[PERMISSION] missing (%{public}s)", permissionName.c_str());
        if (IsNeedAddPermissionUsedRecord(permissionName, callerToken)) {
            int ret = PrivacyKit::AddPermissionUsedRecord(callerToken, permissionName, 0, 1);
            if (ret != 0) {
                HILOGE("AddPermissionUsedRecord falied, ret is %{public}d", ret);
            }
        }
        return false;
    }
}

bool NearLinkPermissionManager::IsPermissionsGranted(const std::set<std::string> &permissions)
{
    for (auto &it : permissions) {
        NL_CHECK_RETURN_RET(VerifyPermission(it), false,
            "[PERMISSION] check permission failed, permission(%{public}s), callingName(%{public}s)",
            it.c_str(), GetCallingName().c_str());
    }
    return true;
}

int32_t NearLinkPermissionManager::VerifyMultiPermissions(const std::shared_ptr<NearLinkPermissionItem> &item)
{
    if (g_permissionAlwaysGrant) {
        HILOGW("Permission is granted.");
        return NL_NO_ERROR;
    }

    if (item == nullptr) {
        return NL_NO_ERROR;
    }

    std::set<std::string> perm = item->GetPermissions();
    // Check whether the caller is a system HAP.
    if (item->SystemHapNeeded() && !CheckSystemPermission()) {
        HILOGE("[PERMISSION] system hap needed.");
        return NL_ERR_SYSTEM_PERMISSION_FAILED;
    }

    // Check permissions.
    NL_CHECK_RETURN_RET(IsPermissionsGranted(perm), NL_ERR_PERMISSION_FAILED,
        "[PERMISSION] check permissions failed.");

    return NL_NO_ERROR;
}

int32_t NearLinkPermissionManager::GetNearlinkApiVersion()
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    return GetNearlinkApiVersion(tokenId);
}

int32_t NearLinkPermissionManager::GetNearlinkApiVersion(uint32_t tokenId)
{
    if (!IsHapCaller(tokenId)) {
        return API_VERSION_INVALID;
    }
    HapTokenInfo hapTokenInfo;
    if (AccessTokenKit::GetHapTokenInfo(tokenId, hapTokenInfo) != AccessTokenKitRet::RET_SUCCESS) {
        HILOGE("[PERMISSION] GetHapTokenInfo failed.");
        return API_VERSION_INVALID;
    }
    HILOGD("[PERMISSION] apiVersion(%{public}d).", hapTokenInfo.apiVersion);
    return hapTokenInfo.apiVersion;
}

std::string NearLinkPermissionManager::GetCallingName()
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    return GetCallingName(tokenId);
}

std::string NearLinkPermissionManager::GetCallingName(const uint32_t &tokenId)
{
    ATokenTypeEnum callingType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    switch (callingType) {
        case ATokenTypeEnum::TOKEN_HAP : {
            HapTokenInfo hapTokenInfo;
            if (AccessTokenKit::GetHapTokenInfo(tokenId, hapTokenInfo) == AccessTokenKitRet::RET_SUCCESS) {
                return hapTokenInfo.bundleName;
            }
            HILOGE("[PERMISSION] callingType(%{public}d), GetHapTokenInfo failed.", callingType);
            return "";
        }
        case ATokenTypeEnum::TOKEN_SHELL:
        case ATokenTypeEnum::TOKEN_NATIVE: {
            NativeTokenInfo nativeTokenInfo;
            if (AccessTokenKit::GetNativeTokenInfo(tokenId, nativeTokenInfo) == AccessTokenKitRet::RET_SUCCESS) {
                return nativeTokenInfo.processName;
            }
            HILOGE("[PERMISSION] callingType(%{public}d), GetNativeTokenInfo failed.", callingType);
            return "";
        }
        default:
            HILOGE("[PERMISSION] callingType(%{public}d) is invalid.", callingType);
            return "";
    }
}

bool NearLinkPermissionManager::IsHapCaller(void)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    return IsHapCaller(tokenId);
}

bool NearLinkPermissionManager::IsHapCaller(uint32_t tokenId)
{
    ATokenTypeEnum callingType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (callingType == ATokenTypeEnum::TOKEN_HAP) {
        return true;
    }
    return false;
}

bool NearLinkPermissionManager::IsNativeCaller(void)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    return IsNativeCaller(tokenId);
}

bool NearLinkPermissionManager::IsNativeCaller(const uint32_t &tokenId)
{
    ATokenTypeEnum callingType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (callingType == ATokenTypeEnum::TOKEN_NATIVE || callingType == ATokenTypeEnum::TOKEN_SHELL) {
        return true;
    }
    return false;
}

bool NearLinkPermissionManager::IsSystemHap()
{
    return IsSystemHap(IPCSkeleton::GetCallingFullTokenID());
}

bool NearLinkPermissionManager::IsSystemHap(uint64_t fullTokenId)
{
    bool isSystemApp = TokenIdKit::IsSystemAppByFullTokenID(fullTokenId);
    ATokenTypeEnum callingType = AccessTokenKit::GetTokenTypeFlag(static_cast<uint32_t>(fullTokenId));
    if (callingType == ATokenTypeEnum::TOKEN_HAP && isSystemApp) {
        return true;
    }
    return false;
}

bool NearLinkPermissionManager::CheckSystemPermission()
{
    return CheckSystemPermission(IPCSkeleton::GetCallingFullTokenID());
}

bool NearLinkPermissionManager::CheckSystemPermission(uint64_t fullTokenId)
{
    if (g_permissionAlwaysGrant) {
        HILOGW("Permission is granted.");
        return true;
    }

    bool isSystemApp = TokenIdKit::IsSystemAppByFullTokenID(fullTokenId);
    ATokenTypeEnum callingType = AccessTokenKit::GetTokenTypeFlag(static_cast<uint32_t>(fullTokenId));
    if (callingType == ATokenTypeEnum::TOKEN_NATIVE || callingType == ATokenTypeEnum::TOKEN_SHELL ||
        (callingType == ATokenTypeEnum::TOKEN_HAP && isSystemApp)) {
        return true;
    }
    return false;
}

bool NearLinkPermissionManager::IsUseRealAddr()
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    return IsUseRealAddr(tokenId);
}

bool NearLinkPermissionManager::IsUseRealAddr(const uint32_t &tokenId)
{
    return VerifyPermission(GET_NEARLINK_PEER_MAC, tokenId);
}

std::shared_ptr<NearLinkPermissionItem> NearLinkPermissionManager::CreateItem(bool isSystemApi,
    const std::set<std::string> permSet)
{
    return std::make_shared<NearLinkPermissionItem>(isSystemApi, permSet);
}

std::string NearLinkPermissionManager::GetHapAppIdentifier(int32_t userId, const std::string &bundleName)
{
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (!systemAbilityManager) {
        HILOGE("fail to get system ability mgr.");
        return "";
    }
    auto remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (!remoteObject) {
        HILOGE("fail to get bundle manager proxy.");
        return "";
    }
    sptr<AppExecFwk::IBundleMgr> bundleMgrProxy = iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
    if (bundleMgrProxy == nullptr) {
        HILOGE("Failed to get bundle manager proxy.");
        return "";
    }
    AppExecFwk::BundleInfo bundleInfo;
    ErrCode ret = bundleMgrProxy->GetBundleInfoV9(bundleName,
        static_cast<int>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_SIGNATURE_INFO), bundleInfo, userId);
    if (ret != ERR_OK) {
        HILOGE("failed to get bundle info for %{public}s due to errCode %{public}d", bundleName.c_str(), ret);
        return "";
    }
    return bundleInfo.signatureInfo.appIdentifier;
}

std::string NearLinkPermissionManager::GetHapAppId(uint32_t tokenId)
{
    Security::AccessToken::HapTokenInfo hapTokenInfo;
    int accessRet = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, hapTokenInfo);
    if (accessRet != 0) {
        HILOGD("Failed to get HapTokenInfo, This is not a HAP application.");
        return "";
    }
    int32_t userId = hapTokenInfo.userID;
    std::string bundleName = hapTokenInfo.bundleName;
    std::string appId = GetHapAppIdentifier(userId, bundleName);
    return appId;
}

std::string NearLinkPermissionManager::GetHapBundleName(uint32_t tokenId)
{
    Security::AccessToken::HapTokenInfo hapTokenInfo;
    int accessRet = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, hapTokenInfo);
    if (accessRet != 0) {
        HILOGD("Failed to get HapTokenInfo, This is not a HAP application.");
        return "";
    }
    return hapTokenInfo.bundleName;
}
}  // namespace Nearlink
}  // namespace OHOS