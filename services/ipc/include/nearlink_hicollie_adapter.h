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

#ifndef OHOS_NEARLINK_STANDARD_HICOLLIE_ADAPTER_H
#define OHOS_NEARLINK_STANDARD_HICOLLIE_ADAPTER_H

#include <string>
#include "xcollie/xcollie_define.h"

namespace OHOS {
namespace Nearlink {
class NearlinkHicollieAdapter {
public:
    NearlinkHicollieAdapter(const std::string &tag, uint8_t ipcCode, unsigned int timeoutSeconds = 30,
        std::function<void(void *)> func = nullptr, void* arg = nullptr,
        unsigned int flag = HiviewDFX::XCOLLIE_FLAG_LOG | HiviewDFX::XCOLLIE_FLAG_RECOVERY);
    ~NearlinkHicollieAdapter();
    
    void CancelNearlinkHicollie();
 
private:
    int timerId_{-1};
    std::string tag_;
    bool isCanceled_{false};
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_HICOLLIE_ADAPTER_H