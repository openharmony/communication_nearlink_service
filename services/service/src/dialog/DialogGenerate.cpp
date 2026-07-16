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
#include "DialogGenerate.h"
#include "ipc_skeleton.h"
#include "log.h"
#include "log_util.h"
#include "extension_manager_client.h"
#include "int_wrapper.h"
#include "string_wrapper.h"
#include "want_params_wrapper.h"
#include "bool_wrapper.h"
#include "double_wrapper.h"

namespace OHOS {
namespace Nearlink {

DialogGenerate &DialogGenerate::GetInstance()
{
    static DialogGenerate dialogDenerate;
    return dialogDenerate;
}

bool DialogGenerate::DialogConnectExtensionAbility(const AAFwk::Want &want, const std::string commandStr,
    const std::string bundleName, const std::string abilityName)
{
    DialogDisconnectExtensionAbility(abilityName);
    sptr<NearlinkAbilityConnection> connection = sptr<NearlinkAbilityConnection>::MakeSptr(commandStr, bundleName,
                                                                                            abilityName);
    NL_CHECK_RETURN_RET(connection != nullptr, false, "connection_ is nullptr.");
    abilityConnectionHashMap.EnsureInsert(abilityName, connection);
    HILOGI("calling pid is : %{public}d, calling uid is: %{public}d, fullTokenId:0x%{public}lx.",
        IPCSkeleton::GetCallingPid(), IPCSkeleton::GetCallingUid(), IPCSkeleton::GetCallingFullTokenID());
    std::string identity = IPCSkeleton::ResetCallingIdentity();
    auto ret = AAFwk::ExtensionManagerClient::GetInstance().ConnectServiceExtensionAbility(want, connection, nullptr,
                                                                                           -1);
    IPCSkeleton::SetCallingIdentity(identity);
    NL_CHECK_RETURN_RET(ret == ERR_OK, false, "ret isn't ERR_OK");
    return true;
}

bool DialogGenerate::DialogConnectExtension(const std::string commandStr,
    const std::string bundleName, const std::string abilityName)
{
    AAFwk::Want want;
    std::string sceneboardName = "com.ohos.sceneboard";
    std::string abilityNames = "com.ohos.sceneboard.systemdialog";
    want.SetElementName(sceneboardName, abilityNames);
    NL_CHECK_RETURN_RET(DialogConnectExtensionAbility(want, commandStr, bundleName, abilityName),
                        false, "ConnectExtensionAbility failed.");
    return true;
}

void DialogGenerate::DialogDisconnectExtensionAbility(std::string abilityName)
{ 
    auto func = [&abilityName](std::string temName, sptr<NearlinkAbilityConnection> temConnection) -> bool {
        if (temName == abilityName && temConnection != nullptr) {
            HILOGI("Dialog disconnect ability for %{public}s,", abilityName.c_str());
            AAFwk::ExtensionManagerClient::GetInstance().DisconnectAbility(temConnection);
            return true;
        } else {
            return false;
        }
    };
    abilityConnectionHashMap.FindAndRmv(func);
}

} // namespace Nearlink
} // namespace OHOS