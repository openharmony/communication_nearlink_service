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
#include "nearlink_access_token_mock.h"

#include "log.h"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
namespace OHOS {
namespace Nearlink {
void NearlinkAccessTokenMock::SetNativeTokenInfo()
{
    constexpr int permissionNum = 5;
    const char *perms[permissionNum] = {
        "ohos.permission.MANAGE_NEARLINK",
        "ohos.permission.ACCESS_NEARLINK",
        "ohos.permission.GET_NEARLINK_LOCAL_MAC",
        "ohos.permission.GET_NEARLINK_PEER_MAC",
        "ohos.permission.MANAGE_SECURE_SETTINGS"
    };
    SetNativeTokenInfo(perms, permissionNum);
}

void NearlinkAccessTokenMock::SetNativeTokenInfo(const char **perms, int permissionNum)
{
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = permissionNum,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "nearlink_test",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    HILOGI("GetAccessTokenId tokenId(%{public}lu)", tokenId);
    int32_t ret = SetSelfTokenID(tokenId);
    HILOGI("SetSelfTokenID ret(%{public}d)", ret);
    ret = Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    HILOGI("ReloadNativeTokenInfo ret(%{public}d)", ret);
}
} // namespace OHOS
} // namespace Nearlink