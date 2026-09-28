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
#include "base/nearlink_sle_controller_core.h"

namespace OHOS {
namespace Nearlink {

namespace {
inline NearlinkSleControllerCore *ToCore(NearlinkSleController *self)
{
    return reinterpret_cast<NearlinkSleControllerCore *>(self);
}
inline const NearlinkSleControllerCore *ToCore(const NearlinkSleController *self)
{
    return reinterpret_cast<const NearlinkSleControllerCore *>(self);
}
}
NearlinkSleController &NearlinkSleController::GetInstance()
{
    return reinterpret_cast<NearlinkSleController&>(NearlinkSleControllerCore::GetInstance());
}

NlErrCode NearlinkSleController::SetSleCoexParam(SLEBitRate maxBitRate, SledutyCycle dutyCycle)
{
    return ToCore(this)->SetSleCoexParam(maxBitRate, dutyCycle);
}

NlErrCode NearlinkSleController::UpdateConnectInterval(
    const std::string &device, ConnectionInterval intervalType) const
{
    return ToCore(this)->UpdateConnectInterval(device, intervalType);
}

NlErrCode NearlinkSleController::SetSleCoexMode(SleCoexMode mode, const std::vector<std::string> &deviceList,
    const std::vector<ConnectionInterval> &paramList)
{
    return ToCore(this)->SetSleCoexMode(mode, deviceList, paramList);
}

// 镜像构造/析构保持空实现：对象实体由 base.so 中的 Core 类构造，
// 镜像入口通过 ToCore() cast 直调 Core 真身方法。
NearlinkSleController::NearlinkSleController()
{
}

NearlinkSleController::~NearlinkSleController()
{
}

}  // namespace Nearlink
}  // namespace OHOS
