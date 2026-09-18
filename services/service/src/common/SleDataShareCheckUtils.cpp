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

#include "SleSwitchDependency.h"

#include "datashare_helper.h"
#include "iservice_registry.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

constexpr const char *SETTINGS_DATASHARE_URI =
    "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";
constexpr const char *SETTINGS_DATASHARE_EXTENSION_URI = "datashare://com.ohos.settingsdata.DataAbility";

static std::pair<int, std::shared_ptr<DataShare::DataShareHelper>> CreateDataShareHelper()
{
    HILOGI("enter");
    sptr<ISystemAbilityManager> saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        HILOGE("GetSystemAbilityManager failed.");
        return std::make_pair(DataShare::E_DATA_SHARE_NOT_READY, nullptr);
    }
    sptr<IRemoteObject> remoteObj = saManager->GetSystemAbility(COMM_NET_CONN_MANAGER_SYS_ABILITY_ID);
    if (remoteObj == nullptr) {
        HILOGE("GetSystemAbility Service Failed.");
        return std::make_pair(DataShare::E_DATA_SHARE_NOT_READY, nullptr);
    }
    std::pair<int, std::shared_ptr<DataShare::DataShareHelper>> helperPair =
        DataShare::DataShareHelper::Create(remoteObj, SETTINGS_DATASHARE_URI, SETTINGS_DATASHARE_EXTENSION_URI);
    if (helperPair.first != DataShare::E_OK) {
        HILOGE("DataShareHelper create failed, ret: %{public}d", helperPair.first);
    }
    return helperPair;
}

bool SleDataShareCheckUtils::IsDataShareReady()
{
    auto [ret, _] = CreateDataShareHelper();
    return ret != DataShare::E_DATA_SHARE_NOT_READY;
}

}  // namespace Nearlink
}  // namespace OHOS
