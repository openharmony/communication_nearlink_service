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

static std::map<std::string, ClassCreateFun> &GetRegisterInstance()
{
    static std::map<std::string, ClassCreateFun> instance;
    return instance;
}

void ClassFactory::RegisterClass(const std::string &name, ClassCreateFun func)
{
    GetRegisterInstance()[name] = func;
}

void *ClassFactory::NewInstance(const std::string &name)
{
    auto &reg = GetRegisterInstance();
    for (auto it = reg.cbegin(); it != reg.cend(); ++it) {
        if (it->first.compare(name) == 0) {
            return it->second();
        }
    }
    return nullptr;
}
}  // namespace Nearlink
}