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
#ifndef SSAP_SERVICE_BASE_H
#define SSAP_SERVICE_BASE_H

#include <atomic>

namespace OHOS {
namespace Nearlink {

class SsapServiceBase {
public:
    bool InRunningState()
    {
        return runningState_.load();
    }

    void Start()
    {
        runningState_ = true;
    }

    void Stop()
    {
        runningState_ = false;
    }

private:
    std::atomic_bool runningState_ = false;
};

} // namespace Nearlink
} // namespace OHOS

#endif // SSAP_SERVICE_BASE_H