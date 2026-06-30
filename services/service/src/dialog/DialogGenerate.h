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

#ifndef DIALOG_GENERATE_H
#define DIALOG_GENERATE_H

#include <string>
#include "common_event_manager.h"
#include "common_event_subscriber.h"
#include "DialogUtils.h"
#include "nearlink_safe_hashmap.h"

namespace OHOS {
namespace Nearlink {

class DialogGenerate {
public:
    static DialogGenerate &GetInstance();
    bool DialogConnectExtension(const std::string commandStr, const std::string bundleName,
        const std::string abilityName);

private:
    bool DialogConnectExtensionAbility(const AAFwk::Want &want, const std::string commandStr,
        const std::string bundleName, const std::string abilityName);
    void DialogDisconnectExtensionAbility(std::string abilityName);

    NearlinkSafeHashMap<std::string,  sptr<NearlinkAbilityConnection>> abilityConnectionHashMap;
};
} // namespace Nearlink
} // namespace OHOS

#endif // DIALOG_GENERATE_H