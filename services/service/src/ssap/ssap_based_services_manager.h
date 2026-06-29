/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef SSAP_BASED_SERVICES_MANAGER_H
#define SSAP_BASED_SERVICES_MANAGER_H

#include <BaseDef.h>
#include "LisServer.h"
#include "ssap_server_service.h"
#include "ssap_log.h"

namespace OHOS {
namespace Nearlink {
class SsapBasedServicesManager {
public:
    SsapBasedServicesManager();
    ~SsapBasedServicesManager();

    void Enable() const;
    void Disable() const;

    SLE_DISALLOW_COPY_AND_ASSIGN(SsapBasedServicesManager);
};
} // namespace Sle
} // namespace OHOS
#endif // SSAP_BASED_SERVICES_MANAGER_H