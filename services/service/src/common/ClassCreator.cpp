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
#include "ClassCreator.h"

namespace OHOS {
namespace Nearlink {
std::map<std::string, ClassCreateFun> ClassFactory::registerInstance;

void ClassFactory::RegisterClass(const std::string &name, ClassCreateFun func)
{
    registerInstance[name] = func;
}

void *ClassFactory::NewInstance(const std::string &name)
{
    for (auto it = registerInstance.cbegin(); it != registerInstance.cend(); ++it) {
        if (it->first.compare(name) == 0) {
            return it->second();
        }
    }
    return nullptr;
}
}  // namespace Sle
}