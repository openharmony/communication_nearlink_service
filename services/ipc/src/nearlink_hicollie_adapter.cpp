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

#include "nearlink_hicollie_adapter.h"
#include "log.h"
#include <string>
#include "xcollie/xcollie.h"

namespace OHOS {
namespace Nearlink {
NearlinkHicollieAdapter::NearlinkHicollieAdapter(const std::string &tag, uint8_t ipcCode, unsigned int timeoutSeconds,
        std::function<void(void *)> func, void* arg, unsigned int flag)
{
    tag_ = tag + " ipcCode: " + std::to_string(ipcCode);
    timerId_ = HiviewDFX::XCollie::GetInstance().SetTimer(tag_, timeoutSeconds, func, arg, flag);
    isCanceled_ = false;
}

NearlinkHicollieAdapter::~NearlinkHicollieAdapter()
{
    CancelNearlinkHicollie();
}

void NearlinkHicollieAdapter::CancelNearlinkHicollie()
{
    if (!isCanceled_) {
        HiviewDFX::XCollie::GetInstance().CancelTimer(timerId_);
        isCanceled_ = true;
    }
}
}  // namespace Nearlink
}  // namespace OHOS
