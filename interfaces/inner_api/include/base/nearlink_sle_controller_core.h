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

#ifndef NEARLINK_SLE_CONTROLLER_CORE_H
#define NEARLINK_SLE_CONTROLLER_CORE_H

#include "nearlink_sle_controller.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief base.so implementation class of framework SLE controller.
 *
 * Kept in sync with NearlinkSleController declared in nearlink_sle_controller.h.
 */
class NEARLINK_API NearlinkSleControllerCore {
public:
    static NearlinkSleControllerCore &GetInstance();

    NlErrCode SetSleCoexParam(SLEBitRate maxBitRate, SledutyCycle dutyCycle);
    NlErrCode UpdateConnectInterval(const std::string &device, ConnectionInterval intervalType) const;
    NlErrCode SetSleCoexMode(SleCoexMode mode, const std::vector<std::string> &deviceList,
        const std::vector<ConnectionInterval> &paramList);

private:
    NearlinkSleControllerCore();
    ~NearlinkSleControllerCore();
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkSleControllerCore);
    NEARLINK_DECLARE_IMPL();
};

} // namespace Nearlink
} // namespace OHOS

#endif  // NEARLINK_SLE_CONTROLLER_CORE_H
