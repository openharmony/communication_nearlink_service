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

#include "DialogPairing.h"
#include "log.h"
#include "log_util.h"
#include "DialogGenerate.h"
#include "ServiceManagerPluginLoader.h"
#include "refbase.h"
#include "ability_connection.h"
#include "ability_connect_callback.h"
#include "extension_manager_client.h"
#include "int_wrapper.h"
#include "string_wrapper.h"
#include "want_params_wrapper.h"
#include "bool_wrapper.h"
#include "double_wrapper.h"
#include "cJSON.h"

namespace OHOS {
namespace Nearlink {
bool DialogPairing::PullUpPairingDialog(const RawAddress &device, const std::string &passKey, int type)
{
    HILOGI("Pairing dialog");
    const std::string abilityName = "NearlinkPairDialog";
    const std::string bundleName =
        ServiceManagerPluginInterface::GetInstance()->GetBundleName(BundleNameType::BUNDLE_NAME_SETTINGS);
    std::string connectStr = BuildStartCommand(device, passKey, type);
    NL_CHECK_RETURN_RET(DialogGenerate::GetInstance().DialogConnectExtension(connectStr, bundleName, abilityName),
                        false, "failed to connect dialog.");
    return true;
}

std::string DialogPairing::BuildStartCommand(const RawAddress &device, const std::string &passKey, int type)
{
    cJSON *root = cJSON_CreateObject();
    NL_CHECK_RETURN_RET(root, "", "Failed to create cJSON object.");
    std::string uiType = "sysDialog/common";
    cJSON_AddItemToObject(root, "ability.want.params.uiExtensionType", cJSON_CreateString(uiType.c_str()));
    cJSON_AddItemToObject(root, "deviceId", cJSON_CreateString(device.GetAddress().c_str()));
    cJSON_AddItemToObject(root, "pinCode", cJSON_CreateString(passKey.c_str()));
    cJSON_AddItemToObject(root, "pinType", cJSON_CreateNumber(type));
    HILOGI("address: %{public}s, type: %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), type);
    char* cmdData = cJSON_Print(root);
    if (cmdData == nullptr) {
        HILOGE("cJSON_Print error.");
        cJSON_Delete(root);
        return "";
    }
    std::string result = std::string(cmdData);
    cJSON_free(cmdData);
    cJSON_Delete(root);
    return result;
}
} // namespace Nearlink
} // namespace OHOS