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

#include "nearlink_sle_controller.h"
#include "nearlink_base_sle_controller.h"

namespace OHOS {
namespace Nearlink {

NearlinkSleController &NearlinkSleController::GetInstance()
{
    static NearlinkSleController instance;
    return instance;
}

NlErrCode NearlinkSleController::SetSleCoexParam(SLEBitRate maxBitRate, SledutyCycle dutyCycle)
{
    return impl_.SetSleCoexParam(maxBitRate, dutyCycle);
}

NlErrCode NearlinkSleController::UpdateConnectInterval(
    const std::string &device, ConnectionInterval intervalType) const
{
    return impl_.UpdateConnectInterval(device, intervalType);
}

NlErrCode NearlinkSleController::SetSleCoexMode(SleCoexMode mode, const std::vector<std::string> &deviceList,
    const std::vector<ConnectionInterval> &paramList)
{
    return impl_.SetSleCoexMode(mode, deviceList, paramList);
}

NearlinkSleController::NearlinkSleController() : impl_(NearlinkBaseSleController::GetInstance())
{
}

NearlinkSleController::~NearlinkSleController()
{
}

}  // namespace Nearlink
}  // namespace OHOS
