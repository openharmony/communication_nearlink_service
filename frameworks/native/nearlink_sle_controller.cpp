/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "nearlink_sa_manager.h"
#include "log_util.h"
#include "i_nearlink_sle_controller.h"
#include "nearlink_host.h"
#include "nearlink_def.h"
#include "nearlink_utils.h"

namespace OHOS {
namespace Nearlink {

struct NearlinkSleController::impl {
    impl();
    ~impl();
    int32_t profileRegisterId_{0};
};

NearlinkSleController::impl::impl()
{
    HILOGI("start");
    std::shared_ptr<NearlinkRegisterInfo> info = std::make_shared<NearlinkRegisterInfo>(NEARLINK_SLE_CONTROLLER_SERVER);
    info->serviceStartedFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        HILOGI("service started");
    };

    profileRegisterId_ = NearlinkSaManager::GetInstance().RegisterFunc(info);
    if (profileRegisterId_ == INVALID_PROFILE_ID) {
        HILOGE("NearlinkSleController profileRegisterId_ is invalid");
    }
}

NearlinkSleController::impl::~impl()
{
    HILOGI("start");
    NearlinkSaManager::GetInstance().DeregisterFunc(profileRegisterId_);
}

NearlinkSleController::NearlinkSleController()
{
    if (pimpl == nullptr) {
        pimpl = std::make_unique<impl>();
    }
}

NearlinkSleController::~NearlinkSleController() {}

NearlinkSleController &NearlinkSleController::GetInstance()
{
    // C++11 static local variable initialization is thread-safe.
    static NearlinkSleController controller;
    return controller;
}

NlErrCode NearlinkSleController::SetSleCoexParam(SLEBitRate maxBitRate, SledutyCycle dutyCycle)
{
    HILOGD("enter, maxBitRate: %{public}u, dutyCycle: %{public}u",
           static_cast<uint16_t>(maxBitRate), static_cast<uint8_t>(dutyCycle));
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(),
        NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");

    sptr<INearlinkSleController> proxy = GetProxy<INearlinkSleController>(NEARLINK_SLE_CONTROLLER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->SetSleCoexParam(static_cast<uint16_t>(maxBitRate), static_cast<uint8_t>(dutyCycle));
}

NlErrCode NearlinkSleController::UpdateConnectInterval(const std::string &device, ConnectionInterval intervalType) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(),
                        NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    NL_CHECK_RETURN_RET(IsValidAddress(device), NL_ERR_INVALID_PARAM, "device invalid");

    sptr<INearlinkSleController> proxy = GetProxy<INearlinkSleController>(NEARLINK_SLE_CONTROLLER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    NlErrCode errCode = proxy->UpdateConnectInterval(device, static_cast<int32_t>(intervalType));
    NL_CHECK_RETURN_RET(errCode == NL_NO_ERROR, errCode,
        "UpdateConnectInterval failed, error code: %{public}d", errCode);
    HILOGI("intervalType = %{public}d", intervalType);
    return errCode;
}

} // namespace Nearlink
} // namespace OHOS
