/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef NEARLINK_DFT_MANAGER_H
#define NEARLINK_DFT_MANAGER_H

#include "interface_dft_manager.h"

namespace OHOS {
namespace Nearlink {
constexpr const int DEFAULT_APPEARANCE = 0xFFFF;
const std::string DEFAULT_DEVICE_NAME = "";
constexpr int32_t CHECK_TIME_INTERVAL_MS = 3000; // 3s
class NearlinkDftManager : public InterfaceDftManager {
public:
    static NearlinkDftManager *GetInstance(void);

    void Start(void) override;
    void Stop(void) override;
    void Report(DftEventEnum eventId, const ParamTypeSet &set) override;
    void Cache(DftEventEnum eventId, const ParamTypeSet &set) override;
    void EraseCache(DftEventEnum eventId, const ParamTypeSet &set);
private:
    NearlinkDftManager(void);

    struct impl;
    std::unique_ptr<impl> pimpl_;
    friend class DftRefParam;
};

}  // namespace Nearlink
}  // namespace OHOS

#endif // NEARLINK_DFT_MANAGER_H